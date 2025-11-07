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
#include <QPixmap>
#include <string>

#include "buttons.h"
class ScreenCaptureController;

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
    static const QColor m_sliderContainerColor;

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
    static QString notificationRedLabelStyle();
    static QString sliderContainerStyle();
};

class CallWidget : public QWidget {
    Q_OBJECT

public:
    CallWidget(QWidget* parent = nullptr);
    ~CallWidget() = default;

    void setCallInfo(const QString& friendNickname);
    void updateTimer();
    void setInputVolume(int newVolume);
    void setOutputVolume(int newVolume);
    void setMicrophoneMuted(bool muted);
    void setSpeakerMuted(bool muted);
    void setAudioMuted(bool muted);
    void addIncomingCall(const QString& friendNickName, int remainingTime = 32);
    void removeIncomingCall(const QString& callerName);
    void clearIncomingCalls();
    void showErrorNotification(const QString& text, int durationMs);
    void setRemoteSharingActive(bool active);
    void showRemoteShareFrame(const QPixmap& frame);

signals:
    void hangupClicked();
    void inputVolumeChanged(int newVolume);
    void outputVolumeChanged(int newVolume);
    void muteMicrophoneClicked(bool mute);
    void muteSpeakerClicked(bool mute);
    void acceptCallButtonClicked(const QString& callerName);
    void declineCallButtonClicked(const QString& callerName);
    void shareScreenClicked();
    void shareScreenStopped();

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void onMuteMicrophoneClicked();
    void onSpeakerClicked();
    void onHangupClicked();
    void onInputVolumeChanged();
    void onOutputVolumeChanged();
    void updateCallTimer();
    void onIncomingCallAccepted(const QString& callerName);
    void onIncomingCallDeclined(const QString& callerName);
    void setupElementShadow(QWidget* widget, int blurRadius, const QColor& color);
    void onScreenShareToggled();
    void onCaptureStarted();
    void onCaptureStopped();
    void onScreenCaptured(const QPixmap& pixmap, const std::string& imageData);
    void onMicLabelToggled(bool toggled);
    void onSpeakerLabelToggled(bool toggled);

private:
    void setupUI();
    void updateSlidersVisibility();
    void setupShadowEffect();
    void updateIncomingCallsVisibility();
    QColor generateRandomColor(const QString& seed);
    QPixmap cropToHorizontal(const QPixmap& pixmap);
    void updateShareDisplayVisibility();

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
    QLabel* m_shareDisplayLabel;

    // Control panels
    QWidget* m_buttonsPanel;
    QHBoxLayout* m_buttonsLayout;
    QWidget* m_slidersContainer;
    QVBoxLayout* m_slidersLayout;

    // Volume sliders
    QWidget* m_micSliderWidget;
    QVBoxLayout* m_micSliderLayout;
    QHBoxLayout* m_micLabelSliderLayout;
    ToggleButtonIcon* m_micLabel;
    QSlider* m_micVolumeSlider;

    QWidget* m_speakerSliderWidget;
    QVBoxLayout* m_speakerSliderLayout;
    QHBoxLayout* m_speakerLabelSliderLayout;
    ToggleButtonIcon* m_speakerLabel;
    QSlider* m_speakerVolumeSlider;

    // Buttons
    ToggleButtonIcon* m_muteMicrophoneButton;
    ToggleButtonIcon* m_screenShareButton;
    ToggleButtonIcon* m_speakerButton;
    QPushButton* m_hangupButton;

    // Timer
    QTimer* m_callTimer;
    QTime* m_callDuration;
    QString m_friendNickname;

    QWidget* m_notificationWidget;
    QHBoxLayout* m_notificationLayout;
    QLabel* m_notificationLabel;
    QTimer* m_notificationTimer;

    // States
    bool m_microphoneMuted = false;
    bool m_audioMuted = false;
    bool m_slidersVisible = false;
    bool m_localSharingActive = false;
    bool m_remoteSharingActive = false;

    // Screen sharing
    ScreenCaptureController* m_screenCaptureController = nullptr;

    // Incoming calls management
    QMap<QString, IncomingCallWidget*> m_incomingCallWidgets;
};