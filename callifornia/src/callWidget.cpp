#include "CallWidget.h"
#include "incomingCallWidget.h"
#include <QPainter>
#include <QPainterPath>
#include <QGraphicsDropShadowEffect>
#include <QPixmap>
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

QString StyleCallWidget::containerStyle() {
    return QString("QWidget {"
        "   background-color: transparent;"
        "   border-radius: 20px;"
        "   padding: 0px;"
        "}");
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
            background-color: rgb(77, 77, 77); 
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
    )")
        .arg(QString::fromStdString(std::to_string(scale(8))))
        .arg(QString::fromStdString(std::to_string(scale(4))))
        .arg(QString::fromStdString(std::to_string(scale(16))))
        .arg(QString::fromStdString(std::to_string(scale(16))))
        .arg(QString::fromStdString(std::to_string(scale(8))))
        .arg(QString::fromStdString(std::to_string(scale(4))));
}

CallWidget::CallWidget(QWidget* parent) : QWidget(parent) {
    setupUI();
    setupShadowEffect();

    // Initialize timer
    m_callTimer = new QTimer(this);
    m_callDuration = new QTime(0, 0, 0);
    connect(m_callTimer, &QTimer::timeout, this, &CallWidget::updateCallTimer);

    // Initialize refresh cooldown timer
    m_refreshCooldownTimer = new QTimer(this);
    m_refreshCooldownTimer->setSingleShot(true);
    m_refreshEnabled = true;
    connect(m_refreshCooldownTimer, &QTimer::timeout, this, &CallWidget::onRefreshCooldownFinished);
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

    // Mic button
    m_micButton = new ButtonIcon(m_buttonsPanel, QIcon(":/resources/microphone.png"), QIcon(":/resources/microphoneHover.png"), 50, 50);
    m_micButton->setSize(scale(35), scale(35));

    // Speaker button
    m_speakerButton = new ButtonIcon(m_buttonsPanel, QIcon(":/resources/speaker.png"), QIcon(":/resources/speakerHover.png"), 50, 50);
    m_speakerButton->setSize(scale(35), scale(35));

    // Refresh button
    m_refreshButton = new ButtonIcon(m_buttonsPanel, QIcon(":/resources/reload.png"), QIcon(":/resources/reloadHover.png"), 40, 40);
    m_refreshButton->setSize(scale(52), scale(34));
    m_refreshButton->setToolTip("Refresh audio devices if changed");

    // Mute button
    m_muteButton = new ToggleButtonIcon(m_buttonsPanel, QIcon(":/resources/mute-microphone.png"), QIcon(":/resources/mute-microphoneHover.png"), QIcon(":/resources/mute-enabled-microphone.png"), QIcon(":/resources/mute-enabled-microphoneHover.png"), scale(40), scale(40));
    m_muteButton->setSize(scale(35), scale(35));

    // Hangup button
    m_hangupButton = new QPushButton(m_buttonsPanel);
    m_hangupButton->setFixedSize(scale(60), scale(60));
    m_hangupButton->setStyleSheet(StyleCallWidget::hangupButtonStyle());
    m_hangupButton->setCursor(Qt::PointingHandCursor);
    m_hangupButton->setIcon(QIcon(":/resources/decline.png"));
    m_hangupButton->setIconSize(QSize(scale(30), scale(30)));

    // Add buttons to layout
    m_buttonsLayout->addWidget(m_refreshButton);
    m_buttonsLayout->addWidget(m_muteButton);
    m_buttonsLayout->addWidget(m_micButton);
    m_buttonsLayout->addWidget(m_speakerButton);
    m_buttonsLayout->addWidget(m_hangupButton);

    // Sliders panel (initially hidden)
    m_slidersPanel = new QWidget(m_mainContainer);
    m_slidersPanel->setStyleSheet(StyleCallWidget::panelStyle());
    m_slidersPanel->hide();

    m_slidersLayout = new QHBoxLayout(m_slidersPanel);
    m_slidersLayout->setSpacing(scale(30));
    m_slidersLayout->setContentsMargins(scale(20), scale(10), scale(20), scale(10));
    m_slidersLayout->setAlignment(Qt::AlignCenter);

    // Mic volume slider
    m_micSliderWidget = new QWidget(m_slidersPanel);
    m_micSliderLayout = new QVBoxLayout(m_micSliderWidget);
    m_micSliderLayout->setSpacing(scale(5));
    m_micSliderLayout->setContentsMargins(0, 0, 0, 0);

    // Mic label and slider layout
    m_micLabelSliderLayout = new QHBoxLayout();
    m_micLabelSliderLayout->setSpacing(scale(10));
    m_micLabelSliderLayout->setContentsMargins(0, 0, 0, 0);
    m_micLabelSliderLayout->setAlignment(Qt::AlignLeft);

    // Mic label (ButtonIcon like in SettingsPanel)
    m_micLabel = new ButtonIcon(m_micSliderWidget,
        QIcon(":/resources/microphone.png"),
        QIcon(":/resources/microphone.png"),
        scale(24), scale(24));
    m_micLabel->setSize(scale(24), scale(24));

    m_micVolumeSlider = new QSlider(Qt::Horizontal, m_micSliderWidget);
    m_micVolumeSlider->setRange(0, 100);
    m_micVolumeSlider->setValue(80);
    m_micVolumeSlider->setStyleSheet(StyleCallWidget::volumeSliderStyle());

    m_micLabelSliderLayout->addWidget(m_micLabel);
    m_micLabelSliderLayout->addWidget(m_micVolumeSlider);
    m_micSliderLayout->addLayout(m_micLabelSliderLayout);

    // Speaker volume slider
    m_speakerSliderWidget = new QWidget(m_slidersPanel);
    m_speakerSliderLayout = new QVBoxLayout(m_speakerSliderWidget);
    m_speakerSliderLayout->setSpacing(scale(5));
    m_speakerSliderLayout->setContentsMargins(0, 0, 0, 0);

    // Speaker label and slider layout
    m_speakerLabelSliderLayout = new QHBoxLayout();
    m_speakerLabelSliderLayout->setSpacing(scale(10));
    m_speakerLabelSliderLayout->setContentsMargins(0, 0, 0, 0);
    m_speakerLabelSliderLayout->setAlignment(Qt::AlignLeft);

    // Speaker label (ButtonIcon like in SettingsPanel)
    m_speakerLabel = new ButtonIcon(m_speakerSliderWidget,
        QIcon(":/resources/speaker.png"),
        QIcon(":/resources/speaker.png"),
        scale(22), scale(22));
    m_speakerLabel->setSize(scale(22), scale(22));

    m_speakerVolumeSlider = new QSlider(Qt::Horizontal, m_speakerSliderWidget);
    m_speakerVolumeSlider->setRange(0, 100);
    m_speakerVolumeSlider->setValue(80);
    m_speakerVolumeSlider->setStyleSheet(StyleCallWidget::volumeSliderStyle());

    m_speakerLabelSliderLayout->addWidget(m_speakerLabel);
    m_speakerLabelSliderLayout->addWidget(m_speakerVolumeSlider);
    m_speakerSliderLayout->addLayout(m_speakerLabelSliderLayout);

    m_slidersLayout->addWidget(m_micSliderWidget);
    m_slidersLayout->addWidget(m_speakerSliderWidget);

    // Add widgets to main layout
    m_containerLayout->addWidget(m_incomingCallsContainer);
    m_containerLayout->addStretch();
    m_containerLayout->addWidget(m_timerLabel);
    m_containerLayout->addWidget(m_friendNicknameLabel);
    m_containerLayout->addSpacing(scale(10));
    m_containerLayout->addWidget(m_buttonsPanel);
    m_containerLayout->addWidget(m_slidersPanel);
    m_containerLayout->addStretch();

    m_mainLayout->addWidget(m_mainContainer, 0, Qt::AlignCenter);

    // Connect signals
    connect(m_micButton, &ButtonIcon::clicked, this, &CallWidget::showMicSlider);
    connect(m_muteButton, &ToggleButtonIcon::toggled, this, &CallWidget::onMuteClicked);
    connect(m_speakerButton, &ButtonIcon::clicked, this, &CallWidget::showSpeakerSlider);
    connect(m_hangupButton, &QPushButton::clicked, this, &CallWidget::onHangupClicked);
    connect(m_refreshButton, &ButtonIcon::clicked, this, [this]() {
        if (m_refreshEnabled) {
            // Start cooldown
            m_refreshEnabled = false;
            m_refreshButton->setEnabled(false);

            // Visual feedback for cooldown
            m_refreshButton->setIcons(QIcon(":/resources/reloadDisabled.png"), QIcon(":/resources/reloadDisabled.png")); // Optional: different icon
            m_refreshButton->setToolTip("Refresh cooldown: 2s");

            m_refreshCooldownTimer->start(2000); // 2 seconds cooldown
            emit refreshAudioDevicesButtonClicked();
        }
        });

    // Connect label buttons to show/hide sliders (optional functionality)
    connect(m_micLabel, &ButtonIcon::clicked, this, &CallWidget::showMicSlider);
    connect(m_speakerLabel, &ButtonIcon::clicked, this, &CallWidget::showSpeakerSlider);

    connect(m_micVolumeSlider, &QSlider::valueChanged, this, &CallWidget::onInputVolumeChanged);
    connect(m_speakerVolumeSlider, &QSlider::valueChanged, this, &CallWidget::onOutputVolumeChanged);
}

