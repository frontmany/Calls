#pragma once

#include <QMediaPlayer>
#include <QAudioOutput>
#include <QAudioDevice>
#include <QUrl>
#include <QString>
#include <memory>

#include "core.h"

class AudioEffectsManager : public QObject {
    Q_OBJECT

public:
    explicit AudioEffectsManager(std::shared_ptr<core::Client> client, QObject* parent = nullptr);
    ~AudioEffectsManager();

    void playIncomingCallRingtone();
    void stopIncomingCallRingtone();
    void playCallingRingtone();
    void stopCallingRingtone();
    void playCallJoinedEffect();
    void playCallingEndedEffect();
    void playEndCallEffect();

private slots:
    void onPlaybackStateChanged(QMediaPlayer::PlaybackState state);

private:
    void playRingtone(const QUrl& ringtoneUrl);
    void stopRingtone();
    void playSoundEffect(const QUrl& soundUrl);
    QAudioDevice resolveOutputDevice() const;

    std::shared_ptr<core::Client> m_coreClient = nullptr;
    QMediaPlayer* m_ringtonePlayer = nullptr;
    QAudioOutput* m_audioOutput = nullptr;
};
