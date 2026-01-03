#include "callManager.h"
#include "audioEffectsManager.h"
#include "navigationController.h"
#include "mainMenuWidget.h"
#include "callWidget.h"
#include "screenCaptureController.h"
#include "CameraCaptureController.h"
#include "client.h"
#include "logger.h"

CallManager::CallManager(callifornia::Client* client, AudioEffectsManager* audioManager, NavigationController* navigationController, QObject* parent)
    : QObject(parent)
    , m_client(client)
    , m_audioManager(audioManager)
    , m_navigationController(navigationController)
{
}

void CallManager::setWidgets(MainMenuWidget* mainMenuWidget, CallWidget* callWidget, QStackedLayout* stackedLayout)
{
    m_mainMenuWidget = mainMenuWidget;
    m_callWidget = callWidget;
    m_stackedLayout = stackedLayout;
}

void CallManager::setScreenCaptureController(ScreenCaptureController* controller)
{
    m_screenCaptureController = controller;
}

void CallManager::setCameraCaptureController(CameraCaptureController* controller)
{
    m_cameraCaptureController = controller;
}

void CallManager::setNavigationController(NavigationController* navigationController)
{
    m_navigationController = navigationController;
}

void CallManager::onStartCallingButtonClicked(const QString& friendNickname)
{
    if (!m_mainMenuWidget) return;

    if (!m_client) return;
    if (!friendNickname.isEmpty() && friendNickname.toStdString() != m_client->getMyNickname()) {
        bool requestSent = m_client->startOutgoingCall(friendNickname.toStdString());
        if (!requestSent) {
            handleStartCallingErrorNotificationAppearance();
            return;
        }
    }
    else {
        if (friendNickname.isEmpty()) {
            m_mainMenuWidget->setErrorMessage("cannot be empty");
        }
        else if (friendNickname.toStdString() == m_client->getMyNickname()) {
            m_mainMenuWidget->setErrorMessage("cannot call yourself");
        }
    }
}

void CallManager::onStopCallingButtonClicked()
{
    if (!m_mainMenuWidget) return;

    if (!m_client) return;
    bool requestSent = m_client->stopOutgoingCall();
    if (!requestSent) {
        handleStopCallingErrorNotificationAppearance();
    }
    else {
        m_mainMenuWidget->removeCallingPanel();
        m_mainMenuWidget->setState(callifornia::State::FREE);
        if (m_audioManager) {
            m_audioManager->stopCallingRingtone();
        }
        if (m_audioManager) {
            m_audioManager->playSoundEffect(":/resources/callingEnded.wav");
        }
    }
}

void CallManager::onAcceptCallButtonClicked(const QString& friendNickname)
{
    if (m_screenCaptureController && m_screenCaptureController->isCapturing()) {
        emit stopScreenCaptureRequested();
    }

    if (m_cameraCaptureController && m_cameraCaptureController->isCapturing()) {
        emit stopCameraCaptureRequested();
    }

    if (!m_client) return;
    bool requestSent = m_client->acceptCall(friendNickname.toStdString());
    if (!requestSent) {
        handleAcceptCallErrorNotificationAppearance();
        return;
    }
}

void CallManager::onDeclineCallButtonClicked(const QString& friendNickname)
{
    if (!m_client) return;
    bool requestSent = m_client->declineCall(friendNickname.toStdString());
    if (!requestSent) {
        handleDeclineCallErrorNotificationAppearance();
    }
    else {
        if (m_client->getIncomingCallsCount() == 0 && m_audioManager) {
            m_audioManager->stopIncomingCallRingtone();
        }

        if (m_stackedLayout && m_callWidget && m_stackedLayout->currentWidget() == m_callWidget) {
            m_callWidget->removeIncomingCall(friendNickname);
        }

        if (m_mainMenuWidget) {
            m_mainMenuWidget->removeIncomingCall(friendNickname);
        }

        if (m_audioManager) {
            m_audioManager->playSoundEffect(":/resources/callingEnded.wav");
        }
    }
}

