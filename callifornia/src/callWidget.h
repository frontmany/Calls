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
#include <QMap>

#include "buttons.h"
#include "screen.h"

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
    static QString controlButtonStyle();
    static QString hangupButtonStyle();
    static QString panelStyle();
    static QString volumeLabelStyle();
    static QString scrollAreaStyle();
    static QString volumeSliderStyle();
    static QString sliderContainerStyle();
    static QString notificationRedLabelStyle();
};

class CallWidget : public QWidget {
    Q_OBJECT

public:
    CallWidget(QWidget* parent = nullptr);
    ~CallWidget() = default;

    bool isMainScreenVisible() const;
    bool isAdditionalScreenVisible(const std::string& id) const;
    bool isFullScreen() const;
    void setCallInfo(const QString& friendNickname);
    void setInputVolume(int newVolume);
    void setOutputVolume(int newVolume);
    void setMicrophoneMuted(bool muted);
    void setSpeakerMuted(bool muted);
    void addIncomingCall(const QString& friendNickName, int remainingTime = 32);
    void removeIncomingCall(const QString& callerName);
    void clearIncomingCalls();
    void hideMainScreen();
    void hideAdditionalScreens();
    void enterFullscreen();
    void exitFullscreen();
    void showFrameInMainScreen(const QPixmap& frame, Screen::ScaleMode scaleMode);
    void showFrameInAdditionalScreen(const QPixmap& frame, const std::string& id);
    void removeAdditionalScreen(const std::string& id);
    void restrictScreenShareButton();
    void setScreenShareButtonActive(bool active);
    void restrictCameraButton();
    void setCameraButtonActive(bool active);
    void showEnterFullscreenButton();
    void hideEnterFullscreenButton();
    void showErrorNotification(const QString& text, int durationMs);

signals:
    void hangupClicked();
    void inputVolumeChanged(int newVolume);
    void outputVolumeChanged(int newVolume);
    void muteMicrophoneClicked(bool mute);
    void muteSpeakerClicked(bool mute);
    void requestEnterFullscreen();
    void requestExitFullscreen();
    void acceptCallButtonClicked(const QString& callerName);
    void declineCallButtonClicked(const QString& callerName);
    void screenShareClicked(bool toggled);
    void cameraClicked(bool toggled);


protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

private slots:
    void onSpeakerClicked(bool toggled);
    void updateCallTimer();
    void setupElementShadow(QWidget* widget, int blurRadius, const QColor& color);
    void onIncomingCallsDialogClosed();
    void onExitFullscreenHideTimerTimeout();

private:
    void setupUI();
    void setupShadowEffect();
    void updateIncomingCallsVisibility();
    void updateIncomingCallWidths();
    void restoreIncomingCallsContainer();
    void updateExitFullscreenButtonPosition();

    void applyStandardSize();
    void applyDecreasedSize();
    void applyExtraDecreasedSize();
    void applyFullscreenSize();
    void updateMainScreenSize();

    QSize scaledScreenSize16by9(int baseWidth);

    // Spacers
    QSpacerItem* m_topMainLayoutSpacer;
    QSpacerItem* m_middleMainLayoutSpacer;
    QList<QWidget*> m_spacerWidgets;

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
    Screen* m_mainScreen;

    // Additional screens container
    QWidget* m_additionalScreensContainer;
    QHBoxLayout* m_additionalScreensLayout;
    QMap<std::string, Screen*> m_additionalScreens;

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
    ButtonIcon* m_enterFullscreenButton;
    ButtonIcon* m_exitFullscreenButton;
    ToggleButtonIcon* m_screenShareButton;
    ToggleButtonIcon* m_cameraButton;
    ToggleButtonIcon* m_speakerButton;
    QPushButton* m_hangupButton;

    // Screen share button icons
    QIcon m_screenShareIconNormal;
    QIcon m_screenShareIconHover;
    QIcon m_screenShareIconActive;
    QIcon m_screenShareIconActiveHover;
    QIcon m_screenShareIconRestricted;

    // Camera button icons
    QIcon m_cameraIconActive;
    QIcon m_cameraIconActiveHover;
    QIcon m_cameraIconRestricted;
    QIcon m_cameraIconDisabled;
    QIcon m_cameraIconDisabledHover;

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
    bool m_slidersVisible = false;
    bool m_screenFullscreenActive = false;

    // Incoming calls
    QDialog* m_incomingCallsDialog = nullptr;
    QMap<QString, IncomingCallWidget*> m_incomingCallWidgets;

    // Error notification
    QWidget* m_notificationWidget = nullptr;
    QLabel* m_notificationLabel = nullptr;
    QHBoxLayout* m_notificationLayout = nullptr;
    QTimer* m_notificationTimer = nullptr;
};