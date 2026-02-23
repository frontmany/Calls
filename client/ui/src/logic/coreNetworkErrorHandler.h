#pragma once

#include <QObject>
#include <memory>

#include "core.h"

class AuthorizationWidget;
class DialogsController;
class NotificationController;
class NavigationController;
class MainMenuWidget;
class AudioEffectsManager;
class CallManager;
class ConfigManager;

class CoreNetworkErrorHandler : public QObject {
    Q_OBJECT

public:
    explicit CoreNetworkErrorHandler(std::shared_ptr<core::Core> client, NavigationController* navigationController, ConfigManager* configManager, AudioEffectsManager* audioManager, QObject* parent = nullptr);
    
    void setWidgets(AuthorizationWidget* authWidget, MainMenuWidget* mainMenuWidget, DialogsController* dialogsController);
    void setManagers(CallManager* callManager);
    void setNotificationController(NotificationController* notificationController);

public slots:
    void onConnectionDown();
    void onConnectionEstablished();
    void onConnectionEstablishedAuthorizationNeeded();

private:
    std::shared_ptr<core::Core> m_coreClient = nullptr;
    NavigationController* m_navigationController = nullptr;
    ConfigManager* m_configManager = nullptr;
    AuthorizationWidget* m_authorizationWidget = nullptr;
    MainMenuWidget* m_mainMenuWidget = nullptr;
    DialogsController* m_dialogsController = nullptr;
    NotificationController* m_notificationController = nullptr;
    AudioEffectsManager* m_audioManager = nullptr;
    CallManager* m_callManager = nullptr;
    bool m_connectionWasDown = false;
};
