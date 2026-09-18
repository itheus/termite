#pragma once

#include <QString>

// Path used to (re)launch Termite.
//
// When running from an AppImage this is the AppImage file itself ($APPIMAGE)
// rather than the ephemeral inner mount path (e.g.
// /tmp/.mount_TermitXXXX/usr/bin/termite), so autostart entries and detached
// child processes keep working after the mount goes away.
QString termiteLaunchPath();

// True when the current process is running from an AppImage.
bool termiteRunsAsAppImage();
