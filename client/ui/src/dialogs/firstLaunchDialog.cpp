#include "dialogs/firstLaunchDialog.h"
#include "utilities/utilities.h"
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
        "   background-color: rgb(245, 245, 245);"
        "   border-radius: %1px;"
        "   border: %2px solid rgb(210, 210, 210);"
        "}")
        .arg(radius)
        .arg(border);
}

QString StyleFirstLaunchDialog::descriptionStyle(int fontSize, int padding)
{
    return QString(
        "color: rgb(60, 60, 60);"
        "font-size: %1px;"
        "font-family: 'Outfit';"
        "line-height: 1.6;"
        "padding: %2px;"
        "background-color: transparent;"
        "text-align: left;"
    ).arg(fontSize).arg(padding);
}

QString StyleFirstLaunchDialog::okButtonStyle(int radius, int fontSize)
{
    return QString(
        "QPushButton {"
        "   background-color: rgb(21, 119, 232);"
        "   color: rgb(255, 255, 255);"
        "   border-radius: %1px;"
        "   padding: 8px 16px;"
        "   font-family: 'Outfit';"
        "   font-size: %2px;"
        "   font-weight: 500;"
        "   border: none;"
        "}"
        "QPushButton:hover {"
        "   background-color: rgb(18, 113, 222);"
        "}"
        "QPushButton:pressed {"
        "   background-color: rgb(16, 107, 212);"
        "}"
        "QPushButton:focus {"
        "   outline: none;"
        "   border: none;"
        "}"
    ).arg(radius).arg(fontSize);
}

FirstLaunchDialog::FirstLaunchDialog(QWidget* parent, const QString& imagePath, const QString& descriptionText)
    : QWidget(parent)
{
    QFont font("Outfit", scale(14), QFont::Normal);

    setAttribute(Qt::WA_TranslucentBackground);
    setMinimumWidth(scale(500));
    setMinimumHeight(scale(400));

    QGraphicsDropShadowEffect* shadowEffect = new QGraphicsDropShadowEffect();
    shadowEffect->setBlurRadius(scale(30));
    shadowEffect->setXOffset(0);
    shadowEffect->setYOffset(0);
    shadowEffect->setColor(QColor(0, 0, 0, 150));

    QWidget* mainWidget = new QWidget(this);
    mainWidget->setGraphicsEffect(shadowEffect);
    mainWidget->setObjectName("mainWidget");
    mainWidget->setStyleSheet(StyleFirstLaunchDialog::mainWidgetStyle(scale(16), scale(1)));

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(mainWidget);

    QVBoxLayout* contentLayout = new QVBoxLayout(mainWidget);
    contentLayout->setContentsMargins(scale(32), scale(32), scale(32), scale(32));
    contentLayout->setSpacing(scale(24));

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
    int imageSize = scale(330);
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
            actualDescription = QString("Welcome to Callifornia!\n\n"
                "What's new:\n"
                "• Improved call quality\n"
                "• Better screen sharing performance\n"
                "• Enhanced UI design\n"
                "• Bug fixes and stability improvements");
        }
    }

    QLabel* descriptionLabel = new QLabel(actualDescription);
    descriptionLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    descriptionLabel->setWordWrap(true);
    descriptionLabel->setStyleSheet(StyleFirstLaunchDialog::descriptionStyle(scale(15), scale(8)));
    descriptionLabel->setFont(font);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setAlignment(Qt::AlignCenter);
    buttonLayout->addStretch();

    QPushButton* okButton = new QPushButton("OK");
    okButton->setFixedWidth(scale(120));
    okButton->setMinimumHeight(scale(42));
    okButton->setCursor(Qt::PointingHandCursor);
    okButton->setStyleSheet(StyleFirstLaunchDialog::okButtonStyle(scale(15), scale(14)));
    okButton->setFont(font);

    buttonLayout->addWidget(okButton);
    buttonLayout->addStretch();

    contentLayout->addWidget(imageLabel, 0, Qt::AlignCenter);

    QWidget* textContainer = new QWidget();
    textContainer->setStyleSheet("background-color: transparent;");
    QHBoxLayout* textContainerLayout = new QHBoxLayout(textContainer);
    textContainerLayout->setContentsMargins(0, 0, 0, 0);
    textContainerLayout->setSpacing(0);
    textContainerLayout->addStretch();
    textContainerLayout->addWidget(descriptionLabel, 0, Qt::AlignLeft);
    textContainerLayout->addStretch();
    contentLayout->addWidget(textContainer, 0, Qt::AlignCenter);

    contentLayout->addStretch(1);
    contentLayout->addSpacing(scale(20));
    contentLayout->addLayout(buttonLayout);

    connect(okButton, &QPushButton::clicked, this, &FirstLaunchDialog::closeRequested);
}
