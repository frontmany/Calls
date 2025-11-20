#include "callWidget.h"
#include "incomingCallWidget.h"
#include <QGraphicsDropShadowEffect>
#include <QDialog>
#include <QEvent>
#include <algorithm>
#include <string>
#include "scaleFactor.h"
#include "screen.h"

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

QString StyleCallWidget::longTimerStyle() {
    return QString("QLabel {"
        "   color: %1;"
        "   margin: 0px;"
        "   padding: 0px;"
        "}").arg(m_whiteColor.name());
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
        .arg(QString::fromStdString(std::to_string(scale(25))))  // border-radius
        .arg(QString::fromStdString(std::to_string(scale(15))))  // padding
        .arg(QString::fromStdString(std::to_string(scale(5))));  // margin
}

QString StyleCallWidget::panelStyle() {
    return QString("QWidget {"
        "   background-color: transparent;"
        "   border-radius: %1px;"
        "   margin: %2px;"
        "   padding: %3px;"
        "}")
        .arg(QString::fromStdString(std::to_string(scale(25))))  // border-radius
        .arg(QString::fromStdString(std::to_string(scale(5))))   // margin
        .arg(QString::fromStdString(std::to_string(scale(15)))); // padding
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
        .arg(QString::fromStdString(std::to_string(scale(12))))  // font-size
        .arg(QString::fromStdString(std::to_string(scale(2))))   // margin vertical
        .arg(QString::fromStdString(std::to_string(scale(0))));  // margin horizontal
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

CallWidget::CallWidget(QWidget* parent) : QWidget(parent) {
    setupUI();
    setupShadowEffect();

    m_callTimer = new QTimer(this);
    m_callDuration = new QTime(0, 0, 0);
    connect(m_callTimer, &QTimer::timeout, this, &CallWidget::updateCallTimer);
}

void CallWidget::setupUI()
{
    setFocusPolicy(Qt::StrongFocus);

    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setSpacing(scale(10));
    m_mainLayout->setContentsMargins(scale(0), scale(0), scale(0), scale(0));
    m_mainLayout->setAlignment(Qt::AlignCenter);

    // Incoming calls container (initially hidden)
    m_incomingCallsContainer = new QWidget(this);
    m_incomingCallsContainer->setStyleSheet("background-color: transparent;");
    m_incomingCallsContainer->setFixedHeight(scale(0)); // Start collapsed
    m_incomingCallsContainer->hide();

    m_incomingCallsLayout = new QVBoxLayout(m_incomingCallsContainer);
    m_incomingCallsLayout->setContentsMargins(0, 0, 0, 0);
    m_incomingCallsLayout->setSpacing(scale(5));

    // Scroll area for incoming calls
    m_incomingCallsScrollArea = new QScrollArea(m_incomingCallsContainer);
    m_incomingCallsScrollArea->setStyleSheet(StyleCallWidget::scrollAreaStyle());
    m_incomingCallsScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_incomingCallsScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_incomingCallsScrollArea->setWidgetResizable(true);
    m_incomingCallsScrollArea->setFixedHeight(scale(0)); // Start collapsed

    m_incomingCallsScrollWidget = new QWidget();
    m_incomingCallsScrollWidget->setStyleSheet("background-color: transparent;");

    m_incomingCallsScrollLayout = new QVBoxLayout(m_incomingCallsScrollWidget);
    m_incomingCallsScrollLayout->setContentsMargins(scale(2), scale(2), scale(2), scale(2));
    m_incomingCallsScrollLayout->setSpacing(scale(8));
    m_incomingCallsScrollLayout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    m_incomingCallsScrollArea->setWidget(m_incomingCallsScrollWidget);

    m_incomingCallsLayout->addWidget(m_incomingCallsScrollArea);

    // Timer label
    m_timerLabel = new QLabel("00:00", this);
    m_timerLabel->setAlignment(Qt::AlignCenter);
    m_timerLabel->setStyleSheet(StyleCallWidget::timerStyle());
    m_timerLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

    m_screenWidget = new Screen(this);
    m_screenWidget->setFixedSize(extraScale(1380, 4) - scale(15), extraScale(820, 4) - scale(15));
    m_screenWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_screenWidget->hide();



    // Friend nickname
    m_friendNicknameLabel = new QLabel("Friend", this);
    m_friendNicknameLabel->setAlignment(Qt::AlignCenter);
    m_friendNicknameLabel->setStyleSheet(StyleCallWidget::titleStyle());
    m_friendNicknameLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    QFont nicknameFont("Outfit", scale(18), QFont::Normal);
    m_friendNicknameLabel->setFont(nicknameFont);

    // Buttons panel
    m_buttonsPanel = new QWidget(this);
    m_buttonsPanel->setStyleSheet(StyleCallWidget::panelStyle());
    m_buttonsPanel->setFixedHeight(scale(80));

    m_buttonsLayout = new QHBoxLayout(m_buttonsPanel);
    m_buttonsLayout->setSpacing(scale(15));
    m_buttonsLayout->setContentsMargins(scale(20), scale(10), scale(20), scale(10));
    m_buttonsLayout->setAlignment(Qt::AlignCenter);

    // Screen share button 
    m_screenShareIconNormal = QIcon(":/resources/screenShare.png");
    m_screenShareIconHover = QIcon(":/resources/screenShareHover.png");
    m_screenShareIconActive = QIcon(":/resources/screenShareActive.png");
    m_screenShareIconActiveHover = QIcon(":/resources/screenShareActiveHover.png");
    m_screenShareIconDisabled = QIcon(":/resources/screenShareDisabled.png");
    m_screenShareButton = new ToggleButtonIcon(m_buttonsPanel,
        m_screenShareIconNormal,
        m_screenShareIconHover,
        m_screenShareIconActive,
        m_screenShareIconActiveHover,
        scale(48), scale(48));
    m_screenShareButton->setSize(scale(40), scale(40));
    m_screenShareButton->setToolTip("Share screen");
    m_screenShareButton->setCursor(Qt::PointingHandCursor);

    // Fullscreen button
    m_fullscreenIconMaximize = QIcon(":/resources/maximize.png");
    m_fullscreenIconMaximizeHover = QIcon(":/resources/maximizeHover.png");
    m_fullscreenIconMinimize = QIcon(":/resources/minimize.png");
    m_fullscreenIconMinimizeHover = QIcon(":/resources/minimizeHover.png");
    m_fullscreenButton = new ToggleButtonIcon(m_buttonsPanel,
        m_fullscreenIconMaximize,
        m_fullscreenIconMaximizeHover,
        m_fullscreenIconMinimize,
        m_fullscreenIconMinimizeHover,
        scale(32), scale(32));
    m_fullscreenButton->setSize(scale(28), scale(28));
    m_fullscreenButton->setToolTip("Fullscreen");
    m_fullscreenButton->setCursor(Qt::PointingHandCursor);

    // Speaker button (toggles sliders visibility)
    m_speakerButton = new ToggleButtonIcon(m_buttonsPanel,
        QIcon(":/resources/speaker.png"),
        QIcon(":/resources/speakerHover.png"),
        QIcon(":/resources/speakerActive.png"),
        QIcon(":/resources/speakerActiveHover.png"),
        scale(40), scale(40));
    m_speakerButton->setSize(scale(35), scale(35));
    m_speakerButton->setToolTip("Show volume controls");
    m_speakerButton->setCursor(Qt::PointingHandCursor);

    // Hangup button
    m_hangupButton = new QPushButton(m_buttonsPanel);
    m_hangupButton->setFixedSize(scale(60), scale(60));
    m_hangupButton->setStyleSheet(StyleCallWidget::hangupButtonStyle());
    m_hangupButton->setCursor(Qt::PointingHandCursor);
    m_hangupButton->setIcon(QIcon(":/resources/decline.png"));
    m_hangupButton->setIconSize(QSize(scale(30), scale(30)));
    m_hangupButton->setToolTip("Hang up");
    m_hangupButton->setCursor(Qt::PointingHandCursor);

    // Add buttons to layout
    m_buttonsLayout->addSpacing(scale(10));
    m_buttonsLayout->addWidget(m_screenShareButton);
    m_buttonsLayout->addWidget(m_fullscreenButton);
    m_fullscreenButton->hide(); // Initially hidden, shown only when displaying screen
    m_buttonsLayout->addWidget(m_speakerButton);
    m_buttonsLayout->addWidget(m_hangupButton);

    // Exit fullscreen button (positioned absolutely in top-right corner)
    m_exitFullscreenButton = new QPushButton(this);
    m_exitFullscreenButton->setFixedSize(scale(40), scale(40));
    m_exitFullscreenButton->setStyleSheet(
        QString("QPushButton {"
            "   background-color: rgba(255, 255, 255, 180);"
            "   border: none;"
            "   border-radius: %1px;"
            "}"
            "QPushButton:hover {"
            "   background-color: rgba(255, 255, 255, 220);"
            "}").arg(QString::fromStdString(std::to_string(scale(20)))));
    m_exitFullscreenButton->setCursor(Qt::PointingHandCursor);
    m_exitFullscreenButton->setIcon(m_fullscreenIconMinimize);
    m_exitFullscreenButton->setIconSize(QSize(scale(20), scale(20)));
    m_exitFullscreenButton->setToolTip("Exit fullscreen");
    m_exitFullscreenButton->hide();
    m_exitFullscreenButton->installEventFilter(this);
    connect(m_exitFullscreenButton, &QPushButton::clicked, this, &CallWidget::exitScreenFullscreen);

    // Sliders container (initially hidden)
    m_slidersContainer = new QWidget(this);
    m_slidersContainer->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    m_slidersContainer->setFixedHeight(scale(110));
    m_slidersContainer->setFixedWidth(scale(400));
    m_slidersContainer->setObjectName("slidersContainer");
    m_slidersContainer->setStyleSheet(StyleCallWidget::sliderContainerStyle());
    m_slidersContainer->hide();

    m_slidersLayout = new QVBoxLayout(m_slidersContainer);
    m_slidersLayout->setSpacing(scale(20));
    m_slidersLayout->setContentsMargins(scale(20), scale(20), scale(20), scale(20));
    m_slidersLayout->setAlignment(Qt::AlignCenter);

    // Mic volume slider
    m_micSliderWidget = new QWidget(m_slidersContainer);
    m_micSliderWidget->setAttribute(Qt::WA_TranslucentBackground);
    m_micSliderLayout = new QVBoxLayout(m_micSliderWidget);
    m_micSliderLayout->setSpacing(scale(8));
    m_micSliderLayout->setContentsMargins(0, 0, 0, 0);

    // Mic label and slider layout
    m_micLabelSliderLayout = new QHBoxLayout();
    m_micLabelSliderLayout->setSpacing(scale(10));
    m_micLabelSliderLayout->setContentsMargins(0, 0, 0, 0);
    m_micLabelSliderLayout->setAlignment(Qt::AlignCenter);

    // Mic label (toggle mute)
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
    m_micVolumeSlider->setRange(0, 100);
    m_micVolumeSlider->setValue(80);
    m_micVolumeSlider->setStyleSheet(StyleCallWidget::volumeSliderStyle());
    m_micVolumeSlider->setToolTip("Adjust microphone volume");
    m_micVolumeSlider->setTracking(false);
    m_micVolumeSlider->setCursor(Qt::PointingHandCursor);

    m_micLabelSliderLayout->addWidget(m_micLabel);
    m_micLabelSliderLayout->addWidget(m_micVolumeSlider);
    m_micSliderLayout->addLayout(m_micLabelSliderLayout);

    // Speaker volume slider
    m_speakerSliderWidget = new QWidget(m_slidersContainer);
    m_speakerSliderLayout = new QVBoxLayout(m_speakerSliderWidget);
    m_speakerSliderLayout->setSpacing(scale(8));
    m_speakerSliderLayout->setContentsMargins(0, 0, 0, 0);

    // Speaker label and slider layout
    m_speakerLabelSliderLayout = new QHBoxLayout();
    m_speakerLabelSliderLayout->setSpacing(scale(10));
    m_speakerLabelSliderLayout->setContentsMargins(0, 0, 0, 0);
    m_speakerLabelSliderLayout->setAlignment(Qt::AlignCenter);

    // Speaker label (toggle mute)
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
    m_speakerVolumeSlider->setRange(0, 100);
    m_speakerVolumeSlider->setValue(80);
    m_speakerVolumeSlider->setStyleSheet(StyleCallWidget::volumeSliderStyle());
    m_speakerVolumeSlider->setToolTip("Adjust speaker volume");
    m_speakerVolumeSlider->setTracking(false);
    m_speakerVolumeSlider->setCursor(Qt::PointingHandCursor);

    m_speakerLabelSliderLayout->addWidget(m_speakerLabel);
    m_speakerLabelSliderLayout->addWidget(m_speakerVolumeSlider);
    m_speakerSliderLayout->addLayout(m_speakerLabelSliderLayout);

    // Add sliders to container
    m_slidersLayout->addWidget(m_micSliderWidget);
    m_slidersLayout->addSpacing(scale(4));
    m_slidersLayout->addWidget(m_speakerSliderWidget);


    m_topMainLayoutSpacer = new QSpacerItem(0, scale(0), QSizePolicy::Minimum, QSizePolicy::Fixed);
    m_middleMainLayoutSpacer = new QSpacerItem(0, scale(15), QSizePolicy::Minimum, QSizePolicy::Fixed);

    // Add widgets to main layout
    m_mainLayout->addSpacerItem(m_topMainLayoutSpacer);
    m_mainLayout->addWidget(m_incomingCallsContainer);
    m_mainLayout->addWidget(m_timerLabel);
    m_mainLayout->addWidget(m_screenWidget, 1, Qt::AlignHCenter);
    m_mainLayout->addWidget(m_friendNicknameLabel);
    m_mainLayout->addSpacerItem(m_middleMainLayoutSpacer);
    m_mainLayout->addWidget(m_buttonsPanel, 0, Qt::AlignHCenter);
    m_mainLayout->addWidget(m_slidersContainer, 0, Qt::AlignHCenter);

    // Connect signals
    connect(m_fullscreenButton, &ToggleButtonIcon::toggled, this, &CallWidget::onFullscreenClicked);
    connect(m_speakerButton, &ToggleButtonIcon::toggled, this, &CallWidget::onSpeakerClicked);
    connect(m_hangupButton, &QPushButton::clicked, this, &CallWidget::onHangupClicked);
    connect(m_screenShareButton, &ToggleButtonIcon::toggled, this, &CallWidget::onScreenShareToggled);

    // Connect label toggles to mute actions
    connect(m_micLabel, &ToggleButtonIcon::toggled, this, &CallWidget::onMicLabelToggled);
    connect(m_speakerLabel, &ToggleButtonIcon::toggled, this, &CallWidget::onSpeakerLabelToggled);

    connect(m_micVolumeSlider, &QSlider::sliderReleased, this, &CallWidget::onInputVolumeChanged);
    connect(m_speakerVolumeSlider, &QSlider::sliderReleased, this, &CallWidget::onOutputVolumeChanged);

    updateTopSpacerHeight();
}

void CallWidget::setupShadowEffect() {
    setupElementShadow(m_timerLabel, 15, QColor(0, 0, 0, 60));
    setupElementShadow(m_friendNicknameLabel, 10, QColor(0, 0, 0, 50));
    setupElementShadow(m_fullscreenButton, 10, QColor(0, 0, 0, 50));
    setupElementShadow(m_screenShareButton, 10, QColor(0, 0, 0, 50));
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

    // Draw background image
    QPixmap background(":/resources/blur.png");
    if (!background.isNull()) {
        painter.drawPixmap(rect(), background);
    }
    else {
        // Fallback gradient background if image not found
        QLinearGradient gradient(0, 90, width(), height());
        gradient.setColorAt(0.0, QColor(230, 230, 230));
        gradient.setColorAt(0.5, QColor(220, 230, 240));
        gradient.setColorAt(1.0, QColor(240, 240, 240));
        painter.fillRect(rect(), gradient);
    }

    QWidget::paintEvent(event);
}

void CallWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    updateIncomingCallWidths();
    updateExitFullscreenButtonPosition();
}

