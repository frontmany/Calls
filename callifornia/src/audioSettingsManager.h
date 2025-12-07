#pragma once

#include <QObject>

class ConfigManager;

class AudioSettingsManager : public QObject {
    Q_OBJECT

public:
    explicit AudioSettingsManager(ConfigManager* configManager, QObject* parent = nullptr);

public slots:
    void onRefreshAudioDevicesButtonClicked();
    void onInputVolumeChanged(int newVolume);
    void onOutputVolumeChanged(int newVolume);
    void onMuteMicrophoneButtonClicked(bool mute);
    void onMuteSpeakerButtonClicked(bool mute);

private:
    ConfigManager* m_configManager = nullptr;
};
