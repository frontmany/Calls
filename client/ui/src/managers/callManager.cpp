#include "callManager.h"
#include "media/audioEffectsManager.h"
#include "managers/navigationController.h"
#include "managers/dialogsController.h"
#include "widgets/mainMenuWidget.h"
#include "widgets/callWidget.h"
#include "media/screenCaptureController.h"
#include "media/cameraCaptureController.h"
#include "utilities/logger.h"
#include "errorCode.h"
#include <system_error>

CallManager::CallManager(std::shared_ptr<core::Client> client, AudioEffectsManager* audioManager, NavigationController* navigationController, ScreenCaptureController* screenCaptureController, CameraCaptureController* cameraCaptureController, DialogsController* dialogsController, QObject* parent)
    : QObject(parent)
    , m_coreClient(client)
    , m_audioManager(audioManager)
    , m_navigationController(navigationController)
    , m_screenCaptureController(screenCaptureController)
    , m_cameraCaptureController(cameraCaptureController)
    , m_dialogsController(dialogsController)
    , m_operationTimer(new QTimer(this))
{
    m_operationTimer->setSingleShot(true);
    m_operationTimer->setInterval(1000);
    connect(m_operationTimer, &QTimer::timeout, this, &CallManager::onTimeToShowWaitingNotification);
}

void CallManager::setWidgets(MainMenuWidget* mainMenuWidget, CallWidget* callWidget, QStackedLayout* stackedLayout)
{
    m_mainMenuWidget = mainMenuWidget;
    m_callWidget = callWidget;
    m_stackedLayout = stackedLayout;
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
        startOperationTimer("Calling begins...");
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
        startOperationTimer("Stopping call...");
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
        if (m_mainMenuWidget) {
            m_mainMenuWidget->setIncomingCallButtonsEnabled(friendNickname, false);
        }
        if (m_callWidget) {
            m_callWidget->setIncomingCallButtonsEnabled(friendNickname, false);
        }
        startOperationTimer("Accepting call...");
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
        if (m_mainMenuWidget) {
            m_mainMenuWidget->setIncomingCallButtonsEnabled(friendNickname, false);
        }
        if (m_callWidget) {
            m_callWidget->setIncomingCallButtonsEnabled(friendNickname, false);
        }
        startOperationTimer("Declining call...");
    }
}

void CallManager::onEndCallButtonClicked()
{
    if (!m_coreClient) return;

    std::error_code ec = m_coreClient->endCall();
    if (ec) {
        if (!m_coreClient->isConnectionDown()) {
            handleEndCallErrorNotificationAppearance();
        }
    }
    else {
        if (m_callWidget) {
            m_callWidget->setHangupButtonEnabled(false);
        }
        startOperationTimer("Ending call...");
    }
}

