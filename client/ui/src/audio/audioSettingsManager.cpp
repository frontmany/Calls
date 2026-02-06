#include "audioSettingsManager.h"
#include "managers/configManager.h"
#include "core.h"

AudioSettingsManager::AudioSettingsManager(std::shared_ptr<core::Client> client, ConfigManager* configManager, QObject* parent)
    : QObject(parent)
    , m_coreClient(client)
    , m_configManager(configManager)
{
}

void AudioSettingsManager::onRefreshAudioDevicesButtonClicked()
{
    if (m_coreClient) {
        m_coreClient->refreshAudioDevices();
    }
}

void AudioSettingsManager::onInputVolumeChanged(int newVolume)
{
    if (m_coreClient) {
        m_coreClient->setInputVolume(newVolume);
    }
    if (m_configManager) {
        m_configManager->setInputVolume(newVolume);
    }
}

void AudioSettingsManager::onOutputVolumeChanged(int newVolume)
{
    if (m_coreClient) {
        m_coreClient->setOutputVolume(newVolume);
    }
    if (m_configManager) {
        m_configManager->setOutputVolume(newVolume);
    }
}

void AudioSettingsManager::onMuteMicrophoneButtonClicked(bool mute)
{
    if (m_coreClient) {
        m_coreClient->muteMicrophone(mute);
    }
    if (m_configManager) {
        m_configManager->setMicrophoneMuted(mute);
    }
}

void AudioSettingsManager::onMuteSpeakerButtonClicked(bool mute)
{
    if (m_coreClient) {
        m_coreClient->muteSpeaker(mute);
    }
    if (m_configManager) {
        m_configManager->setSpeakerMuted(mute);
    }
}

void AudioSettingsManager::onInputDeviceSelected(int deviceIndex)
{
    if (m_coreClient) {
        m_coreClient->setInputDevice(deviceIndex);
    }
}

void AudioSettingsManager::onOutputDeviceSelected(int deviceIndex)
{
    if (m_coreClient) {
        m_coreClient->setOutputDevice(deviceIndex);
    }
}
