#include "updateManager.h"
#include "widgets/authorizationWidget.h"
#include "widgets/mainMenuWidget.h"
#include "dialogs/dialogsController.h"
#include "events/updaterEventListener.h"
#include "constants/constant.h"

#include <QProcess>
#include <QObject>
#include <QDir>
#include <QJsonObject>
#include <QFileInfo>

#include "../utilities/logger.h"
#include "constants/constant.h"

UpdateManager::UpdateManager(std::shared_ptr<core::Core> client, std::shared_ptr<updater::Client> updater, ConfigManager* configManager, QObject* parent)
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

bool UpdateManager::isUpdateNeeded() const {
    return m_updateNeeded;
}

void UpdateManager::showUpdateAvailableDialogIfNeeded()
{
    if (m_updateNeeded && !m_pendingUpdateVersion.isEmpty() && m_dialogsController) {
        m_dialogsController->showUpdateAvailableDialog(m_pendingUpdateVersion);
    }
}

void UpdateManager::onUpdateCheckResult(updater::CheckResult result, const QString& newVersion)
{
    if (result == updater::CheckResult::POSSIBLE_UPDATE) {
        m_updateNeeded = true;
        m_pendingUpdateVersion = newVersion;
        // Показываем диалог только если пользователь не в звонке
        if (m_dialogsController && m_coreClient && !m_coreClient->isActiveCall()) {
            m_dialogsController->showUpdateAvailableDialog(newVersion);
        }
    }
    else if (result == updater::CheckResult::REQUIRED_UPDATE) {
        m_updateNeeded = true;
        if (m_coreClient) {
            m_coreClient->stop();
        }
        if (m_updaterClient) {
            m_updaterClient->startUpdate(resolveOperationSystemType());
        }
        if (m_dialogsController) {
            m_dialogsController->showUpdatingDialog();
        }
    }
    else if (result == updater::CheckResult::UPDATE_NOT_NEEDED) {
        m_updateNeeded = false;
        m_pendingUpdateVersion.clear();
    }
    else {
        LOG_ERROR("error: unknown CheckResult type");
    }
}

void UpdateManager::onUpdateButtonClicked()
{
    emit stopAllRingtonesRequested();

    if (m_coreClient) {
        m_coreClient->stop();
    }

    if (m_mainMenuWidget) {
        m_mainMenuWidget->removeOutgoingCallPanel();
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

        m_updaterClient->stop();

        m_dialogsController->setUpdateDialogStatus("Restarting...");

        if (m_coreClient->isAuthorized()) {
            std::error_code ec = m_coreClient->logout();
            if (ec) {
                LOG_WARN("Logout failed during update: {}", ec.message());
            }
        }
        else {
            QTimer::singleShot(UPDATE_APPLIER_DELAY_MS, [this]() {
                launchUpdateApplier();
            });
        }
    }
    else {
        m_dialogsController->setUpdateDialogStatus("Already up to date");
        m_dialogsController->hideUpdatingDialog();
        onUpdateAborted();
    }
}

void UpdateManager::onUpdateAborted()
{
    emit coreRestartRequested();
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
   
    if (m_configManager) {
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

        bool started = QProcess::startDetached(updateApplierPath, arguments, applicationDirectoryPath);
        if (started) {
            LOG_INFO("Successfully launched update applier: {}", updateApplierPath.toStdString());
        } else {
            LOG_ERROR("Failed to launch update applier: {}", updateApplierPath.toStdString());
        }
    }
} 
