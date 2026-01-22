#include "callWidget.h"
#include <QGraphicsDropShadowEffect>
#include <cmath>
#include <algorithm>
#include <string>

#include "utilities/utilities.h"
#include "utilities/constant.h"
#include "utilities/color.h"

// Style definitions
const QColor StyleCallWidget::m_primaryColor = COLOR_PRIMARY;
const QColor StyleCallWidget::m_hoverColor = COLOR_PRIMARY_HOVER;
const QColor StyleCallWidget::m_backgroundColor = COLOR_BG_PRIMARY;
const QColor StyleCallWidget::m_textColor = COLOR_TEXT_PRIMARY;
const QColor StyleCallWidget::m_containerColor = QColor(255, 255, 255, 50);
const QColor StyleCallWidget::m_whiteColor = COLOR_BG_WHITE;
const QColor StyleCallWidget::m_controlButtonColor = COLOR_GLASS_WHITE_180;
const QColor StyleCallWidget::m_controlButtonHoverColor = COLOR_GLASS_WHITE_220;
const QColor StyleCallWidget::m_hangupButtonColor = COLOR_ERROR_BANNER;
const QColor StyleCallWidget::m_hangupButtonHoverColor = COLOR_ERROR_BANNER_HOVER;
const QColor StyleCallWidget::m_sliderGrooveColor = COLOR_SLIDER_GROOVE;
const QColor StyleCallWidget::m_sliderHandleColor = COLOR_SLIDER_HANDLE;
const QColor StyleCallWidget::m_sliderSubPageColor = COLOR_SLIDER_SUBPAGE;
const QColor StyleCallWidget::m_volumeLabelColor = COLOR_TEXT_SECONDARY;
const QColor StyleCallWidget::m_scrollAreaBackgroundColor = QColor(0, 0, 0, 0);
const QColor StyleCallWidget::m_sliderContainerColor = COLOR_GLASS_SETTINGS_120;

QString StyleCallWidget::containerStyle() {
    return QString("QWidget {"
        "   background-color: transparent;"
        "   border-radius: %1px;"
        "   padding: 0px;"
        "}")
        .arg(scale(20));
}

QString StyleCallWidget::sliderContainerStyle() {
    return QString("#slidersContainer {"
        "   background-color: rgba(%1, %2, %3, %4);"
        "   border-radius: %5px;"
        "   padding: %6px;"
        "   margin: %7px;"
        "}").arg(m_sliderContainerColor.red())
        .arg(m_sliderContainerColor.green())
        .arg(m_sliderContainerColor.blue())
        .arg(m_sliderContainerColor.alpha())
        .arg(QString::fromStdString(std::to_string(scale(15))))
        .arg(QString::fromStdString(std::to_string(scale(20))))
        .arg(QString::fromStdString(std::to_string(scale(10))));
}

QString StyleCallWidget::titleStyle() {
    return QString("QLabel {"
        "   color: %1;"
        "   margin: 0px;"
        "   padding: 0px;"
        "}").arg(m_textColor.name());
}

QString StyleCallWidget::timerStyle() {
    return QString("QLabel {"
        "   color: %1;"
        "   margin: 0px;"
        "   padding: 0px;"
        "}").arg(m_whiteColor.name());
}

QString StyleCallWidget::controlButtonStyle() {
    return QString("QPushButton {"
        "   background-color: rgba(%1, %2, %3, %4);"
        "   border: none;"
        "   border-radius: %9px;"
        "   padding: %10px;"
        "   margin: %11px;"
        "}"
        "QPushButton:hover {"
        "   background-color: rgba(%5, %6, %7, %8);"
        "}").arg(m_controlButtonColor.red()).arg(m_controlButtonColor.green())
        .arg(m_controlButtonColor.blue()).arg(m_controlButtonColor.alpha())
        .arg(m_controlButtonHoverColor.red()).arg(m_controlButtonHoverColor.green())
        .arg(m_controlButtonHoverColor.blue()).arg(m_controlButtonHoverColor.alpha())
        .arg(QString::fromStdString(std::to_string(scale(25))))
        .arg(QString::fromStdString(std::to_string(scale(15))))
        .arg(QString::fromStdString(std::to_string(scale(5))));
}

QString StyleCallWidget::hangupButtonStyle() {
    return QString("QPushButton {"
        "   background-color: %1;"
        "   border: none;"
        "   border-radius: %3px;"
        "   padding: %4px;"
        "   margin: %5px;"
        "}"
        "QPushButton:focus {"
        "   outline: none;"
        "   border: none;"
        "}"
        "QPushButton:hover {"
        "   background-color: %2;"
        "}").arg(m_hangupButtonColor.name()).arg(m_hangupButtonHoverColor.name())
        .arg(QString::fromStdString(std::to_string(scale(25)))) 
        .arg(QString::fromStdString(std::to_string(scale(15))))
        .arg(QString::fromStdString(std::to_string(scale(5))));  
}

