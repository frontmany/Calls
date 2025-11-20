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
#include <QStatusBar>
#include <QEvent>

#include "authorizationWidget.h"
#include "mainMenuWidget.h"
#include "overlayWidget.h"
#include "callWidget.h"
#include "dialogsController.h"
#include "screenCaptureController.h"

#include "clientCallbacksHandler.h"
#include "updaterCallbacksHandler.h"
#include "logger.h"

MainWindow::~MainWindow() {
    if (m_ringtonePlayer) {
        m_ringtonePlayer->stop();
    }
}

void MainWindow::init() {
    LOG_INFO("Initializing main window");
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
    LOG_INFO("Connecting to Callifornia server: {}:{}", host, port);
    
    std::unique_ptr<UpdaterCallbacksHandler> updaterHandler = std::make_unique<UpdaterCallbacksHandler>(this);
    updater::init(std::move(updaterHandler));
    updater::connect(host, port);
    
    std::string currentVersion = parseVersionFromConfig();
    LOG_INFO("Current application version: {}", currentVersion);
    updater::checkUpdates(currentVersion);

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

void MainWindow::stopLocalScreenCapture()
{
    if (!m_screenCaptureController) return;

    if (m_dialogsController)
    {
        m_dialogsController->hideScreenShareDialog();
    }
    m_screenCaptureController->stopCapture();
}

void MainWindow::showTransientStatusMessage(const QString& message, int durationMs)
{
    if (QStatusBar* bar = statusBar())
    {
        bar->showMessage(message, durationMs);
    }
}

void MainWindow::loadFonts() {
    if (QFontDatabase::addApplicationFont(":/resources/Pacifico-Regular.ttf") == -1) {
        LOG_ERROR("Failed to load font: Pacifico-Regular.ttf");
        qWarning() << "Font load error:";
    }

    if (QFontDatabase::addApplicationFont(":/resources/Outfit-VariableFont_wght.ttf") == -1) {
        LOG_ERROR("Failed to load font: Outfit-VariableFont_wght.ttf");
        qWarning() << "Font load error:";
    }
    else {
        LOG_DEBUG("Fonts loaded successfully");
    }
}

std::string MainWindow::parseVersionFromConfig() {
    const QString filename = "config.json";

    // ������� ������� ������� �������
    QString currentPath = QDir::currentPath();
    LOG_DEBUG("Current working directory: {}", currentPath.toStdString());

    // ������� ������ ���� � �����
    QFileInfo fileInfo(filename);
    QString absolutePath = fileInfo.absoluteFilePath();
    LOG_DEBUG("Absolute file path: {}", absolutePath.toStdString());

    // ��������� ������������� �����
    if (!fileInfo.exists()) {
        LOG_WARN("File does not exist: {}", absolutePath.toStdString());
        LOG_WARN("Failed to open config.json, version lost");
        return "versionLost";
    }

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        LOG_WARN("Failed to open config.json, version lost");
        LOG_WARN("File error: {}", file.errorString().toStdString());
        return "versionLost";
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (doc.isNull()) {
        LOG_ERROR("Failed to parse config.json");
        return "";
    }

    QJsonObject json = doc.object();
    std::string version = json[calls::VERSION].toString().toStdString();

    LOG_DEBUG("Parsed version from config: {}", version);
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
        m_dialogsController->swapUpdatingToUpToDate();
        updater::disconnect();

        QTimer::singleShot(1500, [this]() {
            m_dialogsController->hideUpdatingDialog();
            m_authorizationWidget->hideUpdatesCheckingNotification();
            m_authorizationWidget->setAuthorizationDisabled(false);
        });
    }
}

