#include "cameraSharingManager.h"
#include "media/cameraCaptureController.h"
#include "managers/dialogsController.h"
#include "widgets/callWidget.h"
#include "widgets/mainMenuWidget.h"
#include "managers/configManager.h"
#include <QTimer>

CameraSharingManager::CameraSharingManager(std::shared_ptr<core::Client> client, ConfigManager* configManager, CameraCaptureController* cameraController, DialogsController* dialogsController, QObject* parent)
    : QObject(parent)
    , m_coreClient(client)
    , m_configManager(configManager)
    , m_cameraCaptureController(cameraController)
    , m_dialogsController(dialogsController)
    , m_operationTimer(new QTimer(this))
{
    m_operationTimer->setSingleShot(true);
    m_operationTimer->setInterval(1000);
    connect(m_operationTimer, &QTimer::timeout, this, &CameraSharingManager::onTimeToShowWaitingNotification);
}

void CameraSharingManager::setWidgets(CallWidget* callWidget, MainMenuWidget* mainMenuWidget)
{
    m_callWidget = callWidget;
    m_mainMenuWidget = mainMenuWidget;
}

void CameraSharingManager::stopLocalCameraCapture()
{
    if (!m_cameraCaptureController) return;

    if (m_cameraCaptureController->isCapturing()) {
        m_cameraCaptureController->stopCapture();
    }
}

void CameraSharingManager::initializeCameraForCall()
{
    if (!m_cameraCaptureController || !m_callWidget || !m_configManager || !m_mainMenuWidget) return;

    bool shouldStartCamera = m_configManager->isCameraActive() && m_cameraCaptureController->isCameraAvailable();
    m_configManager->setCameraActive(shouldStartCamera);
    m_mainMenuWidget->setCameraActive(shouldStartCamera);
    m_callWidget->setCameraButtonActive(shouldStartCamera);

    if (shouldStartCamera) {
        if (!m_coreClient) {
            m_configManager->setCameraActive(false);
            m_mainMenuWidget->setCameraActive(false);
            m_callWidget->setCameraButtonActive(false);
        } else {
            std::error_code ec = m_coreClient->startCameraSharing();
            if (ec) {
                m_configManager->setCameraActive(false);
                m_mainMenuWidget->setCameraActive(false);
                m_callWidget->setCameraButtonActive(false);
            }
        }
    }
}

void CameraSharingManager::onCameraButtonClicked(bool toggled)
{
    if (!m_callWidget || !m_configManager || !m_mainMenuWidget || !m_coreClient) return;

    bool active = toggled;

    if (active) {
        if (!m_cameraCaptureController || !m_cameraCaptureController->isCameraAvailable()) {
            if (!m_coreClient->isConnectionDown()) {
                m_callWidget->showErrorNotification("No cameras available", 1500);
            }
            m_callWidget->setCameraButtonActive(false);
            return;
        }

        const std::string friendNickname = m_coreClient->getNicknameInCallWith();
        if (friendNickname.empty()) {
            if (!m_coreClient->isConnectionDown()) {
                m_callWidget->showErrorNotification("No active call to share camera with", 1500);
            }
            m_callWidget->setCameraButtonActive(false);
            return;
        }

        std::error_code ec = m_coreClient->startCameraSharing();
        if (ec) {
            if (!m_coreClient->isConnectionDown()) {
                m_callWidget->showErrorNotification("Failed to send camera sharing request", 1500);
            }
            m_callWidget->setCameraButtonActive(false);
        }
        else {
            if (m_callWidget) {
                m_callWidget->setCameraButtonRestricted(true);
            }
            startOperationTimer("Starting camera sharing...");
        }
    }
    else {
        std::error_code ec = m_coreClient->stopCameraSharing();
        if (ec) {
            if (!m_coreClient->isConnectionDown()) {
                if (m_callWidget) {
                    m_callWidget->setCameraButtonActive(true);
                }
            }
        }
        else {
            if (m_callWidget) {
                m_callWidget->setCameraButtonRestricted(true);
            }
            startOperationTimer("Stopping camera sharing...");
        }
    }
}

