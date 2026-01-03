#include "networkErrorHandler.h"
#include "authorizationWidget.h"
#include "mainMenuWidget.h"
#include "dialogsController.h"
#include "navigationController.h"
#include "updateManager.h"
#include "configManager.h"
#include "audioEffectsManager.h"
#include "updater.h"
#include "client.h"

NetworkErrorHandler::NetworkErrorHandler(callifornia::Client* client, callifornia::updater::Updater* updater, NavigationController* navigationController, UpdateManager* updateManager, ConfigManager* configManager, QObject* parent)
    : QObject(parent)
    , m_client(client)
    , m_updater(updater)
    , m_navigationController(navigationController)
    , m_updateManager(updateManager)
    , m_configManager(configManager)
{
}

void NetworkErrorHandler::setWidgets(AuthorizationWidget* authWidget, MainMenuWidget* mainMenuWidget, DialogsController* dialogsController)
{
    m_authorizationWidget = authWidget;
    m_mainMenuWidget = mainMenuWidget;
    m_dialogsController = dialogsController;
}

void NetworkErrorHandler::setAudioManager(AudioEffectsManager* audioManager)
{
    m_audioManager = audioManager;
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
