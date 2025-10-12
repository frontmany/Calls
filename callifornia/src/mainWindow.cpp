#include "mainWindow.h"
#include <QStackedLayout>
#include <QHBoxLayout>
#include <QFontDatabase>
#include <QApplication>
#include <QSoundEffect>
#include <QFileInfo>

#include "AuthorizationWidget.h"
#include "MainMenuWidget.h"
#include "CallWidget.h"

MainWindow::MainWindow(QWidget* parent, const std::string& host, const std::string& port)
    : QMainWindow(parent)
{
    setWindowIcon(QIcon(":/resources/callifornia.ico"));

    m_ringtonePlayer = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);
    m_ringtonePlayer->setAudioOutput(m_audioOutput);
    m_audioOutput->setVolume(0.4f);
    m_ringtonePlayer->setSource(QUrl("qrc:/resources/ringtone.mp3"));

    connect(m_ringtonePlayer, &QMediaPlayer::playbackStateChanged, this, [this](QMediaPlayer::PlaybackState state) {
        if (state == QMediaPlayer::StoppedState || state == QMediaPlayer::PausedState) {
            if (calls::getIncomingCallsCount() != 0) {
                m_ringtonePlayer->play();
            }
        }
    });

    loadFonts();
    setupUI();

    std::unique_ptr<CallsClientHandler> callsClientHandler = std::make_unique<CallsClientHandler>(this);

    calls::init(host, port, std::move(callsClientHandler));
    calls::run();
}

MainWindow::~MainWindow() {
    if (m_ringtonePlayer) {
        m_ringtonePlayer->stop();
    }
}

void MainWindow::playRingtone() {
    if (!m_ringtonePlayer) return;

    if (m_ringtonePlayer->playbackState() != QMediaPlayer::PlayingState) {
        m_ringtonePlayer->setLoops(QMediaPlayer::Infinite);
        m_ringtonePlayer->play();
    }
}

void MainWindow::pauseRingtone() {
    if (!m_ringtonePlayer) return;

    if (m_ringtonePlayer->playbackState() == QMediaPlayer::PlayingState) {
        m_ringtonePlayer->pause();
    }
}

void MainWindow::playSoundEffect(const QString& soundPath) {
    QSoundEffect* effect = new QSoundEffect(this);
    effect->setSource(QUrl::fromLocalFile((soundPath)));
    effect->setVolume(1.0f);
    effect->play();

    connect(effect, &QSoundEffect::playingChanged, this, [effect]() {
        if (!effect->isPlaying()) {
            effect->deleteLater();
        }
    });
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
    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);

    m_mainLayout = new QHBoxLayout(m_centralWidget);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    m_stackedLayout = new QStackedLayout();
    m_mainLayout->addLayout(m_stackedLayout);

    m_authorizationWidget = new AuthorizationWidget(this);
    connect(m_authorizationWidget, &AuthorizationWidget::authorizationButtonClicked, this, &MainWindow::onAuthorizationButtonClicked);
    connect(m_authorizationWidget, &AuthorizationWidget::blurAnimationFinished, this, &MainWindow::onBlurAnimationFinished);

    m_stackedLayout->addWidget(m_authorizationWidget);

    m_mainMenuWidget = new MainMenuWidget(this);
    connect(m_mainMenuWidget, &MainMenuWidget::acceptCallButtonClicked, this, &MainWindow::onAcceptCallButtonClicked);
    connect(m_mainMenuWidget, &MainMenuWidget::declineCallButtonClicked, this, &MainWindow::onDeclineCallButtonClicked);
    connect(m_mainMenuWidget, &MainMenuWidget::createCallButtonClicked, this, &MainWindow::onStartCallingButtonClicked);
    connect(m_mainMenuWidget, &MainMenuWidget::stopCallingButtonClicked, this, &MainWindow::onStopCallingButtonClicked);
    connect(m_mainMenuWidget, &MainMenuWidget::refreshAudioDevicesButtonClicked, this, &MainWindow::onRefreshAudioDevicesButtonClicked);
    connect(m_mainMenuWidget, &MainMenuWidget::inputVolumeChanged, this, &MainWindow::onInputVolumeChanged);
    connect(m_mainMenuWidget, &MainMenuWidget::outputVolumeChanged, this, &MainWindow::onOutputVolumeChanged);
    connect(m_mainMenuWidget, &MainMenuWidget::muteButtonClicked, this, &MainWindow::onMuteButtonClicked);
    m_stackedLayout->addWidget(m_mainMenuWidget);

    m_callWidget = new CallWidget(this);
    connect(m_callWidget, &CallWidget::hangupClicked, this, &MainWindow::onEndCallButtonClicked);
    connect(m_callWidget, &CallWidget::refreshAudioDevicesButtonClicked, this, &MainWindow::onRefreshAudioDevicesButtonClicked);
    connect(m_callWidget, &CallWidget::inputVolumeChanged, this, &MainWindow::onInputVolumeChanged);
    connect(m_callWidget, &CallWidget::outputVolumeChanged, this, &MainWindow::onOutputVolumeChanged);
    connect(m_callWidget, &CallWidget::muteButtonClicked, this, &MainWindow::onMuteButtonClicked);
    connect(m_callWidget, &CallWidget::acceptCallButtonClicked, this, &MainWindow::onAcceptCallButtonClicked);
    connect(m_callWidget, &CallWidget::declineCallButtonClicked, this, &MainWindow::onDeclineCallButtonClicked);
    m_stackedLayout->addWidget(m_callWidget);

    switchToAuthorizationWidget();
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (calls::getState() == calls::State::UNAUTHORIZED) {
        event->accept();
    }
    else {
        calls::initiateShutdown();
        event->ignore();
    }
}


