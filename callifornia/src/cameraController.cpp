#include "cameraController.h"

#include <QVideoFrame>
#include <QImage>
#include <QPainter>
#include <QBuffer>

CameraController::CameraController(QObject* parent)
    : QObject(parent),
    m_captureTimer(new QTimer(this)),
    m_isCapturing(false),
    m_camera(nullptr),
    m_captureSession(new QMediaCaptureSession(this)),
    m_videoSink(new QVideoSink(this))
{
    // Настройка видеосинка для получения кадров
    connect(m_videoSink, &QVideoSink::videoFrameChanged, this, &CameraController::handleVideoFrame);
    m_captureSession->setVideoSink(m_videoSink);
}

CameraController::~CameraController()
{
    stopCapture();
}

void CameraController::startCapture()
{
    if (m_isCapturing)
    {
        return;
    }

    if (m_camera) {
        m_camera->stop();
        delete m_camera;
        m_camera = nullptr;
    }

    // Используем первую доступную камеру
    auto cameras = QMediaDevices::videoInputs();
    if (cameras.isEmpty()) {
        emit errorOccurred("No cameras available");
        return;
    }

    try {
        m_camera = new QCamera(cameras.first());
        connect(m_camera, &QCamera::errorOccurred, this, &CameraController::handleCameraError);

        m_captureSession->setCamera(m_camera);
        m_camera->start();

        m_isCapturing = true;
        emit captureStarted();
    }
    catch (const std::exception& e) {
        emit errorOccurred(QString("Failed to start camera: %1").arg(e.what()));
    }
}

void CameraController::stopCapture()
{
    if (!m_isCapturing)
    {
        return;
    }

    if (m_camera) {
        m_camera->stop();
        delete m_camera;
        m_camera = nullptr;
    }

    m_isCapturing = false;
    m_previousImageData.clear();

    emit captureStopped();
}

bool CameraController::isCapturing() const
{
    return m_isCapturing;
}

void CameraController::handleVideoFrame(const QVideoFrame& frame)
{
    QVideoFrame clonedFrame = frame;
    if (!clonedFrame.map(QVideoFrame::ReadOnly)) {
        return;
    }

    QPixmap pixmap;
    try {
        pixmap = videoFrameToPixmap(clonedFrame);
    }
    catch (const std::exception& e) {
        clonedFrame.unmap();
        emit errorOccurred(QString("Failed to convert video frame: %1").arg(e.what()));
        return;
    }

    clonedFrame.unmap();

    if (!pixmap.isNull() && pixmap.width() > 0 && pixmap.height() > 0) {
        try {
            QPixmap croppedPixmap = cropToHorizontal(pixmap);

            if (!croppedPixmap.isNull() && croppedPixmap.width() > 0 && croppedPixmap.height() > 0) {
                // Конвертируем в байты с тем же целевым размером
                QSize targetSize = QSize(1280, 720);
                std::vector<unsigned char> imageData = pixmapToBytes(croppedPixmap, targetSize);

                // Эмитим сигнал с захваченным изображением
                emit cameraCaptured(croppedPixmap, imageData);

                m_previousImageData = std::move(imageData);
            }
        }
        catch (const std::exception& e) {
            emit errorOccurred(QString("Failed to process video frame: %1").arg(e.what()));
        }
    }
}

void CameraController::handleCameraError(QCamera::Error error, const QString& errorString)
{
    emit errorOccurred(QString("Camera error %1: %2").arg(error).arg(errorString));

    if (m_isCapturing) {
        stopCapture();
    }
}

QPixmap CameraController::videoFrameToPixmap(const QVideoFrame& frame)
{
    QImage image = frame.toImage();

    if (!image.isNull()) {
        return QPixmap::fromImage(image);
    }

    QVideoFrame localFrame = frame;
    if (!localFrame.map(QVideoFrame::ReadOnly)) {
        return QPixmap();
    }

    QImage convertedImage;
    QImage::Format format = QVideoFrameFormat::imageFormatFromPixelFormat(localFrame.pixelFormat());
    if (format == QImage::Format_Invalid) {
        format = QImage::Format_RGB32;
    }

    convertedImage = QImage(localFrame.bits(0),
        localFrame.width(),
        localFrame.height(),
        localFrame.bytesPerLine(0),
        format);

    localFrame.unmap();

    return QPixmap::fromImage(convertedImage);
}

bool CameraController::isCameraAvailable() const {
    auto cameras = QMediaDevices::videoInputs();
    return !cameras.isEmpty();
}

QPixmap CameraController::cropToHorizontal(const QPixmap& pixmap)
{
    if (pixmap.isNull()) return pixmap;

    int w = pixmap.width();
    int h = pixmap.height();

    if (h <= w)
    {
        return pixmap;
    }

    int targetH = static_cast<int>(w * 9.0 / 16.0);
    targetH = qMin(targetH, h);
    QRect cropRect(0, 0, w, targetH);
    return pixmap.copy(cropRect);
}

std::vector<unsigned char> CameraController::pixmapToBytes(const QPixmap& pixmap, QSize targetSize)
{
    QImage image = pixmap.toImage();

    if (image.format() != QImage::Format_RGB32 && image.format() != QImage::Format_ARGB32)
    {
        image = image.convertToFormat(QImage::Format_RGB32);
    }

    QImage scaledImage = image.scaled(targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    buffer.open(QIODevice::WriteOnly);

    scaledImage.save(&buffer, "JPG", 50);

    return std::vector<unsigned char>(byteArray.begin(), byteArray.end());
}