void CallManager::onStartCallingResult(std::error_code ec)
{
    stopOperationTimer();
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

void CallManager::onAcceptCallResult(std::error_code ec, const QString& nickname)
{
    stopOperationTimer();
    if (!m_mainMenuWidget || !m_coreClient) return;

    if (!ec) {
        if (m_screenCaptureController && m_screenCaptureController->isCapturing()) {
            emit stopScreenCaptureRequested();
        }

        if (m_cameraCaptureController && m_cameraCaptureController->isCapturing()) {
            emit stopCameraCaptureRequested();
        }

        m_mainMenuWidget->removeCallingPanel();
        m_mainMenuWidget->clearIncomingCalls();

        if (m_stackedLayout && m_callWidget && m_stackedLayout->currentWidget() == m_callWidget) {
            m_callWidget->clearIncomingCalls();
        }

        if (m_audioManager) {
            m_audioManager->playCallJoinedEffect();
        }

        if (m_navigationController) {
            m_navigationController->switchToCallWidget(nickname);
        }

        if (m_audioManager) {
            m_audioManager->stopIncomingCallRingtone();
        }
    }
    else {
        if (m_mainMenuWidget) {
            m_mainMenuWidget->setIncomingCallButtonsEnabled(nickname, true);
        }
        if (m_callWidget) {
            m_callWidget->setIncomingCallButtonsEnabled(nickname, true);
        }
        if (ec == core::make_error_code(core::ErrorCode::network_error) || ec == core::make_error_code(core::ErrorCode::taken_nickname)) {
            if (m_mainMenuWidget) {
                m_mainMenuWidget->removeIncomingCall(nickname);
            }

            if (m_stackedLayout && m_callWidget && m_stackedLayout->currentWidget() == m_callWidget) {
                m_callWidget->removeIncomingCall(nickname);
            }

            if (!m_coreClient->isConnectionDown()) {
                handleAcceptCallErrorNotificationAppearance();
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

    m_mainMenuWidget->clearIncomingCalls();

    if (m_audioManager) {
        m_audioManager->stopCallingRingtone();
    }

    m_mainMenuWidget->removeCallingPanel();
    m_mainMenuWidget->setStatusLabelBusy();

    if (m_navigationController) {
        m_navigationController->switchToCallWidget(QString::fromStdString(m_coreClient->getNicknameInCallWith()));
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

void CallManager::onRemoteUserEndedCall()
{
    if (m_screenCaptureController && m_screenCaptureController->isCapturing()) {
        emit stopScreenCaptureRequested();
    }

    if (m_cameraCaptureController && m_cameraCaptureController->isCapturing()) {
        emit stopCameraCaptureRequested();
    }


    if (m_callWidget && m_callWidget->isFullScreen()) {
        emit endCallFullscreenExitRequested();
    }

    if (m_navigationController) {
        m_navigationController->switchToMainMenuWidget();
    }

    if (m_mainMenuWidget) {
        m_mainMenuWidget->setStatusLabelOnline();
    }

    if (m_audioManager) {
        m_audioManager->playEndCallEffect();
    }
}

void CallManager::onIncomingCall(const QString& friendNickname)
{
    if (!m_mainMenuWidget) return;


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

    m_mainMenuWidget->removeIncomingCall(friendNickname);

    if (m_stackedLayout && m_callWidget && m_stackedLayout->currentWidget() == m_callWidget) {
        m_callWidget->removeIncomingCall(friendNickname);
    }

    if (m_coreClient && m_coreClient->getIncomingCallsCount() == 0 && m_audioManager) {
        m_audioManager->stopIncomingCallRingtone();
    }

    if (m_audioManager) {
        m_audioManager->playCallingEndedEffect();
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

void CallManager::onStopOutgoingCallResult(std::error_code ec)
{
    stopOperationTimer();
    if (m_mainMenuWidget) {
        m_mainMenuWidget->setStopCallingButtonEnabled(true);
    }
    if (ec) {
        LOG_WARN("Failed to stop outgoing call: {}", ec.message());
        if (m_coreClient && !m_coreClient->isConnectionDown()) {
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
            m_audioManager->playCallingEndedEffect();
        }
    }
}

void CallManager::onDeclineCallResult(std::error_code ec, const QString& nickname)
{
    stopOperationTimer();
    if (ec) {
        if (m_mainMenuWidget) {
            m_mainMenuWidget->setIncomingCallButtonsEnabled(nickname, true);
        }
        if (m_callWidget) {
            m_callWidget->setIncomingCallButtonsEnabled(nickname, true);
        }
        LOG_WARN("Failed to decline call: {}", ec.message());
        if (m_coreClient && !m_coreClient->isConnectionDown()) {
            handleDeclineCallErrorNotificationAppearance();
        }
    }
    else {
        if (m_stackedLayout && m_callWidget && m_stackedLayout->currentWidget() == m_callWidget) {
            m_callWidget->removeIncomingCall(nickname);
        }

        if (m_coreClient && m_coreClient->getIncomingCallsCount() == 0 && m_audioManager) {
            m_audioManager->stopIncomingCallRingtone();
        }

        if (m_mainMenuWidget) {
            m_mainMenuWidget->removeIncomingCall(nickname);
        }

        if (m_audioManager) {
            m_audioManager->playCallingEndedEffect();
        }
    }
}

void CallManager::onEndCallResult(std::error_code ec)
{
    stopOperationTimer();
    if (m_callWidget) {
        m_callWidget->setHangupButtonEnabled(true);
        m_callWidget->setCameraButtonActive(true);
        m_callWidget->setCameraButtonActive(true);
    }
    if (ec) {
        LOG_WARN("Failed to end call: {}", ec.message());
        if (m_coreClient && !m_coreClient->isConnectionDown()) {
            handleEndCallErrorNotificationAppearance();
        }
    }
    else {
        emit stopScreenCaptureRequested();
        emit stopCameraCaptureRequested();

        if (m_mainMenuWidget) {
            m_mainMenuWidget->setStatusLabelOnline();
        }

        if (m_navigationController) {
            m_navigationController->switchToMainMenuWidget();
        }

        if (m_audioManager) {
            m_audioManager->playEndCallEffect();
        }
    }
}

void CallManager::onCallParticipantConnectionDown()
{
    LOG_WARN("Call participant connection down");
    
    stopOperationTimer();
    
    if (m_screenCaptureController && m_screenCaptureController->isCapturing()) {
        emit stopScreenCaptureRequested();
    }

    if (m_cameraCaptureController && m_cameraCaptureController->isCapturing()) {
        emit stopCameraCaptureRequested();
    }

    if (m_callWidget) {
        m_callWidget->restrictCameraButton();
        m_callWidget->restrictScreenShareButton();
    }

    if (m_dialogsController)
    {
        m_dialogsController->showNotificationDialog("Connection with participant lost. Waiting for them...", false, false, true);
    }
}

void CallManager::onCallParticipantConnectionRestored()
{
    if (m_dialogsController)
    {
        const int restoredDurationMs = 2500;
        m_dialogsController->hideNotificationDialog();
        m_dialogsController->showNotificationDialog("Connection restored", false, true, false);

        QTimer::singleShot(restoredDurationMs, this, [this]()
        {
            if (m_callWidget) {

                // todo
                /*
                m_callWidget->restrictCameraButton();
                m_callWidget->restrictScreenShareButton();
                */
                // abort this actions 
            }

            if (m_dialogsController)
            {
                m_dialogsController->hideNotificationDialog();
            }
        });
    }
}

void CallManager::startOperationTimer(const QString& dialogText)
{
    m_pendingOperationDialogText = dialogText;
    m_operationTimer->start();
}

void CallManager::stopOperationTimer()
{
    m_operationTimer->stop();
    m_pendingOperationDialogText.clear();
    if (m_dialogsController) {
        m_dialogsController->hideNotificationDialog();
    }
}

void CallManager::hideOperationDialog()
{
    stopOperationTimer();
}

void CallManager::onTimeToShowWaitingNotification()
{
    if (m_coreClient && !m_coreClient->isConnectionDown() && !m_pendingOperationDialogText.isEmpty()) {
        if (m_dialogsController) {

            m_dialogsController->showNotificationDialog(m_pendingOperationDialogText, false, false);
        }
    }
}