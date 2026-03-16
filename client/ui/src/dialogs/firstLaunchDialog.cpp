#include "dialogs/firstLaunchDialog.h"
#include "utilities/utility.h"
#include "constants/color.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGraphicsDropShadowEffect>
#include <QPushButton>
#include <QPainter>
#include <QPainterPath>
#include <QFile>
#include <QTextStream>
#include <QScreen>

QString StyleFirstLaunchDialog::mainWidgetStyle(int radius, int border)
{
    return QString(
        "QWidget#mainWidget {"
        "   background-color: %1;"
        "   border-radius: %2px;"
        "   border: %3px solid %4;"
        "}")
        .arg(COLOR_BACKGROUND_PURE.name())
        .arg(radius)
        .arg(border)
        .arg(COLOR_BORDER_SUBTLE.name(QColor::HexArgb));
}

QString StyleFirstLaunchDialog::titleStyle()
{
    return QString(
        "color: %1;"
        "background-color: transparent;"
    ).arg(COLOR_TEXT_SECONDARY.name());
}

QString StyleFirstLaunchDialog::descriptionStyle(int fontSize, int padding)
{
    return QString(
        "color: %1;"
        "font-size: %2px;"
        "font-family: 'Outfit';"
        "line-height: 1.6;"
        "padding: %3px;"
        "background-color: transparent;"
        "text-align: left;"
    ).arg(COLOR_TEXT_TERTIARY.name())
     .arg(fontSize).arg(padding);
}

QString StyleFirstLaunchDialog::okButtonStyle(int radius, int fontSize)
{
    return QString(
        "QPushButton {"
        "   background-color: %1;"
        "   color: %2;"
        "   border-radius: %3px;"
        "   padding: %4px %5px;"
        "   font-family: 'Outfit';"
        "   font-size: %6px;"
        "   font-weight: 600;"
        "   border: none;"
        "}"
        "QPushButton:hover {"
        "   background-color: %7;"
        "}"
        "QPushButton:pressed {"
        "   background-color: %8;"
        "}"
        "QPushButton:focus {"
        "   outline: none;"
        "   border: none;"
        "}"
    ).arg(COLOR_ACCENT.name())
     .arg(COLOR_BACKGROUND_PURE.name())
     .arg(radius)
     .arg(scale(8)).arg(scale(16))
     .arg(fontSize)
     .arg(COLOR_ACCENT_HOVER.name())
     .arg(COLOR_ACCENT_PRESSED.name())
     .arg(scale(8))
     .arg(scale(16));
}

