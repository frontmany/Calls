#include "audioDevicesWatcher.h"

#include <QDebug>

AudioDevicesWatcher::AudioDevicesWatcher(std::shared_ptr<core::Core> client, QObject* parent)
    : QObject(parent)
    , m_coreClient(std::move(client))
    , m_mediaDevices()
{
    connect(&m_mediaDevices, &QMediaDevices::audioInputsChanged, this, &AudioDevicesWatcher::refreshDevices);
    connect(&m_mediaDevices, &QMediaDevices::audioOutputsChanged, this, &AudioDevicesWatcher::refreshDevices);
}

void AudioDevicesWatcher::refreshDevices()
{
    if (m_coreClient) {
        m_coreClient->refreshAudioDevices();
    }
    emit devicesChanged();
}
