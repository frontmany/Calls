#include "dialogs/notificationDialogBase.h"
#include "utilities/utilities.h"
#include <QVBoxLayout>
#include <QHBoxLayout>

QString NotificationDialogStyle::mainWidgetStyle(bool isGreenStyle)
{
    if (isGreenStyle)
    {
        return
            "QWidget#mainWidget {"
            "   background-color: rgb(235, 249, 237);"
            "   border-radius: 18px;"
            "}";
    }

    return
        "QWidget#mainWidget {"
        "   background-color: rgb(255, 255, 255);"
        "   border-radius: 24px;"
        "}";
}

QString NotificationDialogStyle::labelStyle(bool isGreenStyle)
{
    if (isGreenStyle)
    {
        return
            "color: rgb(25, 186, 0);"
            "font-size: 15px;"
            "font-family: 'Outfit';"
            "font-weight: 600;";
    }

    return
        "color: rgb(100, 100, 100);"
        "font-size: 15px;"
        "font-family: 'Outfit';"
        "font-weight: normal;";
}

NotificationDialogBase::NotificationDialogBase(QWidget* parent,
    const QString& statusText,
    bool isGreenStyle,
    bool isAnimation)
    : QWidget(parent)
    , m_isGreenStyle(isGreenStyle)
    , m_isAnimation(isAnimation)
{
    QFont font("Outfit", scale(14), QFont::Normal);

    setAttribute(Qt::WA_TranslucentBackground);
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);

    m_mainWidget = new QWidget(this);
    m_mainWidget->setObjectName("mainWidget");
    m_mainWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(m_mainWidget);

    int horizontalMargin = 20 - scale(2);
    int verticalMargin = 10;
    int contentSpacing = 8 + scale(2);

    QHBoxLayout* contentLayout = new QHBoxLayout(m_mainWidget);
    contentLayout->setContentsMargins(horizontalMargin, verticalMargin, horizontalMargin, verticalMargin);
    contentLayout->setSpacing(contentSpacing);
    contentLayout->setAlignment(Qt::AlignCenter);

    m_statusLabel = new QLabel(statusText);
    m_statusLabel->setAlignment(Qt::AlignCenter | Qt::AlignVCenter);
    m_statusLabel->setFont(font);
    m_statusLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
    m_statusLabel->setWordWrap(false);
    if (m_isAnimation)
    {
        m_statusLabel->setContentsMargins(0, 0, scale(8), 0);
    }

    m_gifLabel = new QLabel();
    m_gifLabel->setAlignment(Qt::AlignCenter);
    m_gifLabel->setFixedSize(32, 32);

    m_movie = new QMovie(":/resources/waiting.gif");
    if (m_movie->isValid())
    {
        m_gifLabel->setMovie(m_movie);
        if (m_isAnimation)
        {
            m_movie->start();
        }
    }

    contentLayout->addWidget(m_statusLabel, 0, Qt::AlignCenter);
    contentLayout->addWidget(m_gifLabel, 0, Qt::AlignCenter);

    QFontMetrics fontMetrics(font);
    int textWidth = fontMetrics.horizontalAdvance(statusText);
    int animationWidth = m_isAnimation ? (contentSpacing + 32) : 0;
    int minDialogWidth = textWidth + horizontalMargin * 2 + animationWidth;
    setMinimumWidth(minDialogWidth);
    setMinimumHeight(50 - scale(2));
    setMaximumHeight(60 - scale(2));

    setAnimationEnabled(m_isAnimation);
    applyStyle();
}

void NotificationDialogBase::applyStyle()
{
    if (m_mainWidget)
    {
        m_mainWidget->setStyleSheet(NotificationDialogStyle::mainWidgetStyle(m_isGreenStyle));
    }

    if (m_statusLabel)
    {
        m_statusLabel->setStyleSheet(NotificationDialogStyle::labelStyle(m_isGreenStyle));
    }
}

void NotificationDialogBase::setStatusText(const QString& text)
{
    if (m_statusLabel)
    {
        m_statusLabel->setText(text);
    }
}

void NotificationDialogBase::setGreenStyle(bool isGreenStyle)
{
    m_isGreenStyle = isGreenStyle;
    applyStyle();
}

void NotificationDialogBase::setAnimationEnabled(bool isAnimation)
{
    m_isAnimation = isAnimation;

    if (m_movie)
    {
        if (m_isAnimation)
        {
            if (m_movie->state() != QMovie::Running)
            {
                m_movie->start();
            }
        }
        else
        {
            m_movie->stop();
        }
    }

    if (m_gifLabel)
    {
        m_gifLabel->setVisible(m_isAnimation);
    }
}
