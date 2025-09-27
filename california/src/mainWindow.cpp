#include "mainWindow.h"
#include <QStackedLayout>
#include <QHBoxLayout>
#include <QFontDatabase>

#include "AuthorizationWidget.h"
#include "MainMenuWidget.h"
#include "CallWidget.h"

MainWindow::MainWindow(QWidget* parent, const std::string& host, const std::string& port)
    : QMainWindow(parent) {
    setWindowIcon(QIcon(":/resources/callifornia.ico"));

    calls::init(host, port,
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

    calls::run();

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
    connect(m_authorizationWidget, &AuthorizationWidget::authorizationButtonClicked, this, &MainWindow::onAuthorizationButtonClicked);
    connect(m_authorizationWidget, &AuthorizationWidget::blurAnimationFinished, [this]() {
        if (m_authorizationResult == calls::Result::EMPTY) {
            return;
        }
        
        if (m_authorizationResult == calls::Result::SUCCESS) {
            switchToMainMenuWidget();
            m_mainMenuWidget->setState(calls::State::FREE);

            // Set real nickname from calls client
            std::string nickname = calls::getNickname();
            if (!nickname.empty()) {
                m_mainMenuWidget->setNickname(QString::fromStdString(nickname));
            }

            m_mainMenuWidget->setFocusToLineEdit();
        }
        else {
            // Show error in authorization widget
            QString errorMessage;
            switch (m_authorizationResult) {
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

            m_authorizationWidget->resetBlur();
            m_authorizationWidget->setErrorMessage(errorMessage);
        }
    });
    m_stackedLayout->addWidget(m_authorizationWidget);

    // Create main menu widget
    m_mainMenuWidget = new MainMenuWidget(this);
    connect(m_mainMenuWidget, &MainMenuWidget::acceptCallButtonClicked, this, &MainWindow::onIncomingCallAccepted);
    connect(m_mainMenuWidget, &MainMenuWidget::declineCallButtonClicked, this, &MainWindow::onIncomingCallDeclined);
    connect(m_mainMenuWidget, &MainMenuWidget::createCallButtonClicked, this, &MainWindow::onCallButtonClicked);
    connect(m_mainMenuWidget, &MainMenuWidget::stopCallingButtonClicked, this, &MainWindow::onStopCallingButtonClicked);
    m_stackedLayout->addWidget(m_mainMenuWidget);

    // Create call widget
    m_callWidget = new CallWidget(this);
    connect(m_callWidget, &CallWidget::hangupClicked, this, &MainWindow::onHangupClicked);
    m_stackedLayout->addWidget(m_callWidget);

    // Set authorization widget as default
    switchToAuthorizationWidget();
}

void MainWindow::onAuthorizationButtonClicked(const QString& friendNickname) {
    calls::authorize(friendNickname.toStdString());
    m_authorizationWidget->startBlurAnimation();
}

void MainWindow::closeEvent(QCloseEvent* event) {
    calls::logout();
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
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
    m_authorizationResult = authorizationResult;
}

void MainWindow::onCreateCallResult(calls::Result createCallResult) {
    QString errorMessage;

    if (createCallResult == calls::Result::CALL_ACCEPTED) {
        m_mainMenuWidget->removeCallingPanel();
        m_mainMenuWidget->setState(calls::State::BUSY);
        switchToCallWidget(QString::fromStdString(calls::getNicknameWhomCalling()));
    }
    else if (createCallResult == calls::Result::CALLING) {
        m_mainMenuWidget->showCallingPanel(QString::fromStdString(calls::getNicknameWhomCalling()));
        m_mainMenuWidget->setState(calls::State::CALLING);
    }
    else if (createCallResult == calls::Result::CALL_DECLINED) {
        m_mainMenuWidget->removeCallingPanel();
        m_mainMenuWidget->setState(calls::State::FREE);
    }
    else if (createCallResult == calls::Result::TIMEOUT) {
        errorMessage = "Call timeout error";
    }
    else if (createCallResult == calls::Result::WRONG_FRIEND_NICKNAME) {
        errorMessage = "Unexisting user nickname";
    }
    else {
        errorMessage = "Call error";
    }

    if (!errorMessage.isEmpty()) {
        m_mainMenuWidget->removeCallingPanel();
        m_mainMenuWidget->setState(calls::State::FREE);
        m_mainMenuWidget->setErrorMessage(errorMessage);
    }
}

void MainWindow::onIncomingCall(const std::string& friendNickName) {
    // Handle incoming call - add to main menu
    m_mainMenuWidget->addIncomingCall(QString::fromStdString(friendNickName));
}

void MainWindow::onCallButtonClicked(const QString& friendNickname) {
    if (!friendNickname.isEmpty() && friendNickname.toStdString() != calls::getNickname()) {
        calls::createCall(friendNickname.toStdString());
    }
    else {
        if (friendNickname.isEmpty()) {
            m_mainMenuWidget->setErrorMessage("cannot be empty");
        }
        else if (friendNickname.toStdString() == calls::getNickname()) {
            m_mainMenuWidget->setErrorMessage("cannot call yourself");
        }
    }
}

void MainWindow::onStopCallingButtonClicked() {
    m_mainMenuWidget->removeCallingPanel();
    m_mainMenuWidget->setState(calls::State::FREE);
    calls::stopCalling();
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
    calls::logout();
    switchToAuthorizationWidget();
    m_authorizationWidget->resetBlur();
    m_authorizationWidget->setErrorMessage("Network error occurred");
}