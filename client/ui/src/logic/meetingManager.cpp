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
#include <vector>

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

bool MeetingManager::hasRemoteParticipants() const
{
    if (!m_coreClient) return false;
    const std::string myNickname = m_coreClient->getMyNickname();
    std::vector<std::string> participants = m_coreClient->getMeetingParticipants();
    for (const auto& p : participants) {
        if (myNickname.empty() || p != myNickname)
            return true;
    }
    return false;
}

void MeetingManager::restrictMediaButtons()
{
    if (m_meetingWidget) {
        m_meetingWidget->setCameraButtonRestricted(true);
        m_meetingWidget->setScreenShareButtonRestricted(true);
    }
}

void MeetingManager::unrestrictMediaButtons()
{
    if (m_meetingWidget) {
        m_meetingWidget->setCameraButtonRestricted(false);
        m_meetingWidget->setScreenShareButtonRestricted(false);
    }
}

void MeetingManager::clearLocalParticipantVideo()
{
    if (!m_meetingWidget || !m_coreClient) return;
    const std::string myNickname = m_coreClient->getMyNickname();
    if (!myNickname.empty()) {
        m_meetingWidget->clearParticipantVideo(QString::fromStdString(myNickname));
    }
}

void MeetingManager::clearRemoteParticipantVideos()
{
    if (!m_meetingWidget || !m_coreClient) return;
    const std::string myNickname = m_coreClient->getMyNickname();
    const std::vector<std::string> participants = m_coreClient->getMeetingParticipants();
    for (const auto& participant : participants) {
        if (myNickname.empty() || participant != myNickname) {
            m_meetingWidget->clearParticipantVideo(QString::fromStdString(participant));
        }
    }
}

void MeetingManager::onMeetingCreated(const QString& meetingId)
{
    if (m_meetingWidget) {
        m_meetingWidget->resetMeetingState();
    }
    restrictMediaButtons();
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
        if (!hasRemoteParticipants()) {
            if (m_meetingWidget)
                m_meetingWidget->setCameraButtonActive(false);
            if (m_notificationController)
                m_notificationController->showErrorNotification("Camera sharing requires at least one other participant", 2000);
            return;
        }
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
        clearLocalParticipantVideo();
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
        if (!hasRemoteParticipants()) {
            if (m_meetingWidget)
                m_meetingWidget->setScreenShareButtonActive(false);
            if (m_notificationController)
                m_notificationController->showErrorNotification("Screen sharing requires at least one other participant", 2000);
            return;
        }
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
    if (!hasRemoteParticipants()) {
        onStartScreenSharingError();
        return;
    }

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
    const std::string myNickname = m_coreClient ? m_coreClient->getMyNickname() : "";
    const QString myNicknameQ = QString::fromStdString(myNickname);

    if (m_dialogsController) {
        m_dialogsController->hideMeetingsManagementDialog();
    }
    if (m_meetingWidget && m_coreClient && m_configManager) {
        m_meetingWidget->resetMeetingState();
        m_meetingWidget->setCallName(meetingId);
        m_meetingWidget->setIsOwner(false);
        m_meetingWidget->setInputVolume(m_configManager->getInputVolume());
        m_meetingWidget->setOutputVolume(m_configManager->getOutputVolume());
        for (const QString& nickname : participants) {
            if (!myNickname.empty() && nickname == myNicknameQ) continue;
            m_meetingWidget->addParticipant(nickname);
        }
        if (!myNickname.empty()) {
            m_meetingWidget->addParticipant(myNicknameQ);
        }
        m_meetingWidget->setMicrophoneMuted(m_coreClient->isMicrophoneMuted());
        m_meetingWidget->setSpeakerMuted(m_coreClient->isSpeakerMuted());
        if (hasRemoteParticipants()) {
            unrestrictMediaButtons();
            if (m_coreClient->isViewingRemoteScreen())
                m_meetingWidget->setScreenShareButtonRestricted(true);
        } else {
            restrictMediaButtons();
        }
    }
    if (m_navigationController) {
        m_navigationController->switchToMeetingWidget();
    }

    // Automatically start camera for the local user when joining a meeting,
    // if the corresponding setting is enabled.
    tryStartCameraWithSession();
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
    if (m_meetingWidget) {
        m_meetingWidget->resetMeetingState();
    }
    if (m_navigationController) {
        m_navigationController->switchToMainMenuWidget();
    }
}

