#include "meetingManager.h"
#include "logic/navigationController.h"
#include "dialogs/dialogsController.h"
#include "notifications/notificationController.h"
#include "logic/configManager.h"
#include "widgets/meetingWidget.h"
#include "widgets/components/screen.h"
#include "constants/constant.h"

#include <QGuiApplication>
#include <QScreen>
#include <QList>
#include <QImage>
#include <QPixmap>

#include <cmath>

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

bool MeetingManager::isInMeeting() const
{
    return m_coreClient && m_coreClient->isInMeeting();
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

void MeetingManager::onCameraClicked(bool toggled)
{
    if (!m_coreClient) return;

    if (toggled) {
        if (!m_coreClient->isCameraAvailable()) {
            if (m_meetingWidget) {
                m_meetingWidget->setCameraButtonActive(false);
            }
            if (m_notificationController) {
                m_notificationController->showErrorNotification("Failed to start camera sharing", 1500);
            }
            return;
        }

        std::error_code ec = m_coreClient->startCameraSharing("");
        if (ec) {
            onStartCameraSharingError();
        } else {
            if (m_configManager) {
                m_configManager->setCameraActive(true);
            }
            if (m_meetingWidget) {
                m_meetingWidget->setCameraButtonActive(true);
            }
        }
    } else {
        m_coreClient->stopCameraSharing();
        if (m_configManager) {
            m_configManager->setCameraActive(false);
        }
        if (m_meetingWidget) {
            m_meetingWidget->setCameraButtonActive(false);
        }
    }
}

void MeetingManager::onScreenShareClicked(bool toggled)
{
    if (!m_coreClient || !m_dialogsController) return;

    if (toggled) {
        const QList<QScreen*> screens = QGuiApplication::screens();
        if (screens.isEmpty()) {
            if (m_meetingWidget) {
                m_meetingWidget->setScreenShareButtonActive(false);
            }
            if (m_notificationController) {
                m_notificationController->showErrorNotification("No screens available for sharing", 1500);
            }
            return;
        }

        m_dialogsController->showScreenShareDialog(screens);
    }
    else {
        m_coreClient->stopScreenSharing();

        if (m_meetingWidget) {
            m_meetingWidget->hideMainScreen();
            m_meetingWidget->hideAdditionalScreens();
            m_meetingWidget->setScreenShareButtonActive(false);
        }
    }
}

void MeetingManager::onScreenSelected(int screenIndex)
{
    if (!m_coreClient || !m_coreClient->isInMeeting()) return;

    QList<QScreen*> screens = QGuiApplication::screens();
    if (screenIndex < 0 || screenIndex >= screens.size() || !screens[screenIndex]) {
        onStartScreenSharingError();
        return;
    }

    QScreen* selectedScreen = screens[screenIndex];
    const QRect geometry = selectedScreen->geometry();
    const double dpr = selectedScreen->devicePixelRatio();

    core::media::Screen target;
    target.index = screenIndex;
    target.osId = selectedScreen->name().toStdString();
    target.x = static_cast<int>(std::lround(geometry.x() * dpr));
    target.y = static_cast<int>(std::lround(geometry.y() * dpr));
    target.width = static_cast<int>(std::lround(geometry.width() * dpr));
    target.height = static_cast<int>(std::lround(geometry.height() * dpr));
    target.dpr = dpr;
    std::error_code ec = m_coreClient->startScreenSharing(target);
    if (ec) {
        onStartScreenSharingError();
    }
}

void MeetingManager::onScreenShareDialogCancelled()
{
    if (!m_coreClient || !m_coreClient->isInMeeting()) return;
    if (m_meetingWidget) {
        m_meetingWidget->setScreenShareButtonActive(false);
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

void MeetingManager::onLocalScreenFrame(QByteArray data, int width, int height)
{
    if (!m_meetingWidget || width <= 0 || height <= 0) return;
    if (data.size() < width * height * 3) return;
    if (!m_coreClient || !m_coreClient->isScreenSharing()) {
        m_meetingWidget->hideMainScreen();
        return;
    }

    QImage image(reinterpret_cast<const uchar*>(data.constData()), width, height, width * 3, QImage::Format_RGB888);
    QPixmap pixmap = QPixmap::fromImage(image.copy());
    m_meetingWidget->showFrameInMainScreen(pixmap, Screen::ScaleMode::KeepAspectRatio);
}

void MeetingManager::onLocalCameraFrame(QByteArray data, int width, int height)
{
    if (!m_meetingWidget || width <= 0 || height <= 0) return;
    if (data.size() < width * height * 3) return;
    if (!m_coreClient || !m_coreClient->isCameraSharing()) {
        return;
    }

    QImage image(reinterpret_cast<const uchar*>(data.constData()), width, height, width * 3, QImage::Format_RGB888);
    QPixmap pixmap = QPixmap::fromImage(image.copy());

    const std::string myNickname = m_coreClient->getMyNickname();
    if (!myNickname.empty()) {
        m_meetingWidget->updateParticipantVideo(QString::fromStdString(myNickname), pixmap);
    }
}

void MeetingManager::onIncomingScreenFrame(QByteArray data, int width, int height)
{
    if (!m_meetingWidget || width <= 0 || height <= 0) return;
    if (data.size() < width * height * 3) return;
    if (!m_coreClient || !m_coreClient->isViewingRemoteScreen()) {
        m_meetingWidget->hideMainScreen();
        return;
    }

    QImage image(reinterpret_cast<const uchar*>(data.constData()), width, height, width * 3, QImage::Format_RGB888);
    QPixmap pixmap = QPixmap::fromImage(image.copy());
    m_meetingWidget->showFrameInMainScreen(pixmap, Screen::ScaleMode::KeepAspectRatio);
}

void MeetingManager::onIncomingCameraFrame(QByteArray data, int width, int height, const QString& nickname)
{
    if (!m_meetingWidget || width <= 0 || height <= 0) return;
    if (data.size() < width * height * 3) return;
    if (!m_coreClient || !m_coreClient->isViewingRemoteCamera()) {
        return;
    }

    QImage image(reinterpret_cast<const uchar*>(data.constData()), width, height, width * 3, QImage::Format_RGB888);
    QPixmap pixmap = QPixmap::fromImage(image.copy());

    m_meetingWidget->updateParticipantVideo(nickname, pixmap);
}

void MeetingManager::onIncomingScreenSharingStarted()
{
    if (m_meetingWidget) {
        m_meetingWidget->setScreenShareButtonRestricted(true);
        m_meetingWidget->showEnterFullscreenButton();
    }
}

void MeetingManager::onIncomingScreenSharingStopped()
{
    if (m_meetingWidget) {
        m_meetingWidget->hideMainScreen();
        m_meetingWidget->setScreenShareButtonRestricted(false);
        m_meetingWidget->hideEnterFullscreenButton();
    }
}

void MeetingManager::onIncomingCameraSharingStarted()
{
    // Remote camera frames are handled by onIncomingCameraFrame.
}

void MeetingManager::onIncomingCameraSharingStopped()
{
    if (!m_meetingWidget) return;
    m_meetingWidget->removeAdditionalScreen(ADDITIONAL_SCREEN_ID_REMOTE_CAMERA);
    const bool screenSharingActive = m_coreClient && (m_coreClient->isScreenSharing() || m_coreClient->isViewingRemoteScreen());
    if (!screenSharingActive) {
        m_meetingWidget->hideMainScreen();
    }
}

void MeetingManager::onStartCameraSharingError()
{
    if (m_coreClient) {
        m_coreClient->stopCameraSharing();
    }
    if (m_meetingWidget) {
        m_meetingWidget->setCameraButtonActive(false);
    }
    if (m_notificationController) {
        m_notificationController->showErrorNotification("Failed to start camera sharing", 1500);
    }
}

void MeetingManager::onStartScreenSharingError()
{
    if (m_meetingWidget) {
        m_meetingWidget->setScreenShareButtonActive(false);
        m_meetingWidget->hideMainScreen();
        m_meetingWidget->hideAdditionalScreens();
    }
    if (m_notificationController) {
        m_notificationController->showErrorNotification("Failed to start screen sharing", 1500);
    }
}
