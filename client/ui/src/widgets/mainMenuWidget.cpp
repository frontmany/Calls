#include "mainMenuWidget.h"

#include <QResizeEvent>
#include <QPainterPath>
#include <QRegularExpressionValidator>
#include <QShortcut>

#include "widgets/components/button.h"
#include "utilities/utilities.h"
#include "constants/constant.h"
#include "constants/color.h"
#include "constants/color.h"

// Style definitions
const QColor StyleMainMenuWidget::m_primaryColor = COLOR_ACCENT;
const QColor StyleMainMenuWidget::m_selectionColor = COLOR_SELECTION_ACCENT;
const QColor StyleMainMenuWidget::m_hoverColor = COLOR_ACCENT_HOVER;
const QColor StyleMainMenuWidget::m_backgroundColor = COLOR_BACKGROUND_MAIN;
const QColor StyleMainMenuWidget::m_textColor = COLOR_TEXT_MAIN;
const QColor StyleMainMenuWidget::m_onlineColor = COLOR_STATUS_ONLINE;
const QColor StyleMainMenuWidget::m_offlineColor = COLOR_STATUS_OFFLINE;
const QColor StyleMainMenuWidget::m_callingColor = COLOR_STATUS_CALLING;
const QColor StyleMainMenuWidget::m_errorColor = COLOR_OVERLAY_ERROR_200;
const QColor StyleMainMenuWidget::m_disabledColor = COLOR_STATE_DISABLED;
const QColor StyleMainMenuWidget::m_settingsButtonColor = COLOR_OVERLAY_SETTINGS_190;
const QColor StyleMainMenuWidget::m_settingsButtonHoverColor = COLOR_OVERLAY_SETTINGS_110;
const QColor StyleMainMenuWidget::m_lineEditBackgroundColor = COLOR_OVERLAY_NEUTRAL_190;
const QColor StyleMainMenuWidget::m_lineEditFocusBackgroundColor = COLOR_OVERLAY_PURE_190;
const QColor StyleMainMenuWidget::m_placeholderColor = COLOR_INPUT_PLACEHOLDER;
const QColor StyleMainMenuWidget::m_callingSectionBackgroundColor = COLOR_OVERLAY_CALLING_180;
const QColor StyleMainMenuWidget::m_stopCallingButtonColor = COLOR_OVERLAY_ERROR_BANNER_180;
const QColor StyleMainMenuWidget::m_stopCallingButtonHoverColor = COLOR_OVERLAY_ERROR_BANNER_220;
const QColor StyleMainMenuWidget::m_incomingCallBackgroundColor = COLOR_OVERLAY_NEUTRAL_200;
const QColor StyleMainMenuWidget::m_settingsPanelBackgroundColor = QColor(240, 240, 240, 200);
const QColor StyleMainMenuWidget::m_scrollBarColor = COLOR_SCROLLBAR_BACKGROUND;
const QColor StyleMainMenuWidget::m_scrollBarHoverColor = COLOR_SCROLLBAR_BACKGROUND_HOVER;
const QColor StyleMainMenuWidget::m_scrollBarPressedColor = COLOR_SCROLLBAR_BACKGROUND_PRESSED;

QString StyleMainMenuWidget::containerStyle() {
    return QString("QWidget {"
        "   background-color: transparent;"
        "   border-radius: %1px;"
        "   padding: 0px;"
        "}")
        .arg(scale(20));
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
        "   border-radius: %3px;"
        "   padding: %4px %5px;"
        "   margin: 0px;"
        "}"
        "QPushButton:focus {"
        "   outline: none;"
        "   border: none;"
        "}"
        "QPushButton:hover {"
        "   background-color: %2;"
        "}")
        .arg(m_primaryColor.name())
        .arg(m_hoverColor.name())
        .arg(scale(15))
        .arg(scale(12))
        .arg(scale(24));
}

