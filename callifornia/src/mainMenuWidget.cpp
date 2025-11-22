#include "mainMenuWidget.h"

#include <QResizeEvent>
#include <QPainter>
#include <QPainterPath>
#include <QFontDatabase>
#include <QApplication>
#include <QGraphicsDropShadowEffect>
#include <QRegularExpressionValidator>

#include "buttons.h"
#include "scaleFactor.h"

// Style definitions
const QColor StyleMainMenuWidget::m_primaryColor = QColor(21, 119, 232);
const QColor StyleMainMenuWidget::m_selectionColor = QColor(79, 161, 255);
const QColor StyleMainMenuWidget::m_hoverColor = QColor(18, 113, 222);
const QColor StyleMainMenuWidget::m_backgroundColor = QColor(230, 230, 230);
const QColor StyleMainMenuWidget::m_textColor = QColor(1, 11, 19);
const QColor StyleMainMenuWidget::m_onlineColor = QColor(100, 200, 100);
const QColor StyleMainMenuWidget::m_offlineColor = QColor(150, 150, 150);
const QColor StyleMainMenuWidget::m_callingColor = QColor(255, 165, 0);
const QColor StyleMainMenuWidget::m_errorColor = QColor(220, 80, 80, 200);
const QColor StyleMainMenuWidget::m_disabledColor = QColor(120, 120, 120);
const QColor StyleMainMenuWidget::m_settingsButtonColor = QColor(235, 235, 235, 190);
const QColor StyleMainMenuWidget::m_settingsButtonHoverColor = QColor(235, 235, 235, 110);
const QColor StyleMainMenuWidget::m_lineEditBackgroundColor = QColor(245, 245, 245, 190);
const QColor StyleMainMenuWidget::m_lineEditFocusBackgroundColor = QColor(255, 255, 255, 190);
const QColor StyleMainMenuWidget::m_placeholderColor = QColor(240, 240, 240, 180);
const QColor StyleMainMenuWidget::m_callingSectionBackgroundColor = QColor(255, 245, 235, 180);
const QColor StyleMainMenuWidget::m_stopCallingButtonColor = QColor(232, 53, 53, 180);
const QColor StyleMainMenuWidget::m_stopCallingButtonHoverColor = QColor(232, 53, 53, 220);
const QColor StyleMainMenuWidget::m_incomingCallBackgroundColor = QColor(245, 245, 245, 200);
const QColor StyleMainMenuWidget::m_settingsPanelBackgroundColor = QColor(240, 240, 240, 200);
const QColor StyleMainMenuWidget::m_scrollBarColor = QColor(208, 208, 208);
const QColor StyleMainMenuWidget::m_scrollBarHoverColor = QColor(176, 176, 176);
const QColor StyleMainMenuWidget::m_scrollBarPressedColor = QColor(144, 144, 144);

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
        "QPushButton:focus {"
        "   outline: none;"
        "   border: none;"
        "}"
        "QPushButton:hover {"
        "   background-color: %2;"
        "}").arg(m_primaryColor.name()).arg(m_hoverColor.name());
}

QString StyleMainMenuWidget::disabledButtonStyle() {
    return QString("QPushButton {"
        "   background-color: rgba(%1, %2, %3, 120);"
        "   color: rgba(255, 255, 255, 150);"
        "   border: none;"
        "   border-radius: 15px;"
        "   padding: 12px 24px;"
        "   margin: 0px;"
        "}").arg(m_primaryColor.red()).arg(m_primaryColor.green()).arg(m_primaryColor.blue());
}

QString StyleMainMenuWidget::errorLabelStyle() {
    return QString("QLabel {"
        "   color: %1;"
        "   margin: 2px 2px;"
        "   padding: 5px;"
        "   background: transparent;"
        "   border-radius: 5px;"
        "}").arg(m_errorColor.name());
}

