#pragma once

#include <QEvent>
#include <QWidget>
#include <QShowEvent>

#include <LayerShellQt/Window>

#include "settings.h"

class QScreen;
class QTermWidget;
class QLabel;
class QPropertyAnimation;

class Overlay : public QWidget
{
    Q_OBJECT

public:
    enum class Direction {
        Top,
        Bottom,
    };

    explicit Overlay(QWidget *parent = nullptr);
    ~Overlay() override;

    void showOverlay(const QString &imagePath, Direction direction, double heightPercent);
    void hideOverlay();

    bool isOpen() const;

    void applyTheme(ThemeMode mode);

signals:
    void revealed();
    void hidden();
    void dismissRequested();

private slots:
    void onOpenFinished();
    void onCloseFinished();

private:
    void applyLayout();
    void startOpenAnimation();
    void sizeToScreen();
    void setupLayerShell();
    void setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivity interactivity);
    QScreen *activeScreen() const;
    void showEvent(QShowEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

    QTermWidget *m_terminal = nullptr;
    QLabel *m_screenshot = nullptr;
    QPropertyAnimation *m_openAnim = nullptr;
    QPropertyAnimation *m_closeAnim = nullptr;
    LayerShellQt::Window *m_layer = nullptr;
    Direction m_direction = Direction::Top;
    int m_revealHeight = 0;
    bool m_open = false;
};