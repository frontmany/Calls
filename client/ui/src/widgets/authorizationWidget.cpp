#include "authorizationWidget.h"

#include <QRegularExpressionValidator>
#include <QShortcut>
#include <QApplication>
#include <QEventLoop>

#include "../utilities/utilities.h"

const QColor StyleAuthorizationWidget::m_primaryColor = QColor(21, 119, 232);
const QColor StyleAuthorizationWidget::m_hoverColor = QColor(18, 113, 222);
const QColor StyleAuthorizationWidget::m_errorColor = QColor(220, 80, 80, 200);
const QColor StyleAuthorizationWidget::m_successColor = QColor(100, 200, 100, 200);
const QColor StyleAuthorizationWidget::m_textColor = QColor(1, 11, 19);
const QColor StyleAuthorizationWidget::m_backgroundColor = QColor(245, 245, 245);
const QColor StyleAuthorizationWidget::m_glassColor = QColor(255, 255, 255, 60);
const QColor StyleAuthorizationWidget::m_glassBorderColor = QColor(255, 255, 255, 100);
const QColor StyleAuthorizationWidget::m_textDarkColor = QColor(240, 240, 240);
const QColor StyleAuthorizationWidget::m_disabledColor = QColor(160, 160, 160, 150);
const QColor StyleAuthorizationWidget::m_updateAvailableColor = QColor(21, 119, 232);

QString StyleAuthorizationWidget::glassButtonStyle() {
    return QString("QPushButton {"
        "   background-color: %1;"
        "   color: %2;"
        "   border: 0px solid %3;"
        "   border-radius: %4px;"
        "   padding: %5px %6px;"
        "   margin: 0px;"
        "}"
        "QPushButton:hover {"
        "   background-color: %7;"
        "}"
        "QPushButton:focus {"
        "   outline: none;"
        "   border: none;"
        "}"
        "QPushButton:disabled {"
        "   background-color: rgba(21, 119, 232, 150);"
        "   opacity: 0.6;"
        "}").arg(m_primaryColor.name())
        .arg(m_glassBorderColor.name())
        .arg(m_glassBorderColor.name())
        .arg(QString::fromStdString(std::to_string(scale(15))))
        .arg(QString::fromStdString(std::to_string(scale(12))))
        .arg(QString::fromStdString(std::to_string(scale(24))))
        .arg(m_primaryColor.darker(110).name());
}

QString StyleAuthorizationWidget::glassLineEditStyle() {
    return QString("QLineEdit {"
        "   background-color: rgba(245, 245, 245, 235);"
        "   border: 0px solid %1;"
        "   border-radius: %4px;"
        "   padding: %5px %6px;"
        "   margin: 0px;"
        "   color: %2;"
        "   selection-background-color: %3;"
        "}"
        "QLineEdit:focus {"
        "   border: 0px solid %3;"
        "   background-color: rgba(255, 255, 255, 235);"
        "}"
        "QLineEdit:disabled {"
        "   background-color: rgba(245, 245, 245, 150);"
        "   border: 0px solid %1;"
        "   opacity: 0.7;"
        "}"
        "QLineEdit::placeholder {"
        "   color: rgba(240, 240, 240, 180);"
        "}").arg(m_glassBorderColor.name())
        .arg(m_textColor.name())
        .arg(m_primaryColor.name())
        .arg(QString::fromStdString(std::to_string(scale(12))))
        .arg(QString::fromStdString(std::to_string(scale(12))))
        .arg(QString::fromStdString(std::to_string(scale(15))));
}

QString StyleAuthorizationWidget::glassLabelStyle() {
    return QString("QLabel {"
        "   color: %1;"
        "   font-size: 14px;"
        "   font-weight: bold;"
        "   margin: 0px;"
        "   background: transparent;"
        "}").arg(m_textColor.name());
}

QString StyleAuthorizationWidget::glassErrorLabelStyle() {
    return QString("QLabel {"
        "   color: %1;"
        "   margin: 2px 2px;"
        "   padding: 5px;"
        "   background: transparent;"
        "   border-radius: 5px;"
        "}").arg(m_errorColor.name());
}