QString StyleCallWidget::disabledHangupButtonStyle() {
    return QString("QPushButton {"
        "   background-color: rgba(%1, %2, %3, 120);"
        "   border: none;"
        "   border-radius: %4px;"
        "   padding: %5px;"
        "   margin: %6px;"
        "   opacity: 0.6;"
        "}").arg(m_hangupButtonColor.red()).arg(m_hangupButtonColor.green()).arg(m_hangupButtonColor.blue())
        .arg(QString::fromStdString(std::to_string(scale(25)))) 
        .arg(QString::fromStdString(std::to_string(scale(15))))
        .arg(QString::fromStdString(std::to_string(scale(5))));  
}

QString StyleCallWidget::panelStyle() {
    return QString("QWidget {"
        "   background-color: transparent;"
        "   border-radius: %1px;"
        "   margin: %2px;"
        "   padding: %3px;"
        "}")
        .arg(QString::fromStdString(std::to_string(scale(25)))) 
        .arg(QString::fromStdString(std::to_string(scale(5))))   
        .arg(QString::fromStdString(std::to_string(scale(15))));
}

QString StyleCallWidget::volumeLabelStyle() {
    return QString(
        "QLabel {"
        "   color: %1;"
        "   font-size: %2px;"
        "   font-weight: bold;"
        "   margin: %3px %4px;"
        "}"
    ).arg(m_volumeLabelColor.name())
        .arg(QString::fromStdString(std::to_string(scale(12)))) 
        .arg(QString::fromStdString(std::to_string(scale(2))))   
        .arg(QString::fromStdString(std::to_string(scale(0)))); 
}

QString StyleCallWidget::scrollAreaStyle() {
    return QString(
        "QScrollArea {"
        "   background-color: transparent;"
        "   border: none;"
        "   margin: 0px;"
        "   padding: 0px;"
        "}"
        "QScrollArea > QWidget > QWidget {"
        "   background-color: transparent;"
        "}"
    );
}

QString StyleCallWidget::volumeSliderStyle() {
    return QString(R"(
        QSlider::groove:horizontal {
            height: %1px; 
            border-radius: %2px;
            margin: 0px 0px; 
        }
        QSlider::handle:horizontal {
            background-color: white;
            width: %3px;
            height: %4px; 
            border-radius: %5px;
            margin: -4px 0;
        }
        QSlider::add-page:horizontal {
            background-color: %7;
            border-radius: %6px;
        }
        QSlider::sub-page:horizontal {
            background-color: %8;
            border-radius: %6px;
        }
        QSlider::disabled {
            background-color: transparent;
        }
        QSlider::groove:horizontal:disabled {
            background-color: %9;
        }
        QSlider::handle:horizontal:disabled {
            background-color: %10;
        }
        QSlider::add-page:horizontal:disabled {
            background-color: %9;
        }
        QSlider::sub-page:horizontal:disabled {
            background-color: %11;
        }
    )")
        .arg(scale(8))
        .arg(scale(4))
        .arg(scale(17))
        .arg(scale(17))
        .arg(scale(8))
        .arg(scale(4))
        .arg(COLOR_SLIDER_GROOVE.name())
        .arg(COLOR_SLIDER_SUBPAGE.name())
        .arg(COLOR_GRAY_180.name())
        .arg(COLOR_GRAY_200.name())
        .arg(COLOR_GRAY_150_DARK.name());
}

QString StyleCallWidget::notificationRedLabelStyle() {
    return QString("QWidget {"
        "   background-color: %1;"
        "   border: none;"
        "   border-radius: %1px;"
        "   margin: 0px;"
        "   padding: 0px;"
        "}").arg(QString::fromStdString(std::to_string(scale(8))));
}

CallWidget::CallWidget(QWidget* parent) : QWidget(parent) {
    // timers first (used later in UI helpers)
    m_callTimer = new QTimer(this);
    m_callDuration = new QTime(0, 0, 0);
    connect(m_callTimer, &QTimer::timeout, this, &CallWidget::updateCallTimer);

    m_overlayButtonHideTimer = new QTimer(this);
    m_overlayButtonHideTimer->setSingleShot(true);
    m_overlayButtonHideTimer->setInterval(3000);
    connect(m_overlayButtonHideTimer, &QTimer::timeout, this, &CallWidget::onExitFullscreenHideTimerTimeout);


    setupUI();
    setupShadowEffect();

    showOverlayButtonWithTimeout();
}

