#pragma once

#include <QObject>
#include <memory>

#include "updater.h"
#include "managers/configManager.h"
#include "core.h"

class AuthorizationWidget;
class MainMenuWidget;
class DialogsController;

class UpdateManager : public QObject {
    Q_OBJECT

public:
    explicit UpdateManager(std::shared_ptr<core::Client> client, std::shared_ptr<updater::Client> updater, ConfigManager* configManager, QObject* parent = nullptr);
    void setWidgets(AuthorizationWidget* authWidget, MainMenuWidget* mainMenuWidget, DialogsController* dialogsController);
    bool shouldRestart();
    void launchUpdateApplier();

signals:
    void stopAllRingtonesRequested();

public slots:
    void onUpdateCheckResult(updater::UpdateCheckResult result);
    void onUpdateLoaded(bool emptyUpdate);
    void onLoadingProgress(double progress);
    void onUpdateButtonClicked();

private:
    updater::OperationSystemType resolveOperationSystemType();

private:
    bool m_shouldRestart = false;

    std::shared_ptr<core::Client> m_coreClient = nullptr;
    std::shared_ptr<updater::Client> m_updaterClient = nullptr;

    ConfigManager* m_configManager = nullptr; 
    AuthorizationWidget* m_authorizationWidget = nullptr;
    MainMenuWidget* m_mainMenuWidget = nullptr;
    DialogsController* m_dialogsController = nullptr;
};