QString StyleAuthorizationWidget::glassTitleLabelStyle() {
    return QString("QLabel {"
        "   color: %1;"
        "   margin-bottom: 0px;"
        "   background: transparent;"
        "}").arg(m_textColor.name());
}

QString StyleAuthorizationWidget::glassSubTitleLabelStyle() {
    return QString("QLabel {"
        "   color: %1;"
        "   margin-bottom: 0px;"
        "   background: transparent;"
        "}").arg(m_textColor.name());
}

QString StyleAuthorizationWidget::notificationRedLabelStyle() {
    return QString("QWidget {"
        "   background-color: rgba(220, 80, 80, 65);"
        "   border: none;"
        "   border-radius: %1px;"
        "   margin: 0px;"
        "   padding: 0px;"
        "}").arg(QString::fromStdString(std::to_string(scale(8))));
}

QString StyleAuthorizationWidget::notificationGreenLabelStyle() {
    return QString("QWidget {"
        "   background-color: rgba(82, 196, 65, 100);"
        "   border: none;"
        "   border-radius: %1px;"
        "   margin: 0px;"
        "   padding: 0px;"
        "}").arg(QString::fromStdString(std::to_string(scale(8))));
}

QString StyleAuthorizationWidget::notificationLilacLabelStyle() {
    return QString("QWidget {"
        "   background-color: rgba(200, 180, 220, 80);"  
        "   border: none;"
        "   border-radius: %1px;"
        "   margin: 0px;"
        "   padding: 0px;"
        "}").arg(QString::number(scale(8)));
}

QString StyleAuthorizationWidget::notificationUpdateAvailableStyle() {
    return QString("QWidget {"
        "   background-color: transparent;"
        "   border: none;"
        "   border-radius: %1px;"
        "   margin: 0px;"
        "   padding: 0px;"
        "}").arg(QString::number(scale(8)));
}

QString StyleAuthorizationWidget::updateAvailableButtonStyle() {
    return QString("QPushButton {"
        "   background-color: rgba(21, 119, 232, 80);"
        "   color: #1577E8;"
        "   border: none;"
        "   border-radius: %1px;"
        "   padding: %2px %3px %2px %4px;"
        "   margin: 0px;"
        "}"
        "QPushButton:hover {"
        "   background-color: rgba(21, 119, 232, 120);"
        "   color: #0D6BC8;"
        "}"
        "QPushButton:pressed {"
        "   background-color: rgba(21, 119, 232, 150);"
        "   color: #0A5FC8;"
        "}").arg(QString::number(scale(12)))
        .arg(QString::number(scale(8)))
        .arg(QString::number(scale(18)))
        .arg(QString::number(scale(15)));
}

QString StyleAuthorizationWidget::notificationRedTextStyle() {
    return QString("QLabel {"
        "   color: #DC5050;"
        "   background: transparent;"
        "   font-size: %1px;"
        "   margin: 0px;"
        "   padding: 0px;"
        "}").arg(QString::number(scale(14)));
}

QString StyleAuthorizationWidget::notificationLilacTextStyle() {
    return QString("QLabel {"
        "   color: #8C6BC7;"
        "   background: transparent;"
        "   font-size: %1px;"
        "   margin: 0px;"
        "   padding: 0px;"
        "}").arg(QString::number(scale(14)));
}

QString StyleAuthorizationWidget::notificationGreenTextStyle() {
    return QString("QLabel {"
        "   color: #19ba00;"
        "   background: transparent;"
        "   font-size: %1px;"
        "   margin: 0px;"
        "   padding: 0px;"
        "}").arg(QString::number(scale(14)));
}