void CallWidget::setupUI() {
    setFocusPolicy(Qt::StrongFocus);

    m_timerLabel = new QLabel("00:00", this);
    m_timerLabel->setAlignment(Qt::AlignCenter);
    m_timerLabel->setStyleSheet(StyleCallWidget::timerStyle());
    m_timerLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

    m_mainScreen = new Screen(this);
    m_mainScreen->setStyleSheet("background-color: transparent; border: none;");
    m_mainScreen->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    m_mainScreen->setMinimumSize(scale(100), scale(100));
    m_mainScreen->hide();
    applyStandardSize();

    m_additionalScreensContainer = new QWidget(this);
    m_additionalScreensContainer->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    m_additionalScreensContainer->setStyleSheet("background-color: transparent;");
    m_additionalScreensContainer->hide();

    m_additionalScreensLayout = new QHBoxLayout(m_additionalScreensContainer);
    m_additionalScreensLayout->setContentsMargins(0, 0, 0, scale(10));
    m_additionalScreensLayout->setSpacing(scale(10));
    m_additionalScreensLayout->setAlignment(Qt::AlignCenter);

    m_participantInfoContainer = new QWidget(this);
    m_participantInfoContainer->setAttribute(Qt::WA_TranslucentBackground);
    m_participantInfoLayout = new QHBoxLayout(m_participantInfoContainer);
    m_participantInfoLayout->setContentsMargins(0, 0, 0, 0);
    m_participantInfoLayout->setSpacing(scale(10));
    m_participantInfoLayout->setAlignment(Qt::AlignCenter);

    m_friendNicknameLabel = new QLabel("Friend", m_participantInfoContainer);
    m_friendNicknameLabel->setAlignment(Qt::AlignCenter);
    m_friendNicknameLabel->setStyleSheet(StyleCallWidget::titleStyle());
    m_friendNicknameLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    QFont nicknameFont("Outfit", 18, QFont::Normal);
    m_friendNicknameLabel->setFont(nicknameFont);

    m_connectionErrorLabel = new QLabel(m_participantInfoContainer);
    m_connectionErrorLabel->setAlignment(Qt::AlignCenter);
    m_connectionErrorLabel->setStyleSheet(QString("color: %1; background: transparent; font-size: %2px; margin: 0px; padding: 0px;")
        .arg(COLOR_HEX_ERROR)
        .arg(scale(12)));
    QFont connectionErrorFont("Outfit", 11, QFont::Medium);
    m_connectionErrorLabel->setFont(connectionErrorFont);
    m_connectionErrorLabel->hide();

    m_participantInfoLayout->addWidget(m_friendNicknameLabel);
    m_participantInfoLayout->addWidget(m_connectionErrorLabel);

    m_participantConnectionErrorBanner = new QWidget(this);
    m_participantConnectionErrorBanner->setStyleSheet(QString("background-color: %1;").arg(COLOR_HEX_ERROR_BANNER));
    m_participantConnectionErrorBanner->setFixedHeight(scale(40));
    m_participantConnectionErrorBanner->hide();
    m_participantConnectionErrorBanner->setAttribute(Qt::WA_TranslucentBackground, false);
    m_participantConnectionErrorBanner->raise();

    m_participantConnectionErrorBannerLabel = new QLabel(m_participantConnectionErrorBanner);
    m_participantConnectionErrorBannerLabel->setAlignment(Qt::AlignCenter);
    m_participantConnectionErrorBannerLabel->setStyleSheet(QString("color: %1; background: transparent; font-size: %2px; font-weight: 600; margin: 0px; padding: 0px;")
        .arg(COLOR_HEX_WHITE)
        .arg(scale(14)));
    QFont bannerFont("Outfit", 14, QFont::Bold);
    m_participantConnectionErrorBannerLabel->setFont(bannerFont);

    QHBoxLayout* bannerLayout = new QHBoxLayout(m_participantConnectionErrorBanner);
    bannerLayout->setContentsMargins(0, 0, 0, 0);
    bannerLayout->setSpacing(0);
    bannerLayout->addWidget(m_participantConnectionErrorBannerLabel);

    m_buttonsPanel = new QWidget(this);
    m_buttonsPanel->setStyleSheet(StyleCallWidget::panelStyle());
    m_buttonsPanel->setFixedHeight(scale(80));

    m_buttonsLayout = new QHBoxLayout(m_buttonsPanel);
    m_buttonsLayout->setSpacing(scale(15));
    m_buttonsLayout->setContentsMargins(scale(20), scale(10), scale(20), scale(10));
    m_buttonsLayout->setAlignment(Qt::AlignCenter);

    m_screenShareIconNormal = QIcon(":/resources/screenShare.png");
    m_screenShareIconHover = QIcon(":/resources/screenShareHover.png");
    m_screenShareIconActive = QIcon(":/resources/screenShareActive.png");
    m_screenShareIconActiveHover = QIcon(":/resources/screenShareActiveHover.png");
    m_microphoneButton = new ToggleButtonIcon(m_buttonsPanel,
        QIcon(":/resources/microphone.png"),
        QIcon(":/resources/microphoneHover.png"),
        QIcon(":/resources/mute-enabled-microphone.png"),
        QIcon(":/resources/mute-enabled-microphoneHover.png"),
        scale(40), scale(40));
    m_microphoneButton->setSize(scale(37), scale(37));
    m_microphoneButton->setToolTip("Microphone mute");
    m_microphoneButton->setCursor(Qt::PointingHandCursor);

    m_screenShareIconRestricted = QIcon(":/resources/screenShareRestricted.png");
    m_screenShareButton = new ToggleButtonIcon(m_buttonsPanel,
        m_screenShareIconNormal,
        m_screenShareIconHover,
        m_screenShareIconActive,
        m_screenShareIconActiveHover,
        scale(48), scale(48));
    m_screenShareButton->setSize(scale(40), scale(40));
    m_screenShareButton->setToolTip("Share screen");
    m_screenShareButton->setCursor(Qt::PointingHandCursor);

    m_cameraIconActive = QIcon(":/resources/cameraActive.png");
    m_cameraIconActiveHover = QIcon(":/resources/cameraActiveHover.png");
    m_cameraIconRestricted = QIcon(":/resources/cameraRestricted.png");
    m_cameraIconDisabled = QIcon(":/resources/cameraDisabled.png");
    m_cameraIconDisabledHover = QIcon(":/resources/cameraDisabledHover.png");
    m_cameraButton = new ToggleButtonIcon(m_buttonsPanel,
        m_cameraIconDisabled,
        m_cameraIconDisabledHover,
        m_cameraIconActive,
        m_cameraIconActiveHover,
        scale(48), scale(48));
    m_cameraButton->setSize(scale(43), scale(43));
    m_cameraButton->setToolTip("Enable camera");
    m_cameraButton->setCursor(Qt::PointingHandCursor);

    QVBoxLayout* cameraLayout = new QVBoxLayout;
    cameraLayout->setContentsMargins(0, scale(7), 0, 0);
    cameraLayout->setSpacing(0);
    cameraLayout->addWidget(m_cameraButton);

    QWidget* cameraContainer = new QWidget(m_buttonsPanel);
    cameraContainer->setContentsMargins(0, 0, 0 , 0);
    cameraContainer->setLayout(cameraLayout);

    m_fullscreenIconMaximize = QIcon(":/resources/maximize.png");
    m_fullscreenIconMaximizeHover = QIcon(":/resources/maximizeHover.png");
    m_enterFullscreenButton = new ButtonIcon(m_buttonsPanel, scale(32), scale(32));
    m_enterFullscreenButton->setIcons(m_fullscreenIconMaximize, m_fullscreenIconMaximizeHover);
    m_enterFullscreenButton->setSize(scale(28), scale(28));
    m_enterFullscreenButton->setToolTip("Fullscreen");
    m_enterFullscreenButton->setCursor(Qt::PointingHandCursor);
    m_enterFullscreenButton->hide();

    m_hangupButton = new QPushButton(m_buttonsPanel);
    m_hangupButton->setFixedSize(scale(60), scale(60));
    m_hangupButton->setStyleSheet(StyleCallWidget::hangupButtonStyle());
    m_hangupButton->setCursor(Qt::PointingHandCursor);
    m_hangupButton->setIcon(QIcon(":/resources/decline.png"));
    m_hangupButton->setIconSize(QSize(scale(30), scale(30)));
    m_hangupButton->setToolTip("Hang up");
    m_hangupButton->setCursor(Qt::PointingHandCursor);

    m_buttonsLayout->addWidget(m_microphoneButton);
    m_buttonsLayout->addWidget(cameraContainer);
    m_buttonsLayout->addWidget(m_screenShareButton);
    m_buttonsLayout->addWidget(m_enterFullscreenButton);
    m_buttonsLayout->addWidget(m_hangupButton);

    m_fullscreenIconMinimize = QIcon(":/resources/minimize.png");
    m_fullscreenIconMinimizeHover = QIcon(":/resources/minimizeHover.png");
    
    m_exitFullscreenButton = new ButtonIcon(this, scale(28), scale(28));
    m_exitFullscreenButton->setIcons(m_fullscreenIconMinimize, m_fullscreenIconMinimizeHover);
    m_exitFullscreenButton->setSize(scale(38), scale(38));
    m_exitFullscreenButton->setToolTip("Exit fullscreen");
    m_exitFullscreenButton->setCursor(Qt::PointingHandCursor);
    m_exitFullscreenButton->hide();
    m_settingsButton = new ButtonIcon(this, scale(28), scale(28));
    m_settingsButton->setIcons(QIcon(":/resources/settings.png"), QIcon(":/resources/settingsHover.png"));
    m_settingsButton->setSize(scale(38), scale(38));
    m_settingsButton->setToolTip("Audio settings");
    m_settingsButton->setCursor(Qt::PointingHandCursor);
    m_settingsButton->hide();

    m_topMainLayoutSpacer = new QSpacerItem(0, scale(0), QSizePolicy::Minimum, QSizePolicy::Fixed);
    m_middleMainLayoutSpacer = new QSpacerItem(0, scale(15), QSizePolicy::Minimum, QSizePolicy::Fixed);

    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setSpacing(scale(10));
    m_mainLayout->setContentsMargins(scale(0), scale(0), scale(0), scale(0));
    m_mainLayout->setAlignment(Qt::AlignCenter);

    m_mainLayout->addSpacerItem(m_topMainLayoutSpacer);
    m_mainLayout->addWidget(m_timerLabel);
    m_mainLayout->addWidget(m_additionalScreensContainer);
    m_mainLayout->addWidget(m_mainScreen);
    m_mainLayout->addWidget(m_participantInfoContainer);
    m_mainLayout->addSpacerItem(m_middleMainLayoutSpacer);
    m_mainLayout->addWidget(m_buttonsPanel);

    connect(m_exitFullscreenButton, &ButtonIcon::clicked, [this]() {emit requestExitFullscreen(); });
    connect(m_enterFullscreenButton, &ButtonIcon::clicked, [this]() {emit requestEnterFullscreen(); });
    connect(m_microphoneButton, &ToggleButtonIcon::toggled, [this](bool toggled) {
        m_microphoneMuted = toggled;
        emit muteMicrophoneClicked(toggled);
    });
    connect(m_screenShareButton, &ToggleButtonIcon::toggled, [this](bool toggled) {emit screenShareClicked(toggled); });
    connect(m_cameraButton, &ToggleButtonIcon::toggled, [this](bool toggled) {emit cameraClicked(toggled); });
    connect(m_hangupButton, &QPushButton::clicked, [this]() {emit hangupClicked(); });
    connect(m_settingsButton, &ButtonIcon::clicked, [this]()
    {
        emit audioSettingsRequested(m_microphoneMuted, m_speakerMuted, m_inputVolume, m_outputVolume);
    });

    setMouseTracking(true);
    showOverlayButtonWithTimeout();
}

