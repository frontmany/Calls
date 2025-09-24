#include "MainMenuWidget.h"
#include <QResizeEvent>
#include <QPainter>
#include <QPainterPath>
#include <QFontDatabase>
#include <QApplication>
#include <QGraphicsDropShadowEffect>
#include <QRegularExpressionValidator>

// Style definitions
const QColor StyleMainMenuWidget::m_primaryColor = QColor(21, 119, 232);
const QColor StyleMainMenuWidget::m_hoverColor = QColor(18, 113, 222);
const QColor StyleMainMenuWidget::m_backgroundColor = QColor(230, 230, 230);
const QColor StyleMainMenuWidget::m_textColor = QColor(1, 11, 19);
const QColor StyleMainMenuWidget::m_onlineColor = QColor(100, 200, 100);
const QColor StyleMainMenuWidget::m_offlineColor = QColor(150, 150, 150);

QString StyleMainMenuWidget::containerStyle() {
    return QString("QWidget {"
        "   background-color: transparent;"
        "   border-radius: 20px;"
        "   padding: 0px;"
        "}");
}

QString StyleMainMenuWidget::titleStyle() {
    return QString("QLabel {"
        "   color: %1;"
        "   margin: 0px;"
        "   padding: 0px;"
        "}").arg(m_textColor.name());
}

QString StyleMainMenuWidget::nicknameStyle() {
    return QString("QLabel {"
        "   color: %1;"
        "   margin: 0px;"
        "   padding: 0px;"
        "}").arg(m_textColor.name());
}

QString StyleMainMenuWidget::buttonStyle() {
    return QString("QPushButton {"
        "   background-color: %1;"
        "   color: white;"
        "   border: none;"
        "   border-radius: 15px;"
        "   padding: 12px 24px;"
        "   margin: 0px;"
        "}"
        "QPushButton:hover {"
        "   background-color: %2;"
        "}").arg(m_primaryColor.name()).arg(m_hoverColor.name());
}

QString StyleMainMenuWidget::settingsButtonStyle() {
    return QString("QPushButton {"
        "   background-color: rgba(235, 235, 235, 190);"
        "   color: white;"
        "   border: none;"
        "   border-radius: 15px;"
        "   padding: 12px 40px 12px 15px;"
        "   font-size: 14px;"
        "   font-weight: bold;"
        "   margin: 0px;"
        "}"
        "QPushButton:hover {"
        "   background-color:rgba(235, 235, 235, 110);"
        "}"
        "QPushButton::menu-indicator {"
        "   image: none;"
        "   width: 0px;"
        "}");
}

QString StyleMainMenuWidget::lineEditStyle() {
    return QString("QLineEdit {"
        "   background-color: rgba(245, 245, 245, 190);"
        "   border: 0px solid %1;"
        "   border-radius: 15px;"
        "   padding: 12px 15px;"
        "   margin: 0px;"
        "   color: %2;"
        "   selection-background-color: %3;"
        "}"
        "QLineEdit:focus {"
        "   border: 0px solid %3;"
        "   background-color: rgba(255, 255, 255, 190);"
        "}"
        "QLineEdit::placeholder {"
        "   color: rgba(240, 240, 240, 180);"
        "}").arg(m_textColor.name()).arg(m_textColor.name()).arg(m_textColor.name());
}

QString StyleMainMenuWidget::avatarStyle(const QColor& color) {
    return QString("QLabel {"
        "   background-color: %1;"
        "   border-radius: 25px;"
        "   color: white;"
        "   font-size: 18px;"
        "   font-weight: bold;"
        "   margin: 0px;"
        "   padding: 0px;"
        "}").arg(color.name());
}

QString StyleMainMenuWidget::incomingCallWidgetStyle() {
    return QString("QWidget {"
        "   background-color: rgba(245, 245, 245, 200);"
        "   border-radius: 10px;"
        "   margin: 2px;"
        "}");
}

QString StyleMainMenuWidget::settingsPanelStyle() {
    return QString("QWidget {"
        "   background-color: rgba(240, 240, 240, 200);"
        "   border-radius: 10px;"
        "   padding: 15px;"
        "}");
}

