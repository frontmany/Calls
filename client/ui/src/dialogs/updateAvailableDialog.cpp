#include "dialogs/updateAvailableDialog.h"
#include "utilities/utility.h"
#include "constants/color.h"
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
        .arg(COLOR_ACCENT.name())      
        .arg(COLOR_BACKGROUND_PURE.name())         
        .arg(scale(18))                           
        .arg(COLOR_ACCENT_HOVER.name())  
        .arg(COLOR_BACKGROUND_PURE.name())               
        .arg(COLOR_ACCENT_PRESSED.name()) 
        .arg(COLOR_BACKGROUND_PURE.name());             
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
    buttonLayout->setContentsMargins(scale(15), 0, scale(15), 0);
    buttonLayout->setSpacing(scale(8));
    buttonLayout->setAlignment(Qt::AlignVCenter);

    m_buttonTextLabel = new QLabel(buttonWidget);
    QFont updateFont("Outfit", scale(13), QFont::Medium);
    m_buttonTextLabel->setFont(updateFont);
    m_buttonTextLabel->setStyleSheet(QString("color: %1; background: transparent;").arg(COLOR_BACKGROUND_PURE.name()));
    m_buttonTextLabel->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    m_buttonTextLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    m_buttonTextLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    updateButtonText();

    m_confettiLabel = new QLabel(buttonWidget);
    m_confettiLabel->setPixmap(QPixmap(":/resources/toRightArrow.png"));
    m_confettiLabel->setScaledContents(true);
    m_confettiLabel->setFixedSize(scale(26), scale(26));
    m_confettiLabel->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    m_confettiLabel->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
    m_confettiLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    buttonLayout->addWidget(m_buttonTextLabel, 0, Qt::AlignVCenter);
    buttonLayout->addWidget(m_confettiLabel, 0, Qt::AlignVCenter);

    m_updateButton = new QPushButton(this);
    m_updateButton->setMinimumSize(scale(265), scale(38));
    m_updateButton->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    m_updateButton->setCursor(Qt::PointingHandCursor);
    m_updateButton->setLayout(buttonLayout);
    m_updateButton->setStyleSheet(StyleUpdateAvailableDialog::buttonStyle());

    mainLayout->addWidget(m_updateButton, 0, Qt::AlignCenter);

    connect(m_updateButton, &QPushButton::clicked, this, &UpdateAvailableDialog::updateButtonClicked);
}

void UpdateAvailableDialog::setNewVersion(const QString& newVersion)
{
    if (m_newVersion != newVersion) {
        m_newVersion = newVersion;
        updateButtonText();
    }
}

void UpdateAvailableDialog::updateButtonText()
{
    QString text = m_newVersion.isEmpty()
        ? QStringLiteral("New version available! Click to update")
        : QStringLiteral("New version available  %1").arg(m_newVersion);
    m_buttonTextLabel->setText(text);
}
