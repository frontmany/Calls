#include "audioSettingsManager.h"
#include "managers/configManager.h"
#include "client.h"

AudioSettingsManager::AudioSettingsManager(std::shared_ptr<callifornia::Client> client, ConfigManager* configManager, QObject* parent)
    : QObject(parent)
    , m_client(client)
    , m_configManager(configManager)
{
}

void AudioSettingsManager::onRefreshAudioDevicesButtonClicked()
{
    if (m_client) {
        m_client->refreshAudioDevices();
    }
}

void AudioSettingsManager::onInputVolumeChanged(int newVolume)
{
    if (m_client) {
        m_client->setInputVolume(newVolume);
    }
    if (m_configManager) {
        m_configManager->setInputVolume(newVolume);
    }
}

void AudioSettingsManager::onOutputVolumeChanged(int newVolume)
{
    if (m_client) {
        m_client->setOutputVolume(newVolume);
    }
    if (m_configManager) {
        m_configManager->setOutputVolume(newVolume);
    }
}

void AudioSettingsManager::onMuteMicrophoneButtonClicked(bool mute)
{
    if (m_client) {
        m_client->muteMicrophone(mute);
    }
    if (m_configManager) {
        m_configManager->setMicrophoneMuted(mute);
    }
}

void AudioSettingsManager::onMuteSpeakerButtonClicked(bool mute)
{
    if (m_client) {
        m_client->muteSpeaker(mute);
    }
    if (m_configManager) {
        m_configManager->setSpeakerMuted(mute);
    }
}
