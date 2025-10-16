#include "AuthorizationWidget.h"
#include <QResizeEvent>
#include <QRegularExpressionValidator>
#include <QPainterPath>
#include <QApplication>
#include <QFontDatabase>
#include "scaleFactor.h"

// AuthorizationWidget implementation
AuthorizationWidget::AuthorizationWidget(QWidget* parent) : QWidget(parent)
{
    setupUI();
    setupAnimations();

    m_notificationTimer = new QTimer(this);
    m_notificationTimer->setSingleShot(true);
    connect(m_notificationTimer, &QTimer::timeout, [this]() {m_notificationWidget->hide(); });
}

void AuthorizationWidget::setupUI() {
    // Main layout
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setAlignment(Qt::AlignCenter);

    // Create network error widget
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

    // Create container
    m_container = new QWidget(this);
    m_container->setFixedSize(scale(450), scale(400));
    m_container->setAttribute(Qt::WA_TranslucentBackground);

    // Container layout
    m_glassLayout = new QVBoxLayout(m_container);
    m_glassLayout->setSpacing(scale(15));
    m_glassLayout->setContentsMargins(scale(30), scale(30), scale(30), scale(30));
    m_glassLayout->setAlignment(Qt::AlignTop);

    // Title label
    m_titleLabel = new QLabel("Welcome to Callifornia", m_container);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_titleLabel->setStyleSheet(StyleAuthorizationWidget::glassTitleLabelStyle());
    m_titleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    m_titleLabel->setMinimumHeight(scale(62));
    QFont titleFont("Pacifico", scale(28));
    m_titleLabel->setFont(titleFont);

    // SubTitle label
    m_subtitleLabel = new QLabel("Enter your nickname to start making calls", m_container);
    m_subtitleLabel->setAlignment(Qt::AlignCenter);
    m_subtitleLabel->setStyleSheet(StyleAuthorizationWidget::glassSubTitleLabelStyle());
    m_subtitleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    QFont subTitleFont("Outfit", scale(13), QFont::ExtraLight);
    m_subtitleLabel->setFont(subTitleFont);

    // Nickname label
    m_errorLabel = new QLabel("field cannot be empty", m_container);
    m_errorLabel->setStyleSheet(StyleAuthorizationWidget::glassErrorLabelStyle());
    m_errorLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    m_errorLabel->hide();
    QFont errorLabelFont("Outfit", scale(10), QFont::ExtraLight);
    m_errorLabel->setFont(errorLabelFont);

    // Nickname input
    m_nicknameEdit = new QLineEdit(m_container);
    m_nicknameEdit->setFixedSize(scale(390), scale(60));
    m_nicknameEdit->setStyleSheet(StyleAuthorizationWidget::glassLineEditStyle());
    m_nicknameEdit->setPlaceholderText("Only letters, numbers and _");
    m_nicknameEdit->setMaxLength(scale(20));
    m_nicknameEdit->setMinimumHeight(scale(40));
    m_nicknameEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    m_nicknameEdit->setFont(subTitleFont);

    // Set validator for nickname
    QRegularExpressionValidator* validator = new QRegularExpressionValidator(
        QRegularExpression("^[a-zA-Z0-9_]+$"), this);
    m_nicknameEdit->setValidator(validator);

    // Authorize button
    m_authorizeButton = new QPushButton("Authorize", m_container);
    m_authorizeButton->setFixedSize(scale(390), scale(60));
    m_authorizeButton->setStyleSheet(StyleAuthorizationWidget::glassButtonStyle());
    m_authorizeButton->setCursor(Qt::PointingHandCursor);
    m_authorizeButton->setMinimumHeight(scale(45));
    m_authorizeButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    QFont authorizeButtonFont("Outfit", scale(14), QFont::Bold);
    m_authorizeButton->setFont(authorizeButtonFont);

    // Add widgets to glass layout
    m_glassLayout->addWidget(m_titleLabel);
    m_glassLayout->addWidget(m_subtitleLabel);
    m_glassLayout->addSpacing(scale(16));
    m_glassLayout->addWidget(m_errorLabel);
    m_glassLayout->addWidget(m_nicknameEdit);
    m_glassLayout->addWidget(m_authorizeButton);

    // Add glass panel to main layout
    m_mainLayout->addSpacing(scale(42));
    m_mainLayout->addWidget(m_notificationWidget, 0, Qt::AlignTop | Qt::AlignHCenter);
    m_mainLayout->addWidget(m_container);

    // Connect signals
    connect(m_authorizeButton, &QPushButton::clicked, this, &AuthorizationWidget::onAuthorizationClicked);
    connect(m_nicknameEdit, &QLineEdit::textChanged, this, &AuthorizationWidget::onTextChanged);
    connect(m_nicknameEdit, &QLineEdit::returnPressed, this, &AuthorizationWidget::onAuthorizationClicked);
}

