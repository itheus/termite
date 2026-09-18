#pragma once

#include <QString>

extern const char *kTermiteIpcName;

class IpcClient
{
public:
    static bool isRuntimeRunning();
    static QString send(const QString &command, int timeoutMs = 1000);
};