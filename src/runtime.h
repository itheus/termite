#pragma once

#include <QObject>

#include <QAction>

class QLocalServer;
class QLocalSocket;

class App;

class Runtime : public QObject
{
    Q_OBJECT

public:
    explicit Runtime(QObject *parent = nullptr);
    ~Runtime() override;

    bool claimInstanceAndRun(const QString &request);

private slots:
    void onNewConnection();
    void onClientReadyRead();

private:
    void handleCommand(QLocalSocket *socket, const QString &command);
    void applyAutostart();
    void registerSettingsLauncher();
    void stop();
    void restart();

    App *m_app = nullptr;
    QLocalServer *m_server = nullptr;
    QAction *m_settingsAction = nullptr;
    bool m_isPrimary = false;
};