void CallWidget::setupShadowEffect() {
    setupElementShadow(m_timerLabel, scale(15), COLOR_SHADOW_BLACK_60);
    setupElementShadow(m_friendNicknameLabel, scale(10), COLOR_SHADOW_BLACK_50);
    setupElementShadow(m_enterFullscreenButton, scale(10), COLOR_SHADOW_BLACK_50);
    setupElementShadow(m_microphoneButton, scale(10), COLOR_SHADOW_BLACK_50);
    setupElementShadow(m_screenShareButton, scale(10), COLOR_SHADOW_BLACK_50);
    setupElementShadow(m_cameraButton, scale(10), COLOR_SHADOW_BLACK_50);
    setupElementShadow(m_settingsButton, scale(10), COLOR_SHADOW_BLACK_50);
    setupElementShadow(m_hangupButton, scale(10), COLOR_SHADOW_BLACK_50);
    setupElementShadow(m_exitFullscreenButton, scale(10), COLOR_SHADOW_BLACK_50);
}

void CallWidget::setupElementShadow(QWidget* widget, int blurRadius, const QColor& color) {
    if (!widget) return;

    QGraphicsDropShadowEffect* shadowEffect = new QGraphicsDropShadowEffect(this);
    shadowEffect->setBlurRadius(blurRadius);
    shadowEffect->setColor(color);
    shadowEffect->setOffset(0, 3);
    widget->setGraphicsEffect(shadowEffect);
}

