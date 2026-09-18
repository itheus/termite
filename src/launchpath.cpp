#include "launchpath.h"

#include <QCoreApplication>

QString termiteLaunchPath()
{
    const QString appImage = qEnvironmentVariable("APPIMAGE");
    if (!appImage.isEmpty()) {
        return appImage;
    }
    return QCoreApplication::applicationFilePath();
}

bool termiteRunsAsAppImage()
{
    return !qEnvironmentVariable("APPIMAGE").isEmpty();
}
