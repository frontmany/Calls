#include "callWidget.h"
#include "incomingCallWidget.h"
#include <QGraphicsDropShadowEffect>
#include <QDialog>
#include <cmath>
#include <algorithm>
#include <string>

#include "scaleFactor.h"

// Style definitions
const QColor StyleCallWidget::m_primaryColor = QColor(21, 119, 232);
const QColor StyleCallWidget::m_hoverColor = QColor(18, 113, 222);
const QColor StyleCallWidget::m_backgroundColor = QColor(230, 230, 230);
const QColor StyleCallWidget::m_textColor = QColor(1, 11, 19);
const QColor StyleCallWidget::m_containerColor = QColor(255, 255, 255, 50);
const QColor StyleCallWidget::m_whiteColor = QColor(255, 255, 255);
const QColor StyleCallWidget::m_controlButtonColor = QColor(255, 255, 255, 180);
const QColor StyleCallWidget::m_controlButtonHoverColor = QColor(255, 255, 255, 220);
const QColor StyleCallWidget::m_hangupButtonColor = QColor(232, 53, 53);
const QColor StyleCallWidget::m_hangupButtonHoverColor = QColor(212, 43, 43);
const QColor StyleCallWidget::m_sliderGrooveColor = QColor(77, 77, 77);
const QColor StyleCallWidget::m_sliderHandleColor = QColor(255, 255, 255);
const QColor StyleCallWidget::m_sliderSubPageColor = QColor(21, 119, 232);
const QColor StyleCallWidget::m_volumeLabelColor = QColor(51, 51, 51);
const QColor StyleCallWidget::m_scrollAreaBackgroundColor = QColor(0, 0, 0, 0);
const QColor StyleCallWidget::m_sliderContainerColor = QColor(255, 255, 255, 120);

QString StyleCallWidget::containerStyle() {
    return QString("QWidget {"
        "   background-color: transparent;"
        "   border-radius: 20px;"
        "   padding: 0px;"
        "}");
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
            background-color: rgb(77, 77, 77);
            border-radius: %6px;
        }
        QSlider::sub-page:horizontal {
            background-color: rgb(21, 119, 232);
            border-radius: %6px;
        }
        QSlider::disabled {
            background-color: transparent;
        }
        QSlider::groove:horizontal:disabled {
            background-color: rgb(180, 180, 180);
        }
        QSlider::handle:horizontal:disabled {
            background-color: rgb(230, 230, 230);
        }
        QSlider::add-page:horizontal:disabled {
            background-color: rgb(180, 180, 180);
        }
        QSlider::sub-page:horizontal:disabled {
            background-color: rgb(150, 150, 150);
        }
    )")
        .arg(QString::fromStdString(std::to_string(scale(8))))
        .arg(QString::fromStdString(std::to_string(scale(4))))
        .arg(QString::fromStdString(std::to_string(scale(17))))
        .arg(QString::fromStdString(std::to_string(scale(17))))
        .arg(QString::fromStdString(std::to_string(scale(8))))
        .arg(QString::fromStdString(std::to_string(scale(4))));
}

QString StyleCallWidget::notificationRedLabelStyle() {
    return QString("QWidget {"
        "   background-color: rgba(252, 121, 121, 100);"
        "   border: none;"
        "   border-radius: %1px;"
        "   margin: 0px;"
        "   padding: 0px;"
        "}").arg(QString::fromStdString(std::to_string(scale(8))));
}

CallWidget::CallWidget(QWidget* parent) : QWidget(parent) {
    setupUI();
    setupShadowEffect();

    m_callTimer = new QTimer(this);
    m_callDuration = new QTime(0, 0, 0);
    connect(m_callTimer, &QTimer::timeout, this, &CallWidget::updateCallTimer);

    m_exitFullscreenHideTimer = new QTimer(this);
    m_exitFullscreenHideTimer->setSingleShot(true);
    m_exitFullscreenHideTimer->setInterval(3000);
    connect(m_exitFullscreenHideTimer, &QTimer::timeout, this, &CallWidget::onExitFullscreenHideTimerTimeout);

    m_notificationTimer = new QTimer(this);
    m_notificationTimer->setSingleShot(true);
    connect(m_notificationTimer, &QTimer::timeout, [this]() { m_notificationWidget->hide(); });
}