void CallManager::onEndCallButtonClicked()
{
    if (!m_client) return;
    bool requestSent = m_client->endCall();
    if (!requestSent) {
        handleEndCallErrorNotificationAppearance();
        return;
    }

    emit stopScreenCaptureRequested();
    emit stopCameraCaptureRequested();

    if (m_mainMenuWidget) {
        m_mainMenuWidget->setState(callifornia::State::FREE);
    }

    if (m_navigationController) {
        m_navigationController->switchToMainMenuWidget();
    }

    if (m_audioManager) {
        m_audioManager->playSoundEffect(":/resources/endCall.wav");
    }
}

void CallManager::onStartCallingResult(callifornia::ErrorCode ec)
{
    if (!m_mainMenuWidget || !m_client) return;

    QString errorMessage;

    // Success case - no error code means success
    if (ec == callifornia::ErrorCode::network_error || ec == callifornia::ErrorCode::taken_nickname) {
        // Error cases
    } else {
        // Success
        LOG_INFO("Started calling user: {}", m_client->getNicknameWhomCalling());
        m_mainMenuWidget->showCallingPanel(QString::fromStdString(m_client->getNicknameWhomCalling()));
        m_mainMenuWidget->setState(callifornia::State::CALLING);
        if (m_audioManager) {
            m_audioManager->playCallingRingtone();
        }
        return;
    }
    if (ec == callifornia::ErrorCode::unexisting_user) {
        errorMessage = "User not found";
        LOG_WARN("Start calling failed: user not found");
    }
    else {
        errorMessage = "Timeout (please try again)";
        LOG_ERROR("Start calling failed: timeout or network error");
    }

    m_mainMenuWidget->setErrorMessage(errorMessage);
}

void CallManager::onAcceptCallResult(callifornia::ErrorCode ec, const QString& nickname)
{
    if (!m_mainMenuWidget || !m_client) return;

    // Success case - no error code means success
    if (ec == callifornia::ErrorCode::network_error || ec == callifornia::ErrorCode::taken_nickname) {
        // Error cases
        if (m_mainMenuWidget) {
            m_mainMenuWidget->removeIncomingCall(nickname);
        }

        if (m_stackedLayout && m_callWidget && m_stackedLayout->currentWidget() == m_callWidget) {
            m_callWidget->removeIncomingCall(nickname);
        }

        handleAcceptCallErrorNotificationAppearance();
    } else {
        // Success
        LOG_INFO("Call accepted successfully with: {}", nickname.toStdString());
        m_mainMenuWidget->removeCallingPanel();
        m_mainMenuWidget->clearIncomingCalls();

        if (m_stackedLayout && m_callWidget && m_stackedLayout->currentWidget() == m_callWidget) {
            m_callWidget->clearIncomingCalls();
        }

        if (m_audioManager) {
            m_audioManager->playSoundEffect(":/resources/callJoined.wav");
        }

        if (m_navigationController) {
            m_navigationController->switchToCallWidget(nickname);
        }

        if (m_audioManager) {
            m_audioManager->stopIncomingCallRingtone();
        }
    }
}

void CallManager::onMaximumCallingTimeReached()
{
    if (!m_mainMenuWidget) return;

    m_mainMenuWidget->removeCallingPanel();
    m_mainMenuWidget->setState(callifornia::State::FREE);
    if (m_audioManager) {
        m_audioManager->stopCallingRingtone();
    }
}

void CallManager::onCallingAccepted()
{
    if (!m_mainMenuWidget || !m_client) return;

    LOG_INFO("Outgoing call accepted by user: {}", m_client->getNicknameInCallWith());
    m_mainMenuWidget->clearIncomingCalls();

    if (m_audioManager) {
        m_audioManager->stopCallingRingtone();
    }

    m_mainMenuWidget->removeCallingPanel();
    m_mainMenuWidget->setState(callifornia::State::BUSY);

    if (m_navigationController) {
        m_navigationController->switchToCallWidget(QString::fromStdString(m_client->getNicknameInCallWith()));
    }

    if (m_audioManager) {
        m_audioManager->playSoundEffect(":/resources/callJoined.wav");
    }
}

void CallManager::onCallingDeclined()
{
    if (!m_mainMenuWidget) return;

    LOG_INFO("Outgoing call was declined");
    m_mainMenuWidget->removeCallingPanel();
    m_mainMenuWidget->setState(callifornia::State::FREE);

    if (m_audioManager) {
        m_audioManager->stopCallingRingtone();
    }

    if (m_audioManager) {
        m_audioManager->playSoundEffect(":/resources/callingEnded.wav");
    }
}

