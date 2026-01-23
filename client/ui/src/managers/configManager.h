#pragma once

#include <QString>
#include <unordered_set>
#include <string>
#include "updater.h"

class ConfigManager
{
public:
    ConfigManager(QString configPath = "config.json");
    ~ConfigManager();
    void loadConfig();
    void saveConfig();
    void setDefaultValues();

    bool isFirstLaunch() const;
    bool isCameraActive() const;
    bool isSpeakerMuted() const;
    bool isMicrophoneMuted() const;
    bool isMultiInstanceAllowed() const;
    int getOutputVolume() const; // speaker
    int getInputVolume() const; // mic
    const QString& getConfigPath();
    const QString& getVersion() const;
    const QString& getPort() const;
    const QString& getServerHost() const;
    const QString& getUpdaterHost() const;
    const QString& getLogDirectoryPath() const;
    const QString& getCrashDumpDirectoryPath() const;
    const QString& getAppDirectoryPath() const;
    const QString& getTemporaryUpdateDirectoryPath() const;
    const QString& getDeletionListFileName() const;
    const std::unordered_set<std::string>& getIgnoredFilesWhileCollectingForUpdate() const;
    const std::unordered_set<std::string>& getIgnoredDirectoriesWhileCollectingForUpdate() const;
    updater::OperationSystemType getOperationSystemType() const;

    void setSpeakerMuted(bool muted);
    void setMicrophoneMuted(bool muted);
    void setOutputVolume(int volume);
    void setInputVolume(int volume);
    void setMultiInstanceAllowed(bool allowed);
    void setCameraActive(bool active);
    void setFirstLaunch(bool firstLaunch);

private:
    bool isFirstLaunchFromConfig();
    bool isCameraActiveFromConfig();
    bool isSpeakerMutedFromConfig();
    bool isMicrophoneMutedFromConfig();
    int getOutputVolumeFromConfig();
    int getInputVolumeFromConfig();
    bool isMultiInstanceAllowedFromConfig();
    QString getPortFromConfig();
    QString getServerHostFromConfig();
    QString getUpdaterHostFromConfig();
    QString getApplicationVersionFromConfig();
    QString getLogDirectoryFromConfig();
    QString getCrashDumpDirectoryFromConfig();
    QString getAppDirectoryFromConfig();
    QString getTemporaryUpdateDirectoryFromConfig();
    QString getDeletionListFileNameFromConfig();
    std::unordered_set<std::string> getIgnoredFilesWhileCollectingForUpdateFromConfig();
    std::unordered_set<std::string> getIgnoredDirectoriesWhileCollectingForUpdateFromConfig();
    updater::OperationSystemType getOperationSystemTypeFromConfig();
    QString validateAppDirectoryPath(const QString& path);

private:
    const QString m_configPath;
    bool m_isConfigLoaded = false;

    bool m_firstLaunch;
    bool m_isCameraActive;
    bool m_isSpeakerMuted;
    bool m_isMicrophoneMuted;
    bool m_isMultiInstanceAllowed;
    int m_outputVolume;
    int m_inputVolume;
    QString m_port;
    QString m_serverHost;
    QString m_updaterHost;
    QString m_version;
    QString m_logDirectory;
    QString m_crashDumpDirectory;
    QString m_appDirectory;
    QString m_temporaryUpdateDirectory;
    QString m_deletionListFileName;
    std::unordered_set<std::string> m_ignoredFilesWhileCollectingForUpdate;
    std::unordered_set<std::string> m_ignoredDirectoriesWhileCollectingForUpdate;
    updater::OperationSystemType m_operationSystemType;
};