QString StyleMainMenuWidget::settingsButtonStyle() {
    return QString("QPushButton {"
        "   background-color: rgba(%1, %2, %3, %4);"
        "   color: white;"
        "   border: none;"
        "   border-radius: 15px;"
        "   padding: 12px 40px 12px 15px;"
        "   font-size: 14px;"
        "   font-weight: bold;"
        "   margin: 0px;"
        "}"
        "QPushButton:hover {"
        "   background-color: rgba(%5, %6, %7, %8);"
        "}"
        "QPushButton:pressed {"
        "   background-color: rgba(%5, %6, %7, %8);"
        "}"
        "QPushButton:focus {"
        "   outline: none;"
        "   border: none;"
        "   background-color: rgba(%5, %6, %7, %8);"
        "}"
        "QPushButton::menu-indicator {"
        "   image: none;"
        "   width: 0px;"
        "}").arg(m_settingsButtonColor.red()).arg(m_settingsButtonColor.green())
        .arg(m_settingsButtonColor.blue()).arg(m_settingsButtonColor.alpha())
        .arg(m_settingsButtonHoverColor.red()).arg(m_settingsButtonHoverColor.green())
        .arg(m_settingsButtonHoverColor.blue()).arg(m_settingsButtonHoverColor.alpha());
}

QString StyleMainMenuWidget::lineEditStyle() {
    return QString("QLineEdit {"
        "   background-color: rgba(%1, %2, %3, %4);"
        "   border: 0px solid %5;"
        "   border-radius: %17px;"
        "   padding: %18px %19px;"
        "   margin: 0px;"
        "   color: %6;"
        "   selection-background-color: %7;"
        "}"
        "QLineEdit:focus {"
        "   border: 0px solid %8;"
        "   background-color: rgba(%9, %10, %11, %12);"
        "}"
        "QLineEdit::placeholder {"
        "   color: rgba(%13, %14, %15, %16);"
        "}").arg(m_lineEditBackgroundColor.red()).arg(m_lineEditBackgroundColor.green())
        .arg(m_lineEditBackgroundColor.blue()).arg(m_lineEditBackgroundColor.alpha())
        .arg(m_textColor.name()).arg(m_textColor.name())
        .arg(m_selectionColor.name())  
        .arg(m_textColor.name()) 
        .arg(m_lineEditFocusBackgroundColor.red()).arg(m_lineEditFocusBackgroundColor.green())
        .arg(m_lineEditFocusBackgroundColor.blue()).arg(m_lineEditFocusBackgroundColor.alpha())
        .arg(m_placeholderColor.red()).arg(m_placeholderColor.green())
        .arg(m_placeholderColor.blue()).arg(m_placeholderColor.alpha())
        .arg(QString::fromStdString(std::to_string(scale(15))))  // border-radius
        .arg(QString::fromStdString(std::to_string(scale(12))))  // padding vertical
        .arg(QString::fromStdString(std::to_string(scale(15)))); // padding horizontal
}

QString StyleMainMenuWidget::disabledLineEditStyle() {
    return QString("QLineEdit {"
        "   background-color: rgba(235, 235, 235, 120);"
        "   border: 0px solid %1;"
        "   border-radius: %2px;"
        "   padding: %3px %4px;"
        "   margin: 0px;"
        "   color: #888888;"
        "}").arg(m_textColor.name())
        .arg(QString::fromStdString(std::to_string(scale(15))))  // border-radius
        .arg(QString::fromStdString(std::to_string(scale(12))))  // padding vertical
        .arg(QString::fromStdString(std::to_string(scale(15)))); // padding horizontal
}

QString StyleMainMenuWidget::notificationRedLabelStyle() {
    return QString("QWidget {"
        "   background-color: rgba(220, 80, 80, 100);"
        "   border: none;"
        "   border-radius: %1px;"
        "   margin: 0px;"
        "   padding: 0px;"
        "}").arg(QString::fromStdString(std::to_string(scale(8))));
}

QString StyleMainMenuWidget::avatarStyle() {
    return QString("QLabel {"
        "   background-color: black;"
        "   border-radius: %1;"
        "   color: white;"
        "   font-size: %2;"
        "   font-weight: bold;"
        "   margin: 0px;"
        "   padding: 0px;"
        "}").arg(QString::fromStdString(std::to_string(scale(25))) + "px")
        .arg(QString::fromStdString(std::to_string(scale(18))) + "px");
}

QString StyleMainMenuWidget::incomingCallWidgetStyle() {
    return QString("QWidget {"
        "   background-color: rgba(%1, %2, %3, %4);"
        "   border-radius: 10px;"
        "   margin: 2px;"
        "}").arg(m_incomingCallBackgroundColor.red()).arg(m_incomingCallBackgroundColor.green())
        .arg(m_incomingCallBackgroundColor.blue()).arg(m_incomingCallBackgroundColor.alpha());
}

