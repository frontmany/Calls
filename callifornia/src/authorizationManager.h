#pragma once

#include <QObject>
#include <QString>

#include "calls.h"

class AuthorizationWidget;
class NavigationController;
class ConfigManager;
class DialogsController;

class AuthorizationManager : public QObject {
    Q_OBJECT

public:
    explicit AuthorizationManager(NavigationController* navigationController, ConfigManager* configManager, DialogsController* dialogsController, QObject* parent = nullptr);
    
    void setAuthorizationWidget(AuthorizationWidget* authWidget);
    void setMainMenuWidget(class MainMenuWidget* mainMenuWidget);

public slots:
    void onAuthorizationButtonClicked(const QString& friendNickname);
    void onAuthorizationResult(calls::ErrorCode ec);
    void onBlurAnimationFinished();

private:
    AuthorizationWidget* m_authorizationWidget = nullptr;
    class MainMenuWidget* m_mainMenuWidget = nullptr;
    NavigationController* m_navigationController = nullptr;
    ConfigManager* m_configManager = nullptr;
    DialogsController* m_dialogsController = nullptr;
};
