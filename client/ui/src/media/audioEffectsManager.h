#pragma once

#include <QMediaPlayer>
#include <QAudioOutput>
#include <QUrl>
#include <QString>
#include <memory>

#include "client.h"

class AudioEffectsManager : public QObject {
    Q_OBJECT

public:
    explicit AudioEffectsManager(std::shared_ptr<callifornia::Client> client, QObject* parent = nullptr);
    ~AudioEffectsManager();

    void playIncomingCallRingtone();
    void stopIncomingCallRingtone();
    void playCallingRingtone();
    void stopCallingRingtone();
    void playSoundEffect(const QString& soundPath);

private slots:
    void onPlaybackStateChanged(QMediaPlayer::PlaybackState state);

private:
    void playRingtone(const QUrl& ringtoneUrl);
    void stopRingtone();

    std::shared_ptr<callifornia::Client> m_client = nullptr;
    QMediaPlayer* m_ringtonePlayer = nullptr;
    QAudioOutput* m_audioOutput = nullptr;
};
