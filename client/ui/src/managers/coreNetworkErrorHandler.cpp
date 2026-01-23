#include "coreNetworkErrorHandler.h"
#include "widgets/authorizationWidget.h"
#include "widgets/mainMenuWidget.h"
#include "managers/dialogsController.h"
#include "managers/notificationController.h"
#include "managers/navigationController.h"
#include "managers/configManager.h"
#include "managers/callManager.h"
#include "media/audioEffectsManager.h"
#include "media/screenSharingManager.h"
#include "media/cameraSharingManager.h"
#include <QTimer>

CoreNetworkErrorHandler::CoreNetworkErrorHandler(std::shared_ptr<core::Client> client, NavigationController* navigationController, ConfigManager* configManager, AudioEffectsManager* audioManager, QObject* parent)
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

void CoreNetworkErrorHandler::setManagers(CallManager* callManager, ScreenSharingManager* screenSharingManager, CameraSharingManager* cameraSharingManager)
{
    m_callManager = callManager;
    m_screenSharingManager = screenSharingManager;
    m_cameraSharingManager = cameraSharingManager;
}

void CoreNetworkErrorHandler::setNotificationController(NotificationController* notificationController)
{
    m_notificationController = notificationController;
}



void CoreNetworkErrorHandler::onConnectionRestored()
{
    if (m_notificationController) {
        m_notificationController->showConnectionRestored(1500);
    }

    if (m_authorizationWidget) {
        m_authorizationWidget->resetBlur();
        m_authorizationWidget->setAuthorizationDisabled(false);
    }
}

void CoreNetworkErrorHandler::onConnectionDown()
{
    if (m_callManager) {
        m_callManager->hideOperationDialog();
    }
    if (m_screenSharingManager) {
        m_screenSharingManager->hideOperationDialog();
    }
    if (m_cameraSharingManager) {
        m_cameraSharingManager->hideOperationDialog();
    }

    if (m_coreClient->isAuthorized()) {
        if (m_coreClient && m_coreClient->isActiveCall()) {
            if (m_screenSharingManager) {
                m_screenSharingManager->stopLocalScreenCapture();
            }

            if (m_cameraSharingManager) {
                m_cameraSharingManager->stopLocalCameraCapture();
            }
            
            if (m_callManager) {
                m_callManager->onLocalConnectionDownInCall();
            }
        }
        else {
            if (m_audioManager) {
                m_audioManager->stopIncomingCallRingtone();
                m_audioManager->stopCallingRingtone();
            }

            if (m_mainMenuWidget) {
                m_mainMenuWidget->removeCallingPanel();
            }
        }

        if (m_notificationController) {
            m_notificationController->showConnectionDown();
        }
    }
    else {
        m_authorizationWidget->resetBlur();
        m_authorizationWidget->setAuthorizationDisabled(true);

        if (m_notificationController)
        {
            m_notificationController->showConnectionDown();
        }
    }
}

void CoreNetworkErrorHandler::onConnectionRestoredAuthorizationNeeded()
{
    if (m_notificationController) {
        m_notificationController->showConnectionRestored(1500);
    }

    if (m_authorizationWidget && m_navigationController) {
        m_navigationController->switchToAuthorizationWidget();
        m_authorizationWidget->resetBlur();
        m_authorizationWidget->setAuthorizationDisabled(false);
    }
}
