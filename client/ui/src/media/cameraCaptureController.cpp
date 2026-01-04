#include "cameraCaptureController.h"
#include "media/frameProcessor.h"

CameraCaptureController::CameraCaptureController(QObject* parent)
    : QObject(parent),
    m_captureTimer(new QTimer(this)),
    m_isCapturing(false),
    m_camera(nullptr),
    m_captureSession(new QMediaCaptureSession(this)),
    m_videoSink(new QVideoSink(this)),
    m_processingThread(new QThread(this)),
    m_frameProcessor(new FrameProcessor())
{
    m_frameProcessor->moveToThread(m_processingThread);
    
    connect(m_videoSink, &QVideoSink::videoFrameChanged, this, &CameraCaptureController::handleVideoFrame);
    connect(this, &CameraCaptureController::frameReadyForProcessing, m_frameProcessor, &FrameProcessor::processVideoFrame);
    connect(m_frameProcessor, &FrameProcessor::frameProcessed, this, &CameraCaptureController::onFrameProcessed);
    connect(m_frameProcessor, &FrameProcessor::processingError, this, &CameraCaptureController::onProcessingError);
    
    connect(m_processingThread, &QThread::finished, m_frameProcessor, &QObject::deleteLater);
    
    m_captureSession->setVideoSink(m_videoSink);
    m_processingThread->start();
}

CameraCaptureController::~CameraCaptureController()
{
    stopCapture();
    
    m_processingThread->quit();
    m_processingThread->wait();
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

    if (m_camera)
    {
        m_camera->stop();
        delete m_camera;
        m_camera = nullptr;
    }

    m_isCapturing = false;

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

    emit frameReadyForProcessing(frame);
}

void CameraCaptureController::onFrameProcessed(const QPixmap& pixmap, const std::vector<unsigned char>& imageData)
{
    if (!m_isCapturing)
    {
        return;
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