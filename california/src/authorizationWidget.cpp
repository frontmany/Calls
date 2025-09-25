#include "AuthorizationWidget.h"
#include <QResizeEvent>
#include <QRegularExpressionValidator>
#include <QPainterPath>
#include <QApplication>
#include <QFontDatabase>

#include "calls.h"

// AuthorizationWidget implementation
AuthorizationWidget::AuthorizationWidget(QWidget* parent) : QWidget(parent) 
{
    setupUI();
    setupAnimations();
}

void AuthorizationWidget::setupUI() {
    // Main layout
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setAlignment(Qt::AlignCenter);

    // Create container
    m_container = new QWidget(this);
    m_container->setFixedSize(450, 400);
    m_container->setAttribute(Qt::WA_TranslucentBackground);

    // Container layout
    m_glassLayout = new QVBoxLayout(m_container);
    m_glassLayout->setSpacing(15);
    m_glassLayout->setContentsMargins(30, 30, 30, 30);
    m_glassLayout->setAlignment(Qt::AlignTop);

    // Title label
    m_titleLabel = new QLabel("Welcome to Callifornia", m_container);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_titleLabel->setStyleSheet(StyleAuthorizationWidget::glassTitleLabelStyle());
    m_titleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    m_titleLabel->setMinimumHeight(62);
    QFont titleFont("Pacifico", 28);
    m_titleLabel->setFont(titleFont);

    // SubTitle label
    m_subtitleLabel = new QLabel("Sign in to start making calls", m_container);
    m_subtitleLabel->setAlignment(Qt::AlignCenter);
    m_subtitleLabel->setStyleSheet(StyleAuthorizationWidget::glassSubTitleLabelStyle());
    m_subtitleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    QFont subTitleFont("Outfit", 13, QFont::ExtraLight);
    m_subtitleLabel->setFont(subTitleFont);

    // Nickname label
    m_errorLabel = new QLabel("field cannot be empty", m_container);
    m_errorLabel->setStyleSheet(StyleAuthorizationWidget::glassErrorLabelStyle());
    m_errorLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    m_errorLabel->hide();
    QFont errorLabelFont("Outfit", 10, QFont::ExtraLight);
    m_errorLabel->setFont(errorLabelFont);

    // Nickname input
    m_nicknameEdit = new QLineEdit(m_container);
    m_nicknameEdit->setStyleSheet(StyleAuthorizationWidget::glassLineEditStyle());
    m_nicknameEdit->setPlaceholderText("Only letters, numbers and _");
    m_nicknameEdit->setMaxLength(20);
    m_nicknameEdit->setMinimumHeight(40);
    m_nicknameEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    m_nicknameEdit->setFont(subTitleFont);

    // Set validator for nickname
    QRegularExpressionValidator* validator = new QRegularExpressionValidator(
        QRegularExpression("^[a-zA-Z0-9_]+$"), this);
    m_nicknameEdit->setValidator(validator);

    // Authorize button
    m_authorizeButton = new QPushButton("Authorize", m_container);
    m_authorizeButton->setStyleSheet(
        StyleAuthorizationWidget::glassButtonStyle() +
        StyleAuthorizationWidget::glassButtonHoverStyle()
    );
    m_authorizeButton->setCursor(Qt::PointingHandCursor);
    m_authorizeButton->setMinimumHeight(45);
    m_authorizeButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

    // Add widgets to glass layout
    m_glassLayout->addWidget(m_titleLabel);
    m_glassLayout->addWidget(m_subtitleLabel);
    m_glassLayout->addSpacing(16);
    m_glassLayout->addWidget(m_errorLabel);
    m_glassLayout->addWidget(m_nicknameEdit);
    m_glassLayout->addWidget(m_authorizeButton);

    // Add glass panel to main layout
    m_mainLayout->addSpacing(42);
    m_mainLayout->addWidget(m_container);

    // Connect signals
    connect(m_authorizeButton, &QPushButton::clicked,
        this, &AuthorizationWidget::onAuthorizeClicked);
    connect(m_nicknameEdit, &QLineEdit::textChanged,
        this, &AuthorizationWidget::onTextChanged);
}