QString StyleMainMenuWidget::settingsPanelStyle() {
    return QString("QWidget {"
        "   background-color: rgba(%1, %2, %3, %4);"
        "   border-radius: 10px;"
        "   padding: 15px;"
        "}").arg(m_settingsPanelBackgroundColor.red()).arg(m_settingsPanelBackgroundColor.green())
        .arg(m_settingsPanelBackgroundColor.blue()).arg(m_settingsPanelBackgroundColor.alpha());
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
        "   background-color: %1;"
        "   border-radius: 3px;"
        "   min-height: 20px;"
        "}"
        "QScrollBar::handle:vertical:hover {"
        "   background-color: %2;"
        "}"
        "QScrollBar::handle:vertical:pressed {"
        "   background-color: %3;"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
        "   border: none;"
        "   background: none;"
        "   height: 0px;"
        "}"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {"
        "   background: none;"
        "}"
    ).arg(m_scrollBarColor.name()).arg(m_scrollBarHoverColor.name()).arg(m_scrollBarPressedColor.name());
}

QString StyleMainMenuWidget::callingSectionStyle() {
    return QString("QWidget {"
        "   background-color: rgba(%1, %2, %3, %4);"
        "   border-radius: %5px;"
        "   padding: %6px %7px;"
        "   margin: %8px %9px;"
        "}").arg(m_callingSectionBackgroundColor.red()).arg(m_callingSectionBackgroundColor.green())
        .arg(m_callingSectionBackgroundColor.blue()).arg(m_callingSectionBackgroundColor.alpha())
        .arg(QString::fromStdString(std::to_string(scale(15))))  // border-radius
        .arg(QString::fromStdString(std::to_string(scale(12))))  // padding vertical
        .arg(QString::fromStdString(std::to_string(scale(15))))  // padding horizontal
        .arg(QString::fromStdString(std::to_string(scale(5))))   // margin vertical
        .arg(QString::fromStdString(std::to_string(scale(0))));  // margin horizontal
}

QString StyleMainMenuWidget::callingTextStyle() {
    return QString("QLabel {"
        "   color: %1;"
        "   margin: 0px;"
        "   padding: 0px;"
        "}").arg(m_callingColor.name());
}

QString StyleMainMenuWidget::incomingCallsHeaderStyle() {
    return QString("QLabel {"
        "   color: %1;"
        "   margin: 0px;"
        "   padding: 0px;"
        "}").arg(m_textColor.name());
}

QString StyleMainMenuWidget::stopCallingButtonStyle() {
    return QString("QPushButton {"
        "   background-color: rgba(%1, %2, %3, %4);"
        "   color: white;"
        "   border: none;"
        "   border-radius: %5px;"
        "   padding: %6px %7px;"
        "   font-size: %8px;"
        "   margin: %9px;"
        "}").arg(m_stopCallingButtonColor.red()).arg(m_stopCallingButtonColor.green())
        .arg(m_stopCallingButtonColor.blue()).arg(m_stopCallingButtonColor.alpha())
        .arg(QString::fromStdString(std::to_string(scale(11))))  // border-radius
        .arg(QString::fromStdString(std::to_string(scale(6))))   // padding vertical
        .arg(QString::fromStdString(std::to_string(scale(12))))  // padding horizontal
        .arg(QString::fromStdString(std::to_string(scale(11))))  // font-size
        .arg(QString::fromStdString(std::to_string(scale(8))));  // margin
}

QString StyleMainMenuWidget::stopCallingButtonHoverStyle() {
    return QString("QPushButton:hover {"
        "   background-color: rgba(%1, %2, %3, %4);"
        "}").arg(m_stopCallingButtonHoverColor.red()).arg(m_stopCallingButtonHoverColor.green())
        .arg(m_stopCallingButtonHoverColor.blue()).arg(m_stopCallingButtonHoverColor.alpha());
}

QString StyleMainMenuWidget::notificationBlueLabelStyle() {
    return QString("QWidget {"
        "   background-color: rgba(21, 119, 232, 80);"  // ����� ���� � �������������
        "   border: none;"
        "   border-radius: %1px;"
        "   margin: 0px;"
        "   padding: 0px;"
        "}").arg(QString::fromStdString(std::to_string(scale(8))));
}

