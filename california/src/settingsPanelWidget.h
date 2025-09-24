#pragma once

#include <QWidget>
#include <QSlider>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>

class ButtonIcon;

class SettingsPanel : public QWidget {
    Q_OBJECT
public:
    SettingsPanel(QWidget* parent = nullptr);

private slots:
    void onMicVolumeChanged(int volume);
    void onSpeakerVolumeChanged(int volume);
    void onMicMuteClicked();

private:
    void setupUI();
    QSlider* m_micSlider;
    QSlider* m_speakerSlider;
    QLabel* m_micValueLabel;
    QLabel* m_speakerValueLabel;
    QPushButton* m_refreshButton;
    ButtonIcon* m_micMuteButton = nullptr;
    QPushButton* m_muteButton = nullptr;
    bool m_isMicMuted = false;
    int m_micVolume = 50;

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