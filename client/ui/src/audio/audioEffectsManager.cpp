#include "audioEffectsManager.h"
#include <QSoundEffect>
#include <QMediaDevices>
#include "media/audio/audioEngine.h"

AudioEffectsManager::AudioEffectsManager(std::shared_ptr<core::Client> client, QObject* parent)
    : QObject(parent)
    , m_coreClient(client)
{
    m_ringtonePlayer = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);
    m_audioOutput->setVolume(0.4f);
    m_ringtonePlayer->setAudioOutput(m_audioOutput);

    connect(m_ringtonePlayer, &QMediaPlayer::playbackStateChanged, 
            this, &AudioEffectsManager::onPlaybackStateChanged);
}

AudioEffectsManager::~AudioEffectsManager()
{
}

void AudioEffectsManager::onPlaybackStateChanged(QMediaPlayer::PlaybackState state)
{
    if (state == QMediaPlayer::StoppedState) {
        if (m_coreClient && m_coreClient->getIncomingCallsCount() != 0) {
            m_ringtonePlayer->play();
        }
    }
}

void AudioEffectsManager::playRingtone(const QUrl& ringtoneUrl)
{
    if (!m_ringtonePlayer) return;

    if (m_audioOutput) {
        m_audioOutput->setDevice(resolveOutputDevice());
    }

    if (m_ringtonePlayer->playbackState() == QMediaPlayer::PlayingState) {
        m_ringtonePlayer->stop();
    }

    m_ringtonePlayer->setSource(ringtoneUrl);

    if (m_ringtonePlayer->playbackState() != QMediaPlayer::PlayingState) {
        m_ringtonePlayer->setLoops(QMediaPlayer::Infinite);
        m_ringtonePlayer->play();
    }
}

void AudioEffectsManager::stopRingtone()
{
    if (!m_ringtonePlayer) return;

    if (m_ringtonePlayer->playbackState() == QMediaPlayer::PlayingState) {
        m_ringtonePlayer->stop();
    }
}

void AudioEffectsManager::playIncomingCallRingtone()
{
    playRingtone(QUrl("qrc:/resources/incomingCallRingtone.mp3"));
}

void AudioEffectsManager::stopIncomingCallRingtone()
{
    stopRingtone();
}

void AudioEffectsManager::playCallingRingtone()
{
    playRingtone(QUrl("qrc:/resources/calling.mp3"));
}

void AudioEffectsManager::stopCallingRingtone()
{
    stopRingtone();
}

void AudioEffectsManager::playCallJoinedEffect()
{
    playSoundEffect(QUrl("qrc:/resources/callJoined.wav"));
}

void AudioEffectsManager::playCallingEndedEffect()
{
    playSoundEffect(QUrl("qrc:/resources/callingEnded.wav"));
}

void AudioEffectsManager::playEndCallEffect()
{
    playSoundEffect(QUrl("qrc:/resources/endCall.wav"));
}

void AudioEffectsManager::playSoundEffect(const QUrl& soundUrl)
{
    QSoundEffect* effect = new QSoundEffect(this);
    const QAudioDevice device = resolveOutputDevice();
    if (!device.isNull()) {
        effect->setAudioDevice(device);
    }
    effect->setSource(soundUrl);
    effect->setVolume(1.0f);
    effect->play();

    connect(effect, &QSoundEffect::playingChanged, this, [effect]() {
        if (!effect->isPlaying()) {
            effect->deleteLater();
        }
    });
}

QAudioDevice AudioEffectsManager::resolveOutputDevice() const
{
    const auto outputs = QMediaDevices::audioOutputs();
    if (outputs.isEmpty()) {
        return QAudioDevice();
    }

    if (!m_coreClient) {
        return QMediaDevices::defaultAudioOutput();
    }

    const int currentIndex = m_coreClient->getCurrentOutputDevice();
    const auto deviceInfo = core::audio::AudioEngine::getDeviceInfo(currentIndex);
    if (!deviceInfo) {
        return QMediaDevices::defaultAudioOutput();
    }

    const QString targetName = QString::fromStdString(deviceInfo->name).trimmed();
    for (const auto& dev : outputs) {
        if (dev.description().trimmed().compare(targetName, Qt::CaseInsensitive) == 0) {
            return dev;
        }
    }

    for (const auto& dev : outputs) {
        if (dev.description().contains(targetName, Qt::CaseInsensitive) ||
            targetName.contains(dev.description(), Qt::CaseInsensitive)) {
            return dev;
        }
    }

    return QMediaDevices::defaultAudioOutput();
}
