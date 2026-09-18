#include "settingsui.h"

#include <QComboBox>
#include <QDebug>
#include <QFont>
#include <QFormLayout>
#include <QFrame>
#include <QGraphicsOpacityEffect>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QKeySequence>
#include <QLabel>
#include <QLocalServer>
#include <QLocalSocket>
#include <QMouseEvent>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QCheckBox>
#include <QProcess>
#include <QResizeEvent>
#include <QSpinBox>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

#include "ipc.h"
#include "launchpath.h"
#include "settings.h"
#include "shortcutrecorder.h"

namespace {
void startRuntime()
{
    const QStringList args = termiteRunsAsAppImage()
                                 ? QStringList{QStringLiteral("--runtime")}
                                 : QStringList{};
    QProcess::startDetached(termiteLaunchPath(), args);
}
} // namespace

SettingsUi::SettingsUi(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle(QStringLiteral("Settings"));
    setMinimumWidth(440);

    m_saved = Settings::load();
    m_pending = m_saved;

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(20, 20, 20, 16);
    root->setSpacing(14);

    m_aboutButton = new QToolButton(this);
    m_aboutButton->setIconSize(QSize(72, 72));
    m_aboutButton->setCursor(Qt::PointingHandCursor);
    m_aboutButton->setToolTip(QStringLiteral("About Termite"));
    m_aboutButton->setAccessibleName(QStringLiteral("About Termite"));
    m_aboutButton->setStyleSheet(
        QStringLiteral("QToolButton { border: none; background: transparent; }"));
    root->addWidget(m_aboutButton, 0, Qt::AlignHCenter);

    //  Status poo
    auto *statusBox = new QGroupBox(QStringLiteral("Status"), this);
    auto *statusLayout = new QHBoxLayout(statusBox);
    m_statusLabel = new QLabel(QStringLiteral("Checking…"), this);
    statusLayout->addWidget(m_statusLabel);
    statusLayout->addStretch(1);
    m_startButton = new QPushButton(QStringLiteral("Start"), this);
    m_stopButton = new QPushButton(QStringLiteral("Stop"), this);
    m_restartButton = new QPushButton(QStringLiteral("Restart"), this);
    statusLayout->addWidget(m_startButton);
    statusLayout->addWidget(m_stopButton);
    statusLayout->addWidget(m_restartButton);
    root->addWidget(statusBox);

    // terminal crapiola
    auto *termBox = new QGroupBox(QStringLiteral("Appearance"), this);
    auto *termLayout = new QFormLayout(termBox);
    termLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    m_position = new QComboBox(this);
    m_position->addItem(QStringLiteral("Top"), true);
    m_position->addItem(QStringLiteral("Bottom"), false);
    m_position->setCurrentIndex(m_saved.topPosition ? 0 : 1);
    termLayout->addRow(QStringLiteral("Position"), m_position);

    m_theme = new QComboBox(this);
    m_theme->addItem(QStringLiteral("Follow System Theme"),
                     static_cast<int>(ThemeMode::FollowSystem));
    m_theme->addItem(QStringLiteral("Light"), static_cast<int>(ThemeMode::Light));
    m_theme->addItem(QStringLiteral("Dark"), static_cast<int>(ThemeMode::Dark));
    m_theme->setCurrentIndex(
        m_saved.themeMode == ThemeMode::Light
            ? 1
            : m_saved.themeMode == ThemeMode::Dark ? 2 : 0);
    termLayout->addRow(QStringLiteral("Theme"), m_theme);

    m_heightSpin = new QSpinBox(this);
    m_heightSpin->setRange(10, 90);
    m_heightSpin->setValue(qRound(m_saved.heightPercent * 100));
    termLayout->addRow(QStringLiteral("Height"), m_heightSpin);

    root->addWidget(termBox);

    // Shortcut poop
    auto *scBox = new QGroupBox(QStringLiteral("Shortcut"), this);
    auto *scLayout = new QFormLayout(scBox);
    scLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    m_shortcutRecorder = new ShortcutRecorder(m_saved.shortcut, this);
    scLayout->addRow(m_shortcutRecorder);
    m_shortcutStatus = new QLabel(this);
    m_shortcutStatus->setWordWrap(true);
    scLayout->addRow(QString(), m_shortcutStatus);
    root->addWidget(scBox);

    // Startup poop
    auto *behBox = new QGroupBox(QStringLiteral("Startup"), this);
    auto *behLayout = new QVBoxLayout(behBox);
    m_autostart = new QCheckBox(
        QStringLiteral("Start Termite automatically with the KDE session"), this);
    m_autostart->setChecked(m_saved.autostart);
    behLayout->addWidget(m_autostart);
    root->addWidget(behBox);

    root->addStretch(1);

    // Footer poop
    auto *bottomRow = new QHBoxLayout;
    m_appliedLabel = new QLabel(this);
    m_appliedLabel->hide();
    bottomRow->addWidget(m_appliedLabel);
    bottomRow->addStretch(1);
    m_cancelButton = new QPushButton(QStringLiteral("Cancel"), this);
    m_applyButton = new QPushButton(QStringLiteral("Apply"), this);
    m_applyButton->setDefault(true);
    bottomRow->addWidget(m_cancelButton);
    bottomRow->addWidget(m_applyButton);
    root->addLayout(bottomRow);

    // About 
    m_aboutOverlay = new QWidget(this);
    m_aboutOverlay->setObjectName(QStringLiteral("aboutOverlay"));
    m_aboutOverlay->setAttribute(Qt::WA_StyledBackground, true);
    m_aboutOverlay->hide();
    m_aboutOverlay->installEventFilter(this);

    m_aboutEffect = new QGraphicsOpacityEffect(m_aboutOverlay);
    m_aboutOverlay->setGraphicsEffect(m_aboutEffect);

    m_aboutAnim = new QPropertyAnimation(m_aboutEffect, "opacity", this);
    m_aboutAnim->setDuration(200);
    m_aboutAnim->setEasingCurve(QEasingCurve::InOutCubic);
    connect(m_aboutAnim, &QPropertyAnimation::finished, this, [this]() {
        if (m_aboutEffect->opacity() <= 0.0) {
            m_aboutOverlay->hide();
        }
    });

    auto *overlayLayout = new QVBoxLayout(m_aboutOverlay);
    overlayLayout->setContentsMargins(24, 24, 24, 24);

    m_aboutCard = new QFrame(m_aboutOverlay);
    m_aboutCard->setFrameShape(QFrame::StyledPanel);
    m_aboutCard->setFrameShadow(QFrame::Raised);
    m_aboutCard->setMaximumWidth(380);
    m_aboutCard->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);

    auto *cardLayout = new QVBoxLayout(m_aboutCard);
    cardLayout->setContentsMargins(28, 28, 28, 28);
    cardLayout->setSpacing(10);

    auto *cardTitle = new QLabel(m_aboutCard);
    cardTitle->setTextFormat(Qt::RichText);
    cardTitle->setAlignment(Qt::AlignHCenter);
    cardTitle->setText(
        QStringLiteral("<span style=\"font-size:13pt; font-weight:bold;\">Termite</span><br/>"
                       "<span style=\"font-size:9pt;\">The terminal that lives under your desktop</span>"));

    auto *cardAuthor = new QLabel(QStringLiteral("Author: itheus00"), m_aboutCard);
    cardAuthor->setAlignment(Qt::AlignHCenter);

    auto *cardWebsite = new QLabel(
        QStringLiteral("<a href=\"https://x.com/itheus00\">https://x.com/itheus00</a>"),
        m_aboutCard);
    cardWebsite->setTextFormat(Qt::RichText);
    cardWebsite->setAlignment(Qt::AlignHCenter);
    cardWebsite->setOpenExternalLinks(true);
    cardWebsite->setTextInteractionFlags(Qt::TextBrowserInteraction);

    cardLayout->addWidget(cardTitle);
    cardLayout->addSpacing(6);
    cardLayout->addWidget(cardAuthor);
    cardLayout->addWidget(cardWebsite);

    overlayLayout->addStretch(1);
    overlayLayout->addWidget(m_aboutCard, 0, Qt::AlignHCenter);
    overlayLayout->addStretch(1);

    connect(m_aboutButton, &QToolButton::clicked, this, &SettingsUi::showAbout);

    connect(m_position, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SettingsUi::onPositionChanged);
    connect(m_theme, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SettingsUi::onThemeChanged);
    connect(m_heightSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &SettingsUi::onHeightChanged);
    connect(m_shortcutRecorder, &ShortcutRecorder::shortcutRecorded,
            this, &SettingsUi::onShortcutEdited);
    connect(m_autostart, &QCheckBox::toggled, this, &SettingsUi::onAutostartToggled);
    connect(m_applyButton, &QPushButton::clicked, this, &SettingsUi::onApplyClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &SettingsUi::onCancelClicked);
    connect(m_startButton, &QPushButton::clicked, this, &SettingsUi::onStartClicked);
    connect(m_stopButton, &QPushButton::clicked, this, &SettingsUi::onStopClicked);
    connect(m_restartButton, &QPushButton::clicked, this, &SettingsUi::onRestartClicked);

    updateAboutTheme();
    refreshStatus();
}

