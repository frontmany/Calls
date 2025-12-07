#include "screenSharingManager.h"
#include "screenCaptureController.h"
#include "dialogsController.h"
#include "callWidget.h"
#include "CameraCaptureController.h"
#include "calls.h"
#include <QTimer>

ScreenSharingManager::ScreenSharingManager(QObject* parent)
    : QObject(parent)
{
}

void ScreenSharingManager::setControllers(ScreenCaptureController* screenController, DialogsController* dialogsController)
{
    m_screenCaptureController = screenController;
    m_dialogsController = dialogsController;
}

void ScreenSharingManager::setWidgets(CallWidget* callWidget, QStatusBar* statusBar)
{
    m_callWidget = callWidget;
    m_statusBar = statusBar;
}

void ScreenSharingManager::setCameraCaptureController(CameraCaptureController* cameraController)
{
    m_cameraCaptureController = cameraController;
}

void ScreenSharingManager::stopLocalScreenCapture()
{
    if (!m_screenCaptureController) return;

    if (m_dialogsController) {
        m_dialogsController->hideScreenShareDialog();
    }
    m_screenCaptureController->stopCapture();

    calls::stopScreenSharing();
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
        stopLocalScreenCapture();
        m_callWidget->setScreenShareButtonActive(false);

        if (!m_cameraCaptureController || !m_cameraCaptureController->isCapturing()) {
            m_callWidget->hideMainScreen();
        }
    }
}

void ScreenSharingManager::onScreenSelected(int screenIndex)
{
    if (!m_screenCaptureController || !m_callWidget) {
        showTransientStatusMessage("Unable to start screen sharing", 2000);
        if (m_callWidget) {
            m_callWidget->setScreenShareButtonActive(false);
        }
        return;
    }

    m_screenCaptureController->refreshAvailableScreens();
    m_screenCaptureController->setSelectedScreenIndex(screenIndex);

    if (m_screenCaptureController->selectedScreenIndex() == -1) {
        showTransientStatusMessage("Selected screen is no longer available", 3000);
        m_callWidget->setScreenShareButtonActive(false);
        return;
    }

    const std::string friendNickname = calls::getNicknameInCallWith();
    if (friendNickname.empty()) {
        showTransientStatusMessage("No active call to share screen with", 3000);
        m_callWidget->setScreenShareButtonActive(false);
        return;
    }

    if (!calls::startScreenSharing()) {
        showTransientStatusMessage("Failed to send screen sharing request", 3000);
        m_callWidget->setScreenShareButtonActive(false);
        return;
    }
}

void ScreenSharingManager::onScreenSharingStarted()
{
    if (m_screenCaptureController) {
        m_screenCaptureController->startCapture();
    }
}

void ScreenSharingManager::onScreenCaptureStarted()
{
    LOG_INFO("Screen capture started locally");
    if (m_callWidget) {
        m_callWidget->hideEnterFullscreenButton();
    }
}

void ScreenSharingManager::onScreenCaptureStopped()
{
    calls::stopScreenSharing();
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

    if (!calls::sendScreen(imageData)) {
        showTransientStatusMessage("Failed to send screen frame", 2000);
    }
}

void ScreenSharingManager::onStartScreenSharingError()
{
    showTransientStatusMessage("Screen sharing rejected by server", 3000);
    stopLocalScreenCapture();
    if (m_callWidget) {
        m_callWidget->setScreenShareButtonActive(false);
    }
}

void ScreenSharingManager::onIncomingScreenSharingStarted()
{
    if (m_callWidget) {
        m_callWidget->restrictScreenShareButton();
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
    if (!calls::isViewingRemoteCamera()) {
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
    if (!m_callWidget || data.empty() || !calls::isViewingRemoteScreen()) return;

    QPixmap frame;
    const auto* raw = reinterpret_cast<const uchar*>(data.data());

    if (frame.loadFromData(raw, static_cast<int>(data.size()), "JPG")) {
        m_callWidget->showFrameInMainScreen(frame, Screen::ScaleMode::KeepAspectRatio);
    }
}

void ScreenSharingManager::showTransientStatusMessage(const QString& message, int durationMs)
{
    if (m_statusBar) {
        m_statusBar->showMessage(message, durationMs);
    }
}
