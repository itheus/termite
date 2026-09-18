#include <QApplication>
#include <QDebug>
#include <QIcon>
#include <QLocalSocket>

#include "runtime.h"
#include "settingsui.h"

int main(int argc, char *argv[])
{
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) {
        qputenv("QT_QPA_PLATFORM", "wayland");
    }
    if (qEnvironmentVariableIsEmpty("QT_WAYLAND_SHELL_INTEGRATION")) {
        qputenv("QT_WAYLAND_SHELL_INTEGRATION", "xdg-shell");
    }

    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("termite"));
    app.setApplicationDisplayName(QStringLiteral("Termite"));
    app.setApplicationVersion(QStringLiteral("0.6.0"));
    app.setWindowIcon(QIcon(QStringLiteral(":/assets/icon.png")));

    const QStringList args = app.arguments();

    if (args.contains(QStringLiteral("--settings"))) {
        const QString settingsName = QStringLiteral("termite-settings");
        QLocalSocket probe;
        probe.connectToServer(settingsName);
        if (probe.waitForConnected(300)) {
            probe.write("show\n");
            probe.waitForBytesWritten(500);
            probe.disconnectFromServer();
            qInfo() << "Termite: settings window already open; focusing it.";
            return 0;
        }
        SettingsUi ui;
        if (!ui.listenForFocus(settingsName)) {
            QLocalSocket focus;
            focus.connectToServer(settingsName);
            if (focus.waitForConnected(300)) {
                focus.write("show\n");
                focus.waitForBytesWritten(500);
            }
            return 0;
        }
        ui.show();
        return app.exec();
    }

    QString request;
    if (args.size() > 1 && args.at(1) != QStringLiteral("--runtime")) {
        request = args.at(1);
    }

    Runtime runtime;

    if (!runtime.claimInstanceAndRun(request)) {
        qInfo() << "Termite: another runtime is already active; command forwarded.";
        return 0;
    }

    return app.exec();
}