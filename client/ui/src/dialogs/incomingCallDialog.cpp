#include "dialogs/incomingCallDialog.h"
#include "widgets/buttons.h"
#include "widgets/incomingCallWidget.h"
#include "utilities/utilities.h"
#include "utilities/constant.h"
#include "utilities/color.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFont>
#include <QPainter>
#include <QPixmap>

IncomingCallDialog::IncomingCallDialog(QWidget* parent, const QString& friendNickname, int remainingTime)
    : QDialog(parent)
    , m_friendNickname(friendNickname)
    , m_remainingSeconds(remainingTime > 0 ? remainingTime : 0)
    , m_totalSeconds(remainingTime > 0 ? remainingTime : 1)
{
    setModal(false);
    setAttribute(Qt::WA_DeleteOnClose, false);
    setWindowFlags(windowFlags() | Qt::Window | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground, false);
    setStyleSheet("QDialog { background: transparent; }");
    setWindowTitle("Incoming call");
    setWindowIcon(QIcon(":/resources/logo.png"));

    m_backgroundTexture = QPixmap(":/resources/blur.png");

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

void IncomingCallDialog::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    if (!m_backgroundTexture.isNull()) {
        painter.drawPixmap(rect(), m_backgroundTexture);
    } else {
        painter.fillRect(rect(), Qt::white);
    }

    QDialog::paintEvent(event);
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
    QVBoxLayout* cardLayout = new QVBoxLayout(this);
    cardLayout->setContentsMargins(scale(32), scale(28), scale(32), scale(30));
    cardLayout->setSpacing(scale(14));
    cardLayout->setAlignment(Qt::AlignHCenter | Qt::AlignTop);

    QWidget* topBar = new QWidget(this);
    topBar->setContentsMargins(0, 0, 0, 0);
    topBar->setFixedHeight(scale(30));

    QHBoxLayout* topBarLayout = new QHBoxLayout(topBar);
    topBarLayout->setContentsMargins(0, 0, 0, 0);
    topBarLayout->setSpacing(0);

    topBarLayout->addStretch();

    const int avatarSize = scale(72);
    m_avatarLabel = new QLabel(this);
    m_avatarLabel->setFixedSize(avatarSize, avatarSize);
    m_avatarLabel->setAlignment(Qt::AlignCenter);
    QString avatarStyle = QString("QLabel {"
        "background-color: %1;"
        "border: %2px solid %3;"
        "border-radius: %4px;"
        "color: %5;"
        "font-size: %6px;"
        "font-weight: bold;"
        "}")
        .arg(QColor(20, 22, 28).name())
        .arg(scale(1))
        .arg(COLOR_BORDER_WHITE.name())
        .arg(QString::fromStdString(std::to_string(avatarSize / 2)))
        .arg(COLOR_HEX_WHITE)
        .arg(QString::fromStdString(std::to_string(scale(20))));
    m_avatarLabel->setStyleSheet(avatarStyle);
    QString firstLetter = m_friendNickname.isEmpty() ? "G" : m_friendNickname.left(1).toUpper();
    m_avatarLabel->setText(firstLetter);

    m_nicknameLabel = new QLabel(m_friendNickname, this);
    m_nicknameLabel->setAlignment(Qt::AlignCenter);
    m_nicknameLabel->setStyleSheet(StyleIncomingCallWidget::nicknameStyle());
    m_nicknameLabel->setWordWrap(true);
    QFont nicknameFont("Outfit", 20, QFont::Bold);
    m_nicknameLabel->setFont(nicknameFont);

    m_callTypeLabel = new QLabel("Incoming call", this);
    m_callTypeLabel->setAlignment(Qt::AlignCenter);
    m_callTypeLabel->setStyleSheet(StyleIncomingCallWidget::callTypeStyle());
    QFont callTypeFont("Outfit", 14, QFont::Light);
    m_callTypeLabel->setFont(callTypeFont);

    m_timerLabel = new QLabel(this);
    m_timerLabel->setAlignment(Qt::AlignCenter);
    QString timerStyle = QString("QLabel {"
        "color: %1;"
        "font-size: %2px;"
        "font-weight: bold;"
        "background-color: transparent;"
        "}").arg(StyleIncomingCallWidget::m_timerTextColor.name())
        .arg(QString::fromStdString(std::to_string(scale(22))));
    m_timerLabel->setStyleSheet(timerStyle);
    updateTimerLabel();

    QWidget* buttonsWidget = new QWidget(this);
    QHBoxLayout* buttonsLayout = new QHBoxLayout(buttonsWidget);
    buttonsLayout->setContentsMargins(0, 0, 0, 0);
    buttonsLayout->setSpacing(scale(18));
    buttonsLayout->setAlignment(Qt::AlignCenter);

    QIcon acceptIcon(":/resources/acceptCall.png");
    QIcon acceptIconHover(":/resources/acceptCallHover.png");
    m_acceptButton = new ButtonIcon(buttonsWidget, acceptIcon, acceptIconHover, scale(46), scale(46));
    m_acceptButton->setCursor(Qt::PointingHandCursor);

    QIcon declineIcon(":/resources/declineCall.png");
    QIcon declineIconHover(":/resources/declineCallHover.png");
    m_declineButton = new ButtonIcon(buttonsWidget, declineIcon, declineIconHover, scale(46), scale(46));
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
    cardLayout->addSpacing(scale(6));
    cardLayout->addWidget(m_timerLabel);
    cardLayout->addSpacing(scale(14));
    cardLayout->addWidget(buttonsWidget);

    setMinimumWidth(scale(340));
}

void IncomingCallDialog::setupTimer()
{
    if (m_remainingSeconds <= 0)
    {
        return;
    }

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &IncomingCallDialog::updateTimer);
    m_timer->start(TIMER_INTERVAL_MS);
}

void IncomingCallDialog::updateTimerLabel()
{
    if (m_timerLabel)
    {
        m_timerLabel->setText(QString::number(m_remainingSeconds));
    }
}