MainMenuWidget::MainMenuWidget(QWidget* parent) : QWidget(parent) {
    setupUI();
    setupAnimations();

    m_notificationTimer = new QTimer(this);
    m_notificationTimer->setSingleShot(true);
    connect(m_notificationTimer, &QTimer::timeout, [this]() { m_notificationWidget->hide(); });

    // ������ ��� ����������� �� ����������
    m_updateNotificationTimer = new QTimer(this);
    m_updateNotificationTimer->setSingleShot(true);
    connect(m_updateNotificationTimer, &QTimer::timeout, this, &MainMenuWidget::hideUpdateAvailableNotification);
}

void MainMenuWidget::setupUI() {
    m_backgroundTexture = QPixmap(":/resources/blur.png");

    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setAlignment(Qt::AlignCenter);
    m_mainLayout->setContentsMargins(scale(40), scale(40), scale(40), scale(40));

    // Main container
    m_mainContainer = new QWidget(this);
    m_mainContainer->setFixedWidth(scale(500));
    m_mainContainer->setStyleSheet(StyleMainMenuWidget::containerStyle());

    m_containerLayout = new QVBoxLayout(m_mainContainer);
    m_containerLayout->setSpacing(scale(20));
    m_containerLayout->setContentsMargins(scale(30), scale(30), scale(30), scale(30));

    // Create network error widget
    m_notificationWidget = new QWidget(this);
    m_notificationWidget->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    m_notificationWidget->hide();
    m_notificationWidget->setStyleSheet(StyleMainMenuWidget::notificationRedLabelStyle());


    m_notificationLayout = new QHBoxLayout(m_notificationWidget);
    m_notificationLayout->setAlignment(Qt::AlignCenter);
    m_notificationLayout->setContentsMargins(scale(18), scale(8), scale(18), scale(8));

    m_notificationLabel = new QLabel(m_notificationWidget);
    QFont errorFont("Outfit", scale(12), QFont::Medium);
    m_notificationLabel->setFont(errorFont);
    m_notificationLabel->setStyleSheet("color: #DC5050; background: transparent; font-size: 14px; margin: 0px; padding: 0px;");

    m_notificationLayout->addWidget(m_notificationLabel);

    m_updateNotificationButton = new QPushButton(this);
    m_updateNotificationButton->setMinimumSize(scale(295), scale(32));
    m_updateNotificationButton->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    m_updateNotificationButton->hide();
    m_updateNotificationButton->setCursor(Qt::PointingHandCursor);

    m_updateNotificationButton->setStyleSheet(
        "QPushButton {"
        "   background-color: rgba(21, 119, 232, 80);"
        "   color: #1577E8;"
        "   border: none;"
        "   border-radius: 12px;"
        "   padding: 8px 18px 8px 15px;"
        "   margin: 0px;"
        "}"
        "QPushButton:hover {"
        "   background-color: rgba(21, 119, 232, 120);"
        "   color: #0D6BC8;"
        "}"
        "QPushButton:pressed {"
        "   background-color: rgba(21, 119, 232, 150);"
        "   color: #0A5FC8;"
        "}"
    );

    QFont updateFont("Outfit", scale(13), QFont::Medium);
    m_updateNotificationButton->setFont(updateFont);
    m_updateNotificationButton->setText("New version available! Click to update");
    m_updateNotificationButton->setCursor(Qt::PointingHandCursor);


    // Title
    m_titleLabel = new QLabel("Callifornia", m_mainContainer);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_titleLabel->setStyleSheet(StyleMainMenuWidget::titleStyle());
    m_titleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    m_titleLabel->setMinimumHeight(scale(70));
    QFont titleFont("Pacifico", scale(32));
    m_titleLabel->setFont(titleFont);

    // User info section
    m_userInfoWidget = new QWidget(m_mainContainer);
    m_userInfoLayout = new QHBoxLayout(m_userInfoWidget);
    m_userInfoLayout->setSpacing(scale(15));
    m_userInfoLayout->setContentsMargins(0, 0, 0, 0);
    m_userInfoLayout->setAlignment(Qt::AlignCenter);
    
    m_avatarLabel = new QLabel("", m_userInfoWidget);
    m_avatarLabel->setFixedSize(scale(50), scale(50));
    m_avatarLabel->setAlignment(Qt::AlignCenter);

    m_userTextLayout = new QVBoxLayout();
    m_nicknameLabel = new QLabel("Guest", m_userInfoWidget);
    QFont nicknameFont("Outfit", scale(18), QFont::Normal);
    m_nicknameLabel->setFont(nicknameFont);

    m_statusLabel = new QLabel("Offline", m_userInfoWidget);
    QFont statusFont("Outfit", scale(11), QFont::Light);
    m_statusLabel->setFont(statusFont);

    m_nicknameLabel->setStyleSheet(StyleMainMenuWidget::nicknameStyle());

    m_userTextLayout->addWidget(m_nicknameLabel);
    m_userTextLayout->addSpacing(scale(-15));
    m_userTextLayout->addWidget(m_statusLabel);

    m_userInfoLayout->addWidget(m_avatarLabel);
    m_userInfoLayout->addLayout(m_userTextLayout);

    // Calling section (initially hidden)
    m_callingSection = new QWidget(m_mainContainer);
    m_callingSection->setStyleSheet(StyleMainMenuWidget::callingSectionStyle());
    m_callingSection->setFixedHeight(scale(50));
    m_callingSection->hide();

    m_callingLayout = new QHBoxLayout(m_callingSection);
    m_callingLayout->setSpacing(scale(10));
    m_callingLayout->setContentsMargins(0, 0, 0, 0);
    m_callingLayout->setAlignment(Qt::AlignCenter);

    // Calling text
    m_callingText = new QLabel("Calling...", m_callingSection);
    m_callingText->setStyleSheet(StyleMainMenuWidget::callingTextStyle());
    QFont callingFont("Outfit", scale(12), QFont::Normal);
    m_callingText->setFont(callingFont);

    // Cancel call button
    m_stopCallingButton = new QPushButton("Cancel", m_callingSection);
    m_stopCallingButton->setStyleSheet(
        StyleMainMenuWidget::stopCallingButtonStyle() +
        StyleMainMenuWidget::stopCallingButtonHoverStyle()
    );
    m_stopCallingButton->setCursor(Qt::PointingHandCursor);

    m_callingLayout->addSpacing(scale(15));
    m_callingLayout->addWidget(m_callingText);
    m_callingLayout->addStretch();
    m_callingLayout->addWidget(m_stopCallingButton);

    // Call section
    m_errorLabel = new QLabel("field cannot be empty", m_mainContainer);
    m_errorLabel->setStyleSheet(StyleMainMenuWidget::errorLabelStyle());
    m_errorLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    m_errorLabel->hide();
    QFont errorLabelFont("Outfit", scale(10), QFont::ExtraLight);
    m_errorLabel->setFont(errorLabelFont);

    m_friendNicknameEdit = new QLineEdit(m_mainContainer);
    m_friendNicknameEdit->setFixedHeight(scale(50));
    m_friendNicknameEdit->setPlaceholderText("Enter friend's nickname");
    m_friendNicknameEdit->setStyleSheet(StyleMainMenuWidget::lineEditStyle());
    QFont friendNicknameFont("Outfit", scale(12), QFont::Light);
    m_friendNicknameEdit->setFont(friendNicknameFont);

    // Set validator for nickname
    QRegularExpressionValidator* validator = new QRegularExpressionValidator(
        QRegularExpression("[\\p{L}0-9_]{3,15}"), this);
    m_friendNicknameEdit->setValidator(validator);

    m_callButton = new QPushButton("Make Call", m_mainContainer);
    m_callButton->setFixedHeight(scale(50));
    m_callButton->setStyleSheet(StyleMainMenuWidget::buttonStyle());
    m_callButton->setCursor(Qt::PointingHandCursor);
    QFont callButtonFont("Outfit", scale(14), QFont::Normal);
    m_callButton->setFont(callButtonFont);

    m_incomingCallsContainer = new QWidget();
    m_incomingCallsLayout = new QVBoxLayout(m_incomingCallsContainer);
    m_incomingCallsLayout->setAlignment(Qt::AlignTop);
    m_incomingCallsLayout->addSpacing(scale(5));

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
    buttonLayout->setContentsMargins(scale(15), scale(5), scale(15), scale(5));
    buttonLayout->setSpacing(scale(10));


    ButtonIcon* leftIcon = new ButtonIcon(this,
        QIcon(":/resources/speaker.png"),
        QIcon(":/resources/speaker.png"),
        scale(20), scale(20));
    leftIcon->setAttribute(Qt::WA_TransparentForMouseEvents, true);

    QLabel* textLabel = new QLabel("Audio Controls");
    textLabel->setStyleSheet("color: inherit; background: transparent;");
    QFont textLabelFont("Outfit", scale(12), QFont::Light);
    textLabel->setFont(textLabelFont);

    ButtonIcon* rightIcon = new ButtonIcon(this,
        QIcon(":/resources/arrowDown.png"),
        QIcon(":/resources/arrowDown.png"),
        scale(16), scale(16));
    rightIcon->setAttribute(Qt::WA_TransparentForMouseEvents, true);

    buttonLayout->addWidget(leftIcon);
    buttonLayout->addWidget(textLabel);
    buttonLayout->addStretch();
    buttonLayout->addWidget(rightIcon);

    m_settingsButton = new QPushButton(m_mainContainer);
    m_settingsButton->setFixedHeight(scale(50));
    m_settingsButton->setLayout(buttonLayout);
    m_settingsButton->setStyleSheet(StyleMainMenuWidget::settingsButtonStyle());
    m_settingsButton->setCursor(Qt::PointingHandCursor);

    // Settings panel (initially hidden)
    m_settingsPanel = new SettingsPanel(m_mainContainer);
    m_settingsPanel->hide();

    // Add widgets to layout
    m_containerLayout->addWidget(m_titleLabel);
    m_containerLayout->addSpacing(scale(-4));
    m_containerLayout->addWidget(m_userInfoWidget);
    m_containerLayout->addSpacing(scale(12));
    m_containerLayout->addWidget(m_callingSection);
    m_containerLayout->addWidget(m_errorLabel);
    m_containerLayout->addWidget(m_friendNicknameEdit);
    m_containerLayout->addWidget(m_callButton);
    m_containerLayout->addWidget(m_incomingCallsScrollArea);
    m_containerLayout->addWidget(m_settingsButton);
    m_containerLayout->addWidget(m_settingsPanel);
    m_containerLayout->addSpacing(scale(10));

    m_mainLayout->addWidget(m_updateNotificationButton, 0, Qt::AlignCenter);
    m_mainLayout->addSpacing(scale(20));
    m_mainLayout->addWidget(m_notificationWidget, 0, Qt::AlignCenter);
    m_mainLayout->addSpacing(scale(20));
    m_mainLayout->addWidget(m_mainContainer, 0, Qt::AlignCenter);

    // Connect signals
    connect(m_callButton, &QPushButton::clicked, this, &MainMenuWidget::onCallButtonClicked);
    connect(m_settingsButton, &QPushButton::clicked, this, &MainMenuWidget::onSettingsButtonClicked);
    connect(m_stopCallingButton, &QPushButton::clicked, this, &MainMenuWidget::onStopCallingButtonClicked);
    connect(m_friendNicknameEdit, &QLineEdit::textChanged, this, &MainMenuWidget::clearErrorMessage);
    connect(m_friendNicknameEdit, &QLineEdit::returnPressed, this, &MainMenuWidget::onCallButtonClicked);
    connect(m_settingsPanel, &SettingsPanel::refreshAudioDevicesButtonClicked, [this]() {emit refreshAudioDevicesButtonClicked(); });
    connect(m_settingsPanel, &SettingsPanel::inputVolumeChanged, [this](int newVolume) {emit inputVolumeChanged(newVolume); });
    connect(m_settingsPanel, &SettingsPanel::outputVolumeChanged, [this](int newVolume) {emit outputVolumeChanged(newVolume); });
    connect(m_settingsPanel, &SettingsPanel::muteMicrophoneClicked, [this](bool mute) {emit muteMicrophoneClicked(mute); });
    connect(m_settingsPanel, &SettingsPanel::muteSpeakerClicked, [this](bool mute) {emit muteSpeakerClicked(mute); });
    connect(m_updateNotificationButton, &QPushButton::clicked, this, [this]() {emit updateButtonClicked(); hideUpdateAvailableNotification(); });
}

