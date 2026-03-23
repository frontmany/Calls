#include "deviceSettingsManager.h"
#include "logic/configManager.h"
#include "core.h"

DeviceSettingsManager::DeviceSettingsManager(std::shared_ptr<core::Core> client, ConfigManager* configManager, QObject* parent)
    : QObject(parent)
    , m_coreClient(std::move(client))
    , m_configManager(configManager)
{
}

void DeviceSettingsManager::onRefreshAudioDevicesButtonClicked()
{
    if (m_coreClient) {
        m_coreClient->refreshAudioDevices();
    }
}

void DeviceSettingsManager::onInputVolumeChanged(int newVolume)
{
    if (m_coreClient) {
        m_coreClient->setInputVolume(newVolume);
    }
    if (m_configManager) {
        m_configManager->setInputVolume(newVolume);
    }
}

void DeviceSettingsManager::onOutputVolumeChanged(int newVolume)
{
    if (m_coreClient) {
        m_coreClient->setOutputVolume(newVolume);
    }
    if (m_configManager) {
        m_configManager->setOutputVolume(newVolume);
    }
}

void DeviceSettingsManager::onMuteMicrophoneButtonClicked(bool mute)
{
    if (m_coreClient) {
        m_coreClient->muteMicrophone(mute);
    }
    if (m_configManager) {
        m_configManager->setMicrophoneMuted(mute);
    }
}

void DeviceSettingsManager::onMuteSpeakerButtonClicked(bool mute)
{
    if (m_coreClient) {
        m_coreClient->muteSpeaker(mute);
    }
    if (m_configManager) {
        m_configManager->setSpeakerMuted(mute);
    }
}

void DeviceSettingsManager::onInputDeviceSelected(int deviceIndex)
{
    if (m_coreClient) {
        m_coreClient->setInputDevice(deviceIndex);
    }
}

void DeviceSettingsManager::onOutputDeviceSelected(int deviceIndex)
{
    if (m_coreClient) {
        m_coreClient->setOutputDevice(deviceIndex);
    }
}

void DeviceSettingsManager::onCameraDeviceSelected(const QString& deviceId)
{
    if (m_configManager) {
        m_configManager->setPreferredCameraDeviceId(deviceId);
    }

    if (!m_coreClient || !m_coreClient->isCameraSharing()) {
        return;
    }

    m_coreClient->stopCameraSharing();
    const std::string id = deviceId.toStdString();
    (void)m_coreClient->startCameraSharing(id.empty() ? std::string() : id);
}