void AuthorizationWidget::reset() {
    m_backgroundBlurEffect->setBlurRadius(0);
    m_nicknameEdit->setDisabled(false);
    m_authorizeButton->setDisabled(false);
    clearErrorMessage();
}

void AuthorizationWidget::setupAnimations() {
    // Background blur effect
    m_backgroundBlurEffect = new QGraphicsBlurEffect(this);
    m_backgroundBlurEffect->setBlurRadius(0);

    // Blur animation
    m_blurAnimation = new QPropertyAnimation(m_backgroundBlurEffect, "blurRadius", this);
    m_blurAnimation->setDuration(1200);
    m_blurAnimation->setEasingCurve(QEasingCurve::OutCubic);
}

void AuthorizationWidget::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QLinearGradient gradient(0, 90, width(), height());
    gradient.setColorAt(0.0, QColor(230, 230, 230));  // Темнее серый
    gradient.setColorAt(0.5, QColor(220, 230, 240));  // Темнее голубовато-серый  
    gradient.setColorAt(1.0, QColor(240, 240, 240));  // Темнее белый

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

void AuthorizationWidget::onAuthorizeClicked() {
    QString nickname = m_nicknameEdit->text().trimmed();

    if (validateNickname(nickname)) {
        // Success animation with blur
        setGraphicsEffect(m_backgroundBlurEffect);
        m_blurAnimation->setStartValue(0);
        m_blurAnimation->setEndValue(10);
        m_blurAnimation->start();

        m_nicknameEdit->setDisabled(true);
        m_authorizeButton->setDisabled(true);

        clearErrorMessage();

        
        calls::authorize(nickname.toStdString());
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

    // Shake animation
    QPropertyAnimation* shakeAnimation = new QPropertyAnimation(m_errorLabel, "pos", this);
    shakeAnimation->setDuration(100);
    shakeAnimation->setLoopCount(3);
    shakeAnimation->setKeyValueAt(0, m_errorLabel->pos());
    shakeAnimation->setKeyValueAt(0.25, m_errorLabel->pos() + QPoint(8, 0));
    shakeAnimation->setKeyValueAt(0.5, m_errorLabel->pos() + QPoint(-8, 0));
    shakeAnimation->setKeyValueAt(0.75, m_errorLabel->pos() + QPoint(8, 0));
    shakeAnimation->setKeyValueAt(1, m_errorLabel->pos());
    shakeAnimation->start(QAbstractAnimation::DeleteWhenStopped);
}

void AuthorizationWidget::clearErrorMessage() {
    m_nicknameEdit->setStyleSheet(StyleAuthorizationWidget::glassLineEditStyle());
    m_errorLabel->hide();
}

// Style definitions
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
        "   border-radius: 15px;"
        "   padding: 12px 24px;"
        "   font-size: 14px;"
        "   font-weight: bold;"
        "   margin: 0px;"
        "}").arg(m_primaryColor.name()).arg(m_glassBorderColor.name()).arg(m_glassBorderColor.name());
}

QString StyleAuthorizationWidget::glassButtonHoverStyle() {
    return QString("QPushButton:hover {"
        "   background-color: %1;"
        "}").arg(m_hoverColor.name());
}

QString StyleAuthorizationWidget::glassButtonDisabledStyle() {
    return QString("QPushButton:disabled {"
        "   background-color: %1;"
        "}").arg(m_disabledColor.name());
}

QString StyleAuthorizationWidget::glassLineEditStyle() {
    return QString("QLineEdit {"
        "   background-color: rgba(245, 245, 245, 235);"
        "   border: 0px solid %1;"
        "   border-radius: 12px;"
        "   padding: 12px 15px;"
        "   margin: 0px;"
        "   color: %2;"
        "   selection-background-color: %3;"
        "}"
        "QLineEdit:focus {"
        "   border: 0px solid %3;"
        "   background-color: rgba(255, 255, 255, 235);"
        "}"
        "QLineEdit::placeholder {"
        "   color: rgba(240, 240, 240, 180);"
        "}").arg(m_glassBorderColor.name()).arg(m_textColor.name()).arg(m_primaryColor.name());
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