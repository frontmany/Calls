#include "screenSharingManager.h"
#include "media/screenCaptureController.h"
#include "media/h264Decoder.h"
#include "managers/dialogsController.h"
#include "managers/notificationController.h"
#include "widgets/callWidget.h"
#include "media/cameraCaptureController.h"
#include "utilities/constant.h"
#include <QApplication>
#include <QResizeEvent>
#include <QThread>

// DecodingWorker implementation
DecodingWorker::DecodingWorker(H264Decoder* decoder, QObject* parent)
    : QObject(parent)
    , m_decoder(decoder)
    , m_shouldStop(false)
{
}

DecodingWorker::~DecodingWorker()
{
}

void DecodingWorker::decodeFrame(const std::vector<unsigned char>& data, int frameId)
{
    if (m_shouldStop || !m_decoder || !m_decoder->isInitialized()) {
        return;
    }
    
    QImage decodedImage = m_decoder->decode(data);
    if (!decodedImage.isNull()) {
        QPixmap pixmap = QPixmap::fromImage(decodedImage);
        emit frameDecoded(pixmap, frameId);
    }
}

void DecodingWorker::stop()
{
    m_shouldStop = true;
}

ScreenSharingManager::ScreenSharingManager(std::shared_ptr<core::Client> client, ScreenCaptureController* screenController, DialogsController* dialogsController, CameraCaptureController* cameraController, QObject* parent)
    : QObject(parent)
    , m_coreClient(client)
    , m_screenCaptureController(screenController)
    , m_dialogsController(dialogsController)
    , m_cameraCaptureController(cameraController)
    , m_currentFrameId(0)
{
    m_h264Decoder = new H264Decoder();
    m_h264Decoder->initialize();
    
    // Создаем поток для декодирования
    m_decodingThread = new QThread(this);
    m_decodingWorker = new DecodingWorker(m_h264Decoder);
    m_decodingWorker->moveToThread(m_decodingThread);
    
    // Подключаем сигналы
    connect(m_decodingWorker, &DecodingWorker::frameDecoded, this, 
            [this](const QPixmap& pixmap, int frameId) {
                // Обновляем только если это актуальный кадр
                if (frameId == m_currentFrameId.loadAcquire() - 1) {
                    if (m_callWidget) {
                        m_callWidget->showFrameInMainScreen(pixmap, Screen::ScaleMode::KeepAspectRatio);
                    }
                }
            });
    
    m_decodingThread->start();
}

ScreenSharingManager::~ScreenSharingManager()
{
    // Останавливаем поток декодирования
    if (m_decodingWorker) {
        m_decodingWorker->stop();
        m_decodingThread->quit();
        m_decodingThread->wait(1000); // Ждем до 1 секунды
        delete m_decodingWorker;
        m_decodingWorker = nullptr;
    }
    
    if (m_decodingThread) {
        delete m_decodingThread;
        m_decodingThread = nullptr;
    }
    
    if (m_h264Decoder) {
        delete m_h264Decoder;
        m_h264Decoder = nullptr;
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
    // Убираем спам логов для каждого кадра
    // LOG_DEBUG("ScreenSharingManager: Received screen data, size: {} bytes", data.size());
    if (!m_callWidget || data.empty() || !m_coreClient || !m_coreClient->isViewingRemoteScreen()) {
        LOG_WARN("ScreenSharingManager: Cannot process frame - callWidget: {}, data.empty: {}, coreClient: {}, viewingRemoteScreen: {}", 
                 static_cast<bool>(m_callWidget), data.empty(), 
                 static_cast<bool>(m_coreClient), 
                 m_coreClient ? m_coreClient->isViewingRemoteScreen() : false);
        return;
    }

    // Отправляем кадр в поток для декодирования
    if (m_decodingWorker && m_h264Decoder && m_h264Decoder->isInitialized()) {
        int frameId = m_currentFrameId.fetchAndAddRelaxed(1);
        QMetaObject::invokeMethod(m_decodingWorker, "decodeFrame", Qt::QueuedConnection,
                                 Q_ARG(std::vector<unsigned char>, data), Q_ARG(int, frameId));
    } else {
        LOG_ERROR("ScreenSharingManager: Decoding worker or H.264 decoder not available");
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
