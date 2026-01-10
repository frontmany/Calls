#pragma once

#include <QObject>
#include <memory>

#include "core.h"
#include "utilities/logger.h"

class AuthorizationWidget;
class DialogsController;
class NavigationController;
class ConfigManager;
class MainMenuWidget;
class AudioEffectsManager;
class CallManager;
class ScreenSharingManager;
class CameraSharingManager;

class CoreNetworkErrorHandler : public QObject {
    Q_OBJECT

public:
    explicit CoreNetworkErrorHandler(std::shared_ptr<core::Client> client, NavigationController* navigationController, ConfigManager* configManager, AudioEffectsManager* audioManager, QObject* parent = nullptr);
    
    void setWidgets(AuthorizationWidget* authWidget, MainMenuWidget* mainMenuWidget, DialogsController* dialogsController);
    void setManagers(CallManager* callManager, ScreenSharingManager* screenSharingManager, CameraSharingManager* cameraSharingManager);

public slots:
    void onConnectionDown();
    void onConnectionRestored();
    void onConnectionRestoredAuthorizationNeeded();

private:
    std::shared_ptr<core::Client> m_coreClient = nullptr;
    NavigationController* m_navigationController = nullptr;
    ConfigManager* m_configManager = nullptr;
    AuthorizationWidget* m_authorizationWidget = nullptr;
    MainMenuWidget* m_mainMenuWidget = nullptr;
    DialogsController* m_dialogsController = nullptr;
    AudioEffectsManager* m_audioManager = nullptr;
    CallManager* m_callManager = nullptr;
    ScreenSharingManager* m_screenSharingManager = nullptr;
    CameraSharingManager* m_cameraSharingManager = nullptr;
};