void CallWidget::setCallInfo(const QString& friendNickname) {
    m_friendNickname = friendNickname;
    m_friendNicknameLabel->setText(friendNickname);

    // Start the call timer
    *m_callDuration = QTime(0, 0, 0);
    m_timerLabel->setText("00:00");

    // Ensure initial style is set
    m_timerLabel->setStyleSheet(StyleCallWidget::timerStyle());
    QFont timerFont("Outfit", scale(48), QFont::Bold);
    m_timerLabel->setFont(timerFont);

    m_callTimer->start(1000); // Update every second
}

void CallWidget::onScreenShareToggled(bool toggled) {
    emit screenShareClicked(toggled);
}

void CallWidget::updateCallTimer() {
    *m_callDuration = m_callDuration->addSecs(1);

    // Check if call duration exceeds 60 minutes
    bool isLongCall = (m_callDuration->hour() > 0);

    // Update timer style if needed
    if (isLongCall && m_timerLabel->styleSheet() != StyleCallWidget::longTimerStyle()) {
        m_timerLabel->setStyleSheet(StyleCallWidget::longTimerStyle());
        QFont timerFont("Pacifico", scale(52), QFont::Bold);
        m_timerLabel->setFont(timerFont);
    }

    // Format time based on duration
    QString timeFormat;
    if (m_callDuration->hour() > 0) {
        timeFormat = "hh:mm:ss";
    }
    else {
        timeFormat = "mm:ss";
    }

    m_timerLabel->setText(m_callDuration->toString(timeFormat));
}

