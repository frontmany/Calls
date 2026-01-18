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
{
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
            startOperationTimer(core::UserOperationType::STOP_SCREEN_SHARING, "Stopping screen sharing...");
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
        startOperationTimer(core::UserOperationType::START_SCREEN_SHARING, "Starting screen sharing...");
    }
}

void ScreenSharingManager::onScreenSharingStarted()
{
    stopOperationTimer(core::UserOperationType::START_SCREEN_SHARING);
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
    stopOperationTimer(core::UserOperationType::START_SCREEN_SHARING);
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
    stopOperationTimer(core::UserOperationType::STOP_SCREEN_SHARING);
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


void ScreenSharingManager::startOperationTimer(core::UserOperationType operationKey, const QString& dialogText)
{
    m_pendingOperationTexts.insert(operationKey, dialogText);

    QTimer* timer = m_operationTimers.value(operationKey, nullptr);
    if (!timer)
    {
        timer = new QTimer(this);
        timer->setSingleShot(true);
        timer->setInterval(1000);
        connect(timer, &QTimer::timeout, this, [this, operationKey]()
        {
            onOperationTimerTimeout(operationKey);
        });
        m_operationTimers.insert(operationKey, timer);
    }

    timer->start();
}

void ScreenSharingManager::stopOperationTimer(core::UserOperationType operationKey)
{
    if (QTimer* timer = m_operationTimers.value(operationKey, nullptr))
    {
        timer->stop();
    }

    m_pendingOperationTexts.remove(operationKey);

    if (m_dialogsController)
    {
        m_dialogsController->hidePendingOperationDialog(operationKey);
    }
}

void ScreenSharingManager::stopAllOperationTimers()
{
    if (m_dialogsController)
    {
        for (auto it = m_operationTimers.constBegin(); it != m_operationTimers.constEnd(); ++it)
        {
            m_dialogsController->hidePendingOperationDialog(it.key());
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

void ScreenSharingManager::onOperationTimerTimeout(core::UserOperationType operationKey)
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

    if (m_dialogsController)
    {
        m_dialogsController->showPendingOperationDialog(dialogText, operationKey);
    }
}

void ScreenSharingManager::hideOperationDialog()
{
    stopAllOperationTimers();
}
