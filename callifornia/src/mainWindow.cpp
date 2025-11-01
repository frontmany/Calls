#include "mainWindow.h"
#include <QStackedLayout>
#include <QHBoxLayout>
#include <QFontDatabase>
#include <QApplication>
#include <QSoundEffect>
#include <QFileInfo>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

#include "authorizationWidget.h"
#include "mainMenuWidget.h"
#include "overlayWidget.h"
#include "callWidget.h"
#include "dialogsController.h"

#include "clientCallbacksHandler.h"
#include "updaterCallbacksHandler.h"

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
}

MainWindow::~MainWindow() {
    if (m_ringtonePlayer) {
        m_ringtonePlayer->stop();
    }
}

void MainWindow::init() {
    setWindowIcon(QIcon(":/resources/callifornia.ico"));

    m_ringtonePlayer = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);
    m_audioOutput->setVolume(0.4f);
    m_ringtonePlayer->setAudioOutput(m_audioOutput);

    connect(m_ringtonePlayer, &QMediaPlayer::playbackStateChanged, [this](QMediaPlayer::PlaybackState state) {
        if (state == QMediaPlayer::StoppedState) {
            if (calls::getIncomingCallsCount() != 0) {
                m_ringtonePlayer->play();
            }
        }
    });

    loadFonts();
    setupUI();
    showMaximized();
}

void MainWindow::connectCallifornia(const std::string& host, const std::string& port) {
    std::unique_ptr<UpdaterCallbacksHandler> updaterHandler = std::make_unique<UpdaterCallbacksHandler>(this);
    updater::init(std::move(updaterHandler));
    updater::connect(host, port);
    updater::checkUpdates(parseVersionFromConfig());

    std::unique_ptr<ClientCallbacksHandler> callsClientHandler = std::make_unique<ClientCallbacksHandler>(this);
    calls::init(host, port, std::move(callsClientHandler));
    calls::run();
}

void MainWindow::playRingtone(const QUrl& ringtoneUrl) {
    if (!m_ringtonePlayer) return;

    if (m_ringtonePlayer->playbackState() == QMediaPlayer::PlayingState) {
        m_ringtonePlayer->stop();
    }

    m_ringtonePlayer->setSource(ringtoneUrl);

    if (m_ringtonePlayer->playbackState() != QMediaPlayer::PlayingState) {
        m_ringtonePlayer->setLoops(QMediaPlayer::Infinite);
        m_ringtonePlayer->play();
    }
}

void MainWindow::stopRingtone() {
    if (!m_ringtonePlayer) return;

    if (m_ringtonePlayer->playbackState() == QMediaPlayer::PlayingState) {
        m_ringtonePlayer->stop();
    }
}

void MainWindow::playIncomingCallRingtone() {
    playRingtone(QUrl("qrc:/resources/incomingCallRingtone.mp3"));
}

void MainWindow::stopIncomingCallRingtone() {
    stopRingtone();
}

void MainWindow::playCallingRingtone() {
    playRingtone(QUrl("qrc:/resources/calling.mp3"));
}

void MainWindow::stopCallingRingtone() {
    stopRingtone();
}