void CallWidget::setupShadowEffect() {
    setupElementShadow(m_timerLabel, 15, QColor(0, 0, 0, 60));
    setupElementShadow(m_friendNicknameLabel, 10, QColor(0, 0, 0, 50));
    setupElementShadow(m_micButton, 10, QColor(0, 0, 0, 50));
    setupElementShadow(m_speakerButton, 10, QColor(0, 0, 0, 50));
    setupElementShadow(m_refreshButton, 10, QColor(0, 0, 0, 50));
    setupElementShadow(m_muteButton, 10, QColor(0, 0, 0, 50));
    setupElementShadow(m_hangupButton, 10, QColor(0, 0, 0, 50));
}

void CallWidget::setupElementShadow(QWidget* widget, int blurRadius, const QColor& color) {
    if (!widget) return;

    QGraphicsDropShadowEffect* shadowEffect = new QGraphicsDropShadowEffect(this);
    shadowEffect->setBlurRadius(blurRadius);
    shadowEffect->setColor(color);
    shadowEffect->setOffset(0, 3);
    widget->setGraphicsEffect(shadowEffect);
}

void CallWidget::onRefreshCooldownFinished() {
    m_refreshEnabled = true;
    m_refreshButton->setEnabled(true);
    m_refreshButton->setIcons(QIcon(":/resources/reload.png"), QIcon(":/resources/reloadHover.png"));
    m_refreshButton->setToolTip("Refresh audio devices");
}

