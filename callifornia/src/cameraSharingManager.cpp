#include "cameraSharingManager.h"
#include "CameraCaptureController.h"
#include "callWidget.h"
#include "mainMenuWidget.h"
#include "configManager.h"
#include "calls.h"

CameraSharingManager::CameraSharingManager(ConfigManager* configManager, QObject* parent)
    : QObject(parent)
    , m_configManager(configManager)
{
}

void CameraSharingManager::setControllers(CameraCaptureController* cameraController)
{
    m_cameraCaptureController = cameraController;
}

void CameraSharingManager::setWidgets(CallWidget* callWidget, MainMenuWidget* mainMenuWidget, QStatusBar* statusBar)
{
    m_callWidget = callWidget;
    m_mainMenuWidget = mainMenuWidget;
    m_statusBar = statusBar;
}

void CameraSharingManager::stopLocalCameraCapture()
{
    if (!m_cameraCaptureController) return;

    if (m_cameraCaptureController->isCapturing()) {
        m_cameraCaptureController->stopCapture();
    }

    calls::stopCameraSharing();
}

void CameraSharingManager::initializeCameraForCall()
{
    if (!m_cameraCaptureController || !m_callWidget || !m_configManager || !m_mainMenuWidget) return;

    bool shouldStartCamera = m_configManager->isCameraActive() && m_cameraCaptureController->isCameraAvailable();
    m_configManager->setCameraActive(shouldStartCamera);
    m_mainMenuWidget->setCameraActive(shouldStartCamera);
    m_callWidget->setCameraButtonActive(shouldStartCamera);

    if (shouldStartCamera) {
        if (!calls::startCameraSharing()) {
            m_configManager->setCameraActive(false);
            m_mainMenuWidget->setCameraActive(false);
            m_callWidget->setCameraButtonActive(false);
        }
    }
}

void CameraSharingManager::onCameraButtonClicked(bool toggled)
{
    if (!m_callWidget || !m_configManager || !m_mainMenuWidget) return;

    bool active = toggled;

    m_configManager->setCameraActive(active);
    m_mainMenuWidget->setCameraActive(active);

    if (active) {
        if (!m_cameraCaptureController || !m_cameraCaptureController->isCameraAvailable()) {
            m_callWidget->showErrorNotification("No cameras available", 1500);
            m_callWidget->setCameraButtonActive(false);
            return;
        }

        const std::string friendNickname = calls::getNicknameInCallWith();
        if (friendNickname.empty()) {
            m_callWidget->showErrorNotification("No active call to share camera with", 1500);
            m_callWidget->setCameraButtonActive(false);
            return;
        }

        if (!calls::startCameraSharing()) {
            m_callWidget->showErrorNotification("Failed to send camera sharing request", 1500);
            m_callWidget->setCameraButtonActive(false);
            return;
        }
    }
    else {
        if (m_cameraCaptureController && m_cameraCaptureController->isCapturing()) {
            m_cameraCaptureController->stopCapture();
        }

        if (m_callWidget->isMainScreenVisible() && (!calls::isViewingRemoteScreen() && !calls::isViewingRemoteCamera())) {
            m_callWidget->hideMainScreen();
        }

        if (m_callWidget->isAdditionalScreenVisible(calls::getNickname()) && (calls::isViewingRemoteScreen() || calls::isViewingRemoteCamera())) {
            m_callWidget->removeAdditionalScreen(calls::getNickname());
        }

        calls::stopCameraSharing();
    }
}

void CameraSharingManager::onCameraSharingStarted()
{
    if (m_cameraCaptureController) {
        m_cameraCaptureController->startCapture();
    }
}

void CameraSharingManager::onStartCameraSharingError()
{
    showTransientStatusMessage("Camera sharing rejected by server", 3000);
    if (m_callWidget) {
        m_callWidget->setCameraButtonActive(false);
    }
}

void CameraSharingManager::onCameraCaptureStarted()
{
    LOG_INFO("Camera capture started locally");
}

void CameraSharingManager::onCameraCaptureStopped()
{
    if (!m_callWidget) return;

    if (m_isCameraInAdditionalScreen) {
        m_callWidget->removeAdditionalScreen(calls::getNickname());
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
        bool shouldBeInAdditionalScreen = calls::isViewingRemoteScreen() || calls::isViewingRemoteCamera() || calls::isScreenSharing();

        if (shouldBeInAdditionalScreen) {
            if (!m_isCameraInAdditionalScreen) {
                m_callWidget->hideMainScreen();
                m_isCameraInAdditionalScreen = true;
            }
            m_callWidget->showFrameInAdditionalScreen(pixmap, calls::getNickname());
        }
        else {
            if (m_isCameraInAdditionalScreen) {
                m_callWidget->removeAdditionalScreen(calls::getNickname());
                m_isCameraInAdditionalScreen = false;
            }
            m_callWidget->showFrameInMainScreen(pixmap, Screen::ScaleMode::CropToFit);
        }
    }

    if (imageData.empty()) return;

    if (!calls::sendCamera(imageData)) {
        showTransientStatusMessage("Failed to send camera frame", 2000);
    }
}

void CameraSharingManager::onIncomingCameraSharingStarted()
{
    if (!m_callWidget) return;

    bool shouldBeInAdditionalScreen = calls::isScreenSharing() || calls::isViewingRemoteScreen();

    if (!shouldBeInAdditionalScreen) {
        m_callWidget->showEnterFullscreenButton();
    }
}

void CameraSharingManager::onIncomingCameraSharingStopped()
{
    if (!m_callWidget) return;

    if (m_isRemoteCameraInAdditionalScreen) {
        m_callWidget->removeAdditionalScreen(calls::getNicknameInCallWith());
        m_isRemoteCameraInAdditionalScreen = false;
    }
    else {
        m_callWidget->hideMainScreen();
    }

    // Hide fullscreen button if no remote content to view
    if (!calls::isViewingRemoteScreen() && !calls::isViewingRemoteCamera()) {
        m_callWidget->hideEnterFullscreenButton();
    }
}

void CameraSharingManager::onIncomingCamera(const std::vector<unsigned char>& data)
{
    if (!m_callWidget || data.empty() || !calls::isViewingRemoteCamera()) return;

    QPixmap frame;
    const auto* raw = reinterpret_cast<const uchar*>(data.data());

    if (frame.loadFromData(raw, static_cast<int>(data.size()), "JPG")) {
        bool shouldBeInAdditionalScreen = calls::isScreenSharing() || calls::isViewingRemoteScreen();

        if (shouldBeInAdditionalScreen) {
            if (!m_isRemoteCameraInAdditionalScreen) {
                m_callWidget->hideMainScreen();
                m_isRemoteCameraInAdditionalScreen = true;
            }
            m_callWidget->showFrameInAdditionalScreen(frame, calls::getNicknameInCallWith());
        }
        else {
            if (m_isRemoteCameraInAdditionalScreen) {
                m_callWidget->removeAdditionalScreen(calls::getNicknameInCallWith());
                m_isRemoteCameraInAdditionalScreen = false;
            }
            m_callWidget->showFrameInMainScreen(frame, Screen::ScaleMode::CropToFit);
        }
    }
}

void CameraSharingManager::onCameraErrorOccurred(const QString& errorMessage)
{
    showTransientStatusMessage(errorMessage, 3000);
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

void CameraSharingManager::showTransientStatusMessage(const QString& message, int durationMs)
{
    if (m_statusBar) {
        m_statusBar->showMessage(message, durationMs);
    }
}
