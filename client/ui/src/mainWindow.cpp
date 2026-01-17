#include "mainWindow.h"
#include <QApplication>
#include <QTimer>
#include <QCloseEvent>

#include "widgets/authorizationWidget.h"
#include "widgets/mainMenuWidget.h"
#include "widgets/callWidget.h"
#include "managers/dialogsController.h"
#include "media/screenCaptureController.h"
#include "media/cameraCaptureController.h"
#include "managers/configManager.h"
#include "utilities/logger.h"
#include "utilities/utilities.h"

#include "media/audioEffectsManager.h"
#include "media/audioSettingsManager.h"
#include "managers/updateManager.h"
#include "managers/navigationController.h"
#include "managers/authorizationManager.h"
#include "managers/coreNetworkErrorHandler.h"
#include "managers/updaterNetworkErrorHandler.h"
#include "managers/callManager.h"
#include "media/screenSharingManager.h"
#include "media/cameraSharingManager.h"
#include "media/audioDevicesWatcher.h"

#include "events/coreEventListener.h"
#include "events/updaterEventListener.h"

void MainWindow::init() {
    setWindowIcon(QIcon(":/resources/callifornia.ico"));

    m_configManager = new ConfigManager("config.json");
    m_configManager->loadConfig();

    m_coreClient = std::make_shared<core::Client>();

    m_updaterClient = std::make_shared<updater::Client>();

    loadFonts();

    initializeCentralStackedWidget();
    initializeAuthorizationWidget();
    initializeMainMenuWidget();
    initializeCallWidget();

    initializeScreenCaptureController();
    initializeCameraCaptureController();
    initializeDialogsController();
    initializeAudioManager();
    initializeAudioDevicesWatcher();
    initializeAudioSettingsManager();
    initializeNavigationController();
    initializeUpdateManager();
    initializeAuthorizationManager();
    initializeCallManager();
    initializeScreenSharingManager();
    initializeCameraSharingManager();
    initializeCoreNetworkErrorHandler();
    initializeUpdaterNetworkErrorHandler();
    
    // Initialize updater client after both error handlers are ready
    m_updaterClient->init(std::make_shared<UpdaterEventListener>(m_updateManager, m_updaterNetworkErrorHandler),
        m_configManager->getTemporaryUpdateDirectoryName().toStdString(),
        m_configManager->getDeletionListFileName().toStdString(),
        m_configManager->getIgnoredFilesWhileCollectingForUpdate(),
        m_configManager->getIgnoredDirectoriesWhileCollectingForUpdate()
    );

    connectWidgetsToManagers();
    applyAudioSettings();

    showMaximized();
}

void MainWindow::showEvent(QShowEvent* event) {
    if (!isFirstInstance() && !m_configManager->isMultiInstanceAllowed()) {
        m_dialogsController->showAlreadyRunningDialog();
    }
    else if (!m_clientsStarted) {
        m_coreClient->start(m_configManager->getServerHost().toStdString(), m_configManager->getPort().toStdString(), 
            std::make_shared<CoreEventListener>(m_authorizationManager, m_callManager, m_screenSharingManager, m_cameraSharingManager, m_coreNetworkErrorHandler));
        m_updaterClient->start(m_configManager->getUpdaterHost().toStdString(), m_configManager->getPort().toStdString());
        m_clientsStarted = true;
    }
}

