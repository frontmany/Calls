#pragma once

#include <QObject>
#include <QPixmap>
#include <vector>
#include <QStatusBar>

#include "calls.h"
#include "screen.h"
#include "logger.h"

class ScreenCaptureController;
class DialogsController;
class CallWidget;
class CameraCaptureController;

class ScreenSharingManager : public QObject {
    Q_OBJECT

public:
    explicit ScreenSharingManager(QObject* parent = nullptr);
    
    void setControllers(ScreenCaptureController* screenController, DialogsController* dialogsController);
    void setWidgets(CallWidget* callWidget, QStatusBar* statusBar);
    void setCameraCaptureController(CameraCaptureController* cameraController);

    void stopLocalScreenCapture();

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

signals:
    void fullscreenExitRequested();

private:
    void showTransientStatusMessage(const QString& message, int durationMs);

    ScreenCaptureController* m_screenCaptureController = nullptr;
    DialogsController* m_dialogsController = nullptr;
    CallWidget* m_callWidget = nullptr;
    QStatusBar* m_statusBar = nullptr;
    CameraCaptureController* m_cameraCaptureController = nullptr;
};