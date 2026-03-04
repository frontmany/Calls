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
#include <QPixmap>
#include <QIcon>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QMap>
#include <QListWidget>
#include <QGridLayout>
#include <QScrollBar>
#include <QScrollArea>

#include "widgets/components/button.h"
#include "widgets/components/screen.h"

class QResizeEvent;
class MeetingParticipantWidget;

struct StyleMeetingWidget
{
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
    static const QColor m_participantContainerColor;
    static const QColor m_participantBorderColor;
    static const QColor m_joinRequestPanelBackgroundColor;
    static const QColor m_joinRequestPanelBorderColor;
    static const QColor m_acceptButtonColor;
    static const QColor m_acceptButtonHoverColor;
    static const QColor m_declineButtonColor;
    static const QColor m_declineButtonHoverColor;

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
    static QString participantsScrollAreaStyle();
    static QString participantsScrollBarStyle();
    static QString participantContainerStyle();
    static QString participantNameStyle();
    static QString joinRequestPanelStyle();
    static QString joinRequestItemStyle();
    static QString joinRequestScrollAreaStyle();
    static QString joinRequestScrollBarStyle();
    static QString joinRequestContentContainerStyle();
    static QString joinRequestButtonsContainerStyle();
    static QString joinRequestAcceptButtonStyle();
    static QString joinRequestDeclineButtonStyle();
    static QString callNamePanelStyle();
    static QString callNameLabelStyle();
};

class MeetingWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MeetingWidget(QWidget* parent = nullptr);
    ~MeetingWidget() = default;

    bool isMainScreenVisible() const;
    bool isAdditionalScreenVisible(const std::string& id) const;
    bool isFullScreen() const;
    void setCallName(const QString& callName);
    void setInputVolume(int newVolume);
    void setOutputVolume(int newVolume);
    void setMicrophoneMuted(bool muted);
    void setSpeakerMuted(bool muted);
    void addParticipant(const QString& nickname);
    void removeParticipant(const QString& nickname);
    void updateParticipantVideo(const QString& nickname, const QPixmap& frame);
    void clearParticipantVideo(const QString& nickname);
    void setParticipantMuted(const QString& nickname, bool muted);
    void setParticipantSpeaking(const QString& nickname, bool speaking);
    void setParticipantCameraEnabled(const QString& nickname, bool enabled);
    void clearParticipants();
    void hideMainScreen();
    void hideAdditionalScreens();
    void enterFullscreen();
    void exitFullscreen();
    void updateMainScreenSize();
    void showFrameInMainScreen(const QPixmap& frame);
    void showFrameInAdditionalScreen(const QPixmap& frame, const std::string& id);
    void removeAdditionalScreen(const std::string& id);
    void restrictScreenShareButton();
    void setScreenShareButtonActive(bool active);
    void restrictCameraButton();
    void setCameraButtonActive(bool active);
    void showEnterFullscreenButton();
    void hideEnterFullscreenButton();
    void showErrorNotification(const QString& text, int durationMs);
    void addJoinRequest(const QString& nickname);
    void removeJoinRequest(const QString& nickname);
    void setIsOwner(bool isOwner);

signals:
    void hangupClicked();
    void hangupConfirmationRequested();
    void inputVolumeChanged(int newVolume);
    void outputVolumeChanged(int newVolume);
    void muteMicrophoneClicked(bool mute);
    void muteSpeakerClicked(bool mute);
    void requestEnterFullscreen();
    void requestExitFullscreen();
    void screenShareClicked(bool toggled);
    void cameraClicked(bool toggled);
    void joinRequestAccepted(const QString& nickname);
    void joinRequestDeclined(const QString& nickname);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private slots:
    void onSlidersClicked(bool toggled);
    void updateCallTimer();
    void setupElementShadow(QWidget* widget, int blurRadius, const QColor& color);
    void onExitFullscreenHideTimerTimeout();
    void updateParticipantsLayout();
    void onPrevPageClicked();
    void onNextPageClicked();

