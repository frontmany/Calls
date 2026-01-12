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
    void setCameraActive(bool active);

private slots:
    void onMicVolumeChanged(int volume);
    void onSpeakerVolumeChanged(int volume);
    void onMicMuteClicked();
    void onSpeakerMuteClicked();
    void onCameraButtonClicked(bool enabled);

signals:
    void audioDevicePickerRequested();
    void inputVolumeChanged(int newVolume);
    void outputVolumeChanged(int newVolume);
    void muteMicrophoneClicked(bool mute);
    void muteSpeakerClicked(bool mute);
    void cameraButtonClicked(bool mute);

private:
    void setupUI();
    QSlider* m_micSlider;
    QSlider* m_speakerSlider;
    QLabel* m_micValueLabel;
    QLabel* m_speakerValueLabel;
    QPushButton* m_devicePickerButton;
    ToggleButtonIcon* m_cameraButton = nullptr;
    ToggleButtonIcon* m_micMuteButton = nullptr;
    ToggleButtonIcon* m_speakerMuteButton = nullptr;
    bool m_isMicMuted = false;
    bool m_isSpeakerMuted = false;
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