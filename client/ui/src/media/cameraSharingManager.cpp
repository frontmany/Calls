#include "cameraSharingManager.h"
#include "media/cameraCaptureController.h"
#include "media/h264Decoder.h"
#include "managers/dialogsController.h"
#include "managers/notificationController.h"
#include "widgets/callWidget.h"
#include "widgets/mainMenuWidget.h"
#include "managers/configManager.h"
#include "utilities/constant.h"
#include <QTimer>

CameraSharingManager::CameraSharingManager(std::shared_ptr<core::Client> client, ConfigManager* configManager, CameraCaptureController* cameraController, DialogsController* dialogsController, QObject* parent)
    : QObject(parent)
    , m_coreClient(client)
    , m_configManager(configManager)
    , m_cameraCaptureController(cameraController)
    , m_dialogsController(dialogsController)
{
    m_h264Decoder = new H264Decoder();
    m_h264Decoder->initialize();
}

CameraSharingManager::~CameraSharingManager()
{
    if (m_h264Decoder) {
        delete m_h264Decoder;
        m_h264Decoder = nullptr;
    }
}

void CameraSharingManager::setWidgets(CallWidget* callWidget, MainMenuWidget* mainMenuWidget)
{
    m_callWidget = callWidget;
    m_mainMenuWidget = mainMenuWidget;
}

void CameraSharingManager::setNotificationController(NotificationController* notificationController)
{
    m_notificationController = notificationController;
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
    if (!m_cameraCaptureController || !m_callWidget || !m_configManager) return;

    bool shouldStartCamera = m_configManager->isCameraActive() && m_cameraCaptureController->isCameraAvailable();
    m_callWidget->setCameraButtonActive(shouldStartCamera);

    if (shouldStartCamera) {
        if (!m_coreClient) {
            m_callWidget->setCameraButtonActive(false);
        } else {
            std::error_code ec = m_coreClient->startCameraSharing();
            if (ec) {
                m_callWidget->setCameraButtonActive(false);
            }
        }
    }
}

void CameraSharingManager::onCameraButtonClicked(bool toggled)
{
    if (!m_callWidget || !m_coreClient) return;

    bool active = toggled;

    if (active) {
        if (!m_cameraCaptureController || !m_cameraCaptureController->isCameraAvailable()) {
            if (!m_coreClient->isConnectionDown()) {
                if (m_notificationController) {
                    m_notificationController->showErrorNotification("No cameras available", 1500);
                }
            }
            m_callWidget->setCameraButtonActive(false);
            return;
        }

        const std::string friendNickname = m_coreClient->getNicknameInCallWith();
        if (friendNickname.empty()) {
            if (!m_coreClient->isConnectionDown()) {
                if (m_notificationController) {
                    m_notificationController->showErrorNotification("No active call to share camera with", 1500);
                }
            }
            m_callWidget->setCameraButtonActive(false);
            return;
        }

        std::error_code ec = m_coreClient->startCameraSharing();
        if (ec) {
            if (!m_coreClient->isConnectionDown()) {
                if (m_notificationController) {
                    m_notificationController->showErrorNotification("Failed to send camera sharing request", 1500);
                }
            }
            m_callWidget->setCameraButtonActive(false);
        }
        else {
            if (m_callWidget) {
                m_callWidget->setCameraButtonRestricted(true);
            }
            startOperationTimer(core::UserOperationType::START_CAMERA_SHARING, "Starting camera sharing...");
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
            startOperationTimer(core::UserOperationType::STOP_CAMERA_SHARING, "Stopping camera sharing...");
        }
    }
}

void CameraSharingManager::onCameraSharingStarted()
{
    stopOperationTimer(core::UserOperationType::START_CAMERA_SHARING);
    if (m_callWidget) {
        m_callWidget->setCameraButtonRestricted(false);
        m_callWidget->setCameraButtonActive(true);
    }

    if (m_cameraCaptureController) {
        m_cameraCaptureController->startCapture();
    }
}

void CameraSharingManager::onStartCameraSharingError()
{
    stopOperationTimer(core::UserOperationType::START_CAMERA_SHARING);
    if (m_callWidget) {
        m_callWidget->setCameraButtonRestricted(false);
        m_callWidget->setCameraButtonActive(false);
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

    if (m_h264Decoder && m_h264Decoder->isInitialized()) {
        QImage decodedImage = m_h264Decoder->decode(data);
        if (!decodedImage.isNull()) {
            frame = QPixmap::fromImage(decodedImage);
        }
    }

    if (frame.isNull()) {
        const auto* raw = reinterpret_cast<const uchar*>(data.data());
        frame.loadFromData(raw, static_cast<int>(data.size()), "JPG");
    }

    if (frame.isNull()) return;

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

void CameraSharingManager::onCameraErrorOccurred(const QString& errorMessage)
{
    // Stop local camera capture
    if (m_cameraCaptureController && m_cameraCaptureController->isCapturing()) {
        m_cameraCaptureController->stopCapture();
    }
    
    // If camera sharing is active, stop it to notify the remote user
    if (m_coreClient && m_coreClient->isCameraSharing()) {
        std::error_code ec = m_coreClient->stopCameraSharing();
        if (ec) {
            LOG_WARN("Failed to stop camera sharing after camera error: {}", ec.message());
        } else {
            if (m_callWidget) {
                m_callWidget->setCameraButtonRestricted(true);
            }
            startOperationTimer(core::UserOperationType::STOP_CAMERA_SHARING, "Stopping camera sharing...");
        }
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
    stopOperationTimer(core::UserOperationType::STOP_CAMERA_SHARING);
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


void CameraSharingManager::startOperationTimer(core::UserOperationType operationKey, const QString& dialogText)
{
    m_pendingOperationTexts.insert(operationKey, dialogText);

    QTimer* timer = m_operationTimers.value(operationKey, nullptr);
    if (!timer)
    {
        timer = new QTimer(this);
        timer->setSingleShot(true);
        timer->setInterval(TIMER_INTERVAL_MS);
        connect(timer, &QTimer::timeout, this, [this, operationKey]()
        {
            onOperationTimerTimeout(operationKey);
        });
        m_operationTimers.insert(operationKey, timer);
    }

    timer->start();
}

void CameraSharingManager::stopOperationTimer(core::UserOperationType operationKey)
{
    if (QTimer* timer = m_operationTimers.value(operationKey, nullptr))
    {
        timer->stop();
    }

    m_pendingOperationTexts.remove(operationKey);

    if (m_notificationController)
    {
        m_notificationController->hidePendingOperation(operationKey);
    }
}

void CameraSharingManager::stopAllOperationTimers()
{
    if (m_notificationController)
    {
        for (auto it = m_operationTimers.constBegin(); it != m_operationTimers.constEnd(); ++it)
        {
            m_notificationController->hidePendingOperation(it.key());
        }
    }

    for (auto it = m_operationTimers.constBegin(); it != m_operationTimers.constEnd(); ++it)
    {
        if (QTimer* timer = it.value())
        {
            timer->stop();
        }
    }

    m_pendingOperationTexts.clear();
}

void CameraSharingManager::onOperationTimerTimeout(core::UserOperationType operationKey)
{
    if (!m_coreClient || m_coreClient->isConnectionDown())
    {
        return;
    }

    const QString dialogText = m_pendingOperationTexts.value(operationKey);
    if (dialogText.isEmpty())
    {
        return;
    }

    if (m_notificationController)
    {
        m_notificationController->showPendingOperation(dialogText, operationKey);
    }
}

void CameraSharingManager::hideOperationDialog()
{
    stopAllOperationTimers();
}
