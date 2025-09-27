#include "CallWidget.h"
#include <QPainter>
#include <QPainterPath>
#include <QGraphicsDropShadowEffect>
#include <QPixmap>

// Style definitions
const QColor StyleCallWidget::m_primaryColor = QColor(21, 119, 232);
const QColor StyleCallWidget::m_hoverColor = QColor(18, 113, 222);
const QColor StyleCallWidget::m_backgroundColor = QColor(230, 230, 230);
const QColor StyleCallWidget::m_textColor = QColor(1, 11, 19);
const QColor StyleCallWidget::m_containerColor = QColor(255, 255, 255, 50);

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
        "}").arg(m_textColor.name());
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
        "   color: rgb(255, 255, 255);"
        "   margin: 0px;"
        "   padding: 0px;"
        "}");
}

QString StyleCallWidget::controlButtonStyle() {
    return QString("QPushButton {"
        "   background-color: rgba(255, 255, 255, 180);"
        "   border: none;"
        "   border-radius: 25px;"
        "   padding: 15px;"
        "   margin: 5px;"
        "}"
        "QPushButton:hover {"
        "   background-color: rgba(255, 255, 255, 220);"
        "}");
}

QString StyleCallWidget::hangupButtonStyle() {
    return QString("QPushButton {"
        "   background-color: %1;"
        "   border: none;"
        "   border-radius: 25px;"
        "   padding: 15px;"
        "   margin: 5px;"
        "}"
        "QPushButton:hover {"
        "   background-color: %2;"
        "}").arg(QColor(232, 53, 53).name()).arg(QColor(212, 43, 43).name());
}

QString StyleCallWidget::panelStyle() {
    return QString("QWidget {"
        "   background-color: transparent;"
        "   border-radius: 25px;"
        "   margin: 5px;"
        "   padding: 15px;"
        "}");
}

QString StyleCallWidget::sliderStyle() {
    return QString(
        "QSlider::groove:horizontal {"
        "   border: 1px solid #999999;"
        "   height: 6px;"
        "   background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #B1B1B1, stop:1 #D4D4D4);"
        "   margin: 2px 0;"
        "   border-radius: 3px;"
        "}"
        "QSlider::handle:horizontal {"
        "   background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #1577E8, stop:1 #1E90FF);"
        "   border: 1px solid #5c5c5c;"
        "   width: 16px;"
        "   margin: -6px 0;"
        "   border-radius: 8px;"
        "}"
        "QSlider::handle:horizontal:hover {"
        "   background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #1E90FF, stop:1 #3498db);"
        "}"
    );
}

QString StyleCallWidget::volumeLabelStyle() {
    return QString(
        "QLabel {"
        "   color: #333333;"
        "   font-size: 12px;"
        "   font-weight: bold;"
        "   margin: 2px 0px;"
        "}"
    );
}

CallWidget::CallWidget(QWidget* parent) : QWidget(parent) {
    setupUI();

    // Initialize timer
    m_callTimer = new QTimer(this);
    m_callDuration = QTime(0, 0, 0);
    connect(m_callTimer, &QTimer::timeout, this, &CallWidget::updateCallTimer);

    // Initialize refresh cooldown timer
    m_refreshCooldownTimer = new QTimer(this);
    m_refreshCooldownTimer->setSingleShot(true);
    m_refreshEnabled = true;
    connect(m_refreshCooldownTimer, &QTimer::timeout, this, &CallWidget::onRefreshCooldownFinished);
}