void CallWidget::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QPixmap background(":/resources/blur.png");
    painter.drawPixmap(rect(), background);

    QWidget::paintEvent(event);
}

void CallWidget::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    updateParticipantConnectionErrorBannerPosition();
    updateOverlayButtonsPosition();
}

void CallWidget::resizeEvent(QResizeEvent* event)
{
    updateOverlayButtonsPosition();
    updateParticipantConnectionErrorBannerPosition();
    
    if (m_mainScreen->isVisible())
        updateMainScreenSize();
}

void CallWidget::setCallInfo(const QString& friendNickname) {
    m_friendNickname = friendNickname;
    m_friendNicknameLabel->setText(friendNickname);

    *m_callDuration = QTime(0, 0, 0);
    m_timerLabel->setText("00:00");

    m_timerLabel->setStyleSheet(StyleCallWidget::timerStyle());
    QFont timerFont("Outfit", 48, QFont::Bold);
    m_timerLabel->setFont(timerFont);

    m_callTimer->start(TIMER_INTERVAL_MS);
}

void CallWidget::updateCallTimer() {
    *m_callDuration = m_callDuration->addSecs(1);

    bool isLongCall = (m_callDuration->hour() > 0);

    if (isLongCall) {
        QFont timerFont("Pacifico", 52, QFont::Bold);
        m_timerLabel->setFont(timerFont);
    }

    QString timeFormat;
    if (m_callDuration->hour() > 0) {
        timeFormat = "hh:mm:ss";
    }
    else {
        timeFormat = "mm:ss";
    }

    m_timerLabel->setText(m_callDuration->toString(timeFormat));
}

