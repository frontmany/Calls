#include "incomingCallWidget.h"
#include "buttons.h"

#include <QPainter>
#include <QFontDatabase>
#include "utilities/utilities.h"
#include "utilities/constant.h"
#include "utilities/color.h"

// Style definitions
const QColor StyleIncomingCallWidget::m_backgroundColor = COLOR_BACKGROUND_SECONDARY;
const QColor StyleIncomingCallWidget::m_borderColor = COLOR_BORDER_SUBTLE;
const QColor StyleIncomingCallWidget::m_nicknameTextColor = COLOR_TEXT_MAIN;
const QColor StyleIncomingCallWidget::m_callTypeTextColor = COLOR_TEXT_MUTED;
const QColor StyleIncomingCallWidget::m_timerTextColor = COLOR_TEXT_MUTED_SECONDARY;
const QColor StyleIncomingCallWidget::m_timerCircleColor = COLOR_TEXT_MUTED_SECONDARY;

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

QString StyleIncomingCallWidget::avatarStyle() {
    return QString("QLabel {"
        "   background-color: black;"
        "   border-radius: %1px;"
        "   color: white;"
        "   font-size: %2px;"
        "   font-weight: bold;"
        "}").arg(QString::fromStdString(std::to_string(scale(25))))  // border-radius
        .arg(QString::fromStdString(std::to_string(scale(18)))); // font-size
}
IncomingCallWidget::IncomingCallWidget(QWidget* parent, const QString& friendNickname, int remainingTime)
    : QWidget(parent)
    , m_friendNickname(friendNickname)
    , m_timer(nullptr)
    , m_remainingSeconds(remainingTime > 0 ? remainingTime : 0)
    , m_totalSeconds(remainingTime > 0 ? remainingTime : 1)
{
    setupUI();
    setupTimer();
    setFixedHeight(scale(80));
    setMinimumWidth(scale(440));
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
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

    m_avatarLabel->setStyleSheet(StyleIncomingCallWidget::avatarStyle());

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
    m_acceptButton->setCursor(Qt::PointingHandCursor);

    QIcon declineIcon(":/resources/declineCall.png");
    QIcon declineIconHover(":/resources/declineCallHover.png");
    m_declineButton = new ButtonIcon(this, declineIcon, declineIconHover, scale(34), scale(34));
    m_declineButton->setCursor(Qt::PointingHandCursor);

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

const QString& IncomingCallWidget::getFriendNickname() const {
    return m_friendNickname;
}

void IncomingCallWidget::setButtonsEnabled(bool enabled)
{
    if (m_acceptButton) {
        m_acceptButton->setEnabled(enabled);
    }
    if (m_declineButton) {
        m_declineButton->setEnabled(enabled);
    }
}

void IncomingCallWidget::setupTimer() {
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &IncomingCallWidget::updateTimer);
    m_timer->start(TIMER_INTERVAL_MS);
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

void IncomingCallWidget::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    painter.setPen(Qt::NoPen);
    painter.setBrush(StyleIncomingCallWidget::m_backgroundColor);
    painter.drawRoundedRect(rect(), 15, 15);

    QWidget* timerWidget = m_timerLabel->parentWidget();
    if (timerWidget)
    {
        QPoint timerPos = timerWidget->mapTo(this, QPoint(0, 0));
        int circleSize = qMin(timerWidget->width(), timerWidget->height()) - 4;

        if (circleSize > 0)
        {
            int x = timerPos.x() + (timerWidget->width() - circleSize) / 2;
            int y = timerPos.y() + (timerWidget->height() - circleSize) / 2;

            QRect circleRect(x, y, circleSize, circleSize);
            int basePenWidth = scale(2);
            if (basePenWidth < 1)
            {
                basePenWidth = 1;
            }

            int highlightPenWidth = scale(4);
            if (highlightPenWidth <= basePenWidth)
            {
                highlightPenWidth = basePenWidth + 1;
            }

            QRectF arcRect(circleRect);
            const qreal inset = static_cast<qreal>(highlightPenWidth) / 2.0;
            arcRect.adjust(inset, inset, -inset, -inset);

            if (arcRect.width() > 0.0 && arcRect.height() > 0.0)
            {
                painter.setBrush(Qt::NoBrush);

                QColor baseColor = StyleIncomingCallWidget::m_timerCircleColor.lighter(150);
                baseColor.setAlpha(140);
                QPen basePen(baseColor, basePenWidth);
                basePen.setCapStyle(Qt::RoundCap);
                painter.setPen(basePen);
                painter.drawEllipse(arcRect);

                double progressRatio = 0.0;
                if (m_totalSeconds > 0)
                {
                    progressRatio = 1.0 - static_cast<double>(m_remainingSeconds) / static_cast<double>(m_totalSeconds);
                    if (progressRatio < 0.0)
                    {
                        progressRatio = 0.0;
                    }
                    else if (progressRatio > 1.0)
                    {
                        progressRatio = 1.0;
                    }
                }

                if (progressRatio > 0.0)
                {
                    QColor highlightColor = StyleIncomingCallWidget::m_timerCircleColor;
                    highlightColor.setAlpha(255);

                    QPen highlightPen(highlightColor, highlightPenWidth);
                    highlightPen.setCapStyle(Qt::RoundCap);
                    painter.setPen(highlightPen);

                    int spanAngle = static_cast<int>(progressRatio * 360.0);
                    painter.drawArc(arcRect, 90 * 16, -spanAngle * 16);
                }
            }
        }
    }

    QWidget::paintEvent(event);
}
