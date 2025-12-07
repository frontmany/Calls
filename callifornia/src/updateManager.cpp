#include "updateManager.h"
#include "authorizationWidget.h"
#include "mainMenuWidget.h"
#include "dialogsController.h"
#include "updaterCallbacksHandler.h"

UpdateManager::UpdateManager(ConfigManager* configManager, QObject* parent)
    : QObject(parent)
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
    LOG_INFO("Connecting to Callifornia server: {}:{}", m_configManager->getUpdaterHost().toStdString(), m_configManager->getPort().toStdString());
    
    std::unique_ptr<UpdaterCallbacksHandler> updaterHandler = std::make_unique<UpdaterCallbacksHandler>(this);
    updater::init(std::move(updaterHandler));
    updater::connect(m_configManager->getUpdaterHost().toStdString(), m_configManager->getPort().toStdString());
    
    std::string currentVersion = parseVersionFromConfig();
    LOG_INFO("Current application version: {}", currentVersion);
    updater::checkUpdates(currentVersion);
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
    std::string version = json[calls::VERSION].toString().toStdString();

    LOG_DEBUG("Parsed version from config: {}", version);
    return version;
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

void UpdateManager::onUpdaterCheckResult(updater::UpdatesCheckResult checkResult)
{
    if (!m_authorizationWidget) return;

    m_authorizationWidget->hideUpdatesCheckingNotification();

    if (checkResult == updater::UpdatesCheckResult::POSSIBLE_UPDATE) {
        if (!calls::isNetworkError()) {
            m_authorizationWidget->setAuthorizationDisabled(false);
        }

        if (calls::isNetworkError()) {
            m_authorizationWidget->hideNetworkErrorNotification();
        }

        m_authorizationWidget->showUpdateAvailableNotification();
        if (m_mainMenuWidget) {
            m_mainMenuWidget->showUpdateAvailableNotification();
        }
    }
    else if (checkResult == updater::UpdatesCheckResult::REQUIRED_UPDATE) {
        updater::startUpdate(resolveOperationSystemType());
        if (m_dialogsController) {
            m_dialogsController->showUpdatingDialog();
        }
    }
    else if (checkResult == updater::UpdatesCheckResult::UPDATE_NOT_NEEDED) {
        updater::disconnect();

        if (!calls::isNetworkError()) {
            m_authorizationWidget->setAuthorizationDisabled(false);

            if (!calls::isRunning()) {
                calls::run();
            }
        }
    }
    else {
        qWarning() << "error: unknown UpdatesCheckResult type";
    }
}

void UpdateManager::onUpdateButtonClicked()
{
    if (calls::isCalling()) {
        calls::stopCalling();
    }

    auto callers = calls::getCallers();
    for (const auto& nickname : callers) {
        calls::declineCall(nickname);
    }

    emit stopAllRingtonesRequested();
    
    if (m_mainMenuWidget) {
        m_mainMenuWidget->removeCallingPanel();
        m_mainMenuWidget->clearIncomingCalls();
    }

    updater::startUpdate(resolveOperationSystemType());
    if (m_dialogsController) {
        m_dialogsController->showUpdatingDialog();
    }
}

void UpdateManager::onUpdateLoaded(bool emptyUpdate)
{
    if (!emptyUpdate) {
        calls::logout();
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
        updater::disconnect();

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