FirstLaunchDialog::FirstLaunchDialog(QWidget* parent, const QString& imagePath, const QString& descriptionText)
    : QWidget(parent)
{
    QFont baseFont("Outfit", scale(16), QFont::Light);
    QFont titleFont("Outfit", scale(16), QFont::Normal);

    setAttribute(Qt::WA_TranslucentBackground);
    setFixedWidth(scale(370));
    
    QGraphicsDropShadowEffect* shadowEffect = new QGraphicsDropShadowEffect();
    shadowEffect->setBlurRadius(scale(36));
    shadowEffect->setXOffset(0);
    shadowEffect->setYOffset(scale(6));
    shadowEffect->setColor(COLOR_SHADOW_MEDIUM_60);

    QWidget* mainWidget = new QWidget(this);
    mainWidget->setGraphicsEffect(shadowEffect);
    mainWidget->setObjectName("mainWidget");
    mainWidget->setStyleSheet(StyleFirstLaunchDialog::mainWidgetStyle(scale(20), scale(1)));

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(mainWidget);

    QVBoxLayout* contentLayout = new QVBoxLayout(mainWidget);
    contentLayout->setContentsMargins(scale(32), scale(34), scale(32), scale(30));
    contentLayout->setSpacing(scale(18));

    auto createRoundedPixmap = [](const QPixmap& source, int maxSize, int radius) -> QPixmap {
        QPixmap scaled = source.scaled(maxSize, maxSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        int width = scaled.width();
        int height = scaled.height();

        QPixmap rounded(width, height);
        rounded.fill(Qt::transparent);

        QPainter painter(&rounded);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

        QPainterPath path;
        path.addRoundedRect(0, 0, width, height, radius, radius);
        painter.setClipPath(path);

        painter.drawPixmap(0, 0, scaled);
        painter.end();

        return rounded;
    };

    QLabel* imageLabel = new QLabel();
    imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel->setMinimumHeight(scale(280));
    imageLabel->setStyleSheet("background-color: transparent;");

    QString actualImagePath = imagePath.isEmpty() ? ":/resources/welcome.jpg" : imagePath;
    QPixmap welcomePixmap(actualImagePath);
    int imageSize = scale(270);
    int cornerRadius = scale(16);

    if (welcomePixmap.isNull()) {
        QPixmap placeholder(imageSize, imageSize);
        placeholder.fill(Qt::transparent);
        imageLabel->setPixmap(createRoundedPixmap(placeholder, imageSize, cornerRadius));
    }
    else {
        QPixmap roundedPixmap = createRoundedPixmap(welcomePixmap, imageSize, cornerRadius);
        imageLabel->setPixmap(roundedPixmap);
    }

    QString actualDescription = descriptionText;
    if (actualDescription.isEmpty()) {
        QFile textFile("welcomeText.txt");
        if (textFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&textFile);
            actualDescription = in.readAll();
            textFile.close();
        }

        if (actualDescription.isEmpty()) {
            actualDescription = QString("Welcome to new Callifornia!\n\n"
                "What's new:\n"
                "• Improved call quality\n"
                "• Better screen sharing performance\n"
                "• Enhanced UI design\n"
                "• Bug fixes and stability improvements");
        }
    }

    QLabel* titleLabel = new QLabel("Welcome to new Callifornia");
    titleLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    titleLabel->setStyleSheet(StyleFirstLaunchDialog::titleStyle());
    titleLabel->setFont(titleFont);

    QString formattedDescription = actualDescription.toHtmlEscaped();
    formattedDescription.replace("\n\n", "</p><p>");
    formattedDescription.replace("\n", "<br/>");
    formattedDescription = QString("<p style=\"margin:0; line-height:180%%;\">%1</p>").arg(formattedDescription);

    QLabel* descriptionLabel = new QLabel(actualDescription);
    descriptionLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    descriptionLabel->setWordWrap(true);
    descriptionLabel->setTextFormat(Qt::RichText);
    descriptionLabel->setText(formattedDescription);
    descriptionLabel->setStyleSheet(StyleFirstLaunchDialog::descriptionStyle(scale(15), scale(0)));
    descriptionLabel->setFont(baseFont);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setAlignment(Qt::AlignCenter);
    buttonLayout->addStretch();

    QPushButton* okButton = new QPushButton("OK");
    okButton->setFixedWidth(scale(270));
    okButton->setMinimumHeight(scale(46));
    okButton->setCursor(Qt::PointingHandCursor);
    okButton->setStyleSheet(StyleFirstLaunchDialog::okButtonStyle(scale(14), scale(15)));
    okButton->setFont(baseFont);

    buttonLayout->addWidget(okButton);
    buttonLayout->addStretch();

    contentLayout->addWidget(imageLabel, 0, Qt::AlignCenter);

    QWidget* textContainer = new QWidget();
    textContainer->setStyleSheet("background-color: transparent;");
    textContainer->setMaximumWidth(scale(440));
    QHBoxLayout* textContainerLayout = new QHBoxLayout(textContainer);
    textContainerLayout->setContentsMargins(0, 0, 0, 0);
    textContainerLayout->setSpacing(scale(10));
    textContainerLayout->addStretch();
    QVBoxLayout* textLayout = new QVBoxLayout();
    textLayout->setContentsMargins(0, 0, 0, 0);
    textLayout->setSpacing(scale(14));
    textLayout->addWidget(titleLabel, 0, Qt::AlignLeft);
    textLayout->addWidget(descriptionLabel, 0, Qt::AlignLeft);
    textContainerLayout->addLayout(textLayout, 0);
    textContainerLayout->addStretch();
    contentLayout->addWidget(textContainer, 0, Qt::AlignCenter);

    contentLayout->addSpacing(scale(20));
    contentLayout->addStretch(1);
    contentLayout->addLayout(buttonLayout);

    connect(okButton, &QPushButton::clicked, this, &FirstLaunchDialog::closeRequested);
}
