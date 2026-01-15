#include "dialogs/incomingCallDialog.h"
#include "widgets/buttons.h"
#include "widgets/incomingCallWidget.h"
#include "utilities/utilities.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFont>
#include <QGraphicsDropShadowEffect>

IncomingCallDialog::IncomingCallDialog(QWidget* parent, const QString& friendNickname, int remainingTime)
    : QDialog(parent)
    , m_friendNickname(friendNickname)
    , m_remainingSeconds(remainingTime > 0 ? remainingTime : 0)
    , m_totalSeconds(remainingTime > 0 ? remainingTime : 1)
{
    setModal(false);
    setAttribute(Qt::WA_DeleteOnClose, false);
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint | Qt::Window);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setStyleSheet("background-color: transparent;");

    setupUi();
    setupTimer();
}

const QString& IncomingCallDialog::getFriendNickname() const
{
    return m_friendNickname;
}

void IncomingCallDialog::setButtonsEnabled(bool enabled)
{
    if (m_acceptButton)
    {
        m_acceptButton->setEnabled(enabled);
    }

    if (m_declineButton)
    {
        m_declineButton->setEnabled(enabled);
    }
}

void IncomingCallDialog::setRemainingTime(int remainingTime)
{
    const int nextValue = remainingTime > 0 ? remainingTime : 0;
    m_remainingSeconds = nextValue;
    m_totalSeconds = nextValue > 0 ? nextValue : 1;
    updateTimerLabel();
}

void IncomingCallDialog::reject()
{
    emit dialogClosed({ m_friendNickname });
    QDialog::reject();
}

void IncomingCallDialog::mousePressEvent(QMouseEvent* event)
{
    if (event && event->button() == Qt::LeftButton)
    {
        m_dragging = true;
        m_dragStartPos = event->globalPosition().toPoint() - frameGeometry().topLeft();
        event->accept();
        return;
    }

    QDialog::mousePressEvent(event);
}

void IncomingCallDialog::mouseMoveEvent(QMouseEvent* event)
{
    if (m_dragging && event && (event->buttons() & Qt::LeftButton))
    {
        move(event->globalPosition().toPoint() - m_dragStartPos);
        event->accept();
        return;
    }

    QDialog::mouseMoveEvent(event);
}

void IncomingCallDialog::mouseReleaseEvent(QMouseEvent* event)
{
    if (event && event->button() == Qt::LeftButton)
    {
        m_dragging = false;
        event->accept();
        return;
    }

    QDialog::mouseReleaseEvent(event);
}

void IncomingCallDialog::updateTimer()
{
    if (m_remainingSeconds <= 0)
    {
        if (m_timer && m_timer->isActive())
        {
            m_timer->stop();
        }
        return;
    }

    --m_remainingSeconds;
    if (m_remainingSeconds <= 0)
    {
        if (m_timer && m_timer->isActive())
        {
            m_timer->stop();
        }
        m_remainingSeconds = 0;
    }

    updateTimerLabel();
}

