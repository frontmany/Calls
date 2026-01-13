#include "dialogs/updatingDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QMovie>
#include <QGraphicsDropShadowEffect>

QString StyleUpdatingDialog::mainWidgetStyle(int borderRadius)
{
    return QString(
        "QWidget#mainWidget {"
        "   background-color: rgb(226, 243, 231);"
        "   border-radius: %1px;"
        "   border: 1px solid rgb(210, 210, 210);"
        "}"
    ).arg(borderRadius);
}

QString StyleUpdatingDialog::progressStyle()
{
    return
        "color: rgb(80, 80, 80);"
        "font-size: 14px;"
        "font-family: 'Outfit';"
        "font-weight: bold;";
}

QString StyleUpdatingDialog::titleStyle()
{
    return
        "color: rgb(60, 60, 60);"
        "font-size: 16px;"
        "font-family: 'Outfit';"
        "font-weight: bold;";
}

QString StyleUpdatingDialog::exitButtonStyle(int radius, int paddingH, int paddingV, int fontSize)
{
    return QString(
        "QPushButton {"
        "   background-color: transparent;"
        "   color: rgb(120, 120, 120);"
        "   border-radius: %1px;"
        "   padding: %2px %3px;"
        "   font-family: 'Outfit';"
        "   font-size: %4px;"
        "   border: none;"
        "}"
        "QPushButton:hover {"
        "   background-color: rgba(0, 0, 0, 8);"
        "   color: rgb(100, 100, 100);"
        "}"
        "QPushButton:pressed {"
        "   background-color: rgba(0, 0, 0, 15);"
        "}"
    ).arg(radius).arg(paddingV).arg(paddingH).arg(fontSize);
}

UpdatingDialog::UpdatingDialog(QWidget* parent)
    : QWidget(parent)
{
    QFont font("Outfit", 14, QFont::Normal);

    setAttribute(Qt::WA_TranslucentBackground);
    setMinimumWidth(300);
    setMinimumHeight(250);

    QGraphicsDropShadowEffect* shadowEffect = new QGraphicsDropShadowEffect();
    shadowEffect->setBlurRadius(30);
    shadowEffect->setXOffset(0);
    shadowEffect->setYOffset(0);
    shadowEffect->setColor(QColor(0, 0, 0, 150));

    QWidget* mainWidget = new QWidget(this);
    mainWidget->setObjectName("mainWidget");
    mainWidget->setGraphicsEffect(shadowEffect);
    mainWidget->setStyleSheet(StyleUpdatingDialog::mainWidgetStyle(16));

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(mainWidget);

    QVBoxLayout* contentLayout = new QVBoxLayout(mainWidget);
    contentLayout->setContentsMargins(24, 24, 24, 24);
    contentLayout->setSpacing(16);

    m_gifLabel = new QLabel();
    m_gifLabel->setAlignment(Qt::AlignCenter);
    m_gifLabel->setMinimumHeight(120);

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
    m_exitButton->setFixedWidth(120);
    m_exitButton->setMinimumHeight(36);
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

void UpdatingDialog::swapToRestarting()
{
    if (m_movie && m_movie->isValid()) m_movie->stop();
    if (m_statusLabel) m_statusLabel->setText("Restarting...");
    if (m_progressLabel) m_progressLabel->setVisible(false);
}

void UpdatingDialog::swapToUpToDate()
{
    if (m_movie && m_movie->isValid()) m_movie->stop();
    if (m_statusLabel) m_statusLabel->setText("Already up to date");
    if (m_progressLabel) m_progressLabel->setVisible(false);
}