void CallWidget::onFullscreenClicked(bool toggled)
{
    if (toggled)
    {
        enterScreenFullscreen();
    }
    else
    {
        exitScreenFullscreen();
    }
}

void CallWidget::onSpeakerClicked()
{
    m_slidersVisible = m_speakerButton->isToggled();
    m_slidersContainer->setVisible(m_slidersVisible);
    
    if (m_slidersVisible)
    {
        applyDecreasedSize();
    }
    else
    {
        applyStandardSize();
    }
}

void CallWidget::onInputVolumeChanged() {
    int volume = m_micVolumeSlider->value();
    emit inputVolumeChanged(volume);
}

void CallWidget::onOutputVolumeChanged() {
    int volume = m_speakerVolumeSlider->value();
    emit outputVolumeChanged(volume);
}

void CallWidget::onHangupClicked() {
    if (m_showingDisplay) {
        m_screenShareButton->setToggled(false);
    }

    m_callTimer->stop();
    emit hangupClicked();
}

void CallWidget::onMicLabelToggled(bool toggled)
{
    m_microphoneMuted = toggled;
    m_micVolumeSlider->setEnabled(!toggled);
    emit muteMicrophoneClicked(toggled);
}

void CallWidget::onSpeakerLabelToggled(bool toggled)
{
    m_audioMuted = toggled;
    m_speakerVolumeSlider->setEnabled(!toggled);
    emit muteSpeakerClicked(toggled);
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
    m_microphoneMuted = muted;
    m_micVolumeSlider->setEnabled(!muted);
}