void MainMenuWidget::setupAnimations() {
    // Settings panel animation
    m_settingsAnimation = new QPropertyAnimation(m_settingsPanel, "maximumHeight", this);
    m_settingsAnimation->setDuration(200);
    m_settingsAnimation->setEasingCurve(QEasingCurve::InOutQuad);
    m_settingsAnimation->setStartValue(0);
    m_settingsAnimation->setEndValue(scale(170));

    connect(m_settingsAnimation, &QPropertyAnimation::finished, this, [this]() {
        if (m_settingsAnimation->direction() == QAbstractAnimation::Backward) {
            m_settingsPanel->hide();
        }
        });

    // Incoming calls area animation
    m_incomingCallsAnimation = new QPropertyAnimation(m_incomingCallsScrollArea, "maximumHeight", this);
    m_incomingCallsAnimation->setDuration(90);
    m_incomingCallsAnimation->setEasingCurve(QEasingCurve::InOutQuad);
    m_incomingCallsAnimation->setStartValue(0);
    m_incomingCallsAnimation->setEndValue(scale(120));

    connect(m_incomingCallsAnimation, &QPropertyAnimation::finished, this, [this]() {
        if (m_incomingCallsAnimation->direction() == QAbstractAnimation::Backward) {
            m_incomingCallsScrollArea->hide();
        }
        else {
            m_incomingCallsScrollArea->setMinimumHeight(scale(110));
        }
        });

    // Calling section animation
    m_callingAnimation = new QPropertyAnimation(m_callingSection, "maximumHeight", this);
    m_callingAnimation->setDuration(200);
    m_callingAnimation->setEasingCurve(QEasingCurve::InOutQuad);
    m_callingAnimation->setStartValue(0);
    m_callingAnimation->setEndValue(scale(50));

    connect(m_callingAnimation, &QPropertyAnimation::finished, this, [this]() {
        if (m_callingAnimation->direction() == QAbstractAnimation::Backward) {
            m_callingSection->hide();
        }
    });
}