bool CallWidget::isFullScreen() const {
    return m_screenFullscreenActive;
}

bool CallWidget::isMainScreenVisible() const {
    return !m_mainScreen->isHidden();
}

bool CallWidget::isAdditionalScreenVisible(const std::string& screenId) const {
    return m_additionalScreens.contains(screenId);
}

void CallWidget::setInputVolume(int newVolume) {
    m_inputVolume = newVolume;
}

void CallWidget::setOutputVolume(int newVolume) {
    m_outputVolume = newVolume;
}

void CallWidget::setMicrophoneMuted(bool muted) {
    if (m_microphoneButton && m_microphoneButton->isToggled() != muted)
    {
        m_microphoneButton->setToggled(muted);
    }
    m_microphoneMuted = muted;
}

void CallWidget::setSpeakerMuted(bool muted) {
    m_speakerMuted = muted;
}

void CallWidget::applyStandardSize() {
    QSize targetSize;

    QSize availableSize = size();
    targetSize = scaledScreenSize16by9(1440);
    
    int reservedHeight = scale(100);
    int availableHeight = availableSize.height() - reservedHeight;
    if (availableHeight > 0 && targetSize.height() > availableHeight)
    {
        int adjustedWidth = static_cast<int>(availableHeight * 16.0 / 9.0);
        targetSize = QSize(adjustedWidth, availableHeight);
    }

    m_mainScreen->setRoundedCornersEnabled(true);
    m_mainScreen->setMinimumSize(scale(100), scale(100));
    m_mainScreen->setMaximumSize(targetSize);
}

void CallWidget::applyDecreasedSize() {
    QSize targetSize = scaledScreenSize16by9(scale(1280));
    
    QSize availableSize = size();
    int reservedHeight = scale(100) + scale(154);
    int availableHeight = availableSize.height() - reservedHeight;
    if (availableHeight > 0 && targetSize.height() > availableHeight)
    {
        int adjustedWidth = static_cast<int>(availableHeight * 16.0 / 9.0);
        targetSize = QSize(adjustedWidth, availableHeight);
    }
    
    m_mainScreen->setMinimumSize(scale(100), scale(100));
    m_mainScreen->setMaximumSize(targetSize);
}

void CallWidget::applyFullscreenSize()
{
    QSize availableSize = size();
    auto width = availableSize.width();
    auto height = availableSize.height();

    m_mainScreen->setRoundedCornersEnabled(false);
    m_mainScreen->setFixedSize(width, height);
}

void CallWidget::updateMainScreenSize()
{
    if (m_screenFullscreenActive) 
    {
        applyFullscreenSize();
    }
    else 
    {
        if (m_additionalScreens.isEmpty()) {
            applyStandardSize();
        }
        else {
            applyDecreasedSize();
        }
    }
}

QSize CallWidget::scaledScreenSize16by9(int baseWidth)
{
    const int scaledWidth = scale(baseWidth) - scale(15);
    if (scaledWidth <= 0) return QSize();

    const int scaledHeight = std::max(1, static_cast<int>(std::lround(scaledWidth * 9.0 / 16.0)));
    return QSize(scaledWidth, scaledHeight);
}

void CallWidget::showFrameInMainScreen(const QPixmap& frame, Screen::ScaleMode scaleMode)
{
    if (frame.isNull()) return;

    m_mainScreen->setScaleMode(scaleMode);
    m_mainScreen->setPixmap(frame);

    if (!m_mainScreen->isVisible())
    {
        m_mainScreen->show();
        m_timerLabel->hide();
        m_participantInfoContainer->hide();
    }
}

