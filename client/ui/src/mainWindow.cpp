#include "mainWindow.h"
#include <QApplication>
#include <QGuiApplication>
#include <QTimer>
#include <QCloseEvent>
#include <QDir>
#include <QCoreApplication>
#include <QFile>
#include <QScreen>

#include "widgets/authorizationWidget.h"
#include "widgets/mainMenuWidget.h"
#include "widgets/callWidget.h"
#include "dialogs/dialogsController.h"
#include "notifications/notificationController.h"
#include "logic/configManager.h"
#include "utilities/logger.h"
#include "utilities/diagnostic.h"
#include "utilities/updaterDiagnostics.h"
#include "utilities/utilities.h"
#include "constants/constant.h"

#include "audio/audioEffectsManager.h"
#include "audio/audioSettingsManager.h"
#include "logic/updateManager.h"
#include "logic/navigationController.h"
#include "logic/authorizationManager.h"
#include "logic/coreNetworkErrorHandler.h"
#include "logic/updaterNetworkErrorHandler.h"
#include "logic/callManager.h"
#include "audio/audioDevicesWatcher.h"

#include "events/coreEventListener.h"
#include "events/updaterEventListener.h"

const QEvent::Type StartupEvent::StartupEventType = static_cast<QEvent::Type>(QEvent::User + 1);

void MainWindow::init() {
    setWindowIcon(QIcon(":/resources/callifornia.ico"));

    m_configManager = new ConfigManager(getConfigFilePath()); 

    m_configManager->loadConfig();

    replaceUpdateApplier();
    initializeDiagnostics();

    m_coreClient = std::make_shared<core::Core>();

    m_updaterClient = std::make_shared<updater::Client>();

    m_initialConnectionRetryTimer = new QTimer(this);
    m_initialConnectionRetryTimer->setSingleShot(true);
    connect(m_initialConnectionRetryTimer, &QTimer::timeout, this, &MainWindow::onInitialConnectionRetry);

    loadFonts();

    initializeCentralStackedWidget();
    initializeAuthorizationWidget();
    initializeMainMenuWidget();
    initializeCallWidget();

    initializeDialogsController();
    initializeNotificationController();
    initializeAudioManager();
    initializeAudioDevicesWatcher();
    initializeAudioSettingsManager();
    initializeNavigationController();
    initializeUpdateManager();
    initializeAuthorizationManager();
    initializeCallManager();
    initializeCoreNetworkErrorHandler();
    initializeUpdaterNetworkErrorHandler();

        connectWidgetsToManagers();
    // Apply UI-only settings (widgets); Core audio settings applied after Core::start()
    if (m_mainMenuWidget) {
        m_mainMenuWidget->setInputVolume(m_configManager->getInputVolume());
        m_mainMenuWidget->setOutputVolume(m_configManager->getOutputVolume());
    }
    if (m_callWidget) {
        m_callWidget->setInputVolume(m_configManager->getInputVolume());
        m_callWidget->setOutputVolume(m_configManager->getOutputVolume());
    }
    showMaximized();

    QCoreApplication::postEvent(this, new StartupEvent());
}