void CallWidget::setSpeakerMuted(bool muted) {
    if (m_speakerLabel && m_speakerLabel->isToggled() != muted)
    {
        m_speakerLabel->setToggled(muted);
    }
}

void CallWidget::setAudioMuted(bool muted) {
    if (m_speakerLabel && m_speakerLabel->isToggled() != muted)
    {
        m_speakerLabel->setToggled(muted);
    }
}

void CallWidget::setShowingDisplayActive(bool active, bool viewingRemoteDisplay)
{
    m_showingDisplay = active;
    m_viewingRemoteDisplay = viewingRemoteDisplay;

    const bool shouldShowFullscreenButton = m_showingDisplay && m_viewingRemoteDisplay;

    if (m_fullscreenButton)
    {
        if (!shouldShowFullscreenButton)
        {
            setFullscreenButtonState(false);
        }

        m_fullscreenButton->setVisible(shouldShowFullscreenButton);
    }

    if (!shouldShowFullscreenButton && m_exitFullscreenButton && m_exitFullscreenButton->isVisible())
    {
        exitScreenFullscreen();
    }

    if (m_showingDisplay)
    {
        m_timerLabel->hide();
        m_friendNicknameLabel->hide();
        m_screenWidget->show();
        if (m_mainLayout)
        {
            m_mainLayout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
            if (m_screenWidget && m_mainLayout->indexOf(m_screenWidget) != -1)
            {
                m_mainLayout->setStretchFactor(m_screenWidget, 1);
            }
        }
    }
    else
    {
        m_screenWidget->clear();
        m_screenWidget->hide();
        m_timerLabel->show();
        m_friendNicknameLabel->show();
        if (m_mainLayout)
        {
            m_mainLayout->setAlignment(Qt::AlignCenter);
            if (m_screenWidget && m_mainLayout->indexOf(m_screenWidget) != -1)
            {
                m_mainLayout->setStretchFactor(m_screenWidget, 0);
            }
        }
    }

    updateIncomingCallsVisibility();
    updateTopSpacerHeight();
    refreshMainLayoutGeometry();
}