void CallManager::onRemoteUserEndedCall()
{
    if (m_screenCaptureController && m_screenCaptureController->isCapturing()) {
        emit stopScreenCaptureRequested();
    }

    if (m_cameraCaptureController && m_cameraCaptureController->isCapturing()) {
        emit stopCameraCaptureRequested();
    }

    LOG_INFO("Remote user ended the call");

    if (m_callWidget && m_callWidget->isFullScreen()) {
        emit endCallFullscreenExitRequested();
    }

    if (m_navigationController) {
        m_navigationController->switchToMainMenuWidget();
    }

    if (m_mainMenuWidget) {
        m_mainMenuWidget->setState(callifornia::State::FREE);
    }

    if (m_audioManager) {
        m_audioManager->playSoundEffect(":/resources/endCall.wav");
    }
}

void CallManager::onIncomingCall(const QString& friendNickname)
{
    if (!m_mainMenuWidget) return;

    LOG_INFO("Incoming call from: {}", friendNickname.toStdString());

    if (m_audioManager) {
        m_audioManager->playIncomingCallRingtone();
    }

    m_mainMenuWidget->addIncomingCall(friendNickname);

    if (m_stackedLayout && m_callWidget && m_stackedLayout->currentWidget() == m_callWidget) {
        m_callWidget->addIncomingCall(friendNickname);
    }
}

void CallManager::onIncomingCallExpired(const QString& friendNickname)
{
    if (!m_mainMenuWidget) return;

    LOG_INFO("Incoming call from {} expired", friendNickname.toStdString());
    m_mainMenuWidget->removeIncomingCall(friendNickname);

    if (m_stackedLayout && m_callWidget && m_stackedLayout->currentWidget() == m_callWidget) {
        m_callWidget->removeIncomingCall(friendNickname);
    }

    if (m_client && m_client->getIncomingCallsCount() == 0 && m_audioManager) {
        m_audioManager->stopIncomingCallRingtone();
    }

    if (m_audioManager) {
        m_audioManager->playSoundEffect(":/resources/callingEnded.wav");
    }
}

void CallManager::handleAcceptCallErrorNotificationAppearance()
{
    QString errorText = "Failed to accept call";

    if (!m_stackedLayout) return;

    if (m_stackedLayout->currentWidget() == m_mainMenuWidget && m_mainMenuWidget) {
        m_mainMenuWidget->showErrorNotification(errorText, 1500);
    }
    else if (m_stackedLayout->currentWidget() == m_callWidget && m_callWidget) {
        m_callWidget->showErrorNotification(errorText, 1500);
    }
    else {
        LOG_WARN("Trying to accept call from unexpected widget");
    }
}

void CallManager::handleDeclineCallErrorNotificationAppearance()
{
    QString errorText = "Failed to decline call. Please try again";

    if (!m_stackedLayout) return;

    if (m_stackedLayout->currentWidget() == m_mainMenuWidget && m_mainMenuWidget) {
        m_mainMenuWidget->showErrorNotification(errorText, 1500);
    }
    else if (m_stackedLayout->currentWidget() == m_callWidget && m_callWidget) {
        m_callWidget->showErrorNotification(errorText, 1500);
    }
    else {
        LOG_WARN("Trying to decline call from unexpected widget");
    }
}

void CallManager::handleStartCallingErrorNotificationAppearance()
{
    QString errorText = "Failed to start calling. Please try again";

    if (m_mainMenuWidget) {
        m_mainMenuWidget->showErrorNotification(errorText, 1500);
    }
    else {
        LOG_WARN("Trying to start calling from unexpected widget");
    }
}

void CallManager::handleStopCallingErrorNotificationAppearance()
{
    QString errorText = "Failed to stop calling. Please try again";

    if (m_mainMenuWidget) {
        m_mainMenuWidget->showErrorNotification(errorText, 1500);
    }
    else {
        LOG_WARN("Trying to stop calling from unexpected widget");
    }
}

void CallManager::handleEndCallErrorNotificationAppearance()
{
    QString errorText = "Failed to end call. Please try again";

    if (m_callWidget) {
        m_callWidget->showErrorNotification(errorText, 1500);
    }
    else {
        LOG_WARN("Trying to end call from unexpected widget");
    }
}
