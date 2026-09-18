#pragma once

#include <QString>

#include <QKeySequence>

enum class ThemeMode {
    FollowSystem,
    Light,
    Dark,
};

struct Settings
{
    double heightPercent = 0.1;
    bool topPosition = true;
    bool autostart = false;
    QKeySequence shortcut = QKeySequence(Qt::META | Qt::Key_M);
    ThemeMode themeMode = ThemeMode::FollowSystem;

    static Settings fromEnvironment();
    static Settings load();
    void save() const;

    static QString configDir();
    static QString configFile();

    static bool systemPrefersDark();
};