QPixmap CallWidget::cropToHorizontal(const QPixmap& pixmap)
{
    if (pixmap.isNull()) return pixmap;

    int w = pixmap.width();
    int h = pixmap.height();

    if (h <= w)
    {
        return pixmap;
    }

    // Crop portrait to 16:9 horizontal area from top
    int targetH = static_cast<int>(w * 9.0 / 16.0);
    targetH = qMin(targetH, h);
    QRect cropRect(0, 0, w, targetH);
    return pixmap.copy(cropRect);
}

void CallWidget::applyStandardSize() {
    QSize targetSize = QSize(extraScale(1380, 4) - scale(15), extraScale(820, 4) - scale(15));
    m_screenWidget->setFixedSize(targetSize);
    m_screenWidget->updateGeometry();
}

void CallWidget::applyDecreasedSize() {
    QSize targetSize = QSize(scale(1080), scale(520));

    int reservedHeight = m_slidersContainer->height();
    reservedHeight += scale(40);

    int adjustedHeight = targetSize.height() - reservedHeight;
    adjustedHeight = std::max(scale(360), adjustedHeight);

    targetSize.setHeight(adjustedHeight);
    

    m_screenWidget->setFixedSize(targetSize);
    m_screenWidget->updateGeometry();
}

void CallWidget::applyIncreasedSize()
{
    QSize targetSize = QSize(extraScale(1652, 4) - scale(15), extraScale(1092, 4) - scale(15));
    m_screenWidget->setFixedSize(targetSize);
    m_screenWidget->updateGeometry();
}