void IncomingCallDialog::setupUi()
{
    QWidget* container = new QWidget(this);
    container->setObjectName("incomingCallCard");

    const QColor& backgroundColor = StyleIncomingCallWidget::m_backgroundColor;
    const int radius = scale(16);
    QString cardStyle = QString("#incomingCallCard {"
        "background-color: rgba(%1, %2, %3, %4);"
        "border-radius: %5px;"
        "border: none;"
        "}")
        .arg(backgroundColor.red())
        .arg(backgroundColor.green())
        .arg(backgroundColor.blue())
        .arg(backgroundColor.alpha())
        .arg(radius);

    container->setStyleSheet(cardStyle);

    QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect(container);
    shadow->setBlurRadius(scale(24));
    shadow->setOffset(0, scale(6));
    shadow->setColor(QColor(0, 0, 0, 50));
    container->setGraphicsEffect(shadow);

    QVBoxLayout* cardLayout = new QVBoxLayout(container);
    cardLayout->setContentsMargins(scale(28), scale(24), scale(28), scale(26));
    cardLayout->setSpacing(scale(12));
    cardLayout->setAlignment(Qt::AlignHCenter | Qt::AlignTop);

    QWidget* topBar = new QWidget(container);
    topBar->setContentsMargins(0, 0, 0, 0);

    QHBoxLayout* topBarLayout = new QHBoxLayout(topBar);
    topBarLayout->setContentsMargins(0, 0, 0, 0);
    topBarLayout->setSpacing(0);

    m_closeButton = new ButtonIcon(topBar, scale(28), scale(28));
    m_closeButton->setIcons(QIcon(":/resources/close.png"), QIcon(":/resources/closeHover.png"));
    m_closeButton->setSize(scale(28), scale(28));
    m_closeButton->setCursor(Qt::PointingHandCursor);

    connect(m_closeButton, &ButtonIcon::clicked, this, &IncomingCallDialog::reject);

    topBarLayout->addStretch();
    topBarLayout->addWidget(m_closeButton);

    const int avatarSize = scale(60);
    m_avatarLabel = new QLabel(container);
    m_avatarLabel->setFixedSize(avatarSize, avatarSize);
    m_avatarLabel->setAlignment(Qt::AlignCenter);
    QString avatarStyle = QString("QLabel {"
        "background-color: black;"
        "border-radius: %1px;"
        "color: white;"
        "font-size: %2px;"
        "font-weight: bold;"
        "}").arg(QString::fromStdString(std::to_string(avatarSize / 2)))
        .arg(QString::fromStdString(std::to_string(scale(18))));
    m_avatarLabel->setStyleSheet(avatarStyle);
    QString firstLetter = m_friendNickname.isEmpty() ? "G" : m_friendNickname.left(1).toUpper();
    m_avatarLabel->setText(firstLetter);

    m_nicknameLabel = new QLabel(m_friendNickname, container);
    m_nicknameLabel->setAlignment(Qt::AlignCenter);
    m_nicknameLabel->setStyleSheet(StyleIncomingCallWidget::nicknameStyle());
    m_nicknameLabel->setWordWrap(true);
    QFont nicknameFont("Outfit", scale(16), QFont::Bold);
    m_nicknameLabel->setFont(nicknameFont);

    m_callTypeLabel = new QLabel("incomingcall", container);
    m_callTypeLabel->setAlignment(Qt::AlignCenter);
    m_callTypeLabel->setStyleSheet(StyleIncomingCallWidget::callTypeStyle());
    QFont callTypeFont("Outfit", scale(13), QFont::Light);
    m_callTypeLabel->setFont(callTypeFont);

    m_timerLabel = new QLabel(container);
    m_timerLabel->setAlignment(Qt::AlignCenter);
    QString timerStyle = QString("QLabel {"
        "color: %1;"
        "font-size: %2px;"
        "font-weight: bold;"
        "background-color: transparent;"
        "}").arg(StyleIncomingCallWidget::m_timerTextColor.name())
        .arg(QString::fromStdString(std::to_string(scale(18))));
    m_timerLabel->setStyleSheet(timerStyle);
    updateTimerLabel();

    QWidget* buttonsWidget = new QWidget(container);
    QHBoxLayout* buttonsLayout = new QHBoxLayout(buttonsWidget);
    buttonsLayout->setContentsMargins(0, 0, 0, 0);
    buttonsLayout->setSpacing(scale(16));
    buttonsLayout->setAlignment(Qt::AlignCenter);

    QIcon acceptIcon(":/resources/acceptCall.png");
    QIcon acceptIconHover(":/resources/acceptCallHover.png");
    m_acceptButton = new ButtonIcon(buttonsWidget, acceptIcon, acceptIconHover, scale(38), scale(38));
    m_acceptButton->setCursor(Qt::PointingHandCursor);

    QIcon declineIcon(":/resources/declineCall.png");
    QIcon declineIconHover(":/resources/declineCallHover.png");
    m_declineButton = new ButtonIcon(buttonsWidget, declineIcon, declineIconHover, scale(38), scale(38));
    m_declineButton->setCursor(Qt::PointingHandCursor);

    connect(m_acceptButton, &ButtonIcon::clicked, [this]()
    {
        if (m_timer && m_timer->isActive())
        {
            m_timer->stop();
        }
        emit callAccepted(m_friendNickname);
    });

    connect(m_declineButton, &ButtonIcon::clicked, [this]()
    {
        if (m_timer && m_timer->isActive())
        {
            m_timer->stop();
        }
        emit callDeclined(m_friendNickname);
    });

    buttonsLayout->addWidget(m_acceptButton);
    buttonsLayout->addWidget(m_declineButton);

    cardLayout->addWidget(topBar);
    cardLayout->addWidget(m_avatarLabel, 0, Qt::AlignHCenter);
    cardLayout->addWidget(m_nicknameLabel);
    cardLayout->addWidget(m_callTypeLabel);
    cardLayout->addSpacing(scale(4));
    cardLayout->addWidget(m_timerLabel);
    cardLayout->addSpacing(scale(10));
    cardLayout->addWidget(buttonsWidget);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(scale(8), scale(8), scale(8), scale(8));
    mainLayout->setSpacing(0);
    mainLayout->addWidget(container);

    setFixedWidth(scale(290));
    adjustSize();
}

void IncomingCallDialog::setupTimer()
{
    if (m_remainingSeconds <= 0)
    {
        return;
    }

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &IncomingCallDialog::updateTimer);
    m_timer->start(1000);
}

void IncomingCallDialog::updateTimerLabel()
{
    if (m_timerLabel)
    {
        m_timerLabel->setText(QString::number(m_remainingSeconds));
    }
}
