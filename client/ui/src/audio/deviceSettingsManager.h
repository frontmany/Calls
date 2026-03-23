#pragma once

#include <QObject>
#include <memory>
#include "core.h"

class ConfigManager;

class DeviceSettingsManager : public QObject {
    Q_OBJECT

public:
    explicit DeviceSettingsManager(std::shared_ptr<core::Core> client, ConfigManager* configManager, QObject* parent = nullptr);

public slots:
    void onRefreshAudioDevicesButtonClicked();
    void onInputVolumeChanged(int newVolume);
    void onOutputVolumeChanged(int newVolume);
    void onMuteMicrophoneButtonClicked(bool mute);
    void onMuteSpeakerButtonClicked(bool mute);
    void onInputDeviceSelected(int deviceIndex);
    void onOutputDeviceSelected(int deviceIndex);
    void onCameraDeviceSelected(const QString& deviceId);

private:
    std::shared_ptr<core::Core> m_coreClient = nullptr;
    ConfigManager* m_configManager = nullptr;
};