void CameraSharingManager::onCameraSharingStarted()
{
    stopOperationTimer();
    if (m_callWidget) {
        m_callWidget->setCameraButtonRestricted(false);
        m_callWidget->setCameraButtonActive(true);
    }
    if (m_configManager) {
        m_configManager->setCameraActive(true);
    }
    if (m_mainMenuWidget) {
        m_mainMenuWidget->setCameraActive(true);
    }

    if (m_cameraCaptureController) {
        m_cameraCaptureController->startCapture();
    }
}

void CameraSharingManager::onStartCameraSharingError()
{
    stopOperationTimer();
    if (m_callWidget) {
        m_callWidget->setCameraButtonRestricted(false);
        m_callWidget->setCameraButtonActive(false);
    }
    if (m_configManager) {
        m_configManager->setCameraActive(false);
    }
    if (m_mainMenuWidget) {
        m_mainMenuWidget->setCameraActive(false);
    }
}

void CameraSharingManager::onCameraCaptureStarted()
{
}

void CameraSharingManager::onCameraCaptureStopped()
{
    if (!m_callWidget) return;

    if (m_isCameraInAdditionalScreen) {
        if (m_coreClient) {
            m_callWidget->removeAdditionalScreen(m_coreClient->getMyNickname());
        }
        m_isCameraInAdditionalScreen = false;
    }
    else {
        m_callWidget->hideMainScreen();
    }
}

void CameraSharingManager::onCameraCaptured(const QPixmap& pixmap, const std::vector<unsigned char>& imageData)
{
    if (!m_callWidget) return;

    if (!pixmap.isNull()) {
        if (!m_coreClient) return;
        bool shouldBeInAdditionalScreen = m_coreClient->isViewingRemoteScreen() || m_coreClient->isViewingRemoteCamera() || m_coreClient->isScreenSharing();

        if (shouldBeInAdditionalScreen) {
            if (!m_isCameraInAdditionalScreen) {
                m_callWidget->hideMainScreen();
                m_isCameraInAdditionalScreen = true;
            }
            m_callWidget->showFrameInAdditionalScreen(pixmap, m_coreClient->getMyNickname());
        }
        else {
            if (m_isCameraInAdditionalScreen) {
                m_callWidget->removeAdditionalScreen(m_coreClient->getMyNickname());
                m_isCameraInAdditionalScreen = false;
            }
            m_callWidget->showFrameInMainScreen(pixmap, Screen::ScaleMode::CropToFit);
        }
    }

    if (imageData.empty()) return;

    if (!m_coreClient || !m_coreClient->sendCamera(imageData)) {
    }
}

void CameraSharingManager::onIncomingCameraSharingStarted()
{
    if (!m_callWidget) return;

    if (!m_coreClient) return;
    bool shouldBeInAdditionalScreen = m_coreClient->isScreenSharing() || m_coreClient->isViewingRemoteScreen();

    if (!shouldBeInAdditionalScreen) {
        m_callWidget->showEnterFullscreenButton();
    }
}

void CameraSharingManager::onIncomingCameraSharingStopped()
{
    if (!m_callWidget) return;

    if (m_isRemoteCameraInAdditionalScreen) {
        if (m_coreClient) {
            m_callWidget->removeAdditionalScreen(m_coreClient->getNicknameInCallWith());
        }
        m_isRemoteCameraInAdditionalScreen = false;
    }
    else {
        m_callWidget->hideMainScreen();
    }

    // Hide fullscreen button if no remote content to view
    if (!m_coreClient || (!m_coreClient->isViewingRemoteScreen() && !m_coreClient->isViewingRemoteCamera())) {
        m_callWidget->hideEnterFullscreenButton();
    }
}

