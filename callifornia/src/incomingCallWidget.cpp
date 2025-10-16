#include "incomingCallWidget.h"
#include "buttons.h"

#include <QPainter>
#include <QFontDatabase>
#include "scaleFactor.h"

// Style definitions
const QColor StyleIncomingCallWidget::m_backgroundColor = QColor(235, 235, 235, 110);
const QColor StyleIncomingCallWidget::m_borderColor = QColor(200, 200, 200, 100);
const QColor StyleIncomingCallWidget::m_nicknameTextColor = QColor(1, 11, 19);
const QColor StyleIncomingCallWidget::m_callTypeTextColor = QColor(102, 102, 102);
const QColor StyleIncomingCallWidget::m_timerTextColor = QColor(224, 168, 0);
const QColor StyleIncomingCallWidget::m_timerCircleColor = QColor(224, 168, 0);

QString StyleIncomingCallWidget::widgetStyle() {
    return QString("IncomingCallWidget {"
        "   background-color: rgba(%1, %2, %3, %4);"
        "   border-radius: %9px;"
        "   border: 1px solid rgba(%5, %6, %7, %8);"
        "}").arg(m_backgroundColor.red()).arg(m_backgroundColor.green())
        .arg(m_backgroundColor.blue()).arg(m_backgroundColor.alpha())
        .arg(m_borderColor.red()).arg(m_borderColor.green())
        .arg(m_borderColor.blue()).arg(m_borderColor.alpha())
        .arg(QString::fromStdString(std::to_string(scale(15))));  // border-radius
}

QString StyleIncomingCallWidget::nicknameStyle() {
    return QString("QLabel {"
        "   color: %1;"
        "}").arg(m_nicknameTextColor.name());
}

QString StyleIncomingCallWidget::callTypeStyle() {
    return QString("QLabel {"
        "   color: %1;"
        "}").arg(m_callTypeTextColor.name());
}

QString StyleIncomingCallWidget::timerStyle() {
    return QString("QLabel {"
        "   color: %1;"
        "   font-size: %2px;"
        "   font-weight: bold;"
        "   background-color: transparent;"
        "}").arg(m_timerTextColor.name())
        .arg(QString::fromStdString(std::to_string(scale(16))));  // font-size
}

QString StyleIncomingCallWidget::avatarStyle(const QColor& color) {
    return QString("QLabel {"
        "   background-color: %1;"
        "   border-radius: %2px;"
        "   color: white;"
        "   font-size: %3px;"
        "   font-weight: bold;"
        "}").arg(color.name())
        .arg(QString::fromStdString(std::to_string(scale(25))))  // border-radius
        .arg(QString::fromStdString(std::to_string(scale(18)))); // font-size
}
IncomingCallWidget::IncomingCallWidget(QWidget* parent, const QString& friendNickname, int remainingTime)
    : QWidget(parent), m_friendNickname(friendNickname), m_remainingSeconds(remainingTime) {

    setupUI();
    setupTimer();
    setFixedHeight(scale(80));
}

IncomingCallWidget::~IncomingCallWidget() {
    if (m_timer && m_timer->isActive()) {
        m_timer->stop();
    }
}