void MainWindow::customEvent(QEvent* event) {
    if (!isFirstInstance() && !m_configManager->isMultiInstanceAllowed()) {
        m_dialogsController->showAlreadyRunningDialog();

        return;
    }

    if (event->type() == StartupEvent::StartupEventType) {

        LOG_INFO("Config: {}", m_configManager->getConfigPath().toStdString());

        LOG_INFO("Connecting: Core {} TCP {} UDP {}, Updater {}:{}",
            m_configManager->getServerHost().toStdString(),
            m_configManager->getMainServerTcpPort().toStdString(),
            m_configManager->getMainServerUdpPort().toStdString(),
            m_configManager->getUpdaterHost().toStdString(),
            m_configManager->getUpdaterServerTcpPort().toStdString());

        m_updaterClient->init(std::make_shared<UpdaterEventListener>(m_updateManager, m_updaterNetworkErrorHandler),
            m_configManager->getAppDirectoryPath().toStdString(),
            m_configManager->getTemporaryUpdateDirectoryPath().toStdString(),
            m_configManager->getDeletionListFileName().toStdString(),
            m_configManager->getIgnoredFilesWhileCollectingForUpdate(),
            m_configManager->getIgnoredDirectoriesWhileCollectingForUpdate()
        );

        const bool coreStarted = m_coreClient->start(
            m_configManager->getServerHost().toStdString(),
            m_configManager->getServerHost().toStdString(),
            m_configManager->getMainServerTcpPort().toStdString(),
            m_configManager->getMainServerUdpPort().toStdString(),
            std::make_shared<CoreEventListener>(m_authorizationManager, m_callManager, m_coreNetworkErrorHandler)
        );

        if (coreStarted) {
            applyAudioSettings();  // Apply to Core (m_audioEngine now exists)
        }

        if (!coreStarted) {
            LOG_ERROR("Core client failed to start (check server {} TCP {} UDP {}, network, and core.log). Will retry every {} s.",
                m_configManager->getServerHost().toStdString(),
                m_configManager->getMainServerTcpPort().toStdString(),
                m_configManager->getMainServerUdpPort().toStdString(),
                INITIAL_CONNECTION_RETRY_INTERVAL_MS / 1000);
            if (m_coreNetworkErrorHandler)
                m_coreNetworkErrorHandler->onConnectionDown();
            if (m_initialConnectionRetryTimer)
                m_initialConnectionRetryTimer->start(INITIAL_CONNECTION_RETRY_INTERVAL_MS);
        }

        m_updaterClient->start(m_configManager->getUpdaterHost().toStdString(),
            m_configManager->getUpdaterServerTcpPort().toStdString()
        );
    }

    QMainWindow::customEvent(event);
}

void MainWindow::onInitialConnectionRetry() {
    const bool coreStarted = m_coreClient->start(
        m_configManager->getServerHost().toStdString(),
        m_configManager->getServerHost().toStdString(),
        m_configManager->getMainServerTcpPort().toStdString(),
        m_configManager->getMainServerUdpPort().toStdString(),
        std::make_shared<CoreEventListener>(m_authorizationManager, m_callManager, m_coreNetworkErrorHandler)
    );

    if (coreStarted) {
        if (m_initialConnectionRetryTimer && m_initialConnectionRetryTimer->isActive())
            m_initialConnectionRetryTimer->stop();
        if (m_coreNetworkErrorHandler)
            m_coreNetworkErrorHandler->onConnectionRestored();
        applyAudioSettings();
        LOG_INFO("Core connected to server after retry.");
    }
    else {
        if (m_initialConnectionRetryTimer)
            m_initialConnectionRetryTimer->start(INITIAL_CONNECTION_RETRY_INTERVAL_MS);
    }
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (m_initialConnectionRetryTimer && m_initialConnectionRetryTimer->isActive())
        m_initialConnectionRetryTimer->stop();

    if (m_updaterClient) {
        m_updaterClient->stop();
    }

    if (m_coreClient && m_coreClient->isAuthorized()) {
        std::error_code ec = m_coreClient->logout();
        if (ec) {
            LOG_WARN("Logout failed, closing application immediately. Error: {}", ec.message());
            event->accept();
        }
        else {
            event->accept();
        }
    }
    else {
        event->accept();
    }
}

void MainWindow::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::WindowStateChange)
    {
        QWindowStateChangeEvent* stateChangeEvent = static_cast<QWindowStateChangeEvent*>(event);
        Qt::WindowStates oldState = stateChangeEvent->oldState();
        Qt::WindowStates newState = windowState();

        if ((oldState & Qt::WindowMaximized) && 
            !(newState & Qt::WindowMaximized) && 
            !(newState & Qt::WindowFullScreen))
        {
            resize(scale(800), scale(600));
        }
    }

    QMainWindow::changeEvent(event);
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
    }
}

void MainWindow::initializeCentralStackedWidget() {
    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);

    m_mainLayout = new QHBoxLayout(m_centralWidget);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    m_stackedLayout = new QStackedLayout();
    m_mainLayout->addLayout(m_stackedLayout);
}

void MainWindow::initializeDialogsController() {
    m_dialogsController = new DialogsController(this);
    connect(m_dialogsController, &DialogsController::closeRequested, this, &MainWindow::close);
    if (m_updaterClient) {
        m_dialogsController->setUpdateClient(m_updaterClient);
    }
}

void MainWindow::initializeNotificationController() {
    m_notificationController = new NotificationController(this);
}