void SettingsUi::onPositionChanged(int)
{
    m_pending.topPosition = m_position->currentData().toBool();
    markDirty();
}

void SettingsUi::onThemeChanged(int)
{
    m_pending.themeMode = static_cast<ThemeMode>(m_theme->currentData().toInt());
    markDirty();
    updateAboutTheme();
}

void SettingsUi::onHeightChanged(int value)
{
    m_pending.heightPercent = value / 100.0;
    markDirty();
}

void SettingsUi::onShortcutEdited(const QKeySequence &sequence)
{
    if (sequence.isEmpty()) {
        showShortcutStatus(QStringLiteral("Invalid: empty shortcut."));
        return;
    }
    m_shortcutStatus->clear();
    m_pending.shortcut = sequence;
    markDirty();
}

void SettingsUi::onAutostartToggled(bool)
{
    m_pending.autostart = m_autostart->isChecked();
    markDirty();
}

void SettingsUi::onApplyClicked()
{
    if (m_pending.shortcut.isEmpty()) {
        showShortcutStatus(QStringLiteral("Choose a valid shortcut first."));
        return;
    }

    if (IpcClient::isRuntimeRunning()) {
        IpcClient::send(QStringLiteral("stop"));
        QTimer::singleShot(300, this, [this]() { pollThenApply(); });
    } else {
        m_pending.save();
        m_saved = m_pending;
        m_dirty = false;
        updateApplyState();
        startRuntime();
        QTimer::singleShot(500, this, &SettingsUi::refreshStatus);
        showApplied(QStringLiteral("Applied."));
    }
}

