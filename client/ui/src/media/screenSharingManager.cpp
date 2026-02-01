#include "screenSharingManager.h"
#include "media/screenCaptureController.h"
#include "media/av1Decoder.h"
#include "managers/dialogsController.h"
#include "managers/notificationController.h"
#include "widgets/callWidget.h"
#include "media/cameraCaptureController.h"
#include "utilities/constant.h"
#include <QApplication>
#include <QResizeEvent>

ScreenSharingManager::ScreenSharingManager(std::shared_ptr<core::Client> client, ScreenCaptureController* screenController, DialogsController* dialogsController, CameraCaptureController* cameraController, QObject* parent)
    : QObject(parent)
    , m_coreClient(client)
    , m_screenCaptureController(screenController)
    , m_dialogsController(dialogsController)
    , m_cameraCaptureController(cameraController)
    , m_av1Decoder(nullptr)
{
    m_av1Decoder = new AV1Decoder();
    m_av1Decoder->initialize();
}

ScreenSharingManager::~ScreenSharingManager()
{
    if (m_av1Decoder) {
        delete m_av1Decoder;
        m_av1Decoder = nullptr;
    }
}

void ScreenSharingManager::setWidgets(CallWidget* callWidget)
{
    m_callWidget = callWidget;
}

void ScreenSharingManager::setNotificationController(NotificationController* notificationController)
{
    m_notificationController = notificationController;
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
        // Schedule updateMainScreenSize to be called in the next event loop iteration
        // This ensures the widget has finished processing the exitFullscreen changes
        // We use QApplication::postEvent to post a custom event that will trigger
        // the resize event, which in turn calls updateMainScreenSize()
        QSize currentSize = m_callWidget->size();
        
        // Post a resize event with the same size to trigger resizeEvent handler
        // The oldSize is slightly different to ensure the event is processed
        QResizeEvent* resizeEvent = new QResizeEvent(currentSize, QSize(currentSize.width() - 1, currentSize.height() - 1));
        QApplication::postEvent(m_callWidget, resizeEvent);
    }
}

void ScreenSharingManager::onIncomingScreen(const std::vector<unsigned char>& data)
{
    LOG_DEBUG("ScreenSharingManager: Received screen data, size: {} bytes", data.size());
    if (!m_callWidget || data.empty() || !m_coreClient || !m_coreClient->isViewingRemoteScreen()) {
        LOG_WARN("ScreenSharingManager: Cannot process frame - callWidget: {}, data.empty: {}, coreClient: {}, viewingRemoteScreen: {}", 
                 static_cast<bool>(m_callWidget), data.empty(), 
                 static_cast<bool>(m_coreClient), 
                 m_coreClient ? m_coreClient->isViewingRemoteScreen() : false);
        return;
    }

    QPixmap frame;
    
    // Always use AV1 format now - no format detection needed
    if (m_av1Decoder && m_av1Decoder->isInitialized()) {
        LOG_DEBUG("ScreenSharingManager: Decoding AV1 data...");
        // Decode AV1 data directly (no version byte to skip)
        QImage decodedImage = m_av1Decoder->decode(data);
        if (!decodedImage.isNull()) {
            frame = QPixmap::fromImage(decodedImage);
            LOG_DEBUG("ScreenSharingManager: AV1 decoded successfully, image size: {}x{}", 
                     decodedImage.width(), decodedImage.height());
        } else {
            LOG_WARN("ScreenSharingManager: AV1 decoder returned null image");
        }
    } else {
        LOG_ERROR("ScreenSharingManager: AV1 decoder not available or not initialized");
    }
    
    if (!frame.isNull()) {
        LOG_DEBUG("ScreenSharingManager: Displaying frame in main screen");
        m_callWidget->showFrameInMainScreen(frame, Screen::ScaleMode::KeepAspectRatio);
    } else {
        LOG_WARN("ScreenSharingManager: Frame is null, not displaying");
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
        timer->setInterval(TIMER_INTERVAL_MS);
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

    if (m_notificationController)
    {
        m_notificationController->hidePendingOperation(operationKey);
    }
}

void ScreenSharingManager::stopAllOperationTimers()
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

    if (m_notificationController)
    {
        m_notificationController->showPendingOperation(dialogText, operationKey);
    }
}

void ScreenSharingManager::hideOperationDialog()
{
    stopAllOperationTimers();
}

void ScreenSharingManager::requestFullscreenExit()
{
    emit fullscreenExitRequested();
}
