#pragma once

#include <QObject>
#include <QString>
#include <QStackedLayout>
#include <memory>

#include "client.h"
#include "../state.h"

class AudioEffectsManager;
class MainMenuWidget;
class CallWidget;
class NavigationController;
class ScreenCaptureController;
class CameraCaptureController;

class CallManager : public QObject {
    Q_OBJECT

public:
    explicit CallManager(std::shared_ptr<callifornia::Client> client, AudioEffectsManager* audioManager, NavigationController* navigationController, ScreenCaptureController* screenCaptureController, CameraCaptureController* cameraCaptureController, QObject* parent = nullptr);
    
    void setWidgets(MainMenuWidget* mainMenuWidget, CallWidget* callWidget, QStackedLayout* stackedLayout);

public slots:
    void onStartCallingButtonClicked(const QString& friendNickname);
    void onStopCallingButtonClicked();
    void onAcceptCallButtonClicked(const QString& friendNickname);
    void onDeclineCallButtonClicked(const QString& friendNickname);
    void onEndCallButtonClicked();

    void onStartCallingResult(callifornia::ErrorCode ec);
    void onAcceptCallResult(callifornia::ErrorCode ec, const QString& nickname);
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

    std::shared_ptr<callifornia::Client> m_client = nullptr;
    AudioEffectsManager* m_audioManager = nullptr;
    NavigationController* m_navigationController = nullptr;
    ScreenCaptureController* m_screenCaptureController = nullptr;
    CameraCaptureController* m_cameraCaptureController = nullptr;
    MainMenuWidget* m_mainMenuWidget = nullptr;
    CallWidget* m_callWidget = nullptr;
    QStackedLayout* m_stackedLayout = nullptr;
};
