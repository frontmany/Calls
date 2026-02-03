#pragma once

#include <QObject>
#include <QPixmap>
#include <QTimer>
#include <QMap>
#include <QThread>
#include <QQueue>
#include <QMutex>
#include <QWaitCondition>
#include <QAtomicInt>
#include <vector>
#include <memory>

#include "core.h"
#include "widgets/screen.h"
#include "utilities/logger.h"

class ScreenCaptureController;
class DialogsController;
class CallWidget;
class CameraCaptureController;
class NotificationController;
class H264Decoder;

class DecodingWorker : public QObject {
    Q_OBJECT

public:
    explicit DecodingWorker(H264Decoder* decoder, QObject* parent = nullptr);
    ~DecodingWorker();

public slots:
    void decodeFrame(const std::vector<unsigned char>& data, int frameId);
    void stop();

signals:
    void frameDecoded(const QPixmap& pixmap, int frameId);

private:
    H264Decoder* m_decoder;
    bool m_shouldStop;
};

class ScreenSharingManager : public QObject {
    Q_OBJECT

public:
    explicit ScreenSharingManager(std::shared_ptr<core::Client> client, ScreenCaptureController* screenController, DialogsController* dialogsController, CameraCaptureController* cameraController, QObject* parent = nullptr);
    ~ScreenSharingManager();
    
    void setWidgets(CallWidget* callWidget);
    void setNotificationController(NotificationController* notificationController);

    void stopLocalScreenCapture();
    void hideOperationDialog();
    void requestFullscreenExit();

public slots:
    void onScreenShareButtonClicked(bool toggled);
    void onScreenSelected(int screenIndex);
    void onScreenSharingStarted();
    void onStartScreenSharingError();
    void onScreenCaptureStarted();
    void onScreenCaptureStopped();
    void onScreenCaptured(const QPixmap& pixmap, const std::vector<unsigned char>& imageData);
    void onIncomingScreenSharingStarted();
    void onIncomingScreenSharingStopped();
    void onIncomingScreen(const std::vector<unsigned char>& data);
    void onStopScreenSharingResult(std::error_code ec);

signals:
    void fullscreenExitRequested();

private slots:
private:
    void startOperationTimer(core::UserOperationType operationKey, const QString& dialogText);
    void stopOperationTimer(core::UserOperationType operationKey);
    void stopAllOperationTimers();
    void onOperationTimerTimeout(core::UserOperationType operationKey);

    std::shared_ptr<core::Client> m_coreClient = nullptr;
    ScreenCaptureController* m_screenCaptureController = nullptr;
    DialogsController* m_dialogsController = nullptr;
    NotificationController* m_notificationController = nullptr;
    CallWidget* m_callWidget = nullptr;
    CameraCaptureController* m_cameraCaptureController = nullptr;
    QMap<core::UserOperationType, QTimer*> m_operationTimers;
    QMap<core::UserOperationType, QString> m_pendingOperationTexts;
    H264Decoder* m_h264Decoder = nullptr;
    QThread* m_decodingThread = nullptr;
    DecodingWorker* m_decodingWorker = nullptr;
    QAtomicInt m_currentFrameId;
    static const int MAX_QUEUE_SIZE = 10;
};