QString StyleMainMenuWidget::disabledButtonStyle() {
    return QString("QPushButton {"
        "   background-color: rgba(%1, %2, %3, 120);"
        "   color: rgba(255, 255, 255, 150);"
        "   border: none;"
        "   border-radius: %4px;"
        "   padding: %5px %6px;"
        "   margin: 0px;"
        "}")
        .arg(m_primaryColor.red())
        .arg(m_primaryColor.green())
        .arg(m_primaryColor.blue())
        .arg(scale(15))
        .arg(scale(12))
        .arg(scale(24));
}

QString StyleMainMenuWidget::errorLabelStyle() {
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

QString StyleMainMenuWidget::settingsButtonStyle() {
    return QString("QPushButton {"
        "   background-color: rgba(%1, %2, %3, %4);"
        "   color: white;"
        "   border: none;"
        "   border-radius: %9px;"
        "   padding: %10px %11px %10px %12px;"
        "   font-size: %13px;"
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
        .arg(m_settingsButtonHoverColor.blue()).arg(m_settingsButtonHoverColor.alpha())
        .arg(scale(15))
        .arg(scale(12))
        .arg(scale(40))
        .arg(scale(15))
        .arg(scale(14));
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
        "   border-radius: %5px;"
        "   margin: %6px;"
        "}")
        .arg(m_incomingCallBackgroundColor.red())
        .arg(m_incomingCallBackgroundColor.green())
        .arg(m_incomingCallBackgroundColor.blue())
        .arg(m_incomingCallBackgroundColor.alpha())
        .arg(scale(10))
        .arg(scale(2));
}

QString StyleMainMenuWidget::settingsPanelStyle() {
    return QString("QWidget {"
        "   background-color: rgba(%1, %2, %3, %4);"
        "   border-radius: %5px;"
        "   padding: %6px;"
        "}")
        .arg(m_settingsPanelBackgroundColor.red())
        .arg(m_settingsPanelBackgroundColor.green())
        .arg(m_settingsPanelBackgroundColor.blue())
        .arg(m_settingsPanelBackgroundColor.alpha())
        .arg(scale(10))
        .arg(scale(15));
}

QString StyleMainMenuWidget::scrollAreaStyle() {
    return QString(
        "QScrollArea {"
        "   border: none;"
        "   background: transparent;"
        "}"
        "QScrollBar:vertical {"
        "   background-color: transparent;"
        "   width: %2px;"
        "   margin: 0px;"
        "   border-radius: %3px;"
        "}"
        "QScrollBar::handle:vertical {"
        "   background-color: %1;"
        "   border-radius: %3px;"
        "   min-height: %4px;"
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
    )
        .arg(m_scrollBarColor.name())
        .arg(scale(6))
        .arg(scale(3))
        .arg(scale(20))
        .arg(m_scrollBarHoverColor.name())
        .arg(m_scrollBarPressedColor.name());
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
        "}").arg(m_stopCallingButtonColor.red())                // %1 - red
        .arg(m_stopCallingButtonColor.green())                   // %2 - green
        .arg(m_stopCallingButtonColor.blue())                    // %3 - blue
        .arg(m_stopCallingButtonColor.alpha())                  // %4 - alpha
        .arg(scale(11))                                          // %5 - border-radius
        .arg(scale(6))                                           // %6 - padding vertical
        .arg(scale(12))                                          // %7 - padding horizontal
        .arg(scale(11))                                          // %8 - font-size
        .arg(scale(8));                                          // %9 - margin
}

QString StyleMainMenuWidget::stopCallingButtonHoverStyle() {
    return QString("QPushButton:hover {"
        "   background-color: rgba(%1, %2, %3, %4);"
        "}").arg(m_stopCallingButtonHoverColor.red()).arg(m_stopCallingButtonHoverColor.green())
        .arg(m_stopCallingButtonHoverColor.blue()).arg(m_stopCallingButtonHoverColor.alpha());
}