void MainMenuWidget::setFocusToLineEdit() {
    m_friendNicknameEdit->setFocus();
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

    QString firstLetter = nickname.left(1).toUpper();

    m_avatarLabel->setText(firstLetter);
    m_avatarLabel->setStyleSheet(StyleMainMenuWidget::avatarStyle());
}

void MainMenuWidget::setState(calls::State state) {
    if (state == calls::State::FREE) {
        m_statusLabel->setText("Online");
        m_statusLabel->setStyleSheet(QString("QLabel { color: %1; }").arg(StyleMainMenuWidget::m_primaryColor.name()));
    }
    else if (state == calls::State::CALLING) {
        m_statusLabel->setText("Calling...");
        m_statusLabel->setStyleSheet(QString("QLabel { color: %1; }").arg(StyleMainMenuWidget::m_callingColor.name()));
    }
    else if (state == calls::State::BUSY) {
        m_statusLabel->setText("Busy");
        m_statusLabel->setStyleSheet(QString("QLabel { color: %1; }").arg(StyleMainMenuWidget::m_callingColor.name()));
    }
    else if (state == calls::State::UNAUTHORIZED) {
        m_statusLabel->setText("Offline");
        m_statusLabel->setStyleSheet(QString("QLabel { color: %1; }").arg(StyleMainMenuWidget::m_offlineColor.name()));
    }
}