AuthorizationWidget::AuthorizationWidget(QWidget* parent) : QWidget(parent)
{
    setupUI();
    setupAnimations();

    m_notificationTimer = new QTimer(this);
    m_notificationTimer->setSingleShot(true);
    connect(m_notificationTimer, &QTimer::timeout, [this]() { m_notificationWidget->hide(); });

    m_updatesNotificationTimer = new QTimer(this);
    m_updatesNotificationTimer->setSingleShot(true);
    connect(m_updatesNotificationTimer, &QTimer::timeout, this, &AuthorizationWidget::hideUpdatesCheckingNotification);
}

void AuthorizationWidget::setupUI() {
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setAlignment(Qt::AlignCenter);

    m_notificationWidget = new QWidget(this);
    m_notificationWidget->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    m_notificationWidget->hide();

    m_notificationLayout = new QHBoxLayout(m_notificationWidget);
    m_notificationLayout->setAlignment(Qt::AlignCenter);
    m_notificationLayout->setContentsMargins(scale(18), scale(8), scale(18), scale(8));

    m_notificationLabel = new QLabel(m_notificationWidget);
    QFont errorFont("Outfit", scale(12), QFont::Medium);
    m_notificationLabel->setFont(errorFont);

    m_notificationLayout->addWidget(m_notificationLabel);

    m_updateAvailableWidget = new QWidget(this);
    m_updateAvailableWidget->setStyleSheet(StyleAuthorizationWidget::notificationUpdateAvailableStyle());
    m_updateAvailableWidget->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    m_updateAvailableWidget->hide();

    m_updateAvailableLayout = new QHBoxLayout(m_updateAvailableWidget);
    m_updateAvailableLayout->setAlignment(Qt::AlignCenter);
    m_updateAvailableLayout->setContentsMargins(scale(18), scale(8), scale(18), scale(8));

    m_updateAvailableButton = new QPushButton(m_updateAvailableWidget);
    m_updateAvailableButton->setMinimumSize(scale(295), scale(32));
    m_updateAvailableButton->setCursor(Qt::PointingHandCursor);
    QFont updateFont("Outfit", scale(12), QFont::Medium);
    m_updateAvailableButton->setFont(updateFont);
    m_updateAvailableButton->setText("Update available! Click to download");
    m_updateAvailableButton->setStyleSheet(StyleAuthorizationWidget::updateAvailableButtonStyle());

    m_updateAvailableLayout->addWidget(m_updateAvailableButton);

    m_container = new QWidget(this);
    m_container->setFixedSize(scale(450), scale(400));
    m_container->setAttribute(Qt::WA_TranslucentBackground);

    m_glassLayout = new QVBoxLayout(m_container);
    m_glassLayout->setSpacing(scale(15));
    m_glassLayout->setContentsMargins(scale(30), scale(30), scale(30), scale(30));
    m_glassLayout->setAlignment(Qt::AlignTop);

    m_titleLabel = new QLabel("Welcome to Callifornia", m_container);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_titleLabel->setStyleSheet(StyleAuthorizationWidget::glassTitleLabelStyle());
    m_titleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    m_titleLabel->setMinimumHeight(scale(62));
    QFont titleFont("Pacifico", scale(28));
    m_titleLabel->setFont(titleFont);

    m_subtitleLabel = new QLabel("Enter your nickname to start making calls", m_container);
    m_subtitleLabel->setAlignment(Qt::AlignCenter);
    m_subtitleLabel->setStyleSheet(StyleAuthorizationWidget::glassSubTitleLabelStyle());
    m_subtitleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    QFont subTitleFont("Outfit", scale(13), QFont::ExtraLight);
    m_subtitleLabel->setFont(subTitleFont);

    m_errorLabel = new QLabel("field cannot be empty", m_container);
    m_errorLabel->setStyleSheet(StyleAuthorizationWidget::glassErrorLabelStyle());
    m_errorLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    m_errorLabel->hide();
    QFont errorLabelFont("Outfit", scale(10), QFont::ExtraLight);
    m_errorLabel->setFont(errorLabelFont);

    m_nicknameEdit = new QLineEdit(m_container);
    m_nicknameEdit->setFixedSize(scale(390), scale(60));
    m_nicknameEdit->setStyleSheet(StyleAuthorizationWidget::glassLineEditStyle());
    m_nicknameEdit->setPlaceholderText("Only letters, numbers and _");
    m_nicknameEdit->setMaxLength(scale(20));
    m_nicknameEdit->setMinimumHeight(scale(40));
    m_nicknameEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    QFont segoeFont("Segoe UI", scale(12), QFont::Medium);
    m_nicknameEdit->setFont(segoeFont);
    m_nicknameEdit->setAttribute(Qt::WA_InputMethodEnabled, true);
    m_nicknameEdit->setFocus();

    QRegularExpressionValidator* validator = new QRegularExpressionValidator(
        QRegularExpression("[\\p{L}0-9_]{3,15}"), this);
    m_nicknameEdit->setValidator(validator);

    m_authorizeButton = new QPushButton("Authorize", m_container);
    m_authorizeButton->setFixedSize(scale(390), scale(60));
    m_authorizeButton->setStyleSheet(StyleAuthorizationWidget::glassButtonStyle());
    m_authorizeButton->setCursor(Qt::PointingHandCursor);
    m_authorizeButton->setMinimumHeight(scale(45));
    m_authorizeButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    QFont authorizeButtonFont("Outfit", scale(14), QFont::Bold);
    m_authorizeButton->setFont(authorizeButtonFont);

    m_glassLayout->addWidget(m_titleLabel);
    m_glassLayout->addWidget(m_subtitleLabel);
    m_glassLayout->addSpacing(scale(16));
    m_glassLayout->addWidget(m_errorLabel);
    m_glassLayout->addWidget(m_nicknameEdit);
    m_glassLayout->addWidget(m_authorizeButton);

    m_mainLayout->addSpacing(scale(42));
    m_mainLayout->addWidget(m_notificationWidget, 0, Qt::AlignTop | Qt::AlignHCenter);
    m_mainLayout->addWidget(m_updateAvailableWidget, 0, Qt::AlignTop | Qt::AlignHCenter);
    m_mainLayout->addWidget(m_container);

    connect(m_authorizeButton, &QPushButton::clicked, this, &AuthorizationWidget::onAuthorizationClicked);
    connect(m_nicknameEdit, &QLineEdit::textChanged, this, &AuthorizationWidget::onTextChanged);
    connect(m_nicknameEdit, &QLineEdit::returnPressed, this, &AuthorizationWidget::onAuthorizationClicked);
    connect(m_updateAvailableButton, &QPushButton::clicked, [this]() {emit updateButtonClicked(); });

    QShortcut* enterShortcut = new QShortcut(QKeySequence(Qt::Key_Return), this);
    QShortcut* returnShortcut = new QShortcut(QKeySequence(Qt::Key_Enter), this);

    connect(enterShortcut, &QShortcut::activated, this, &AuthorizationWidget::onAuthorizationClicked);
    connect(returnShortcut, &QShortcut::activated, this, &AuthorizationWidget::onAuthorizationClicked);
}