QString StyleMainMenuWidget::scrollAreaStyle() {
    return QString(
        "QScrollArea {"
        "   border: none;"
        "   background: transparent;"
        "}"
        "QScrollBar:vertical {"
        "   background-color: transparent;"
        "   width: 6px;"
        "   margin: 0px;"
        "   border-radius: 3px;"
        "}"
        "QScrollBar::handle:vertical {"
        "   background-color: #D0D0D0;"
        "   border-radius: 3px;"
        "   min-height: 20px;"
        "}"
        "QScrollBar::handle:vertical:hover {"
        "   background-color: #B0B0B0;"
        "}"
        "QScrollBar::handle:vertical:pressed {"
        "   background-color: #909090;"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
        "   border: none;"
        "   background: none;"
        "   height: 0px;"
        "}"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {"
        "   background: none;"
        "}"
    );
}

MainMenuWidget::MainMenuWidget(QWidget* parent) : QWidget(parent) {
    setupUI();
    setupAnimations();
}

void MainMenuWidget::setupUI() {
    m_backgroundTexture = QPixmap(":/resources/blur.png");

    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(40, 40, 40, 40);

    // Main container
    m_mainContainer = new QWidget(this);
    m_mainContainer->setFixedWidth(500);
    m_mainContainer->setStyleSheet(StyleMainMenuWidget::containerStyle());

    m_containerLayout = new QVBoxLayout(m_mainContainer);
    m_containerLayout->setSpacing(20);
    m_containerLayout->setContentsMargins(30, 30, 30, 30);

    // Title
    m_titleLabel = new QLabel("Callifornia", m_mainContainer);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_titleLabel->setStyleSheet(StyleMainMenuWidget::titleStyle());
    m_titleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    m_titleLabel->setMinimumHeight(70);
    QFont titleFont("Pacifico", 32);
    m_titleLabel->setFont(titleFont);

    // User info section
    m_userInfoWidget = new QWidget(m_mainContainer);
    m_userInfoLayout = new QHBoxLayout(m_userInfoWidget);
    m_userInfoLayout->setSpacing(15);
    m_userInfoLayout->setContentsMargins(0, 0, 0, 0);
    m_userInfoLayout->setAlignment(Qt::AlignCenter);

    m_avatarLabel = new QLabel("", m_userInfoWidget);
    m_avatarLabel->setFixedSize(50, 50);
    m_avatarLabel->setAlignment(Qt::AlignCenter);

    m_userTextLayout = new QVBoxLayout();
    m_nicknameLabel = new QLabel("Guest", m_userInfoWidget);
    QFont nicknameFont("Outfit", 18, QFont::Normal);
    m_nicknameLabel->setFont(nicknameFont);

    m_statusLabel = new QLabel("Offline", m_userInfoWidget);
    QFont statusFont("Outfit", 11, QFont::Light);
    m_statusLabel->setFont(statusFont);

    m_nicknameLabel->setStyleSheet(StyleMainMenuWidget::nicknameStyle());

    m_userTextLayout->addWidget(m_nicknameLabel);
    m_userTextLayout->addSpacing(-15);
    m_userTextLayout->addWidget(m_statusLabel);

    m_userInfoLayout->addWidget(m_avatarLabel);
    m_userInfoLayout->addLayout(m_userTextLayout);

    // Call section
    m_friendNicknameEdit = new QLineEdit(m_mainContainer);
    m_friendNicknameEdit->setPlaceholderText("Enter friend's nickname");
    m_friendNicknameEdit->setStyleSheet(StyleMainMenuWidget::lineEditStyle());
    QFont friendNicknameFont("Outfit", 12, QFont::Light);
    m_friendNicknameEdit->setFont(friendNicknameFont);

    // Set validator for nickname
    QRegularExpressionValidator* validator = new QRegularExpressionValidator(
        QRegularExpression("^[a-zA-Z0-9_]+$"), this);
    m_friendNicknameEdit->setValidator(validator);

    m_callButton = new QPushButton("Make Call", m_mainContainer);
    m_callButton->setStyleSheet(StyleMainMenuWidget::buttonStyle());
    m_callButton->setCursor(Qt::PointingHandCursor);
    QFont callButtonFont("Outfit", 14, QFont::Normal);
    m_callButton->setFont(callButtonFont);

    m_incomingCallsContainer = new QWidget();
    m_incomingCallsLayout = new QVBoxLayout(m_incomingCallsContainer);
    m_incomingCallsLayout->setAlignment(Qt::AlignTop);

    m_incomingCallsScrollArea = new QScrollArea(m_mainContainer);
    m_incomingCallsScrollArea->setWidget(m_incomingCallsContainer);
    m_incomingCallsScrollArea->setWidgetResizable(true);
    m_incomingCallsScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_incomingCallsScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_incomingCallsScrollArea->hide();
    m_incomingCallsScrollArea->setStyleSheet(StyleMainMenuWidget::scrollAreaStyle());

    // Settings button with icon
    QWidget* buttonWidget = new QWidget();
    QHBoxLayout* buttonLayout = new QHBoxLayout(buttonWidget);
    buttonLayout->setContentsMargins(15, 5, 15, 5);
    buttonLayout->setSpacing(10);

    QLabel* leftIcon = new QLabel();
    leftIcon->setPixmap(QPixmap(":/resources/speaker.png").scaled(20, 20, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    leftIcon->setStyleSheet("color: inherit; background: transparent;");

    QLabel* textLabel = new QLabel("Audio Controls");
    textLabel->setStyleSheet("color: inherit; background: transparent;");
    QFont textLabelFont("Outfit", 12, QFont::Light);
    textLabel->setFont(textLabelFont);

    QLabel* rightIcon = new QLabel();
    rightIcon->setPixmap(QPixmap(":/resources/arrowDown.png").scaled(16, 16, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    rightIcon->setStyleSheet("color: inherit; background: transparent;");

    buttonLayout->addWidget(leftIcon);
    buttonLayout->addWidget(textLabel);
    buttonLayout->addStretch();
    buttonLayout->addWidget(rightIcon);

    m_settingsButton = new QPushButton(m_mainContainer);
    m_settingsButton->setLayout(buttonLayout);
    m_settingsButton->setStyleSheet(StyleMainMenuWidget::settingsButtonStyle());
    m_settingsButton->setCursor(Qt::PointingHandCursor);

    // Settings panel (initially hidden)
    m_settingsPanel = new SettingsPanel(m_mainContainer);
    m_settingsPanel->hide();

    // Add widgets to layout
    m_containerLayout->addWidget(m_titleLabel);
    m_containerLayout->addSpacing(-4);
    m_containerLayout->addWidget(m_userInfoWidget);
    m_containerLayout->addSpacing(18);
    m_containerLayout->addWidget(m_friendNicknameEdit);
    m_containerLayout->addWidget(m_callButton);
    m_containerLayout->addWidget(m_incomingCallsScrollArea);
    m_containerLayout->addWidget(m_settingsButton);
    m_containerLayout->addWidget(m_settingsPanel);
    m_containerLayout->addSpacing(10);

    m_mainLayout->addWidget(m_mainContainer, 0, Qt::AlignCenter);

    // Connect signals
    connect(m_callButton, &QPushButton::clicked, this, &MainMenuWidget::onCallButtonClicked);
    connect(m_settingsButton, &QPushButton::clicked, this, &MainMenuWidget::onSettingsButtonClicked);
}

void MainMenuWidget::setupAnimations() {
    // Settings panel animation
    m_settingsAnimation = new QPropertyAnimation(m_settingsPanel, "maximumHeight", this);
    m_settingsAnimation->setDuration(200);
    m_settingsAnimation->setEasingCurve(QEasingCurve::InOutQuad);
    m_settingsAnimation->setStartValue(0);
    m_settingsAnimation->setEndValue(170);

    connect(m_settingsAnimation, &QPropertyAnimation::finished, this, [this]() {
        if (m_settingsAnimation->direction() == QAbstractAnimation::Backward) {
            m_settingsPanel->hide();
        }
        });

    // Incoming calls area animation
    m_incomingCallsAnimation = new QPropertyAnimation(m_incomingCallsScrollArea, "maximumHeight", this);
    m_incomingCallsAnimation->setDuration(250);
    m_incomingCallsAnimation->setEasingCurve(QEasingCurve::InOutQuad);
    m_incomingCallsAnimation->setStartValue(0);
    m_incomingCallsAnimation->setEndValue(150);

    connect(m_incomingCallsAnimation, &QPropertyAnimation::finished, this, [this]() {
        if (m_incomingCallsAnimation->direction() == QAbstractAnimation::Backward) {
            m_incomingCallsScrollArea->hide();
        }
        else {
            m_incomingCallsScrollArea->setMinimumHeight(100);
        }
        });
}

void MainMenuWidget::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Draw background gradient
    QLinearGradient gradient(0, 90, width(), height());
    gradient.setColorAt(0.0, QColor(230, 230, 230));
    gradient.setColorAt(0.5, QColor(220, 230, 240));
    gradient.setColorAt(1.0, QColor(240, 240, 240));
    painter.fillRect(rect(), gradient);

    // Draw the main container background with texture if loaded
    if (!m_backgroundTexture.isNull()) {
        QPainterPath path;
        QRect containerRect = m_mainContainer->geometry();
        path.addRoundedRect(containerRect, 20, 20);

        painter.setClipPath(path);
        painter.drawPixmap(containerRect, m_backgroundTexture);
        painter.setClipping(false);
    }

    QWidget::paintEvent(event);
}

void MainMenuWidget::setNickname(const QString& nickname) {
    m_currentNickname = nickname;
    m_nicknameLabel->setText(nickname);
    m_nicknameLabel->setStyleSheet(StyleMainMenuWidget::nicknameStyle());

    // Generate avatar
    QColor avatarColor = generateRandomColor(nickname);
    QString firstLetter = nickname.left(1).toUpper();

    m_avatarLabel->setText(firstLetter);
    m_avatarLabel->setStyleSheet(StyleMainMenuWidget::avatarStyle(avatarColor));
}

void MainMenuWidget::setStatus(calls::ClientStatus status) {
    if (status == calls::ClientStatus::FREE) {
        m_statusLabel->setText("Online");
        m_statusLabel->setStyleSheet(QString("QLabel { color: %1; }").arg(StyleMainMenuWidget::m_onlineColor.name()));
    }
    else if (status == calls::ClientStatus::CALLING) {
        // Implementation for calling status
    }
    else if (status == calls::ClientStatus::BUSY) {
        // Implementation for busy status
    }
    else if (status == calls::ClientStatus::UNAUTHORIZED) {
        // Implementation for unauthorized status
    }
}

QColor MainMenuWidget::generateRandomColor(const QString& seed) {
    int hash = qHash(seed);
    return QColor::fromHsv(hash % 360, 150 + hash % 106, 150 + hash % 106);
}

void MainMenuWidget::onCallButtonClicked() {
    QString friendNickname = m_friendNicknameEdit->text().trimmed();
    if (!friendNickname.isEmpty()) {
        emit callRequested(friendNickname);
    }
}

void MainMenuWidget::onSettingsButtonClicked() {
    if (m_settingsPanel->isHidden()) {
        m_settingsPanel->setMaximumHeight(0);
        m_settingsPanel->show();
        m_settingsAnimation->setDirection(QAbstractAnimation::Forward);
        m_settingsAnimation->start();
    }
    else {
        m_settingsAnimation->setDirection(QAbstractAnimation::Backward);
        m_settingsAnimation->start();
    }
}

void MainMenuWidget::addIncomingCall(const QString& friendNickname) {
    if (m_incomingCallsLayout->count() == 0) {
        showIncomingCallsArea();
    }

    IncomingCallWidget* callWidget = new IncomingCallWidget(friendNickname, this);
    connect(callWidget, &IncomingCallWidget::callAccepted,
        this, &MainMenuWidget::onIncomingCallAccepted);
    connect(callWidget, &IncomingCallWidget::callDeclined,
        this, &MainMenuWidget::onIncomingCallDeclined);

    m_incomingCallsLayout->addWidget(callWidget);
}

void MainMenuWidget::removeIncomingCall(const QString& friendNickname) {
    for (int i = 0; i < m_incomingCallsLayout->count(); ++i) {
        QWidget* widget = m_incomingCallsLayout->itemAt(i)->widget();
        if (IncomingCallWidget* callWidget = qobject_cast<IncomingCallWidget*>(widget)) {
            if (callWidget->getFriendNickname() == friendNickname) {
                m_incomingCallsLayout->removeWidget(widget);
                widget->deleteLater();
                break;
            }
        }
    }

    if (m_incomingCallsLayout->count() == 0) {
        hideIncomingCallsArea();
    }
}

void MainMenuWidget::clearIncomingCalls() {
    while (m_incomingCallsLayout->count() > 0) {
        QWidget* widget = m_incomingCallsLayout->itemAt(0)->widget();
        m_incomingCallsLayout->removeWidget(widget);
        widget->deleteLater();
        hideIncomingCallsArea();
    }
}

void MainMenuWidget::showIncomingCallsArea() {
    if (m_incomingCallsScrollArea->isHidden()) {
        m_incomingCallsScrollArea->show();
    }

    m_incomingCallsAnimation->setDirection(QAbstractAnimation::Forward);
    m_incomingCallsAnimation->start();
}

void MainMenuWidget::hideIncomingCallsArea() {
    if (!m_incomingCallsScrollArea->isHidden()) {
        m_incomingCallsAnimation->setDirection(QAbstractAnimation::Backward);
        m_incomingCallsScrollArea->setMinimumHeight(0);
        m_incomingCallsAnimation->start();
    }
}

void MainMenuWidget::onIncomingCallAccepted(const QString& friendNickname) {
    emit callAccepted(friendNickname);
}

void MainMenuWidget::onIncomingCallDeclined(const QString& friendNickname) {
    removeIncomingCall(friendNickname);
    emit callDeclined(friendNickname);
}