void MainWindow::closeEvent(QCloseEvent* event) {
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
            event->ignore();
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

void MainWindow::initializeScreenCaptureController() {
    m_screenCaptureController = new ScreenCaptureController(this);
}

void MainWindow::initializeCameraCaptureController() {
    m_CameraCaptureController = new CameraCaptureController(this);
}

void MainWindow::initializeDialogsController() {
    m_dialogsController = new DialogsController(this);
    connect(m_dialogsController, &DialogsController::closeRequested, this, &MainWindow::close);
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
    m_callManager = new CallManager(m_coreClient, m_audioManager, m_navigationController, m_screenCaptureController, m_CameraCaptureController, m_dialogsController, this);
    if (m_mainMenuWidget && m_callWidget && m_stackedLayout) {
        m_callManager->setWidgets(m_mainMenuWidget, m_callWidget, m_stackedLayout);
    }
}

void MainWindow::initializeScreenSharingManager() {
    m_screenSharingManager = new ScreenSharingManager(m_coreClient, m_screenCaptureController, m_dialogsController, m_CameraCaptureController, this);
    if (m_callWidget) {
        m_screenSharingManager->setWidgets(m_callWidget);
    }
}

void MainWindow::initializeCameraSharingManager() {
    m_cameraSharingManager = new CameraSharingManager(m_coreClient, m_configManager, m_CameraCaptureController, m_dialogsController, this);
    if (m_callWidget && m_mainMenuWidget) {
        m_cameraSharingManager->setWidgets(m_callWidget, m_mainMenuWidget);
    }
}

void MainWindow::initializeCoreNetworkErrorHandler() {
    m_coreNetworkErrorHandler = new CoreNetworkErrorHandler(m_coreClient, m_navigationController, m_configManager, m_audioManager, this);
    if (m_authorizationWidget && m_mainMenuWidget && m_dialogsController) {
        m_coreNetworkErrorHandler->setWidgets(m_authorizationWidget, m_mainMenuWidget, m_dialogsController);
    }
    if (m_callManager && m_screenSharingManager && m_cameraSharingManager) {
        m_coreNetworkErrorHandler->setManagers(m_callManager, m_screenSharingManager, m_cameraSharingManager);
    }
}

void MainWindow::initializeUpdaterNetworkErrorHandler() {
    m_updaterNetworkErrorHandler = new UpdaterNetworkErrorHandler(m_coreClient, m_updaterClient, m_navigationController, m_updateManager, m_configManager, this);
    if (m_authorizationWidget && m_mainMenuWidget && m_dialogsController) {
        m_updaterNetworkErrorHandler->setWidgets(m_authorizationWidget, m_mainMenuWidget, m_dialogsController);
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
        if (m_updateManager) {
            connect(m_authorizationWidget, &AuthorizationWidget::updateButtonClicked, m_updateManager, &UpdateManager::onUpdateButtonClicked);
        }
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
        if (m_dialogsController) {
            connect(m_mainMenuWidget, &MainMenuWidget::audioDevicePickerRequested, this, [this]() {
                if (m_dialogsController && m_configManager) {
                    const bool micMuted = m_configManager->isMicrophoneMuted();
                    const bool speakerMuted = m_configManager->isSpeakerMuted();
                    const int inputVolume = m_configManager->getInputVolume();
                    const int outputVolume = m_configManager->getOutputVolume();
                    const int currentInputDevice = (m_coreClient) ? m_coreClient->getCurrentInputDevice() : -1;
                    const int currentOutputDevice = (m_coreClient) ? m_coreClient->getCurrentOutputDevice() : -1;

                    m_dialogsController->showAudioSettingsDialog(
                        false,
                        micMuted,
                        speakerMuted,
                        inputVolume,
                        outputVolume,
                        currentInputDevice,
                        currentOutputDevice);
                }
            });
        }
        if (m_updateManager) {
            connect(m_mainMenuWidget, &MainMenuWidget::updateButtonClicked, m_updateManager, &UpdateManager::onUpdateButtonClicked);
        }
        if (m_callManager) {
            connect(m_mainMenuWidget, &MainMenuWidget::startCallingButtonClicked, m_callManager, &CallManager::onStartCallingButtonClicked);
            connect(m_mainMenuWidget, &MainMenuWidget::stopCallingButtonClicked, m_callManager, &CallManager::onStopCallingButtonClicked);
        }
        if (m_cameraSharingManager) {
            connect(m_mainMenuWidget, &MainMenuWidget::activateCameraClicked, m_cameraSharingManager, &CameraSharingManager::onActivateCameraButtonClicked);
        }
    }

    // CallWidget connections
    if (m_callWidget) {
        if (m_navigationController) {
            connect(m_callWidget, &CallWidget::requestEnterFullscreen, m_navigationController, &NavigationController::onCallWidgetEnterFullscreenRequested);
            connect(m_callWidget, &CallWidget::requestExitFullscreen, m_navigationController, &NavigationController::onCallWidgetExitFullscreenRequested);
        }
        if (m_dialogsController) {
            connect(m_callWidget, &CallWidget::audioSettingsRequested, this, [this](bool micMuted, bool speakerMuted, int inputVolume, int outputVolume) {
                if (m_dialogsController) {
                    const bool micMutedValue = (m_configManager) ? m_configManager->isMicrophoneMuted() : micMuted;
                    const bool speakerMutedValue = (m_configManager) ? m_configManager->isSpeakerMuted() : speakerMuted;
                    const int inputVolumeValue = (m_configManager) ? m_configManager->getInputVolume() : inputVolume;
                    const int outputVolumeValue = (m_configManager) ? m_configManager->getOutputVolume() : outputVolume;
                    const int currentInputDevice = (m_coreClient) ? m_coreClient->getCurrentInputDevice() : -1;
                    const int currentOutputDevice = (m_coreClient) ? m_coreClient->getCurrentOutputDevice() : -1;

                    m_dialogsController->showAudioSettingsDialog(
                        true,
                        micMutedValue,
                        speakerMutedValue,
                        inputVolumeValue,
                        outputVolumeValue,
                        currentInputDevice,
                        currentOutputDevice);
                }
            });
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
        if (m_screenSharingManager) {
            connect(m_callWidget, &CallWidget::screenShareClicked, m_screenSharingManager, &ScreenSharingManager::onScreenShareButtonClicked);
        }
        if (m_cameraSharingManager) {
            connect(m_callWidget, &CallWidget::cameraClicked, m_cameraSharingManager, &CameraSharingManager::onCameraButtonClicked);
        }
    }

    // ScreenCaptureController connections
    if (m_screenCaptureController && m_screenSharingManager) {
        connect(m_screenCaptureController, &ScreenCaptureController::captureStarted, m_screenSharingManager, &ScreenSharingManager::onScreenCaptureStarted);
        connect(m_screenCaptureController, &ScreenCaptureController::captureStopped, m_screenSharingManager, &ScreenSharingManager::onScreenCaptureStopped);
        connect(m_screenCaptureController, &ScreenCaptureController::screenCaptured, m_screenSharingManager, &ScreenSharingManager::onScreenCaptured);
    }

    // CameraCaptureController connections
    if (m_CameraCaptureController && m_cameraSharingManager) {
        connect(m_CameraCaptureController, &CameraCaptureController::cameraCaptured, m_cameraSharingManager, &CameraSharingManager::onCameraCaptured);
        connect(m_CameraCaptureController, &CameraCaptureController::captureStarted, m_cameraSharingManager, &CameraSharingManager::onCameraCaptureStarted);
        connect(m_CameraCaptureController, &CameraCaptureController::captureStopped, m_cameraSharingManager, &CameraSharingManager::onCameraCaptureStopped);
        connect(m_CameraCaptureController, &CameraCaptureController::errorOccurred, m_cameraSharingManager, &CameraSharingManager::onCameraErrorOccurred);
    }

    // DialogsController connections
    if (m_dialogsController && m_screenSharingManager) {
        connect(m_dialogsController, &DialogsController::screenSelected, m_screenSharingManager, &ScreenSharingManager::onScreenSelected);
        connect(m_dialogsController, &DialogsController::screenShareDialogCancelled, [this]() {
            if (m_callWidget) {
                m_callWidget->setScreenShareButtonActive(false);
            }
        });
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

    // Manager to MainWindow connections
    if (m_navigationController) {
        connect(m_navigationController, &NavigationController::windowTitleChanged, this, &MainWindow::onWindowTitleChanged);
        connect(m_navigationController, &NavigationController::windowFullscreenRequested, this, &MainWindow::onWindowFullscreenRequested);
        connect(m_navigationController, &NavigationController::windowMaximizedRequested, this, &MainWindow::onWindowMaximizedRequested);
        if (m_cameraSharingManager) {
            connect(m_navigationController, &NavigationController::callWidgetShown, m_cameraSharingManager, &CameraSharingManager::initializeCameraForCall);
        }
    }
    if (m_updateManager) {
        connect(m_updateManager, &UpdateManager::stopAllRingtonesRequested, this, &MainWindow::onStopAllRingtonesRequested);
    }
    if (m_callManager) {
        connect(m_callManager, &CallManager::stopScreenCaptureRequested, this, &MainWindow::onStopScreenCaptureRequested);
        connect(m_callManager, &CallManager::stopCameraCaptureRequested, this, &MainWindow::onStopCameraCaptureRequested);
        connect(m_callManager, &CallManager::endCallFullscreenExitRequested, this, &MainWindow::onEndCallFullscreenExitRequested);
    }
    if (m_screenSharingManager) {
        connect(m_screenSharingManager, &ScreenSharingManager::fullscreenExitRequested, this, &MainWindow::onWindowMaximizedRequested);
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

void MainWindow::onStopScreenCaptureRequested()
{
    if (m_screenSharingManager) {
        m_screenSharingManager->stopLocalScreenCapture();
    }
}

void MainWindow::onStopCameraCaptureRequested()
{
    if (m_cameraSharingManager) {
        m_cameraSharingManager->stopLocalCameraCapture();
    }
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
        m_audioManager->stopCallingRingtone();
        m_audioManager->stopIncomingCallRingtone();
    }
}