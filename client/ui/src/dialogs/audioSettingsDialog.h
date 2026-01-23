#pragma once

#include <QDialog>
#include <QLabel>
#include <QSlider>
#include <QVBoxLayout>
#include <QPushButton>
#include <QScrollArea>
#include <QButtonGroup>
#include <QAbstractButton>
#include <QVariant>
#include <vector>

#include "widgets/buttons.h"
#include "utilities/utilities.h"
#include "audio/audioEngine.h"

class AudioSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AudioSettingsDialog(QWidget* parent = nullptr);

    void refreshDevices(int currentInputIndex, int currentOutputIndex);
    void setInputVolume(int volume);
    void setOutputVolume(int volume);
    void setMicrophoneMuted(bool muted);
    void setSpeakerMuted(bool muted);
    void setSlidersVisible(bool visible);

signals:
    void inputDeviceSelected(int deviceIndex);
    void outputDeviceSelected(int deviceIndex);
    void inputVolumeChanged(int volume);
    void outputVolumeChanged(int volume);
    void muteMicrophoneClicked(bool mute);
    void muteSpeakerClicked(bool mute);
    void refreshAudioDevicesRequested();
    void closeRequested();

private:
    void applyStyle();
    QString sliderStyle() const;
    QString scrollAreaStyle() const;
    void buildDeviceList(QVBoxLayout* layout, QButtonGroup* group, const std::vector<core::audio::DeviceInfo>& devices, int currentIndex, bool isInput);
    void clearLayout(QLayout* layout);
    void updateDeviceLogos(QButtonGroup* group);

    QWidget* m_container = nullptr;
    ButtonIcon* m_closeButton = nullptr;
    QScrollArea* m_inputDevicesArea = nullptr;
    QScrollArea* m_outputDevicesArea = nullptr;
    QWidget* m_inputDevicesWidget = nullptr;
    QWidget* m_outputDevicesWidget = nullptr;
    QVBoxLayout* m_inputDevicesLayout = nullptr;
    QVBoxLayout* m_outputDevicesLayout = nullptr;
    QButtonGroup* m_inputButtonsGroup = nullptr;
    QButtonGroup* m_outputButtonsGroup = nullptr;
    ToggleButtonIcon* m_micToggle = nullptr;
    ToggleButtonIcon* m_speakerToggle = nullptr;
    QWidget* m_slidersContainer = nullptr;
    QSlider* m_micSlider = nullptr;
    QSlider* m_speakerSlider = nullptr;
    bool m_blockDeviceSignals = false;
};
