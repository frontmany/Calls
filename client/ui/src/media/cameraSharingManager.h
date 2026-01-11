#pragma once

#include <QObject>
#include <QPixmap>
#include <QTimer>
#include <vector>
#include <QStatusBar>
#include <memory>

#include "core.h"
#include "widgets/screen.h"
#include "utilities/logger.h"

class CameraCaptureController;
class CallWidget;
class ConfigManager;
class MainMenuWidget;
class DialogsController;

class CameraSharingManager : public QObject {
    Q_OBJECT

public:
    explicit CameraSharingManager(std::shared_ptr<core::Client> client, ConfigManager* configManager, CameraCaptureController* cameraController, DialogsController* dialogsController, QObject* parent = nullptr);
    
    void setWidgets(CallWidget* callWidget, MainMenuWidget* mainMenuWidget, QStatusBar* statusBar);

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
    void onOperationTimerTimeout();

private:
    void showTransientStatusMessage(const QString& message, int durationMs);
    void startOperationTimer(const QString& dialogText);
    void stopOperationTimer();

    std::shared_ptr<core::Client> m_coreClient = nullptr;
    ConfigManager* m_configManager = nullptr;
    CameraCaptureController* m_cameraCaptureController = nullptr;
    DialogsController* m_dialogsController = nullptr;
    CallWidget* m_callWidget = nullptr;
    MainMenuWidget* m_mainMenuWidget = nullptr;
    QStatusBar* m_statusBar = nullptr;
    QTimer* m_operationTimer = nullptr;
    QString m_pendingOperationDialogText;
    bool m_isCameraInAdditionalScreen = false;
    bool m_isRemoteCameraInAdditionalScreen = false;
};
