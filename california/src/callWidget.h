#pragma once
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>
#include <QTime>
#include <QSlider>

#include "buttons.h"
#include "calls.h"

struct StyleCallWidget {
    static const QColor m_primaryColor;
    static const QColor m_hoverColor;
    static const QColor m_backgroundColor;
    static const QColor m_textColor;
    static const QColor m_containerColor;

    static QString containerStyle();
    static QString titleStyle();
    static QString timerStyle();
    static QString controlButtonStyle();
    static QString hangupButtonStyle();
    static QString panelStyle();
    static QString sliderStyle();
    static QString volumeLabelStyle();
    static QString longTimerStyle();
};

class CallWidget : public QWidget {
    Q_OBJECT

public:
    CallWidget(QWidget* parent = nullptr);
    ~CallWidget() = default;

    void setCallInfo(const QString& friendNickname);
    void updateTimer();
    void setInputVolume(int volume);
    void setOutputVolume(int volume);
    void setMuted(bool muted);

signals:
    void hangupClicked();

protected:
    void paintEvent(QPaintEvent* event) override;

private slots:
    void onMuteClicked();
    void onHangupClicked();
    void onRefreshAudioDevicesClicked();
    void onInputVolumeChanged(int volume);
    void onOutputVolumeChanged(int volume);
    void updateCallTimer();
    void showMicSlider();
    void showSpeakerSlider();

private:
    void setupUI();
    void hideSliders();
    QColor generateRandomColor(const QString& seed);

    // Main layouts
    QVBoxLayout* m_mainLayout;
    QWidget* m_mainContainer;
    QVBoxLayout* m_containerLayout;

    // Call info section
    QLabel* m_timerLabel;
    QLabel* m_friendNicknameLabel;

    // Control panels
    QWidget* m_buttonsPanel;
    QHBoxLayout* m_buttonsLayout;
    QWidget* m_slidersPanel;
    QHBoxLayout* m_slidersLayout;

    // Volume sliders (initially hidden)
    QWidget* m_micSliderWidget;
    QVBoxLayout* m_micSliderLayout;
    QHBoxLayout* m_micLabelSliderLayout;
    ButtonIcon* m_micLabel;
    QSlider* m_micVolumeSlider;

    QWidget* m_speakerSliderWidget;
    QVBoxLayout* m_speakerSliderLayout;
    QHBoxLayout* m_speakerLabelSliderLayout;
    ButtonIcon* m_speakerLabel;
    QSlider* m_speakerVolumeSlider;

    // Buttons
    ButtonIcon* m_micButton;
    ToggleButtonIcon* m_muteButton;
    ButtonIcon* m_speakerButton;
    QPushButton* m_hangupButton;
    ButtonIcon* m_refreshButton;

    // Timer
    QTimer* m_callTimer;
    QTime m_callDuration;
    QString m_friendNickname;

    // States
    bool m_muted = false;
    bool m_showingMicSlider = false;
    bool m_showingSpeakerSlider = false;
};