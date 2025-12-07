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
#include "configManager.h"
#include "calls.h"
#include "logger.h"

class AuthorizationWidget;
class MainMenuWidget;
class DialogsController;

class UpdateManager : public QObject {
    Q_OBJECT

public:
    explicit UpdateManager(ConfigManager* configManager, QObject* parent = nullptr);
    
    void checkUpdates();
    void setWidgets(AuthorizationWidget* authWidget, MainMenuWidget* mainMenuWidget, DialogsController* dialogsController);

signals:
    void stopAllRingtonesRequested();

public slots:
    void onUpdaterCheckResult(updater::UpdatesCheckResult checkResult);
    void onUpdateButtonClicked();
    void onUpdateLoaded(bool emptyUpdate);
    void onLoadingProgress(double progress);

private:
    std::string parseVersionFromConfig();
    updater::OperationSystemType resolveOperationSystemType();
    void launchUpdateApplier();

    ConfigManager* m_configManager = nullptr;
    AuthorizationWidget* m_authorizationWidget = nullptr;
    MainMenuWidget* m_mainMenuWidget = nullptr;
    DialogsController* m_dialogsController = nullptr;
};
