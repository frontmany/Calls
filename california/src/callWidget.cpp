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
        "   font-size: 48px;"
        "   font-weight: bold;"
        "}").arg(m_textColor.name());
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
        "   background-color: rgba(245, 245, 245, 150);"
        "   border-radius: 25px;"
        "   margin: 10px;"
        "   padding: 15px;"
        "}");
}

CallWidget::CallWidget(QWidget* parent) : QWidget(parent) {
    setupUI();

    // Initialize timer
    m_callTimer = new QTimer(this);
    m_callDuration = QTime(0, 0, 0);
    connect(m_callTimer, &QTimer::timeout, this, &CallWidget::updateCallTimer);
}

void CallWidget::setupUI() {
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(40, 40, 40, 40);

    // Main container
    m_mainContainer = new QWidget(this);
    m_mainContainer->setFixedWidth(500);
    m_mainContainer->setStyleSheet(StyleCallWidget::containerStyle());

    m_containerLayout = new QVBoxLayout(m_mainContainer);
    m_containerLayout->setSpacing(20);
    m_containerLayout->setContentsMargins(30, 30, 30, 30);
    m_containerLayout->setAlignment(Qt::AlignCenter);

    // Title
    m_titleLabel = new QLabel("Call in Progress", m_mainContainer);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_titleLabel->setStyleSheet(StyleCallWidget::titleStyle());
    m_titleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    QFont titleFont("Outfit", 24, QFont::Normal);
    m_titleLabel->setFont(titleFont);

    // Timer
    m_timerLabel = new QLabel("00:00", m_mainContainer);
    m_timerLabel->setAlignment(Qt::AlignCenter);
    m_timerLabel->setStyleSheet(StyleCallWidget::timerStyle());
    m_timerLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    QFont timerFont("Outfit", 48, QFont::Bold);
    m_timerLabel->setFont(timerFont);

    // Friend nickname
    m_friendNicknameLabel = new QLabel("Friend", m_mainContainer);
    m_friendNicknameLabel->setAlignment(Qt::AlignCenter);
    m_friendNicknameLabel->setStyleSheet(StyleCallWidget::titleStyle());
    m_friendNicknameLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    QFont nicknameFont("Outfit", 18, QFont::Normal);
    m_friendNicknameLabel->setFont(nicknameFont);

    // Control panel
    m_controlPanel = new QWidget(m_mainContainer);
    m_controlPanel->setStyleSheet(StyleCallWidget::panelStyle());
    m_controlPanel->setFixedHeight(100);

    m_controlLayout = new QHBoxLayout(m_controlPanel);
    m_controlLayout->setSpacing(10);
    m_controlLayout->setContentsMargins(20, 10, 20, 10);
    m_controlLayout->setAlignment(Qt::AlignCenter);

    // Mute Mic button
    m_muteMicButton = new QPushButton(m_controlPanel);
    m_muteMicButton->setFixedSize(60, 60);
    m_muteMicButton->setStyleSheet(StyleCallWidget::controlButtonStyle());
    m_muteMicButton->setCursor(Qt::PointingHandCursor);
    m_muteMicButton->setIcon(QIcon(":/resources/mic.png"));
    m_muteMicButton->setIconSize(QSize(30, 30));

    // Mute Speaker button
    m_muteSpeakerButton = new QPushButton(m_controlPanel);
    m_muteSpeakerButton->setFixedSize(60, 60);
    m_muteSpeakerButton->setStyleSheet(StyleCallWidget::controlButtonStyle());
    m_muteSpeakerButton->setCursor(Qt::PointingHandCursor);
    m_muteSpeakerButton->setIcon(QIcon(":/resources/speaker.png"));
    m_muteSpeakerButton->setIconSize(QSize(30, 30));

    // Hangup button
    m_hangupButton = new QPushButton(m_controlPanel);
    m_hangupButton->setFixedSize(60, 60);
    m_hangupButton->setStyleSheet(StyleCallWidget::hangupButtonStyle());
    m_hangupButton->setCursor(Qt::PointingHandCursor);
    m_hangupButton->setIcon(QIcon(":/resources/hangup.png"));
    m_hangupButton->setIconSize(QSize(30, 30));
    m_hangupButton->setIcon(QIcon(":/resources/hangup.png"));

    // Add buttons to layout
    m_controlLayout->addWidget(m_muteMicButton);
    m_controlLayout->addWidget(m_muteSpeakerButton);
    m_controlLayout->addWidget(m_hangupButton);

    // Add widgets to main layout
    m_containerLayout->addWidget(m_titleLabel);
    m_containerLayout->addWidget(m_timerLabel);
    m_containerLayout->addWidget(m_friendNicknameLabel);
    m_containerLayout->addSpacing(20);
    m_containerLayout->addWidget(m_controlPanel);

    m_mainLayout->addWidget(m_mainContainer, 0, Qt::AlignCenter);

    // Connect signals
    connect(m_muteMicButton, &QPushButton::clicked, this, &CallWidget::onMuteMicClicked);
    connect(m_muteSpeakerButton, &QPushButton::clicked, this, &CallWidget::onMuteSpeakerClicked);
    connect(m_hangupButton, &QPushButton::clicked, this, &CallWidget::onHangupClicked);

    // Setup shadow effect
    QGraphicsDropShadowEffect* shadowEffect = new QGraphicsDropShadowEffect(this);
    shadowEffect->setBlurRadius(20);
    shadowEffect->setColor(QColor(0, 0, 0, 80));
    shadowEffect->setOffset(0, 5);
    m_mainContainer->setGraphicsEffect(shadowEffect);
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
    m_callTimer->start(1000); // Update every second
}

void CallWidget::updateCallTimer() {
    m_callDuration = m_callDuration.addSecs(1);
    m_timerLabel->setText(m_callDuration.toString("mm:ss"));
}

void CallWidget::onMuteMicClicked() {
    static bool isMuted = false;
    isMuted = !isMuted;

    if (isMuted) {
        m_muteMicButton->setStyleSheet(
            "QPushButton {"
            "   background-color: rgba(232, 53, 53, 180);"
            "   border: none;"
            "   border-radius: 25px;"
            "   padding: 15px;"
            "   margin: 5px;"
            "}"
            "QPushButton:hover {"
            "   background-color: rgba(232, 53, 53, 220);"
            "}"
        );
    }
    else {
        m_muteMicButton->setStyleSheet(StyleCallWidget::controlButtonStyle());
    }

    emit muteMicClicked();
}

void CallWidget::onMuteSpeakerClicked() {
    static bool isMuted = false;
    isMuted = !isMuted;

    if (isMuted) {
        m_muteSpeakerButton->setStyleSheet(
            "QPushButton {"
            "   background-color: rgba(232, 53, 53, 180);"
            "   border: none;"
            "   border-radius: 25px;"
            "   padding: 15px;"
            "   margin: 5px;"
            "}"
            "QPushButton:hover {"
            "   background-color: rgba(232, 53, 53, 220);"
            "}"
        );
    }
    else {
        m_muteSpeakerButton->setStyleSheet(StyleCallWidget::controlButtonStyle());
    }

    emit muteSpeakerClicked();
}

void CallWidget::onHangupClicked() {
    m_callTimer->stop();
    emit hangupClicked();
}

QColor CallWidget::generateRandomColor(const QString& seed) {
    int hash = qHash(seed);
    return QColor::fromHsv(hash % 360, 150 + hash % 106, 150 + hash % 106);
}