void SettingsUi::pollThenApply()
{
    if (IpcClient::isRuntimeRunning()) {
        QTimer::singleShot(300, this, [this]() { pollThenApply(); });
        return;
    }
    m_pending.save();
    m_saved = m_pending;
    m_dirty = false;
    updateApplyState();
    startRuntime();
    QTimer::singleShot(500, this, &SettingsUi::refreshStatus);
    showApplied(QStringLiteral("Applied."));
}

void SettingsUi::showApplied(const QString &text)
{
    m_appliedLabel->setText(text);
    m_appliedLabel->show();
    QTimer::singleShot(3000, m_appliedLabel, &QLabel::hide);
}

void SettingsUi::showShortcutStatus(const QString &text)
{
    m_shortcutStatus->setText(text);
    if (!m_shortcutStatusTimer) {
        m_shortcutStatusTimer = new QTimer(this);
        m_shortcutStatusTimer->setSingleShot(true);
        connect(m_shortcutStatusTimer, &QTimer::timeout,
                m_shortcutStatus, &QLabel::clear);
    }
    m_shortcutStatusTimer->start(3000);
}

void SettingsUi::onCancelClicked()
{
    revertAll();
    close();
}

void SettingsUi::onStartClicked()
{
    if (IpcClient::isRuntimeRunning()) {
        refreshStatus();
        return;
    }
    startRuntime();
    QTimer::singleShot(400, this, &SettingsUi::refreshStatus);
}

void SettingsUi::onStopClicked()
{
    IpcClient::send(QStringLiteral("stop"));
    QTimer::singleShot(300, this, &SettingsUi::refreshStatus);
}

void SettingsUi::onRestartClicked()
{
    IpcClient::send(QStringLiteral("restart"));
    QTimer::singleShot(500, this, &SettingsUi::refreshStatus);
}

bool SettingsUi::listenForFocus(const QString &name)
{
    m_focusServer = new QLocalServer(this);
    QLocalServer::removeServer(name);
    if (!m_focusServer->listen(name)) {
        delete m_focusServer;
        m_focusServer = nullptr;
        return false;
    }
    connect(m_focusServer, &QLocalServer::newConnection, this, &SettingsUi::onFocusRequest);
    return true;
}

