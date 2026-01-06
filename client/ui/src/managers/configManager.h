#pragma once

#include <QString>

class ConfigManager
{
public:
    ConfigManager(QString configPath = "config.json");
    ~ConfigManager();
    void loadConfig();
    void saveConfig();
    void setDefaultValues();

    bool isConfigLoaded();
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

    void setVersion(const QString& version);
    void setUpdaterHost(const QString& host);
    void setSpeakerMuted(bool muted);
    void setMicrophoneMuted(bool muted);
    void setOutputVolume(int volume);
    void setInputVolume(int volume);
    void setMultiInstanceAllowed(bool allowed);
    void setPort(const QString& port);
    void setServerHost(const QString& host);
    void setCameraActive(bool active);
    void setFirstLaunch(bool firstLaunch);

private:
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
    bool isFirstLaunchFromConfig();

private:
    const QString m_configPath = "config.json";
    bool m_isConfigLoaded = false;

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
    bool m_firstLaunch;
};