void MainWindow::playSoundEffect(const QString& soundPath) {
    QSoundEffect* effect = new QSoundEffect(this);
    effect->setSource(QUrl::fromLocalFile(soundPath));
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

std::string MainWindow::parseVersionFromConfig() {
    const QString filename = "config.json";

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        return "versionLost";
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (doc.isNull()) return "";

    QJsonObject json = doc.object();
    std::string version = json[calls::VERSION].toString().toStdString();

    return version;
}

updater::OperationSystemType MainWindow::resolveOperationSystemType() {
#if defined(Q_OS_WINDOWS)
    return updater::OperationSystemType::WINDOWS;
#elif defined(Q_OS_LINUX)
    return updater::OperationSystemType::LINUX;
#elif defined(Q_OS_MACOS)
    return OperationSystemType::MAC;
#else
#if defined(_WIN32)
    return updater::OperationSystemType::WINDOWS;
#elif defined(__linux__)
    return updater::OperationSystemType::LINUX;
#elif defined(__APPLE__)
    return updater::OperationSystemType::MAC;
#else
    qWarning() << "Unknown operating system";
    return updater::OperationSystemType::WINDOWS;
#endif
#endif
}

void MainWindow::onUpdaterCheckResult(updater::UpdatesCheckResult checkResult) {
    m_authorizationWidget->hideUpdatesCheckingNotification();

    if (checkResult == updater::UpdatesCheckResult::POSSIBLE_UPDATE) {
        if (!calls::isNetworkError()) {
            m_authorizationWidget->setAuthorizationDisabled(false);
        }

        if (calls::isNetworkError()) {
            m_authorizationWidget->hideNetworkErrorNotification();
        }

        m_authorizationWidget->showUpdateAvailableNotification();
        m_mainMenuWidget->showUpdateAvailableNotification();
    }
    else if (checkResult == updater::UpdatesCheckResult::REQUIRED_UPDATE) {
        updater::startUpdate(resolveOperationSystemType());
        m_dialogsController->showUpdatingDialog();
    }
    else if (checkResult == updater::UpdatesCheckResult::UPDATE_NOT_NEEDED) {
        updater::disconnect();

        if (!calls::isNetworkError()) {
            m_authorizationWidget->setAuthorizationDisabled(false);
        }
    }
    else {
        qWarning() << "error: unknown UpdatesCheckResult type";
    }
}


void MainWindow::onUpdateButtonClicked() {
    if (calls::isCalling()) {
        calls::stopCalling();
    }

    auto callers = calls::getCallers();
    for (const auto& nickname : callers) {
        calls::declineCall(nickname);
    }

    stopRingtone();
    m_mainMenuWidget->removeCallingPanel();
    m_mainMenuWidget->clearIncomingCalls();

    updater::startUpdate(resolveOperationSystemType());
    m_dialogsController->showUpdatingDialog();
}

void MainWindow::onUpdateLoaded(bool emptyUpdate)
{
    if (!emptyUpdate) {
        calls::logout();
        m_dialogsController->swapUpdatingToRestarting();
        qApp->processEvents();

        QTimer::singleShot(1500, [this]() {
                launchUpdateApplier();
        });
    }
    else {
        m_dialogsController->hideUpdatingDialog();
    }

    
}

void MainWindow::launchUpdateApplier() {
    qint64 currentPid = QCoreApplication::applicationPid();
    QString appPath = QCoreApplication::applicationFilePath();

    QString updateApplierName;
#ifdef Q_OS_WIN
    updateApplierName = "update_applier.exe";
#else
    updateApplierName = "update_applier";
#endif

    QFileInfo updateSupplierFile(updateApplierName);
    if (!updateSupplierFile.exists()) {
        qWarning() << "Update applier not found:" << updateApplierName;
        return;
    }

    QStringList arguments;
    arguments << QString::number(currentPid) << appPath;

    QProcess::startDetached(updateApplierName, arguments);
}

void MainWindow::onLoadingProgress(double progress)
{
    m_dialogsController->updateLoadingProgress(progress);
}

void MainWindow::setupUI() {
    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);

    m_mainLayout = new QHBoxLayout(m_centralWidget);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    m_stackedLayout = new QStackedLayout();
    m_mainLayout->addLayout(m_stackedLayout);

    m_dialogsController = new DialogsController(this);
    connect(m_dialogsController, &DialogsController::exitButtonClicked, this, &MainWindow::close);

    m_authorizationWidget = new AuthorizationWidget(this);
    connect(m_authorizationWidget, &AuthorizationWidget::updateButtonClicked, this, &MainWindow::onUpdateButtonClicked);
    connect(m_authorizationWidget, &AuthorizationWidget::authorizationButtonClicked, this, &MainWindow::onAuthorizationButtonClicked);
    connect(m_authorizationWidget, &AuthorizationWidget::blurAnimationFinished, this, &MainWindow::onBlurAnimationFinished);

    m_stackedLayout->addWidget(m_authorizationWidget);

    m_mainMenuWidget = new MainMenuWidget(this);
    connect(m_mainMenuWidget, &MainMenuWidget::updateButtonClicked, this, &MainWindow::onUpdateButtonClicked);
    connect(m_mainMenuWidget, &MainMenuWidget::acceptCallButtonClicked, this, &MainWindow::onAcceptCallButtonClicked);
    connect(m_mainMenuWidget, &MainMenuWidget::declineCallButtonClicked, this, &MainWindow::onDeclineCallButtonClicked);
    connect(m_mainMenuWidget, &MainMenuWidget::startCallingButtonClicked, this, &MainWindow::onStartCallingButtonClicked);
    connect(m_mainMenuWidget, &MainMenuWidget::stopCallingButtonClicked, this, &MainWindow::onStopCallingButtonClicked);
    connect(m_mainMenuWidget, &MainMenuWidget::refreshAudioDevicesButtonClicked, this, &MainWindow::onRefreshAudioDevicesButtonClicked);
    connect(m_mainMenuWidget, &MainMenuWidget::inputVolumeChanged, this, &MainWindow::onInputVolumeChanged);
    connect(m_mainMenuWidget, &MainMenuWidget::outputVolumeChanged, this, &MainWindow::onOutputVolumeChanged);
    connect(m_mainMenuWidget, &MainMenuWidget::muteMicrophoneClicked, this, &MainWindow::onMuteMicrophoneButtonClicked);
    connect(m_mainMenuWidget, &MainMenuWidget::muteSpeakerClicked, this, &MainWindow::onMuteSpeakerButtonClicked);
    m_stackedLayout->addWidget(m_mainMenuWidget); 

    m_callWidget = new CallWidget(this);
    connect(m_callWidget, &CallWidget::hangupClicked, this, &MainWindow::onEndCallButtonClicked);
    connect(m_callWidget, &CallWidget::inputVolumeChanged, this, &MainWindow::onInputVolumeChanged);
    connect(m_callWidget, &CallWidget::outputVolumeChanged, this, &MainWindow::onOutputVolumeChanged);
    connect(m_callWidget, &CallWidget::muteSpeakerClicked, this, &MainWindow::onMuteSpeakerButtonClicked);
    connect(m_callWidget, &CallWidget::muteMicrophoneClicked, this, &MainWindow::onMuteMicrophoneButtonClicked);
    connect(m_callWidget, &CallWidget::acceptCallButtonClicked, this, &MainWindow::onAcceptCallButtonClicked);
    connect(m_callWidget, &CallWidget::declineCallButtonClicked, this, &MainWindow::onDeclineCallButtonClicked);
    m_stackedLayout->addWidget(m_callWidget);

    m_authorizationWidget->showUpdatesCheckingNotification();
    m_authorizationWidget->setAuthorizationDisabled(true);

    switchToAuthorizationWidget();
}

