#pragma once

#include <QObject>

#include "client.h"

class ConfigManager;

class AudioSettingsManager : public QObject {
    Q_OBJECT

public:
    explicit AudioSettingsManager(callifornia::Client* client, ConfigManager* configManager, QObject* parent = nullptr);

public slots:
    void onRefreshAudioDevicesButtonClicked();
    void onInputVolumeChanged(int newVolume);
    void onOutputVolumeChanged(int newVolume);
    void onMuteMicrophoneButtonClicked(bool mute);
    void onMuteSpeakerButtonClicked(bool mute);

private:
    callifornia::Client* m_client = nullptr;
    ConfigManager* m_configManager = nullptr;
};
