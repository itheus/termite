#pragma once

#include <QWidget>

class QKeySequence;
class QLabel;
class QPushButton;

class ShortcutRecorder : public QWidget
{
    Q_OBJECT

public:
    explicit ShortcutRecorder(const QKeySequence &initial, QWidget *parent = nullptr);

    QKeySequence sequence() const { return m_sequence; }

    void setSequence(const QKeySequence &sequence);

signals:
    void shortcutRecorded(const QKeySequence &sequence);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private:
    void startRecording();
    void stopRecording(bool apply);
    void updateLabel();

    enum class ModState { Idle, Recording };

    ModState m_state = ModState::Idle;
    QKeySequence m_sequence;
    int m_modifiers = 0;
    int m_lastKey = 0;
    bool m_captured = false;

    QLabel *m_label = nullptr;
    QPushButton *m_button = nullptr;
};