void CallWidget::setupUI() {
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(40, 40, 40, 40);

    // Main container
    m_mainContainer = new QWidget(this);
    m_mainContainer->setFixedWidth(500);
    m_mainContainer->setStyleSheet(StyleCallWidget::containerStyle());

    m_containerLayout = new QVBoxLayout(m_mainContainer);
    m_containerLayout->setSpacing(10);
    m_containerLayout->setContentsMargins(30, 30, 30, 30);
    m_containerLayout->setAlignment(Qt::AlignCenter);

    // Timer
    m_timerLabel = new QLabel("00:00", m_mainContainer);
    m_timerLabel->setAlignment(Qt::AlignCenter);
    m_timerLabel->setStyleSheet(StyleCallWidget::timerStyle());
    m_timerLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    QFont timerFont("Pacifico", 60, QFont::Bold);
    m_timerLabel->setFont(timerFont);

    // Friend nickname
    m_friendNicknameLabel = new QLabel("Friend", m_mainContainer);
    m_friendNicknameLabel->setAlignment(Qt::AlignCenter);
    m_friendNicknameLabel->setStyleSheet(StyleCallWidget::titleStyle());
    m_friendNicknameLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    QFont nicknameFont("Outfit", 18, QFont::Normal);
    m_friendNicknameLabel->setFont(nicknameFont);

    // Buttons panel
    m_buttonsPanel = new QWidget(m_mainContainer);
    m_buttonsPanel->setStyleSheet(StyleCallWidget::panelStyle());
    m_buttonsPanel->setFixedHeight(80);

    m_buttonsLayout = new QHBoxLayout(m_buttonsPanel);
    m_buttonsLayout->setSpacing(15);
    m_buttonsLayout->setContentsMargins(20, 10, 20, 10);
    m_buttonsLayout->setAlignment(Qt::AlignCenter);

    // Mic button
    m_micButton = new ButtonIcon(m_buttonsPanel, QIcon(":/resources/microphone.png"), QIcon(":/resources/microphoneHover.png"), 50, 50);
    m_micButton->setSize(35, 35);

    // Speaker button
    m_speakerButton = new ButtonIcon(m_buttonsPanel, QIcon(":/resources/speaker.png"), QIcon(":/resources/speakerHover.png"), 50, 50);
    m_speakerButton->setSize(35, 35);

    // Refresh button
    m_refreshButton = new ButtonIcon(m_buttonsPanel, QIcon(":/resources/reload.png"), QIcon(":/resources/reloadHover.png"), 40, 40);
    m_refreshButton->setSize(52, 34);
    m_refreshButton->setToolTip("Refresh audio devices if changed");

    // Mute button
    m_muteButton = new ToggleButtonIcon(m_buttonsPanel, QIcon(":/resources/mute-microphone.png"), QIcon(":/resources/mute-microphoneHover.png"), QIcon(":/resources/mute-enabled-microphone.png"), QIcon(":/resources/mute-enabled-microphoneHover.png"), 40, 40);
    m_muteButton->setSize(35, 35);

    // Hangup button
    m_hangupButton = new QPushButton(m_buttonsPanel);
    m_hangupButton->setFixedSize(60, 60);
    m_hangupButton->setStyleSheet(StyleCallWidget::hangupButtonStyle());
    m_hangupButton->setCursor(Qt::PointingHandCursor);
    m_hangupButton->setIcon(QIcon(":/resources/decline.png"));
    m_hangupButton->setIconSize(QSize(30, 30));

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
    m_slidersLayout->setSpacing(30);
    m_slidersLayout->setContentsMargins(20, 10, 20, 10);
    m_slidersLayout->setAlignment(Qt::AlignCenter);

    // Mic volume slider
    m_micSliderWidget = new QWidget(m_slidersPanel);
    m_micSliderLayout = new QVBoxLayout(m_micSliderWidget);
    m_micSliderLayout->setSpacing(5);
    m_micSliderLayout->setContentsMargins(0, 0, 0, 0);

    // Mic label and slider layout
    m_micLabelSliderLayout = new QHBoxLayout();
    m_micLabelSliderLayout->setSpacing(10);
    m_micLabelSliderLayout->setContentsMargins(0, 0, 0, 0);
    m_micLabelSliderLayout->setAlignment(Qt::AlignLeft);

    // Mic label (ButtonIcon like in SettingsPanel)
    m_micLabel = new ButtonIcon(m_micSliderWidget,
        QIcon(":/resources/microphone.png"),
        QIcon(":/resources/microphone.png"),
        24, 24);
    m_micLabel->setSize(24, 24);

    m_micVolumeSlider = new QSlider(Qt::Horizontal, m_micSliderWidget);
    m_micVolumeSlider->setRange(0, 100);
    m_micVolumeSlider->setValue(80);
    m_micVolumeSlider->setStyleSheet(R"(
        QSlider::groove:horizontal {
            background-color: rgb(77, 77, 77); 
            height: 8px; 
            border-radius: 4px;
            margin: 0px 0px; 
        }
        QSlider::handle:horizontal {
            background-color: white;
            width: 16px;
            height: 16px; 
            border-radius: 8px;
            margin: -4px 0;
        }
        QSlider::add-page:horizontal {
            background-color: rgb(77, 77, 77);
            border-radius: 4px;
        }
        QSlider::sub-page:horizontal {
            background-color: rgb(21, 119, 232);
            border-radius: 4px;
        }
    )");

    m_micLabelSliderLayout->addWidget(m_micLabel);
    m_micLabelSliderLayout->addWidget(m_micVolumeSlider);
    m_micSliderLayout->addLayout(m_micLabelSliderLayout);

    // Speaker volume slider
    m_speakerSliderWidget = new QWidget(m_slidersPanel);
    m_speakerSliderLayout = new QVBoxLayout(m_speakerSliderWidget);
    m_speakerSliderLayout->setSpacing(5);
    m_speakerSliderLayout->setContentsMargins(0, 0, 0, 0);

    // Speaker label and slider layout
    m_speakerLabelSliderLayout = new QHBoxLayout();
    m_speakerLabelSliderLayout->setSpacing(10);
    m_speakerLabelSliderLayout->setContentsMargins(0, 0, 0, 0);
    m_speakerLabelSliderLayout->setAlignment(Qt::AlignLeft);

    // Speaker label (ButtonIcon like in SettingsPanel)
    m_speakerLabel = new ButtonIcon(m_speakerSliderWidget,
        QIcon(":/resources/speaker.png"),
        QIcon(":/resources/speaker.png"),
        22, 22);
    m_speakerLabel->setSize(22, 22);

    m_speakerVolumeSlider = new QSlider(Qt::Horizontal, m_speakerSliderWidget);
    m_speakerVolumeSlider->setRange(0, 100);
    m_speakerVolumeSlider->setValue(80);
    m_speakerVolumeSlider->setStyleSheet(R"(
        QSlider::groove:horizontal {
            background-color: rgb(77, 77, 77); 
            height: 8px; 
            border-radius: 4px;
            margin: 0px 0px; 
        }
        QSlider::handle:horizontal {
            background-color: white;
            width: 16px;
            height: 16px; 
            border-radius: 8px;
            margin: -4px 0;
        }
        QSlider::add-page:horizontal {
            background-color: rgb(77, 77, 77);
            border-radius: 4px;
        }
        QSlider::sub-page:horizontal {
            background-color: rgb(21, 119, 232);
            border-radius: 4px;
        }
    )");

    m_speakerLabelSliderLayout->addWidget(m_speakerLabel);
    m_speakerLabelSliderLayout->addWidget(m_speakerVolumeSlider);
    m_speakerSliderLayout->addLayout(m_speakerLabelSliderLayout);

    m_slidersLayout->addWidget(m_micSliderWidget);
    m_slidersLayout->addWidget(m_speakerSliderWidget);

    // Add widgets to main layout
    //m_containerLayout->addWidget(m_titleLabel);
    m_containerLayout->addWidget(m_timerLabel);
    m_containerLayout->addWidget(m_friendNicknameLabel);
    m_containerLayout->addSpacing(10);
    m_containerLayout->addWidget(m_buttonsPanel);
    m_containerLayout->addWidget(m_slidersPanel);

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

    // Setup shadow effect
    QGraphicsDropShadowEffect* shadowEffect = new QGraphicsDropShadowEffect(this);
    shadowEffect->setBlurRadius(20);
    shadowEffect->setColor(QColor(0, 0, 0, 80));
    shadowEffect->setOffset(0, 5);
    m_mainContainer->setGraphicsEffect(shadowEffect);
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
    m_callDuration = QTime(0, 0, 0);
    m_timerLabel->setText("00:00");

    // Ensure initial style is set
    m_timerLabel->setStyleSheet(StyleCallWidget::timerStyle());
    QFont timerFont("Outfit", 48, QFont::Bold);
    m_timerLabel->setFont(timerFont);

    m_callTimer->start(1000); // Update every second
}

void CallWidget::updateCallTimer() {
    m_callDuration = m_callDuration.addSecs(1);

    // Check if call duration exceeds 60 minutes
    bool isLongCall = (m_callDuration.hour() > 0);

    // Update timer style if needed
    if (isLongCall && m_timerLabel->styleSheet() != StyleCallWidget::longTimerStyle()) {
        m_timerLabel->setStyleSheet(StyleCallWidget::longTimerStyle());
        QFont timerFont("Pacifico", 52, QFont::Bold); // Smaller font for long calls
        m_timerLabel->setFont(timerFont);
    }

    // Format time based on duration
    QString timeFormat;
    if (m_callDuration.hour() > 0) {
        timeFormat = "hh:mm:ss"; // Show hours when call exceeds 1 hour
    }
    else {
        timeFormat = "mm:ss"; // Show only minutes and seconds for calls under 1 hour
    }

    m_timerLabel->setText(m_callDuration.toString(timeFormat));
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