void AuthorizationWidget::setupAnimations() {
    m_backgroundBlurEffect = new QGraphicsBlurEffect(this);
    m_backgroundBlurEffect->setBlurRadius(0);

    m_blurAnimation = new QPropertyAnimation(m_backgroundBlurEffect, "blurRadius", this);
    m_blurAnimation->setDuration(1200);
    m_blurAnimation->setEasingCurve(QEasingCurve::OutCubic);
}

void AuthorizationWidget::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QLinearGradient gradient(0, 90, width(), height());
    gradient.setColorAt(0.0, QColor(230, 230, 230));
    gradient.setColorAt(0.5, QColor(220, 230, 240));
    gradient.setColorAt(1.0, QColor(240, 240, 240));

    painter.fillRect(rect(), gradient);

    QWidget::paintEvent(event);
}

bool AuthorizationWidget::validateNickname(const QString& nickname) {
    if (nickname.isEmpty()) {
        return false;
    }
    QRegularExpression regex("[\\p{L}0-9_]{3,15}");
    return regex.match(nickname).hasMatch();
}

void AuthorizationWidget::startBlurAnimation() {
    setGraphicsEffect(m_backgroundBlurEffect);
    m_blurAnimation->setStartValue(0);
    m_blurAnimation->setEndValue(10);
    m_blurAnimation->start();
}

