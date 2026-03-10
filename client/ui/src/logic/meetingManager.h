#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
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

public slots:
    void onMeetingCreated(const QString& meetingId);
    void onMeetingCreateRejected(std::error_code ec);
    void onJoinMeetingRequestTimeout();

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

signals:
    void meetingCreated(const QString& meetingId);

private:
    std::shared_ptr<core::Core> m_coreClient = nullptr;
    NavigationController* m_navigationController = nullptr;
    DialogsController* m_dialogsController = nullptr;
    NotificationController* m_notificationController = nullptr;
    ConfigManager* m_configManager = nullptr;
    MeetingWidget* m_meetingWidget = nullptr;
};
