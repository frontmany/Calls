#pragma once
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QSize>
#include <QHBoxLayout>
#include <QTimer>
#include <QTime>
#include <QSlider>
#include <QScrollArea>
#include <QPixmap>
#include <QIcon>
#include <QKeyEvent>
#include <QMouseEvent>

#include "buttons.h"
class QDialog;
class QResizeEvent;
class Screen;

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
    void setShowingDisplayActive(bool active, bool viewingRemoteDisplay = false);
    void showFrame(const QPixmap& frame);
    void disableStartScreenShareButton(bool disable);
    void setFullscreenButtonState(bool fullscreen);
    void resetScreenShareButton();

signals:
    void hangupClicked();
    void inputVolumeChanged(int newVolume);
    void outputVolumeChanged(int newVolume);
    void muteMicrophoneClicked(bool mute);
    void muteSpeakerClicked(bool mute);
    void acceptCallButtonClicked(const QString& callerName);
    void declineCallButtonClicked(const QString& callerName);
    void screenShareClicked(bool toggled);
    void fullscreenClicked(bool toggled);
    void requestEnterWindowFullscreen();
    void requestExitWindowFullscreen();

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private slots:
    void onFullscreenClicked(bool toggled);
    void onSpeakerClicked();
    void onHangupClicked();
    void onInputVolumeChanged(int volume);
    void onOutputVolumeChanged(int volume);
    void updateCallTimer();
    void onIncomingCallAccepted(const QString& callerName);
    void onIncomingCallDeclined(const QString& callerName);
    void setupElementShadow(QWidget* widget, int blurRadius, const QColor& color);
    void onScreenShareToggled(bool toggled);
    void onMicLabelToggled(bool toggled);
    void onSpeakerLabelToggled(bool toggled);
    void onIncomingCallsDialogClosed();
    void onExitFullscreenHideTimerTimeout();

private:
    void setupUI();
    void setupShadowEffect();
    void updateIncomingCallsVisibility();
    void updateIncomingCallWidths();
    void restoreIncomingCallsContainer();
    QPixmap cropToHorizontal(const QPixmap& pixmap);
    void applyStandardSize();
    void applyDecreasedSize();
    void applyIncreasedSize();
    QSize scaledScreenSize16by9(int baseWidth);
    void enterScreenFullscreen();
    void exitScreenFullscreen();
    void updateExitFullscreenButtonPosition();
    void refreshMainLayoutGeometry();
    void updateTopSpacerHeight();

    // Spacers
    QSpacerItem* m_topMainLayoutSpacer;
    QSpacerItem* m_middleMainLayoutSpacer;

    // Main layouts
    QVBoxLayout* m_mainLayout;

    // Incoming calls section
    QWidget* m_incomingCallsContainer;
    QVBoxLayout* m_incomingCallsLayout;
    QScrollArea* m_incomingCallsScrollArea;
    QWidget* m_incomingCallsScrollWidget;
    QVBoxLayout* m_incomingCallsScrollLayout;

    // Call info section
    QLabel* m_timerLabel;
    QLabel* m_friendNicknameLabel;
    Screen* m_screenWidget;

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
    ToggleButtonIcon* m_fullscreenButton;
    ToggleButtonIcon* m_screenShareButton;
    ToggleButtonIcon* m_speakerButton;
    QPushButton* m_hangupButton;
    QPushButton* m_exitFullscreenButton;

    // Screen share button icons
    QIcon m_screenShareIconNormal;
    QIcon m_screenShareIconHover;
    QIcon m_screenShareIconActive;
    QIcon m_screenShareIconActiveHover;
    QIcon m_screenShareIconDisabled;

    // Fullscreen button icons
    QIcon m_fullscreenIconMaximize;
    QIcon m_fullscreenIconMaximizeHover;
    QIcon m_fullscreenIconMinimize;
    QIcon m_fullscreenIconMinimizeHover;

    // Timer
    QTimer* m_callTimer;
    QTime* m_callDuration;
    QTimer* m_exitFullscreenHideTimer;
    QString m_friendNickname;

    // States
    bool m_microphoneMuted = false;
    bool m_audioMuted = false;
    bool m_slidersVisible = false;
    bool m_showingDisplay = false;
    bool m_screenFullscreenActive = false;
    bool m_viewingRemoteDisplay = false;
    QList<QWidget*> m_spacerWidgets;

    // Incoming calls
    QDialog* m_incomingCallsDialog = nullptr;
    QMap<QString, IncomingCallWidget*> m_incomingCallWidgets;
};