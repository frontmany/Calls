#include "updaterNetworkErrorHandler.h"
#include "widgets/authorizationWidget.h"
#include "widgets/mainMenuWidget.h"
#include "managers/dialogsController.h"
#include "managers/notificationController.h"
#include "managers/navigationController.h"
#include "managers/updateManager.h"
#include "managers/configManager.h"
#include <QTimer>

UpdaterNetworkErrorHandler::UpdaterNetworkErrorHandler(std::shared_ptr<updater::Client> updater, NavigationController* navigationController, UpdateManager* updateManager, ConfigManager* configManager, QObject* parent)
    : QObject(parent)
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

void UpdaterNetworkErrorHandler::setNotificationController(NotificationController* notificationController)
{
    m_notificationController = notificationController;
}

void UpdaterNetworkErrorHandler::onNetworkError()
{
    LOG_ERROR("Updater network error occurred");

    if (m_dialogsController) {
        m_dialogsController->hideUpdateAvailableDialog();
    }

    if (!m_updaterClient->isLoadingUpdate()) {
        return; // Do nothing if not loading an update
    }

    if (m_dialogsController) {
        m_dialogsController->hideUpdatingDialog();
        if (m_notificationController) {
            m_notificationController->showUpdateError(1500);
        }
    }
} 

void UpdaterNetworkErrorHandler::onConnected()
{
    LOG_INFO("Updater connected");

    if (m_updaterClient && m_configManager) {
        m_updaterClient->checkUpdates(m_configManager->getVersion().toStdString());
    }
}