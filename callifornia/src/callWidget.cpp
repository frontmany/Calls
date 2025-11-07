#include "callWidget.h"
#include "incomingCallWidget.h"
#include "screenCaptureController.h"
#include <QPainter>
#include <QPainterPath>
#include <QGraphicsDropShadowEffect>
#include <QPixmap>
#include <QGuiApplication>
#include <QScreen>
#include <QDialog>
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

QString StyleCallWidget::notificationRedLabelStyle() {
    return QString("QWidget {"
        "   background-color: rgba(220, 80, 80, 100);"
        "   border: none;"
        "   border-radius: %1px;"
        "   margin: 0px;"
        "   padding: 0px;"
        "}").arg(QString::fromStdString(std::to_string(scale(8))));
}

CallWidget::CallWidget(QWidget* parent) : QWidget(parent) {
    setupUI();
    setupShadowEffect();

    // Initialize timer
    m_callTimer = new QTimer(this);
    m_callDuration = new QTime(0, 0, 0);
    connect(m_callTimer, &QTimer::timeout, this, &CallWidget::updateCallTimer);

    m_notificationTimer = new QTimer(this);
    m_notificationTimer->setSingleShot(true);
    connect(m_notificationTimer, &QTimer::timeout, [this]() {m_notificationWidget->hide(); });

    // Screen capture controller will be created lazily on first use
}

void CallWidget::setupUI() {
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(scale(40), scale(40), scale(40), scale(40));

    // Main container
    m_mainContainer = new QWidget(this);
    m_mainContainer->setFixedWidth(scale(500));
    m_mainContainer->setStyleSheet(StyleCallWidget::containerStyle());

    m_containerLayout = new QVBoxLayout(m_mainContainer);
    m_containerLayout->setSpacing(scale(10));
    m_containerLayout->setContentsMargins(scale(30), scale(30), scale(30), scale(30));
    m_containerLayout->setAlignment(Qt::AlignCenter);

    // Create network error widget
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

    // Incoming calls container (initially hidden)
    m_incomingCallsContainer = new QWidget(m_mainContainer);
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
    m_incomingCallsScrollLayout->setAlignment(Qt::AlignTop);

    m_incomingCallsScrollArea->setWidget(m_incomingCallsScrollWidget);

    m_incomingCallsLayout->addWidget(m_incomingCallsScrollArea);

    // Timer
    m_timerLabel = new QLabel("00:00", m_mainContainer);
    m_timerLabel->setAlignment(Qt::AlignCenter);
    m_timerLabel->setStyleSheet(StyleCallWidget::timerStyle());
    m_timerLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

    // Share display label (hidden by default)
    m_shareDisplayLabel = new QLabel(m_mainContainer);
    m_shareDisplayLabel->setAlignment(Qt::AlignCenter);
    m_shareDisplayLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_shareDisplayLabel->setMinimumHeight(scale(240));
    m_shareDisplayLabel->setStyleSheet("background: transparent;");
    m_shareDisplayLabel->setScaledContents(true);
    m_shareDisplayLabel->hide();

    // Friend nickname
    m_friendNicknameLabel = new QLabel("Friend", m_mainContainer);
    m_friendNicknameLabel->setAlignment(Qt::AlignCenter);
    m_friendNicknameLabel->setStyleSheet(StyleCallWidget::titleStyle());
    m_friendNicknameLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    QFont nicknameFont("Outfit", scale(18), QFont::Normal);
    m_friendNicknameLabel->setFont(nicknameFont);

    // Buttons panel
    m_buttonsPanel = new QWidget(m_mainContainer);
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

    // Mute microphone button
    m_muteMicrophoneButton = new ToggleButtonIcon(m_buttonsPanel,
        QIcon(":/resources/mute-microphone.png"),
        QIcon(":/resources/mute-microphoneHover.png"),
        QIcon(":/resources/mute-enabled-microphone.png"),
        QIcon(":/resources/mute-enabled-microphoneHover.png"),
        scale(40), scale(40));
    m_muteMicrophoneButton->setSize(scale(35), scale(35));
    m_muteMicrophoneButton->setToolTip("Mute microphone");
    m_muteMicrophoneButton->setCursor(Qt::PointingHandCursor);

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
    m_buttonsLayout->addWidget(m_muteMicrophoneButton);
    m_buttonsLayout->addWidget(m_speakerButton);
    m_buttonsLayout->addWidget(m_hangupButton);

    // Sliders container (initially hidden)
    m_slidersContainer = new QWidget(m_mainContainer);
    m_slidersContainer->setParent(window());
    //m_slidersContainer->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    m_slidersContainer->setFixedWidth(extraScale(400, 3));
    m_slidersContainer->setFixedHeight(scale(110));
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
    m_micLabelSliderLayout->setAlignment(Qt::AlignLeft);

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
    m_speakerLabelSliderLayout->setAlignment(Qt::AlignLeft);

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

    // Add widgets to main layout
    m_containerLayout->addWidget(m_incomingCallsContainer);
    m_containerLayout->addStretch();
    m_containerLayout->addWidget(m_timerLabel);
    m_containerLayout->addWidget(m_shareDisplayLabel, 1);
    m_containerLayout->addWidget(m_friendNicknameLabel);
    m_containerLayout->addSpacing(scale(10));
    m_containerLayout->addWidget(m_buttonsPanel);
    m_containerLayout->addStretch();

    m_mainLayout->addWidget(m_notificationWidget, 0, Qt::AlignCenter);
    m_mainLayout->addSpacing(scale(20));
    m_mainLayout->addWidget(m_mainContainer, 0, Qt::AlignCenter);

    // Connect signals
    // removed: mute audio button no longer exists
    connect(m_muteMicrophoneButton, &ToggleButtonIcon::toggled, this, &CallWidget::onMuteMicrophoneClicked);
    connect(m_speakerButton, &ToggleButtonIcon::toggled, this, &CallWidget::onSpeakerClicked);
    connect(m_hangupButton, &QPushButton::clicked, this, &CallWidget::onHangupClicked);
    connect(m_screenShareButton, &ToggleButtonIcon::toggled, this, &CallWidget::onScreenShareToggled);

    // Connect label toggles to mute actions
    connect(m_micLabel, &ToggleButtonIcon::toggled, this, &CallWidget::onMicLabelToggled);
    connect(m_speakerLabel, &ToggleButtonIcon::toggled, this, &CallWidget::onSpeakerLabelToggled);

    connect(m_micVolumeSlider, &QSlider::sliderReleased, this, &CallWidget::onInputVolumeChanged);
    connect(m_speakerVolumeSlider, &QSlider::sliderReleased, this, &CallWidget::onOutputVolumeChanged);
}

