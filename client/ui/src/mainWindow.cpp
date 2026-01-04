#include "mainWindow.h"
#include <QFontDatabase>
#include <QApplication>
#include <QStatusBar>
#include <QEvent>
#include <QTimer>
#include <QIcon>
#include <QCloseEvent>
#include <QWindowStateChangeEvent>

#include "widgets/authorizationWidget.h"
#include "widgets/mainMenuWidget.h"
#include "widgets/callWidget.h"
#include "managers/dialogsController.h"
#include "media/screenCaptureController.h"
#include "media/cameraCaptureController.h"
#include "prerequisites.h"
#include "events/calliforniaEventListener.h"
#include "managers/configManager.h"
#include "utilities/logger.h"

#include "media/audioEffectsManager.h"
#include "media/audioSettingsManager.h"
#include "managers/updateManager.h"
#include "managers/navigationController.h"
#include "managers/authorizationManager.h"
#include "managers/callManager.h"
#include "media/screenSharingManager.h"
#include "media/cameraSharingManager.h"
#include "network/networkErrorHandler.h"

#include "client.h"
#include "updater.h"

MainWindow::MainWindow() {
    m_configManager = new ConfigManager();
    m_configManager->loadConfig();
}

MainWindow::~MainWindow() 
{
}

void MainWindow::executePrerequisites() {
    makeRemainingReplacements();
}

void MainWindow::init() {
    // Create Client instance
    m_client = std::make_unique<callifornia::Client>();
    std::shared_ptr<ClientCallbacksHandler> callsClientHandler = std::make_shared<ClientCallbacksHandler>(this);
    
    if (!m_client->init(m_configManager->getServerHost().toStdString(), 
                       m_configManager->getPort().toStdString(), 
                       callsClientHandler)) {
        LOG_ERROR("Failed to initialize client");
        return;
    }
    
    m_client->setOutputVolume(m_configManager->getOutputVolume());
    m_client->setInputVolume(m_configManager->getInputVolume());
    m_client->muteMicrophone(m_configManager->isMicrophoneMuted());
    m_client->muteSpeaker(m_configManager->isSpeakerMuted());

    // Create Updater instance
    m_updater = std::make_unique<callifornia::updater::Updater>();

    LOG_INFO("Initializing main window");
    setWindowIcon(QIcon(":/resources/callifornia.ico"));

    // Create managers (navigation controller created after UI setup)
    m_audioManager = new AudioEffectsManager(m_client.get(), this);
    m_audioSettingsManager = new AudioSettingsManager(m_client.get(), m_configManager, this);
    m_updateManager = new UpdateManager(m_client.get(), m_updater.get(), m_configManager, this);
    m_authorizationManager = nullptr; // Created after navigation controller
    m_callManager = new CallManager(m_client.get(), m_audioManager, nullptr, this); // Navigation controller set later
    m_screenSharingManager = new ScreenSharingManager(m_client.get(), this);
    m_cameraSharingManager = new CameraSharingManager(m_client.get(), m_configManager, this);
    m_networkErrorHandler = nullptr; // Created after navigation controller

    loadFonts();
    setupUI();
    setupManagerConnections();
    setSettingsFromConfig();
    showMaximized();

    QTimer::singleShot(0, [this]() {
        bool firstInstance = isFirstInstance();
        bool multipleInstancesAllowed = m_configManager->isMultiInstanceAllowed();

        if (!firstInstance && !multipleInstancesAllowed) {
            if (m_dialogsController) {
                m_dialogsController->showAlreadyRunningDialog();
            }
        } 
        else {
            if (m_updateManager) {
                m_updateManager->checkUpdates();
            }
        }
    });
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (m_client) {
        m_client.reset();
    }
    if (m_updater) {
        m_updater->disconnect();
        m_updater.reset();
    }
    event->accept();
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
            resize(800, 600);
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
        LOG_DEBUG("Fonts loaded successfully");
    }
}