//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------

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

//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------

void MainWindow::onBlurAnimationFinished() {
    if (calls::getState() != calls::State::UNAUTHORIZED) {
        switchToMainMenuWidget();

        m_mainMenuWidget->setState(calls::State::FREE);

        std::string nickname = calls::getNickname();
        if (!nickname.empty()) {
            m_mainMenuWidget->setNickname(QString::fromStdString(nickname));
        }

        m_mainMenuWidget->setFocusToLineEdit();
    }
    else {
        m_authorizationWidget->resetBlur();
        QString errorMessage = "Authorization failed";
        m_authorizationWidget->setErrorMessage(errorMessage);
    }
}

void MainWindow::onAuthorizationButtonClicked(const QString& friendNickname) {
    calls::authorize(friendNickname.toStdString());
    m_authorizationWidget->startBlurAnimation();
}

void MainWindow::onEndCallButtonClicked() {
    calls::endCall();
}

void MainWindow::onRefreshAudioDevicesButtonClicked() {
    calls::refreshAudioDevices();
}

void MainWindow::onInputVolumeChanged(int newVolume) {
    calls::setInputVolume(newVolume);
}

void MainWindow::onOutputVolumeChanged(int newVolume) {
    calls::setOutputVolume(newVolume);
}

void MainWindow::onMuteButtonClicked(bool mute) {
    calls::mute(mute);
}

void MainWindow::onAcceptCallButtonClicked(const QString& friendNickname) {
    calls::acceptCall(friendNickname.toStdString());
}

void MainWindow::onDeclineCallButtonClicked(const QString& friendNickname) {
    calls::declineCall(friendNickname.toStdString());
}

