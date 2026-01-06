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
    , m_client(client)
    , m_updater(updater)
    , m_configManager(configManager)
{
}

void UpdateManager::setWidgets(AuthorizationWidget* authWidget, MainMenuWidget* mainMenuWidget, DialogsController* dialogsController)
{
    m_authorizationWidget = authWidget;
    m_mainMenuWidget = mainMenuWidget;
    m_dialogsController = dialogsController;
}

void UpdateManager::onUpdaterCheckResult(updater::UpdateStatus checkResult)
{
    m_authorizationWidget->hideUpdatesCheckingNotification();

    if (checkResult == updater::UpdateStatus::possible_update) {
        if (m_client && !m_client->isConnectionDown()) {
            m_authorizationWidget->setAuthorizationDisabled(false);
        }

        if (m_client && m_client->isConnectionDown()) {
            m_authorizationWidget->hideNetworkErrorNotification();
        }

        m_authorizationWidget->showUpdateAvailableNotification();
        if (m_mainMenuWidget) {
            m_mainMenuWidget->showUpdateAvailableNotification();
        }
    }
    else if (checkResult == updater::UpdateStatus::required_update) {
        if (m_updater) {
            m_updater->startUpdate(resolveOperationSystemType());
        }
        if (m_dialogsController) {
            m_dialogsController->showUpdatingDialog();
        }
    }
    else if (checkResult == updater::UpdateStatus::update_not_needed) {
        if (m_updater) {
            m_updater->disconnect();
        }
    }
    else {
        qWarning() << "error: unknown UpdateCheckResult type";
    }
}

void UpdateManager::onUpdateButtonClicked()
{
    if (m_client && m_client->isOutgoingCall()) {
        m_client->stopOutgoingCall();
    }

    if (m_client) {
        auto callers = m_client->getCallers();
        for (const auto& nickname : callers) {
            m_client->declineCall(nickname);
        }
    }

    emit stopAllRingtonesRequested();
    
    if (m_mainMenuWidget) {
        m_mainMenuWidget->removeCallingPanel();
        m_mainMenuWidget->clearIncomingCalls();
    }

    if (m_updater) {
        m_updater->startUpdate(resolveOperationSystemType());
    }
    if (m_dialogsController) {
        m_dialogsController->showUpdatingDialog();
    }
}

void UpdateManager::onUpdateLoaded(bool emptyUpdate)
{
    if (!emptyUpdate) {
        m_client->logout();
        m_updater->disconnect();
        m_dialogsController->setUpdateDialogStatus("Restarting...");

        QTimer::singleShot(1500, this, [this]() {
            launchUpdateApplier();
        });
    }
    else {
        m_updater->disconnect();
        m_dialogsController->setUpdateDialogStatus("Already up to date");

        QTimer::singleShot(1500, this, [this]() {
            if (m_dialogsController) {
                m_dialogsController->hideUpdatingDialog();
            }
            if (m_authorizationWidget) {
                m_authorizationWidget->hideUpdatesCheckingNotification();
                m_authorizationWidget->setAuthorizationDisabled(false);
            }
        });
    }
}

void UpdateManager::onLoadingProgress(double progress)
{
    if (m_dialogsController) {
        m_dialogsController->updateLoadingProgress(progress);
    }
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
        LOG_INFO("Running as AppImage");
    }
    else {
        LOG_INFO("Running not as as AppImage");
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

void UpdateManager::onUpdateLoadingFailed()
{
    // TODO

    LOG_ERROR("Updater network error occurred");

    if (m_client->isAuthorized()) {
        
    }
    else {

    }
}