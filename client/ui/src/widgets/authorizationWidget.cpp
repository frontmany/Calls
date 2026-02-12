#include "authorizationWidget.h"
#include "constants/constant.h"
#include "constants/color.h"

#include <QRegularExpressionValidator>
#include <QShortcut>

#include "utilities/utility.h"

const QColor StyleAuthorizationWidget::m_primaryColor = COLOR_ACCENT;
const QColor StyleAuthorizationWidget::m_hoverColor = COLOR_ACCENT_HOVER;
const QColor StyleAuthorizationWidget::m_errorColor = COLOR_OVERLAY_ERROR_200;
const QColor StyleAuthorizationWidget::m_successColor = COLOR_STATUS_SUCCESS;
const QColor StyleAuthorizationWidget::m_textColor = COLOR_TEXT_MAIN;
const QColor StyleAuthorizationWidget::m_backgroundColor = COLOR_NEUTRAL_100;
const QColor StyleAuthorizationWidget::m_glassColor = COLOR_OVERLAY_PURE_60;
const QColor StyleAuthorizationWidget::m_glassBorderColor = COLOR_OVERLAY_PURE_100;
const QColor StyleAuthorizationWidget::m_textDarkColor = COLOR_NEUTRAL_150;
const QColor StyleAuthorizationWidget::m_disabledColor = COLOR_OVERLAY_DISABLED_150;

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
        "   background-color: %8;"
        "   opacity: 0.6;"
        "}")
        .arg(m_primaryColor.name())                    // %1 - background-color
        .arg(COLOR_BACKGROUND_PURE.name())                     // %2 - color (белый текст)
        .arg(m_glassBorderColor.name())               // %3 - border color
        .arg(scale(15))                                // %4 - border-radius
        .arg(scale(12))                                // %5 - padding vertical
        .arg(scale(24))                                // %6 - padding horizontal
        .arg(m_hoverColor.name())                      // %7 - hover background-color
        .arg(COLOR_OVERLAY_ACCENT_150.name());          // %8 - disabled background-color
}

QString StyleAuthorizationWidget::glassLineEditStyle() {
    return QString("QLineEdit {"
        "   background-color: %7;"
        "   border: 0px solid %1;"
        "   border-radius: %4px;"
        "   padding: %5px %6px;"
        "   margin: 0px;"
        "   color: %2;"
        "   selection-background-color: %3;"
        "}"
        "QLineEdit:focus {"
        "   border: 0px solid %3;"
        "   background-color: %8;"
        "}"
        "QLineEdit:disabled {"
        "   background-color: %9;"
        "   border: 0px solid %1;"
        "   opacity: 0.7;"
        "}"
        "QLineEdit::placeholder {"
        "   color: %10;"
        "}").arg(m_glassBorderColor.name())
        .arg(m_textColor.name())
        .arg(m_primaryColor.name())
        .arg(QString::fromStdString(std::to_string(scale(12))))
        .arg(scale(12))
        .arg(scale(15))
        .arg(COLOR_OVERLAY_NEUTRAL_235.name())
        .arg(COLOR_OVERLAY_PURE_235.name())
        .arg(COLOR_OVERLAY_NEUTRAL_150.name())
        .arg(COLOR_INPUT_PLACEHOLDER.name());
}

QString StyleAuthorizationWidget::glassLabelStyle() {
    return QString("QLabel {"
        "   color: %1;"
        "   font-size: %2px;"
        "   font-weight: bold;"
        "   margin: 0px;"
        "   background: transparent;"
        "}")
        .arg(m_textColor.name())
        .arg(scale(14));
}

QString StyleAuthorizationWidget::glassErrorLabelStyle() {
    return QString("QLabel {"
        "   color: %1;"
        "   margin: %2px %2px;"
        "   padding: %3px;"
        "   background: transparent;"
        "   border-radius: %3px;"
        "}")
        .arg(m_errorColor.name())
        .arg(scale(2))
        .arg(scale(5));
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
        "   background-color: %2;"
        "   border: none;"
        "   border-radius: %1px;"
        "   margin: 0px;"
        "   padding: 0px;"
        "}")
        .arg(scale(8))
        .arg(QColor(220, 80, 80, 65).name());
}

