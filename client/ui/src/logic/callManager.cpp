#include "callManager.h"
#include "audio/audioEffectsManager.h"
#include "logic/navigationController.h"
#include "dialogs/dialogsController.h"
#include "notifications/notificationController.h"
#include "logic/updateManager.h"
#include "widgets/mainMenuWidget.h"
#include "widgets/callWidget.h"
#include "widgets/components/screen.h"
#include "utilities/logger.h"
#include "constants/constant.h"
#include "constants/errorCode.h"
#include <system_error>
#include <QList>
#include <QImage>
#include <QPixmap>
#include <QGuiApplication>
#include <cmath>

CallManager::CallManager(std::shared_ptr<core::Core> client, AudioEffectsManager* audioManager, NavigationController* navigationController, DialogsController* dialogsController, UpdateManager* updateManager, QObject* parent)
    : QObject(parent)
    , m_coreClient(client)
    , m_audioManager(audioManager)
    , m_navigationController(navigationController)
    , m_dialogsController(dialogsController)
    , m_updateManager(updateManager)
{
    if (m_dialogsController)
    {
        connect(m_dialogsController, &DialogsController::incomingCallAccepted, this, &CallManager::onAcceptCallButtonClicked);
        connect(m_dialogsController, &DialogsController::incomingCallDeclined, this, &CallManager::onDeclineCallButtonClicked);
        connect(m_dialogsController, &DialogsController::incomingCallsDialogClosed, this, &CallManager::onIncomingCallsDialogClosed);
    }
}

void CallManager::setWidgets(MainMenuWidget* mainMenuWidget, CallWidget* callWidget, QStackedLayout* stackedLayout)
{
    m_mainMenuWidget = mainMenuWidget;
    m_callWidget = callWidget;
    m_stackedLayout = stackedLayout;
}

void CallManager::setNotificationController(NotificationController* notificationController)
{
    m_notificationController = notificationController;
}

void CallManager::onStartCallingButtonClicked(const QString& friendNickname)
{
    if (!m_mainMenuWidget) return;

    if (!m_coreClient) return;

    if (friendNickname.isEmpty()) {
        m_mainMenuWidget->setErrorMessage("cannot be empty");
        return;
    }

    if (friendNickname.toStdString() == m_coreClient->getMyNickname()) {
        m_mainMenuWidget->setErrorMessage("cannot call yourself");
        return;
    }

    std::error_code ec = m_coreClient->startOutgoingCall(friendNickname.toStdString());
    if (ec == core::constant::make_error_code(core::constant::ErrorCode::accept_call_instead_of_start)) {
        switchToActiveCall(friendNickname);
    }
    else if (ec) {
        if (!m_coreClient->isConnectionDown()) {
            handleStartCallingErrorNotificationAppearance();
        }
    }
    else {
        if (m_mainMenuWidget) {
            m_mainMenuWidget->setCallButtonEnabled(false);
        }
    }
}

void CallManager::onStopCallingButtonClicked()
{
    if (!m_mainMenuWidget) return;

    if (!m_coreClient) return;

    std::error_code ec = m_coreClient->stopOutgoingCall();
    if (ec) {
        if (!m_coreClient->isConnectionDown()) {
            handleStopCallingErrorNotificationAppearance();
        }
    }
    else {
        if (m_mainMenuWidget) {
            m_mainMenuWidget->removeCallingPanel();
            m_mainMenuWidget->setStatusLabelOnline();
        }
        if (m_audioManager) {
            m_audioManager->stopCallingRingtone();
        }
    }
}

void CallManager::switchToActiveCall(const QString& friendNickname)
{
    m_incomingCalls.remove(friendNickname);
    if (m_dialogsController) {
        m_dialogsController->hideIncomingCallsDialog(friendNickname);
        m_dialogsController->hideUpdateAvailableDialog();
    }
    if (m_audioManager && m_incomingCalls.isEmpty()) {
        m_audioManager->stopIncomingCallRingtone();
    }
    if (m_coreClient && m_coreClient->isScreenSharing()) {
        m_coreClient->stopScreenSharing();
    }
    if (m_coreClient && m_coreClient->isCameraSharing()) {
        m_coreClient->stopCameraSharing();
    }
    if (m_mainMenuWidget) {
        m_mainMenuWidget->removeCallingPanel();
    }
    updateIncomingCallsUi();
    if (m_callWidget) {
        m_callWidget->show();
        m_callWidget->setCallInfo(friendNickname);
    }
    if (m_navigationController) {
        m_navigationController->switchToCallWidget(friendNickname);
    }
    if (m_audioManager) {
        m_audioManager->playCallJoinedEffect();
    }
}

