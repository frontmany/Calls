#pragma once

#include <QObject>
#include <QPixmap>
#include <QTimer>
#include <QMap>
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

class ScreenSharingManager : public QObject {
    Q_OBJECT

public:
    explicit ScreenSharingManager(std::shared_ptr<core::Client> client, ScreenCaptureController* screenController, DialogsController* dialogsController, CameraCaptureController* cameraController, QObject* parent = nullptr);
    
    void setWidgets(CallWidget* callWidget);
    void setNotificationController(NotificationController* notificationController);

    void stopLocalScreenCapture();
    void hideOperationDialog();

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
};