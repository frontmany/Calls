#include "screenSharingManager.h"
#include "media/screenCaptureController.h"
#include "managers/dialogsController.h"
#include "widgets/callWidget.h"
#include "media/cameraCaptureController.h"
#include <QTimer>

ScreenSharingManager::ScreenSharingManager(std::shared_ptr<core::Client> client, ScreenCaptureController* screenController, DialogsController* dialogsController, CameraCaptureController* cameraController, QObject* parent)
    : QObject(parent)
    , m_coreClient(client)
    , m_screenCaptureController(screenController)
    , m_dialogsController(dialogsController)
    , m_cameraCaptureController(cameraController)
    , m_operationTimer(new QTimer(this))
{
    m_operationTimer->setSingleShot(true);
    m_operationTimer->setInterval(1000);
    connect(m_operationTimer, &QTimer::timeout, this, &ScreenSharingManager::onTimeToShowWaitingNotification);
}

void ScreenSharingManager::setWidgets(CallWidget* callWidget)
{
    m_callWidget = callWidget;
}

void ScreenSharingManager::stopLocalScreenCapture()
{
    if (!m_screenCaptureController) return;

    if (m_dialogsController) {
        m_dialogsController->hideScreenShareDialog();
    }
    m_screenCaptureController->stopCapture();
}

void ScreenSharingManager::onScreenShareButtonClicked(bool toggled)
{
    if (!m_screenCaptureController || !m_callWidget) return;

    if (toggled) {
        m_screenCaptureController->refreshAvailableScreens();
        m_screenCaptureController->resetSelectedScreenIndex();
        if (m_dialogsController) {
            m_dialogsController->showScreenShareDialog(m_screenCaptureController->availableScreens());
        }
    }
    else {
        if (!m_coreClient) return;

        std::error_code ec = m_coreClient->stopScreenSharing();
        if (ec) {
            if (!m_coreClient->isConnectionDown()) {
                if (m_callWidget) {
                    m_callWidget->setScreenShareButtonActive(true);
                }
            }
        }
        else {
            if (m_callWidget) {
                m_callWidget->setScreenShareButtonRestricted(true);
            }
            startOperationTimer("Stopping screen sharing...");
        }
    }
}

void ScreenSharingManager::onScreenSelected(int screenIndex)
{
    if (!m_screenCaptureController || !m_callWidget) {
        if (m_callWidget) {
            m_callWidget->setScreenShareButtonActive(false);
        }
        return;
    }

    m_screenCaptureController->refreshAvailableScreens();
    m_screenCaptureController->setSelectedScreenIndex(screenIndex);

    if (m_screenCaptureController->selectedScreenIndex() == -1) {
        m_callWidget->setScreenShareButtonActive(false);
        return;
    }

    if (!m_coreClient) {
        m_callWidget->setScreenShareButtonActive(false);
        return;
    }
    
    const std::string friendNickname = m_coreClient->getNicknameInCallWith();
    if (friendNickname.empty()) {
        m_callWidget->setScreenShareButtonActive(false);
        return;
    }

    std::error_code ec = m_coreClient->startScreenSharing();
    if (ec) {
        m_callWidget->setScreenShareButtonActive(false);
        return;
    }
    else {
        if (m_callWidget) {
            m_callWidget->setScreenShareButtonRestricted(true);
        }
        startOperationTimer("Starting screen sharing...");
    }
}

void ScreenSharingManager::onScreenSharingStarted()
{
    stopOperationTimer();
    if (m_callWidget) {
        m_callWidget->setScreenShareButtonRestricted(false);
        m_callWidget->setScreenShareButtonActive(true);
    }

    if (m_screenCaptureController) {
        m_screenCaptureController->startCapture();
    }
}

void ScreenSharingManager::onScreenCaptureStarted()
{
    if (m_callWidget) {
        m_callWidget->hideEnterFullscreenButton();
    }
}

void ScreenSharingManager::onScreenCaptureStopped()
{
    if (m_callWidget) {
        m_callWidget->setScreenShareButtonActive(false);

        if (!m_cameraCaptureController || !m_cameraCaptureController->isCapturing()) {
            m_callWidget->hideMainScreen();
        }
    }
}

