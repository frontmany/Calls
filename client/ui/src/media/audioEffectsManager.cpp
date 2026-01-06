#include "audioEffectsManager.h"
#include <QSoundEffect>
#include "client.h"

AudioEffectsManager::AudioEffectsManager(std::shared_ptr<callifornia::Client> client, QObject* parent)
    : QObject(parent)
    , m_client(client)
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
        if (m_client && m_client->getIncomingCallsCount() != 0) {
            m_ringtonePlayer->play();
        }
    }
}

void AudioEffectsManager::playRingtone(const QUrl& ringtoneUrl)
{
    if (!m_ringtonePlayer) return;

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

void AudioEffectsManager::playSoundEffect(const QString& soundPath)
{
    QSoundEffect* effect = new QSoundEffect(this);
    effect->setSource(QUrl::fromLocalFile(soundPath));
    effect->setVolume(1.0f);
    effect->play();

    connect(effect, &QSoundEffect::playingChanged, this, [effect]() {
        if (!effect->isPlaying()) {
            effect->deleteLater();
        }
    });
}
