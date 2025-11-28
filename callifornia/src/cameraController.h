#pragma once

#include <vector>

#include <QObject>
#include <QTimer>
#include <QPixmap>
#include <QMediaDevices>
#include <QCamera>
#include <QMediaCaptureSession>
#include <QVideoSink>
#include <QSize>

class CameraController : public QObject
{
    Q_OBJECT

public:
    explicit CameraController(QObject* parent = nullptr);
    ~CameraController();
    void startCapture();
    void stopCapture();
    bool isCameraAvailable() const;
    bool isCapturing() const;

signals:
    void cameraCaptured(const QPixmap& pixmap, const std::vector<unsigned char>& imageData);
    void captureStarted();
    void captureStopped();
    void errorOccurred(const QString& errorMessage);

private slots:
    void handleVideoFrame(const QVideoFrame& frame);
    void handleCameraError(QCamera::Error error, const QString& errorString);

private:
    QPixmap videoFrameToPixmap(const QVideoFrame& frame);
    QPixmap cropToHorizontal(const QPixmap& pixmap);
    std::vector<unsigned char> pixmapToBytes(const QPixmap& pixmap, QSize targetSize);

private:
    QTimer* m_captureTimer;
    bool m_isCapturing;

    QCamera* m_camera;
    QMediaCaptureSession* m_captureSession;
    QVideoSink* m_videoSink;

    std::vector<unsigned char> m_previousImageData;
};