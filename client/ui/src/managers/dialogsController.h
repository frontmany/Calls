#pragma once
#include <QObject>
#include <QWidget>
#include <QList>
#include <QString>
#include <QScreen>
#include <QPair>
#include <QMap>

class OverlayWidget;
class AudioSettingsDialog;
class UpdatingDialog;
class NotificationDialog;
class ScreenShareDialog;
class AlreadyRunningDialog;
class FirstLaunchDialog;
class IncomingCallDialog;
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
    void swapUpdatingToRestarting();
    void swapUpdatingToUpToDate();

    void showScreenShareDialog(const QList<QScreen*>& screens);
    void hideScreenShareDialog();
	
    void showNotificationDialog(const QString& statusText, bool createOverlay = true, bool isGreenStyle = false, bool isAnimation = true);
    void hideNotificationDialog();

    void showAlreadyRunningDialog();
    void hideAlreadyRunningDialog();

    void showFirstLaunchDialog(const QString& imagePath = ":/resources/welcome.jpg", const QString& descriptionText = "");
    void hideFirstLaunchDialog();

    void showAudioSettingsDialog(bool showSliders, bool micMuted, bool speakerMuted, int inputVolume, int outputVolume, int currentInputDevice = -1, int currentOutputDevice = -1);
    void hideAudioSettingsDialog();

    void showIncomingCallsDialog(const QList<QPair<QString, int>>& calls);
    void hideIncomingCallsDialog();
    void setIncomingCallButtonsEnabled(const QString& friendNickname, bool enabled);
    void removeIncomingCallFromDialog(const QString& friendNickname);
    void clearIncomingCallsDialog();

signals:
    void inputDeviceSelected(int deviceIndex);
    void outputDeviceSelected(int deviceIndex);
    void inputVolumeChanged(int volume);
    void outputVolumeChanged(int volume);
    void muteMicrophoneClicked(bool mute);
    void muteSpeakerClicked(bool mute);
    void refreshAudioDevicesRequested();

signals:
	void closeRequested();
    void screenSelected(int screenIndex);
    void screenShareDialogCancelled();
    void incomingCallAccepted(const QString& friendNickname);
    void incomingCallDeclined(const QString& friendNickname);
    void incomingCallsDialogClosed(const QList<QString>& pendingCalls);

private:
    QWidget* m_parent;

    OverlayWidget* m_updatingOverlay;
    UpdatingDialog* m_updatingDialog;

    OverlayWidget* m_notificationOverlay;
    NotificationDialog* m_notificationDialog;

    OverlayWidget* m_screenShareOverlay;
    ScreenShareDialog* m_screenShareDialog;

    OverlayWidget* m_alreadyRunningOverlay;
    AlreadyRunningDialog* m_alreadyRunningDialog;

    OverlayWidget* m_firstLaunchOverlay;
    FirstLaunchDialog* m_firstLaunchDialog;

    OverlayWidget* m_audioSettingsOverlay = nullptr;
    AudioSettingsDialog* m_audioSettingsDialog = nullptr;

    QMap<QString, IncomingCallDialog*> m_incomingCallDialogs;
};

