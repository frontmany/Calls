#pragma once

#include <QObject>
#include <memory>

#include "client.h"
#include "updater.h"
#include "utilities/logger.h"

class AuthorizationWidget;
class DialogsController;
class NavigationController;
class UpdateManager;
class ConfigManager;
class MainMenuWidget;
class AudioEffectsManager;

class NetworkErrorHandler : public QObject {
    Q_OBJECT

public:
    explicit NetworkErrorHandler(std::shared_ptr<callifornia::Client> client, std::shared_ptr<callifornia::updater::Client> updater, NavigationController* navigationController, UpdateManager* updateManager, ConfigManager* configManager, AudioEffectsManager* audioManager, QObject* parent = nullptr);
    
    void setWidgets(AuthorizationWidget* authWidget, MainMenuWidget* mainMenuWidget, DialogsController* dialogsController);

public slots:
    void onClientNetworkError();
    void onUpdaterNetworkError();
    void onConnectionRestored();

private:
    std::shared_ptr<callifornia::Client> m_client = nullptr;
    std::shared_ptr<callifornia::updater::Client> m_updater = nullptr;
    NavigationController* m_navigationController = nullptr;
    UpdateManager* m_updateManager = nullptr;
    ConfigManager* m_configManager = nullptr;
    AuthorizationWidget* m_authorizationWidget = nullptr;
    MainMenuWidget* m_mainMenuWidget = nullptr;
    DialogsController* m_dialogsController = nullptr;
    AudioEffectsManager* m_audioManager = nullptr;
};
