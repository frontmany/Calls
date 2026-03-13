#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QByteArray>
#include <memory>

#include "core.h"

class MeetingWidget;
class NavigationController;
class DialogsController;
class NotificationController;
class ConfigManager;

class MeetingManager : public QObject {
    Q_OBJECT

public:
    explicit MeetingManager(
        std::shared_ptr<core::Core> client,
        NavigationController* navigationController,
        DialogsController* dialogsController,
        QObject* parent = nullptr);

    void setMeetingWidget(MeetingWidget* meetingWidget);
    void setNotificationController(NotificationController* notificationController);
    void setConfigManager(ConfigManager* configManager);
    bool isInMeeting() const;

public slots:
    void onMeetingCreated(const QString& meetingId);
    void onMeetingCreateRejected(std::error_code ec);
    void onJoinMeetingRequestTimeout();

    // Media controls
    void onCameraClicked(bool toggled);
    void onScreenShareClicked(bool toggled);
    void onScreenSelected(int screenIndex);
    void onScreenShareDialogCancelled();

    void onJoinMeetingRequested(const QString& meetingId);
    void onCancelMeetingJoinRequested();
    void onAcceptJoinMeetingRequestClicked(const QString& friendNickname);
    void onDeclineJoinMeetingRequestClicked(const QString& friendNickname);

    void onMeetingJoinRequestReceived(const QString& friendNickname);
    void onMeetingJoinRequestCancelled(const QString& friendNickname);
    void onJoinMeetingAccepted(const QString& meetingId, const QStringList& participants);
    void onJoinMeetingDeclined(const QString& meetingId);
    void onMeetingNotFound();
    void onMeetingEndedByOwner();
    void onMeetingParticipantJoined(const QString& nickname);
    void onMeetingParticipantLeft(const QString& nickname);

    // Media frame slots (called from CoreEventListener via QMetaObject::invokeMethod)
    void onLocalScreenFrame(QByteArray data, int width, int height);
    void onLocalCameraFrame(QByteArray data, int width, int height);
    void onIncomingScreenFrame(QByteArray data, int width, int height);
    void onIncomingCameraFrame(QByteArray data, int width, int height, const QString& nickname);

    // Media state slots
    void onIncomingScreenSharingStarted();
    void onIncomingScreenSharingStopped();
    void onIncomingCameraSharingStarted();
    void onIncomingCameraSharingStopped();
    void onStartScreenSharingError();
    void onStartCameraSharingError();

signals:
    void meetingCreated(const QString& meetingId);

private:
    bool hasRemoteParticipants() const;
    void clearLocalParticipantVideo();
    void clearRemoteParticipantVideos();
    void restrictMediaButtons();
    void unrestrictMediaButtons();
    void tryStartCameraWithSession();
    std::shared_ptr<core::Core> m_coreClient = nullptr;
    NavigationController* m_navigationController = nullptr;
    DialogsController* m_dialogsController = nullptr;
    NotificationController* m_notificationController = nullptr;
    ConfigManager* m_configManager = nullptr;
    MeetingWidget* m_meetingWidget = nullptr;
};