void MainWindow::closeEvent(QCloseEvent* event) {
    calls::stop();
    updater::disconnect();
    event->accept();
}

void MainWindow::switchToAuthorizationWidget() {
    m_stackedLayout->setCurrentWidget(m_authorizationWidget);
    setWindowTitle("Authorization - Callifornia");
}

void MainWindow::switchToMainMenuWidget() {
    m_stackedLayout->setCurrentWidget(m_mainMenuWidget);
    m_mainMenuWidget->setInputVolume(calls::getInputVolume());
    m_mainMenuWidget->setOutputVolume(calls::getOutputVolume());
    m_mainMenuWidget->setMicrophoneMuted(calls::isMicrophoneMuted());
    m_mainMenuWidget->setSpeakerMuted(calls::isSpeakerMuted());

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
    m_callWidget->setMicrophoneMuted(calls::isMicrophoneMuted());
    m_callWidget->setSpeakerMuted(calls::isSpeakerMuted());

    setWindowTitle("Call In Progress - Callifornia");
    m_callWidget->setCallInfo(friendNickname);
}

void MainWindow::onBlurAnimationFinished() {
    if (calls::isAuthorized()) {
        m_authorizationWidget->resetBlur();
        m_authorizationWidget->clearErrorMessage();

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
    }
}

void MainWindow::onAuthorizationButtonClicked(const QString& friendNickname) {
    bool requestSent = calls::authorize(friendNickname.toStdString());
    if (requestSent) {
        m_authorizationWidget->startBlurAnimation();
    }
    else {
        m_authorizationWidget->resetBlur();
        QString errorMessage = "Failed to send authorization request. Please try again.";
        m_authorizationWidget->setErrorMessage(errorMessage);
    }
}