void CallWidget::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Draw background gradient (same as main menu)
    QLinearGradient gradient(0, 90, width(), height());
    gradient.setColorAt(0.0, QColor(230, 230, 230));
    gradient.setColorAt(0.5, QColor(220, 230, 240));
    gradient.setColorAt(1.0, QColor(240, 240, 240));
    painter.fillRect(rect(), gradient);

    QWidget::paintEvent(event);
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

void CallWidget::updateCallTimer() {
    *m_callDuration = m_callDuration->addSecs(1);

    // Check if call duration exceeds 60 minutes
    bool isLongCall = (m_callDuration->hour() > 0);

    // Update timer style if needed
    if (isLongCall && m_timerLabel->styleSheet() != StyleCallWidget::longTimerStyle()) {
        m_timerLabel->setStyleSheet(StyleCallWidget::longTimerStyle());
        QFont timerFont("Pacifico", scale(52), QFont::Bold); // Smaller font for long calls
        m_timerLabel->setFont(timerFont);
    }

    // Format time based on duration
    QString timeFormat;
    if (m_callDuration->hour() > 0) {
        timeFormat = "hh:mm:ss"; // Show hours when call exceeds 1 hour
    }
    else {
        timeFormat = "mm:ss"; // Show only minutes and seconds for calls under 1 hour
    }

    m_timerLabel->setText(m_callDuration->toString(timeFormat));
}