QString StyleAuthorizationWidget::notificationGreenLabelStyle() {
    return QString("QWidget {"
        "   background-color: %1;"
        "   border: none;"
        "   border-radius: %2px;"
        "   margin: 0px;"
        "   padding: 0px;"
        "}")
        .arg(QColor(82, 196, 65, 100).name())
        .arg(scale(8));
}

QString StyleAuthorizationWidget::notificationLilacLabelStyle() {
    return QString("QWidget {"
        "   background-color: %1;"  
        "   border: none;"
        "   border-radius: %2px;"
        "   margin: 0px;"
        "   padding: 0px;"
        "}")
        .arg(QColor(200, 180, 220, 80).name())
        .arg(scale(8));
}

QString StyleAuthorizationWidget::notificationRedTextStyle() {
    return QString("QLabel {"
        "   color: %1;"
        "   background: transparent;"
        "   font-size: %2px;"
        "   margin: 0px;"
        "   padding: 0px;"
        "}")
        .arg(COLOR_STATUS_ERROR.name())
        .arg(scale(14));
}

QString StyleAuthorizationWidget::notificationLilacTextStyle() {
    return QString("QLabel {"
        "   color: %1;"
        "   background: transparent;"
        "   font-size: %2px;"
        "   margin: 0px;"
        "   padding: 0px;"
        "}")
        .arg(COLOR_ACCENT_PURPLE.name())
        .arg(scale(14));
}

QString StyleAuthorizationWidget::notificationGreenTextStyle() {
    return QString("QLabel {"
        "   color: %1;"
        "   background: transparent;"
        "   font-size: %2px;"
        "   margin: 0px;"
        "   padding: 0px;"
        "}")
        .arg(COLOR_STATUS_SUCCESS.name())
        .arg(scale(14));
}

AuthorizationWidget::AuthorizationWidget(QWidget* parent) : QWidget(parent)
{
    setupUI();
    setupAnimations();
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
        QRegularExpression(QString("[\\p{L}0-9_]{%1,%2}").arg(MIN_NICKNAME_LENGTH).arg(MAX_NICKNAME_LENGTH)), this);
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
    m_mainLayout->addWidget(m_container);

    connect(m_authorizeButton, &QPushButton::clicked, this, &AuthorizationWidget::onAuthorizationClicked);
    connect(m_nicknameEdit, &QLineEdit::textChanged, this, &AuthorizationWidget::onTextChanged);
    connect(m_nicknameEdit, &QLineEdit::returnPressed, this, &AuthorizationWidget::onAuthorizationClicked);

    QShortcut* enterShortcut = new QShortcut(QKeySequence(Qt::Key_Return), this);
    QShortcut* returnShortcut = new QShortcut(QKeySequence(Qt::Key_Enter), this);

    connect(enterShortcut, &QShortcut::activated, this, &AuthorizationWidget::onAuthorizationClicked);
    connect(returnShortcut, &QShortcut::activated, this, &AuthorizationWidget::onAuthorizationClicked);
}

void AuthorizationWidget::setupAnimations() {
    m_backgroundBlurEffect = new QGraphicsBlurEffect(this);
    m_backgroundBlurEffect->setBlurRadius(0);

    m_blurAnimation = new QPropertyAnimation(m_backgroundBlurEffect, "blurRadius", this);
    m_blurAnimation->setDuration(BLUR_ANIMATION_DURATION_MS);
    m_blurAnimation->setEasingCurve(QEasingCurve::OutCubic);
}

void AuthorizationWidget::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QLinearGradient gradient(0, 90, width(), height());
    gradient.setColorAt(0.0, COLOR_GRADIENT_START);
    gradient.setColorAt(0.5, COLOR_GRADIENT_MIDDLE);
    gradient.setColorAt(1.0, COLOR_GRADIENT_END);

    painter.fillRect(rect(), gradient);

    QWidget::paintEvent(event);
}

bool AuthorizationWidget::validateNickname(const QString& nickname) {
    if (nickname.isEmpty()) {
        return false;
    }
    QRegularExpression regex(QString("[\\p{L}0-9_]{%1,%2}").arg(MIN_NICKNAME_LENGTH).arg(MAX_NICKNAME_LENGTH));
    return regex.match(nickname).hasMatch();
}

void AuthorizationWidget::startBlurAnimation() {
    setGraphicsEffect(m_backgroundBlurEffect);
    m_blurAnimation->setStartValue(0);
    m_blurAnimation->setEndValue(BLUR_ANIMATION_END_VALUE);
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
