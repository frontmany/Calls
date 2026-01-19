#include "updateManager.h"
#include "widgets/authorizationWidget.h"
#include "widgets/mainMenuWidget.h"
#include "managers/dialogsController.h"
#include "events/updaterEventListener.h"

#include <QProcess>
#include <QObject>
#include <QDir>
#include <QJsonObject>
#include <QFileInfo>

#include "../utilities/logger.h"
#include "../utilities/constant.h"

UpdateManager::UpdateManager(std::shared_ptr<core::Client> client, std::shared_ptr<updater::Client> updater, ConfigManager* configManager, QObject* parent)
    : QObject(parent)
    , m_coreClient(client)
    , m_updaterClient(updater)
    , m_configManager(configManager)
{
}

void UpdateManager::setWidgets(AuthorizationWidget* authWidget, MainMenuWidget* mainMenuWidget, DialogsController* dialogsController)
{
    m_authorizationWidget = authWidget;
    m_mainMenuWidget = mainMenuWidget;
    m_dialogsController = dialogsController;
}

bool UpdateManager::shouldRestart() {
    return m_shouldRestart;
}


void UpdateManager::onUpdateCheckResult(updater::UpdateCheckResult result)
{
    m_authorizationWidget->hideUpdatesCheckingNotification();

    if (result == updater::UpdateCheckResult::possible_update) {
        m_authorizationWidget->showUpdateAvailableNotification();
        m_mainMenuWidget->showUpdateAvailableNotification();
    }
    else if (result == updater::UpdateCheckResult::required_update) {
        if (m_updaterClient) {
            m_updaterClient->startUpdate(resolveOperationSystemType());
        }
        if (m_dialogsController) {
            m_dialogsController->showUpdatingDialog();
        }
    }
    else if (result == updater::UpdateCheckResult::update_not_needed) {
    }
    else {
        LOG_ERROR("error: unknown UpdateCheckResult type");
    }
}

void UpdateManager::onUpdateButtonClicked()
{
    if (m_coreClient && m_coreClient->isOutgoingCall()) {
        m_coreClient->stopOutgoingCall();
    }

    if (m_coreClient) {
        auto callers = m_coreClient->getCallers();
        for (const auto& nickname : callers) {
            m_coreClient->declineCall(nickname);
        }
    }

    emit stopAllRingtonesRequested();
    
    if (m_mainMenuWidget) {
        m_mainMenuWidget->removeCallingPanel();
    }

    if (m_updaterClient) {
        m_updaterClient->startUpdate(resolveOperationSystemType());
    }
    if (m_dialogsController) {
        m_dialogsController->showUpdatingDialog();
    }
}

void UpdateManager::onUpdateLoaded(bool emptyUpdate)
{
    if (!emptyUpdate) {
        m_shouldRestart = true;

        m_coreClient->logout();
        m_updaterClient->stop();

        m_dialogsController->setUpdateDialogStatus("Restarting...");
    }
    else {
        m_dialogsController->setUpdateDialogStatus("Already up to date");
        m_dialogsController->hideUpdatingDialog();
    }
}

void UpdateManager::onLoadingProgress(double progress)
{
    m_dialogsController->setUpdateLoadingProgress(progress);
}

updater::OperationSystemType UpdateManager::resolveOperationSystemType()
{
    return m_configManager->getOperationSystemType();
}

void UpdateManager::launchUpdateApplier()
{
    qint64 currentPid = QCoreApplication::applicationPid();
   
    QString applicationDirectoryPath = m_configManager->getAppDirectoryPath();

    QString applicationPath;
    if (m_configManager->getOperationSystemType() == updater::OperationSystemType::WINDOWS) {
        applicationPath = QDir(applicationDirectoryPath).filePath(APPLICATION_EXECUTABLE_NAME_WINDOWS);
    }
    else {
        applicationPath = QDir(applicationDirectoryPath).filePath(APPLICATION_EXECUTABLE_NAME_LINUX_AND_MAC);
    }

    QString updateApplierPath;

    if (m_configManager && m_configManager->getOperationSystemType() == updater::OperationSystemType::WINDOWS) {
        updateApplierPath = QDir(applicationDirectoryPath).filePath(UPDATE_APPLIER_EXECUTABLE_NAME_WINDOWS);
    }
    else {
        updateApplierPath = QDir(applicationDirectoryPath).filePath(UPDATE_APPLIER_EXECUTABLE_NAME_LINUX_AND_MAC);
    } 

    QString temporaryUpdateDirectoryPath = m_configManager->getTemporaryUpdateDirectoryPath();
    QString configFileName = QFileInfo(m_configManager->getConfigPath()).fileName();
    QString deletionListFileName = m_configManager->getDeletionListFileName();

    QStringList arguments;
    arguments << QString::number(currentPid) << applicationPath << temporaryUpdateDirectoryPath << deletionListFileName << configFileName;

    QProcess::startDetached(updateApplierPath, arguments, applicationDirectoryPath);
}