void AuthorizationWidget::resetBlur() {
    m_backgroundBlurEffect->setBlurRadius(0);
    m_nicknameEdit->setDisabled(false);
    m_authorizeButton->setDisabled(false);
}

void AuthorizationWidget::setupAnimations() {
    // Background blur effect
    m_backgroundBlurEffect = new QGraphicsBlurEffect(this);
    m_backgroundBlurEffect->setBlurRadius(0);

    // Blur animation
    m_blurAnimation = new QPropertyAnimation(m_backgroundBlurEffect, "blurRadius", this);
    m_blurAnimation->setDuration(1200);
    m_blurAnimation->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_blurAnimation, &QPropertyAnimation::finished, [this]() {emit blurAnimationFinished(); });
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
    QRegularExpression regex("^[a-zA-Z0-9_]+$");
    return regex.match(nickname).hasMatch();
}

void AuthorizationWidget::startBlurAnimation() {
    // Success animation with blur
    setGraphicsEffect(m_backgroundBlurEffect);
    m_blurAnimation->setStartValue(0);
    m_blurAnimation->setEndValue(10);
    m_blurAnimation->start();
}

void AuthorizationWidget::onAuthorizationClicked() {
    QString nickname = m_nicknameEdit->text().trimmed();

    if (validateNickname(nickname)) {
        m_nicknameEdit->setDisabled(true);
        m_authorizeButton->setDisabled(true);
        clearErrorMessage();
        emit authorizationButtonClicked(nickname);
    }
    else {
        setErrorMessage("field cannot be empty");
    }
}

void AuthorizationWidget::onTextChanged(const QString& text) {
    if (validateNickname(text)) {
        clearErrorMessage();
    }
}

void AuthorizationWidget::setErrorMessage(const QString& errorText) {
    m_errorLabel->setText(errorText);
    m_errorLabel->show();
    m_nicknameEdit->setFocus();
}

void AuthorizationWidget::clearErrorMessage() {
    m_nicknameEdit->setStyleSheet(StyleAuthorizationWidget::glassLineEditStyle());
    m_errorLabel->hide();
}

void AuthorizationWidget::showNetworkErrorNotification() {
    m_notificationWidget->setStyleSheet(StyleAuthorizationWidget::notificationRedLabelStyle());

    m_notificationLabel->setText("Network error occurred");
    m_notificationLabel->setStyleSheet("color: #DC5050; background: transparent; font-size: 14px; margin: 0px; padding: 0px;");

    m_notificationWidget->show();
}

void AuthorizationWidget::hideNetworkErrorNotification() {
    m_notificationLabel->setText("");
    m_notificationWidget->hide();
}

void AuthorizationWidget::showConnectionRestoredNotification(int durationMs) {
    m_notificationWidget->setStyleSheet(StyleAuthorizationWidget::notificationGreenLabelStyle());

    m_notificationLabel->setText("Connection restored");
    m_notificationLabel->setStyleSheet("color: #19ba00; background: transparent; font-size: 14px; margin: 0px; padding: 0px;"); 

    m_notificationWidget->show();
    m_notificationTimer->start(durationMs);
}

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