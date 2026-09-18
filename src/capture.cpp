#include "capture.h"

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QStandardPaths>
#include <QDebug>

Capture::Capture(QObject *parent)
    : QObject(parent)
    , m_process(new QProcess(this))
{
    connect(m_process, &QProcess::finished, this, [this](int exitCode, QProcess::ExitStatus status) {
        const QString err = QString::fromLocal8Bit(m_process->readAllStandardError()).trimmed();
        const QString out = QString::fromLocal8Bit(m_process->readAllStandardOutput()).trimmed();
        Q_UNUSED(out);

        if (status != QProcess::NormalExit || exitCode != 0) {
            qWarning().noquote() << "Spectacle capture failed (exit" << exitCode << "):" << err;
            emit finished(QString());
            return;
        }

        if (!QFile::exists(m_requestedPath)) {
            qWarning().noquote() << "Spectacle reported success but no file was produced:"
                                 << m_requestedPath;
            if (!err.isEmpty()) {
                qWarning().noquote() << "stderr:" << err;
            }
            emit finished(QString());
            return;
        }

        emit finished(m_requestedPath);
    });
}

void Capture::capture()
{
    if (m_process->state() != QProcess::NotRunning) {
        qWarning() << "Capture already in progress; ignoring duplicate request.";
        return;
    }

    m_requestedPath = outputPath();
    ensureRuntimeDir();

    QFile::remove(m_requestedPath);

    // Background capture of the current monitor, no notification, no pointer.
    // -i spawns a private, non-DBus instance so an already-running Spectacle
    // (e.g. an active screen recording) is not routed to / interrupted.
    const QString program = QStringLiteral("spectacle");
    const QStringList args = {
        QStringLiteral("-i"),
        QStringLiteral("-b"),
        QStringLiteral("-n"),
        QStringLiteral("-m"),
        QStringLiteral("-o"),
        m_requestedPath,
    };

    m_process->start(program, args);
}

QString Capture::prepare(const QString &path) const
{
    return path;
}

void Capture::discard(const QString &path)
{
    if (!path.isEmpty()) {
        QFile::remove(path);
    }
}

QString Capture::outputPath() const
{
    const QString runtime = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
    return QDir(runtime).filePath(QStringLiteral("termite/screen.png"));
}

void Capture::ensureRuntimeDir() const
{
    const QString runtime = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
    QDir dir(runtime);
    dir.mkpath(QStringLiteral("termite"));
}