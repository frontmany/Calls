#include "dialogs/waitingStatusDialog.h"
#include "utilities/utilities.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMovie>
#include <QScreen>

QString StyleWaitingStatusDialog::mainWidgetStyle()
{
    return
        "QWidget#mainWidget {"
        "   background-color: rgb(255, 255, 255);"
        "   border-radius: 24px;"
        "}";
}

QString StyleWaitingStatusDialog::labelStyle()
{
    return
        "color: rgb(100, 100, 100);"
        "font-size: 15px;"
        "font-family: 'Outfit';"
        "font-weight: normal;";
}

WaitingStatusDialog::WaitingStatusDialog(QWidget* parent, const QString& statusText)
    : QWidget(parent)
{
    QFont font("Outfit", scale(14), QFont::Normal);

    setAttribute(Qt::WA_TranslucentBackground);
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);

    QWidget* mainWidget = new QWidget(this);
    mainWidget->setObjectName("mainWidget");
    mainWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    mainWidget->setStyleSheet(StyleWaitingStatusDialog::mainWidgetStyle());

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(mainWidget);

    int horizontalMargin = 20 - scale(2);
    int verticalMargin = 10;
    int contentSpacing = 8 + scale(2);

    QHBoxLayout* contentLayout = new QHBoxLayout(mainWidget);
    contentLayout->setContentsMargins(horizontalMargin, verticalMargin, horizontalMargin, verticalMargin);
    contentLayout->setSpacing(contentSpacing);
    contentLayout->setAlignment(Qt::AlignCenter);

    m_statusLabel = new QLabel(statusText);
    m_statusLabel->setAlignment(Qt::AlignCenter | Qt::AlignVCenter);
    m_statusLabel->setStyleSheet(StyleWaitingStatusDialog::labelStyle());
    m_statusLabel->setFont(font);
    m_statusLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
    m_statusLabel->setWordWrap(false);

    m_gifLabel = new QLabel();
    m_gifLabel->setAlignment(Qt::AlignCenter);
    m_gifLabel->setFixedSize(32, 32);

    m_movie = new QMovie(":/resources/waiting.gif");
    if (m_movie->isValid())
    {
        m_gifLabel->setMovie(m_movie);
        m_movie->start();
    }

    contentLayout->addWidget(m_statusLabel, 0, Qt::AlignCenter);
    contentLayout->addWidget(m_gifLabel, 0, Qt::AlignCenter);

    QFontMetrics fontMetrics(font);
    int textWidth = fontMetrics.horizontalAdvance(statusText);
    int minDialogWidth = textWidth + horizontalMargin * 2 + contentSpacing + 32;
    setMinimumWidth(minDialogWidth);
    setMinimumHeight(50 - scale(2));
    setMaximumHeight(60 - scale(2));
}

void WaitingStatusDialog::setStatusText(const QString& text)
{
    if (m_statusLabel)
    {
        m_statusLabel->setText(text);
    }
}
