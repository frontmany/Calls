#pragma once
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>
#include <QTime>
#include <QSlider>
#include <QScrollArea>

#include "buttons.h"

class IncomingCallWidget;

struct StyleCallWidget {
    static const QColor m_primaryColor;
    static const QColor m_hoverColor;
    static const QColor m_backgroundColor;
    static const QColor m_textColor;
    static const QColor m_containerColor;
    static const QColor m_whiteColor;
    static const QColor m_controlButtonColor;
    static const QColor m_controlButtonHoverColor;
    static const QColor m_hangupButtonColor;
    static const QColor m_hangupButtonHoverColor;
    static const QColor m_sliderGrooveColor;
    static const QColor m_sliderHandleColor;
    static const QColor m_sliderSubPageColor;
    static const QColor m_volumeLabelColor;
    static const QColor m_scrollAreaBackgroundColor;

    static QString containerStyle();
    static QString titleStyle();
    static QString timerStyle();
    static QString longTimerStyle();
    static QString controlButtonStyle();
    static QString hangupButtonStyle();
    static QString panelStyle();
    static QString sliderStyle();
    static QString volumeLabelStyle();
    static QString scrollAreaStyle();
    static QString volumeSliderStyle();
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
    void addIncomingCall(const QString& friendNickName);
    void removeIncomingCall(const QString& callerName);
    void clearIncomingCalls();

signals:
    void hangupClicked();
    void refreshAudioDevicesButtonClicked();
    void inputVolumeChanged(int newVolume);
    void outputVolumeChanged(int newVolume);
    void muteButtonClicked(bool mute);
    void incomingCallAccepted(const QString& callerName);
    void incomingCallDeclined(const QString& callerName);

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
    void onRefreshCooldownFinished();
    void onIncomingCallAccepted(const QString& callerName);
    void onIncomingCallDeclined(const QString& callerName);
    void setupElementShadow(QWidget* widget, int blurRadius, const QColor& color);

private:
    void setupUI();
    void hideSliders();
    void setupShadowEffect();
    void updateIncomingCallsVisibility();
    QColor generateRandomColor(const QString& seed);

    // Main layouts
    QVBoxLayout* m_mainLayout;
    QWidget* m_mainContainer;
    QVBoxLayout* m_containerLayout;

    // Incoming calls section
    QWidget* m_incomingCallsContainer;
    QVBoxLayout* m_incomingCallsLayout;
    QScrollArea* m_incomingCallsScrollArea;
    QWidget* m_incomingCallsScrollWidget;
    QVBoxLayout* m_incomingCallsScrollLayout;

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
    QTime* m_callDuration;
    QString m_friendNickname;

    // refresh button cooldown 
    QTimer* m_refreshCooldownTimer;
    bool m_refreshEnabled;

    // States
    bool m_muted = false;
    bool m_showingMicSlider = false;
    bool m_showingSpeakerSlider = false;

    // Incoming calls management
    QMap<QString, IncomingCallWidget*> m_incomingCallWidgets;
};