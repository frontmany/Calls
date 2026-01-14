#include "coreNetworkErrorHandler.h"
#include "widgets/authorizationWidget.h"
#include "widgets/mainMenuWidget.h"
#include "managers/dialogsController.h"
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



void CoreNetworkErrorHandler::onConnectionRestored()
{

    if (m_dialogsController) {
        m_dialogsController->hideNotificationDialog();
        m_dialogsController->showNotificationDialog("Connection restored", false, true, false);

        QTimer::singleShot(1500, this, [this]()
        {
            if (m_dialogsController)
            {
                m_dialogsController->hideNotificationDialog();
            }
        });
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
        }
        else {
            if (m_audioManager) {
                m_audioManager->stopIncomingCallRingtone();
                m_audioManager->stopCallingRingtone();
            }

            if (m_mainMenuWidget) {
                m_mainMenuWidget->removeCallingPanel();
                m_mainMenuWidget->clearIncomingCalls();
            }
        }

        if (m_dialogsController) {
            m_dialogsController->showNotificationDialog("Reconnecting...", true, false, true);
        }
    }
    else {
        m_authorizationWidget->resetBlur();
        m_authorizationWidget->setAuthorizationDisabled(true);

        if (m_dialogsController)
        {
            m_dialogsController->showNotificationDialog("Reconnecting...", true, false, true);
        }
    }
}

void CoreNetworkErrorHandler::onConnectionRestoredAuthorizationNeeded()
{
    if (m_dialogsController) {
        m_dialogsController->hideNotificationDialog();
        m_dialogsController->showNotificationDialog("Connection restored", false, true, false);

        QTimer::singleShot(1500, this, [this]()
        {
            if (m_dialogsController)
            {
                m_dialogsController->hideNotificationDialog();
            }
        });
    }

    if (m_authorizationWidget && m_navigationController) {
        m_navigationController->switchToAuthorizationWidget();
        m_authorizationWidget->resetBlur();
        m_authorizationWidget->setAuthorizationDisabled(false);
    }
}