void CallWidget::showFrameInAdditionalScreen(const QPixmap& frame, const std::string& screenId)
{
    if (frame.isNull()) return;

    bool wasEmpty = m_additionalScreens.isEmpty();
    
    if (wasEmpty)
        m_topMainLayoutSpacer->changeSize(0, scale(20), QSizePolicy::Minimum, QSizePolicy::Fixed);

    Screen* screen = nullptr;
    if (m_additionalScreens.contains(screenId))
    {
        screen = m_additionalScreens[screenId];
    }
    else
    {
        screen = new Screen(m_additionalScreensContainer);
        screen->setRoundedCornersEnabled(true);
        screen->setScaleMode(Screen::ScaleMode::CropToFit);

        const int scaledWidth = scale(256);
        const int scaledHeight = scale(144);
        screen->setFixedSize(scaledWidth, scaledHeight);

        m_additionalScreens[screenId] = screen;
        m_additionalScreensLayout->addWidget(screen);
        
        if (wasEmpty && !m_screenFullscreenActive)
        {
            updateMainScreenSize();
        }
    }

    if (m_screenFullscreenActive)
        m_additionalScreensContainer->hide();
    else
        m_additionalScreensContainer->show();

    screen->setPixmap(frame);
}

void CallWidget::removeAdditionalScreen(const std::string& screenId)
{
    if (!m_additionalScreens.contains(screenId)) return;

    Screen* screen = m_additionalScreens[screenId];
    m_additionalScreensLayout->removeWidget(screen);
    screen->deleteLater();
    m_additionalScreens.remove(screenId);

    if (m_additionalScreens.isEmpty())
    {
        m_topMainLayoutSpacer->changeSize(0, 0, QSizePolicy::Minimum, QSizePolicy::Fixed);
        m_additionalScreensContainer->hide();
        
        if (!m_screenFullscreenActive)
            updateMainScreenSize();
    }
}

void CallWidget::enterFullscreen()
{
    m_screenFullscreenActive = true;

    m_topMainLayoutSpacer->changeSize(0, 0, QSizePolicy::Minimum, QSizePolicy::Fixed);
    m_middleMainLayoutSpacer->changeSize(0, 0, QSizePolicy::Minimum, QSizePolicy::Fixed);

    m_enterFullscreenButton->hide();
    m_buttonsPanel->hide();
    m_settingsButton->hide();
    
    m_additionalScreensContainer->hide();

    m_exitFullscreenButton->show();
    updateOverlayButtonsPosition();

    setMouseTracking(true);
    showOverlayButtonWithTimeout();

    m_mainLayout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
    applyFullscreenSize();
}

void CallWidget::exitFullscreen()
{
    m_screenFullscreenActive = false;

    if (m_additionalScreens.isEmpty())
    {
        m_topMainLayoutSpacer->changeSize(0, 0, QSizePolicy::Minimum, QSizePolicy::Fixed);
        m_additionalScreensContainer->hide();
    }
    else
    {
        m_topMainLayoutSpacer->changeSize(0, scale(20), QSizePolicy::Minimum, QSizePolicy::Fixed);
        m_additionalScreensContainer->show();
    }

    m_middleMainLayoutSpacer->changeSize(0, scale(15), QSizePolicy::Minimum, QSizePolicy::Fixed);
    
    m_enterFullscreenButton->show();
    m_buttonsPanel->show();

    m_exitFullscreenButton->hide();
    m_settingsButton->show();
    updateOverlayButtonsPosition();

    if (m_overlayButtonHideTimer) {
        m_overlayButtonHideTimer->stop();
        showOverlayButtonWithTimeout();
    }

    m_mainLayout->setAlignment(Qt::AlignCenter);
    updateMainScreenSize();
}

void CallWidget::hideAdditionalScreens() {
    for (auto it = m_additionalScreens.begin(); it != m_additionalScreens.end(); ++it) {
        Screen* screen = it.value();
        m_additionalScreensLayout->removeWidget(screen);
        screen->deleteLater();
    }
    m_additionalScreens.clear();
     
    m_topMainLayoutSpacer->changeSize(0, 0, QSizePolicy::Minimum, QSizePolicy::Fixed);
    m_additionalScreensContainer->hide();
}

void CallWidget::hideMainScreen()
{
    if (m_mainScreen) {
        m_mainScreen->clear();
        m_mainScreen->hide();
    }

    m_timerLabel->show();
    m_participantInfoContainer->show();
}

void CallWidget::updateOverlayButtonsPosition() {
    int buttonSize = scale(38);
    int margin = scale(10);
    int x = width() - buttonSize - margin;
    int y = margin;

    if (m_exitFullscreenButton) {
        m_exitFullscreenButton->move(x, y);
    }
    if (m_settingsButton) {
        m_settingsButton->move(x, y);
    }
}

void CallWidget::updateParticipantConnectionErrorBannerPosition() {
    if (!m_participantConnectionErrorBanner) return;

    int bannerHeight = scale(40);
    int x = 0;
    int y = 0;
    int bannerWidth = width();

    m_participantConnectionErrorBanner->setGeometry(x, y, bannerWidth, bannerHeight);
    m_participantConnectionErrorBanner->raise();
}

