#include "dialogs/alreadyRunningDialog.h"
#include "utilities/utilities.h"
#include "utilities/color.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGraphicsDropShadowEffect>
#include <QPushButton>
#include <QScreen>

QString StyleAlreadyRunningDialog::mainWidgetStyle(int radius, int border)
{
    return QString(
        "QWidget#mainWidget {"
        "   background-color: %1;"
        "   border-radius: %2px;"
        "   border: %3px solid %4;"
        "}")
        .arg(COLOR_BACKGROUND_ERROR_SUBTLE.name())
        .arg(radius)
        .arg(border)
        .arg(COLOR_BORDER_NEUTRAL.name());
}

QString StyleAlreadyRunningDialog::titleStyle(int scalePx, const QString& fontFamily)
{
    return QString(
        "color: %1;"
        "font-size: %2px;"
        "font-family: '%3';"
        "font-weight: bold;"
    ).arg(COLOR_TEXT_TERTIARY.name())
     .arg(scalePx).arg(fontFamily);
}

QString StyleAlreadyRunningDialog::messageStyle(int scalePx, const QString& fontFamily)
{
    return QString(
        "color: %1;"
        "font-size: %2px;"
        "font-family: '%3';"
        "font-weight: normal;"
    ).arg(COLOR_TEXT_PLACEHOLDER.name())
     .arg(scalePx).arg(fontFamily);
}

QString StyleAlreadyRunningDialog::closeButtonStyle(int radius, int paddingH, int paddingV, int fontSize)
{
    return QString(
        "QPushButton {"
        "   background-color: %1;"
        "   color: %2;"
        "   border-radius: %3px;"
        "   padding: %4px %5px;"
        "   font-family: 'Outfit';"
        "   font-size: %6px;"
        "   border: none;"
        "}"
        "QPushButton:hover {"
        "   background-color: %7;"
        "   color: %8;"
        "}"
        "QPushButton:pressed {"
        "   background-color: %9;"
        "}"
    ).arg(QColor(232, 53, 53, 51).name(QColor::HexArgb))  // %1 - background-color (0.20 * 255 ≈ 51)
     .arg(COLOR_BANNER_ERROR.name())                        // %2 - color
     .arg(radius)                                           // %3 - border-radius
     .arg(paddingV)                                         // %4 - padding vertical
     .arg(paddingH)                                         // %5 - padding horizontal
     .arg(fontSize)                                         // %6 - font-size
     .arg(QColor(232, 53, 53, 64).name(QColor::HexArgb))  // %7 - hover background-color (0.25 * 255 ≈ 64)
     .arg(COLOR_BANNER_ERROR_HOVER.name())                 // %8 - hover color
     .arg(QColor(232, 53, 53, 89).name(QColor::HexArgb));  // %9 - pressed background-color (0.35 * 255 ≈ 89)
}

AlreadyRunningDialog::AlreadyRunningDialog(QWidget* parent)
    : QWidget(parent)
{
    QFont font("Outfit", scale(14), QFont::Normal);

    setAttribute(Qt::WA_TranslucentBackground);
    setMinimumWidth(scale(420));
    setMinimumHeight(scale(340));

    QGraphicsDropShadowEffect* shadowEffect = new QGraphicsDropShadowEffect();
    shadowEffect->setBlurRadius(scale(30));
    shadowEffect->setXOffset(0);
    shadowEffect->setYOffset(0);
    shadowEffect->setColor(COLOR_SHADOW_STRONG_150);

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
    imageLabel->setFixedSize(scale(128), scale(128));
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
