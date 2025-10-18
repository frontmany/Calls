#pragma once

#include <QWidget>
#include <QSlider>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>
#include <QHBoxLayout>

class ButtonIcon;
class ToggleButtonIcon;

class SettingsPanel : public QWidget {
    Q_OBJECT
public:
    SettingsPanel(QWidget* parent = nullptr);
    void setInputVolume(int volume);
    void setOutputVolume(int volume);
    void setMicrophoneMuted(bool muted);
    void setSpeakerMuted(bool muted);

private slots:
    void onMicVolumeChanged();
    void onSpeakerVolumeChanged();
    void onMicMuteClicked();
    void onSpeakerMuteClicked();

signals:
    void refreshAudioDevicesButtonClicked();
    void inputVolumeChanged(int newVolume);
    void outputVolumeChanged(int newVolume);
    void muteMicrophoneClicked(bool mute);
    void muteSpeakerClicked(bool mute);

private:
    void setupUI();
    QSlider* m_micSlider;
    QSlider* m_speakerSlider;
    QLabel* m_micValueLabel;
    QLabel* m_speakerValueLabel;
    QPushButton* m_refreshButton;
    QTimer* m_refreshCooldownTimer = nullptr;
    ButtonIcon* m_micMuteButton = nullptr;
    ToggleButtonIcon* m_muteMicrophoneButton = nullptr;
    ToggleButtonIcon* m_muteSpeakerButton = nullptr;
    bool m_isMicMuted = false;
    bool m_isSpeakerMuted = false;
    bool m_refreshEnabled = false;

    struct StyleSettingsPanel {
        static const QColor primaryColor;
        static const QColor hoverColor;
        static const QColor backgroundColor;
        static const QColor textColor;
        static const QColor containerColor;

        static QString containerStyle();
        static QString titleStyle();
        static QString sliderStyle();
        static QString refreshButtonStyle();
        static QString volumeValueStyle();
        static QString volumeLabelStyle();
        static QString settingsPanelStyle();
    };
};