void MainWindow::launchUpdateApplier() {
    qint64 currentPid = QCoreApplication::applicationPid();

    QString appPath;
#ifdef Q_OS_LINUX
    QString appimagePath = qgetenv("APPIMAGE");
    if (!appimagePath.isEmpty()) {
        appPath = appimagePath;
         LOG_INFO("Running as AppImage");
    }
    else {
        LOG_INFO("Running not as as AppImage");
        appPath = QCoreApplication::applicationFilePath();
    }
#else
    appPath = QCoreApplication::applicationFilePath();
#endif

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

    QString workingDir = QDir::currentPath();
    QProcess::startDetached(updateApplierName, arguments, workingDir);
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
    connect(m_callWidget, &CallWidget::screenShareClicked, this, &MainWindow::onScreenShareButtonClicked);
    connect(m_callWidget, &CallWidget::requestEnterWindowFullscreen, this, &MainWindow::onCallWidgetEnterFullscreenRequested);
    connect(m_callWidget, &CallWidget::requestExitWindowFullscreen, this, &MainWindow::onCallWidgetExitFullscreenRequested);
    m_stackedLayout->addWidget(m_callWidget);

    m_screenCaptureController = new ScreenCaptureController(this);
    connect(m_screenCaptureController, &ScreenCaptureController::captureStarted, this, &MainWindow::onCaptureStarted);
    connect(m_screenCaptureController, &ScreenCaptureController::captureStopped, this, &MainWindow::onCaptureStopped);
    connect(m_screenCaptureController, &ScreenCaptureController::screenCaptured, this, &MainWindow::onScreenCaptured);

    m_dialogsController = new DialogsController(this);
    connect(m_dialogsController, &DialogsController::exitButtonClicked, this, &MainWindow::close);
    connect(m_dialogsController, &DialogsController::screenSelected, this, &MainWindow::onScreenSelected);

    QTimer::singleShot(2000, [this]() {
        if (updater::isAwaitingServerResponse()) {
            m_authorizationWidget->showUpdatesCheckingNotification();
            m_authorizationWidget->setAuthorizationDisabled(true);
        }
    });

    // Temporary: show CallWidget immediately for testing
    /*
    m_callWidget->setCallInfo("Test User");
    m_stackedLayout->setCurrentWidget(m_callWidget);
    setWindowTitle("Call In Progress - Callifornia");

    QTimer::singleShot(2000, [this]() {
        m_callWidget->addIncomingCall("Test User");
    });

    */
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

    m_callWidget->setShowingDisplayActive(false);
    m_callWidget->disableStartScreenShareButton(false);

    m_callWidget->setInputVolume(calls::getInputVolume());
    m_callWidget->setOutputVolume(calls::getOutputVolume());
    m_callWidget->setMicrophoneMuted(calls::isMicrophoneMuted());
    m_callWidget->setSpeakerMuted(calls::isSpeakerMuted());

    setWindowTitle("Call In Progress - Callifornia");
    m_callWidget->setCallInfo(friendNickname);
}

void MainWindow::onScreenShareButtonClicked(bool toggled) {
    if (toggled) {
        m_screenCaptureController->refreshAvailableScreens();
        m_screenCaptureController->resetSelectedScreenIndex();
        m_dialogsController->showScreenShareDialog(m_screenCaptureController->availableScreens());
    }
    else {
        stopLocalScreenCapture();
        m_callWidget->disableStartScreenShareButton(false);
        m_callWidget->setShowingDisplayActive(false);
    }
}


void MainWindow::onScreenSelected(int screenIndex)
{
    if (!m_screenCaptureController)
    {
        showTransientStatusMessage("Unable to start screen sharing", 2000);
        return;
    }

    m_screenCaptureController->refreshAvailableScreens();
    m_screenCaptureController->setSelectedScreenIndex(screenIndex);

    if (m_screenCaptureController->selectedScreenIndex() == -1)
    {
        showTransientStatusMessage("Selected screen is no longer available", 3000);
        return;
    }

    m_screenCaptureController->startCapture();
}












void MainWindow::onCaptureStarted()
{
    const std::string friendNickname = calls::getNicknameInCallWith();
    if (friendNickname.empty())
    {
        showTransientStatusMessage("No active call to share screen with", 3000);
        stopLocalScreenCapture();
        return;
    }
    

    if (!calls::startScreenSharing())
    {
        showTransientStatusMessage("Failed to start screen sharing", 3000);
        stopLocalScreenCapture();
        return;
    }

    m_callWidget->setShowingDisplayActive(true);
}

void MainWindow::onCaptureStopped()
{
    calls::stopScreenSharing();
    m_callWidget->disableStartScreenShareButton(false);
    m_callWidget->setShowingDisplayActive(false);
}

void MainWindow::onScreenCaptured(const QPixmap& pixmap, const std::vector<unsigned char>& imageData)
{
    if (m_callWidget && !pixmap.isNull())
    {
        m_callWidget->setShowingDisplayActive(true);
        m_callWidget->showFrame(pixmap);
    }

    if (imageData.empty()) return;

    if (!calls::sendScreen(imageData))
        showTransientStatusMessage("Failed to send screen frame", 2000);
}

