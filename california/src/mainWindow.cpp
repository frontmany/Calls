#include "mainWindow.h"
#include <QStackedLayout>
#include <QHBoxLayout>
#include <QFontDatabase>

#include "AuthorizationWidget.h"
#include "MainMenuWidget.h"
#include "CallWidget.h"

MainWindow::MainWindow(QWidget* parent, const std::string& host, const std::string& port)
    : QMainWindow(parent) {

    calls::init(host, port,
        [this](calls::Result authorizationResult) {
            if (authorizationResult == calls::Result::SUCCESS) {
                m_authorized = true;
            }
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
        [this](const std::string& friendNickName) {
            QMetaObject::invokeMethod(this, "onSimultaneousCalling", Qt::QueuedConnection,
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
    connect(m_authorizationWidget, &AuthorizationWidget::animationFinished, [this]() {
        switchToMainMenuWidget(); 
        m_mainMenuWidget->setStatus(calls::ClientStatus::FREE);

         
        // Set real nickname from calls client
        std::string nickname = calls::getNickname();
        if (!nickname.empty()) {
            m_mainMenuWidget->setNickname(QString::fromStdString(nickname));
        }

        m_mainMenuWidget->setFocusToLineEdit();
    });

    m_stackedLayout->addWidget(m_authorizationWidget);

    // Create main menu widget
    m_mainMenuWidget = new MainMenuWidget(this);
    connect(m_mainMenuWidget, &MainMenuWidget::callAccepted, this, &MainWindow::onIncomingCallAccepted);
    m_stackedLayout->addWidget(m_mainMenuWidget);

    // Create call widget
    m_callWidget = new CallWidget(this);
    connect(m_callWidget, &CallWidget::hangupClicked, this, &MainWindow::onHangupClicked);
    m_stackedLayout->addWidget(m_callWidget);

    // Set authorization widget as default
    switchToAuthorizationWidget();
}

void MainWindow::closeEvent(QCloseEvent* event) {
    m_authorized = false;
    calls::stop();
}

void MainWindow::switchToAuthorizationWidget() {
    m_stackedLayout->setCurrentWidget(m_authorizationWidget);
    setWindowTitle("Authorization - Callifornia");
}

void MainWindow::switchToMainMenuWidget() {
    m_stackedLayout->setCurrentWidget(m_mainMenuWidget);
    m_mainMenuWidget->setInputVolume(calls::getInputVolume());
    m_mainMenuWidget->setOutputVolume(calls::getOutputVolume());
    m_mainMenuWidget->setMuted(calls::isMuted());

    setWindowTitle("Callifornia");

    // Set nickname from calls client
    std::string nickname = calls::getNickname();
    if (!nickname.empty()) {
        m_mainMenuWidget->setNickname(QString::fromStdString(nickname));
    }
}

void MainWindow::switchToCallWidget(const QString& friendNickname) {
    m_stackedLayout->setCurrentWidget(m_callWidget);

    m_callWidget->setInputVolume(calls::getInputVolume());
    m_callWidget->setOutputVolume(calls::getOutputVolume());
    m_callWidget->setMuted(calls::isMuted());

    setWindowTitle("Call In Progress - Callifornia");
    m_callWidget->setCallInfo(friendNickname);
}

void MainWindow::onIncomingCallAccepted(const QString& friendNickname) {
    calls::acceptCall(friendNickname.toStdString());
    switchToCallWidget(friendNickname);
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
        m_authorizationWidget->startBlurAnimation();
        m_authorized = true;
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

        if (!m_authorized) {
            m_authorizationWidget->reset();
            m_authorizationWidget->setErrorMessage(errorMessage);
        }
    }
}

void MainWindow::onCreateCallResult(calls::Result createCallResult) {
    if (createCallResult != calls::Result::SUCCESS) {
        QString errorMessage;
        switch (createCallResult) {
        case calls::Result::FAIL:
            errorMessage = "Call failed";
            break;
        case calls::Result::TIMEOUT:
            errorMessage = "Call timeout error";
            break;
        case calls::Result::WRONG_FRIEND_NICKNAME:
            errorMessage = "unexisting user nickname";
            break;
        default:
            errorMessage = "Call error";
            break;
        }
        
        m_mainMenuWidget->clearCallingInfo();
        QTimer::singleShot(400, this, [this, errorMessage]() {
            m_mainMenuWidget->setErrorMessage(errorMessage);
        });
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

void MainWindow::onSimultaneousCalling(const std::string& friendNickName) {
    calls::acceptCall(friendNickName);
    switchToCallWidget(QString::fromStdString(friendNickName));
}

void MainWindow::onCallHangUp() {
    // Handle call hang up from remote side
    switchToMainMenuWidget();
    m_mainMenuWidget->clearIncomingCalls();
}

void MainWindow::onNetworkError() {
    // Handle network error
    m_authorized = false;
    calls::logout();
    switchToAuthorizationWidget();
    m_authorizationWidget->reset();
    m_authorizationWidget->setErrorMessage("Network error occurred");
}