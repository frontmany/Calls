#pragma once

#include <QObject>

#include "calls.h"
#include "updater.h"
#include "logger.h"

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
    explicit NetworkErrorHandler(NavigationController* navigationController, UpdateManager* updateManager, ConfigManager* configManager, QObject* parent = nullptr);
    
    void setWidgets(AuthorizationWidget* authWidget, MainMenuWidget* mainMenuWidget, DialogsController* dialogsController);
    void setAudioManager(AudioEffectsManager* audioManager);

public slots:
    void onClientNetworkError();
    void onUpdaterNetworkError();
    void onConnectionRestored();

private:
    NavigationController* m_navigationController = nullptr;
    UpdateManager* m_updateManager = nullptr;
    ConfigManager* m_configManager = nullptr;
    AuthorizationWidget* m_authorizationWidget = nullptr;
    MainMenuWidget* m_mainMenuWidget = nullptr;
    DialogsController* m_dialogsController = nullptr;
    AudioEffectsManager* m_audioManager = nullptr;
};
