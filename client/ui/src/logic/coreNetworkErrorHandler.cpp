#include "coreNetworkErrorHandler.h"
#include "widgets/authorizationWidget.h"
#include "widgets/mainMenuWidget.h"
#include "dialogs/dialogsController.h"
#include "notifications/notificationController.h"
#include "logic/navigationController.h"
#include "logic/configManager.h"
#include "logic/callManager.h"
#include "logic/meetingManager.h"
#include "audio/audioEffectsManager.h"

CoreNetworkErrorHandler::CoreNetworkErrorHandler(std::shared_ptr<core::Core> client, NavigationController* navigationController, ConfigManager* configManager, AudioEffectsManager* audioManager, QObject* parent)
    : QObject(parent)
    , m_coreClient(client)
    , m_navigationController(navigationController)
    , m_configManager(configManager)
    , m_audioManager(audioManager)
{
}

void CoreNetworkErrorHandler::setWidgets(AuthorizationWidget* authWidget, MainMenuWidget* mainMenuWidget, DialogsController* dialogsController)
{
    m_authorizationWidget = authWidget;
    m_mainMenuWidget = mainMenuWidget;
    m_dialogsController = dialogsController;
}

void CoreNetworkErrorHandler::setManagers(CallManager* callManager, MeetingManager* meetingManager)
{
    m_callManager = callManager;
    m_meetingManager = meetingManager;
}

void CoreNetworkErrorHandler::setNotificationController(NotificationController* notificationController)
{
    m_notificationController = notificationController;
}

void CoreNetworkErrorHandler::onConnectionDown()
{
    m_connectionWasDown = true;

    if (m_callManager) {
        m_callManager->onLocalConnectionDown();
    }

    if (m_callManager) {
        m_callManager->hideOperationDialog();
    }

    if (!m_coreClient) return;

    if (m_coreClient->isAuthorized()) {
        if (m_coreClient && m_coreClient->isActiveCall()) {
            if (m_coreClient->isScreenSharing()) {
                m_coreClient->stopScreenSharing();
            }
            if (m_coreClient->isCameraSharing()) {
                m_coreClient->stopCameraSharing();
            }
            
            if (m_callManager) {
                m_callManager->onLocalConnectionDownInCall();
            }
        }
        else if (m_coreClient->isInMeeting()) {
            if (m_coreClient->isScreenSharing()) {
                m_coreClient->stopScreenSharing();
            }
            if (m_coreClient->isCameraSharing()) {
                m_coreClient->stopCameraSharing();
            }
            if (m_meetingManager) {
                QMetaObject::invokeMethod(m_meetingManager, "onLocalConnectionDownInMeeting", Qt::QueuedConnection);
            }
        }
        else {
            if (m_audioManager) {
                m_audioManager->stopIncomingCallRingtone();
                m_audioManager->stopOutgoingCallRingtone();
            }

            if (m_mainMenuWidget) {
                m_mainMenuWidget->removeOutgoingCallPanel();
                m_mainMenuWidget->setStatusLabelOnline();
            }
        }

        if (m_notificationController) {
            m_notificationController->showConnectionDown();
        }
    }
    else {
        if (m_authorizationWidget) {
            m_authorizationWidget->resetBlur();
            m_authorizationWidget->setAuthorizationDisabled(true);
        }

        if (m_notificationController)
        {
            m_notificationController->showConnectionDown();
        }
    }
}

void CoreNetworkErrorHandler::onConnectionEstablished()
{
    const int connectionRestoredDurationMs = 1500;
    if (m_notificationController && m_connectionWasDown) {
        m_notificationController->showConnectionRestored(connectionRestoredDurationMs);
    }
    m_connectionWasDown = false;

    if (m_authorizationWidget) {
        m_authorizationWidget->resetBlur();
        m_authorizationWidget->setAuthorizationDisabled(false);
    }
}

void CoreNetworkErrorHandler::onConnectionEstablishedAuthorizationNeeded()
{
    const int connectionRestoredDurationMs = 1500;
    if (m_notificationController && m_connectionWasDown) {
        m_notificationController->showConnectionRestored(connectionRestoredDurationMs);
    }
    m_connectionWasDown = false;

    if (m_authorizationWidget && m_navigationController) {
        m_navigationController->switchToAuthorizationWidget();
        m_authorizationWidget->resetBlur();
        m_authorizationWidget->setAuthorizationDisabled(false);
    }
}
