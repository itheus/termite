#include "settings.h"

#include <QApplication>
#include <QColor>
#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>
#include <QKeySequence>
#include <QStyleHints>
#include <QPalette>
#include <QSettings>
#include <QStandardPaths>
#include <QString>
#include <QtGlobal>

QString Settings::configDir()
{
    const QString base = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    return QDir(base).filePath(QStringLiteral("termite"));
}

QString Settings::configFile()
{
    return QDir(configDir()).filePath(QStringLiteral("termite.conf"));
}

Settings Settings::fromEnvironment()
{
    Settings s = load();

    if (qEnvironmentVariableIsSet("TERMITE_HEIGHT")) {
        bool ok = false;
        const double v = qEnvironmentVariable("TERMITE_HEIGHT").toDouble(&ok);
        if (ok && v >= 0.1 && v <= 0.9) {
            s.heightPercent = v;
        }
    }

    if (qEnvironmentVariableIsSet("TERMITE_POSITION")) {
        const QString p = qEnvironmentVariable("TERMITE_POSITION").trimmed().toLower();
        if (p == QLatin1String("bottom")) {
            s.topPosition = false;
        } else if (p == QLatin1String("top")) {
            s.topPosition = true;
        }
    }

    return s;
}

Settings Settings::load()
{
    Settings s;

    QSettings cfg(configFile(), QSettings::IniFormat);

    const double h = cfg.value(QStringLiteral("heightPercent"), s.heightPercent).toDouble();
    if (h >= 0.1 && h <= 0.9) {
        s.heightPercent = h;
    }

    s.topPosition = cfg.value(QStringLiteral("topPosition"), s.topPosition).toBool();
    s.autostart = cfg.value(QStringLiteral("autostart"), s.autostart).toBool();

    const QString seq = cfg.value(QStringLiteral("shortcut"), s.shortcut.toString()).toString();
    const QKeySequence ks(seq);
    if (!ks.isEmpty()) {
        s.shortcut = ks;
    }

    const QString theme = cfg.value(QStringLiteral("themeMode"), QStringLiteral("follow"))
                              .toString()
                              .toLower();
    if (theme == QLatin1String("light")) {
        s.themeMode = ThemeMode::Light;
    } else if (theme == QLatin1String("dark")) {
        s.themeMode = ThemeMode::Dark;
    } else {
        s.themeMode = ThemeMode::FollowSystem;
    }

    return s;
}

void Settings::save() const
{
    QDir().mkpath(configDir());
    QSettings cfg(configFile(), QSettings::IniFormat);
    cfg.setValue(QStringLiteral("heightPercent"), heightPercent);
    cfg.setValue(QStringLiteral("topPosition"), topPosition);
    cfg.setValue(QStringLiteral("autostart"), autostart);
    cfg.setValue(QStringLiteral("shortcut"), shortcut.toString());

    const QString theme = themeMode == ThemeMode::Light  ? QStringLiteral("light")
                          : themeMode == ThemeMode::Dark ? QStringLiteral("dark")
                                                        : QStringLiteral("follow");
    cfg.setValue(QStringLiteral("themeMode"), theme);
    cfg.sync();
}

bool Settings::systemPrefersDark()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    const Qt::ColorScheme scheme = QGuiApplication::styleHints()->colorScheme();
    if (scheme == Qt::ColorScheme::Dark) {
        return true;
    }
    if (scheme == Qt::ColorScheme::Light) {
        return false;
    }
#endif

    const QColor window = QApplication::palette().color(QPalette::Window);
    if (window.isValid()) {
        const double luminance =
            0.299 * window.red() + 0.587 * window.green() + 0.114 * window.blue();
        return luminance < 128.0;
    }

    const QString config =
        QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    QSettings kg(QDir(config).filePath(QStringLiteral("kdeglobals")), QSettings::IniFormat);
    const QString name = kg.value(QStringLiteral("General/ColorScheme"), QString())
                             .toString()
                             .toLower();
    if (name.contains(QLatin1String("dark")) || name.contains(QLatin1String("nokto"))) {
        return true;
    }
    if (name.contains(QLatin1String("light"))) {
        return false;
    }

    return false;
}