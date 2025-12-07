#pragma once

#include <QMediaPlayer>
#include <QAudioOutput>
#include <QUrl>
#include <QString>

class AudioEffectsManager : public QObject {
    Q_OBJECT

public:
    explicit AudioEffectsManager(QObject* parent = nullptr);
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

    QMediaPlayer* m_ringtonePlayer = nullptr;
    QAudioOutput* m_audioOutput = nullptr;
};