void MainWindow::initializeNavigationController() {
    m_navigationController = new NavigationController(m_coreClient, this);
    if (m_stackedLayout && m_authorizationWidget && m_mainMenuWidget && m_callWidget) {
        m_navigationController->setWidgets(m_stackedLayout, m_authorizationWidget, m_mainMenuWidget, m_callWidget);
    }
}

void MainWindow::initializeAudioManager() {
    m_audioManager = new AudioEffectsManager(m_coreClient, this);
}

void MainWindow::initializeAudioDevicesWatcher() {
    m_audioDevicesWatcher = new AudioDevicesWatcher(m_coreClient, this);
}

void MainWindow::initializeAudioSettingsManager() {
    m_audioSettingsManager = new AudioSettingsManager(m_coreClient, m_configManager, this);
}

void MainWindow::initializeUpdateManager() {
    m_updateManager = new UpdateManager(m_coreClient, m_updaterClient, m_configManager, this);
    if (m_authorizationWidget && m_mainMenuWidget && m_dialogsController) {
        m_updateManager->setWidgets(m_authorizationWidget, m_mainMenuWidget, m_dialogsController);
    }
}

void MainWindow::initializeAuthorizationManager() {
    m_authorizationManager = new AuthorizationManager(m_coreClient, m_navigationController, m_configManager, m_dialogsController, m_updateManager, this);
    if (m_authorizationWidget && m_mainMenuWidget) {
        m_authorizationManager->setWidgets(m_authorizationWidget, m_mainMenuWidget);
    }
}

void MainWindow::initializeCallManager() {
    m_callManager = new CallManager(m_coreClient, m_audioManager, m_navigationController, m_dialogsController, m_updateManager, this);
    if (m_mainMenuWidget && m_callWidget && m_stackedLayout) {
        m_callManager->setWidgets(m_mainMenuWidget, m_callWidget, m_stackedLayout);
    }
    if (m_callManager && m_notificationController) {
        m_callManager->setNotificationController(m_notificationController);
    }
}

void MainWindow::initializeCoreNetworkErrorHandler() {
    m_coreNetworkErrorHandler = new CoreNetworkErrorHandler(m_coreClient, m_navigationController, m_configManager, m_audioManager, this);
    if (m_authorizationWidget && m_mainMenuWidget && m_dialogsController) {
        m_coreNetworkErrorHandler->setWidgets(m_authorizationWidget, m_mainMenuWidget, m_dialogsController);
    }
    if (m_callManager) {
        m_coreNetworkErrorHandler->setManagers(m_callManager);
    }
    if (m_coreNetworkErrorHandler && m_notificationController) {
        m_coreNetworkErrorHandler->setNotificationController(m_notificationController);
    }
}

void MainWindow::initializeUpdaterNetworkErrorHandler() {
    m_updaterNetworkErrorHandler = new UpdaterNetworkErrorHandler(m_updaterClient, m_navigationController, m_updateManager, m_configManager, this);
    if (m_authorizationWidget && m_mainMenuWidget && m_dialogsController) {
        m_updaterNetworkErrorHandler->setWidgets(m_authorizationWidget, m_mainMenuWidget, m_dialogsController);
    }
    if (m_updaterNetworkErrorHandler && m_notificationController) {
        m_updaterNetworkErrorHandler->setNotificationController(m_notificationController);
    }
}

void MainWindow::applyAudioSettings() {
    if (m_mainMenuWidget) {
        m_mainMenuWidget->setInputVolume(m_configManager->getInputVolume());
        m_mainMenuWidget->setOutputVolume(m_configManager->getOutputVolume());
        m_mainMenuWidget->setCameraActive(m_configManager->isCameraActive());
    }

    if (m_callWidget) {
        m_callWidget->setInputVolume(m_configManager->getInputVolume());
        m_callWidget->setOutputVolume(m_configManager->getOutputVolume());
    }

    m_coreClient->setInputVolume(m_configManager->getInputVolume());
    m_coreClient->setOutputVolume(m_configManager->getOutputVolume());
    m_coreClient->muteMicrophone(m_configManager->isMicrophoneMuted());
    m_coreClient->muteSpeaker(m_configManager->isSpeakerMuted());
}

