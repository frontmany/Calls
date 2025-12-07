#include "audioSettingsManager.h"
#include "configManager.h"
#include "calls.h"

AudioSettingsManager::AudioSettingsManager(ConfigManager* configManager, QObject* parent)
    : QObject(parent)
    , m_configManager(configManager)
{
}

void AudioSettingsManager::onRefreshAudioDevicesButtonClicked()
{
    calls::refreshAudioDevices();
}

void AudioSettingsManager::onInputVolumeChanged(int newVolume)
{
    calls::setInputVolume(newVolume);
    if (m_configManager) {
        m_configManager->setInputVolume(newVolume);
    }
}

void AudioSettingsManager::onOutputVolumeChanged(int newVolume)
{
    calls::setOutputVolume(newVolume);
    if (m_configManager) {
        m_configManager->setOutputVolume(newVolume);
    }
}

void AudioSettingsManager::onMuteMicrophoneButtonClicked(bool mute)
{
    calls::muteMicrophone(mute);
    if (m_configManager) {
        m_configManager->setMicrophoneMuted(mute);
    }
}

void AudioSettingsManager::onMuteSpeakerButtonClicked(bool mute)
{
    calls::muteSpeaker(mute);
    if (m_configManager) {
        m_configManager->setSpeakerMuted(mute);
    }
}
