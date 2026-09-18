#include "app.h"

#include <QDebug>

#include "capture.h"
#include "shortcut.h"

App::App(QObject *parent)
    : QObject(parent)
    , m_capture(new Capture(this))
    , m_shortcut(new Shortcut(this))
    , m_overlay(new Overlay())
    , m_settings(Settings::fromEnvironment())
{
    connect(m_shortcut, &Shortcut::activated, this, &App::toggle);
    connect(m_shortcut, &Shortcut::conflictDetected, this, &App::shortcutConflict);
    connect(m_capture, &Capture::finished, this, &App::onCaptureFinished);
    connect(m_overlay, &Overlay::revealed, this, &App::onRevealed);
    connect(m_overlay, &Overlay::hidden, this, &App::onHidden);
    connect(m_overlay, &Overlay::dismissRequested, this, &App::hideTerminal);

    m_overlay->applyTheme(m_settings.themeMode);
    m_shortcut->registerShortcut(m_settings.shortcut);
}

App::~App()
{
    delete m_overlay;
}

void App::toggle()
{
    switch (m_state) {
    case State::Hidden:
        qInfo() << "Termite: capturing...";
        m_state = State::Capturing;
        m_capture->capture();
        break;

    case State::Visible:
        qInfo() << "Termite: hiding...";
        startHide();
        break;

    case State::Capturing:
    case State::Showing:
    case State::Hiding:
        qInfo() << "Termite: ignoring shortcut during transition.";
        break;
    }
}

void App::onCaptureFinished(const QString &path)
{
    if (m_state != State::Capturing) {
        return;
    }

    if (path.isEmpty()) {
        qWarning() << "Termite: capture failed; returning to Hidden.";
        resetForError();
        return;
    }

    m_currentCapturePath = path;
    m_state = State::Showing;
    const Overlay::Direction direction =
        m_settings.topPosition ? Overlay::Direction::Top : Overlay::Direction::Bottom;
    m_overlay->showOverlay(path, direction, m_settings.heightPercent);
}

void App::onRevealed()
{
    if (m_state != State::Showing) {
        return;
    }
    m_state = State::Visible;
    qInfo() << "Termite: revealed.";
}

void App::onHidden()
{
    if (m_state != State::Hiding) {
        return;
    }
    m_state = State::Hidden;
    if (!m_currentCapturePath.isEmpty()) {
        m_capture->discard(m_currentCapturePath);
        m_currentCapturePath.clear();
    }
    qInfo() << "Termite: hidden.";
}

void App::startHide()
{
    if (m_state != State::Visible) {
        return;
    }
    m_state = State::Hiding;
    m_overlay->hideOverlay();
}

void App::showTerminal()
{
    if (m_state == State::Hidden) {
        toggle();
    }
}

void App::hideTerminal()
{
    if (m_state == State::Visible) {
        startHide();
    }
}

bool App::setShortcut(const QKeySequence &sequence)
{
    if (sequence.isEmpty()) {
        qWarning() << "Termite: refusing to set an empty shortcut.";
        return false;
    }

    const bool ok = m_shortcut->setShortcut(sequence);
    if (ok) {
        m_settings.shortcut = sequence;
        m_settings.save();
        qInfo() << "Termite: shortcut changed and persisted to"
                << Settings::configFile();
    }
    return ok;
}

void App::resetForError()
{
    if (!m_currentCapturePath.isEmpty()) {
        m_capture->discard(m_currentCapturePath);
        m_currentCapturePath.clear();
    }
    m_state = State::Hidden;
}