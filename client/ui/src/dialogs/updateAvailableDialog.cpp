#include "dialogs/updateAvailableDialog.h"
#include "utilities/utilities.h"
#include "utilities/color.h"
#include <QHBoxLayout>
#include <QVBoxLayout>

QString StyleUpdateAvailableDialog::buttonStyle()
{
    return QString("QPushButton {"
        "   background-color: %1;"
        "   color: %2;"
        "   border: none;"
        "   border-radius: %3px;"
        "   padding: 0px;"
        "   margin: 0px;"
        "}"
        "QPushButton:hover {"
        "   background-color: %4;"
        "   color: %5;"
        "}"
        "QPushButton:pressed {"
        "   background-color: %6;"
        "   color: %7;"
        "}")
        .arg(COLOR_UPDATE_AVAILABLE.name())        // %1 - background-color
        .arg(COLOR_BACKGROUND_PURE.name())         // %2 - color (белый текст)
        .arg(scale(18))                            // %3 - border-radius
        .arg(COLOR_UPDATE_AVAILABLE_HOVER.name())   // %4 - hover background-color
        .arg(COLOR_BACKGROUND_PURE.name())         // %5 - hover color (белый текст)
        .arg(COLOR_UPDATE_AVAILABLE_PRESSED.name()) // %6 - pressed background-color
        .arg(COLOR_BACKGROUND_PURE.name());        // %7 - pressed color (белый текст)
}

QString StyleUpdateAvailableDialog::containerStyle()
{
    return QString("QWidget {"
        "   background-color: transparent;"
        "   border: none;"
        "}");
}

UpdateAvailableDialog::UpdateAvailableDialog(QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setStyleSheet(StyleUpdateAvailableDialog::containerStyle());

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Create button with custom layout inside
    QWidget* buttonWidget = new QWidget();
    buttonWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QHBoxLayout* buttonLayout = new QHBoxLayout(buttonWidget);
    buttonLayout->setContentsMargins(scale(15), 0, scale(18), 0);
    buttonLayout->setSpacing(scale(8));
    buttonLayout->setAlignment(Qt::AlignVCenter);

    QLabel* buttonTextLabel = new QLabel("New version! Click to update", buttonWidget);
    QFont updateFont("Outfit", scale(13), QFont::Medium);
    buttonTextLabel->setFont(updateFont);
    buttonTextLabel->setStyleSheet(QString("color: %1; background: transparent;").arg(COLOR_BACKGROUND_PURE.name()));
    buttonTextLabel->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    buttonTextLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    buttonTextLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

    m_confettiLabel = new QLabel(buttonWidget);
    m_confettiLabel->setPixmap(QPixmap(":/resources/confetti.png"));
    m_confettiLabel->setScaledContents(true);
    m_confettiLabel->setFixedSize(scale(22), scale(22));
    m_confettiLabel->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    m_confettiLabel->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
    m_confettiLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    buttonLayout->addWidget(buttonTextLabel, 0, Qt::AlignVCenter);
    buttonLayout->addWidget(m_confettiLabel, 0, Qt::AlignVCenter);

    m_updateButton = new QPushButton(this);
    m_updateButton->setMinimumSize(scale(280), scale(38));
    m_updateButton->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    m_updateButton->setCursor(Qt::PointingHandCursor);
    m_updateButton->setLayout(buttonLayout);
    m_updateButton->setStyleSheet(StyleUpdateAvailableDialog::buttonStyle());

    mainLayout->addWidget(m_updateButton, 0, Qt::AlignCenter);

    connect(m_updateButton, &QPushButton::clicked, this, &UpdateAvailableDialog::updateButtonClicked);
}