void MainWindow::onStartScreenSharingError()
{
    showTransientStatusMessage("Screen sharing rejected by server", 3000);
    stopLocalScreenCapture();
}

void MainWindow::onIncomingScreenSharingStarted()
{
    if (m_callWidget) {
        m_callWidget->setShowingDisplayActive(true, true);
        m_callWidget->disableStartScreenShareButton(true);
    }
}

void MainWindow::onIncomingScreenSharingStopped()
{
    if (m_callWidget) {
        m_callWidget->setShowingDisplayActive(false);
        m_callWidget->disableStartScreenShareButton(false);
    }
}

void MainWindow::onIncomingScreen(const std::vector<unsigned char>& data)
{
    if (!m_callWidget || data.empty() || !calls::isViewingRemoteScreen()) return;

    QPixmap frame;
    const auto* raw = reinterpret_cast<const uchar*>(data.data());

    if (frame.loadFromData(raw, static_cast<int>(data.size()), "JPG"))
        m_callWidget->showFrame(frame);
}

void MainWindow::onCallWidgetEnterFullscreenRequested()
{
    showFullScreen();
}

void MainWindow::onCallWidgetExitFullscreenRequested()
{
    showMaximized(); 
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

    stopLocalScreenCapture();

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
    if (m_screenCaptureController->isCapturing())
        stopLocalScreenCapture();
    
    bool requestSent = calls::acceptCall(friendNickname.toStdString());
    if (!requestSent) {
        handleAcceptCallErrorNotificationAppearance();
        return;
    }
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
        showTransientStatusMessage(errorText, 1500);
    }
    else {
        LOG_WARN("Trying to accept call from unexpected widget");
    }
}

void MainWindow::handleDeclineCallErrorNotificationAppearance() {
    QString errorText = "Failed to decline call. Please try again";

    if (m_stackedLayout->currentWidget() == m_mainMenuWidget) {
        m_mainMenuWidget->showErrorNotification(errorText, 1500);
    }
    else if (m_stackedLayout->currentWidget() == m_callWidget) {
        showTransientStatusMessage(errorText, 1500);
    }
    else {
        LOG_WARN("Trying to decline call from unexpected widget");
    }
}

void MainWindow::handleStartCallingErrorNotificationAppearance() {
    QString errorText = "Failed to start calling. Please try again";

    if (m_stackedLayout->currentWidget() == m_mainMenuWidget) {
        m_mainMenuWidget->showErrorNotification(errorText, 1500);
    }
    else {
        LOG_WARN("Trying to start calling from unexpected widget");
    }
}

void MainWindow::handleStopCallingErrorNotificationAppearance() {
    QString errorText = "Failed to stop calling. Please try again";

    if (m_stackedLayout->currentWidget() == m_mainMenuWidget) {
        m_mainMenuWidget->showErrorNotification(errorText, 1500);
    }
    else {
        LOG_WARN("Trying to stop calling from unexpected widget");
    }
}

void MainWindow::handleEndCallErrorNotificationAppearance() {
    QString errorText = "Failed to end call. Please try again";

    if (m_stackedLayout->currentWidget() == m_callWidget) {
        showTransientStatusMessage(errorText, 1500);
    }
    else {
        LOG_WARN("Trying to end call from unexpected widget");
    }
}

void MainWindow::onAuthorizationResult(calls::ErrorCode ec) {
    QString errorMessage;

    if (ec == calls::ErrorCode::OK) {
        LOG_INFO("User authorization successful");
        return;
    }
    else if (ec == calls::ErrorCode::TAKEN_NICKNAME) {
        errorMessage = "Taken nickname";
        LOG_WARN("Authorization failed: nickname already taken");
    }
    else if (ec == calls::ErrorCode::TIMEOUT) {
        errorMessage = "Timeout (please try again)";
        LOG_ERROR("Authorization failed: timeout");
    }

    m_authorizationWidget->setErrorMessage(errorMessage);
}