void MainWindow::connectWidgetsToManagers() {
    // AuthorizationWidget connections
    if (m_authorizationWidget) {
        if (m_authorizationManager) {
            connect(m_authorizationWidget, &AuthorizationWidget::authorizationButtonClicked, m_authorizationManager, &AuthorizationManager::onAuthorizationButtonClicked);
        }
    }

    // MainMenuWidget connections
    if (m_mainMenuWidget) {
        if (m_audioSettingsManager) {
            connect(m_mainMenuWidget, &MainMenuWidget::inputVolumeChanged, m_audioSettingsManager, &AudioSettingsManager::onInputVolumeChanged);
            connect(m_mainMenuWidget, &MainMenuWidget::outputVolumeChanged, m_audioSettingsManager, &AudioSettingsManager::onOutputVolumeChanged);
            connect(m_mainMenuWidget, &MainMenuWidget::muteMicrophoneClicked, m_audioSettingsManager, &AudioSettingsManager::onMuteMicrophoneButtonClicked);
            connect(m_mainMenuWidget, &MainMenuWidget::muteSpeakerClicked, m_audioSettingsManager, &AudioSettingsManager::onMuteSpeakerButtonClicked);
        }
        if (m_callManager) {
            connect(m_mainMenuWidget, &MainMenuWidget::audioDevicePickerRequested, m_callManager, &CallManager::onAudioDevicePickerRequested);
            connect(m_mainMenuWidget, &MainMenuWidget::startOutgoingCallButtonClicked, m_callManager, &CallManager::onStartOutgoingCallButtonClicked);
            connect(m_mainMenuWidget, &MainMenuWidget::stopOutgoingCallButtonClicked, m_callManager, &CallManager::onStopOutgoingCallButtonClicked);
            connect(m_mainMenuWidget, &MainMenuWidget::activateCameraClicked, m_callManager, &CallManager::onActivateCameraClicked);
        }
    }

    // CallWidget connections
    if (m_callWidget) {
        if (m_navigationController) {
            connect(m_callWidget, &CallWidget::requestEnterFullscreen, m_navigationController, &NavigationController::onCallWidgetEnterFullscreenRequested);
            connect(m_callWidget, &CallWidget::requestExitFullscreen, m_navigationController, &NavigationController::onCallWidgetExitFullscreenRequested);
        }
        if (m_callManager) {
            connect(m_callWidget, &CallWidget::audioSettingsRequested, m_callManager, &CallManager::onCallWidgetAudioSettingsRequested);
        }
        if (m_audioSettingsManager) {
            connect(m_callWidget, &CallWidget::inputVolumeChanged, m_audioSettingsManager, &AudioSettingsManager::onInputVolumeChanged);
            connect(m_callWidget, &CallWidget::outputVolumeChanged, m_audioSettingsManager, &AudioSettingsManager::onOutputVolumeChanged);
            connect(m_callWidget, &CallWidget::muteSpeakerClicked, m_audioSettingsManager, &AudioSettingsManager::onMuteSpeakerButtonClicked);
            connect(m_callWidget, &CallWidget::muteMicrophoneClicked, m_audioSettingsManager, &AudioSettingsManager::onMuteMicrophoneButtonClicked);
        }
        if (m_callManager) {
            connect(m_callWidget, &CallWidget::hangupClicked, m_callManager, &CallManager::onEndCallButtonClicked);
        }
        if (m_callManager) {
            connect(m_callWidget, &CallWidget::screenShareClicked, m_callManager, &CallManager::onScreenShareClicked);
            connect(m_callWidget, &CallWidget::cameraClicked, m_callManager, &CallManager::onCallWidgetCameraClicked);
        }
    }


    // DialogsController connections
    if (m_dialogsController && m_callManager) {
        connect(m_dialogsController, &DialogsController::screenSelected, m_callManager, &CallManager::onScreenSelected);
        connect(m_dialogsController, &DialogsController::screenShareDialogCancelled, m_callManager, &CallManager::onScreenShareDialogCancelled);
        connect(m_dialogsController, &DialogsController::audioSettingsDialogClosed, m_callManager, &CallManager::onAudioSettingsDialogClosed);
    }

    // Audio settings dialog connections
    if (m_dialogsController && m_audioSettingsManager) {
        connect(m_dialogsController, &DialogsController::refreshAudioDevicesRequested, m_audioSettingsManager, &AudioSettingsManager::onRefreshAudioDevicesButtonClicked);
        connect(m_dialogsController, &DialogsController::inputDeviceSelected, m_audioSettingsManager, &AudioSettingsManager::onInputDeviceSelected);
        connect(m_dialogsController, &DialogsController::outputDeviceSelected, m_audioSettingsManager, &AudioSettingsManager::onOutputDeviceSelected);
        connect(m_dialogsController, &DialogsController::inputVolumeChanged, m_audioSettingsManager, &AudioSettingsManager::onInputVolumeChanged);
        connect(m_dialogsController, &DialogsController::outputVolumeChanged, m_audioSettingsManager, &AudioSettingsManager::onOutputVolumeChanged);
        connect(m_dialogsController, &DialogsController::muteMicrophoneClicked, m_audioSettingsManager, &AudioSettingsManager::onMuteMicrophoneButtonClicked);
        connect(m_dialogsController, &DialogsController::muteSpeakerClicked, m_audioSettingsManager, &AudioSettingsManager::onMuteSpeakerButtonClicked);
    }

    // Refresh audio settings dialog when system audio devices change (e.g. plug/unplug)
    if (m_audioDevicesWatcher && m_dialogsController && m_coreClient) {
        connect(m_audioDevicesWatcher, &AudioDevicesWatcher::devicesChanged, [this]() {
            m_dialogsController->refreshAudioSettingsDialogDevices(m_coreClient->getCurrentInputDevice(), m_coreClient->getCurrentOutputDevice());
        });
    }

    // Update available dialog connections
    if (m_dialogsController && m_updateManager) {
        connect(m_dialogsController, &DialogsController::updateButtonClicked, m_updateManager, &UpdateManager::onUpdateButtonClicked);
    }

    // Manager to MainWindow connections
    if (m_navigationController) {
        connect(m_navigationController, &NavigationController::windowTitleChanged, this, &MainWindow::onWindowTitleChanged);
        connect(m_navigationController, &NavigationController::windowFullscreenRequested, this, &MainWindow::onWindowFullscreenRequested);
        connect(m_navigationController, &NavigationController::windowMaximizedRequested, this, &MainWindow::onWindowMaximizedRequested);
        // Camera initialization during call start is handled in core
    }
    if (m_updateManager) {
        connect(m_updateManager, &UpdateManager::stopAllRingtonesRequested, this, &MainWindow::onStopAllRingtonesRequested);
    }
    if (m_callManager) {
        connect(m_callManager, &CallManager::endCallFullscreenExitRequested, this, &MainWindow::onEndCallFullscreenExitRequested);
    }
}