void ScreenSharingManager::onScreenCaptured(const QPixmap& pixmap, const std::vector<unsigned char>& imageData)
{
    if (m_callWidget && !pixmap.isNull()) {
        m_callWidget->showFrameInMainScreen(pixmap, Screen::ScaleMode::KeepAspectRatio);
    }

    if (imageData.empty()) return;

    if (!m_coreClient || !m_coreClient->sendScreen(imageData)) {
    }
}

void ScreenSharingManager::onStartScreenSharingError()
{
    stopOperationTimer();
    if (m_callWidget) {
        m_callWidget->setScreenShareButtonRestricted(false);
        m_callWidget->setScreenShareButtonActive(false);
    }
    stopLocalScreenCapture();
    if (m_dialogsController) {
        m_dialogsController->hideScreenShareDialog();
    }
}

void ScreenSharingManager::onIncomingScreenSharingStarted()
{
    if (m_callWidget) {
        m_callWidget->setScreenShareButtonRestricted(true);
        m_callWidget->showEnterFullscreenButton();
    }
}

void ScreenSharingManager::onIncomingScreenSharingStopped()
{
    if (!m_callWidget) return;

    bool wasFullscreen = m_callWidget->isFullScreen();

    if (wasFullscreen) {
        m_callWidget->exitFullscreen();
        emit fullscreenExitRequested();
    }

    m_callWidget->setScreenShareButtonActive(false);
    m_callWidget->hideMainScreen();

    // Note: Camera screen management is handled by CameraSharingManager

    // Hide fullscreen button only if remote camera is not present
    if (!m_coreClient || !m_coreClient->isViewingRemoteCamera()) {
        m_callWidget->hideEnterFullscreenButton();
    }

    if (wasFullscreen) {
        QTimer::singleShot(0, m_callWidget, [this]() {
            m_callWidget->updateMainScreenSize();
        });
    }
}

void ScreenSharingManager::onIncomingScreen(const std::vector<unsigned char>& data)
{
    if (!m_callWidget || data.empty() || !m_coreClient || !m_coreClient->isViewingRemoteScreen()) return;

    QPixmap frame;
    const auto* raw = reinterpret_cast<const uchar*>(data.data());

    if (frame.loadFromData(raw, static_cast<int>(data.size()), "JPG")) {
        m_callWidget->showFrameInMainScreen(frame, Screen::ScaleMode::KeepAspectRatio);
    }
}

void ScreenSharingManager::onStopScreenSharingResult(std::error_code ec)
{
    stopOperationTimer();
    if (m_callWidget) {
        m_callWidget->setScreenShareButtonRestricted(false);
    }
    if (ec) {
        LOG_WARN("Failed to stop screen sharing: {}", ec.message());
        if (m_coreClient && !m_coreClient->isConnectionDown()) {
            if (m_callWidget) {
                m_callWidget->setScreenShareButtonActive(true);
            }
        }
    }
    else {
        stopLocalScreenCapture();
        if (m_callWidget) {
            m_callWidget->setScreenShareButtonActive(false);
        }

        if (!m_cameraCaptureController || !m_cameraCaptureController->isCapturing()) {
            if (m_callWidget) {
                m_callWidget->hideMainScreen();
            }
        }
    }
}


void ScreenSharingManager::startOperationTimer(const QString& dialogText)
{
    m_pendingOperationDialogText = dialogText;
    m_operationTimer->start();
}

void ScreenSharingManager::stopOperationTimer()
{
    m_operationTimer->stop();
    m_pendingOperationDialogText.clear();
    if (m_dialogsController) {
        m_dialogsController->hideNotificationDialog();
    }
}

void ScreenSharingManager::hideOperationDialog()
{
    stopOperationTimer();
}

void ScreenSharingManager::onTimeToShowWaitingNotification()
{
    if (m_coreClient && !m_coreClient->isConnectionDown() && !m_pendingOperationDialogText.isEmpty()) {
        if (m_dialogsController) {
            m_dialogsController->showNotificationDialog(m_pendingOperationDialogText, false, false);
        }
    }
}
