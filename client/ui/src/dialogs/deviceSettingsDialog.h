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
#include <QStackedWidget>
#include <QResizeEvent>
#include <vector>

#include "widgets/components/button.h"
#include "utilities/utilities.h"
#include "media/audio/audioEngine.h"
#include "media/camera/cameraDeviceInfo.h"

class DeviceSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DeviceSettingsDialog(QWidget* parent = nullptr);

    void refreshDevices(int currentInputIndex, int currentOutputIndex);
    void refreshCameraDevices(const std::vector<core::media::Camera>& cameras, const QString& selectedDeviceId);
    void setInputVolume(int volume);
    void setOutputVolume(int volume);
    void setMicrophoneMuted(bool muted);
    void setSpeakerMuted(bool muted);
    void setSlidersVisible(bool visible);

signals:
    void inputDeviceSelected(int deviceIndex);
    void outputDeviceSelected(int deviceIndex);
    void cameraDeviceSelected(const QString& deviceId);
    void inputVolumeChanged(int volume);
    void outputVolumeChanged(int volume);
    void muteMicrophoneClicked(bool mute);
    void muteSpeakerClicked(bool mute);
    void refreshAudioDevicesRequested();
    void closeRequested();

protected:
    void resizeEvent(QResizeEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void applyStyle();
    QString sliderStyle() const;
    QString scrollAreaStyle() const;
    void updateDeviceListElision();
    int elideWidthForScrollArea(const QScrollArea* area) const;
    void buildAudioDeviceList(QVBoxLayout* layout, QButtonGroup* group, QScrollArea* listArea,
        const std::vector<core::media::DeviceInfo>& devices, int currentIndex, bool isInput);
    void buildCameraDeviceList(const std::vector<core::media::Camera>& cameras, const QString& selectedDeviceId);
    void clearLayout(QLayout* layout);
    void updateRowSelectionStyle(QButtonGroup* group);
    QScrollArea* hoveredScrollAreaForObject(QObject* watched) const;
    void setScrollBarVisibleOnHover(QScrollArea* area, bool visible);

    QWidget* m_container = nullptr;
    ButtonIcon* m_closeButton = nullptr;

    QLabel* m_deviceCenterLabel = nullptr;
    QButtonGroup* m_navButtonGroup = nullptr;
    QPushButton* m_navInputButton = nullptr;
    QPushButton* m_navOutputButton = nullptr;
    QPushButton* m_navCameraButton = nullptr;
    QWidget* m_navInputRow = nullptr;
    QWidget* m_navOutputRow = nullptr;
    QWidget* m_navCameraRow = nullptr;

    QStackedWidget* m_stack = nullptr;

    QScrollArea* m_inputDevicesArea = nullptr;
    QScrollArea* m_outputDevicesArea = nullptr;
    QScrollArea* m_cameraDevicesArea = nullptr;
    QWidget* m_inputDevicesWidget = nullptr;
    QWidget* m_outputDevicesWidget = nullptr;
    QWidget* m_cameraDevicesWidget = nullptr;
    QVBoxLayout* m_inputDevicesLayout = nullptr;
    QVBoxLayout* m_outputDevicesLayout = nullptr;
    QVBoxLayout* m_cameraDevicesLayout = nullptr;
    QButtonGroup* m_inputButtonsGroup = nullptr;
    QButtonGroup* m_outputButtonsGroup = nullptr;
    QButtonGroup* m_cameraButtonsGroup = nullptr;

    QWidget* m_inputPage = nullptr;
    QWidget* m_outputPage = nullptr;
    QWidget* m_cameraPage = nullptr;

    ToggleButtonIcon* m_micToggle = nullptr;
    ToggleButtonIcon* m_speakerToggle = nullptr;
    QSlider* m_micSlider = nullptr;
    QSlider* m_speakerSlider = nullptr;

    bool m_blockDeviceSignals = false;
};
