#include "overlay.h"

#include <QApplication>
#include <QCursor>
#include <QDebug>
#include <QScreen>
#include <QLabel>
#include <QPixmap>
#include <QPropertyAnimation>
#include <QShortcut>
#include <QTimer>

#include <qtermwidget.h>

#include <KWindowSystem>

Overlay::Overlay(QWidget *parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_TranslucentBackground, true);

    m_terminal = new QTermWidget(1, this);
    m_terminal->setAutoClose(false);
    m_terminal->setScrollBarPosition(QTermWidgetInterface::ScrollBarPosition::NoScrollBar);

    auto *pasteShortcut = new QShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+V")), this);
    connect(pasteShortcut, &QShortcut::activated, this, [this]() {
        m_terminal->pasteClipboard();
    });
    auto *pasteInsert = new QShortcut(QKeySequence(QStringLiteral("Shift+Insert")), this);
    connect(pasteInsert, &QShortcut::activated, this, [this]() {
        m_terminal->pasteClipboard();
    });
    auto *copyShortcut = new QShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+C")), this);
    connect(copyShortcut, &QShortcut::activated, this, [this]() {
        m_terminal->copyClipboard();
    });

    m_screenshot = new QLabel(this);
    m_screenshot->setScaledContents(true);
    m_screenshot->installEventFilter(this);

    m_openAnim = new QPropertyAnimation(m_screenshot, "pos", this);
    m_openAnim->setDuration(280);
    m_openAnim->setEasingCurve(QEasingCurve::InOutCubic);
    connect(m_openAnim, &QPropertyAnimation::finished, this, &Overlay::onOpenFinished);

    m_closeAnim = new QPropertyAnimation(m_screenshot, "pos", this);
    m_closeAnim->setDuration(280);
    m_closeAnim->setEasingCurve(QEasingCurve::InOutCubic);
    connect(m_closeAnim, &QPropertyAnimation::finished, this, &Overlay::onCloseFinished);

    m_terminal->hide();
    m_screenshot->hide();
    setMask(QRegion(QRect(0, 0, 1, 1)));

    sizeToScreen();
    showFullScreen();

    setupLayerShell();
}

void Overlay::setupLayerShell()
{
    if (m_layer) {
        return;
    }

    auto *handle = windowHandle();
    if (!handle) {
        qWarning() << "Overlay: no window handle yet; layer-shell deferred.";
        return;
    }

    m_layer = LayerShellQt::Window::get(handle);
    if (!m_layer) {
        qWarning() << "Overlay: failed to obtain layer-shell window.";
        return;
    }

    m_layer->setLayer(LayerShellQt::Window::LayerOverlay);
    m_layer->setAnchors(LayerShellQt::Window::Anchors(
        LayerShellQt::Window::AnchorTop | LayerShellQt::Window::AnchorBottom
        | LayerShellQt::Window::AnchorLeft | LayerShellQt::Window::AnchorRight));
    m_layer->setExclusiveZone(-1);
    m_layer->setCloseOnDismissed(false);
    m_layer->setScope(QStringLiteral("termite"));
    m_layer->setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivityExclusive);
}

void Overlay::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);

    setupLayerShell();
}

bool Overlay::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_screenshot && event->type() == QEvent::MouseButtonPress) {
        emit dismissRequested();
        return true;
    }
    return QWidget::eventFilter(watched, event);
}

Overlay::~Overlay() = default;

void Overlay::showOverlay(const QString &imagePath, Direction direction, double heightPercent)
{
    m_direction = direction;
    m_open = true;

    QPixmap pixmap(imagePath);
    if (pixmap.isNull()) {
        qWarning() << "Overlay: failed to load screenshot" << imagePath;
        sizeToScreen();
        m_terminal->show();
        clearMask();
        setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivityExclusive);
        activateWindow();
        m_terminal->setFocus();
        emit revealed();
        return;
    }

    m_screenshot->setPixmap(pixmap);

    const double clamped = qBound(0.1, heightPercent, 0.9);
    m_revealHeight = static_cast<int>(activeScreen()->size().height() * clamped);

    m_screenshot->show();
    m_terminal->show();
    clearMask();
    setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivityExclusive);
    activateWindow();

    sizeToScreen();
    applyLayout();
    m_screenshot->raise();
    m_screenshot->move(0, 0);

    QTimer::singleShot(16, this, [this]() {
        applyLayout();
        m_screenshot->raise();
        m_screenshot->move(0, 0);
        m_terminal->setFocus();
        startOpenAnimation();
    });
}

void Overlay::hideOverlay()
{
    if (!m_open) {
        return;
    }

    m_openAnim->stop();
    m_closeAnim->stop();

    m_closeAnim->setStartValue(m_screenshot->pos());
    m_closeAnim->setEndValue(QPoint(0, 0));
    m_closeAnim->start();
}

bool Overlay::isOpen() const
{
    return m_open;
}

void Overlay::applyTheme(ThemeMode mode)
{
    const bool dark = (mode == ThemeMode::Dark)
                      || (mode == ThemeMode::FollowSystem && Settings::systemPrefersDark());
    m_terminal->setColorScheme(dark ? QStringLiteral("WhiteOnBlack")
                                    : QStringLiteral("BlackOnWhite"));
}

void Overlay::onOpenFinished()
{
    m_terminal->setFocus();
    emit revealed();
}

void Overlay::onCloseFinished()
{
    m_open = false;
    m_screenshot->hide();
    m_terminal->hide();
    setMask(QRegion(QRect(0, 0, 1, 1)));
    setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivityNone);
    clearFocus();
    emit hidden();
}

void Overlay::setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivity interactivity)
{
    if (m_layer) {
        m_layer->setKeyboardInteractivity(interactivity);
    }
}

QScreen *Overlay::activeScreen() const
{
    if (QGuiApplication::screenAt(QCursor::pos())) {
        return QGuiApplication::screenAt(QCursor::pos());
    }
    return QGuiApplication::primaryScreen();
}

void Overlay::sizeToScreen()
{
    resize(activeScreen()->size());
}

void Overlay::applyLayout()
{
    const int w = width();
    const int h = height();

    if (m_direction == Direction::Top) {
        m_terminal->setGeometry(0, 0, w, m_revealHeight);
    } else {
        m_terminal->setGeometry(0, h - m_revealHeight, w, m_revealHeight);
    }

    m_screenshot->setGeometry(0, 0, w, h);
}

void Overlay::startOpenAnimation()
{
    const int target = (m_direction == Direction::Top) ? m_revealHeight : -m_revealHeight;
    m_screenshot->move(0, 0);
    m_screenshot->raise();
    m_openAnim->setStartValue(QPoint(0, 0));
    m_openAnim->setEndValue(QPoint(0, target));
    m_openAnim->start();
}