void MainWindow::onStartCallingButtonClicked(const QString& friendNickname) {
    if (!friendNickname.isEmpty() && friendNickname.toStdString() != calls::getNickname()) {
        calls::startCalling(friendNickname.toStdString());
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
    calls::stopCalling();
}

//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------

void MainWindow::onAuthorizationResult(bool success) {}

void MainWindow::onLogoutResult(bool success) {

}

void MainWindow::onShutdownResult(bool success) {
    if (success) {
        calls::completeShutdown();
        QCoreApplication::quit();
    }
    else {
        calls::forceShutdown();
        QCoreApplication::quit();
    }
}

void MainWindow::onStartCallingResult(bool success) {
    if (success) {
        m_mainMenuWidget->showCallingPanel(QString::fromStdString(calls::getNicknameWhomCalling()));
        m_mainMenuWidget->setState(calls::State::CALLING);
    }
    else {
        QString errorMessage = "cannot call";
        m_mainMenuWidget->removeCallingPanel();
        m_mainMenuWidget->setState(calls::State::FREE);
        m_mainMenuWidget->setErrorMessage(errorMessage);
    }
}

void MainWindow::onCallingStoppedResult(bool success) {
    if (success) {
        m_mainMenuWidget->removeCallingPanel();
        m_mainMenuWidget->setState(calls::State::FREE);
    }
    else {
        // TODO
    }
}

void MainWindow::onDeclineIncomingCallResult(bool success, QString nickname) {
    if (success) {
        if (calls::getIncomingCallsCount() == 0) {
            pauseRingtone();
        }

        m_mainMenuWidget->removeIncomingCall(nickname);
        playSoundEffect(":/resources/callingEndedByMe.wav");
    }
    else {
        // TODO
    }
}

void MainWindow::onAcceptIncomingCallResult(bool success, QString nickname) {
    if (success) {
        m_mainMenuWidget->removeCallingPanel();

        m_mainMenuWidget->clearIncomingCalls();

        if (m_stackedLayout->currentWidget() == m_callWidget)
            m_callWidget->clearIncomingCalls();


        switchToCallWidget(nickname);
        pauseRingtone();
    }
    else {
        // TODO
    }
}

void MainWindow::onAllIncomingCallsDeclinedResult(bool success) {
    if (success) {
        m_mainMenuWidget->clearIncomingCalls();

        if (m_stackedLayout->currentWidget() == m_callWidget)
            m_callWidget->clearIncomingCalls();
    }
    else {
        // TODO
    }
}

void MainWindow::onEndCallResult(bool success) {
    if (success) {
        m_mainMenuWidget->setState(calls::State::FREE);
        switchToMainMenuWidget();
        m_callWidget->clearIncomingCalls();
        playSoundEffect(":/resources/callEnd.wav");
    }
    else {
        // TODO
    }
}

void MainWindow::onCallingAccepted() {
    calls::declineAllCalls();

    pauseRingtone();
    m_mainMenuWidget->removeCallingPanel();
    m_mainMenuWidget->setState(calls::State::BUSY);
    switchToCallWidget(QString::fromStdString(calls::getNicknameInCallWith()));
}

void MainWindow::onCallingDeclined() {
    m_mainMenuWidget->removeCallingPanel();
    m_mainMenuWidget->setState(calls::State::FREE);
}

void MainWindow::onRemoteUserEndedCall() {
    switchToMainMenuWidget();
    m_mainMenuWidget->setState(calls::State::FREE);
    playSoundEffect(":/resources/callEnd.wav");
}

void MainWindow::onIncomingCall(const std::string& friendNickName) {
    playRingtone();

    m_mainMenuWidget->addIncomingCall(QString::fromStdString(friendNickName));

    if (calls::getState() == calls::State::BUSY) {
        m_callWidget->addIncomingCall(QString::fromStdString(friendNickName));
    }
}

void MainWindow::onIncomingCallEnded(const std::string& friendNickName) {
    m_mainMenuWidget->removeIncomingCall(QString::fromStdString(friendNickName));

    if (calls::getState() == calls::State::BUSY) {
        m_callWidget->removeIncomingCall(QString::fromStdString(friendNickName));
    }

    if (calls::getIncomingCallsCount() == 0) {
        pauseRingtone();
    }


    playSoundEffect(":/resources/callingEndedByRemote.wav");
}

void MainWindow::onNetworkError() {
    calls::reset();

    pauseRingtone();
    m_mainMenuWidget->removeCallingPanel();
    m_mainMenuWidget->clearIncomingCalls();

    switchToAuthorizationWidget();
    m_authorizationWidget->resetBlur();
    m_authorizationWidget->setErrorMessage("Network error occurred");
}