void CallWidget::showFrame(const QPixmap& frame)
{
    if (frame.isNull() || !m_showingDisplay || !m_screenWidget) return;

    QPixmap preparedFrame = cropToHorizontal(frame);
    if (preparedFrame.isNull()) return;

    // Let Screen handle scaling with aspect ratio
    m_screenWidget->setPixmap(preparedFrame);
}

void CallWidget::disableStartScreenShareButton(bool disable) {
    m_screenShareButton->setDisabled(disable);

    if (disable) {    
        m_screenShareButton->setToolTip("Share disabled: remote screen is being shared");
        m_screenShareButton->setIcons(m_screenShareIconDisabled, m_screenShareIconDisabled, m_screenShareIconDisabled, m_screenShareIconDisabled);
    }
    else {
        m_screenShareButton->setIcons(m_screenShareIconNormal, m_screenShareIconHover, m_screenShareIconActive, m_screenShareIconActiveHover);
        m_screenShareButton->setToolTip("Share screen");
        m_screenShareButton->setToggled(false);
    }
}

void CallWidget::setFullscreenButtonState(bool fullscreen)
{
    if (m_fullscreenButton && m_fullscreenButton->isToggled() != fullscreen)
    {
        m_fullscreenButton->setToggled(fullscreen);
    }
}