void MainWindow::onEndCallButtonClicked() {
    bool requestSent = calls::endCall();
    if (!requestSent) {
        handleEndCallErrorNotificationAppearance();
        return;
    }

    m_callWidget->clearIncomingCalls();
    m_mainMenuWidget->setState(calls::State::FREE);
    switchToMainMenuWidget();
    playSoundEffect(":/resources/endCall.wav");
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

void MainWindow::onMuteMicrophoneButtonClicked(bool mute) {
    calls::muteMicrophone(mute);
}

void MainWindow::onMuteSpeakerButtonClicked(bool mute) {
    calls::muteSpeaker(mute);
}

void MainWindow::onAcceptCallButtonClicked(const QString& friendNickname) {
    bool requestSent = calls::acceptCall(friendNickname.toStdString());
    if (!requestSent) {
        handleAcceptCallErrorNotificationAppearance();
        return;
    }

    playSoundEffect(":/resources/callJoined.wav");
}

void MainWindow::onDeclineCallButtonClicked(const QString& friendNickname) {
    bool requestSent = calls::declineCall(friendNickname.toStdString());
    if (!requestSent) {
        handleDeclineCallErrorNotificationAppearance();
    }
    else {
        if (calls::getIncomingCallsCount() == 0) {
            stopRingtone();
        }

        if (m_stackedLayout->currentWidget() == m_callWidget)
            m_callWidget->removeIncomingCall(friendNickname);

        m_mainMenuWidget->removeIncomingCall(friendNickname);
        playSoundEffect(":/resources/callingEnded.wav");
    }
}

void MainWindow::onStartCallingButtonClicked(const QString& friendNickname) {
    if (!friendNickname.isEmpty() && friendNickname.toStdString() != calls::getNickname()) {
        bool requestSent = calls::startCalling(friendNickname.toStdString());
        if (!requestSent) {
            handleStartCallingErrorNotificationAppearance();
            return;
        }
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
    bool requestSent = calls::stopCalling();
    if (!requestSent) {
        handleStopCallingErrorNotificationAppearance();
    }
    else {
        m_mainMenuWidget->removeCallingPanel();
        m_mainMenuWidget->setState(calls::State::FREE);
        stopRingtone();
        playSoundEffect(":/resources/callingEnded.wav");
    }
}

void MainWindow::handleAcceptCallErrorNotificationAppearance() {
    QString errorText = "Failed to accept call";

    if (m_stackedLayout->currentWidget() == m_mainMenuWidget) {
        m_mainMenuWidget->showErrorNotification(errorText, 1500);
    }
    else if (m_stackedLayout->currentWidget() == m_callWidget) {
        m_callWidget->showErrorNotification(errorText, 1500);
    }
    else {
        DEBUG_LOG("Trying to accept call from weird widget");
    }
}

void MainWindow::handleDeclineCallErrorNotificationAppearance() {
    QString errorText = "Failed to decline call. Please try again";

    if (m_stackedLayout->currentWidget() == m_mainMenuWidget) {
        m_mainMenuWidget->showErrorNotification(errorText, 1500);
    }
    else if (m_stackedLayout->currentWidget() == m_callWidget) {
        m_callWidget->showErrorNotification(errorText, 1500);
    }
    else {
        DEBUG_LOG("Trying to decline call from weird widget");
    }
}

void MainWindow::handleStartCallingErrorNotificationAppearance() {
    QString errorText = "Failed to start calling. Please try again";

    if (m_stackedLayout->currentWidget() == m_mainMenuWidget) {
        m_mainMenuWidget->showErrorNotification(errorText, 1500);
    }
    else {
        DEBUG_LOG("Trying to start calling from weird widget");
    }
}

void MainWindow::handleStopCallingErrorNotificationAppearance() {
    QString errorText = "Failed to stop calling. Please try again";

    if (m_stackedLayout->currentWidget() == m_mainMenuWidget) {
        m_mainMenuWidget->showErrorNotification(errorText, 1500);
    }
    else {
        DEBUG_LOG("Trying to stop calling from weird widget");
    }
}

void MainWindow::handleEndCallErrorNotificationAppearance() {
    QString errorText = "Failed to end call. Please try again";

    if (m_stackedLayout->currentWidget() == m_callWidget) {
        m_callWidget->showErrorNotification(errorText, 1500);
    }
    else {
        DEBUG_LOG("Trying to end call from weird widget");
    }
}

void MainWindow::onAuthorizationResult(calls::ErrorCode ec) {
    QString errorMessage;

    if (ec == calls::ErrorCode::OK) {
        return;
    }
    else if (ec == calls::ErrorCode::TAKEN_NICKNAME) {
        errorMessage = "Taken nickname";
    }
    else if (ec == calls::ErrorCode::TIMEOUT) {
        errorMessage = "Timeout (please try again)";
    }

    m_authorizationWidget->setErrorMessage(errorMessage);
}

void MainWindow::onStartCallingResult(calls::ErrorCode ec) {
    QString errorMessage;

    if (ec == calls::ErrorCode::OK) {
        m_mainMenuWidget->showCallingPanel(QString::fromStdString(calls::getNicknameWhomCalling()));
        m_mainMenuWidget->setState(calls::State::CALLING);
        playCallingRingtone();
        return;
    }
    else if (ec == calls::ErrorCode::UNEXISTING_USER) {
        errorMessage = "User not found";
    }
    else if (ec == calls::ErrorCode::TIMEOUT) {
        errorMessage = "Timeout (please try again)";
    }

    m_mainMenuWidget->setErrorMessage(errorMessage);
}

void MainWindow::onAcceptCallResult(calls::ErrorCode ec, const QString& nickname) {
    if (ec == calls::ErrorCode::OK) {
        m_mainMenuWidget->removeCallingPanel();

        m_mainMenuWidget->clearIncomingCalls();

        if (m_stackedLayout->currentWidget() == m_callWidget)
            m_callWidget->clearIncomingCalls();

        switchToCallWidget(nickname);
        stopRingtone();
    }
    else {
        m_mainMenuWidget->removeIncomingCall(nickname);

        if (m_stackedLayout->currentWidget() == m_callWidget)
            m_callWidget->removeIncomingCall(nickname);

        handleAcceptCallErrorNotificationAppearance();
    }
}

void MainWindow::onMaximumCallingTimeReached() {
    m_mainMenuWidget->removeCallingPanel();
    m_mainMenuWidget->setState(calls::State::FREE);
    stopRingtone();
}

void MainWindow::onCallingAccepted() {
    m_mainMenuWidget->clearIncomingCalls();

    stopRingtone();
    m_mainMenuWidget->removeCallingPanel();
    m_mainMenuWidget->setState(calls::State::BUSY);
    switchToCallWidget(QString::fromStdString(calls::getNicknameInCallWith()));
    playSoundEffect(":/resources/callJoined.wav");
}

void MainWindow::onCallingDeclined() {
    m_mainMenuWidget->removeCallingPanel();
    m_mainMenuWidget->setState(calls::State::FREE);
    stopRingtone();
    playSoundEffect(":/resources/callingEnded.wav");
}

void MainWindow::onRemoteUserEndedCall() {
    m_callWidget->clearIncomingCalls();
    switchToMainMenuWidget();

    m_mainMenuWidget->setState(calls::State::FREE);
    playSoundEffect(":/resources/endCall.wav");
}

void MainWindow::onIncomingCall(const QString& friendNickName) {
    playIncomingCallRingtone();

    m_mainMenuWidget->addIncomingCall(friendNickName);

    if (m_stackedLayout->currentWidget() == m_callWidget) 
        m_callWidget->addIncomingCall(friendNickName);
}

void MainWindow::onIncomingCallExpired(const QString& friendNickName) {
    m_mainMenuWidget->removeIncomingCall(friendNickName);

    if (m_stackedLayout->currentWidget() == m_callWidget)
        m_callWidget->removeIncomingCall(friendNickName);

    if (calls::getIncomingCallsCount() == 0)
        stopRingtone();

    playSoundEffect(":/resources/callingEnded.wav");
}

void MainWindow::onClientNetworkError() {
    stopRingtone();

    m_mainMenuWidget->removeCallingPanel();
    m_mainMenuWidget->clearIncomingCalls();

    if (m_stackedLayout->currentWidget() != m_authorizationWidget) {
        switchToAuthorizationWidget();
    }

    m_authorizationWidget->resetBlur();
    m_authorizationWidget->setAuthorizationDisabled(true);

    if (!updater::isAwaitingServerResponse())
        m_authorizationWidget->showNetworkErrorNotification();
}

void MainWindow::onUpdaterNetworkError() {
    m_authorizationWidget->hideUpdateAvailableNotification();
    m_mainMenuWidget->hideUpdateAvailableNotification();

    m_dialogsController->hideUpdatingDialog();
    if (!calls::isAuthorized() || updater::isAwaitingServerResponse()) {
        m_dialogsController->showConnectionErrorDialog();
    }

    updater::disconnect();
}

void MainWindow::onConnectionRestored() {
    m_authorizationWidget->resetBlur();
    m_authorizationWidget->setAuthorizationDisabled(false);
    m_authorizationWidget->showConnectionRestoredNotification(1500);

    if (!updater::isConnected()) {
        updater::connect(updater::getServerHost(), updater::getServerPort());
        updater::checkUpdates(parseVersionFromConfig());
    }
}
