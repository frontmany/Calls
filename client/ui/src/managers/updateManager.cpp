#include "updateManager.h"
#include "widgets/authorizationWidget.h"
#include "widgets/mainMenuWidget.h"
#include "managers/dialogsController.h"
#include "events/updaterEventListener.h"

#include <QProcess>
#include <QObject>
#include <QDir>
#include <QJsonObject>

#include "../utilities/logger.h"

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

        QTimer::singleShot(1500, [this]() {
            m_dialogsController->hideUpdatingDialog();
        });
    }
}

void UpdateManager::onLoadingProgress(double progress)
{
    m_dialogsController->setUpdateLoadingProgress(progress);
}

updater::OperationSystemType UpdateManager::resolveOperationSystemType()
{
#if defined(Q_OS_WINDOWS)
    return updater::OperationSystemType::WINDOWS;
#elif defined(Q_OS_LINUX)
    return updater::OperationSystemType::LINUX;
#elif defined(Q_OS_MACOS)
    return updater::OperationSystemType::MAC;
#else
#if defined(_WIN32)
    return updater::OperationSystemType::WINDOWS;
#elif defined(__linux__)
    return updater::OperationSystemType::LINUX;
#elif defined(__APPLE__)
    return updater::OperationSystemType::MAC;
#else
    qWarning() << "Unknown operating system";
    return updater::OperationSystemType::WINDOWS;
#endif
#endif
}

void UpdateManager::launchUpdateApplier()
{
    qint64 currentPid = QCoreApplication::applicationPid();

    QString appPath;
#ifdef Q_OS_LINUX
    QString appimagePath = qgetenv("APPIMAGE");
    if (!appimagePath.isEmpty()) {
        appPath = appimagePath;
    }
    else {
        appPath = QCoreApplication::applicationFilePath();
    }
#else
    appPath = QCoreApplication::applicationFilePath();
#endif

    QString updateApplierName;
#ifdef Q_OS_WIN
    updateApplierName = "update_applier.exe";
#else
    updateApplierName = "update_applier";
#endif

    QFileInfo updateSupplierFile(updateApplierName);
    if (!updateSupplierFile.exists()) {
        qWarning() << "Update applier not found:" << updateApplierName;
        return;
    }

    QStringList arguments;
    arguments << QString::number(currentPid) << appPath;

    QString workingDir = QDir::currentPath();
    QProcess::startDetached(updateApplierName, arguments, workingDir);
}