QString StyleMainMenuWidget::disabledStopCallingButtonStyle() {
    return QString("QPushButton {"
        "   background-color: rgba(%1, %2, %3, 120);"
        "   color: rgba(255, 255, 255, 150);"
        "   border: none;"
        "   border-radius: %4px;"
        "   padding: %5px %6px;"
        "   font-size: %7px;"
        "   margin: %8px;"
        "   opacity: 0.6;"
        "}").arg(m_stopCallingButtonColor.red()).arg(m_stopCallingButtonColor.green()).arg(m_stopCallingButtonColor.blue())
        .arg(QString::fromStdString(std::to_string(scale(11))))  // border-radius
        .arg(QString::fromStdString(std::to_string(scale(6))))   // padding vertical
        .arg(QString::fromStdString(std::to_string(scale(12))))  // padding horizontal
        .arg(QString::fromStdString(std::to_string(scale(11))))  // font-size
        .arg(QString::fromStdString(std::to_string(scale(8))));  // margin
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


    // ������ ��� ����������� �� ����������
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
    QFont friendNicknameFont("Segoe UI", scale(12), QFont::Medium);
    m_friendNicknameEdit->setFont(friendNicknameFont);

    // Set validator for nickname
    QRegularExpressionValidator* validator = new QRegularExpressionValidator(
        QRegularExpression(QString("[\\p{L}0-9_]{%1,%2}").arg(MIN_NICKNAME_LENGTH).arg(MAX_NICKNAME_LENGTH)), this);
    m_friendNicknameEdit->setValidator(validator); 

    m_callButton = new QPushButton("Make Call", m_mainContainer);
    m_callButton->setFixedHeight(scale(50));
    m_callButton->setStyleSheet(StyleMainMenuWidget::buttonStyle());
    m_callButton->setCursor(Qt::PointingHandCursor);
    QFont callButtonFont("Outfit", scale(14), QFont::Normal);
    m_callButton->setFont(callButtonFont);

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
    m_containerLayout->addWidget(m_settingsButton);
    m_containerLayout->addWidget(m_settingsPanel);
    m_containerLayout->addSpacing(scale(10));

    m_mainLayout->addWidget(m_mainContainer, 0, Qt::AlignCenter);

    // Connect signals
    connect(m_callButton, &QPushButton::clicked, this, &MainMenuWidget::onCallButtonClicked);
    connect(m_settingsButton, &QPushButton::clicked, this, &MainMenuWidget::onSettingsButtonClicked);
    connect(m_stopCallingButton, &QPushButton::clicked, this, &MainMenuWidget::onStopCallingButtonClicked);
    connect(m_friendNicknameEdit, &QLineEdit::textChanged, this, &MainMenuWidget::clearErrorMessage);
    connect(m_settingsPanel, &SettingsPanel::audioDevicePickerRequested, [this]() {emit audioDevicePickerRequested(); });
    connect(m_settingsPanel, &SettingsPanel::inputVolumeChanged, [this](int newVolume) {emit inputVolumeChanged(newVolume); });
    connect(m_settingsPanel, &SettingsPanel::outputVolumeChanged, [this](int newVolume) {emit outputVolumeChanged(newVolume); });
    connect(m_settingsPanel, &SettingsPanel::muteMicrophoneClicked, [this](bool mute) {emit muteMicrophoneClicked(mute); });
    connect(m_settingsPanel, &SettingsPanel::muteSpeakerClicked, [this](bool mute) {emit muteSpeakerClicked(mute); });
    connect(m_settingsPanel, &SettingsPanel::cameraButtonClicked, [this](bool activated) {emit activateCameraClicked(activated); });

    QShortcut* enterShortcut = new QShortcut(QKeySequence(Qt::Key_Return), this);
    QShortcut* returnShortcut = new QShortcut(QKeySequence(Qt::Key_Enter), this);

    connect(enterShortcut, &QShortcut::activated, this, &MainMenuWidget::onCallButtonClicked);
    connect(returnShortcut, &QShortcut::activated, this, &MainMenuWidget::onCallButtonClicked);
}

