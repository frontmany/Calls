#pragma once

#include <QObject>
#include <QTimer>
#include <QProcess>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>

#include "updater.h"
#include "managers/configManager.h"
#include "client.h"
#include "jsonType.h"
#include "utilities/logger.h"

class AuthorizationWidget;
class MainMenuWidget;
class DialogsController;

class UpdateManager : public QObject {
    Q_OBJECT

public:
    explicit UpdateManager(callifornia::Client* client, callifornia::updater::Updater* updater, ConfigManager* configManager, QObject* parent = nullptr);
    
    void checkUpdates();
    void setWidgets(AuthorizationWidget* authWidget, MainMenuWidget* mainMenuWidget, DialogsController* dialogsController);

signals:
    void stopAllRingtonesRequested();

public slots:
    void onUpdaterCheckResult(callifornia::updater::UpdateCheckResult checkResult);
    void onUpdateButtonClicked();
    void onUpdateLoaded(bool emptyUpdate);
    void onLoadingProgress(double progress);

private:
    std::string parseVersionFromConfig();
    callifornia::updater::OperationSystemType resolveOperationSystemType();
    void launchUpdateApplier();

    callifornia::Client* m_client = nullptr;
    callifornia::updater::Updater* m_updater = nullptr;
    ConfigManager* m_configManager = nullptr; 
    AuthorizationWidget* m_authorizationWidget = nullptr;
    MainMenuWidget* m_mainMenuWidget = nullptr;
    DialogsController* m_dialogsController = nullptr;
};