void CallWidget::setupUI() {
    setFocusPolicy(Qt::StrongFocus);

    m_incomingCallsContainer = new QWidget(this);
    m_incomingCallsContainer->setStyleSheet("background-color: transparent;");
    m_incomingCallsContainer->hide();

    m_incomingCallsScrollWidget = new QWidget();
    m_incomingCallsScrollWidget->setStyleSheet("background-color: transparent;");

    m_incomingCallsScrollLayout = new QVBoxLayout(m_incomingCallsScrollWidget);
    m_incomingCallsScrollLayout->setContentsMargins(scale(2), scale(2), scale(2), scale(2));
    m_incomingCallsScrollLayout->setSpacing(scale(8));
    m_incomingCallsScrollLayout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    m_incomingCallsScrollArea = new QScrollArea(m_incomingCallsContainer);
    m_incomingCallsScrollArea->setStyleSheet(StyleCallWidget::scrollAreaStyle());
    m_incomingCallsScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_incomingCallsScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_incomingCallsScrollArea->setWidgetResizable(true);
    m_incomingCallsScrollArea->setWidget(m_incomingCallsScrollWidget);

    m_incomingCallsLayout = new QVBoxLayout(m_incomingCallsContainer);
    m_incomingCallsLayout->setContentsMargins(0, 0, 0, 0);
    m_incomingCallsLayout->setSpacing(scale(5));
    m_incomingCallsLayout->addWidget(m_incomingCallsScrollArea);

    m_timerLabel = new QLabel("00:00", this);
    m_timerLabel->setAlignment(Qt::AlignCenter);
    m_timerLabel->setStyleSheet(StyleCallWidget::timerStyle());
    m_timerLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

    m_mainScreen = new Screen(this);
    m_mainScreen->setStyleSheet("background-color: transparent; border: none;");
    m_mainScreen->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    m_mainScreen->setMinimumSize(100, 100);
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

    m_friendNicknameLabel = new QLabel("Friend", this);
    m_friendNicknameLabel->setAlignment(Qt::AlignCenter);
    m_friendNicknameLabel->setStyleSheet(StyleCallWidget::titleStyle());
    m_friendNicknameLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    QFont nicknameFont("Outfit", scale(18), QFont::Normal);
    m_friendNicknameLabel->setFont(nicknameFont);

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

    m_speakerButton = new ToggleButtonIcon(m_buttonsPanel,
        QIcon(":/resources/speaker.png"),
        QIcon(":/resources/speakerHover.png"),
        QIcon(":/resources/speakerActive.png"),
        QIcon(":/resources/speakerActiveHover.png"),
        scale(40), scale(40));
    m_speakerButton->setSize(scale(35), scale(35));
    m_speakerButton->setToolTip("Show volume controls");
    m_speakerButton->setCursor(Qt::PointingHandCursor);

    m_hangupButton = new QPushButton(m_buttonsPanel);
    m_hangupButton->setFixedSize(scale(60), scale(60));
    m_hangupButton->setStyleSheet(StyleCallWidget::hangupButtonStyle());
    m_hangupButton->setCursor(Qt::PointingHandCursor);
    m_hangupButton->setIcon(QIcon(":/resources/decline.png"));
    m_hangupButton->setIconSize(QSize(scale(30), scale(30)));
    m_hangupButton->setToolTip("Hang up");
    m_hangupButton->setCursor(Qt::PointingHandCursor);

    m_buttonsLayout->addWidget(m_screenShareButton);
    m_buttonsLayout->addWidget(cameraContainer);
    m_buttonsLayout->addWidget(m_enterFullscreenButton);
    m_buttonsLayout->addWidget(m_speakerButton);
    m_buttonsLayout->addWidget(m_hangupButton);

    m_fullscreenIconMinimize = QIcon(":/resources/minimize.png");
    m_fullscreenIconMinimizeHover = QIcon(":/resources/minimizeHover.png");
    
    m_exitFullscreenButton = new ButtonIcon(this, scale(28), scale(28));
    m_exitFullscreenButton->setIcons(m_fullscreenIconMinimize, m_fullscreenIconMinimizeHover);
    m_exitFullscreenButton->setSize(scale(38), scale(38));
    m_exitFullscreenButton->setToolTip("Exit fullscreen");
    m_exitFullscreenButton->setCursor(Qt::PointingHandCursor);
    m_exitFullscreenButton->hide();

    m_slidersContainer = new QWidget(this);
    m_slidersContainer->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    m_slidersContainer->setFixedSize(scale(400), scale(110));
    m_slidersContainer->setObjectName("slidersContainer");
    m_slidersContainer->setStyleSheet(StyleCallWidget::sliderContainerStyle());
    m_slidersContainer->hide();

    m_micSliderWidget = new QWidget(m_slidersContainer);
    m_micSliderWidget->setAttribute(Qt::WA_TranslucentBackground);

    m_micLabel = new ToggleButtonIcon(m_micSliderWidget,
        QIcon(":/resources/microphone.png"),
        QIcon(":/resources/microphoneHover.png"),
        QIcon(":/resources/mute-enabled-microphone.png"),
        QIcon(":/resources/mute-enabled-microphoneHover.png"),
        scale(24), scale(24));
    m_micLabel->setSize(scale(24), scale(24));
    m_micLabel->setToolTip("Microphone mute");
    m_micLabel->setCursor(Qt::PointingHandCursor);

    m_micVolumeSlider = new QSlider(Qt::Horizontal, m_micSliderWidget);
    m_micVolumeSlider->setRange(0, 200);
    m_micVolumeSlider->setValue(100);
    m_micVolumeSlider->setStyleSheet(StyleCallWidget::volumeSliderStyle());
    m_micVolumeSlider->setToolTip("Adjust microphone volume");
    m_micVolumeSlider->setTracking(false);
    m_micVolumeSlider->setCursor(Qt::PointingHandCursor);

    m_micLabelSliderLayout = new QHBoxLayout();
    m_micLabelSliderLayout->setSpacing(scale(10));
    m_micLabelSliderLayout->setContentsMargins(0, 0, 0, 0);
    m_micLabelSliderLayout->setAlignment(Qt::AlignCenter);
    m_micLabelSliderLayout->addWidget(m_micLabel);
    m_micLabelSliderLayout->addWidget(m_micVolumeSlider);

    m_micSliderLayout = new QVBoxLayout(m_micSliderWidget);
    m_micSliderLayout->setSpacing(scale(8));
    m_micSliderLayout->setContentsMargins(0, 0, 0, 0);
    m_micSliderLayout->addLayout(m_micLabelSliderLayout);

    m_speakerSliderWidget = new QWidget(m_slidersContainer);
    m_speakerSliderLayout = new QVBoxLayout(m_speakerSliderWidget);
    m_speakerSliderLayout->setSpacing(scale(8));
    m_speakerSliderLayout->setContentsMargins(0, 0, 0, 0);

    m_speakerLabelSliderLayout = new QHBoxLayout();
    m_speakerLabelSliderLayout->setSpacing(scale(10));
    m_speakerLabelSliderLayout->setContentsMargins(0, 0, 0, 0);
    m_speakerLabelSliderLayout->setAlignment(Qt::AlignCenter);

    m_speakerLabel = new ToggleButtonIcon(m_speakerSliderWidget,
        QIcon(":/resources/speaker.png"),
        QIcon(":/resources/speakerHover.png"),
        QIcon(":/resources/speakerMutedActive.png"),
        QIcon(":/resources/speakerMutedActiveHover.png"),
        scale(22), scale(22));
    m_speakerLabel->setSize(scale(22), scale(22));
    m_speakerLabel->setToolTip("Speaker mute");
    m_speakerLabel->setCursor(Qt::PointingHandCursor);

    m_speakerVolumeSlider = new QSlider(Qt::Horizontal, m_speakerSliderWidget);
    m_speakerVolumeSlider->setRange(0, 200);
    m_speakerVolumeSlider->setValue(100);
    m_speakerVolumeSlider->setStyleSheet(StyleCallWidget::volumeSliderStyle());
    m_speakerVolumeSlider->setToolTip("Adjust speaker volume");
    m_speakerVolumeSlider->setTracking(false);
    m_speakerVolumeSlider->setCursor(Qt::PointingHandCursor);

    m_speakerLabelSliderLayout->addWidget(m_speakerLabel);
    m_speakerLabelSliderLayout->addWidget(m_speakerVolumeSlider);
    m_speakerSliderLayout->addLayout(m_speakerLabelSliderLayout);

    m_slidersLayout = new QVBoxLayout(m_slidersContainer);
    m_slidersLayout->setSpacing(scale(20));
    m_slidersLayout->setContentsMargins(scale(20), scale(20), scale(20), scale(20));
    m_slidersLayout->setAlignment(Qt::AlignCenter);
    m_slidersLayout->addWidget(m_micSliderWidget);
    m_slidersLayout->addSpacing(scale(4));
    m_slidersLayout->addWidget(m_speakerSliderWidget);

    m_notificationWidget = new QWidget(this);
    m_notificationWidget->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    m_notificationWidget->hide();
    m_notificationWidget->setStyleSheet(StyleCallWidget::notificationRedLabelStyle());

    m_notificationLayout = new QHBoxLayout(m_notificationWidget);
    m_notificationLayout->setAlignment(Qt::AlignCenter);
    m_notificationLayout->setContentsMargins(scale(18), scale(8), scale(18), scale(8));

    m_notificationLabel = new QLabel(m_notificationWidget);
    QFont errorFont("Outfit", scale(12), QFont::Medium);
    m_notificationLabel->setFont(errorFont);
    m_notificationLabel->setStyleSheet("color: #DC5050; background: transparent; font-size: 14px; margin: 0px; padding: 0px;");
    m_notificationLayout->addWidget(m_notificationLabel);

    m_topMainLayoutSpacer = new QSpacerItem(0, scale(0), QSizePolicy::Minimum, QSizePolicy::Fixed);
    m_middleMainLayoutSpacer = new QSpacerItem(0, scale(15), QSizePolicy::Minimum, QSizePolicy::Fixed);

    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setSpacing(scale(10));
    m_mainLayout->setContentsMargins(scale(0), scale(0), scale(0), scale(0));
    m_mainLayout->setAlignment(Qt::AlignCenter);

    m_mainLayout->addSpacerItem(m_topMainLayoutSpacer);
    m_mainLayout->addWidget(m_notificationWidget, 0, Qt::AlignHCenter);
    m_mainLayout->addWidget(m_incomingCallsContainer);
    m_mainLayout->addWidget(m_timerLabel);
    m_mainLayout->addWidget(m_additionalScreensContainer);
    m_mainLayout->addWidget(m_mainScreen);
    m_mainLayout->addWidget(m_friendNicknameLabel);
    m_mainLayout->addSpacerItem(m_middleMainLayoutSpacer);
    m_mainLayout->addWidget(m_buttonsPanel);
    m_mainLayout->addWidget(m_slidersContainer, 0, Qt::AlignHCenter);

    connect(m_exitFullscreenButton, &ButtonIcon::clicked, [this]() {emit requestExitFullscreen(); });
    connect(m_enterFullscreenButton, &ButtonIcon::clicked, [this]() {emit requestEnterFullscreen(); });
    connect(m_screenShareButton, &ToggleButtonIcon::toggled, [this](bool toggled) {emit screenShareClicked(toggled); });
    connect(m_cameraButton, &ToggleButtonIcon::toggled, [this](bool toggled) {emit cameraClicked(toggled); });
    connect(m_speakerButton, &ToggleButtonIcon::toggled, this, &CallWidget::onSpeakerClicked);
    connect(m_hangupButton, &QPushButton::clicked, [this]() {emit hangupClicked(); });
    connect(m_micLabel, &ToggleButtonIcon::toggled, [this](bool toggled) {m_micVolumeSlider->setEnabled(!toggled); emit muteMicrophoneClicked(toggled); });
    connect(m_speakerLabel, &ToggleButtonIcon::toggled, [this](bool toggled) {m_speakerVolumeSlider->setEnabled(!toggled); emit muteSpeakerClicked(toggled); });
    connect(m_micVolumeSlider, &QSlider::valueChanged, [this](int volume) {emit inputVolumeChanged(volume); });
    connect(m_speakerVolumeSlider, &QSlider::valueChanged, [this](int volume) {emit outputVolumeChanged(volume); });
}