void MainWindow::initializeAuthorizationWidget() {
    m_authorizationWidget = new AuthorizationWidget(this);
    m_stackedLayout->addWidget(m_authorizationWidget);
}

void MainWindow::initializeMainMenuWidget() {
    m_mainMenuWidget = new MainMenuWidget(this);
    m_stackedLayout->addWidget(m_mainMenuWidget);
}

void MainWindow::initializeCallWidget() {
    m_callWidget = new CallWidget(this);
    m_stackedLayout->addWidget(m_callWidget);
}

void MainWindow::replaceUpdateApplier() {
    QString appDirectory = m_configManager->getAppDirectoryPath();
    QDir appDir(appDirectory);
    
    if (!appDir.exists()) {
        LOG_WARN("Application directory does not exist: {}", appDirectory.toStdString());
        return;
    }
    
    updater::OperationSystemType osType = m_configManager->getOperationSystemType();
    
    QString oldUpdateApplierName;
    QString newUpdateApplierName;
    
    if (osType == updater::OperationSystemType::WINDOWS) {
        oldUpdateApplierName = UPDATE_APPLIER_EXECUTABLE_NAME_WINDOWS;
        newUpdateApplierName = UPDATE_APPLIER_EXECUTABLE_NAME_WINDOWS_NEW;
    } else {
        oldUpdateApplierName = UPDATE_APPLIER_EXECUTABLE_NAME_LINUX_AND_MAC;
        newUpdateApplierName = UPDATE_APPLIER_EXECUTABLE_NAME_LINUX_AND_MAC_NEW;
    }
    
    QString newUpdateApplierPath = appDir.filePath(newUpdateApplierName);
    QString oldUpdateApplierPath = appDir.filePath(oldUpdateApplierName);
    
    if (!QFile::exists(newUpdateApplierPath)) {
        LOG_INFO("New updateApplier not found: {}, skipping replacement", newUpdateApplierPath.toStdString());
        return;
    }
    
    if (QFile::exists(oldUpdateApplierPath)) {
        if (!QFile::remove(oldUpdateApplierPath)) {
            LOG_ERROR("Failed to remove old updateApplier: {}", oldUpdateApplierPath.toStdString());
            return;
        }
        LOG_INFO("Removed old updateApplier");
    }
    
    QFile newUpdateApplierFile(newUpdateApplierPath);
    if (!newUpdateApplierFile.rename(oldUpdateApplierPath)) {
        LOG_ERROR("Failed to rename new updateApplier: {} -> {}", 
            newUpdateApplierPath.toStdString(), oldUpdateApplierPath.toStdString());
        return;
    }
    
    QFile::setPermissions(oldUpdateApplierPath,
        QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner |
        QFile::ReadUser | QFile::WriteUser | QFile::ExeUser |
        QFile::ReadGroup | QFile::ExeGroup |
        QFile::ReadOther | QFile::ExeOther);
    
    LOG_INFO("Successfully replaced updateApplier: {} -> {}", 
        newUpdateApplierPath.toStdString(), oldUpdateApplierPath.toStdString());
}

