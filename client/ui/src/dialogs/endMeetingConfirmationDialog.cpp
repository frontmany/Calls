#include "endMeetingConfirmationDialog.h"
#include "utilities/utility.h"
#include "constants/color.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QGraphicsDropShadowEffect>
#include <QShortcut>
#include <QFont>

EndMeetingConfirmationDialog::EndMeetingConfirmationDialog(QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TranslucentBackground);

    QFont font("Outfit", scale(14), QFont::Normal);
    QFont titleFont("Outfit", scale(18), QFont::Bold);

    const int shadowMargin = scale(24);

    QGraphicsDropShadowEffect* shadowEffect = new QGraphicsDropShadowEffect();
    shadowEffect->setBlurRadius(scale(30));
    shadowEffect->setXOffset(0);
    shadowEffect->setYOffset(scale(4));
    shadowEffect->setColor(QColor(0, 0, 0, 120));

    QWidget* mainWidget = new QWidget(this);
    mainWidget->setGraphicsEffect(shadowEffect);
    mainWidget->setObjectName("mainWidget");
    mainWidget->setStyleSheet(QString(
        "QWidget#mainWidget {"
        "   background-color: %1;"
        "   border-radius: %2px;"
        "   border: %3px solid %4;"
        "}")
        .arg(COLOR_BACKGROUND_ERROR_SUBTLE.name())
        .arg(scale(16))
        .arg(scale(1))
        .arg(COLOR_BORDER_NEUTRAL.name()));

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(shadowMargin, shadowMargin, shadowMargin, shadowMargin);
    mainLayout->addWidget(mainWidget);

    QVBoxLayout* contentLayout = new QVBoxLayout(mainWidget);
    contentLayout->setContentsMargins(scale(32), scale(32), scale(32), scale(32));
    contentLayout->setSpacing(scale(20));

    QLabel* titleLabel = new QLabel("End Meeting?");
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(QString(
        "color: %1;"
        "font-size: %2px;"
        "font-family: 'Outfit';"
        "font-weight: bold;"
        "padding: %3px;"
        ).arg(COLOR_BANNER_ERROR.name()).arg(scale(18)).arg(scale(5)));
    titleLabel->setFont(titleFont);

    QLabel* messageLabel = new QLabel(
        "You are the owner of this meeting and there are other participants.\n"
        "Do you want to end the meeting for everyone?");
    messageLabel->setAlignment(Qt::AlignCenter);
    messageLabel->setWordWrap(true);
    messageLabel->setStyleSheet(QString(
        "color: %1;"
        "font-size: %2px;"
        "font-family: 'Outfit';"
        "line-height: %3px;"
        "padding: %4px;"
        ).arg(COLOR_TEXT_PLACEHOLDER.name()).arg(scale(14)).arg(scale(20)).arg(scale(5)));

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setAlignment(Qt::AlignCenter);
    buttonLayout->setSpacing(scale(10));

    m_cancelButton = new QPushButton("Cancel");
    m_cancelButton->setCursor(Qt::PointingHandCursor);
    m_cancelButton->setFixedWidth(scale(120));
    m_cancelButton->setMinimumHeight(scale(42));
    m_cancelButton->setStyleSheet(QString(
        "QPushButton {"
        "   background-color: #f0f0f0;"
        "   color: #333333;"
        "   border: none;"
        "   border-radius: %1px;"
        "   padding: %2px %3px;"
        "   font-family: 'Outfit';"
        "   font-size: %4px;"
        "   font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "   background-color: #dcdcdc;"
        "}"
        "QPushButton:pressed {"
        "   background-color: #d0d0d0;"
        "}"
        ).arg(scale(8)).arg(scale(10)).arg(scale(20)).arg(scale(13)));
    m_cancelButton->setFont(font);

    m_okButton = new QPushButton("End");
    m_okButton->setCursor(Qt::PointingHandCursor);
    m_okButton->setFixedWidth(scale(120));
    m_okButton->setMinimumHeight(scale(42));
    m_okButton->setStyleSheet(QString(
        "QPushButton {"
        "   background-color: rgba(232, 53, 53, 0.15);"
        "   color: %1;"
        "   border: none;"
        "   border-radius: %2px;"
        "   padding: %3px %4px;"
        "   font-family: 'Outfit';"
        "   font-size: %5px;"
        "   font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "   background-color: rgba(232, 53, 53, 0.25);"
        "   color: %6;"
        "}"
        "QPushButton:pressed {"
        "   background-color: rgba(232, 53, 53, 0.35);"
        "}"
        ).arg(COLOR_BANNER_ERROR.name()).arg(scale(8)).arg(scale(10)).arg(scale(20)).arg(scale(13)).arg(COLOR_BANNER_ERROR_HOVER.name()));
    m_okButton->setFont(font);

    buttonLayout->addWidget(m_cancelButton);
    buttonLayout->addWidget(m_okButton);

    contentLayout->addWidget(titleLabel);
    contentLayout->addWidget(messageLabel);
    contentLayout->addStretch(1);
    contentLayout->addLayout(buttonLayout);

    connect(m_cancelButton, &QPushButton::clicked, this, [this]() {
        emit endMeetingCancelled();
    });
    connect(m_okButton, &QPushButton::clicked, this, [this]() {
        emit endMeetingConfirmed();
    });

    auto* escShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), this, [this]() {
        emit endMeetingCancelled();
    });
    escShortcut->setContext(Qt::ApplicationShortcut);

    setMinimumWidth(scale(400) + shadowMargin * 2);
    setMinimumHeight(scale(280) + shadowMargin * 2);
}