void CallWidget::setupShadowEffect() {
    setupElementShadow(m_timerLabel, 15, QColor(0, 0, 0, 60));
    setupElementShadow(m_friendNicknameLabel, 10, QColor(0, 0, 0, 50));
    // removed: mute audio button
    setupElementShadow(m_muteMicrophoneButton, 10, QColor(0, 0, 0, 50));
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
	if (m_slidersVisible && m_slidersContainer && !m_captureDialogVisible)
	{
		positionSlidersPopup();
		m_slidersContainer->raise();
	}

	if (m_incomingCallsContainer && (m_localSharingActive || m_remoteSharingActive))
	{
		// Position dialog or fallback overlay depending on current mode
		if (m_incomingCallsDialog && m_incomingCallsDialog->isVisible())
		{
			positionIncomingCallsDialog();
		}
		else if (m_incomingCallsOverlay)
		{
			positionIncomingCallsPopup();
			m_incomingCallsContainer->raise();
		}
	}
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

void CallWidget::onScreenShareToggled()
{
    bool toggled = m_screenShareButton->isToggled();

    if (toggled)
    {
        if (m_remoteSharingActive)
        {
            showErrorNotification("Remote screen is being shared", 2000);
            m_screenShareButton->setToggled(false);
            return;
        }

		// Hide sliders while capture dialog is shown
		m_captureDialogVisible = true;
		m_restoreSlidersAfterDialog = (m_slidersContainer && m_slidersContainer->isVisible());
		if (m_slidersContainer) m_slidersContainer->hide();

        emit shareScreenClicked();
        if (!m_screenCaptureController)
        {
            m_screenCaptureController = new ScreenCaptureController(this);
            // For local preview we do not need JPEG encoding each frame
            m_screenCaptureController->setEncodeImageData(false);
            connect(m_screenCaptureController, &ScreenCaptureController::captureStarted, this, &CallWidget::onCaptureStarted);
            connect(m_screenCaptureController, &ScreenCaptureController::captureStopped, this, &CallWidget::onCaptureStopped);
            connect(m_screenCaptureController, &ScreenCaptureController::screenCaptured, this, &CallWidget::onScreenCaptured);
        }
        m_screenCaptureController->showCaptureDialog();
    }
    else
    {
        // Immediately restore timer UI
        m_localSharingActive = false;
        if (m_shareDisplayLabel) m_shareDisplayLabel->clear();
        updateShareDisplayVisibility();

        if (m_screenCaptureController)
        {
            m_screenCaptureController->stopCapture();
        }
    }
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

// removed: onMuteAudioClicked (no separate mute audio button)

void CallWidget::onMuteMicrophoneClicked()
{
    m_microphoneMuted = m_muteMicrophoneButton->isToggled();

    if (m_microphoneMuted) {
        m_micVolumeSlider->setEnabled(false);
    }
    else {
        m_micVolumeSlider->setEnabled(true);
    }

    if (m_micLabel && m_micLabel->isToggled() != m_microphoneMuted)
    {
        m_micLabel->setToggled(m_microphoneMuted);
    }

    emit muteMicrophoneClicked(m_microphoneMuted);
}

void CallWidget::onSpeakerClicked()
{
    m_slidersVisible = m_speakerButton->isToggled();
    updateSlidersVisibility();
	if (m_slidersVisible && !m_captureDialogVisible) {
        positionSlidersPopup();
        m_slidersContainer->raise();
    }
}

void CallWidget::updateSlidersVisibility() {
	if (m_slidersVisible && !m_captureDialogVisible) {
        m_slidersContainer->show();
    }
    else {
        m_slidersContainer->hide();
    }
}

void CallWidget::positionSlidersPopup()
{
    if (!m_slidersContainer || !m_speakerButton) return;

    const QSize popupSize = m_slidersContainer->size();
	int x = 0;
	int y = 0;

	const bool anySharing = m_localSharingActive || m_remoteSharingActive;

	if (anySharing)
	{
		// Position from bottom-left corner of the window client area when sharing is active
		const QWidget* win = window();
		QPoint bottomLeftGlobal = win ? win->mapToGlobal(QPoint(0, win->height())) : QPoint(0, 0);

		const int marginLeft = extraScale(390, 4);
		const int marginBottom = scale(35);

		x = bottomLeftGlobal.x() + marginLeft;
		y = bottomLeftGlobal.y() - marginBottom - popupSize.height();
	}
	else
	{
		// Default positioning relative to the speaker button
		const QRect buttonRect = m_speakerButton->rect();
		const QPoint buttonBottomCenter = m_speakerButton->mapToGlobal(QPoint(buttonRect.center().x(), buttonRect.bottom()));
		x = buttonBottomCenter.x() - popupSize.width() / 2;
        x -= scale(15);

		y = buttonBottomCenter.y() + scale(8);
        y += scale(15);
	}

	QPoint screenAnchor = QPoint(x, y);
	QScreen* screen = QGuiApplication::screenAt(screenAnchor);
    if (!screen) screen = QGuiApplication::primaryScreen();
    if (screen)
    {
        const QRect avail = screen->availableGeometry();
		if (x + popupSize.width() > avail.right()) x = avail.right() - popupSize.width();
		if (x < avail.left()) x = avail.left();
		if (y + popupSize.height() > avail.bottom())
        {
			// For non-sharing fallback, place above anchor; for sharing, just clamp to bottom
			if (!anySharing)
			{
				const QRect buttonRect = m_speakerButton->rect();
				y = m_speakerButton->mapToGlobal(QPoint(buttonRect.center().x(), buttonRect.top())).y() - scale(8) - popupSize.height();
			}
			if (y < avail.top()) y = avail.top();
        }
		if (y < avail.top()) y = avail.top();
    }

	// Convert to parent (window) coordinates before moving
	if (QWidget* win = window())
	{
		QPoint localPos = win->mapFromGlobal(QPoint(x, y));
		m_slidersContainer->move(localPos);
	}
	else
	{
		m_slidersContainer->move(x, y);
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
    m_callTimer->stop();
    emit hangupClicked();
}

void CallWidget::onMicLabelToggled(bool toggled)
{
    m_microphoneMuted = toggled;
    m_micVolumeSlider->setEnabled(!toggled);
    if (m_muteMicrophoneButton && m_muteMicrophoneButton->isToggled() != toggled)
    {
        m_muteMicrophoneButton->setToggled(toggled);
    }
    emit muteMicrophoneClicked(toggled);
}

void CallWidget::onSpeakerLabelToggled(bool toggled)
{
    m_audioMuted = toggled;
    m_speakerVolumeSlider->setEnabled(!toggled);
    emit muteSpeakerClicked(toggled);
}

void CallWidget::onCaptureStarted()
{
    m_localSharingActive = true;
	// Dialog is now closed
	m_captureDialogVisible = false;
	// Hide incoming container from layout immediately
	if (m_incomingCallsContainer)
	{
		m_incomingCallsContainer->hide();
	}
	if (m_restoreSlidersAfterDialog)
	{
		m_restoreSlidersAfterDialog = false;
		if (m_slidersVisible)
		{
			updateSlidersVisibility();
			if (m_slidersContainer->isVisible())
			{
				positionSlidersPopup();
				m_slidersContainer->raise();
			}
		}
	}
    updateShareDisplayVisibility();
    updateIncomingCallsVisibility();
}

void CallWidget::onCaptureStopped()
{
    m_localSharingActive = false;
    emit shareScreenStopped();
    if (m_screenShareButton && m_screenShareButton->isToggled())
    {
        m_screenShareButton->setToggled(false);
    }
	// Dialog may have been closed without starting capture
	m_captureDialogVisible = false;
	if (m_restoreSlidersAfterDialog)
	{
		m_restoreSlidersAfterDialog = false;
		if (m_slidersVisible)
		{
			updateSlidersVisibility();
			if (m_slidersContainer->isVisible())
			{
				positionSlidersPopup();
				m_slidersContainer->raise();
			}
		}
	}
    updateShareDisplayVisibility();
    updateIncomingCallsVisibility();
}

void CallWidget::onScreenCaptured(const QPixmap& pixmap, const std::string& /*imageData*/)
{
    if (!m_localSharingActive) return;

    QPixmap processed = cropToHorizontal(pixmap);
    if (!processed.isNull())
    {
        QSize labelSize = m_shareDisplayLabel->size();
        QPixmap scaled = processed.scaled(labelSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        m_shareDisplayLabel->setPixmap(scaled);
    }
}

void CallWidget::setInputVolume(int newVolume) {
    m_micVolumeSlider->setValue(newVolume);
}

void CallWidget::setOutputVolume(int newVolume) {
    m_speakerVolumeSlider->setValue(newVolume);
}

void CallWidget::setMicrophoneMuted(bool muted) {
    if (m_muteMicrophoneButton->isToggled() != muted) {
        m_muteMicrophoneButton->setToggled(muted);
    }
    if (m_micLabel && m_micLabel->isToggled() != muted)
    {
        m_micLabel->setToggled(muted);
    }
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

QColor CallWidget::generateRandomColor(const QString& seed) {
    int hash = qHash(seed);
    return QColor::fromHsv(hash % 360, 150 + hash % 106, 150 + hash % 106);
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

    // Crop portrait to 16:9 horizontal area centered
    int targetH = static_cast<int>(w * 9.0 / 16.0);
    targetH = qMin(targetH, h);
    int y = (h - targetH) / 2;
    QRect cropRect(0, y, w, targetH);
    return pixmap.copy(cropRect);
}

void CallWidget::updateShareDisplayVisibility()
{
    bool anySharing = m_localSharingActive || m_remoteSharingActive;
    if (anySharing)
    {
        m_timerLabel->hide();

        m_mainLayout->setContentsMargins(scale(10), scale(10), scale(10), scale(10));
        m_containerLayout->setContentsMargins(scale(10), scale(10), scale(10), scale(10));

        m_mainContainer->setMinimumWidth(extraScale(1450, 5));

        // Let layout stretch allocate the remaining space without forcing parent to grow
        m_shareDisplayLabel->setMinimumHeight(extraScale(820, 5));
        m_shareDisplayLabel->show();
		if (m_slidersVisible && !m_captureDialogVisible)
		{
			positionSlidersPopup();
			m_slidersContainer->raise();
		}
        updateIncomingCallsVisibility();
    }
    else
    {
        m_shareDisplayLabel->hide();
        m_timerLabel->show();

        m_mainLayout->setContentsMargins(scale(40), scale(40), scale(40), scale(40));
        m_containerLayout->setContentsMargins(scale(30), scale(30), scale(30), scale(30));
        m_mainContainer->setFixedWidth(scale(500));
		if (m_slidersVisible && !m_captureDialogVisible)
		{
			positionSlidersPopup();
			m_slidersContainer->raise();
		}
        updateIncomingCallsVisibility();
    }
}

void CallWidget::setRemoteSharingActive(bool active)
{
    m_remoteSharingActive = active;

    if (m_remoteSharingActive && m_localSharingActive)
    {
        if (m_screenCaptureController)
        {
            m_screenCaptureController->stopCapture();
            m_screenCaptureController->hideCaptureDialog();
        }
        if (m_screenShareButton && m_screenShareButton->isToggled())
        {
            m_screenShareButton->setToggled(false);
        }
        m_localSharingActive = false;
    }

    // Disable local share button while remote is sharing
    if (m_screenShareButton)
    {
        m_screenShareButton->setEnabled(!m_remoteSharingActive);
        if (m_remoteSharingActive)
        {
            if (m_screenShareButton->isToggled())
            {
                m_screenShareButton->setToggled(false);
            }
            m_screenShareButton->setToolTip("Share disabled: remote screen is being shared");
            // Use disabled icon for all states
            m_screenShareButton->setIcons(m_screenShareIconDisabled, m_screenShareIconDisabled, m_screenShareIconDisabled, m_screenShareIconDisabled);
        }
        else
        {
            // Restore normal icons
            m_screenShareButton->setIcons(m_screenShareIconNormal, m_screenShareIconHover, m_screenShareIconActive, m_screenShareIconActiveHover);
            m_screenShareButton->setToolTip("Share screen");
        }
    }

    updateShareDisplayVisibility();
}

void CallWidget::showRemoteShareFrame(const QPixmap& frame)
{
    if (!m_remoteSharingActive) return;
    if (frame.isNull()) return;
    QSize labelSize = m_shareDisplayLabel->size();
    QPixmap scaled = frame.scaled(labelSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    m_shareDisplayLabel->setPixmap(scaled);
}

void CallWidget::positionIncomingCallsPopup()
{
    if (!m_incomingCallsContainer) return;

    const QSize popupSize = m_incomingCallsContainer->size();
    int x = 0;
    int y = 0;

    const QWidget* win = window();
    QPoint topLeftGlobal = win ? win->mapToGlobal(QPoint(0, 0)) : QPoint(0, 0);

    const int marginLeft = scale(20);
    const int marginTop = scale(20);

    x = topLeftGlobal.x() + marginLeft;
    y = topLeftGlobal.y() + marginTop;

    QPoint screenAnchor = QPoint(x, y);
    QScreen* screen = QGuiApplication::screenAt(screenAnchor);
    if (!screen) screen = QGuiApplication::primaryScreen();
    if (screen)
    {
        const QRect avail = screen->availableGeometry();
        if (x + popupSize.width() > avail.right()) x = qMax(avail.left(), avail.right() - popupSize.width());
        if (y + popupSize.height() > avail.bottom()) y = qMax(avail.top(), avail.bottom() - popupSize.height());
        if (x < avail.left()) x = avail.left();
        if (y < avail.top()) y = avail.top();
    }

    if (QWidget* winw = window())
    {
        QPoint localPos = winw->mapFromGlobal(QPoint(x, y));
        m_incomingCallsContainer->move(localPos);
    }
    else
    {
        m_incomingCallsContainer->move(x, y);
    }
}

void CallWidget::positionIncomingCallsDialog()
{
	if (!m_incomingCallsDialog) return;

	const QWidget* win = window();
	if (!win) return;

	// Position near top-left of the main window with margins
	const int marginLeft = scale(20);
	const int marginTop = scale(20);

	QPoint topLeftGlobal = win->mapToGlobal(QPoint(0, 0));
	int x = topLeftGlobal.x() + marginLeft;
	int y = topLeftGlobal.y() + marginTop;

	// Ensure the dialog stays within available screen
	QScreen* screen = QGuiApplication::screenAt(QPoint(x, y));
	if (!screen) screen = QGuiApplication::primaryScreen();
	if (screen)
	{
		const QRect avail = screen->availableGeometry();
		QSize dlgSize = m_incomingCallsDialog->size();
		if (x + dlgSize.width() > avail.right()) x = qMax(avail.left(), avail.right() - dlgSize.width());
		if (y + dlgSize.height() > avail.bottom()) y = qMax(avail.top(), avail.bottom() - dlgSize.height());
		if (x < avail.left()) x = avail.left();
		if (y < avail.top()) y = avail.top();
	}

	m_incomingCallsDialog->move(x, y);
	m_incomingCallsDialog->raise();
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
		m_incomingCallsContainer->setParent(m_mainContainer);
		if (m_containerLayout)
		{
			m_containerLayout->insertWidget(0, m_incomingCallsContainer);
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
void CallWidget::showErrorNotification(const QString& text, int durationMs) {
    m_notificationLabel->setText(text);
    m_notificationWidget->show();
    m_notificationTimer->start(durationMs);
}

void CallWidget::addIncomingCall(const QString& friendNickName, int remainingTime) {
    if (m_incomingCallWidgets.contains(friendNickName)) return;

    // Create new incoming call widget
    IncomingCallWidget* callWidget = new IncomingCallWidget(m_incomingCallsScrollWidget, friendNickName, remainingTime);
    setupElementShadow(callWidget, 10, QColor(0, 0, 3, 50));
    m_incomingCallsScrollLayout->addWidget(callWidget);
    m_incomingCallWidgets[friendNickName] = callWidget;

    // Connect signals
    connect(callWidget, &IncomingCallWidget::callAccepted, this, &CallWidget::onIncomingCallAccepted);
    connect(callWidget, &IncomingCallWidget::callDeclined, this, &CallWidget::onIncomingCallDeclined);

    // Show incoming calls section if hidden
    updateIncomingCallsVisibility();
}

void CallWidget::removeIncomingCall(const QString& callerName) {
    if (m_incomingCallWidgets.contains(callerName)) {
        IncomingCallWidget* callWidget = m_incomingCallWidgets[callerName];
        m_incomingCallsScrollLayout->removeWidget(callWidget);
        callWidget->deleteLater();
        m_incomingCallWidgets.remove(callerName);

        updateIncomingCallsVisibility();
    }
}

void CallWidget::updateIncomingCallsVisibility() {
    const bool anySharing = m_localSharingActive || m_remoteSharingActive;

    if (!m_incomingCallWidgets.isEmpty()) {
        int callCount = m_incomingCallWidgets.size();
        int visibleCount = qMin(callCount, 3);
        int scrollAreaHeight = visibleCount * scale(90);
        int containerHeight = scrollAreaHeight + scale(40);

        if (anySharing)
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
				if (m_containerLayout && m_incomingCallsContainer->parent() == m_mainContainer)
				{
					m_containerLayout->removeWidget(m_incomingCallsContainer);
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
					if (m_containerLayout && m_incomingCallsContainer->parent() == m_mainContainer)
					{
						m_containerLayout->removeWidget(m_incomingCallsContainer);
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
			positionIncomingCallsDialog();
		}
		else
		{
			// Return to layout if dialog was used
			if (m_incomingCallsDialog)
			{
				m_incomingCallsDialog->hide();
			}

			if (m_incomingCallsContainer->parent() != m_mainContainer)
			{
				m_incomingCallsContainer->setParent(m_mainContainer);
				if (m_containerLayout)
				{
					m_containerLayout->insertWidget(0, m_incomingCallsContainer);
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
}

void CallWidget::onIncomingCallAccepted(const QString& callerName) {
    emit acceptCallButtonClicked(callerName);
}

void CallWidget::onIncomingCallDeclined(const QString& callerName) {
    emit declineCallButtonClicked(callerName);
}