void CallWidget::enterScreenFullscreen()
{
    m_screenFullscreenActive = true;
    emit requestEnterWindowFullscreen();

    if (m_topMainLayoutSpacer)
    {
        m_topMainLayoutSpacer->changeSize(0, 0, QSizePolicy::Minimum, QSizePolicy::Fixed);
    }

    if (m_buttonsPanel)
    {
        m_buttonsPanel->hide();
    }

    if (m_speakerButton)
    {
        m_speakerButton->setToggled(false);
    }

    if (m_slidersContainer)
    {
        m_slidersContainer->hide();
    }

    if (m_exitFullscreenButton)
    {
        m_exitFullscreenButton->show();
    }

    applyIncreasedSize();
    updateTopSpacerHeight();
    refreshMainLayoutGeometry();
}

void CallWidget::exitScreenFullscreen()
{
    m_screenFullscreenActive = false;

    if (m_buttonsPanel)
    {
        m_buttonsPanel->show();
    }

    if (m_exitFullscreenButton)
    {
        m_exitFullscreenButton->hide();
    }

    updateTopSpacerHeight();
    applyStandardSize();
    setFullscreenButtonState(false);
    refreshMainLayoutGeometry();

    emit requestExitWindowFullscreen();
}

void CallWidget::updateExitFullscreenButtonPosition()
{
    if (!m_exitFullscreenButton || !m_exitFullscreenButton->isVisible())
        return;

    int buttonSize = scale(40);
    int margin = scale(15);
    int x = width() - buttonSize - margin;
    int y = margin;

    m_exitFullscreenButton->move(x, y);
}

void CallWidget::refreshMainLayoutGeometry()
{
    if (m_mainLayout)
    {
        m_mainLayout->invalidate();
        m_mainLayout->activate();
    }

    updateGeometry();
}

void CallWidget::updateTopSpacerHeight()
{
    if (!m_topMainLayoutSpacer)
    {
        return;
    }

    const bool shouldReserveSpace = m_showingDisplay && !m_screenFullscreenActive;
    const int targetHeight = shouldReserveSpace ? scale(40) : 0;
    m_topMainLayoutSpacer->changeSize(0, targetHeight, QSizePolicy::Minimum, QSizePolicy::Fixed);
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
	// Ensure the incoming container returns to main layout before dialog deletes children
	if (m_incomingCallsContainer && m_incomingCallsContainer->parent() == m_incomingCallsDialog)
	{
		// Detach from dialog layout if present
		if (m_incomingCallsDialog && m_incomingCallsDialog->layout())
		{
			m_incomingCallsDialog->layout()->removeWidget(m_incomingCallsContainer);
		}
		m_incomingCallsContainer->setParent(this);
		if (m_mainLayout)
		{
			m_mainLayout->insertWidget(0, m_incomingCallsContainer);
		}
	}

	// Decline and remove all incoming calls
	const QList<QString> names = m_incomingCallWidgets.keys();
	for (const QString& name : names)
	{
		emit declineCallButtonClicked(name);
		removeIncomingCall(name);
	}

	// Ensure container hidden in layout
	if (m_incomingCallsContainer)
	{
		m_incomingCallsContainer->hide();
		m_incomingCallsContainer->setFixedHeight(scale(0));
	}
	if (m_incomingCallsScrollArea)
	{
		m_incomingCallsScrollArea->setFixedHeight(scale(0));
	}
}

void CallWidget::addIncomingCall(const QString& friendNickName, int remainingTime) {
    if (m_incomingCallWidgets.contains(friendNickName)) return;

    // Create new incoming call widget
    IncomingCallWidget* callWidget = new IncomingCallWidget(m_incomingCallsScrollWidget, friendNickName, remainingTime);
    callWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_incomingCallsScrollLayout->addWidget(callWidget, 0, Qt::AlignHCenter);
    m_incomingCallWidgets[friendNickName] = callWidget;

    // Connect signals
    connect(callWidget, &IncomingCallWidget::callAccepted, this, &CallWidget::onIncomingCallAccepted);
    connect(callWidget, &IncomingCallWidget::callDeclined, this, &CallWidget::onIncomingCallDeclined);

    // Show incoming calls section if hidden
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

    m_incomingCallsContainer->setFixedHeight(0);
    m_incomingCallsScrollArea->setFixedHeight(0);
    m_incomingCallsContainer->hide();
}

