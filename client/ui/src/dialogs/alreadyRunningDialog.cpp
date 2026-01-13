#include "dialogs/alreadyRunningDialog.h"
#include "utilities/utilities.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGraphicsDropShadowEffect>
#include <QPushButton>
#include <QScreen>

QString StyleAlreadyRunningDialog::mainWidgetStyle(int radius, int border)
{
    return QString(
        "QWidget#mainWidget {"
        "   background-color: rgb(255, 240, 240);"
        "   border-radius: %1px;"
        "   border: %2px solid rgb(210, 210, 210);"
        "}")
        .arg(radius)
        .arg(border);
}

QString StyleAlreadyRunningDialog::titleStyle(int scalePx, const QString& fontFamily)
{
    return QString(
        "color: rgb(60, 60, 60);"
        "font-size: %1px;"
        "font-family: '%2';"
        "font-weight: bold;"
    ).arg(scalePx).arg(fontFamily);
}

QString StyleAlreadyRunningDialog::messageStyle(int scalePx, const QString& fontFamily)
{
    return QString(
        "color: rgb(120, 120, 120);"
        "font-size: %1px;"
        "font-family: '%2';"
        "font-weight: normal;"
    ).arg(scalePx).arg(fontFamily);
}

QString StyleAlreadyRunningDialog::closeButtonStyle(int radius, int paddingH, int paddingV, int fontSize)
{
    return QString(
        "QPushButton {"
        "   background-color: rgba(232, 53, 53, 0.20);"
        "   color: rgb(232, 53, 53);"
        "   border-radius: %1px;"
        "   padding: %2px %3px;"
        "   font-family: 'Outfit';"
        "   font-size: %4px;"
        "   border: none;"
        "}"
        "QPushButton:hover {"
        "   background-color: rgba(232, 53, 53, 0.25);"
        "   color: rgb(212, 43, 43);"
        "}"
        "QPushButton:pressed {"
        "   background-color: rgba(232, 53, 53, 0.35);"
        "}"
    ).arg(radius).arg(paddingV).arg(paddingH).arg(fontSize);
}

AlreadyRunningDialog::AlreadyRunningDialog(QWidget* parent)
    : QWidget(parent)
{
    QFont font("Outfit", scale(14), QFont::Normal);

    setAttribute(Qt::WA_TranslucentBackground);
    setMinimumWidth(scale(400));
    setMinimumHeight(scale(320));

    QGraphicsDropShadowEffect* shadowEffect = new QGraphicsDropShadowEffect();
    shadowEffect->setBlurRadius(scale(30));
    shadowEffect->setXOffset(0);
    shadowEffect->setYOffset(0);
    shadowEffect->setColor(QColor(0, 0, 0, 150));

    QWidget* mainWidget = new QWidget(this);
    mainWidget->setGraphicsEffect(shadowEffect);
    mainWidget->setObjectName("mainWidget");
    mainWidget->setStyleSheet(StyleAlreadyRunningDialog::mainWidgetStyle(scale(16), scale(1)));

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(mainWidget);

    QVBoxLayout* contentLayout = new QVBoxLayout(mainWidget);
    contentLayout->setContentsMargins(scale(32), scale(32), scale(32), scale(32));
    contentLayout->setSpacing(scale(20));
    contentLayout->setAlignment(Qt::AlignTop);

    QLabel* imageLabel = new QLabel();
    imageLabel->setFixedSize(scale(96), scale(96));
    imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel->setStyleSheet("background-color: transparent;");
    QPixmap pix(":/resources/multipleInstancesError.png");
    if (!pix.isNull()) {
        imageLabel->setPixmap(pix.scaled(imageLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }

    QLabel* titleLabel = new QLabel("Callifornia is already running");
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(StyleAlreadyRunningDialog::titleStyle(scale(18)));
    titleLabel->setFont(QFont("Outfit", scale(18), QFont::Bold));

    QLabel* messageLabel = new QLabel("Please close the other instance before starting a new one.");
    messageLabel->setAlignment(Qt::AlignCenter);
    messageLabel->setStyleSheet(StyleAlreadyRunningDialog::messageStyle(scale(14)));
    messageLabel->setWordWrap(true);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setAlignment(Qt::AlignCenter);

    QPushButton* closeButton = new QPushButton("Close");
    closeButton->setFixedWidth(scale(140));
    closeButton->setMinimumHeight(scale(42));
    closeButton->setCursor(Qt::PointingHandCursor);
    closeButton->setStyleSheet(StyleAlreadyRunningDialog::closeButtonStyle(scale(8), scale(20), scale(10), scale(13)));
    closeButton->setFont(font);

    buttonLayout->addWidget(closeButton);

    contentLayout->addWidget(imageLabel, 0, Qt::AlignCenter);
    contentLayout->addWidget(titleLabel, 0, Qt::AlignCenter);
    contentLayout->addWidget(messageLabel, 0, Qt::AlignCenter);
    contentLayout->addStretch(1);
    contentLayout->addLayout(buttonLayout);

    connect(closeButton, &QPushButton::clicked, this, &AlreadyRunningDialog::closeRequested);
}