std::vector<std::pair<std::string, int>> MainMenuWidget::getIncomingCalls() const {
    std::vector<std::pair<std::string, int>> incomingCalls;

    for (int i = 0; i < m_incomingCallsLayout->count(); ++i) {
        QWidget* widget = m_incomingCallsLayout->itemAt(i)->widget();

        if (auto* callWidget = qobject_cast<IncomingCallWidget*>(widget)) {
            QString nickname = callWidget->getFriendNickname();
            int remainingTime = callWidget->getRemainingTime();

            incomingCalls.emplace_back(nickname.toStdString(), remainingTime);
        }
    }

    return incomingCalls;
}

void MainMenuWidget::addIncomingCall(const QString& friendNickName, int remainingTime) {
    if (m_incomingCallWidgets.contains(friendNickName)) return;

    IncomingCallWidget* callWidget = new IncomingCallWidget(this, friendNickName, remainingTime);
    m_incomingCallsLayout->addWidget(callWidget);
    m_incomingCallWidgets[friendNickName] = callWidget;

    connect(callWidget, &IncomingCallWidget::callAccepted, this, &MainMenuWidget::onIncomingCallAccepted);
    connect(callWidget, &IncomingCallWidget::callDeclined, this, &MainMenuWidget::onIncomingCallDeclined);

    showIncomingCallsArea();
}

