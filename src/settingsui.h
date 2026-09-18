#pragma once

#include <QWidget>
#include <QLocalServer>

#include "settings.h"

class QComboBox;
class QLabel;
class QSpinBox;
class QPushButton;
class QCheckBox;
class QToolButton;
class QFrame;
class QGraphicsOpacityEffect;
class QPropertyAnimation;
class QTimer;

class ShortcutRecorder;

class SettingsUi : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsUi(QWidget *parent = nullptr);

    bool listenForFocus(const QString &name);

private slots:
    void onPositionChanged(int index);
    void onThemeChanged(int index);
    void onHeightChanged(int value);
    void onShortcutEdited(const QKeySequence &sequence);
    void onAutostartToggled(bool checked);
    void onApplyClicked();
    void onCancelClicked();
    void pollThenApply();
    void onStartClicked();
    void onStopClicked();
    void onRestartClicked();
    void refreshStatus();

private:
    void revertAll();
    void showApplied(const QString &text);
    void setStatusRunning();
    void setStatusStopped();
    void onFocusRequest();
    void showAbout();
    void hideAbout();
    void updateAboutTheme();
    void showShortcutStatus(const QString &text);

    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

    QToolButton *m_aboutButton = nullptr;
    QWidget *m_aboutOverlay = nullptr;
    QFrame *m_aboutCard = nullptr;
    QGraphicsOpacityEffect *m_aboutEffect = nullptr;
    QPropertyAnimation *m_aboutAnim = nullptr;

    QComboBox *m_position = nullptr;
    QComboBox *m_theme = nullptr;
    QSpinBox *m_heightSpin = nullptr;
    ShortcutRecorder *m_shortcutRecorder = nullptr;
    QLabel *m_shortcutStatus = nullptr;
    QTimer *m_shortcutStatusTimer = nullptr;
    QCheckBox *m_autostart = nullptr;
    QLabel *m_statusLabel = nullptr;
    QPushButton *m_startButton = nullptr;
    QPushButton *m_stopButton = nullptr;
    QPushButton *m_restartButton = nullptr;
    QPushButton *m_applyButton = nullptr;
    QPushButton *m_cancelButton = nullptr;

    QLabel *m_appliedLabel = nullptr;

    QLocalServer *m_focusServer = nullptr;

    Settings m_saved;
    Settings m_pending;
    bool m_dirty = false;

    void markDirty();
    void updateApplyState();
};
