#include "updateManager.h"
#include "widgets/authorizationWidget.h"
#include "widgets/mainMenuWidget.h"
#include "managers/dialogsController.h"
#include "events/updaterEventListener.h"

UpdateManager::UpdateManager(callifornia::Client* client, callifornia::updater::Updater* updater, ConfigManager* configManager, QObject* parent)
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

void UpdateManager::checkUpdates()
{
    if (!m_updater) return;
    
    LOG_INFO("Connecting to Callifornia server: {}:{}", m_configManager->getUpdaterHost().toStdString(), m_configManager->getPort().toStdString());
    
    std::shared_ptr<UpdaterEventListener> updaterHandler = std::make_shared<UpdaterEventListener>(this);
    m_updater->init(updaterHandler);
    m_updater->connect(m_configManager->getUpdaterHost().toStdString(), m_configManager->getPort().toStdString());
    
    std::string currentVersion = parseVersionFromConfig();
    LOG_INFO("Current application version: {}", currentVersion);
    m_updater->checkUpdates(currentVersion);
}

std::string UpdateManager::parseVersionFromConfig()
{
    const QString filename = "config.json";

    QString currentPath = QDir::currentPath();
    LOG_DEBUG("Current working directory: {}", currentPath.toStdString());

    QFileInfo fileInfo(filename);
    QString absolutePath = fileInfo.absoluteFilePath();
    LOG_DEBUG("Absolute file path: {}", absolutePath.toStdString());

    if (!fileInfo.exists()) {
        LOG_WARN("File does not exist: {}", absolutePath.toStdString());
        LOG_WARN("Failed to open config.json, version lost");
        return "versionLost";
    }

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        LOG_WARN("Failed to open config.json, version lost");
        LOG_WARN("File error: {}", file.errorString().toStdString());
        return "versionLost";
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (doc.isNull()) {
        LOG_ERROR("Failed to parse config.json");
        return "";
    }

    QJsonObject json = doc.object();
    std::string version = json["version"].toString().toStdString();

    LOG_DEBUG("Parsed version from config: {}", version);
    return version;
}

callifornia::updater::OperationSystemType UpdateManager::resolveOperationSystemType()
{
#if defined(Q_OS_WINDOWS)
    return callifornia::updater::OperationSystemType::WINDOWS;
#elif defined(Q_OS_LINUX)
    return callifornia::updater::OperationSystemType::LINUX;
#elif defined(Q_OS_MACOS)
    return callifornia::updater::OperationSystemType::MAC;
#else
#if defined(_WIN32)
    return callifornia::updater::OperationSystemType::WINDOWS;
#elif defined(__linux__)
    return callifornia::updater::OperationSystemType::LINUX;
#elif defined(__APPLE__)
    return callifornia::updater::OperationSystemType::MAC;
#else
    qWarning() << "Unknown operating system";
    return callifornia::updater::OperationSystemType::WINDOWS;
#endif
#endif
}

void UpdateManager::onUpdaterCheckResult(callifornia::updater::UpdateCheckResult checkResult)
{
    if (!m_authorizationWidget) return;

    m_authorizationWidget->hideUpdatesCheckingNotification();

    if (checkResult == callifornia::updater::UpdateCheckResult::possible_update) {
        // TODO: Implement network error check if needed
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
    else if (checkResult == callifornia::updater::UpdateCheckResult::required_update) {
        if (m_updater) {
            m_updater->startUpdate(resolveOperationSystemType());
        }
        if (m_dialogsController) {
            m_dialogsController->showUpdatingDialog();
        }
    }
    else if (checkResult == callifornia::updater::UpdateCheckResult::update_not_needed) {
        if (m_updater) {
            m_updater->disconnect();
        }

        // TODO: Implement network error check and run if needed
        if (m_client && !m_client->isConnectionDown()) {
            m_authorizationWidget->setAuthorizationDisabled(false);
            // TODO: Reconnect functionality - leave as stub
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
        if (m_client) {
            m_client->logout();
        }
        if (m_dialogsController) {
            m_dialogsController->swapUpdatingToRestarting();
        }
        qApp->processEvents();

        QTimer::singleShot(1500, this, [this]() {
            launchUpdateApplier();
        });
    }
    else {
        if (m_dialogsController) {
            m_dialogsController->swapUpdatingToUpToDate();
        }
        if (m_updater) {
            m_updater->disconnect();
        }

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