void MainMenuWidget::setupAnimations() {
    // Settings panel animation
    m_settingsAnimation = new QPropertyAnimation(m_settingsPanel, "maximumHeight", this);
    m_settingsAnimation->setDuration(UI_ANIMATION_DURATION_MS);
    m_settingsAnimation->setEasingCurve(QEasingCurve::InOutQuad);
    m_settingsAnimation->setStartValue(0);
    m_settingsAnimation->setEndValue(scale(170));

    connect(m_settingsAnimation, &QPropertyAnimation::finished, this, [this]() {
        if (m_settingsAnimation->direction() == QAbstractAnimation::Backward) {
            m_settingsPanel->hide();
        }
        });

    // Calling section animation
    m_callingAnimation = new QPropertyAnimation(m_callingSection, "maximumHeight", this);
    m_callingAnimation->setDuration(UI_ANIMATION_DURATION_MS);
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
    gradient.setColorAt(0.0, COLOR_GRADIENT_START);
    gradient.setColorAt(0.5, COLOR_GRADIENT_MIDDLE);
    gradient.setColorAt(1.0, COLOR_GRADIENT_END);
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

void MainMenuWidget::setStatusLabelOnline() {
    m_statusLabel->setText("Online");
    m_statusLabel->setStyleSheet(QString("QLabel { color: %1; }").arg(StyleMainMenuWidget::m_primaryColor.name()));
}

void MainMenuWidget::setStatusLabelCalling() {
    m_statusLabel->setText("Calling...");
    m_statusLabel->setStyleSheet(QString("QLabel { color: %1; }").arg(StyleMainMenuWidget::m_callingColor.name()));
}

void MainMenuWidget::setStatusLabelBusy() {
    m_statusLabel->setText("Busy");
    m_statusLabel->setStyleSheet(QString("QLabel { color: %1; }").arg(StyleMainMenuWidget::m_callingColor.name()));
}

void MainMenuWidget::setStatusLabelOffline() {
    m_statusLabel->setText("Offline");
    m_statusLabel->setStyleSheet(QString("QLabel { color: %1; }").arg(StyleMainMenuWidget::m_offlineColor.name()));
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

void MainMenuWidget::onCallButtonClicked() {
    QString friendNickname = m_friendNicknameEdit->text().trimmed();
    emit startCallingButtonClicked(friendNickname);
}

void MainMenuWidget::setErrorMessage(const QString& errorText) {
    m_errorLabel->setText(errorText);
    m_errorLabel->show();
    m_friendNicknameEdit->setFocus();

    QTimer::singleShot(ERROR_MESSAGE_DURATION_MS, this, &MainMenuWidget::clearErrorMessage);
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

void MainMenuWidget::setCameraActive(bool active) {
    m_settingsPanel->setCameraActive(active);
}

void MainMenuWidget::setCallButtonEnabled(bool enabled)
{
    if (m_callButton) {
        m_callButton->setEnabled(enabled);
        if (enabled) {
            m_callButton->setStyleSheet(StyleMainMenuWidget::buttonStyle());
        }
        else {
            m_callButton->setStyleSheet(StyleMainMenuWidget::disabledButtonStyle());
        }
    }
}

void MainMenuWidget::setStopCallingButtonEnabled(bool enabled)
{
    if (m_stopCallingButton) {
        m_stopCallingButton->setEnabled(enabled);
        if (enabled) {
            m_stopCallingButton->setStyleSheet(
                StyleMainMenuWidget::stopCallingButtonStyle() +
                StyleMainMenuWidget::stopCallingButtonHoverStyle()
            );
        }
        else {
            m_stopCallingButton->setStyleSheet(StyleMainMenuWidget::disabledStopCallingButtonStyle());
        }
    }
}