void MainWindow::onStartCallingResult(calls::ErrorCode ec) {
    QString errorMessage;

    if (ec == calls::ErrorCode::OK) {
        LOG_INFO("Started calling user: {}", calls::getNicknameWhomCalling());
        m_mainMenuWidget->showCallingPanel(QString::fromStdString(calls::getNicknameWhomCalling()));
        m_mainMenuWidget->setState(calls::State::CALLING);
        playCallingRingtone();
        return;
    }
    else if (ec == calls::ErrorCode::UNEXISTING_USER) {
        errorMessage = "User not found";
        LOG_WARN("Start calling failed: user not found");
    }
    else if (ec == calls::ErrorCode::TIMEOUT) {
        errorMessage = "Timeout (please try again)";
        LOG_ERROR("Start calling failed: timeout");
    }

    m_mainMenuWidget->setErrorMessage(errorMessage);
}

void MainWindow::onAcceptCallResult(calls::ErrorCode ec, const QString& nickname) {
    if (ec == calls::ErrorCode::OK) {
        LOG_INFO("Call accepted successfully with: {}", nickname.toStdString());
        m_mainMenuWidget->removeCallingPanel();
        m_mainMenuWidget->clearIncomingCalls();

        if (m_stackedLayout->currentWidget() == m_callWidget)
            m_callWidget->clearIncomingCalls();

        playSoundEffect(":/resources/callJoined.wav");
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
    LOG_INFO("Outgoing call accepted by user: {}", calls::getNicknameInCallWith());
    m_mainMenuWidget->clearIncomingCalls();

    stopRingtone();
    m_mainMenuWidget->removeCallingPanel();
    m_mainMenuWidget->setState(calls::State::BUSY);
    switchToCallWidget(QString::fromStdString(calls::getNicknameInCallWith()));
    playSoundEffect(":/resources/callJoined.wav");
}

void MainWindow::onCallingDeclined() {
    LOG_INFO("Outgoing call was declined");
    m_mainMenuWidget->removeCallingPanel();
    m_mainMenuWidget->setState(calls::State::FREE);
    stopRingtone();
    playSoundEffect(":/resources/callingEnded.wav");
}

void MainWindow::onRemoteUserEndedCall() {
    if (m_screenCaptureController->isCapturing())
        stopLocalScreenCapture();

    LOG_INFO("Remote user ended the call");
    m_callWidget->clearIncomingCalls();
    m_callWidget->disableStartScreenShareButton(false);
    m_callWidget->setShowingDisplayActive(false);
    switchToMainMenuWidget();

    m_mainMenuWidget->setState(calls::State::FREE);
    playSoundEffect(":/resources/endCall.wav");
}

void MainWindow::onIncomingCall(const QString& friendNickName) {
    LOG_INFO("Incoming call from: {}", friendNickName.toStdString());
    playIncomingCallRingtone();

    m_mainMenuWidget->addIncomingCall(friendNickName);

    if (m_stackedLayout->currentWidget() == m_callWidget) 
        m_callWidget->addIncomingCall(friendNickName);
}

void MainWindow::onIncomingCallExpired(const QString& friendNickName) {
    LOG_INFO("Incoming call from {} expired", friendNickName.toStdString());
    m_mainMenuWidget->removeIncomingCall(friendNickName);

    if (m_stackedLayout->currentWidget() == m_callWidget)
        m_callWidget->removeIncomingCall(friendNickName);

    if (calls::getIncomingCallsCount() == 0)
        stopRingtone();

    playSoundEffect(":/resources/callingEnded.wav");
}

void MainWindow::onClientNetworkError() {
    LOG_ERROR("Client network error occurred");
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
    LOG_ERROR("Updater network error occurred");
    m_authorizationWidget->hideUpdateAvailableNotification();
    m_mainMenuWidget->hideUpdateAvailableNotification();

    m_dialogsController->hideUpdatingDialog();
    if (!calls::isAuthorized() || updater::isAwaitingServerResponse()) {
        m_dialogsController->showConnectionErrorDialog();
    }

    updater::disconnect();
}

void MainWindow::onConnectionRestored() {
    LOG_INFO("Connection restored to server");
    m_authorizationWidget->resetBlur();
    m_authorizationWidget->setAuthorizationDisabled(false);
    m_authorizationWidget->showConnectionRestoredNotification(1500);

    if (!updater::isConnected()) {
        LOG_INFO("Reconnecting updater");
        updater::connect(updater::getServerHost(), updater::getServerPort());
        updater::checkUpdates(parseVersionFromConfig());
    }
}
