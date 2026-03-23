#pragma once
#include <QObject>
#include <QWidget>
#include <QList>
#include <QString>
#include <QScreen>
#include <QMap>
#include <functional>
#include <QTimer>
#include <vector>

#include "media/camera/cameraDeviceInfo.h"

class OverlayWidget;
class DeviceSettingsDialog;
class UpdatingDialog;
class ScreenShareDialog;
class AlreadyRunningDialog;
class FirstLaunchDialog;
class IncomingCallDialog;
class MeetingManagementDialog;
class EndMeetingConfirmationDialog;
class DialogsController : public QObject
{
	Q_OBJECT

public:
	explicit DialogsController(QWidget* parent);
	~DialogsController();

    void showUpdatingDialog();
    void hideUpdatingDialog();
    void setUpdateLoadingProgress(double progress);
    void setUpdateDialogStatus(const QString& statusText, bool hideProgress = true);

    void showScreenShareDialog(const QList<QScreen*>& screens);
    void hideScreenShareDialog();
    
    void showCameraShareDialog();
    void hideCameraShareDialog();

    void showAlreadyRunningDialog();
    void hideAlreadyRunningDialog();

    void showFirstLaunchDialog(const QString& imagePath = ":/resources/welcome.jpg", const QString& descriptionText = "");
    void hideFirstLaunchDialog();

    void showDeviceSettingsDialog(bool showSliders, bool micMuted, bool speakerMuted, int inputVolume, int outputVolume,
        int currentInputDevice, int currentOutputDevice,
        const std::vector<core::media::Camera>& cameras, const QString& currentCameraDeviceId);
    void hideDeviceSettingsDialog();
    void refreshDeviceSettingsDialog(int currentInputIndex, int currentOutputIndex,
        const std::vector<core::media::Camera>& cameras, const QString& currentCameraDeviceId);

    void showIncomingCallsDialog(const QString& friendNickname, int remainingTime);
    void hideIncomingCallsDialog(const QString& friendNickname);
    void setIncomingCallButtonsActive(const QString& friendNickname, bool active);

    void showMeetingsManagementDialog();
    void hideMeetingsManagementDialog();
    void showMeetingsConnectingState(const QString& roomId);
    void setMeetingsJoinStatus(const QString& status);
    void resetMeetingsJoinRequestUI();

    void showEndMeetingConfirmationDialog();
    void hideEndMeetingConfirmationDialog();

signals:
    void deviceSettingsDialogClosed();
    void inputDeviceSelected(int deviceIndex);
    void outputDeviceSelected(int deviceIndex);
    void cameraDeviceSelected(const QString& deviceId);
    void inputVolumeChanged(int volume);
    void outputVolumeChanged(int volume);
    void muteMicrophoneClicked(bool mute);
    void muteSpeakerClicked(bool mute);
    void refreshAudioDevicesRequested();
	void closeRequested();
    void screenSelected(int screenIndex);
    void screenShareDialogCancelled();
    void incomingCallAccepted(const QString& friendNickname);
    void incomingCallDeclined(const QString& friendNickname);
    void incomingCallsDialogClosed(const QList<QString>& pendingCalls);
    void meetingCreateRequested();
    void meetingJoinRequested(const QString& uid);
    void meetingJoinCancelled();
    void endMeetingConfirmed();
    void endMeetingCancelled();

private:
    QWidget* m_parent;
    QMap<QString, IncomingCallDialog*> m_incomingCallDialogs;
    QTimer m_meetingsJoinWaitingTimer;
    QString m_meetingsJoinWaitingRoomId;

    OverlayWidget* m_updatingOverlay = nullptr;
    UpdatingDialog* m_updatingDialog = nullptr;

    OverlayWidget* m_screenShareOverlay = nullptr;
    ScreenShareDialog* m_screenShareDialog = nullptr;

    OverlayWidget* m_alreadyRunningOverlay = nullptr;
    AlreadyRunningDialog* m_alreadyRunningDialog = nullptr;

    OverlayWidget* m_firstLaunchOverlay = nullptr;
    FirstLaunchDialog* m_firstLaunchDialog = nullptr;

    OverlayWidget* m_deviceSettingsOverlay = nullptr;
    DeviceSettingsDialog* m_deviceSettingsDialog = nullptr;

    OverlayWidget* m_meetingsManagementOverlay = nullptr;
    MeetingManagementDialog* m_meetingsManagementDialog = nullptr;

    OverlayWidget* m_endMeetingConfirmationOverlay = nullptr;
    EndMeetingConfirmationDialog* m_endMeetingConfirmationDialog = nullptr;

};