private:
    void setupUI();
    void setupShadowEffect();
    void updateExitFullscreenButtonPosition();
    void applyStandardSize();
    void applyDecreasedSize();
    void applyExtraDecreasedSize();
    void applyFullscreenSize();
    QSize scaledScreenSize16by9(int baseWidth);
    void updateParticipantsContainerSize();
    int calculateMaxParticipantsPerRow() const;
    int calculateParticipantsPerPage() const;
    void updateParticipantPanels();
    void showPage(int pageIndex);
    void updateNavigationButtons();
    QIcon createArrowIcon(bool left, bool hover = false) const;
    void updateJoinRequestsPanelPosition();
    void updateJoinRequestsPanelSize();
    void updateCallNamePanelPosition();
    void showCallNamePanelIfAvailable();
    void scheduleCallNameHide();

    QSpacerItem* m_topMainLayoutSpacer = nullptr;
    QVBoxLayout* m_mainLayout = nullptr;

    QWidget* m_participantsContainer = nullptr;
    QHBoxLayout* m_participantsContainerLayout = nullptr;
    QList<QWidget*> m_participantPanels;
    QList<QGridLayout*> m_participantPanelLayouts;
    ButtonIcon* m_prevPageButton = nullptr;
    ButtonIcon* m_nextPageButton = nullptr;
    int m_currentPageIndex = 0;

    QLabel* m_timerLabel = nullptr;
    Screen* m_mainScreen = nullptr;

    QWidget* m_additionalScreensContainer = nullptr;
    QHBoxLayout* m_additionalScreensLayout = nullptr;
    QMap<std::string, Screen*> m_additionalScreens;

    QWidget* m_buttonsPanel = nullptr;
    QHBoxLayout* m_buttonsLayout = nullptr;
    QWidget* m_slidersContainer = nullptr;
    QVBoxLayout* m_slidersLayout = nullptr;

    QWidget* m_micSliderWidget = nullptr;
    QVBoxLayout* m_micSliderLayout = nullptr;
    QHBoxLayout* m_micLabelSliderLayout = nullptr;
    ToggleButtonIcon* m_micLabel = nullptr;
    QSlider* m_micVolumeSlider = nullptr;

    QWidget* m_speakerSliderWidget = nullptr;
    QVBoxLayout* m_speakerSliderLayout = nullptr;
    QHBoxLayout* m_speakerLabelSliderLayout = nullptr;
    ToggleButtonIcon* m_speakerLabel = nullptr;
    QSlider* m_speakerVolumeSlider = nullptr;

    ButtonIcon* m_enterFullscreenButton = nullptr;
    ButtonIcon* m_exitFullscreenButton = nullptr;
    ToggleButtonIcon* m_microphoneButton = nullptr;
    ToggleButtonIcon* m_screenShareButton = nullptr;
    ToggleButtonIcon* m_cameraButton = nullptr;
    ToggleButtonIcon* m_slidersButton = nullptr;
    QPushButton* m_hangupButton = nullptr;

    QWidget* m_joinRequestsPanel = nullptr;
    QVBoxLayout* m_joinRequestsLayout = nullptr;
    QScrollArea* m_joinRequestsScrollArea = nullptr;
    QWidget* m_joinRequestsContainer = nullptr;
    QVBoxLayout* m_joinRequestsContainerLayout = nullptr;
    QMap<QString, QWidget*> m_joinRequestItems;

    QWidget* m_callNamePanel = nullptr;
    QHBoxLayout* m_callNameLayout = nullptr;
    QLabel* m_callNameLabel = nullptr;
    ToggleButtonIcon* m_copyCallNameButton = nullptr;
    QTimer* m_callNameHideTimer = nullptr;

    QIcon m_screenShareIconNormal;
    QIcon m_screenShareIconHover;
    QIcon m_screenShareIconActive;
    QIcon m_screenShareIconActiveHover;
    QIcon m_screenShareIconRestricted;
    QIcon m_cameraIconActive;
    QIcon m_cameraIconActiveHover;
    QIcon m_cameraIconRestricted;
    QIcon m_cameraIconDisabled;
    QIcon m_cameraIconDisabledHover;
    QIcon m_fullscreenIconMaximize;
    QIcon m_fullscreenIconMaximizeHover;
    QIcon m_fullscreenIconMinimize;
    QIcon m_fullscreenIconMinimizeHover;

    QTimer* m_callTimer = nullptr;
    QTime* m_callDuration = nullptr;
    QTimer* m_exitFullscreenHideTimer = nullptr;

    bool m_slidersVisible = false;
    bool m_screenFullscreenActive = false;
    bool m_hasScreenSharing = false;

    QString m_callName;
    QMap<QString, MeetingParticipantWidget*> m_participantWidgets;
    bool m_isOwner = true;

    static constexpr int kMaxParticipantsPerRow = 3;

    QWidget* m_notificationWidget = nullptr;
    QLabel* m_notificationLabel = nullptr;
    QHBoxLayout* m_notificationLayout = nullptr;
    QTimer* m_notificationTimer = nullptr;
};