void CallWidget::setupShadowEffect() {
    setupElementShadow(m_timerLabel, 15, QColor(0, 0, 0, 60));
    setupElementShadow(m_friendNicknameLabel, 10, QColor(0, 0, 0, 50));
    setupElementShadow(m_enterFullscreenButton, 10, QColor(0, 0, 0, 50));
    setupElementShadow(m_screenShareButton, 10, QColor(0, 0, 0, 50));
    setupElementShadow(m_cameraButton, 10, QColor(0, 0, 0, 50));
    setupElementShadow(m_speakerButton, 10, QColor(0, 0, 0, 50));
    setupElementShadow(m_hangupButton, 10, QColor(0, 0, 0, 50));
    setupElementShadow(m_slidersContainer, 10, QColor(0, 0, 0, 50));
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

void CallWidget::resizeEvent(QResizeEvent* event)
{
    updateIncomingCallWidths();
    updateExitFullscreenButtonPosition();
    
    if (m_mainScreen->isVisible())
    {
        updateMainScreenSize();
    }
}

void CallWidget::setCallInfo(const QString& friendNickname) {
    m_friendNickname = friendNickname;
    m_friendNicknameLabel->setText(friendNickname);

    *m_callDuration = QTime(0, 0, 0);
    m_timerLabel->setText("00:00");

    m_timerLabel->setStyleSheet(StyleCallWidget::timerStyle());
    QFont timerFont("Outfit", scale(48), QFont::Bold);
    m_timerLabel->setFont(timerFont);

    m_callTimer->start(1000);
}

void CallWidget::updateCallTimer() {
    *m_callDuration = m_callDuration->addSecs(1);

    bool isLongCall = (m_callDuration->hour() > 0);

    if (isLongCall) {
        QFont timerFont("Pacifico", scale(52), QFont::Bold);
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

bool CallWidget::isAdditionalScreenVisible(const std::string& id) const {
    return m_additionalScreens.contains(id);
}

void CallWidget::onSpeakerClicked(bool toggled)
{
    m_slidersVisible = toggled;
    m_slidersContainer->setVisible(toggled);
    
    updateMainScreenSize();
}

void CallWidget::setInputVolume(int newVolume) {
    m_micVolumeSlider->setValue(newVolume);
}

void CallWidget::setOutputVolume(int newVolume) {
    m_speakerVolumeSlider->setValue(newVolume);
}

void CallWidget::setMicrophoneMuted(bool muted) {
    if (m_micLabel && m_micLabel->isToggled() != muted)
    {
        m_micLabel->setToggled(muted);
    }
    m_micVolumeSlider->setEnabled(!muted);
}

void CallWidget::setSpeakerMuted(bool muted) {
    if (m_speakerLabel && m_speakerLabel->isToggled() != muted)
    {
        m_speakerLabel->setToggled(muted);
    }
    m_speakerVolumeSlider->setEnabled(!muted);
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
    m_mainScreen->setMinimumSize(100, 100);
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
    
    m_mainScreen->setMinimumSize(100, 100);
    m_mainScreen->setMaximumSize(targetSize);
}

void CallWidget::applyExtraDecreasedSize() {
    QSize targetSize = scaledScreenSize16by9(scale(820));
    
    QSize availableSize = size();
    int reservedHeight = scale(100) + scale(154) + scale(120);
    int availableHeight = availableSize.height() - reservedHeight;
    if (availableHeight > 0 && targetSize.height() > availableHeight)
    {
        int adjustedWidth = static_cast<int>(availableHeight * 16.0 / 9.0);
        targetSize = QSize(adjustedWidth, availableHeight);
    }
    
    m_mainScreen->setMinimumSize(100, 100);
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
    else if (m_slidersVisible)
    {
        if (m_additionalScreens.isEmpty())
            applyDecreasedSize();
        else
            applyExtraDecreasedSize();
    }
    else
    {
        if (m_additionalScreens.isEmpty())
            applyStandardSize();
        else
            applyDecreasedSize();
    }
}

QSize CallWidget::scaledScreenSize16by9(int baseWidth)
{
    const int scaledWidth = extraScale(baseWidth, 4) - scale(15);
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
        m_friendNicknameLabel->hide();
    }
}

void CallWidget::showFrameInAdditionalScreen(const QPixmap& frame, const std::string& id)
{
    if (frame.isNull()) return;

    bool wasEmpty = m_additionalScreens.isEmpty();
    
    if (wasEmpty)
        m_topMainLayoutSpacer->changeSize(0, scale(20), QSizePolicy::Minimum, QSizePolicy::Fixed);

    Screen* screen = nullptr;
    if (m_additionalScreens.contains(id))
    {
        screen = m_additionalScreens[id];
    }
    else
    {
        screen = new Screen(m_additionalScreensContainer);
        screen->setRoundedCornersEnabled(true);
        screen->setScaleMode(Screen::ScaleMode::CropToFit);

        const int scaledWidth = extraScale(256, 3);
        const int scaledHeight = extraScale(144, 3);
        screen->setFixedSize(scaledWidth, scaledHeight);

        m_additionalScreens[id] = screen;
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

void CallWidget::removeAdditionalScreen(const std::string& id)
{
    if (!m_additionalScreens.contains(id)) return;

    Screen* screen = m_additionalScreens[id];
    m_additionalScreensLayout->removeWidget(screen);
    screen->deleteLater();
    m_additionalScreens.remove(id);

    if (m_additionalScreens.isEmpty())
    {
        m_topMainLayoutSpacer->changeSize(0, 0, QSizePolicy::Minimum, QSizePolicy::Fixed);
        m_additionalScreensContainer->hide();
        
        if (!m_screenFullscreenActive)
        {
            updateMainScreenSize();
        }
    }
}

void CallWidget::restrictScreenShareButton()
{
    m_screenShareButton->setDisabled(true);
    m_screenShareButton->setToolTip("Share disabled: remote screen is being shared");
    m_screenShareButton->setIcons(m_screenShareIconRestricted, m_screenShareIconRestricted, m_screenShareIconRestricted, m_screenShareIconRestricted);
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

void CallWidget::enterFullscreen()
{
    m_screenFullscreenActive = true;

    m_topMainLayoutSpacer->changeSize(0, 0, QSizePolicy::Minimum, QSizePolicy::Fixed);
    m_middleMainLayoutSpacer->changeSize(0, 0, QSizePolicy::Minimum, QSizePolicy::Fixed);

    m_enterFullscreenButton->hide();
    m_buttonsPanel->hide();

    m_speakerButton->setToggled(false);
    m_slidersContainer->hide();
    
    m_additionalScreensContainer->hide();

    m_exitFullscreenButton->show();
    updateExitFullscreenButtonPosition();

    setMouseTracking(true);
    m_exitFullscreenHideTimer->start();

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

    setMouseTracking(false);
    m_exitFullscreenHideTimer->stop();

    m_mainLayout->setAlignment(Qt::AlignCenter);
    applyStandardSize();
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
    m_friendNicknameLabel->show();
}

void CallWidget::updateExitFullscreenButtonPosition() {
    if (!m_exitFullscreenButton->isVisible()) return;

    int buttonSize = scale(38);
    int margin = scale(10);
    int x = width() - buttonSize - margin;
    int y = margin;

    m_exitFullscreenButton->move(x, y);
}

void CallWidget::clearIncomingCalls() {
    for (IncomingCallWidget* callWidget : m_incomingCallWidgets) {
        m_incomingCallsScrollLayout->removeWidget(callWidget);
        callWidget->deleteLater();
    }
    m_incomingCallWidgets.clear();
    updateIncomingCallsVisibility();
}

void CallWidget::onIncomingCallsDialogClosed()
{
	if (m_incomingCallsContainer && m_incomingCallsContainer->parent() == m_incomingCallsDialog)
	{
		if (m_incomingCallsDialog && m_incomingCallsDialog->layout())
			m_incomingCallsDialog->layout()->removeWidget(m_incomingCallsContainer);
		
		m_incomingCallsContainer->setParent(this);
		m_mainLayout->insertWidget(0, m_incomingCallsContainer);
	}

	const QList<QString> names = m_incomingCallWidgets.keys();
	for (const QString& name : names)
	{
		emit declineCallButtonClicked(name);
		removeIncomingCall(name);
	}

	m_incomingCallsContainer->hide();
	m_incomingCallsScrollArea->hide();
}

void CallWidget::addIncomingCall(const QString& friendNickName, int remainingTime) {
    if (m_incomingCallWidgets.contains(friendNickName)) return;

    IncomingCallWidget* callWidget = new IncomingCallWidget(m_incomingCallsScrollWidget, friendNickName, remainingTime);
    callWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_incomingCallsScrollLayout->addWidget(callWidget, 0, Qt::AlignHCenter);
    m_incomingCallWidgets[friendNickName] = callWidget;

    connect(callWidget, &IncomingCallWidget::callAccepted, [this](const QString& callerName) {emit acceptCallButtonClicked(callerName); });
    connect(callWidget, &IncomingCallWidget::callDeclined, [this](const QString& callerName) {emit declineCallButtonClicked(callerName); });

    updateIncomingCallsVisibility();
    updateIncomingCallWidths();
}

void CallWidget::removeIncomingCall(const QString& callerName) {
    if (m_incomingCallWidgets.contains(callerName)) {
        IncomingCallWidget* callWidget = m_incomingCallWidgets[callerName];
        m_incomingCallsScrollLayout->removeWidget(callWidget);
        callWidget->deleteLater();
        m_incomingCallWidgets.remove(callerName);

        updateIncomingCallsVisibility();
        updateIncomingCallWidths();
    }
}

void CallWidget::restoreIncomingCallsContainer() {
    if (m_incomingCallsContainer->parent() != this) {
        if (m_incomingCallsDialog && m_incomingCallsDialog->layout()) {
            m_incomingCallsDialog->layout()->removeWidget(m_incomingCallsContainer);
        }
        m_incomingCallsContainer->setParent(this);
        if (m_mainLayout) {
            m_mainLayout->insertWidget(0, m_incomingCallsContainer);
        }
    }

    m_incomingCallsContainer->hide();
}

void CallWidget::updateIncomingCallsVisibility() {
    if (!m_incomingCallWidgets.isEmpty()) {
        int callCount = m_incomingCallWidgets.size();
        int visibleCount = qMin(callCount, 3);
        int scrollAreaHeight = visibleCount * scale(90);
        int containerHeight = scrollAreaHeight + scale(40);

        if (m_mainScreen && m_mainScreen->isVisible())
		{
			if (!m_incomingCallsDialog)
			{
				m_incomingCallsDialog = new QDialog(window(), Qt::Window);
				m_incomingCallsDialog->setModal(false);
				m_incomingCallsDialog->setStyleSheet("background-color: white;");
				m_incomingCallsDialog->setAttribute(Qt::WA_DeleteOnClose, true);

				QVBoxLayout* dlgLayout = new QVBoxLayout(m_incomingCallsDialog);
				dlgLayout->setContentsMargins(0, 0, 0, 0);
				dlgLayout->setSpacing(0);

				if (m_mainLayout && m_incomingCallsContainer->parent() == this)
				{
					m_mainLayout->removeWidget(m_incomingCallsContainer);
				}
				m_incomingCallsContainer->setParent(m_incomingCallsDialog);
				m_incomingCallsContainer->show();
				dlgLayout->addWidget(m_incomingCallsContainer);

				m_incomingCallsDialog->setMinimumWidth(scale(520));

				connect(m_incomingCallsDialog, &QDialog::rejected, this, &CallWidget::onIncomingCallsDialogClosed);
				connect(m_incomingCallsDialog, &QObject::destroyed, this, [this]() { m_incomingCallsDialog = nullptr; });
			}
			else
			{
				m_incomingCallsDialog->setStyleSheet("background-color: white;");

				if (m_incomingCallsContainer->parent() != m_incomingCallsDialog)
				{
					if (m_mainLayout && m_incomingCallsContainer->parent() == this)
					{
						m_mainLayout->removeWidget(m_incomingCallsContainer);
					}
					m_incomingCallsContainer->setParent(m_incomingCallsDialog);
					m_incomingCallsContainer->show();
					if (QLayout* layout = m_incomingCallsDialog->layout())
					{
						layout->addWidget(m_incomingCallsContainer);
					}
				}

				bool hasRejected = QMetaObject::Connection();
				connect(m_incomingCallsDialog, &QDialog::rejected, this, &CallWidget::onIncomingCallsDialogClosed);
				connect(m_incomingCallsDialog, &QObject::destroyed, this, [this]() { m_incomingCallsDialog = nullptr; });
			}

			m_incomingCallsScrollArea->setFixedHeight(scrollAreaHeight);
			m_incomingCallsContainer->setFixedHeight(containerHeight);
			m_incomingCallsDialog->adjustSize();
			m_incomingCallsDialog->show();
			m_incomingCallsDialog->raise();
			m_incomingCallsDialog->activateWindow();
		}
		else
		{
			if (m_incomingCallsDialog)
			{
				m_incomingCallsDialog->hide();
			}

			if (m_incomingCallsContainer->parent() != this)
			{
				m_incomingCallsContainer->setParent(this);
				if (m_mainLayout)
				{
					m_mainLayout->insertWidget(0, m_incomingCallsContainer);
				}
			}

			m_incomingCallsScrollArea->setFixedHeight(scrollAreaHeight);
			m_incomingCallsContainer->setFixedHeight(containerHeight);
			m_incomingCallsContainer->show();
		}
    } else {
        if (m_incomingCallsContainer) {
            m_incomingCallsContainer->hide();
        }
        if (m_incomingCallsScrollArea) {
            m_incomingCallsScrollArea->hide();
        }
		if (m_incomingCallsDialog) {
			m_incomingCallsDialog->hide();
		}
    }

    updateIncomingCallWidths();
}

void CallWidget::updateIncomingCallWidths()
{
    if (!m_incomingCallsScrollArea) return;

    const int defaultWidth = scale(440);
    const int horizontalPadding = scale(16);
    const int viewportWidth = m_incomingCallsScrollArea->viewport()->width();

    int targetWidth = defaultWidth;

    if (viewportWidth > 0)
    {
        int availableWidth = viewportWidth - horizontalPadding;
        if (availableWidth > 0)
        {
            targetWidth = std::min(defaultWidth, availableWidth);
        }
        else
        {
            targetWidth = std::min(defaultWidth, viewportWidth);
        }
    }

    for (auto it = m_incomingCallWidgets.begin(); it != m_incomingCallWidgets.end(); ++it)
    {
        if (IncomingCallWidget* widget = it.value())
        {
            widget->setFixedWidth(targetWidth);
            widget->updateGeometry();
        }
    }
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
    if (m_screenFullscreenActive && m_exitFullscreenButton)
    {
        m_exitFullscreenButton->show();
        if (m_exitFullscreenHideTimer)
            m_exitFullscreenHideTimer->start();
    }

    QWidget::mousePressEvent(event);
}

void CallWidget::onExitFullscreenHideTimerTimeout()
{
    if (m_screenFullscreenActive && m_exitFullscreenButton)
        m_exitFullscreenButton->hide();
}


void CallWidget::restrictCameraButton()
{
    if (!m_cameraButton) return;

    m_cameraButton->setDisabled(true);
    m_cameraButton->setToolTip("Camera disabled: screen is being shared or camera is active");
    m_cameraButton->setIcons(m_cameraIconRestricted, m_cameraIconRestricted, m_cameraIconRestricted, m_cameraIconRestricted);
}

void CallWidget::showErrorNotification(const QString& text, int durationMs)
{
    if (!m_notificationWidget || !m_notificationLabel) return;

    m_notificationLabel->setText(text);
    m_notificationWidget->show();
    m_notificationTimer->start(durationMs);
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

void CallWidget::showEnterFullscreenButton() {
    m_enterFullscreenButton->show();
}

void CallWidget::hideEnterFullscreenButton() {
    m_enterFullscreenButton->hide();
}