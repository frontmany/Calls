#pragma once
#include <QObject>
#include <QWidget>
#include <QList>
#include <QString>
#include <QScreen>

class OverlayWidget;
class AudioSettingsDialog;
class UpdatingDialog;
class WaitingStatusDialog;
class ScreenShareDialog;
class AlreadyRunningDialog;
class FirstLaunchDialog;
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
	
	void showWaitingStatusDialog(const QString& statusText, bool createOverlay = true);
	void hideWaitingStatusDialog();

    void showAlreadyRunningDialog();
    void hideAlreadyRunningDialog();

    void showFirstLaunchDialog(const QString& imagePath = ":/resources/welcome.jpg", const QString& descriptionText = "");
    void hideFirstLaunchDialog();

    void showAudioSettingsDialog(bool showSliders, bool micMuted, bool speakerMuted, int inputVolume, int outputVolume, int currentInputDevice = -1, int currentOutputDevice = -1);
    void hideAudioSettingsDialog();

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

private:
    QWidget* m_parent;

    OverlayWidget* m_updatingOverlay;
    UpdatingDialog* m_updatingDialog;

    OverlayWidget* m_waitingStatusOverlay;
    WaitingStatusDialog* m_waitingStatusDialog;

    OverlayWidget* m_screenShareOverlay;
    ScreenShareDialog* m_screenShareDialog;

    OverlayWidget* m_alreadyRunningOverlay;
    AlreadyRunningDialog* m_alreadyRunningDialog;

    OverlayWidget* m_firstLaunchOverlay;
    FirstLaunchDialog* m_firstLaunchDialog;

    OverlayWidget* m_audioSettingsOverlay = nullptr;
    AudioSettingsDialog* m_audioSettingsDialog = nullptr;
};