void MainWindow::initializeDiagnostics() {
    const QString appDirectory = m_configManager->getAppDirectoryPath();
    const QString logDirectoryPath = m_configManager->getLogDirectoryPath();
    const QString crashDumpDirectoryPath = m_configManager->getCrashDumpDirectoryPath();
    const QString appVersion = m_configManager->getVersion();

    ui::utilities::initializeDiagnostics(appDirectory.toStdString(),
        logDirectoryPath.toStdString(),
        crashDumpDirectoryPath.toStdString(),
        appVersion.toStdString());

    updater::utilities::initializeDiagnostics(appDirectory.toStdString(),
        logDirectoryPath.toStdString(),
        crashDumpDirectoryPath.toStdString(),
        appVersion.toStdString());

    core::initializeDiagnostics(appDirectory.toStdString(),
        logDirectoryPath.toStdString(),
        crashDumpDirectoryPath.toStdString(),
        appVersion.toStdString());
}

void MainWindow::onWindowTitleChanged(const QString& title)
{
    setWindowTitle(title);
}

void MainWindow::onWindowFullscreenRequested()
{
    showFullScreen();
}

void MainWindow::onWindowMaximizedRequested()
{
    if (m_callWidget && m_callWidget->isFullScreen()) {
        m_callWidget->exitFullscreen();
    }
    showMaximized();
}

void MainWindow::onEndCallFullscreenExitRequested()
{
    if (m_callWidget && m_callWidget->isFullScreen()) {
        m_callWidget->exitFullscreen();
    }
    showMaximized();
}

void MainWindow::onStopAllRingtonesRequested()
{
    if (m_audioManager) {
        m_audioManager->stopOutgoingCallRingtone();
        m_audioManager->stopIncomingCallRingtone();
    }
}

QString MainWindow::getConfigFilePath() const
{
    updater::OperationSystemType operationSystemType = resolveOperationSystemType();
    QString applicationDirPath = QCoreApplication::applicationDirPath();
    QString configNextToApp = QDir(applicationDirPath).filePath("config.json");

    QStringList configPaths;
    configPaths << configNextToApp;  // First priority: next to the application

    if (operationSystemType == updater::OperationSystemType::WINDOWS) {
        configPaths << "C:/Users/Public/Callifornia/config.json"
                    << "C:/Callifornia/config.json";
    }
    else if (operationSystemType == updater::OperationSystemType::LINUX) {
        configPaths << "/opt/Callifornia/config.json"
                    << QDir::homePath() + "/.config/Callifornia/config.json";
    }
    else if (operationSystemType == updater::OperationSystemType::MAC) {
        configPaths << "/Applications/Callifornia/config.json"
                    << QDir::homePath() + "/Library/Application Support/Callifornia/config.json";
    }

    for (const QString& path : configPaths) {
        if (QFile::exists(path)) {
            return path;
        }
    }

    return configNextToApp;
}
