#include "meetingManager.h"
#include "logic/navigationController.h"
#include "dialogs/dialogsController.h"
#include "notifications/notificationController.h"
#include "logic/configManager.h"
#include "widgets/meetingWidget.h"

MeetingManager::MeetingManager(
    std::shared_ptr<core::Core> client,
    NavigationController* navigationController,
    DialogsController* dialogsController,
    QObject* parent)
    : QObject(parent)
    , m_coreClient(client)
    , m_navigationController(navigationController)
    , m_dialogsController(dialogsController)
{
}

void MeetingManager::setMeetingWidget(MeetingWidget* meetingWidget)
{
    m_meetingWidget = meetingWidget;
}

void MeetingManager::setNotificationController(NotificationController* notificationController)
{
    m_notificationController = notificationController;
}

void MeetingManager::setConfigManager(ConfigManager* configManager)
{
    m_configManager = configManager;
}

void MeetingManager::onMeetingCreated(const QString& meetingId)
{
    emit meetingCreated(meetingId);
}

void MeetingManager::onMeetingCreateRejected(std::error_code ec)
{
    (void)ec;
    if (m_notificationController) {
        m_notificationController->showErrorNotification("Meeting creation was rejected", 2000);
    }
}

void MeetingManager::onJoinMeetingRequestTimeout()
{
    if (m_dialogsController) {
        m_dialogsController->showMeetingsJoinRequestTimeout();
    }
    if (m_notificationController) {
        m_notificationController->showErrorNotification("Join meeting request timed out", 2000);
    }
}

void MeetingManager::onJoinMeetingRequested(const QString& meetingId)
{
    if (!m_coreClient) return;
    std::error_code ec = m_coreClient->joinMeeting(meetingId.toStdString());
    if (ec && m_notificationController) {
        m_notificationController->showErrorNotification("Failed to join meeting", 2000);
    }
}

void MeetingManager::onCancelMeetingJoinRequested()
{
    if (!m_coreClient) return;
    std::error_code ec = m_coreClient->cancelMeetingJoin();
    if (ec && m_notificationController) {
        m_notificationController->showErrorNotification("Failed to cancel join request", 2000);
    }
}

void MeetingManager::onAcceptJoinMeetingRequestClicked(const QString& friendNickname)
{
    if (!m_coreClient) return;
    std::error_code ec = m_coreClient->acceptJoinMeetingRequest(friendNickname.toStdString());
    if (ec && m_notificationController) {
        m_notificationController->showErrorNotification("Failed to accept join request", 2000);
    }
}

void MeetingManager::onDeclineJoinMeetingRequestClicked(const QString& friendNickname)
{
    if (!m_coreClient) return;
    std::error_code ec = m_coreClient->declineJoinMeetingRequest(friendNickname.toStdString());
    if (ec && m_notificationController) {
        m_notificationController->showErrorNotification("Failed to decline join request", 2000);
    }
}

void MeetingManager::onMeetingJoinRequestReceived(const QString& friendNickname)
{
    if (m_meetingWidget) {
        m_meetingWidget->addJoinRequest(friendNickname);
    }
}

void MeetingManager::onMeetingJoinRequestCancelled(const QString& friendNickname)
{
    if (m_meetingWidget) {
        m_meetingWidget->removeJoinRequest(friendNickname);
    }
}

void MeetingManager::onJoinMeetingAccepted(const QString& meetingId, const QStringList& participants)
{
    if (m_dialogsController) {
        m_dialogsController->hideMeetingsManagementDialog();
    }
    if (m_meetingWidget && m_coreClient && m_configManager) {
        m_meetingWidget->clearParticipants();
        m_meetingWidget->setCallName(meetingId);
        m_meetingWidget->setIsOwner(false);
        m_meetingWidget->setInputVolume(m_configManager->getInputVolume());
        m_meetingWidget->setOutputVolume(m_configManager->getOutputVolume());
        const std::string myNickname = m_coreClient->getMyNickname();
        const QString myNicknameQ = QString::fromStdString(myNickname);
        for (const QString& nickname : participants) {
            if (!myNickname.empty() && nickname == myNicknameQ) continue;
            m_meetingWidget->addParticipant(nickname);
        }
        if (!myNickname.empty()) {
            m_meetingWidget->addParticipant(myNicknameQ);
        }
        m_meetingWidget->setMicrophoneMuted(m_coreClient->isMicrophoneMuted());
        m_meetingWidget->setSpeakerMuted(m_coreClient->isSpeakerMuted());
    }
    if (m_navigationController) {
        m_navigationController->switchToMeetingWidget();
    }
}

void MeetingManager::onJoinMeetingDeclined(const QString& meetingId)
{
    (void)meetingId;
    if (m_dialogsController) {
        m_dialogsController->showMeetingsJoinRequestTimeout();
    }
    if (m_notificationController) {
        m_notificationController->showErrorNotification("Join meeting was declined", 2000);
    }
}

void MeetingManager::onMeetingNotFound()
{
    if (m_dialogsController) {
        m_dialogsController->showMeetingsJoinRequestTimeout();
    }
    if (m_notificationController) {
        m_notificationController->showErrorNotification("Meeting not found", 2000);
    }
}

void MeetingManager::onMeetingEndedByOwner()
{
    if (m_navigationController) {
        m_navigationController->switchToMainMenuWidget();
    }
    if (m_meetingWidget) {
        m_meetingWidget->clearParticipants();
    }
}

void MeetingManager::onMeetingParticipantJoined(const QString& nickname)
{
    if (m_meetingWidget) {
        m_meetingWidget->addParticipant(nickname);
    }
}

void MeetingManager::onMeetingParticipantLeft(const QString& nickname)
{
    if (m_meetingWidget) {
        m_meetingWidget->removeParticipant(nickname);
    }
}
