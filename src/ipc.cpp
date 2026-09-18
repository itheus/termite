#include "ipc.h"

#include <QLocalSocket>
#include <QCoreApplication>

const char *kTermiteIpcName = "termite-runtime";

bool IpcClient::isRuntimeRunning()
{
    QLocalSocket probe;
    probe.connectToServer(QString::fromLatin1(kTermiteIpcName));
    const bool ok = probe.waitForConnected(300);
    probe.abort();
    return ok;
}

QString IpcClient::send(const QString &command, int timeoutMs)
{
    QLocalSocket socket;
    socket.connectToServer(QString::fromLatin1(kTermiteIpcName));
    if (!socket.waitForConnected(500)) {
        return QString();
    }

    socket.write(command.toUtf8());
    socket.write("\n");

    if (!socket.waitForReadyRead(timeoutMs)) {
        return socket.state() == QLocalSocket::UnconnectedState ? QStringLiteral("ok") : QString();
    }

    const QByteArray data = socket.readAll();
    const int nl = data.indexOf('\n');
    const QByteArray line = (nl >= 0) ? data.left(nl) : data;
    return QString::fromUtf8(line).trimmed();
}