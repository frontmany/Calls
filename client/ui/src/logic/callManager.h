#pragma once

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QStackedLayout>
#include <QMap>
#include <QSet>
#include <memory>

#include "core.h"

class AudioEffectsManager;
class MainMenuWidget;
class CallWidget;
class NavigationController;
class DialogsController;
class NotificationController;
class UpdateManager;

class CallManager : public QObject {
    Q_OBJECT

private:
    struct IncomingCallData
    {
        QString nickname;
        int remainingTime;
    };

public:
    explicit CallManager(std::shared_ptr<core::Core> client, AudioEffectsManager* audioManager, NavigationController* navigationController, DialogsController* dialogsController, UpdateManager* updateManager = nullptr, QObject* parent = nullptr);
    
    void setWidgets(MainMenuWidget* mainMenuWidget, CallWidget* callWidget, QStackedLayout* stackedLayout);
    void setNotificationController(NotificationController* notificationController);
    void hideOperationDialog();

public slots:
    void onStartCallingButtonClicked(const QString& friendNickname);
    void onStopCallingButtonClicked();
    void onAcceptCallButtonClicked(const QString& friendNickname);
    void onDeclineCallButtonClicked(const QString& friendNickname);
    void onEndCallButtonClicked();

    void onAudioDevicePickerRequested();
    void onCallWidgetAudioSettingsRequested();
    void onAudioSettingsDialogClosed();
    void onActivateCameraClicked(bool active);
    void onScreenShareClicked(bool toggled);
    void onScreenSelected(int screenIndex);
    void onScreenShareDialogCancelled();
    void onCallWidgetCameraClicked(bool toggled);
    void onStartCallingResult(std::error_code ec);
    void onMaximumCallingTimeReached();
    void onCallingAccepted();
    void onCallingDeclined();
    void onRemoteUserEndedCall();
    void onIncomingCall(const QString& friendNickname);
    void onIncomingCallExpired(const QString& friendNickname);
    void onCallParticipantConnectionDown();
    void onCallParticipantConnectionRestored();
    void onLocalConnectionDownInCall();

    // Media frame slots (called from CoreEventListener via QMetaObject::invokeMethod)
    void onLocalScreenFrame(QByteArray data, int width, int height);
    void onLocalCameraFrame(QByteArray data, int width, int height);
    void onIncomingScreenFrame(QByteArray data, int width, int height);
    void onIncomingCameraFrame(QByteArray data, int width, int height);

    // Media state slots
    void onIncomingScreenSharingStarted();
    void onIncomingScreenSharingStopped();
    void onIncomingCameraSharingStarted();
    void onIncomingCameraSharingStopped();
    void onStartScreenSharingError();
    void onStartCameraSharingError();

signals:
    void endCallFullscreenExitRequested();

private slots:
    void onIncomingCallsDialogClosed(const QList<QString>& pendingCalls);

private:
    void handleAcceptCallErrorNotificationAppearance();
    void handleDeclineCallErrorNotificationAppearance();
    void handleStartCallingErrorNotificationAppearance();
    void handleStopCallingErrorNotificationAppearance();
    void handleEndCallErrorNotificationAppearance();
    void updateIncomingCallsUi();
    void resetCallWidgetState();

    std::shared_ptr<core::Core> m_coreClient = nullptr;
    AudioEffectsManager* m_audioManager = nullptr;
    NavigationController* m_navigationController = nullptr;
    DialogsController* m_dialogsController = nullptr;
    NotificationController* m_notificationController = nullptr;
    UpdateManager* m_updateManager = nullptr;
    MainMenuWidget* m_mainMenuWidget = nullptr;
    CallWidget* m_callWidget = nullptr;
    QStackedLayout* m_stackedLayout = nullptr;

    QMap<QString, IncomingCallData> m_incomingCalls;
    QSet<QString> m_visibleIncomingCalls;
};