void AuthorizationWidget::waitForBlurAnimation() {
    if (!m_blurAnimation) return;

    while (m_blurAnimation->state() == QAbstractAnimation::Running) {
        QApplication::processEvents(QEventLoop::WaitForMoreEvents, 10);
    }
}

void AuthorizationWidget::resetBlur() {
    m_backgroundBlurEffect->setBlurRadius(0);
}

void AuthorizationWidget::stopBlurAnimation() {
    if (m_blurAnimation) {
        m_blurAnimation->stop();
    }
    resetBlur();
}

void AuthorizationWidget::onAuthorizationClicked() {
    QString nickname = m_nicknameEdit->text().trimmed();

    if (nickname.isEmpty()) {
        setErrorMessage("field cannot be empty");
        return;
    }
    
    clearErrorMessage();

    emit authorizationButtonClicked(nickname);
}

void AuthorizationWidget::onTextChanged(const QString& text) {
    clearErrorMessage();
}

void AuthorizationWidget::setErrorMessage(const QString& errorText, int durationMs) {
    m_errorLabel->setText(errorText);
    m_errorLabel->show();
    m_nicknameEdit->setFocus();

    QTimer::singleShot(durationMs, this, &AuthorizationWidget::clearErrorMessage);
}

void AuthorizationWidget::setAuthorizationDisabled(bool disabled) {
    m_nicknameEdit->setDisabled(disabled);
    m_authorizeButton->setDisabled(disabled);
}

void AuthorizationWidget::clearErrorMessage() {
    m_errorLabel->hide();
}

void AuthorizationWidget::showNetworkErrorNotification() {
    if (m_updateAvailableWidget->isVisible()) {
        return;
    }

    m_notificationWidget->setStyleSheet(StyleAuthorizationWidget::notificationRedLabelStyle());

    m_notificationLabel->setText("Network error occurred, reconnecting...");
    m_notificationLabel->setStyleSheet(StyleAuthorizationWidget::notificationRedTextStyle());

    m_notificationWidget->show();
}

void AuthorizationWidget::hideNetworkErrorNotification() {
    m_notificationLabel->setText("");
    m_notificationWidget->hide();
}

void AuthorizationWidget::showUpdatesCheckingNotification()
{
    if (m_updateAvailableWidget->isVisible()) {
        return;
    }

    m_notificationWidget->setStyleSheet(StyleAuthorizationWidget::notificationLilacLabelStyle());

    m_notificationLabel->setText("Checking for updates...");
    m_notificationLabel->setStyleSheet(StyleAuthorizationWidget::notificationLilacTextStyle());
    m_notificationWidget->show();
}

void AuthorizationWidget::hideUpdatesCheckingNotification()
{
    m_notificationLabel->setText("");
    m_notificationWidget->hide();
}

void AuthorizationWidget::showUpdateAvailableNotification() {
    hideNetworkErrorNotification();
    hideUpdatesCheckingNotification();

    m_updateAvailableWidget->show();
}

void AuthorizationWidget::hideUpdateAvailableNotification() {
    m_updateAvailableWidget->hide();
}

void AuthorizationWidget::showConnectionRestoredNotification(int durationMs) {
    if (m_updateAvailableWidget->isVisible()) {
        return;
    }

    m_notificationWidget->setStyleSheet(StyleAuthorizationWidget::notificationGreenLabelStyle());
    m_notificationLabel->setStyleSheet(StyleAuthorizationWidget::notificationGreenTextStyle());
    m_notificationLabel->setText("Connection restored");

    m_notificationWidget->show();
    m_notificationTimer->start(durationMs);
}