#include "networkErrorHandler.h"
#include "widgets/authorizationWidget.h"
#include "widgets/mainMenuWidget.h"
#include "managers/dialogsController.h"
#include "managers/navigationController.h"
#include "managers/updateManager.h"
#include "managers/configManager.h"
#include "media/audioEffectsManager.h"
#include "updater.h"
#include "client.h"

NetworkErrorHandler::NetworkErrorHandler(std::shared_ptr<callifornia::Client> client, std::shared_ptr<callifornia::updater::Client> updater, NavigationController* navigationController, UpdateManager* updateManager, ConfigManager* configManager, AudioEffectsManager* audioManager, QObject* parent)
    : QObject(parent)
    , m_client(client)
    , m_updater(updater)
    , m_navigationController(navigationController)
    , m_updateManager(updateManager)
    , m_configManager(configManager)
    , m_audioManager(audioManager)
{
}

void NetworkErrorHandler::setWidgets(AuthorizationWidget* authWidget, MainMenuWidget* mainMenuWidget, DialogsController* dialogsController)
{
    m_authorizationWidget = authWidget;
    m_mainMenuWidget = mainMenuWidget;
    m_dialogsController = dialogsController;
}

void NetworkErrorHandler::onClientNetworkError()
{
    LOG_ERROR("Client network error occurred");

    if (m_audioManager) {
        m_audioManager->stopIncomingCallRingtone();
        m_audioManager->stopCallingRingtone();
    }

    if (m_mainMenuWidget) {
        m_mainMenuWidget->removeCallingPanel();
        m_mainMenuWidget->clearIncomingCalls();
    }

    if (m_authorizationWidget && m_navigationController) {
        m_navigationController->switchToAuthorizationWidget();

        m_authorizationWidget->resetBlur();
        m_authorizationWidget->setAuthorizationDisabled(true);

        if (!m_updater || !m_updater->isAwaitingServerResponse()) {
            m_authorizationWidget->showNetworkErrorNotification();
        }
    }
}

void NetworkErrorHandler::onUpdaterNetworkError()
{
    LOG_ERROR("Updater network error occurred");

    if (m_authorizationWidget) {
        m_authorizationWidget->hideUpdateAvailableNotification();
    }

    if (m_mainMenuWidget) {
        m_mainMenuWidget->hideUpdateAvailableNotification();
    }

    if (m_dialogsController) {
        m_dialogsController->hideUpdatingDialog();
        if (!m_client || !m_client->isAuthorized() || (m_updater && m_updater->isAwaitingServerResponse())) {
            m_dialogsController->showConnectionErrorDialog();
        }
    }

    if (m_updater) {
        m_updater->disconnect();
    }
}

void NetworkErrorHandler::onConnectionRestored()
{
    LOG_INFO("Connection restored to server");

    if (m_authorizationWidget) {
        m_authorizationWidget->resetBlur();
        m_authorizationWidget->setAuthorizationDisabled(false);
        m_authorizationWidget->showConnectionRestoredNotification(1500);
    }

    if ((!m_updater || !m_updater->isConnected()) && m_updateManager) {
        LOG_INFO("Reconnecting updater");
        m_updateManager->checkUpdates();
    }
}
