#include "mainWindow.h"
#include <QStackedLayout>
#include <QHBoxLayout>
#include <QFontDatabase>

#include "AuthorizationWidget.h"
#include "MainMenuWidget.h"
#include "CallWidget.h"

MainWindow::MainWindow(QWidget* parent, const std::string& host)
    : QMainWindow(parent) {

    calls::init(host,
        [this](calls::Result authorizationResult) {
            QMetaObject::invokeMethod(this, "onAuthorizationResult", Qt::QueuedConnection,
                Q_ARG(calls::Result, authorizationResult));
        },
        [this](calls::Result createCallResult) {
            QMetaObject::invokeMethod(this, "onCreateCallResult", Qt::QueuedConnection,
                Q_ARG(calls::Result, createCallResult));
        },
        [this](const std::string& friendNickName) {
            QMetaObject::invokeMethod(this, "onIncomingCall", Qt::QueuedConnection,
                Q_ARG(std::string, friendNickName));
        },
        [this](const std::string& friendNickName) {
            QMetaObject::invokeMethod(this, "onIncomingCallExpired", Qt::QueuedConnection,
                Q_ARG(std::string, friendNickName));
        },
        [this]() {
            QMetaObject::invokeMethod(this, "onCallHangUp", Qt::QueuedConnection);
        },
        [this]() {
            QMetaObject::invokeMethod(this, "onNetworkError", Qt::QueuedConnection);
        }
    );

    loadFonts();
    setupUI();

    m_mainMenuWidget->setNickname("feder");

    // TESTING: Uncomment the line below to test CallWidget immediately
    switchToCallWidget("TestFriend");
}

void MainWindow::loadFonts() {
    if (QFontDatabase::addApplicationFont(":/resources/Pacifico-Regular.ttf") == -1) {
        qWarning() << "Font load error:";
    }

    if (QFontDatabase::addApplicationFont(":/resources/Outfit-VariableFont_wght.ttf") == -1) {
        qWarning() << "Font load error:";
    }
}

void MainWindow::setupUI() {
    // Create central widget
    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);

    // Create main layout
    m_mainLayout = new QHBoxLayout(m_centralWidget);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    // Create stacked layout for switching between widgets
    m_stackedLayout = new QStackedLayout();
    m_mainLayout->addLayout(m_stackedLayout);

    // Create authorization widget
    m_authorizationWidget = new AuthorizationWidget(this);
    m_stackedLayout->addWidget(m_authorizationWidget);

    // Create main menu widget
    m_mainMenuWidget = new MainMenuWidget(this);
    connect(m_mainMenuWidget, &MainMenuWidget::callAccepted, this, &MainWindow::onIncomingCallAccepted);
    connect(m_mainMenuWidget, &MainMenuWidget::callDeclined, this, &MainWindow::onIncomingCallDeclined);
    m_stackedLayout->addWidget(m_mainMenuWidget);

    // Create call widget
    m_callWidget = new CallWidget(this);
    connect(m_callWidget, &CallWidget::hangupClicked, this, &MainWindow::onHangupClicked);
    connect(m_callWidget, &CallWidget::muteMicClicked, this, &MainWindow::onMuteMicClicked);
    connect(m_callWidget, &CallWidget::muteSpeakerClicked, this, &MainWindow::onMuteSpeakerClicked);
    m_stackedLayout->addWidget(m_callWidget);

    // Set authorization widget as default
    switchToAuthorizationWidget();
}

void MainWindow::switchToAuthorizationWidget() {
    m_stackedLayout->setCurrentWidget(m_authorizationWidget);
    setWindowTitle("Authorization - Callifornia");
}

void MainWindow::switchToMainMenuWidget() {
    m_stackedLayout->setCurrentWidget(m_mainMenuWidget);
    setWindowTitle("Callifornia");

    // Set nickname from calls client
    std::string nickname = calls::getNickname();
    if (!nickname.empty()) {
        m_mainMenuWidget->setNickname(QString::fromStdString(nickname));
    }
}

void MainWindow::switchToCallWidget(const QString& friendNickname) {
    m_stackedLayout->setCurrentWidget(m_callWidget);
    setWindowTitle("Call in Progress - Callifornia");
    m_callWidget->setCallInfo(friendNickname);
}

void MainWindow::onIncomingCallAccepted(const QString& friendNickname) {
    calls::acceptCall(friendNickname.toStdString());
    switchToCallWidget(friendNickname);
}

void MainWindow::onIncomingCallDeclined(const QString& friendNickname) {
    calls::declineCall(friendNickname.toStdString());
}

void MainWindow::onHangupClicked() {
    calls::endCall();
    switchToMainMenuWidget();
    m_mainMenuWidget->clearIncomingCalls();
}

void MainWindow::onMuteMicClicked() {
    // Implement microphone mute/unmute logic
    qDebug() << "Microphone mute toggled";
}

void MainWindow::onMuteSpeakerClicked() {
    // Implement speaker mute/unmute logic
    qDebug() << "Speaker mute toggled";
}

void MainWindow::onAuthorizationResult(calls::Result authorizationResult) {
    if (authorizationResult == calls::Result::SUCCESS) {
        switchToMainMenuWidget();

        // Set real nickname from calls client
        std::string nickname = calls::getNickname();
        if (!nickname.empty()) {
            m_mainMenuWidget->setNickname(QString::fromStdString(nickname));
        }
    }
    else {
        // Show error in authorization widget
        QString errorMessage;
        switch (authorizationResult) {
        case calls::Result::FAIL:
            errorMessage = "Authorization failed";
            break;
        case calls::Result::TIMEOUT:
            errorMessage = "Authorization timeout";
            break;
        default:
            errorMessage = "Unknown error";
            break;
        }
        m_authorizationWidget->setErrorMessage(errorMessage);
        m_authorizationWidget->reset(); // Unlock button for retry
    }
}

void MainWindow::onCreateCallResult(calls::Result createCallResult) {
    // Handle call creation result
    if (createCallResult != calls::Result::SUCCESS) {
        // If call failed, switch back to main menu
        switchToMainMenuWidget();

        QString errorMessage;
        switch (createCallResult) {
        case calls::Result::FAIL:
            errorMessage = "Call failed";
            break;
        case calls::Result::TIMEOUT:
            errorMessage = "Call timeout";
            break;
        default:
            errorMessage = "Call error";
            break;
        }
        // You might want to show this error in the main menu
        qDebug() << "Call creation failed:" << errorMessage;
    }
}

void MainWindow::onIncomingCall(const std::string& friendNickName) {
    // Handle incoming call - add to main menu
    m_mainMenuWidget->addIncomingCall(QString::fromStdString(friendNickName));
}

void MainWindow::onIncomingCallExpired(const std::string& friendNickName) {
    // Handle expired incoming call - remove from main menu
    m_mainMenuWidget->removeIncomingCall(QString::fromStdString(friendNickName));
}

void MainWindow::onCallHangUp() {
    // Handle call hang up from remote side
    switchToMainMenuWidget();
    m_mainMenuWidget->clearIncomingCalls();
}

void MainWindow::onNetworkError() {
    // Handle network error
    switchToAuthorizationWidget();
    m_authorizationWidget->setErrorMessage("Network error occurred");
}