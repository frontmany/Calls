#include "dialogs/updatingDialog.h"
#include "constants/color.h"
#include "utilities/utility.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QMovie>
#include <QGraphicsDropShadowEffect>

QString StyleUpdatingDialog::mainWidgetStyle(int borderRadius)
{
    return QString(
        "QWidget#mainWidget {"
        "   background-color: %1;"
        "   border-radius: %2px;"
        "   border: %3px solid %4;"
        "}"
    ).arg(COLOR_BACKGROUND_UPDATE.name())
     .arg(borderRadius)
     .arg(scale(1))
     .arg(COLOR_BORDER_NEUTRAL.name());
}

QString StyleUpdatingDialog::progressStyle()
{
    return QString(
        "color: %1;"
        "font-size: %2px;"
        "font-family: 'Outfit';"
        "font-weight: bold;"
    ).arg(COLOR_NEUTRAL_80.name())
     .arg(scale(14));
}

QString StyleUpdatingDialog::titleStyle()
{
    return QString(
        "color: %1;"
        "font-size: %2px;"
        "font-family: 'Outfit';"
        "font-weight: bold;"
    ).arg(COLOR_TEXT_TERTIARY.name())
     .arg(scale(16));
}

QString StyleUpdatingDialog::exitButtonStyle(int radius, int paddingH, int paddingV, int fontSize)
{
    return QString(
        "QPushButton {"
        "   background-color: transparent;"
        "   color: %1;"
        "   border-radius: %2px;"
        "   padding: %3px %4px;"
        "   font-family: 'Outfit';"
        "   font-size: %5px;"
        "   border: none;"
        "}"
        "QPushButton:hover {"
        "   background-color: rgba(%6, %7, %8, %9);"
        "   color: %10;"
        "}"
        "QPushButton:pressed {"
        "   background-color: rgba(%11, %12, %13, %14);"
        "}"
    ).arg(COLOR_TEXT_PLACEHOLDER.name())
     .arg(radius).arg(paddingV).arg(paddingH).arg(fontSize)
     .arg(COLOR_SHADOW_SUBTLE_8.red())
     .arg(COLOR_SHADOW_SUBTLE_8.green())
     .arg(COLOR_SHADOW_SUBTLE_8.blue())
     .arg(COLOR_SHADOW_SUBTLE_8.alpha())
     .arg(COLOR_TEXT_NEUTRAL_100.name())
     .arg(COLOR_SHADOW_SUBTLE_15.red())
     .arg(COLOR_SHADOW_SUBTLE_15.green())
     .arg(COLOR_SHADOW_SUBTLE_15.blue())
     .arg(COLOR_SHADOW_SUBTLE_15.alpha());
}

UpdatingDialog::UpdatingDialog(QWidget* parent)
    : QWidget(parent)
{
    QFont font("Outfit", scale(14), QFont::Normal);

    setAttribute(Qt::WA_TranslucentBackground);
    setMinimumWidth(scale(300));
    setMinimumHeight(scale(250));

    QGraphicsDropShadowEffect* shadowEffect = new QGraphicsDropShadowEffect();
    shadowEffect->setBlurRadius(30);
    shadowEffect->setXOffset(0);
    shadowEffect->setYOffset(0);
    shadowEffect->setColor(COLOR_SHADOW_STRONG_150);

    QWidget* mainWidget = new QWidget(this);
    mainWidget->setObjectName("mainWidget");
    mainWidget->setGraphicsEffect(shadowEffect);
    mainWidget->setStyleSheet(StyleUpdatingDialog::mainWidgetStyle(16));

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(mainWidget);

    QVBoxLayout* contentLayout = new QVBoxLayout(mainWidget);
    contentLayout->setContentsMargins(scale(24), scale(24), scale(24), scale(24));
    contentLayout->setSpacing(scale(16));

    m_gifLabel = new QLabel();
    m_gifLabel->setAlignment(Qt::AlignCenter);
    m_gifLabel->setMinimumHeight(scale(120));

    m_movie = new QMovie(":/resources/updating.gif");
    if (m_movie->isValid())
    {
        m_gifLabel->setMovie(m_movie);
        m_movie->start();
    }

    m_progressLabel = new QLabel("0.00%");
    m_progressLabel->setAlignment(Qt::AlignCenter);
    m_progressLabel->setStyleSheet(StyleUpdatingDialog::progressStyle());

    m_statusLabel = new QLabel("Updating...");
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setStyleSheet(StyleUpdatingDialog::titleStyle());
    m_statusLabel->setFont(font);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setAlignment(Qt::AlignCenter);

    m_exitButton = new QPushButton("Exit");
    m_exitButton->setFixedWidth(scale(120));
    m_exitButton->setMinimumHeight(scale(36));
    m_exitButton->setStyleSheet(StyleUpdatingDialog::exitButtonStyle(8, 12, 6, 13));
    m_exitButton->setFont(font);

    buttonLayout->addWidget(m_exitButton);

    contentLayout->addWidget(m_gifLabel);
    contentLayout->addWidget(m_progressLabel);
    contentLayout->addWidget(m_statusLabel);
    contentLayout->addLayout(buttonLayout);

    connect(m_exitButton, &QPushButton::clicked, this, &UpdatingDialog::closeRequested);
}

void UpdatingDialog::setProgress(double progressPercent)
{
    if (m_progressLabel)
    {
        m_progressLabel->setText(QString::number(progressPercent, 'f', 2) + "%");
        m_progressLabel->setVisible(true);
    }
}

void UpdatingDialog::setStatus(const QString& text, bool hideProgress)
{
    if (m_statusLabel)
    {
        m_statusLabel->setText(text);
    }
    if (m_progressLabel)
    {
        m_progressLabel->setVisible(!hideProgress);
    }
    if (m_movie && m_movie->isValid())
    {
        m_movie->stop();
    }
}
