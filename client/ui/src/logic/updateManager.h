#pragma once

#include <QObject>
#include <QString>
#include <memory>

#include "updater.h"
#include "logic/configManager.h"
#include "core.h"

class AuthorizationWidget;
class MainMenuWidget;
class DialogsController;

class UpdateManager : public QObject {
    Q_OBJECT

public:
    enum class UpdateMode {
        None,
        Background,
        Foreground
    };

    enum class DownloadState {
        None,
        Downloading,
        Downloaded
    };

    explicit UpdateManager(std::shared_ptr<core::Core> client, std::shared_ptr<updater::Client> updater, ConfigManager* configManager, QObject* parent = nullptr);
    void setWidgets(AuthorizationWidget* authWidget, MainMenuWidget* mainMenuWidget, DialogsController* dialogsController);
    bool shouldRestart();
    bool isUpdateNeeded() const;
    void launchUpdateApplier();
    void showUpdateButtonIfNeeded();
    void resetUpdateAvailability();

signals:
    void stopAllRingtonesRequested();
    void coreRestartRequested();

public slots:
    void onUpdateCheckResult(updater::CheckResult result, const QString& newVersion = QString());
    void onUpdateLoaded(bool emptyUpdate);
    void onLoadingProgress(double progress);
    void onManifestProgress(qulonglong filesProcessed, qulonglong totalFiles, const QString& currentFilePath);
    void onUpdateButtonClicked();
    void onUpdateAborted();
    void onCommunicationSessionStarted();

private:
    updater::OperationSystemType resolveOperationSystemType();
    bool isUpdateAlreadyDownloadedForVersion(const QString& expectedVersion) const;
    bool isCommunicationSessionActive() const;
    bool shouldShowProgressDialog() const;
    void clearTemporaryUpdateDirectory();
    void applyUpdate();
    void resetUpdateState();

private:
    bool m_shouldRestart = false;
    bool m_updateNeeded = false;
    bool m_userCommitted = false;
    double m_lastProgress = 0.0;
    UpdateMode m_updateMode = UpdateMode::None;
    DownloadState m_downloadState = DownloadState::None;
    QString m_pendingUpdateVersion;

    std::shared_ptr<core::Core> m_coreClient = nullptr;
    std::shared_ptr<updater::Client> m_updaterClient = nullptr;

    ConfigManager* m_configManager = nullptr; 
    AuthorizationWidget* m_authorizationWidget = nullptr;
    MainMenuWidget* m_mainMenuWidget = nullptr;
    DialogsController* m_dialogsController = nullptr;
};