void CallWidget::updateIncomingCallsVisibility() {
    if (!m_incomingCallWidgets.isEmpty()) {
        int callCount = m_incomingCallWidgets.size();
        int visibleCount = qMin(callCount, 3);
        int scrollAreaHeight = visibleCount * scale(90);
        int containerHeight = scrollAreaHeight + scale(40);

        if (m_showingDisplay)
		{
			// Ensure dialog exists
			if (!m_incomingCallsDialog)
			{
				m_incomingCallsDialog = new QDialog(window(), Qt::Window);
				m_incomingCallsDialog->setModal(false);
				m_incomingCallsDialog->setStyleSheet("background-color: white;");
				m_incomingCallsDialog->setAttribute(Qt::WA_DeleteOnClose, true);

				// Create dialog layout and insert the container
				QVBoxLayout* dlgLayout = new QVBoxLayout(m_incomingCallsDialog);
				dlgLayout->setContentsMargins(0, 0, 0, 0);
				dlgLayout->setSpacing(0);

				// Ensure not in main layout anymore
				if (m_mainLayout && m_incomingCallsContainer->parent() == this)
				{
					m_mainLayout->removeWidget(m_incomingCallsContainer);
				}
				m_incomingCallsContainer->setParent(m_incomingCallsDialog);
				m_incomingCallsContainer->show();
				dlgLayout->addWidget(m_incomingCallsContainer);

				// Reasonable minimum width for readability
				m_incomingCallsDialog->setMinimumWidth(scale(520));

				// Close handlers: decline and clear all, null the pointer
				connect(m_incomingCallsDialog, &QDialog::rejected, this, &CallWidget::onIncomingCallsDialogClosed);
				connect(m_incomingCallsDialog, &QObject::destroyed, this, [this]() { m_incomingCallsDialog = nullptr; });
			}
			else
			{
				m_incomingCallsDialog->setStyleSheet("background-color: white;");

				// If dialog exists but container is not inside it, reparent it
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
				// Ensure connections exist
				bool hasRejected = QMetaObject::Connection();
				connect(m_incomingCallsDialog, &QDialog::rejected, this, &CallWidget::onIncomingCallsDialogClosed);
				connect(m_incomingCallsDialog, &QObject::destroyed, this, [this]() { m_incomingCallsDialog = nullptr; });
			}

			// Size and show
			m_incomingCallsScrollArea->setFixedHeight(scrollAreaHeight);
			m_incomingCallsContainer->setFixedHeight(containerHeight);
			m_incomingCallsDialog->adjustSize();
			m_incomingCallsDialog->show();
			m_incomingCallsDialog->raise();
			m_incomingCallsDialog->activateWindow();
		}
		else
		{
			// Return to layout if dialog was used
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
            m_incomingCallsContainer->setFixedHeight(scale(0));
        }
        if (m_incomingCallsScrollArea) {
            m_incomingCallsScrollArea->setFixedHeight(scale(0));
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

void CallWidget::onIncomingCallAccepted(const QString& callerName) {
    if (m_showingDisplay) {
        restoreIncomingCallsContainer();
    }

    emit acceptCallButtonClicked(callerName);
}

void CallWidget::onIncomingCallDeclined(const QString& callerName) {
    emit declineCallButtonClicked(callerName);

    if (m_showingDisplay) {
        restoreIncomingCallsContainer();
    }
}

bool CallWidget::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == m_exitFullscreenButton)
    {
        if (event->type() == QEvent::Enter)
        {
            m_exitFullscreenButton->setIcon(m_fullscreenIconMinimizeHover);
            return false;
        }
        else if (event->type() == QEvent::Leave)
        {
            m_exitFullscreenButton->setIcon(m_fullscreenIconMinimize);
            return false;
        }
    }

    return QWidget::eventFilter(obj, event);
}

void CallWidget::keyPressEvent(QKeyEvent* event)
{
    if (event && event->key() == Qt::Key_Escape && m_screenFullscreenActive)
    {
        exitScreenFullscreen();
        event->accept();
        return;
    }

    QWidget::keyPressEvent(event);
}