void MeetingManager::onMeetingParticipantJoined(const QString& nickname)
{
    if (m_meetingWidget) {
        m_meetingWidget->addParticipant(nickname);
    }
    unrestrictMediaButtons();
    if (m_coreClient && m_coreClient->isViewingRemoteScreen() && m_meetingWidget)
        m_meetingWidget->setScreenShareButtonRestricted(true);

    // When a participant joins the meeting created by the local user,
    // media controls become available. Respect the "start camera with session"
    // setting and start camera automatically if needed.
    tryStartCameraWithSession();
}

void MeetingManager::onMeetingParticipantLeft(const QString& nickname)
{
    if (m_meetingWidget) {
        m_meetingWidget->clearParticipantVideo(nickname);
        m_meetingWidget->removeParticipant(nickname);
    }

    if (m_coreClient) {
        const std::string myNickname = m_coreClient->getMyNickname();
        std::vector<std::string> participants = m_coreClient->getMeetingParticipants();
        bool hasRemoteParticipants = false;
        for (const auto& p : participants) {
            if (myNickname.empty() || p != myNickname) {
                hasRemoteParticipants = true;
                break;
            }
        }
        if (!hasRemoteParticipants) {
            if (m_coreClient->isScreenSharing()) {
                m_coreClient->stopScreenSharing();
                if (m_meetingWidget) {
                    m_meetingWidget->hideMainScreen();
                    m_meetingWidget->hideAdditionalScreens();
                    m_meetingWidget->setScreenShareButtonActive(false);
                }
            }
            if (m_coreClient->isCameraSharing()) {
                m_coreClient->stopCameraSharing();
                clearLocalParticipantVideo();
                if (m_configManager) {
                    m_configManager->setCameraActive(false);
                }
                if (m_meetingWidget) {
                    m_meetingWidget->setCameraButtonActive(false);
                }
            }
        }
        if (hasRemoteParticipants) {
            unrestrictMediaButtons();
            if (m_coreClient->isViewingRemoteScreen() && m_meetingWidget)
                m_meetingWidget->setScreenShareButtonRestricted(true);
        } else {
            restrictMediaButtons();
        }
    }
}

void MeetingManager::onMeetingParticipantConnectionDown(const QString& nickname)
{
    if (m_meetingWidget) {
        m_meetingWidget->setParticipantConnectionDown(nickname, true);
    }
}

void MeetingManager::onMeetingParticipantConnectionRestored(const QString& nickname)
{
    if (m_meetingWidget) {
        m_meetingWidget->setParticipantConnectionDown(nickname, false);
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
        clearLocalParticipantVideo();
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
        m_meetingWidget->clearParticipantVideo(nickname);
        return;
    }

    QImage image(reinterpret_cast<const uchar*>(data.constData()), width, height, width * 3, QImage::Format_RGB888);
    QPixmap pixmap = QPixmap::fromImage(image.copy());

    m_meetingWidget->updateParticipantVideo(nickname, pixmap);
}

void MeetingManager::onIncomingScreenSharingStarted()
{
    if (m_meetingWidget) {
        m_meetingWidget->showEnterFullscreenButton();
        m_meetingWidget->setScreenShareButtonRestricted(true);
    }
}

void MeetingManager::onIncomingScreenSharingStopped()
{
    if (m_meetingWidget) {
        m_meetingWidget->hideMainScreen();
        m_meetingWidget->hideEnterFullscreenButton();
    }
    if (hasRemoteParticipants() && m_meetingWidget)
        m_meetingWidget->setScreenShareButtonRestricted(false);
}

void MeetingManager::onIncomingCameraSharingStarted()
{
    // Remote camera frames are handled by onIncomingCameraFrame.
}

void MeetingManager::onIncomingCameraSharingStopped()
{
    if (!m_meetingWidget) return;
    clearRemoteParticipantVideos();
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
    clearLocalParticipantVideo();
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

void MeetingManager::tryStartCameraWithSession()
{
    if (!m_coreClient || !m_meetingWidget || !m_configManager) return;

    // Respect user preference: only start camera automatically if enabled.
    if (!m_configManager->isStartCameraWithSession()) return;

    // Do not attempt to start if already sharing or if no remote participants.
    if (m_coreClient->isCameraSharing()) return;
    if (!hasRemoteParticipants()) return;

    if (!m_coreClient->isCameraAvailable()) {
        // If camera is not available, disable auto-start to avoid repeated failures.
        m_configManager->setStartCameraWithSession(false);
        return;
    }

    std::error_code ec = m_coreClient->startCameraSharing("");
    if (ec) {
        onStartCameraSharingError();
        return;
    }

    m_meetingWidget->setCameraButtonActive(true);
    m_configManager->setCameraActive(true);
}