void CallManager::onAcceptCallButtonClicked(const QString& friendNickname)
{
    if (!m_coreClient) return;

    std::error_code ec = m_coreClient->acceptCall(friendNickname.toStdString());
    if (ec) {
        if (!m_coreClient->isConnectionDown()) {
            handleAcceptCallErrorNotificationAppearance();
        }
    }
    else {
        switchToActiveCall(friendNickname);
    }
}

void CallManager::onDeclineCallButtonClicked(const QString& friendNickname)
{
    if (!m_coreClient) return;

    std::error_code ec = m_coreClient->declineCall(friendNickname.toStdString());
    if (ec) {
        if (!m_coreClient->isConnectionDown()) {
            handleDeclineCallErrorNotificationAppearance();
        }
    }
    else {
        m_incomingCalls.remove(friendNickname);
        if (m_dialogsController) {
            m_dialogsController->hideIncomingCallsDialog(friendNickname);
        }
        updateIncomingCallsUi();
        if (m_audioManager) {
            if (m_incomingCalls.isEmpty())
                m_audioManager->stopIncomingCallRingtone();
            m_audioManager->playCallingEndedEffect();
        }
    }
}

void CallManager::onAudioDevicePickerRequested()
{
    if (!m_dialogsController || !m_coreClient) return;

    m_dialogsController->showAudioSettingsDialog(
        true,
        m_coreClient->isMicrophoneMuted(),
        m_coreClient->isSpeakerMuted(),
        m_coreClient->getInputVolume(),
        m_coreClient->getOutputVolume(),
        m_coreClient->getCurrentInputDevice(),
        m_coreClient->getCurrentOutputDevice());
}

void CallManager::onAudioSettingsDialogClosed()
{
    if (m_callWidget)
        m_callWidget->setAudioSettingsDialogOpen(false);
}

void CallManager::onCallWidgetAudioSettingsRequested()
{
    if (!m_dialogsController || !m_coreClient) return;

    m_dialogsController->showAudioSettingsDialog(
        true,
        m_coreClient->isMicrophoneMuted(),
        m_coreClient->isSpeakerMuted(),
        m_coreClient->getInputVolume(),
        m_coreClient->getOutputVolume(),
        m_coreClient->getCurrentInputDevice(),
        m_coreClient->getCurrentOutputDevice());
    if (m_callWidget)
        m_callWidget->setAudioSettingsDialogOpen(true);
}

void CallManager::onActivateCameraClicked(bool active)
{
    if (!m_coreClient) return;

    if (active) {
        std::error_code ec = m_coreClient->startCameraSharing("");
        if (ec) {
            onStartCameraSharingError();
        }
    } else {
        m_coreClient->stopCameraSharing();
    }
}

void CallManager::onScreenShareClicked(bool toggled)
{
    if (!m_coreClient || !m_dialogsController) return;

    if (toggled) {
        QList<QScreen*> screens = QGuiApplication::screens();
        if (screens.isEmpty()) return;

        m_dialogsController->showScreenShareDialog(screens);
    }
    else {
        m_coreClient->stopScreenSharing();

        if (m_callWidget) {
            m_callWidget->hideMainScreen();
            m_callWidget->hideAdditionalScreens();
        }
    }
}

