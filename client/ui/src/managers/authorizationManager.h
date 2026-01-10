#pragma once

#include <QObject>
#include <memory>

#include "core.h"

class AuthorizationWidget;
class NavigationController;
class ConfigManager;
class DialogsController;
class MainMenuWidget;
class UpdateManager;

class AuthorizationManager : public QObject {
    Q_OBJECT

public:
    explicit AuthorizationManager(std::shared_ptr<core::Client> client, NavigationController* navigationController, ConfigManager* configManager, DialogsController* dialogsController, UpdateManager* updateManager, QObject* parent = nullptr);
    
    void setWidgets(AuthorizationWidget* authorizationWidget, MainMenuWidget* mainMenuWidget);

public slots:
    void onAuthorizationButtonClicked(const QString& friendNickname);
    void onAuthorizationResult(std::error_code ec);
    void onLogoutCompleted();

private:
    std::shared_ptr<core::Client> m_coreClient = nullptr;
    AuthorizationWidget* m_authorizationWidget = nullptr;
    class MainMenuWidget* m_mainMenuWidget = nullptr;
    NavigationController* m_navigationController = nullptr;
    ConfigManager* m_configManager = nullptr;
    DialogsController* m_dialogsController = nullptr;
    UpdateManager* m_updateManager = nullptr;
};