void IncomingCallWidget::setupUI() {
    QHBoxLayout* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(scale(15), scale(10), scale(15), scale(10));
    mainLayout->setSpacing(scale(15));

    // Left side - Avatar and text info
    QWidget* leftWidget = new QWidget(this);
    leftWidget->setAttribute(Qt::WA_TranslucentBackground);
    QHBoxLayout* leftLayout = new QHBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(scale(12));

    // Avatar
    m_avatarLabel = new QLabel(leftWidget);
    m_avatarLabel->setFixedSize(scale(50), scale(50));
    m_avatarLabel->setAlignment(Qt::AlignCenter);

    QColor avatarColor = generateRandomColor(m_friendNickname);
    m_avatarLabel->setStyleSheet(StyleIncomingCallWidget::avatarStyle(avatarColor));

    // Generate avatar letter
    QString firstLetter = m_friendNickname.isEmpty() ? "G" : m_friendNickname.left(1).toUpper();
    m_avatarLabel->setText(firstLetter);

    // Text info
    QWidget* textWidget = new QWidget(leftWidget);
    textWidget->setAttribute(Qt::WA_TranslucentBackground);
    QVBoxLayout* textLayout = new QVBoxLayout(textWidget);
    textLayout->setContentsMargins(0, 0, 0, 0);
    textLayout->setSpacing(scale(4));

    m_nicknameLabel = new QLabel(m_friendNickname, textWidget);
    m_nicknameLabel->setAttribute(Qt::WA_TranslucentBackground);
    m_nicknameLabel->setStyleSheet(StyleIncomingCallWidget::nicknameStyle());
    QFont nicknameLabelFont("Outfit", scale(13), QFont::Bold);
    m_nicknameLabel->setFont(nicknameLabelFont);

    m_callTypeLabel = new QLabel("incoming call", textWidget);
    m_callTypeLabel->setAttribute(Qt::WA_TranslucentBackground);
    m_callTypeLabel->setStyleSheet(StyleIncomingCallWidget::callTypeStyle());
    QFont callTypeFont("Outfit", scale(12), QFont::Light);
    m_callTypeLabel->setFont(callTypeFont);

    textLayout->addWidget(m_nicknameLabel);
    textLayout->addSpacing(scale(-7));
    textLayout->addWidget(m_callTypeLabel);

    leftLayout->addWidget(m_avatarLabel);
    leftLayout->addWidget(textWidget);

    // Right side - Timer and buttons
    QWidget* rightWidget = new QWidget(this);
    rightWidget->setAttribute(Qt::WA_TranslucentBackground);
    QHBoxLayout* rightLayout = new QHBoxLayout(rightWidget);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(scale(10));

    // Timer circle
    QWidget* timerWidget = new QWidget(rightWidget);
    timerWidget->setFixedSize(scale(50), scale(50));
    timerWidget->setAttribute(Qt::WA_TranslucentBackground);
    QVBoxLayout* timerLayout = new QVBoxLayout(timerWidget);
    timerLayout->setContentsMargins(0, 0, 0, 0);

    m_timerLabel = new QLabel(QString::number(m_remainingSeconds), timerWidget);
    m_timerLabel->setAlignment(Qt::AlignCenter);
    m_timerLabel->setStyleSheet(StyleIncomingCallWidget::timerStyle());
    timerLayout->addWidget(m_timerLabel);

    // Buttons
    QIcon acceptIcon(":/resources/acceptCall.png");
    QIcon acceptIconHover(":/resources/acceptCallHover.png");
    m_acceptButton = new ButtonIcon(this, acceptIcon, acceptIconHover, scale(34), scale(34));

    QIcon declineIcon(":/resources/declineCall.png");
    QIcon declineIconHover(":/resources/declineCallHover.png");
    m_declineButton = new ButtonIcon(this, declineIcon, declineIconHover, scale(34), scale(34));

    // Connect buttons
    connect(m_acceptButton, &ButtonIcon::clicked, [this]() {
        if (m_timer->isActive()) {
            m_timer->stop();
        }
        emit callAccepted(m_friendNickname);
        });

    connect(m_declineButton, &ButtonIcon::clicked, [this]() {
        if (m_timer->isActive()) {
            m_timer->stop();
        }
        emit callDeclined(m_friendNickname);
        });

    rightLayout->addWidget(timerWidget);
    rightLayout->addWidget(m_acceptButton);
    rightLayout->addWidget(m_declineButton);

    mainLayout->addWidget(leftWidget);
    mainLayout->addStretch();
    mainLayout->addWidget(rightWidget);

    // Set widget style
    setStyleSheet(StyleIncomingCallWidget::widgetStyle());
}

QColor IncomingCallWidget::generateRandomColor(const QString& seed) {
    int hash = qHash(seed);
    return QColor::fromHsv(hash % 360, 150 + hash % 106, 150 + hash % 106);
}

const QString& IncomingCallWidget::getFriendNickname() const {
    return m_friendNickname;
}

int IncomingCallWidget::getRemainingTime() const {
    return m_remainingSeconds;
}

void IncomingCallWidget::setupTimer() {
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &IncomingCallWidget::updateTimer);
    m_timer->start(1000); // Update every second
}

void IncomingCallWidget::updateTimer() {
    m_remainingSeconds--;

    if (m_remainingSeconds <= 0) {
        m_timer->stop();
        return;
    }

    m_timerLabel->setText(QString::number(m_remainingSeconds));
    update(); // Trigger repaint for the circle
}

void IncomingCallWidget::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Draw background first
    painter.setPen(Qt::NoPen);
    painter.setBrush(StyleIncomingCallWidget::m_backgroundColor);
    painter.drawRoundedRect(rect(), 15, 15);

    // Draw timer circle
    QWidget* timerWidget = m_timerLabel->parentWidget();
    if (timerWidget) {
        QPoint timerPos = timerWidget->mapTo(this, QPoint(0, 0));

        int circleSize = qMin(timerWidget->width(), timerWidget->height()) - 4;
        int x = timerPos.x() + (timerWidget->width() - circleSize) / 2;
        int y = timerPos.y() + (timerWidget->height() - circleSize) / 2;

        QRect circleRect(x, y, circleSize, circleSize);

        // Calculate progress angle (360 degrees for 32 seconds)
        int progress = 360 - (m_remainingSeconds * 360 / 32);

        painter.setPen(QPen(StyleIncomingCallWidget::m_timerCircleColor, 3));
        painter.setBrush(Qt::NoBrush);
        painter.drawArc(circleRect, 90 * 16, -progress * 16);
    }

    QWidget::paintEvent(event);
}