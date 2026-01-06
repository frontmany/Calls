#pragma once

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

signals:
    void stopAllRingtonesRequested();

public slots:
    void onUpdaterCheckResult(updater::UpdateStatus status);
    void onUpdateLoadingFailed();
    void onUpdateLoaded(bool emptyUpdate);
    void onLoadingProgress(double progress);
    void onUpdateButtonClicked();

private:
    void launchUpdateApplier();
    std::string parseVersionFromConfig();
    updater::OperationSystemType resolveOperationSystemType();

private:
    std::shared_ptr<core::Client> m_client = nullptr;
    std::shared_ptr<updater::Client> m_updater = nullptr;

    ConfigManager* m_configManager = nullptr; 
    AuthorizationWidget* m_authorizationWidget = nullptr;
    MainMenuWidget* m_mainMenuWidget = nullptr;
    DialogsController* m_dialogsController = nullptr;
};
