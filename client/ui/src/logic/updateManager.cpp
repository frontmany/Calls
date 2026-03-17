#include "updateManager.h"
#include "widgets/authorizationWidget.h"
#include "widgets/mainMenuWidget.h"
#include "dialogs/dialogsController.h"
#include "events/updaterEventListener.h"
#include "constants/constant.h"
#include "constants/configKey.h"

#include <QProcess>
#include <QObject>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>

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

void UpdateManager::showUpdateButtonIfNeeded()
{
    if (m_updateNeeded && !m_pendingUpdateVersion.isEmpty() && m_mainMenuWidget) {
        m_mainMenuWidget->setUpdateButtonVisible(true);
    }
}

void UpdateManager::resetUpdateAvailability()
{
    resetUpdateState();
}

void UpdateManager::onUpdateCheckResult(updater::CheckResult result, const QString& newVersion)
{
    if (result == updater::CheckResult::POSSIBLE_UPDATE) {
        m_updateNeeded = true;
        m_pendingUpdateVersion = newVersion;

        if (isUpdateAlreadyDownloadedForVersion(newVersion)) {
            m_updateMode = UpdateMode::Background;
            m_downloadState = DownloadState::Downloaded;
            m_userCommitted = false;
            m_lastProgress = 100.0;
            if (m_mainMenuWidget) {
                m_mainMenuWidget->setUpdateButtonVisible(true);
            }
            return;
        }

        if (m_updateMode == UpdateMode::Background && m_downloadState == DownloadState::Downloaded) {
            if (m_mainMenuWidget) {
                m_mainMenuWidget->setUpdateButtonVisible(true);
            }
            return;
        }
        m_updateMode = UpdateMode::Background;
        m_downloadState = DownloadState::Downloading;
        m_userCommitted = false;
        m_lastProgress = 0.0;
        if (m_mainMenuWidget) {
            m_mainMenuWidget->setUpdateButtonVisible(true);
        }
        if (m_updaterClient && !m_updaterClient->isLoadingUpdate()) {
            m_updaterClient->startUpdateAsync(resolveOperationSystemType(), m_updaterClient);
        }
    }
    else if (result == updater::CheckResult::REQUIRED_UPDATE) {
        m_updateNeeded = true;
        m_updateMode = UpdateMode::Foreground;
        m_downloadState = DownloadState::Downloading;
        m_userCommitted = true;
        m_lastProgress = 0.0;
        if (m_coreClient) {
            m_coreClient->stop();
        }
        if (m_dialogsController) {
            m_dialogsController->showUpdatingDialog();
            m_dialogsController->setUpdateDialogStatus(QStringLiteral("Preparing update..."), true);
        }
        if (m_updaterClient) {
            m_updaterClient->startUpdateAsync(resolveOperationSystemType(), m_updaterClient);
        }
    }
    else if (result == updater::CheckResult::UPDATE_NOT_NEEDED) {
        resetUpdateState();
    }
    else {
        LOG_ERROR("error: unknown CheckResult type");
    }
}

void UpdateManager::onUpdateButtonClicked()
{
    m_userCommitted = true;
    if (m_mainMenuWidget) {
        m_mainMenuWidget->setUpdateButtonVisible(false);
    }

    if (m_downloadState == DownloadState::Downloaded) {
        applyUpdate();
        return;
    }

    if (m_downloadState == DownloadState::Downloading) {
        if (m_dialogsController) {
            m_dialogsController->showUpdatingDialog();
            m_dialogsController->setUpdateDialogStatus(QStringLiteral("Downloading..."), false);
            if (m_lastProgress > 0.0) {
                m_dialogsController->setUpdateLoadingProgress(m_lastProgress);
            }
        }
        return;
    }

    // Fallback: if user clicked before auto-download started, trigger it now.
    m_updateMode = UpdateMode::Background;
    m_downloadState = DownloadState::Downloading;
    m_lastProgress = 0.0;
    if (m_dialogsController) {
        m_dialogsController->showUpdatingDialog();
        m_dialogsController->setUpdateDialogStatus(QStringLiteral("Preparing update..."), true);
    }
    if (m_updaterClient && !m_updaterClient->isLoadingUpdate()) {
        m_updaterClient->startUpdateAsync(resolveOperationSystemType(), m_updaterClient);
    }
}

void UpdateManager::onManifestProgress(qulonglong filesProcessed, qulonglong totalFiles, const QString& currentFilePath)
{
    if (!m_dialogsController || !shouldShowProgressDialog()) return;
    if (totalFiles > 0) {
        QString status = QStringLiteral("Preparing update... %1/%2 files").arg(filesProcessed).arg(totalFiles);
        if (!currentFilePath.isEmpty()) {
            const QString fileName = QFileInfo(currentFilePath).fileName();
            if (!fileName.isEmpty()) {
                status += QStringLiteral(" (%1)").arg(fileName);
            }
        }
        m_dialogsController->setUpdateDialogStatus(status, true);
    }
}

void UpdateManager::onUpdateLoaded(bool emptyUpdate)
{
    if (!emptyUpdate) {
        m_downloadState = DownloadState::Downloaded;
        if (m_updateMode == UpdateMode::Foreground || m_userCommitted) {
            applyUpdate();
        }
    }
    else {
        if (shouldShowProgressDialog() && m_dialogsController) {
            m_dialogsController->setUpdateDialogStatus("Already up to date");
            m_dialogsController->hideUpdatingDialog();
        }
        onUpdateAborted();
    }
}

