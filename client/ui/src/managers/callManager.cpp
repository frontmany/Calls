#include "callManager.h"
#include "audio/audioEffectsManager.h"
#include "managers/navigationController.h"
#include "managers/dialogsController.h"
#include "managers/notificationController.h"
#include "managers/updateManager.h"
#include "widgets/mainMenuWidget.h"
#include "widgets/callWidget.h"
#include "utilities/logger.h"
#include "utilities/constant.h"
#include "errorCode.h"
#include <system_error>
#include <QList>

CallManager::CallManager(std::shared_ptr<core::Client> client, AudioEffectsManager* audioManager, NavigationController* navigationController, DialogsController* dialogsController, UpdateManager* updateManager, QObject* parent)
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
    if (ec) {
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
            m_mainMenuWidget->setStopCallingButtonEnabled(false);
        }
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
        if (m_dialogsController) {
            m_dialogsController->setIncomingCallButtonsActive(friendNickname, false);
        }
        if (m_coreClient && m_coreClient->isScreenSharing()) {
            m_coreClient->stopScreenSharing();
        }

        if (m_coreClient && m_coreClient->isCameraSharing()) {
            m_coreClient->stopCameraSharing();
        }

        m_mainMenuWidget->removeCallingPanel();
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
        if (m_dialogsController) {
            m_dialogsController->hideIncomingCallsDialog(friendNickname);
        }
        updateIncomingCallsUi();

        if (m_audioManager) {
            m_audioManager->playCallingEndedEffect();
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
        if (m_callWidget && m_callWidget->isFullScreen()) {
            emit endCallFullscreenExitRequested();
        }

        if (m_navigationController) {
            m_navigationController->switchToMainMenuWidget();
        }

        if (m_callWidget) {
            m_callWidget->hide();
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
        if (ec == core::make_error_code(core::ErrorCode::network_error) || ec == core::make_error_code(core::ErrorCode::unexisting_user)) {
            QString errorMessage;
            if (ec == core::make_error_code(core::ErrorCode::unexisting_user)) {
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
void CallManager::onCallParticipantConnectionDown()
{
    LOG_WARN("Call participant connection down");
    
    if (m_screenCaptureController && m_screenCaptureController->isCapturing()) {
        m_coreClient->stopScreenSharing();
    }

    if (m_cameraCaptureController && m_cameraCaptureController->isCapturing()) {
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
        m_callWidget->exitFullscreen();
    }
    m_callWidget->hideEnterFullscreenButton();
    m_callWidget->setCameraButtonActive(false);
    m_callWidget->setScreenShareButtonActive(false);
    m_callWidget->hideMainScreen();
    m_callWidget->hideAdditionalScreens();
    m_callWidget->hideEnterFullscreenButton();
}

void CallManager::hideOperationDialog()
{
    if (m_notificationController) {
        m_notificationController->hideConnectionDownWithUser();
    }
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

    if (m_callWidget) {
        m_callWidget->hide();
    }

    if (m_navigationController) {
        m_navigationController->switchToMainMenuWidget();
    }

    if (m_screenCaptureController && m_screenCaptureController->isCapturing()) {
        m_coreClient->stopScreenSharing();
    }

    if (m_cameraCaptureController && m_cameraCaptureController->isCapturing()) {
        m_coreClient->stopCameraSharing();
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