void SettingsUi::onFocusRequest()
{
    QLocalSocket *socket = m_focusServer ? m_focusServer->nextPendingConnection() : nullptr;
    if (socket) {
        socket->deleteLater();
    }
    show();
    raise();
    activateWindow();
}

void SettingsUi::updateAboutTheme()
{
    const bool light = (m_pending.themeMode == ThemeMode::Light)
                       || (m_pending.themeMode == ThemeMode::FollowSystem
                           && !Settings::systemPrefersDark());

    m_aboutButton->setIcon(QIcon(light ? QStringLiteral(":/assets/termite-alt.png")
                                       : QStringLiteral(":/assets/termite.png")));

    const QString bg = light ? QStringLiteral("rgba(255, 255, 255, 0.95)")
                             : QStringLiteral("rgba(22, 22, 23, 0.95)");
    if (m_aboutOverlay) {
        m_aboutOverlay->setStyleSheet(
            QStringLiteral("#aboutOverlay { background-color: %1; }").arg(bg));
    }
}

void SettingsUi::showAbout()
{
    m_aboutAnim->stop();
    m_aboutEffect->setOpacity(0.0);
    m_aboutOverlay->setGeometry(rect());
    m_aboutOverlay->raise();
    m_aboutOverlay->show();
    m_aboutAnim->setStartValue(0.0);
    m_aboutAnim->setEndValue(1.0);
    m_aboutAnim->start();
}

void SettingsUi::hideAbout()
{
    if (!m_aboutOverlay || !m_aboutOverlay->isVisible()) {
        return;
    }
    m_aboutAnim->stop();
    m_aboutAnim->setStartValue(m_aboutEffect->opacity());
    m_aboutAnim->setEndValue(0.0);
    m_aboutAnim->start();
}

void SettingsUi::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (m_aboutOverlay && m_aboutOverlay->isVisible()) {
        m_aboutOverlay->setGeometry(rect());
    }
}

bool SettingsUi::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_aboutOverlay && event->type() == QEvent::MouseButtonPress) {
        auto *mouseEvent = static_cast<QMouseEvent *>(event);
        bool inside = false;
        if (m_aboutCard) {
            const QPoint pos =
                m_aboutCard->mapFromGlobal(mouseEvent->globalPosition().toPoint());
            inside = m_aboutCard->rect().contains(pos);
        }
        if (!inside) {
            hideAbout();
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void SettingsUi::refreshStatus()
{
    if (IpcClient::isRuntimeRunning()) {
        setStatusRunning();
    } else {
        setStatusStopped();
    }
}

void SettingsUi::revertAll()
{
    m_pending = m_saved;
    m_position->setCurrentIndex(m_saved.topPosition ? 0 : 1);
    m_theme->setCurrentIndex(
        m_saved.themeMode == ThemeMode::Light
            ? 1
            : m_saved.themeMode == ThemeMode::Dark ? 2 : 0);
    m_heightSpin->setValue(qRound(m_saved.heightPercent * 100));
    m_shortcutRecorder->setSequence(m_saved.shortcut);
    m_shortcutStatus->setText(QStringLiteral(""));
    m_autostart->setChecked(m_saved.autostart);
    m_dirty = false;
    updateApplyState();
    updateAboutTheme();
}

void SettingsUi::markDirty()
{
    m_dirty = true;
    updateApplyState();
}

void SettingsUi::updateApplyState()
{
    m_applyButton->setEnabled(m_dirty);
}

void SettingsUi::setStatusRunning()
{
    m_statusLabel->setText(QStringLiteral("● Running"));
    m_statusLabel->setStyleSheet(QStringLiteral("color: #2ecc71;"));
    m_startButton->setEnabled(false);
    m_stopButton->setEnabled(true);
    m_restartButton->setEnabled(true);
}

void SettingsUi::setStatusStopped()
{
    m_statusLabel->setText(QStringLiteral("■ Stopped"));
    m_statusLabel->setStyleSheet(QStringLiteral("color: #e74c3c;"));
    m_startButton->setEnabled(true);
    m_stopButton->setEnabled(false);
    m_restartButton->setEnabled(false);
}
