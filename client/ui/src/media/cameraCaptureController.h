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
#include <QThread>

class FrameProcessor;

class CameraCaptureController : public QObject
{
    Q_OBJECT

public:
    explicit CameraCaptureController(QObject* parent = nullptr);
    ~CameraCaptureController();
    void startCapture();
    void stopCapture();
    bool isCameraAvailable() const;
    bool isCapturing() const;

signals:
    void cameraCaptured(const QPixmap& pixmap, const std::vector<unsigned char>& imageData);
    void captureStarted();
    void captureStopped();
    void errorOccurred(const QString& errorMessage);
    void frameReadyForProcessing(const QVideoFrame& frame);

private slots:
    void handleVideoFrame(const QVideoFrame& frame);
    void handleCameraError(QCamera::Error error, const QString& errorString);
    void onFrameProcessed(const QPixmap& pixmap, const std::vector<unsigned char>& imageData);
    void onProcessingError(const QString& errorMessage);

private:
    QTimer* m_captureTimer;
    bool m_isCapturing;
    int m_pendingFrames = 0;
    int m_maxQueuedFrames = 1;

    QCamera* m_camera;
    QMediaCaptureSession* m_captureSession;
    QVideoSink* m_videoSink;

    QThread* m_processingThread;
    FrameProcessor* m_frameProcessor;
};
