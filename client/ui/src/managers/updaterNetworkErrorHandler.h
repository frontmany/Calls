#pragma once

#include <QObject>
#include <memory>

#include "core.h"
#include "updater.h"
#include "utilities/logger.h"

class AuthorizationWidget;
class DialogsController;
class NavigationController;
class UpdateManager;
class ConfigManager;
class MainMenuWidget;

class UpdaterNetworkErrorHandler : public QObject {
    Q_OBJECT

public:
    explicit UpdaterNetworkErrorHandler(std::shared_ptr<core::Client> coreClient, std::shared_ptr<updater::Client> updater, NavigationController* navigationController, UpdateManager* updateManager, ConfigManager* configManager, QObject* parent = nullptr);
    
    void setWidgets(AuthorizationWidget* authWidget, MainMenuWidget* mainMenuWidget, DialogsController* dialogsController);

public slots:
    void onNetworkError();
    void onConnected();

private:
    std::shared_ptr<core::Client> m_coreClient = nullptr;
    std::shared_ptr<updater::Client> m_updaterClient = nullptr;
    NavigationController* m_navigationController = nullptr;
    UpdateManager* m_updateManager = nullptr;
    ConfigManager* m_configManager = nullptr;
    AuthorizationWidget* m_authorizationWidget = nullptr;
    MainMenuWidget* m_mainMenuWidget = nullptr;
    DialogsController* m_dialogsController = nullptr;
};