void CallManager::onScreenSelected(int screenIndex)
{
    if (!m_coreClient) return;

    QList<QScreen*> screens = QGuiApplication::screens();
    if (screenIndex < 0 || screenIndex >= screens.size() || !screens[screenIndex]) {
        onStartScreenSharingError();
        return;
    }

    QScreen* selectedScreen = screens[screenIndex];
    const QRect geometry = selectedScreen->geometry();
    const double dpr = selectedScreen->devicePixelRatio();

    core::media::ScreenCaptureTarget target;
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

void CallManager::onScreenShareDialogCancelled()
{
    if (m_callWidget) {
        m_callWidget->setScreenShareButtonActive(false);
    }
}

void CallManager::onCallWidgetCameraClicked(bool toggled)
{
    if (!m_coreClient) return;

    if (toggled) {
        std::error_code ec = m_coreClient->startCameraSharing("");
        if (ec) {
            onStartCameraSharingError();
        }
    } else {
        m_coreClient->stopCameraSharing();
        if (m_callWidget) {
            m_callWidget->hideMainScreen();
            m_callWidget->hideAdditionalScreens();
        }
    }
}

void CallManager::onEndCallButtonClicked()
{
    if (!m_coreClient) return;

    hideOperationDialog();

    std::error_code ec = m_coreClient->endCall();
    if (ec) {
        if (!m_coreClient->isConnectionDown()) {
            handleEndCallErrorNotificationAppearance();
        }
    }
    else {
        if (m_mainMenuWidget) {
            m_mainMenuWidget->setStatusLabelOnline();
        }
        if (m_callWidget && m_callWidget->isFullScreen()) {
            emit endCallFullscreenExitRequested();
        }

        if (m_callWidget) {
            resetCallWidgetState();
            m_callWidget->hide();
        }

        if (m_navigationController) {
            m_navigationController->switchToMainMenuWidget();
        }

        if (m_updateManager) {
            m_updateManager->showUpdateAvailableDialogIfNeeded();
        }

        if (m_audioManager) {
            m_audioManager->playEndCallEffect();
        }
    }
}

void CallManager::onStartCallingResult(std::error_code ec)
{
    if (!m_mainMenuWidget || !m_coreClient) return;

    if (!ec) {
        m_mainMenuWidget->showCallingPanel(QString::fromStdString(m_coreClient->getNicknameWhomCalling()));
        m_mainMenuWidget->setStatusLabelCalling();
        if (m_audioManager) {
            m_audioManager->playCallingRingtone();
        }
    }
    else {
        if (m_mainMenuWidget) {
            m_mainMenuWidget->setCallButtonEnabled(true);
        }
        if (ec == core::constant::make_error_code(core::constant::ErrorCode::network_error) || ec == core::constant::make_error_code(core::constant::ErrorCode::unexisting_user)) {
            QString errorMessage;
            if (ec == core::constant::make_error_code(core::constant::ErrorCode::unexisting_user)) {
                errorMessage = "User not found";
                LOG_WARN("Start calling failed: user not found");
            }
            else {
                errorMessage = "Something went wrong please try again";
                LOG_ERROR("Start calling failed: timeout or network error");
            }

            if (!m_coreClient->isConnectionDown()) {
                m_mainMenuWidget->setErrorMessage(errorMessage);
            }
        }
    }
}

void CallManager::onMaximumCallingTimeReached()
{
    if (!m_mainMenuWidget) return;

    m_mainMenuWidget->removeCallingPanel();
    m_mainMenuWidget->setStatusLabelOnline();
    if (m_audioManager) {
        m_audioManager->stopCallingRingtone();
    }
}

void CallManager::onCallingAccepted()
{
    if (!m_mainMenuWidget || !m_coreClient) return;

    m_incomingCalls.clear();
    updateIncomingCallsUi();

    if (m_audioManager) {
        m_audioManager->stopCallingRingtone();
        m_audioManager->stopIncomingCallRingtone();
    }

    m_mainMenuWidget->removeCallingPanel();
    m_mainMenuWidget->setStatusLabelBusy();

    if (m_navigationController) {
        m_navigationController->switchToCallWidget(QString::fromStdString(m_coreClient->getNicknameInCallWith()));
    }

    if (m_dialogsController) {
        m_dialogsController->hideUpdateAvailableDialog();
    }

    if (m_audioManager) {
        m_audioManager->playCallJoinedEffect();
    }
}

void CallManager::onCallingDeclined()
{
    if (!m_mainMenuWidget) return;

    m_mainMenuWidget->removeCallingPanel();
    m_mainMenuWidget->setStatusLabelOnline();

    if (m_audioManager) {
        m_audioManager->stopCallingRingtone();
    }

    if (m_audioManager) {
        m_audioManager->playCallingEndedEffect();
    }
}

void CallManager::onIncomingCall(const QString& friendNickname)
{
    if (!m_coreClient) return;

    if (m_audioManager) {
        m_audioManager->playIncomingCallRingtone();
    }

    m_incomingCalls.insert(friendNickname, { friendNickname, DEFAULT_INCOMING_CALL_SECONDS });
    updateIncomingCallsUi();
}

void CallManager::onIncomingCallExpired(const QString& friendNickname)
{
    m_incomingCalls.remove(friendNickname);
    updateIncomingCallsUi();

    if (m_audioManager && m_incomingCalls.isEmpty()) {
        m_audioManager->stopIncomingCallRingtone();
    }

    if (m_audioManager) {
        m_audioManager->playCallingEndedEffect();
    }
}

void CallManager::handleAcceptCallErrorNotificationAppearance()
{
    QString errorText = "Failed to accept call";

    if (m_notificationController) {
        m_notificationController->showErrorNotification(errorText, 1500);
    }
}

void CallManager::handleDeclineCallErrorNotificationAppearance()
{
    QString errorText = "Failed to decline call. Please try again";

    if (m_notificationController) {
        m_notificationController->showErrorNotification(errorText, 1500);
    }
}

void CallManager::handleStartCallingErrorNotificationAppearance()
{
    QString errorText = "Failed to start calling. Please try again";

    if (m_notificationController) {
        m_notificationController->showErrorNotification(errorText, 1500);
    }
}

void CallManager::handleStopCallingErrorNotificationAppearance()
{
    QString errorText = "Failed to stop calling. Please try again";

    if (m_notificationController) {
        m_notificationController->showErrorNotification(errorText, 1500);
    }
}

void CallManager::handleEndCallErrorNotificationAppearance()
{
    QString errorText = "Failed to end call. Please try again";

    if (m_notificationController) {
        m_notificationController->showErrorNotification(errorText, 1500);
    }
}

void CallManager::onCallParticipantConnectionDown()
{
    LOG_WARN("Call participant connection down");
    
    if (m_coreClient && m_coreClient->isScreenSharing()) {
        m_coreClient->stopScreenSharing();
    }

    if (m_coreClient && m_coreClient->isCameraSharing()) {
        m_coreClient->stopCameraSharing();
    }

    if (m_callWidget) {
        if (m_callWidget->isFullScreen()) {
            m_callWidget->exitFullscreen();
        }
        m_callWidget->hideEnterFullscreenButton();
        m_callWidget->setCameraButtonRestricted(true);
        m_callWidget->setScreenShareButtonRestricted(true);
        m_callWidget->hideMainScreen();
        m_callWidget->hideAdditionalScreens();
    }

    if (m_notificationController)
    {
        m_notificationController->showConnectionDownWithUser("Connection lost. Waiting for participant... (2 min)");
    }
}

void CallManager::onCallParticipantConnectionRestored()
{
    if (!m_coreClient || !m_coreClient->isActiveCall())
    {
        hideOperationDialog();
        return;
    }

    if (m_callWidget) {
        m_callWidget->setCameraButtonRestricted(false);
        m_callWidget->setScreenShareButtonRestricted(false);
        m_callWidget->hideMainScreen();
        m_callWidget->hideAdditionalScreens();
        m_callWidget->hideEnterFullscreenButton();
    }

    if (m_notificationController)
    {
        m_notificationController->hideConnectionDownWithUser();
        
        const int restoredDurationMs = ERROR_MESSAGE_DURATION_MS;
        m_notificationController->showConnectionRestoredWithUser("Connection with participant restored", restoredDurationMs);
    }
}

void CallManager::onLocalConnectionDownInCall()
{
    if (!m_callWidget) return;

    if (m_callWidget->isFullScreen()) {
        emit endCallFullscreenExitRequested();
    }
    m_callWidget->hideEnterFullscreenButton();
    m_callWidget->setCameraButtonActive(false);
    m_callWidget->setScreenShareButtonActive(false);
    m_callWidget->hideMainScreen();
    m_callWidget->hideAdditionalScreens();
    m_callWidget->hideEnterFullscreenButton();
}

void CallManager::onLocalConnectionDown()
{
    hideOperationDialog();

    if (m_mainMenuWidget) {
        m_mainMenuWidget->removeCallingPanel();
        m_mainMenuWidget->setStatusLabelOnline();
        m_mainMenuWidget->setCallButtonEnabled(true);
    }

    m_incomingCalls.clear();
    updateIncomingCallsUi();

    if (m_audioManager) {
        m_audioManager->stopIncomingCallRingtone();
        m_audioManager->stopCallingRingtone();
    }
}

void CallManager::hideOperationDialog()
{
    if (m_notificationController) {
        m_notificationController->hideConnectionDownWithUser();
    }
}

void CallManager::resetCallWidgetState()
{
    if (!m_callWidget) return;

    if (m_callWidget->isFullScreen()) {
        m_callWidget->exitFullscreen();
    }
    m_callWidget->hideEnterFullscreenButton();
    m_callWidget->setCameraButtonActive(false);
    m_callWidget->setScreenShareButtonActive(false);
    m_callWidget->hideMainScreen();
    m_callWidget->hideAdditionalScreens();
}

void CallManager::onRemoteUserEndedCall()
{
    if (!m_mainMenuWidget) return;

    m_mainMenuWidget->removeCallingPanel();
    m_mainMenuWidget->setStatusLabelOnline();
    
    if (m_audioManager) {
        m_audioManager->stopCallingRingtone();
        m_audioManager->stopIncomingCallRingtone();
        m_audioManager->playEndCallEffect();
    }

    if (m_callWidget && m_callWidget->isFullScreen()) {
        emit endCallFullscreenExitRequested();
    }

    if (m_callWidget) {
        resetCallWidgetState();
        m_callWidget->hide();
    }

    if (m_navigationController) {
        m_navigationController->switchToMainMenuWidget();
    }

    if (m_updateManager) {
        m_updateManager->showUpdateAvailableDialogIfNeeded();
    }

    if (m_coreClient && m_coreClient->isScreenSharing()) {
        m_coreClient->stopScreenSharing();
    }

    if (m_coreClient && m_coreClient->isCameraSharing()) {
        m_coreClient->stopCameraSharing();
    }
}

// --- Media frame slots ---

void CallManager::onLocalScreenFrame(QByteArray data, int width, int height)
{
    if (!m_callWidget || width <= 0 || height <= 0) return;
    if (data.size() < width * height * 3) return;
    if (!m_coreClient || !m_coreClient->isScreenSharing()) {
        m_callWidget->hideMainScreen();
        return;
    }

    QImage image(reinterpret_cast<const uchar*>(data.constData()), width, height, width * 3, QImage::Format_RGB888);
    QPixmap pixmap = QPixmap::fromImage(image.copy());
    m_callWidget->showFrameInMainScreen(pixmap, Screen::ScaleMode::KeepAspectRatio);
}

void CallManager::onLocalCameraFrame(QByteArray data, int width, int height)
{
    if (!m_callWidget || width <= 0 || height <= 0) return;
    if (data.size() < width * height * 3) return;
    if (!m_coreClient || !m_coreClient->isCameraSharing()) {
        m_callWidget->removeAdditionalScreen(ADDITIONAL_SCREEN_ID_LOCAL_CAMERA);
        const bool screenSharingActive = m_coreClient && (m_coreClient->isScreenSharing() || m_coreClient->isViewingRemoteScreen());
        if (!screenSharingActive) {
            m_callWidget->hideMainScreen();
        }
        return;
    }

    QImage image(reinterpret_cast<const uchar*>(data.constData()), width, height, width * 3, QImage::Format_RGB888);
    QPixmap pixmap = QPixmap::fromImage(image.copy());

    const bool screenSharingActive = m_coreClient && (m_coreClient->isScreenSharing() || m_coreClient->isViewingRemoteScreen());
    const bool bothCameras = m_coreClient && m_coreClient->isCameraSharing() && m_coreClient->isViewingRemoteCamera();
    if (screenSharingActive || bothCameras) {
        m_callWidget->showFrameInAdditionalScreen(pixmap, ADDITIONAL_SCREEN_ID_LOCAL_CAMERA);
    } else {
        // Ensure local camera is not duplicated in additionalScreens when it moves to mainScreen.
        m_callWidget->removeAdditionalScreen(ADDITIONAL_SCREEN_ID_LOCAL_CAMERA);
        m_callWidget->showFrameInMainScreen(pixmap, Screen::ScaleMode::KeepAspectRatio);
    }
}

void CallManager::onIncomingScreenFrame(QByteArray data, int width, int height)
{
    if (!m_callWidget || width <= 0 || height <= 0) return;
    if (data.size() < width * height * 3) return;
    if (!m_coreClient || !m_coreClient->isViewingRemoteScreen()) {
        m_callWidget->hideMainScreen();
        return;
    }

    QImage image(reinterpret_cast<const uchar*>(data.constData()), width, height, width * 3, QImage::Format_RGB888);
    QPixmap pixmap = QPixmap::fromImage(image.copy());
    m_callWidget->showFrameInMainScreen(pixmap, Screen::ScaleMode::KeepAspectRatio);
}

void CallManager::onIncomingCameraFrame(QByteArray data, int width, int height)
{
    if (!m_callWidget || width <= 0 || height <= 0) return;
    if (data.size() < width * height * 3) return;
    if (!m_coreClient || !m_coreClient->isViewingRemoteCamera()) {
        m_callWidget->removeAdditionalScreen(ADDITIONAL_SCREEN_ID_REMOTE_CAMERA);
        const bool screenSharingActive = m_coreClient && (m_coreClient->isScreenSharing() || m_coreClient->isViewingRemoteScreen());
        if (!screenSharingActive) {
            m_callWidget->hideMainScreen();
        }
        return;
    }

    QImage image(reinterpret_cast<const uchar*>(data.constData()), width, height, width * 3, QImage::Format_RGB888);
    QPixmap pixmap = QPixmap::fromImage(image.copy());

    const bool screenSharingActive = m_coreClient && (m_coreClient->isScreenSharing() || m_coreClient->isViewingRemoteScreen());
    if (screenSharingActive) {
        m_callWidget->showFrameInAdditionalScreen(pixmap, ADDITIONAL_SCREEN_ID_REMOTE_CAMERA);
    } else {
        // Ensure remote camera is not duplicated in additionalScreens when it moves to mainScreen.
        m_callWidget->removeAdditionalScreen(ADDITIONAL_SCREEN_ID_REMOTE_CAMERA);
        m_callWidget->showFrameInMainScreen(pixmap, Screen::ScaleMode::KeepAspectRatio);
    }
}

// --- Media state slots ---

void CallManager::onIncomingScreenSharingStarted()
{
    if (m_callWidget) {
        m_callWidget->setScreenShareButtonRestricted(true);
        m_callWidget->showEnterFullscreenButton();
    }
}

void CallManager::onIncomingScreenSharingStopped()
{
    if (m_callWidget) {
        m_callWidget->hideMainScreen();
        m_callWidget->setScreenShareButtonRestricted(false);
        m_callWidget->hideEnterFullscreenButton();
        if (m_callWidget->isFullScreen()) {
            emit endCallFullscreenExitRequested();
        }
    }
}

void CallManager::onIncomingCameraSharingStarted()
{
    // Remote camera frames will arrive via onIncomingCameraFrame
}

void CallManager::onIncomingCameraSharingStopped()
{
    if (!m_callWidget) return;
    m_callWidget->removeAdditionalScreen(ADDITIONAL_SCREEN_ID_REMOTE_CAMERA);
    const bool screenSharingActive = m_coreClient && (m_coreClient->isScreenSharing() || m_coreClient->isViewingRemoteScreen());
    if (!screenSharingActive) {
        m_callWidget->hideMainScreen();
    }
}

void CallManager::onStartScreenSharingError()
{
    if (m_callWidget) {
        m_callWidget->setScreenShareButtonActive(false);
    }

    if (m_notificationController) {
        m_notificationController->showErrorNotification("Failed to start screen sharing", 1500);
    }
}

void CallManager::onStartCameraSharingError()
{
    if (m_callWidget) {
        m_callWidget->setCameraButtonActive(false);
    }

    if (m_notificationController) {
        m_notificationController->showErrorNotification("Failed to start camera sharing", 1500);
    }
}

void CallManager::onIncomingCallsDialogClosed(const QList<QString>& pendingCalls)
{
    for (const QString& nickname : pendingCalls)
    {
        onDeclineCallButtonClicked(nickname);
        m_incomingCalls.remove(nickname);
    }

    updateIncomingCallsUi();

    if (m_audioManager && m_incomingCalls.isEmpty())
    {
        m_audioManager->stopIncomingCallRingtone();
    }
}

void CallManager::updateIncomingCallsUi()
{
    if (!m_dialogsController)
    {
        return;
    }

    QSet<QString> current;
    for (const auto& entry : m_incomingCalls)
    {
        current.insert(entry.nickname);
        m_dialogsController->showIncomingCallsDialog(entry.nickname, entry.remainingTime);
    }

    for (const QString& nickname : m_visibleIncomingCalls)
    {
        if (!current.contains(nickname))
        {
            m_dialogsController->hideIncomingCallsDialog(nickname);
        }
    }

    m_visibleIncomingCalls = current;
}