void UpdateManager::onUpdateAborted()
{
    const bool shouldRestartCore = (m_updateMode == UpdateMode::Foreground);
    clearTemporaryUpdateDirectory();
    resetUpdateState();

    if (shouldRestartCore) {
        emit coreRestartRequested();
    }
}

void UpdateManager::onLoadingProgress(double progress)
{
    m_lastProgress = progress;
    if (m_dialogsController && progress > 0 && shouldShowProgressDialog()) {
        m_dialogsController->setUpdateDialogStatus(QStringLiteral("Downloading..."), false);
        m_dialogsController->setUpdateLoadingProgress(progress);
    }
}

updater::OperationSystemType UpdateManager::resolveOperationSystemType()
{
    return m_configManager->getOperationSystemType();
}

bool UpdateManager::isUpdateAlreadyDownloadedForVersion(const QString& expectedVersion) const
{
    if (!m_configManager || expectedVersion.isEmpty()) {
        return false;
    }

    const QString updateDirectoryPath = m_configManager->getTemporaryUpdateDirectoryPath();
    if (updateDirectoryPath.isEmpty()) {
        return false;
    }

    const QString updateConfigPath = QDir(updateDirectoryPath).filePath(DOWNLOADED_UPDATE_CONFIG_FILE_NAME);
    QFile updateConfigFile(updateConfigPath);
    if (!updateConfigFile.exists() || !updateConfigFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    const QByteArray jsonData = updateConfigFile.readAll();
    updateConfigFile.close();

    QJsonParseError parseError;
    const QJsonDocument jsonDocument = QJsonDocument::fromJson(jsonData, &parseError);
    if (parseError.error != QJsonParseError::NoError || !jsonDocument.isObject()) {
        return false;
    }

    const QJsonObject jsonObject = jsonDocument.object();
    const QString downloadedVersion = jsonObject[VERSION].toString();
    return !downloadedVersion.isEmpty() && downloadedVersion == expectedVersion;
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

void UpdateManager::onCommunicationSessionStarted()
{
    if (m_updateMode != UpdateMode::Background) {
        return;
    }

    if (m_dialogsController) {
        m_dialogsController->hideUpdatingDialog();
    }

    // User intent is canceled while in call/meeting to avoid forced restart.
    m_userCommitted = false;
}

bool UpdateManager::isCommunicationSessionActive() const
{
    if (!m_coreClient) {
        return false;
    }

    return m_coreClient->isActiveCall() || m_coreClient->isOutgoingCall() || m_coreClient->isInMeeting();
}

bool UpdateManager::shouldShowProgressDialog() const
{
    return m_updateMode == UpdateMode::Foreground || m_userCommitted;
}

void UpdateManager::clearTemporaryUpdateDirectory()
{
    if (!m_configManager) {
        return;
    }

    const QString updateDirectoryPath = m_configManager->getTemporaryUpdateDirectoryPath();
    if (updateDirectoryPath.isEmpty()) {
        return;
    }

    QDir updateDirectory(updateDirectoryPath);
    if (!updateDirectory.exists()) {
        return;
    }

    if (!updateDirectory.removeRecursively()) {
        LOG_WARN("Failed to remove temporary update directory: {}", updateDirectoryPath.toStdString());
    }
}

void UpdateManager::applyUpdate()
{
    if (m_updateMode == UpdateMode::Background && isCommunicationSessionActive()) {
        if (m_dialogsController) {
            m_dialogsController->hideUpdatingDialog();
        }
        // Keep downloaded update, but require explicit user action after session ends.
        m_userCommitted = false;
        if (m_mainMenuWidget && !m_pendingUpdateVersion.isEmpty()) {
            m_mainMenuWidget->setUpdateButtonVisible(true);
        }
        return;
    }

    m_shouldRestart = true;
    emit stopAllRingtonesRequested();

    if (m_mainMenuWidget) {
        m_mainMenuWidget->removeOutgoingCallPanel();
    }

    if (m_updaterClient) {
        m_updaterClient->stop();
    }

    if (m_dialogsController) {
        // For REQUIRED_UPDATE the dialog must be visible; for optional update with already-downloaded payload
        // we restart without showing a dialog. If a dialog is already open (user clicked while downloading),
        // setUpdateDialogStatus will still update it.
        if (m_updateMode == UpdateMode::Foreground) {
            m_dialogsController->showUpdatingDialog();
        }
        m_dialogsController->setUpdateDialogStatus("Restarting...");
    }

    if (m_coreClient) {
        m_coreClient->stop();
    }

    QTimer::singleShot(UPDATE_APPLIER_DELAY_MS, [this]() {
        launchUpdateApplier();
    });
}

void UpdateManager::resetUpdateState()
{
    m_shouldRestart = false;
    m_updateNeeded = false;
    m_userCommitted = false;
    m_lastProgress = 0.0;
    m_updateMode = UpdateMode::None;
    m_downloadState = DownloadState::None;
    m_pendingUpdateVersion.clear();

    if (m_mainMenuWidget) {
        m_mainMenuWidget->setUpdateButtonVisible(false);
    }
}
