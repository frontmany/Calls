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

class CameraCaptureController;
class CallWidget;
class ConfigManager;
class MainMenuWidget;
class DialogsController;
class NotificationController;

class CameraSharingManager : public QObject {
    Q_OBJECT

public:
    explicit CameraSharingManager(std::shared_ptr<core::Client> client, ConfigManager* configManager, CameraCaptureController* cameraController, DialogsController* dialogsController, QObject* parent = nullptr);
    
    void setWidgets(CallWidget* callWidget, MainMenuWidget* mainMenuWidget);
    void setNotificationController(NotificationController* notificationController);

    void stopLocalCameraCapture();
    void initializeCameraForCall();
    void hideOperationDialog();

public slots:
    void onCameraButtonClicked(bool toggled);
    void onCameraSharingStarted();
    void onStartCameraSharingError();
    void onCameraCaptureStarted();
    void onCameraCaptureStopped();
    void onCameraCaptured(const QPixmap& pixmap, const std::vector<unsigned char>& imageData);
    void onIncomingCameraSharingStarted();
    void onIncomingCameraSharingStopped();
    void onIncomingCamera(const std::vector<unsigned char>& data);
    void onCameraErrorOccurred(const QString& errorMessage);
    void onActivateCameraButtonClicked(bool activated);
    void onStopCameraSharingResult(std::error_code ec);

private slots:
private:
    void startOperationTimer(core::UserOperationType operationKey, const QString& dialogText);
    void stopOperationTimer(core::UserOperationType operationKey);
    void stopAllOperationTimers();
    void onOperationTimerTimeout(core::UserOperationType operationKey);

    std::shared_ptr<core::Client> m_coreClient = nullptr;
    ConfigManager* m_configManager = nullptr;
    CameraCaptureController* m_cameraCaptureController = nullptr;
    DialogsController* m_dialogsController = nullptr;
    NotificationController* m_notificationController = nullptr;
    CallWidget* m_callWidget = nullptr;
    MainMenuWidget* m_mainMenuWidget = nullptr;
    QMap<core::UserOperationType, QTimer*> m_operationTimers;
    QMap<core::UserOperationType, QString> m_pendingOperationTexts;
    bool m_isCameraInAdditionalScreen = false;
    bool m_isRemoteCameraInAdditionalScreen = false;
};