void CallWidget::showMicSlider() {
    if (m_showingMicSlider) {
        hideSliders();
    }
    else {
        m_showingMicSlider = true;
        m_showingSpeakerSlider = false;
        m_micSliderWidget->show();
        m_speakerSliderWidget->hide();
        m_slidersPanel->show();
    }
}

void CallWidget::showSpeakerSlider() {
    if (m_showingSpeakerSlider) {
        hideSliders();
    }
    else {
        m_showingSpeakerSlider = true;
        m_showingMicSlider = false;
        m_micSliderWidget->hide();
        m_speakerSliderWidget->show();
        m_slidersPanel->show();
    }
}

void CallWidget::hideSliders() {
    m_showingMicSlider = false;
    m_showingSpeakerSlider = false;
    m_slidersPanel->hide();
}

void CallWidget::onMuteClicked()
{
    m_muted = !m_muted;

    if (m_muted)
    {
        if (m_micVolumeSlider)
        {
            m_micVolumeSlider->setEnabled(false);
            emit muteButtonClicked(true);
        }
    }
    else
    {
        if (m_micVolumeSlider)
        {
            m_micVolumeSlider->setEnabled(true);
            emit muteButtonClicked(false);
        }
    }
}

void CallWidget::onRefreshAudioDevicesClicked() {
    emit refreshAudioDevicesButtonClicked();
}

void CallWidget::onInputVolumeChanged(int volume) {
    emit inputVolumeChanged(volume);
}

void CallWidget::onOutputVolumeChanged(int volume) {
    emit outputVolumeChanged(volume);
}

void CallWidget::onHangupClicked() {
    m_callTimer->stop();
    emit hangupClicked();
}

void CallWidget::setInputVolume(int volume) {
    m_micVolumeSlider->setValue(volume);
}

void CallWidget::setOutputVolume(int volume) {
    m_speakerVolumeSlider->setValue(volume);
}

void CallWidget::setMuted(bool muted) {
    if (m_muteButton->isToggled()) {
        m_muteButton->setToggled(muted);
    }
}

QColor CallWidget::generateRandomColor(const QString& seed) {
    int hash = qHash(seed);
    return QColor::fromHsv(hash % 360, 150 + hash % 106, 150 + hash % 106);
}

void CallWidget::clearIncomingCalls() {
    for (IncomingCallWidget* callWidget : m_incomingCallWidgets) {
        m_incomingCallsScrollLayout->removeWidget(callWidget);
        callWidget->deleteLater();
    }
    m_incomingCallWidgets.clear();
    updateIncomingCallsVisibility();
}

void CallWidget::addIncomingCall(const QString& friendNickName) {
    if (m_incomingCallWidgets.contains(friendNickName)) return;

    // Create new incoming call widget
    IncomingCallWidget* callWidget = new IncomingCallWidget(friendNickName, m_incomingCallsScrollWidget);
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
    if (!m_incomingCallWidgets.isEmpty()) {
        // Calculate required height based on number of calls (max 3 visible)
        int callCount = m_incomingCallWidgets.size();
        int visibleCount = qMin(callCount, 3);
        int scrollAreaHeight = visibleCount * scale(90); // 80px per call + spacing
        int containerHeight = scrollAreaHeight + scale(40); // Add space for label

        m_incomingCallsScrollArea->setFixedHeight(scrollAreaHeight);
        m_incomingCallsContainer->setFixedHeight(containerHeight);
        m_incomingCallsContainer->show();
    }
    else {
        m_incomingCallsContainer->hide();
        m_incomingCallsContainer->setFixedHeight(scale(0));
        m_incomingCallsScrollArea->setFixedHeight(scale(0));
    }
}

void CallWidget::onIncomingCallAccepted(const QString& callerName) {
    emit incomingCallAccepted(callerName);
    removeIncomingCall(callerName);
}

void CallWidget::onIncomingCallDeclined(const QString& callerName) {
    emit incomingCallDeclined(callerName);
    removeIncomingCall(callerName);
}