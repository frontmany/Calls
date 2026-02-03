#include "cameraCaptureController.h"
#include "media/frameProcessorManager.h"

#include <QEventLoop>
#include <QTimer>

CameraCaptureController::CameraCaptureController(QObject* parent)
    : QObject(parent),
    m_captureTimer(new QTimer(this)),
    m_isCapturing(false),
    m_pendingFrames(0),
    m_maxQueuedFrames(6),
    m_camera(nullptr),
    m_captureSession(new QMediaCaptureSession(this)),
    m_videoSink(new QVideoSink(this)),
    m_mediaDevices(new QMediaDevices(this)),
    m_frameProcessorManager(&FrameProcessorManager::getInstance())
{
    connect(m_videoSink, &QVideoSink::videoFrameChanged, this, &CameraCaptureController::handleVideoFrame);
    connect(this, &CameraCaptureController::frameReadyForProcessing, m_frameProcessorManager, &FrameProcessorManager::processVideoFrame);
    connect(m_frameProcessorManager, &FrameProcessorManager::cameraFrameProcessed, this, &CameraCaptureController::onFrameProcessed);
    connect(m_frameProcessorManager, &FrameProcessorManager::processingError, this, &CameraCaptureController::onProcessingError);
    connect(m_mediaDevices, &QMediaDevices::videoInputsChanged, this, &CameraCaptureController::onVideoInputsChanged);
    
    m_captureSession->setVideoSink(m_videoSink);
}

CameraCaptureController::~CameraCaptureController()
{
    // Полная остановка и очистка
    stopCapture();
    
    // Дополнительная очистка Qt Multimedia компонентов
    if (m_captureSession) {
        m_captureSession->setCamera(nullptr);
        m_captureSession->setVideoSink(nullptr);
        delete m_captureSession;
        m_captureSession = nullptr;
    }
    
    if (m_videoSink) {
        delete m_videoSink;
        m_videoSink = nullptr;
    }
    
    if (m_mediaDevices) {
        delete m_mediaDevices;
        m_mediaDevices = nullptr;
    }
}

void CameraCaptureController::startCapture()
{
    if (m_isCapturing)
    {
        return;
    }

    if (m_camera)
    {
        m_camera->stop();
        delete m_camera;
        m_camera = nullptr;
    }

    auto cameras = QMediaDevices::videoInputs();
    if (cameras.isEmpty())
    {
        emit errorOccurred("No cameras available");
        return;
    }

    try
    {
        m_camera = new QCamera(cameras.first());
        connect(m_camera, &QCamera::errorOccurred, this, &CameraCaptureController::handleCameraError);

        m_captureSession->setCamera(m_camera);
        m_camera->start();

        m_isCapturing = true;
        emit captureStarted();
    }
    catch (const std::exception& e)
    {
        emit errorOccurred(QString("Failed to start camera: %1").arg(e.what()));
    }
}

void CameraCaptureController::stopCapture()
{
    if (!m_isCapturing)
    {
        return;
    }
    
    m_isCapturing = false;
    m_pendingFrames = 0;

    // Корректно останавливаем камеру с таймаутом
    if (m_camera) {
        // Отключаем все сигналы ПЕРЕД остановкой
        m_camera->disconnect();
        
        // Останавливаем камеру
        m_camera->stop();
        
        // Ждем завершения с таймаутом
        QEventLoop loop;
        QTimer::singleShot(1000, &loop, &QEventLoop::quit); // 1 секунда таймаут
        
        // Принудительно очищаем сессию
        if (m_captureSession) {
            m_captureSession->setCamera(nullptr);
        }
        
        // Удаляем камеру
        delete m_camera;
        m_camera = nullptr;
    }

    emit captureStopped();
}

bool CameraCaptureController::isCapturing() const
{
    return m_isCapturing;
}

void CameraCaptureController::handleVideoFrame(const QVideoFrame& frame)
{
    if (!m_isCapturing)
    {
        return;
    }

    if (m_pendingFrames >= m_maxQueuedFrames)
    {
        return;
    }

    ++m_pendingFrames;
    emit frameReadyForProcessing(frame);
}

void CameraCaptureController::onFrameProcessed(const QPixmap& pixmap, const std::vector<unsigned char>& imageData)
{
    if (!m_isCapturing)
    {
        return;
    }

    if (m_pendingFrames > 0)
    {
        --m_pendingFrames;
    }

    emit cameraCaptured(pixmap, imageData);
}

void CameraCaptureController::onProcessingError(const QString& errorMessage)
{
    emit errorOccurred(errorMessage);
}

void CameraCaptureController::handleCameraError(QCamera::Error error, const QString& errorString)
{
    emit errorOccurred(QString("Camera error %1: %2").arg(error).arg(errorString));

    if (m_isCapturing)
    {
        stopCapture();
    }
}

bool CameraCaptureController::isCameraAvailable() const
{
    auto cameras = QMediaDevices::videoInputs();
    return !cameras.isEmpty();
}

void CameraCaptureController::onVideoInputsChanged()
{
    // If we're capturing but no cameras are available, emit an error
    if (m_isCapturing && !isCameraAvailable()) {
        emit errorOccurred("Camera device became unavailable");
        stopCapture();
    }
}