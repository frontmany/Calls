#pragma once

#include <QObject>
#include <QPixmap>
#include <vector>
#include <QStatusBar>

#include "calls.h"
#include "screen.h"
#include "logger.h"

class CameraCaptureController;
class CallWidget;
class ConfigManager;
class MainMenuWidget;

class CameraSharingManager : public QObject {
    Q_OBJECT

public:
    explicit CameraSharingManager(ConfigManager* configManager, QObject* parent = nullptr);
    
    void setControllers(CameraCaptureController* cameraController);
    void setWidgets(CallWidget* callWidget, MainMenuWidget* mainMenuWidget, QStatusBar* statusBar);

    void stopLocalCameraCapture();
    void initializeCameraForCall();

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

private:
    void showTransientStatusMessage(const QString& message, int durationMs);

    ConfigManager* m_configManager = nullptr;
    CameraCaptureController* m_cameraCaptureController = nullptr;
    CallWidget* m_callWidget = nullptr;
    MainMenuWidget* m_mainMenuWidget = nullptr;
    QStatusBar* m_statusBar = nullptr;
    bool m_isCameraInAdditionalScreen = false;
    bool m_isRemoteCameraInAdditionalScreen = false;
};
