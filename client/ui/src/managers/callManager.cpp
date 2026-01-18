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
#include <QList>

namespace
{
    constexpr int kDefaultIncomingCallSeconds = 32;
}

CallManager::CallManager(std::shared_ptr<core::Client> client, AudioEffectsManager* audioManager, NavigationController* navigationController, ScreenCaptureController* screenCaptureController, CameraCaptureController* cameraCaptureController, DialogsController* dialogsController, QObject* parent)
    : QObject(parent)
    , m_coreClient(client)
    , m_audioManager(audioManager)
    , m_navigationController(navigationController)
    , m_screenCaptureController(screenCaptureController)
    , m_cameraCaptureController(cameraCaptureController)
    , m_dialogsController(dialogsController)
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
        startOperationTimer(core::UserOperationType::START_OUTGOING_CALL, "Calling begins...");
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
        startOperationTimer(core::UserOperationType::STOP_OUTGOING_CALL, "Stopping call...");
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
        startOperationTimer(core::UserOperationType::ACCEPT_CALL, "Accepting call...");
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
            m_dialogsController->setIncomingCallButtonsActive(friendNickname, false);
        }
        startOperationTimer(core::UserOperationType::DECLINE_CALL, "Declining call...");
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
            m_callWidget->setHangupButtonRestricted(true);
        }
        startOperationTimer(core::UserOperationType::END_CALL, "Ending call...");
    }
}

void CallManager::onStartCallingResult(std::error_code ec)
{
    stopOperationTimer(core::UserOperationType::START_OUTGOING_CALL);
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
    stopOperationTimer(core::UserOperationType::ACCEPT_CALL);
    if (!m_mainMenuWidget || !m_coreClient) return;

    if (!ec) {
        if (m_screenCaptureController && m_screenCaptureController->isCapturing()) {
            emit stopScreenCaptureRequested();
        }

        if (m_cameraCaptureController && m_cameraCaptureController->isCapturing()) {
            emit stopCameraCaptureRequested();
        }

        m_mainMenuWidget->removeCallingPanel();
        m_incomingCalls.clear();
        updateIncomingCallsUi();

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
        if (m_dialogsController) {
            m_dialogsController->setIncomingCallButtonsActive(nickname, true);
        }
        if (ec == core::make_error_code(core::ErrorCode::network_error) || ec == core::make_error_code(core::ErrorCode::taken_nickname)) {
            m_incomingCalls.remove(nickname);
            updateIncomingCallsUi();

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
    hideOperationDialog();

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
    if (!m_coreClient) return;

    if (m_audioManager) {
        m_audioManager->playIncomingCallRingtone();
    }

    m_incomingCalls.insert(friendNickname, { friendNickname, kDefaultIncomingCallSeconds });
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
    stopOperationTimer(core::UserOperationType::DECLINE_CALL);
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
    stopOperationTimer(core::UserOperationType::STOP_OUTGOING_CALL);
    if (ec) {
        if (m_dialogsController) {
            m_dialogsController->setIncomingCallButtonsActive(nickname, true);
        }
        LOG_WARN("Failed to decline call: {}", ec.message());
        if (m_coreClient && !m_coreClient->isConnectionDown()) {
            handleDeclineCallErrorNotificationAppearance();
        }
    }
    else {
        m_incomingCalls.remove(nickname);
        updateIncomingCallsUi();

        if (m_audioManager && m_incomingCalls.isEmpty()) {
            m_audioManager->stopIncomingCallRingtone();
        }

        if (m_audioManager) {
            m_audioManager->playCallingEndedEffect();
        }
    }
}

void CallManager::onEndCallResult(std::error_code ec)
{
    stopOperationTimer(core::UserOperationType::END_CALL);
    if (m_callWidget) {
        m_callWidget->setHangupButtonRestricted(false);
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
        hideOperationDialog();
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
    
    stopAllOperationTimers();
    
    if (m_screenCaptureController && m_screenCaptureController->isCapturing()) {
        emit stopScreenCaptureRequested();
    }

    if (m_cameraCaptureController && m_cameraCaptureController->isCapturing()) {
        emit stopCameraCaptureRequested();
    }

    if (m_callWidget) {
        m_callWidget->setCameraButtonRestricted(true);
        m_callWidget->setScreenShareButtonRestricted(true);
        m_callWidget->hideMainScreen();
        m_callWidget->hideAdditionalScreens();
    }

    if (m_dialogsController)
    {
        m_dialogsController->showConnectionDownWithUserDialog("Connection with participant lost. Waiting for them...");
    }
}

void CallManager::onCallParticipantConnectionRestored()
{
    if (!m_coreClient || !m_coreClient->isActiveCall())
    {
        hideOperationDialog();
        return;
    }

    if (m_dialogsController)
    {
        const int restoredDurationMs = 2500;
        m_dialogsController->showConnectionRestoredWithUserDialog("Connection with participant restored");

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
                m_dialogsController->hideConnectionRestoredWithUserDialog();
            }
        });
    }
}

void CallManager::onLocalConnectionDownInCall()
{
    if (!m_callWidget) return;

    m_callWidget->setCameraButtonActive(false);
    m_callWidget->setScreenShareButtonActive(false);
    m_callWidget->hideMainScreen();
    m_callWidget->hideAdditionalScreens();
}

void CallManager::startOperationTimer(core::UserOperationType operationKey, const QString& dialogText)
{
    m_pendingOperationTexts.insert(operationKey, dialogText);

    QTimer* timer = m_operationTimers.value(operationKey, nullptr);
    if (!timer)
    {
        timer = new QTimer(this);
        timer->setSingleShot(true);
        timer->setInterval(1000);
        connect(timer, &QTimer::timeout, this, [this, operationKey]()
        {
            onOperationTimerTimeout(operationKey);
        });
        m_operationTimers.insert(operationKey, timer);
    }

    timer->start();
}

void CallManager::stopOperationTimer(core::UserOperationType operationKey)
{
    if (QTimer* timer = m_operationTimers.value(operationKey, nullptr))
    {
        timer->stop();
    }

    m_pendingOperationTexts.remove(operationKey);

    if (m_dialogsController)
    {
        m_dialogsController->hidePendingOperationDialog(operationKey);
    }
}

void CallManager::stopAllOperationTimers()
{
    if (m_dialogsController)
    {
        for (auto it = m_operationTimers.constBegin(); it != m_operationTimers.constEnd(); ++it)
        {
            m_dialogsController->hidePendingOperationDialog(it.key());
        }
    }

    for (auto it = m_operationTimers.constBegin(); it != m_operationTimers.constEnd(); ++it)
    {
        if (QTimer* timer = it.value())
        {
            timer->stop();
        }
    }

    m_pendingOperationTexts.clear();
}

void CallManager::onOperationTimerTimeout(core::UserOperationType operationKey)
{
    if (!m_coreClient || m_coreClient->isConnectionDown())
    {
        return;
    }

    const QString dialogText = m_pendingOperationTexts.value(operationKey);
    if (dialogText.isEmpty())
    {
        return;
    }

    if (m_dialogsController)
    {
        m_dialogsController->showPendingOperationDialog(dialogText, operationKey);
    }
}

void CallManager::hideOperationDialog()
{
    stopAllOperationTimers();
    if (m_dialogsController) {
        m_dialogsController->hideConnectionDownWithUserDialog();
        m_dialogsController->hideConnectionRestoredWithUserDialog();
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