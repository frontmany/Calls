#pragma once

#include <QObject>
#include <QString>
#include <QStackedLayout>
#include <QTimer>
#include <memory>

#include "core.h"

class AudioEffectsManager;
class MainMenuWidget;
class CallWidget;
class NavigationController;
class ScreenCaptureController;
class CameraCaptureController;
class DialogsController;

class CallManager : public QObject {
    Q_OBJECT

public:
    explicit CallManager(std::shared_ptr<core::Client> client, AudioEffectsManager* audioManager, NavigationController* navigationController, ScreenCaptureController* screenCaptureController, CameraCaptureController* cameraCaptureController, DialogsController* dialogsController, QObject* parent = nullptr);
    
    void setWidgets(MainMenuWidget* mainMenuWidget, CallWidget* callWidget, QStackedLayout* stackedLayout);
    void hideOperationDialog();

public slots:
    void onStartCallingButtonClicked(const QString& friendNickname);
    void onStopCallingButtonClicked();
    void onAcceptCallButtonClicked(const QString& friendNickname);
    void onDeclineCallButtonClicked(const QString& friendNickname);
    void onEndCallButtonClicked();

    void onStartCallingResult(std::error_code ec);
    void onAcceptCallResult(std::error_code ec, const QString& nickname);
    void onMaximumCallingTimeReached();
    void onCallingAccepted();
    void onCallingDeclined();
    void onRemoteUserEndedCall();
    void onIncomingCall(const QString& friendNickname);
    void onIncomingCallExpired(const QString& friendNickname);
    void onStopOutgoingCallResult(std::error_code ec);
    void onDeclineCallResult(std::error_code ec, const QString& nickname);
    void onEndCallResult(std::error_code ec);
    void onCallParticipantConnectionDown();
    void onCallParticipantConnectionRestored();

signals:
    void stopScreenCaptureRequested();
    void stopCameraCaptureRequested();
    void endCallFullscreenExitRequested();

private slots:
    void onTimeToShowWaitingNotification();

private:
    void handleAcceptCallErrorNotificationAppearance();
    void handleDeclineCallErrorNotificationAppearance();
    void handleStartCallingErrorNotificationAppearance();
    void handleStopCallingErrorNotificationAppearance();
    void handleEndCallErrorNotificationAppearance();
    void startOperationTimer(const QString& dialogText);
    void stopOperationTimer();

    std::shared_ptr<core::Client> m_coreClient = nullptr;
    AudioEffectsManager* m_audioManager = nullptr;
    NavigationController* m_navigationController = nullptr;
    ScreenCaptureController* m_screenCaptureController = nullptr;
    CameraCaptureController* m_cameraCaptureController = nullptr;
    DialogsController* m_dialogsController = nullptr;
    MainMenuWidget* m_mainMenuWidget = nullptr;
    CallWidget* m_callWidget = nullptr;
    QStackedLayout* m_stackedLayout = nullptr;
    QTimer* m_operationTimer = nullptr;
    QString m_pendingOperationDialogText;
};