void MainMenuWidget::removeIncomingCall(const QString& callerName) {
    if (m_incomingCallWidgets.contains(callerName)) {
        IncomingCallWidget* callWidget = m_incomingCallWidgets[callerName];
        m_incomingCallsLayout->removeWidget(callWidget);
        callWidget->deleteLater();
        m_incomingCallWidgets.remove(callerName);

        if (m_incomingCallWidgets.size() == 0) {
            hideIncomingCallsArea();
        }
    }
}

void MainMenuWidget::clearIncomingCalls() {
    for (IncomingCallWidget* callWidget : m_incomingCallWidgets) {
        m_incomingCallsLayout->removeWidget(callWidget);
        callWidget->deleteLater();
    }
    m_incomingCallWidgets.clear();
    hideIncomingCallsArea();
}

void MainMenuWidget::showCallingPanel(const QString& friendNickname) {
    m_callingFriend = friendNickname;
    m_callingText->setText("Calling " + friendNickname + "...");
    updateCallingState(true);

    // Show calling section with animation
    if (m_callingSection->isHidden()) {
        m_callingSection->setMaximumHeight(0);
        m_callingSection->show();
        m_callingAnimation->setDirection(QAbstractAnimation::Forward);
        m_callingAnimation->start();
    }
}

void MainMenuWidget::removeCallingPanel() {
    m_callingFriend.clear();
    updateCallingState(false);

    // Hide calling section with animation
    if (!m_callingSection->isHidden()) {
        m_callingAnimation->setDirection(QAbstractAnimation::Backward);
        m_callingAnimation->start();
    }

    m_friendNicknameEdit->clear();
}

void MainMenuWidget::updateCallingState(bool calling) {
    // Block/unblock input and call button
    m_friendNicknameEdit->setEnabled(!calling);
    m_callButton->setEnabled(!calling);

    // Visual feedback for disabled state
    if (calling) {
        m_friendNicknameEdit->setStyleSheet(StyleMainMenuWidget::disabledLineEditStyle());
        m_callButton->setStyleSheet(StyleMainMenuWidget::disabledButtonStyle());
    }
    else {
        m_friendNicknameEdit->setStyleSheet(StyleMainMenuWidget::lineEditStyle());
        m_callButton->setStyleSheet(StyleMainMenuWidget::buttonStyle());
    }
}

void MainMenuWidget::showUpdateAvailableNotification() {
    m_updateNotificationButton->show();
}

void MainMenuWidget::hideUpdateAvailableNotification() {
    m_updateNotificationButton->hide();
}

void MainMenuWidget::showErrorNotification(const QString& text, int durationMs) {
    m_notificationLabel->setText(text);
    m_notificationWidget->show();
    m_notificationTimer->start(durationMs);
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
        m_incomingCallsScrollArea->setMinimumHeight(0);
        m_incomingCallsAnimation->setDirection(QAbstractAnimation::Backward);
        m_incomingCallsAnimation->start();
    }
}

void MainMenuWidget::onCallButtonClicked() {
    QString friendNickname = m_friendNicknameEdit->text().trimmed();
    emit startCallingButtonClicked(friendNickname);
}

void MainMenuWidget::setErrorMessage(const QString& errorText) {
    m_errorLabel->setText(errorText);
    m_errorLabel->show();
    m_friendNicknameEdit->setFocus();

    QTimer::singleShot(2500, this, &MainMenuWidget::clearErrorMessage);
}

void MainMenuWidget::clearErrorMessage() {
    m_errorLabel->hide();
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

void MainMenuWidget::onStopCallingButtonClicked() {
    emit stopCallingButtonClicked();
}

void MainMenuWidget::setInputVolume(int volume) {
    m_settingsPanel->setInputVolume(volume);
}

void MainMenuWidget::setOutputVolume(int volume) {
    m_settingsPanel->setOutputVolume(volume);
}

void MainMenuWidget::setMicrophoneMuted(bool muted) {
    m_settingsPanel->setMicrophoneMuted(muted);
}

void MainMenuWidget::setSpeakerMuted(bool muted) {
    m_settingsPanel->setSpeakerMuted(muted);
}

void MainMenuWidget::onIncomingCallAccepted(const QString& friendNickname) {
    emit acceptCallButtonClicked(friendNickname);
}

void MainMenuWidget::onIncomingCallDeclined(const QString& friendNickname) {
    emit declineCallButtonClicked(friendNickname);
}