void MainWindow::setSettingsFromConfig() {
    if (m_mainMenuWidget) {
        m_mainMenuWidget->setInputVolume(m_configManager->getInputVolume());
        m_mainMenuWidget->setOutputVolume(m_configManager->getOutputVolume());
        m_mainMenuWidget->setCameraActive(m_configManager->isCameraActive());
    }

    if (m_callWidget) {
        m_callWidget->setInputVolume(m_configManager->getInputVolume());
        m_callWidget->setOutputVolume(m_configManager->getOutputVolume());
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

    // Create dialogs controller first (needed for AuthorizationManager)
    m_dialogsController = new DialogsController(this);
    connect(m_dialogsController, &DialogsController::closeRequested, this, &MainWindow::close);

    // Create navigation controller with layout
    m_navigationController = new NavigationController(m_client.get(), m_stackedLayout, this);
    // Update managers that depend on navigation controller
    if (m_authorizationManager) {
        delete m_authorizationManager;
    }
    m_authorizationManager = new AuthorizationManager(m_client.get(), m_navigationController, m_configManager, m_dialogsController, this);
    if (m_callManager) {
        m_callManager->setNavigationController(m_navigationController);
    }
    if (m_networkErrorHandler) {
        delete m_networkErrorHandler;
    }
    m_networkErrorHandler = new NetworkErrorHandler(m_client.get(), m_updater.get(), m_navigationController, m_updateManager, m_configManager, this);

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
    connect(m_mainMenuWidget, &MainMenuWidget::activateCameraClicked, this, &MainWindow::onActivateCameraButtonClicked);
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
    connect(m_callWidget, &CallWidget::cameraClicked, this, &MainWindow::onCameraButtonClicked);
    connect(m_callWidget, &CallWidget::requestEnterFullscreen, this, &MainWindow::onCallWidgetEnterFullscreenRequested);
    connect(m_callWidget, &CallWidget::requestExitFullscreen, this, &MainWindow::onCallWidgetExitFullscreenRequested);
    m_stackedLayout->addWidget(m_callWidget);

    m_screenCaptureController = new ScreenCaptureController(this);
    connect(m_screenCaptureController, &ScreenCaptureController::captureStarted, this, &MainWindow::onScreenCaptureStarted);
    connect(m_screenCaptureController, &ScreenCaptureController::captureStopped, this, &MainWindow::onScreenCaptureStopped);
    connect(m_screenCaptureController, &ScreenCaptureController::screenCaptured, this, &MainWindow::onScreenCaptured);

    m_CameraCaptureController = new CameraCaptureController(this);
    connect(m_CameraCaptureController, &CameraCaptureController::cameraCaptured, this, &MainWindow::onCameraCaptured);
    connect(m_CameraCaptureController, &CameraCaptureController::captureStarted, this, &MainWindow::onCameraCaptureStarted);
    connect(m_CameraCaptureController, &CameraCaptureController::captureStopped, this, &MainWindow::onCameraCaptureStopped);
    connect(m_CameraCaptureController, &CameraCaptureController::errorOccurred, this, &MainWindow::onCameraErrorOccurred);

    connect(m_dialogsController, &DialogsController::screenSelected, this, &MainWindow::onScreenSelected);
    connect(m_dialogsController, &DialogsController::screenShareDialogCancelled, [this]() {
        if (m_callWidget) {
            m_callWidget->setScreenShareButtonActive(false);
        }
    });

    QTimer::singleShot(2000, [this]() {
        if (m_updater && m_updater->isAwaitingServerResponse()) {
            if (m_authorizationWidget) {
                m_authorizationWidget->showUpdatesCheckingNotification();
                m_authorizationWidget->setAuthorizationDisabled(true);
            }
        }
    });
}

void MainWindow::setupManagerConnections() {
    // Setup NavigationController
    if (m_navigationController) {
        m_navigationController->setWidgets(m_authorizationWidget, m_mainMenuWidget, m_callWidget);
        connect(m_navigationController, &NavigationController::windowTitleChanged, this, &MainWindow::onWindowTitleChanged);
        connect(m_navigationController, &NavigationController::windowFullscreenRequested, this, &MainWindow::onWindowFullscreenRequested);
        connect(m_navigationController, &NavigationController::windowMaximizedRequested, this, &MainWindow::onWindowMaximizedRequested);
        connect(m_navigationController, &NavigationController::callWidgetActivated, this, &MainWindow::onCallWidgetActivated);
        connect(m_callWidget, &CallWidget::requestEnterFullscreen, m_navigationController, &NavigationController::onCallWidgetEnterFullscreenRequested);
        connect(m_callWidget, &CallWidget::requestExitFullscreen, m_navigationController, &NavigationController::onCallWidgetExitFullscreenRequested);
    }

    // Setup UpdateManager
    if (m_updateManager) {
        m_updateManager->setWidgets(m_authorizationWidget, m_mainMenuWidget, m_dialogsController);
        connect(m_updateManager, &UpdateManager::stopAllRingtonesRequested, this, &MainWindow::onStopAllRingtonesRequested);
    }

    // Setup AuthorizationManager
    if (m_authorizationManager) {
        m_authorizationManager->setAuthorizationWidget(m_authorizationWidget);
        m_authorizationManager->setMainMenuWidget(m_mainMenuWidget);
    }

    // Setup CallManager
    if (m_callManager) {
        m_callManager->setWidgets(m_mainMenuWidget, m_callWidget, m_stackedLayout);
        m_callManager->setScreenCaptureController(m_screenCaptureController);
        m_callManager->setCameraCaptureController(m_CameraCaptureController);
        connect(m_callManager, &CallManager::stopScreenCaptureRequested, this, &MainWindow::onStopScreenCaptureRequested);
        connect(m_callManager, &CallManager::stopCameraCaptureRequested, this, &MainWindow::onStopCameraCaptureRequested);
        connect(m_callManager, &CallManager::endCallFullscreenExitRequested, this, &MainWindow::onEndCallFullscreenExitRequested);
    }

    // Setup ScreenSharingManager
    if (m_screenSharingManager) {
        m_screenSharingManager->setControllers(m_screenCaptureController, m_dialogsController);
        m_screenSharingManager->setWidgets(m_callWidget, statusBar());
        m_screenSharingManager->setCameraCaptureController(m_CameraCaptureController);
        connect(m_screenSharingManager, &ScreenSharingManager::fullscreenExitRequested, this, &MainWindow::onWindowMaximizedRequested);
    }

    // Setup CameraSharingManager
    if (m_cameraSharingManager) {
        m_cameraSharingManager->setControllers(m_CameraCaptureController);
        m_cameraSharingManager->setWidgets(m_callWidget, m_mainMenuWidget, statusBar());
    }

    // Hide status bar to prevent resize handle from appearing
    statusBar()->hide();

    // Setup NetworkErrorHandler
    if (m_networkErrorHandler) {
        m_networkErrorHandler->setWidgets(m_authorizationWidget, m_mainMenuWidget, m_dialogsController);
        m_networkErrorHandler->setAudioManager(m_audioManager);
    }
}

// Delegation methods - these forward to the appropriate managers
void MainWindow::onUpdateButtonClicked() {
    if (m_updateManager) {
        m_updateManager->onUpdateButtonClicked();
    }
}

void MainWindow::onStartCallingButtonClicked(const QString& friendNickname) {
    if (m_callManager) {
        m_callManager->onStartCallingButtonClicked(friendNickname);
    }
}

void MainWindow::onStopCallingButtonClicked() {
    if (m_callManager) {
        m_callManager->onStopCallingButtonClicked();
    }
}

void MainWindow::onAcceptCallButtonClicked(const QString& friendNickname) {
    if (m_callManager) {
        m_callManager->onAcceptCallButtonClicked(friendNickname);
    }
}

void MainWindow::onDeclineCallButtonClicked(const QString& friendNickname) {
    if (m_callManager) {
        m_callManager->onDeclineCallButtonClicked(friendNickname);
    }
}

void MainWindow::onEndCallButtonClicked() {
    if (m_callManager) {
        m_callManager->onEndCallButtonClicked();
    }
}

void MainWindow::onAuthorizationButtonClicked(const QString& friendNickname) {
    if (m_authorizationManager) {
        m_authorizationManager->onAuthorizationButtonClicked(friendNickname);
    }
}

void MainWindow::onBlurAnimationFinished() {
    if (m_authorizationManager) {
        m_authorizationManager->onBlurAnimationFinished();
    }
}

void MainWindow::onRefreshAudioDevicesButtonClicked() {
    if (m_audioSettingsManager) {
        m_audioSettingsManager->onRefreshAudioDevicesButtonClicked();
    }
}

void MainWindow::onInputVolumeChanged(int newVolume) {
    if (m_audioSettingsManager) {
        m_audioSettingsManager->onInputVolumeChanged(newVolume);
    }
}

void MainWindow::onOutputVolumeChanged(int newVolume) {
    if (m_audioSettingsManager) {
        m_audioSettingsManager->onOutputVolumeChanged(newVolume);
    }
}

void MainWindow::onMuteMicrophoneButtonClicked(bool mute) {
    if (m_audioSettingsManager) {
        m_audioSettingsManager->onMuteMicrophoneButtonClicked(mute);
    }
}

void MainWindow::onMuteSpeakerButtonClicked(bool mute) {
    if (m_audioSettingsManager) {
        m_audioSettingsManager->onMuteSpeakerButtonClicked(mute);
    }
}

void MainWindow::onActivateCameraButtonClicked(bool activated) {
    if (m_cameraSharingManager) {
        m_cameraSharingManager->onActivateCameraButtonClicked(activated);
    }
}

void MainWindow::onScreenSelected(int screenIndex) {
    if (m_screenSharingManager) {
        m_screenSharingManager->onScreenSelected(screenIndex);
    }
}

void MainWindow::onScreenShareButtonClicked(bool toggled) {
    if (m_screenSharingManager) {
        m_screenSharingManager->onScreenShareButtonClicked(toggled);
    }
}

void MainWindow::onScreenCaptureStarted() {
    if (m_screenSharingManager) {
        m_screenSharingManager->onScreenCaptureStarted();
    }
}

void MainWindow::onScreenCaptureStopped() {
    if (m_screenSharingManager) {
        m_screenSharingManager->onScreenCaptureStopped();
    }
}

void MainWindow::onScreenCaptured(const QPixmap& pixmap, const std::vector<unsigned char>& imageData) {
    if (m_screenSharingManager) {
        m_screenSharingManager->onScreenCaptured(pixmap, imageData);
    }
}

void MainWindow::onScreenSharingStarted() {
    if (m_screenSharingManager) {
        m_screenSharingManager->onScreenSharingStarted();
    }
}

void MainWindow::onStartScreenSharingError() {
    if (m_screenSharingManager) {
        m_screenSharingManager->onStartScreenSharingError();
    }
}

void MainWindow::onIncomingScreenSharingStarted() {
    if (m_screenSharingManager) {
        m_screenSharingManager->onIncomingScreenSharingStarted();
    }
}

void MainWindow::onIncomingScreenSharingStopped() {
    if (m_screenSharingManager) {
        m_screenSharingManager->onIncomingScreenSharingStopped();
    }
}

void MainWindow::onIncomingScreen(const std::vector<unsigned char>& data) {
    if (m_screenSharingManager) {
        m_screenSharingManager->onIncomingScreen(data);
    }
}

void MainWindow::onCameraButtonClicked(bool toggled) {
    if (m_cameraSharingManager) {
        m_cameraSharingManager->onCameraButtonClicked(toggled);
    }
}

void MainWindow::onCameraSharingStarted() {
    if (m_cameraSharingManager) {
        m_cameraSharingManager->onCameraSharingStarted();
    }
}

void MainWindow::onStartCameraSharingError() {
    if (m_cameraSharingManager) {
        m_cameraSharingManager->onStartCameraSharingError();
    }
}

void MainWindow::onIncomingCameraSharingStarted() {
    if (m_cameraSharingManager) {
        m_cameraSharingManager->onIncomingCameraSharingStarted();
    }
}

void MainWindow::onIncomingCameraSharingStopped() {
    if (m_cameraSharingManager) {
        m_cameraSharingManager->onIncomingCameraSharingStopped();
    }
}

void MainWindow::onIncomingCamera(const std::vector<unsigned char>& data) {
    if (m_cameraSharingManager) {
        m_cameraSharingManager->onIncomingCamera(data);
    }
}

void MainWindow::onCameraCaptured(const QPixmap& pixmap, const std::vector<unsigned char>& imageData) {
    if (m_cameraSharingManager) {
        m_cameraSharingManager->onCameraCaptured(pixmap, imageData);
    }
}

void MainWindow::onCameraCaptureStarted() {
    if (m_cameraSharingManager) {
        m_cameraSharingManager->onCameraCaptureStarted();
    }
}

void MainWindow::onCameraCaptureStopped() {
    if (m_cameraSharingManager) {
        m_cameraSharingManager->onCameraCaptureStopped();
    }
}

void MainWindow::onCameraErrorOccurred(const QString& errorMessage) {
    if (m_cameraSharingManager) {
        m_cameraSharingManager->onCameraErrorOccurred(errorMessage);
    }
}

void MainWindow::onCallWidgetEnterFullscreenRequested() {
    if (m_navigationController) {
        m_navigationController->onCallWidgetEnterFullscreenRequested();
    }
}

void MainWindow::onCallWidgetExitFullscreenRequested() {
    if (m_navigationController) {
        m_navigationController->onCallWidgetExitFullscreenRequested();
    }
}


// Callbacks from ClientCallbacksHandler - delegate to managers
void MainWindow::onAuthorizationResult(callifornia::ErrorCode ec) {
    if (m_authorizationManager) {
        m_authorizationManager->onAuthorizationResult(ec);
    }
}

void MainWindow::onStartCallingResult(callifornia::ErrorCode ec) {
    if (m_callManager) {
        m_callManager->onStartCallingResult(ec);
    }
}

void MainWindow::onAcceptCallResult(callifornia::ErrorCode ec, const QString& nickname) {
    if (m_callManager) {
        m_callManager->onAcceptCallResult(ec, nickname);
    }
}

void MainWindow::onMaximumCallingTimeReached() {
    if (m_callManager) {
        m_callManager->onMaximumCallingTimeReached();
    }
}

void MainWindow::onCallingAccepted() {
    if (m_callManager) {
        m_callManager->onCallingAccepted();
    }
}

void MainWindow::onCallingDeclined() {
    if (m_callManager) {
        m_callManager->onCallingDeclined();
    }
}

void MainWindow::onRemoteUserEndedCall() {
    if (m_callManager) {
        m_callManager->onRemoteUserEndedCall();
    }
}

void MainWindow::onIncomingCall(const QString& friendNickname) {
    if (m_callManager) {
        m_callManager->onIncomingCall(friendNickname);
    }
}

void MainWindow::onIncomingCallExpired(const QString& friendNickname) {
    if (m_callManager) {
        m_callManager->onIncomingCallExpired(friendNickname);
    }
}

void MainWindow::onClientNetworkError() {
    if (m_networkErrorHandler) {
        m_networkErrorHandler->onClientNetworkError();
    }
}

void MainWindow::onUpdaterNetworkError() {
    if (m_networkErrorHandler) {
        m_networkErrorHandler->onUpdaterNetworkError();
    }
}

void MainWindow::onConnectionRestored() {
    if (m_networkErrorHandler) {
        m_networkErrorHandler->onConnectionRestored();
    }
}

// Signal handlers from managers
void MainWindow::onWindowTitleChanged(const QString& title) {
    setWindowTitle(title);
}

void MainWindow::onWindowFullscreenRequested() {
    showFullScreen();
    if (m_callWidget) {
        m_callWidget->enterFullscreen();
    }
}

void MainWindow::onWindowMaximizedRequested() {
    showMaximized();
    if (m_callWidget) {
        m_callWidget->exitFullscreen();
    }
}

void MainWindow::onCallWidgetActivated(const QString& friendNickname) {
    if (m_cameraSharingManager) {
        m_cameraSharingManager->initializeCameraForCall();
    }
}

void MainWindow::onStopScreenCaptureRequested() {
    if (m_screenSharingManager) {
        m_screenSharingManager->stopLocalScreenCapture();
    }
}

void MainWindow::onStopCameraCaptureRequested() {
    if (m_cameraSharingManager) {
        m_cameraSharingManager->stopLocalCameraCapture();
    }
}

void MainWindow::onEndCallFullscreenExitRequested() {
    if (m_callWidget && m_callWidget->isFullScreen()) {
        m_callWidget->exitFullscreen();
        showMaximized();
    }
}

void MainWindow::onStopAllRingtonesRequested() {
    if (m_audioManager) {
        m_audioManager->stopIncomingCallRingtone();
        m_audioManager->stopCallingRingtone();
    }
}