void CameraSharingManager::onIncomingCamera(const std::vector<unsigned char>& data)
{
    if (!m_callWidget || data.empty() || !m_coreClient || !m_coreClient->isViewingRemoteCamera()) return;

    QPixmap frame;
    const auto* raw = reinterpret_cast<const uchar*>(data.data());

    if (frame.loadFromData(raw, static_cast<int>(data.size()), "JPG")) {
        bool shouldBeInAdditionalScreen = m_coreClient->isScreenSharing() || m_coreClient->isViewingRemoteScreen();

        if (shouldBeInAdditionalScreen) {
            if (!m_isRemoteCameraInAdditionalScreen) {
                m_callWidget->hideMainScreen();
                m_isRemoteCameraInAdditionalScreen = true;
            }
            m_callWidget->showFrameInAdditionalScreen(frame, m_coreClient->getNicknameInCallWith());
        }
        else {
            if (m_isRemoteCameraInAdditionalScreen) {
                m_callWidget->removeAdditionalScreen(m_coreClient->getNicknameInCallWith());
                m_isRemoteCameraInAdditionalScreen = false;
            }
            m_callWidget->showFrameInMainScreen(frame, Screen::ScaleMode::CropToFit);
        }
    }
}

void CameraSharingManager::onCameraErrorOccurred(const QString& errorMessage)
{
    if (m_cameraCaptureController && m_cameraCaptureController->isCapturing()) {
        m_cameraCaptureController->stopCapture();
    }
    if (m_callWidget) {
        m_callWidget->setCameraButtonActive(false);
    }
}

void CameraSharingManager::onActivateCameraButtonClicked(bool activated)
{
    if (m_configManager) {
        m_configManager->setCameraActive(activated);
    }
}

void CameraSharingManager::onStopCameraSharingResult(std::error_code ec)
{
    stopOperationTimer();
    if (m_callWidget) {
        m_callWidget->setCameraButtonRestricted(false);
    }
    if (ec) {
        LOG_WARN("Failed to stop camera sharing: {}", ec.message());
        if (m_coreClient && !m_coreClient->isConnectionDown()) {
            if (m_callWidget) {
                m_callWidget->setCameraButtonActive(true);
            }
        }
    }
    else {
        if (m_cameraCaptureController && m_cameraCaptureController->isCapturing()) {
            m_cameraCaptureController->stopCapture();
        }

        if (m_configManager) {
            m_configManager->setCameraActive(false);
        }
        if (m_mainMenuWidget) {
            m_mainMenuWidget->setCameraActive(false);
        }
        if (m_callWidget) {
            m_callWidget->setCameraButtonActive(false);
        }

        if (m_callWidget && m_coreClient) {
            if (m_callWidget->isMainScreenVisible() && (!m_coreClient->isViewingRemoteScreen() && !m_coreClient->isViewingRemoteCamera())) {
                m_callWidget->hideMainScreen();
            }

            if (m_callWidget->isAdditionalScreenVisible(m_coreClient->getMyNickname()) && (m_coreClient->isViewingRemoteScreen() || m_coreClient->isViewingRemoteCamera())) {
                m_callWidget->removeAdditionalScreen(m_coreClient->getMyNickname());
            }
        }
    }
}


void CameraSharingManager::startOperationTimer(const QString& dialogText)
{
    m_pendingOperationDialogText = dialogText;
    m_operationTimer->start();
}

void CameraSharingManager::stopOperationTimer()
{
    m_operationTimer->stop();
    m_pendingOperationDialogText.clear();
    if (m_dialogsController) {
        m_dialogsController->hideNotificationDialog();
    }
}

void CameraSharingManager::hideOperationDialog()
{
    stopOperationTimer();
}

void CameraSharingManager::onTimeToShowWaitingNotification()
{
    if (m_coreClient && !m_coreClient->isConnectionDown() && !m_pendingOperationDialogText.isEmpty()) {
        if (m_dialogsController) {

            m_dialogsController->showNotificationDialog(m_pendingOperationDialogText, false, false);
        }
    }
}
