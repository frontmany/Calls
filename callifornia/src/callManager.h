#pragma once

#include <QObject>
#include <QString>
#include <QStackedLayout>

#include "calls.h"
#include "state.h"

class AudioEffectsManager;
class MainMenuWidget;
class CallWidget;
class NavigationController;

class CallManager : public QObject {
    Q_OBJECT

public:
    explicit CallManager(AudioEffectsManager* audioManager, NavigationController* navigationController, QObject* parent = nullptr);
    
    void setWidgets(MainMenuWidget* mainMenuWidget, CallWidget* callWidget, QStackedLayout* stackedLayout);
    void setScreenCaptureController(class ScreenCaptureController* controller);
    void setCameraCaptureController(class CameraCaptureController* controller);
    void setNavigationController(NavigationController* navigationController);

public slots:
    void onStartCallingButtonClicked(const QString& friendNickname);
    void onStopCallingButtonClicked();
    void onAcceptCallButtonClicked(const QString& friendNickname);
    void onDeclineCallButtonClicked(const QString& friendNickname);
    void onEndCallButtonClicked();

    void onStartCallingResult(calls::ErrorCode ec);
    void onAcceptCallResult(calls::ErrorCode ec, const QString& nickname);
    void onMaximumCallingTimeReached();
    void onCallingAccepted();
    void onCallingDeclined();
    void onRemoteUserEndedCall();
    void onIncomingCall(const QString& friendNickname);
    void onIncomingCallExpired(const QString& friendNickname);

signals:
    void stopScreenCaptureRequested();
    void stopCameraCaptureRequested();
    void endCallFullscreenExitRequested();

private:
    void handleAcceptCallErrorNotificationAppearance();
    void handleDeclineCallErrorNotificationAppearance();
    void handleStartCallingErrorNotificationAppearance();
    void handleStopCallingErrorNotificationAppearance();
    void handleEndCallErrorNotificationAppearance();

    AudioEffectsManager* m_audioManager = nullptr;
    NavigationController* m_navigationController = nullptr;
    MainMenuWidget* m_mainMenuWidget = nullptr;
    CallWidget* m_callWidget = nullptr;
    QStackedLayout* m_stackedLayout = nullptr;
    class ScreenCaptureController* m_screenCaptureController = nullptr;
    class CameraCaptureController* m_cameraCaptureController = nullptr;
};
