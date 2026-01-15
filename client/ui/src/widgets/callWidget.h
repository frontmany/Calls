#pragma once
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QSize>
#include <QHBoxLayout>
#include <QTimer>
#include <QTime>
#include <QPixmap>
#include <QIcon>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QMap>

#include "buttons.h"
#include "screen.h"

class QResizeEvent;
class QShowEvent;
class Screen;

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
    static QString disabledHangupButtonStyle();
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
    void setScreenShareButtonActive(bool active);
    void setCameraButtonActive(bool active);
    void setHangupButtonRestricted(bool restricted);
    void setScreenShareButtonRestricted(bool restricted);
    void setCameraButtonRestricted(bool restricted);

    void hideMainScreen();
    void hideAdditionalScreens();
    void enterFullscreen();
    void exitFullscreen();
    void updateMainScreenSize();
    void showFrameInMainScreen(const QPixmap& frame, Screen::ScaleMode scaleMode);
    void showFrameInAdditionalScreen(const QPixmap& frame, const std::string& id);
    void removeAdditionalScreen(const std::string& id);
    void showEnterFullscreenButton();
    void hideEnterFullscreenButton();
    void showErrorNotification(const QString& text, int durationMs);

signals:
    void hangupClicked();
    void inputVolumeChanged(int newVolume);
    void outputVolumeChanged(int newVolume);
    void muteMicrophoneClicked(bool mute);
    void muteSpeakerClicked(bool mute);
    void audioSettingsRequested(bool micMuted, bool speakerMuted, int inputVolume, int outputVolume);
    void requestEnterFullscreen();
    void requestExitFullscreen();
    void screenShareClicked(bool toggled);
    void cameraClicked(bool toggled);


protected:
    void paintEvent(QPaintEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private slots:
    void updateCallTimer();
    void setupElementShadow(QWidget* widget, int blurRadius, const QColor& color);
    void onExitFullscreenHideTimerTimeout();

private:
    void setupUI();
    void setupShadowEffect();
    void updateExitFullscreenButtonPosition();
    void updateOverlayButtonsPosition();
    void updateParticipantConnectionErrorBannerPosition();
    void showOverlayButtonWithTimeout();

    void applyStandardSize();
    void applyDecreasedSize();
    void applyFullscreenSize();

    QSize scaledScreenSize16by9(int baseWidth);

    // Spacers
    QSpacerItem* m_topMainLayoutSpacer;
    QSpacerItem* m_middleMainLayoutSpacer;
    QList<QWidget*> m_spacerWidgets;

    // Main layouts
    QVBoxLayout* m_mainLayout;

    // Call info section
    QLabel* m_timerLabel;
    QWidget* m_participantInfoContainer;
    QHBoxLayout* m_participantInfoLayout;
    QLabel* m_friendNicknameLabel;
    QLabel* m_connectionErrorLabel;
    QWidget* m_participantConnectionErrorBanner;
    QLabel* m_participantConnectionErrorBannerLabel;
    Screen* m_mainScreen;

    // Additional screens container
    QWidget* m_additionalScreensContainer;
    QHBoxLayout* m_additionalScreensLayout;
    QMap<std::string, Screen*> m_additionalScreens;

    // Control panels
    QWidget* m_buttonsPanel;
    QHBoxLayout* m_buttonsLayout;

    // Buttons
    ButtonIcon* m_enterFullscreenButton;
    ButtonIcon* m_exitFullscreenButton;
    ButtonIcon* m_settingsButton;
    ToggleButtonIcon* m_microphoneButton;
    ToggleButtonIcon* m_screenShareButton;
    ToggleButtonIcon* m_cameraButton;
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
    QTimer* m_overlayButtonHideTimer;
    QTimer* m_notificationTimer = nullptr;

    // States
    QString m_friendNickname;
    bool m_screenFullscreenActive = false;
    bool m_microphoneMuted = false;
    bool m_speakerMuted = false;
    int m_inputVolume = 100;
    int m_outputVolume = 100;

    // Error notification
    QWidget* m_notificationWidget = nullptr;
    QLabel* m_notificationLabel = nullptr;
    QHBoxLayout* m_notificationLayout = nullptr;
};