#include "updaterNetworkErrorHandler.h"
#include "widgets/authorizationWidget.h"
#include "widgets/mainMenuWidget.h"
#include "managers/dialogsController.h"
#include "managers/navigationController.h"
#include "managers/updateManager.h"
#include "managers/configManager.h"
#include "core.h"
#include "updater.h"

UpdaterNetworkErrorHandler::UpdaterNetworkErrorHandler(std::shared_ptr<core::Client> coreClient, std::shared_ptr<updater::Client> updater, NavigationController* navigationController, UpdateManager* updateManager, ConfigManager* configManager, QObject* parent)
    : QObject(parent)
    , m_coreClient(coreClient)
    , m_updaterClient(updater)
    , m_navigationController(navigationController)
    , m_updateManager(updateManager)
    , m_configManager(configManager)
{
}

void UpdaterNetworkErrorHandler::setWidgets(AuthorizationWidget* authWidget, MainMenuWidget* mainMenuWidget, DialogsController* dialogsController)
{
    m_authorizationWidget = authWidget;
    m_mainMenuWidget = mainMenuWidget;
    m_dialogsController = dialogsController;
}

void UpdaterNetworkErrorHandler::onNetworkError()
{
    LOG_ERROR("Updater network error occurred");

    if (!m_updaterClient || !m_updaterClient->isLoadingUpdate()) {
        return; // Do nothing if not loading an update
    }

    if (m_authorizationWidget) {
        m_authorizationWidget->hideUpdateAvailableNotification();
    }

    if (m_mainMenuWidget) {
        m_mainMenuWidget->hideUpdateAvailableNotification();
    }

    if (m_dialogsController) {
        m_dialogsController->hideUpdatingDialog();
        m_dialogsController->showUpdateErrorDialog();
    }
}

void UpdaterNetworkErrorHandler::onConnected()
{

    if (m_updaterClient && m_configManager) {
        m_updaterClient->checkUpdates(m_configManager->getVersion().toStdString());
    }
}