void CallWidget::showOverlayButtonWithTimeout()
{
    if (!m_overlayButtonHideTimer) return;

    if (m_screenFullscreenActive) {
        if (m_settingsButton) m_settingsButton->hide();
        if (m_exitFullscreenButton) m_exitFullscreenButton->show();
    }
    else {
        if (m_exitFullscreenButton) m_exitFullscreenButton->hide();
        if (m_settingsButton) m_settingsButton->show();
    }

    updateOverlayButtonsPosition();
    m_overlayButtonHideTimer->start();
}

void CallWidget::keyPressEvent(QKeyEvent* event)
{
    if (event && event->key() == Qt::Key_Escape && m_screenFullscreenActive)
    {
        emit requestExitFullscreen();
        event->accept();
        return;
    }

    QWidget::keyPressEvent(event);
}

void CallWidget::mousePressEvent(QMouseEvent* event)
{
    showOverlayButtonWithTimeout();
    QWidget::mousePressEvent(event);
}

void CallWidget::mouseMoveEvent(QMouseEvent* event)
{
    showOverlayButtonWithTimeout();
    QWidget::mouseMoveEvent(event);
}

void CallWidget::onExitFullscreenHideTimerTimeout()
{
    if (m_screenFullscreenActive && m_exitFullscreenButton) {
        m_exitFullscreenButton->hide();
    }
    if (!m_screenFullscreenActive && m_settingsButton) {
        m_settingsButton->hide();
    }
}

void CallWidget::setScreenShareButtonRestricted(bool restricted)
{
    if (restricted) {
        m_screenShareButton->setDisabled(true);
        m_screenShareButton->setToolTip("Share disabled");
        m_screenShareButton->setIcons(m_screenShareIconRestricted, m_screenShareIconRestricted, m_screenShareIconRestricted, m_screenShareIconRestricted);
    }
    else {
        m_screenShareButton->setDisabled(false);
        m_screenShareButton->setIcons(m_screenShareIconNormal, m_screenShareIconHover, m_screenShareIconActive, m_screenShareIconActiveHover);
        m_screenShareButton->setToggled(false);
        m_screenShareButton->setToolTip("Start screen share");
    }
}

void CallWidget::setScreenShareButtonActive(bool active)
{
    m_screenShareButton->setDisabled(false);

    if (active) {
        m_screenShareButton->setIcons(m_screenShareIconNormal, m_screenShareIconHover, m_screenShareIconActive, m_screenShareIconActiveHover);
        m_screenShareButton->setToggled(true);
        m_screenShareButton->setToolTip("Stop screen share");
    }
    else {
        m_screenShareButton->setIcons(m_screenShareIconNormal, m_screenShareIconHover, m_screenShareIconActive, m_screenShareIconActiveHover);
        m_screenShareButton->setToggled(false);
        m_screenShareButton->setToolTip("Start screen share");
    }
}

void CallWidget::setCameraButtonRestricted(bool restricted)
{
    if (!m_cameraButton) return;

    if (restricted) {
        m_cameraButton->setDisabled(true);
        m_cameraButton->setToolTip("Camera disabled: screen is being shared or camera is active");
        m_cameraButton->setIcons(m_cameraIconRestricted, m_cameraIconRestricted, m_cameraIconRestricted, m_cameraIconRestricted);
    }
    else {
        m_cameraButton->setDisabled(false);
        m_cameraButton->setIcons(m_cameraIconDisabled, m_cameraIconDisabledHover, m_cameraIconActive, m_cameraIconActiveHover);
        m_cameraButton->setToggled(false);
        m_cameraButton->setToolTip("Enable camera");
    }
    
}

void CallWidget::setCameraButtonActive(bool active)
{
    if (!m_cameraButton) return;

    m_cameraButton->setDisabled(false);

    if (active)
    {
        m_cameraButton->setIcons(m_cameraIconDisabled, m_cameraIconDisabledHover, m_cameraIconActive, m_cameraIconActiveHover);
        m_cameraButton->setToggled(true);
        m_cameraButton->setToolTip("Disable camera");
    }
    else
    {
        m_cameraButton->setIcons(m_cameraIconDisabled, m_cameraIconDisabledHover, m_cameraIconActive, m_cameraIconActiveHover);
        m_cameraButton->setToggled(false);
        m_cameraButton->setToolTip("Enable camera");
    }
}

void CallWidget::setHangupButtonRestricted(bool restricted)
{
    if (!m_hangupButton) return;
    m_hangupButton->setEnabled(!restricted);
    if (restricted) {
        m_hangupButton->setStyleSheet(StyleCallWidget::disabledHangupButtonStyle());
    }
    else {
        m_hangupButton->setStyleSheet(StyleCallWidget::hangupButtonStyle());
    }
}


void CallWidget::showEnterFullscreenButton() {
    m_enterFullscreenButton->show();
}

void CallWidget::hideEnterFullscreenButton() {
    m_enterFullscreenButton->hide();
}