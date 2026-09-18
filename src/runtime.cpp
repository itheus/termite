#include "runtime.h"

#include <QCoreApplication>
#include <QLocalServer>
#include <QLocalSocket>
#include <QProcess>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QDebug>
#include <QAction>

#include <KGlobalAccel>

#include "app.h"
#include "launchpath.h"

namespace {
const char *kServerName = "termite-runtime";

QString autostartDesktopPath()
{
    const QString config =
        QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    return QDir(config).filePath(QStringLiteral("autostart/termite.desktop"));
}
} // namespace

Runtime::Runtime(QObject *parent)
    : QObject(parent)
{
}

Runtime::~Runtime() = default;

bool Runtime::claimInstanceAndRun(const QString &request)
{
    {
        QLocalSocket probe;
        probe.connectToServer(QString::fromLatin1(kServerName));
        if (probe.waitForConnected(300)) {
            probe.write(request.toUtf8());
            probe.write("\n");
            probe.waitForBytesWritten(500);
            probe.waitForDisconnected(500);
            return false;
        }
    }

    m_isPrimary = true;

    m_server = new QLocalServer(this);
    QLocalServer::removeServer(QString::fromLatin1(kServerName));
    if (!m_server->listen(QString::fromLatin1(kServerName))) {
        qWarning() << "Termite: failed to start IPC server:" << m_server->errorString();
        return false;
    }
    connect(m_server, &QLocalServer::newConnection, this, &Runtime::onNewConnection);

    m_app = new App(this);
    applyAutostart();
    registerSettingsLauncher();

    qInfo() << "Termite runtime started (pid" << QCoreApplication::applicationPid() << ").";

    if (!request.isEmpty()) {
        QMetaObject::invokeMethod(this, [this, request]() { handleCommand(nullptr, request); },
                                  Qt::QueuedConnection);
    }

    return true;
}

void Runtime::onNewConnection()
{
    QLocalSocket *socket = m_server->nextPendingConnection();
    if (!socket) {
        return;
    }
    connect(socket, &QLocalSocket::readyRead, this, &Runtime::onClientReadyRead);
    connect(socket, &QLocalSocket::disconnected, socket, &QLocalSocket::deleteLater);
}

void Runtime::onClientReadyRead()
{
    auto *socket = qobject_cast<QLocalSocket *>(sender());
    if (!socket) {
        return;
    }
    while (socket->canReadLine()) {
        const QString command = QString::fromUtf8(socket->readLine()).trimmed();
        if (!command.isEmpty()) {
            handleCommand(socket, command);
        }
    }
}

void Runtime::handleCommand(QLocalSocket *socket, const QString &command)
{
    if (command == QStringLiteral("toggle") && m_app) {
        m_app->toggle();
        if (socket) {
            socket->write("ok\n");
        }
    } else if (command == QStringLiteral("show") && m_app) {
        m_app->showTerminal();
        if (socket) {
            socket->write("ok\n");
        }
    } else if (command == QStringLiteral("hide") && m_app) {
        m_app->hideTerminal();
        if (socket) {
            socket->write("ok\n");
        }
    } else if (command == QStringLiteral("status")) {
        if (socket) {
            socket->write("running\n");
        }
    } else if (command == QStringLiteral("stop")) {
        if (socket) {
            socket->write("ok\n");
        }
        stop();
    } else if (command == QStringLiteral("restart")) {
        if (socket) {
            socket->write("ok\n");
        }
        restart();
    } else if (command.startsWith(QStringLiteral("shortcut ")) && m_app) {
        const QString seqStr = command.mid(9).trimmed();
        const QKeySequence seq(seqStr);
        if (seq.isEmpty()) {
            if (socket) {
                socket->write("invalid\n");
            }
        } else if (m_app->setShortcut(seq)) {
            if (socket) {
                socket->write("ok\n");
            }
        } else {
            if (socket) {
                socket->write("conflict\n");
            }
        }
    } else {
        if (socket) {
            socket->write("unknown\n");
        }
    }
}

void Runtime::registerSettingsLauncher()
{
    m_settingsAction = new QAction(this);
    m_settingsAction->setObjectName(QStringLiteral("termite-open-settings"));
    m_settingsAction->setText(QStringLiteral("Open Termite Settings"));

    const QKeySequence seq(Qt::CTRL | Qt::META | Qt::Key_M);
    const QList<QKeySequence> sequences{seq};

    auto *accel = KGlobalAccel::self();
    const auto conflicts = KGlobalAccel::globalShortcutsByKey(seq, KGlobalAccel::Equal);
    if (!conflicts.isEmpty()) {
        KGlobalAccel::stealShortcutSystemwide(seq);
    }
    accel->setDefaultShortcut(m_settingsAction, sequences, KGlobalAccel::NoAutoloading);
    accel->setShortcut(m_settingsAction, sequences, KGlobalAccel::NoAutoloading);

    connect(m_settingsAction, &QAction::triggered, this, []() {
        QProcess::startDetached(termiteLaunchPath(), {QStringLiteral("--settings")});
    });
}

void Runtime::applyAutostart()
{
    const QString path = autostartDesktopPath();
    const bool want = m_app ? m_app->settings().autostart : false;

    if (want) {
        const QString launch = termiteLaunchPath();
        const QString execField = termiteRunsAsAppImage()
                                      ? QStringLiteral("\"%1\" --runtime").arg(launch)
                                      : QStringLiteral("\"%1\"").arg(launch);
        const QString contents = QStringLiteral(
            "[Desktop Entry]\n"
            "Type=Application\n"
            "Name=Termite\n"
            "Comment=A terminal that lives under your KDE desktop\n"
            "Exec=%1\n"
            "Icon=termite\n"
            "Terminal=false\n"
            "X-GNOME-Autostart-enabled=true\n"
            "X-KDE-autostart-enabled=true\n"
            "Categories=Utility;\n")
                                     .arg(execField);
        QDir().mkpath(QFileInfo(path).absolutePath());
        QFile f(path);
        if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            f.write(contents.toUtf8());
            f.close();
            qInfo() << "Termite: wrote autostart entry" << path;
        }
    } else {
        if (QFile::exists(path)) {
            QFile::remove(path);
            qInfo() << "Termite: removed autostart entry" << path;
        }
    }
}

void Runtime::stop()
{
    qInfo() << "Termite: stopping runtime.";
    QCoreApplication::quit();
}

void Runtime::restart()
{
    qInfo() << "Termite: restarting runtime.";
    const QString exe = termiteLaunchPath();
    const QStringList args = termiteRunsAsAppImage()
                                 ? QStringList{QStringLiteral("--runtime")}
                                 : QCoreApplication::arguments().mid(1);
    QProcess::startDetached(exe, args);
    QCoreApplication::quit();
}