#include "shortcutrecorder.h"

#include <QApplication>
#include <QEvent>
#include <QKeyEvent>
#include <QKeySequence>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace {
int modifierMask(int key)
{
    switch (key) {
    case Qt::Key_Shift:     return Qt::ShiftModifier;
    case Qt::Key_Control:   return Qt::ControlModifier;
    case Qt::Key_Alt:       return Qt::AltModifier;
    case Qt::Key_AltGr:     return Qt::GroupSwitchModifier;
    case Qt::Key_Meta:
    case Qt::Key_Super_L:
    case Qt::Key_Super_R:   return Qt::MetaModifier;
    default:                return 0;
    }
}

bool isModifier(int key)
{
    return modifierMask(key) != 0;
}
} // namespace

ShortcutRecorder::ShortcutRecorder(const QKeySequence &initial, QWidget *parent)
    : QWidget(parent)
    , m_sequence(initial)
{
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_label = new QLabel(this);
    m_label->setStyleSheet(
        QStringLiteral("background-color: rgba(46, 204, 113, 0.25);"
                       "border-radius: 15px;"
                       "padding: 3px 10px;"));
    m_button = new QPushButton(QStringLiteral("Record…"), this);
    layout->addWidget(m_label);
    layout->addWidget(m_button);

    connect(m_button, &QPushButton::clicked, this, [this]() {
        if (m_state == ModState::Recording) {
            stopRecording(false);
        } else {
            startRecording();
        }
    });

    updateLabel();
}

void ShortcutRecorder::startRecording()
{
    m_state = ModState::Recording;
    m_modifiers = 0;
    m_lastKey = 0;
    m_captured = false;
    m_button->setText(QStringLiteral("Cancel"));
    m_label->setText(QStringLiteral("Press the new shortcut"));
    grabKeyboard();
    qApp->installEventFilter(this);
}

void ShortcutRecorder::stopRecording(bool apply)
{
    if (m_state != ModState::Recording) {
        return;
    }
    m_state = ModState::Idle;
    releaseKeyboard();
    qApp->removeEventFilter(this);
    m_button->setText(QStringLiteral("Record…"));

    if (apply && m_captured && m_lastKey != 0) {
        m_sequence = QKeySequence(m_modifiers | m_lastKey);
        emit shortcutRecorded(m_sequence);
    }
    updateLabel();
}

void ShortcutRecorder::setSequence(const QKeySequence &sequence)
{
    m_sequence = sequence;
    if (m_state == ModState::Idle) {
        updateLabel();
    }
}

void ShortcutRecorder::updateLabel()
{
    if (m_state == ModState::Recording) {
        return;
    }
    m_label->setText(m_sequence.toString().isEmpty()
                         ? QStringLiteral("(none)")
                         : m_sequence.toString());
}

bool ShortcutRecorder::eventFilter(QObject *watched, QEvent *event)
{
    if (m_state != ModState::Recording) {
        return QWidget::eventFilter(watched, event);
    }
    if (event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease) {
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        if (event->type() == QEvent::KeyPress) {
            keyPressEvent(keyEvent);
        } else {
            keyReleaseEvent(keyEvent);
        }
        return true;
    }
    return QWidget::eventFilter(watched, event);
}

void ShortcutRecorder::keyPressEvent(QKeyEvent *event)
{
    if (m_state != ModState::Recording) {
        QWidget::keyPressEvent(event);
        return;
    }
    const int key = event->key();
    if (key == Qt::Key_Escape) {
        stopRecording(false);
        return;
    }
    if (isModifier(key)) {
        m_modifiers |= modifierMask(key);
        return;
    }
    m_lastKey = key;
    m_modifiers |= event->modifiers() & ~Qt::KeypadModifier;
    m_captured = true;
}

void ShortcutRecorder::keyReleaseEvent(QKeyEvent *event)
{
    if (m_state != ModState::Recording) {
        QWidget::keyReleaseEvent(event);
        return;
    }
    if (m_captured && event->key() == m_lastKey) {
        stopRecording(true);
    }
}