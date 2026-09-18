#pragma once

#include <QObject>
#include <QString>

#include "overlay.h"
#include "settings.h"

class Capture;
class Shortcut;

class App : public QObject
{
    Q_OBJECT

public:
    explicit App(QObject *parent = nullptr);
    ~App() override;

    void showTerminal();
    void hideTerminal();

    const Settings &settings() const { return m_settings; }

    void toggle();

    bool setShortcut(const QKeySequence &sequence);

signals:
    void shortcutConflict(const QKeySequence &sequence);

private:
    enum class State {
        Hidden,
        Capturing,
        Showing,
        Visible,
        Hiding,
    };

    void onCaptureFinished(const QString &path);
    void onRevealed();
    void onHidden();
    void startHide();
    void resetForError();

    Capture *m_capture = nullptr;
    Shortcut *m_shortcut = nullptr;
    Overlay *m_overlay = nullptr;

    Settings m_settings;
    State m_state = State::Hidden;
    QString m_currentCapturePath;
};