#pragma once

#include <QObject>
#include <QPixmap>
#include <QTimer>
#include <vector>
#include <memory>

#include "core.h"
#include "widgets/screen.h"
#include "utilities/logger.h"

class ScreenCaptureController;
class DialogsController;
class CallWidget;
class CameraCaptureController;

class ScreenSharingManager : public QObject {
    Q_OBJECT

public:
    explicit ScreenSharingManager(std::shared_ptr<core::Client> client, ScreenCaptureController* screenController, DialogsController* dialogsController, CameraCaptureController* cameraController, QObject* parent = nullptr);
    
    void setWidgets(CallWidget* callWidget);

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
    void onTimeToShowWaitingNotification();

private:
    void startOperationTimer(const QString& dialogText);
    void stopOperationTimer();

    std::shared_ptr<core::Client> m_coreClient = nullptr;
    ScreenCaptureController* m_screenCaptureController = nullptr;
    DialogsController* m_dialogsController = nullptr;
    CallWidget* m_callWidget = nullptr;
    CameraCaptureController* m_cameraCaptureController = nullptr;
    QTimer* m_operationTimer = nullptr;
    QString m_pendingOperationDialogText;
};