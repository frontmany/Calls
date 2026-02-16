#pragma once
#include <QObject>
#include <QWidget>
#include <QList>
#include <QString>
#include <QScreen>
#include <QMap>
#include <functional>

class OverlayWidget;
class AudioSettingsDialog;
class UpdatingDialog;
class ScreenShareDialog;
class AlreadyRunningDialog;
class FirstLaunchDialog;
class IncomingCallDialog;
class UpdateAvailableDialog;

namespace updater {
    class Client;
}
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

    void showAudioSettingsDialog(bool showSliders, bool micMuted, bool speakerMuted, int inputVolume, int outputVolume, int currentInputDevice = -1, int currentOutputDevice = -1);
    void hideAudioSettingsDialog();
    void refreshAudioSettingsDialogDevices(int currentInputIndex, int currentOutputIndex);

    void showIncomingCallsDialog(const QString& friendNickname, int remainingTime);
    void hideIncomingCallsDialog(const QString& friendNickname);
    void setIncomingCallButtonsActive(const QString& friendNickname, bool active);

    void showUpdateAvailableDialog(const QString& newVersion = QString());
    void hideUpdateAvailableDialog();
    void hideUpdateAvailableDialogTemporarily();
    void showUpdateAvailableDialogTemporarilyHidden();
    void setUpdateClient(std::shared_ptr<updater::Client> updaterClient);

signals:
    void audioSettingsDialogClosed();
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
    void updateButtonClicked();

private:
    QWidget* m_parent;
    QMap<QString, IncomingCallDialog*> m_incomingCallDialogs;

    OverlayWidget* m_updatingOverlay = nullptr;
    UpdatingDialog* m_updatingDialog = nullptr;

    OverlayWidget* m_screenShareOverlay = nullptr;
    ScreenShareDialog* m_screenShareDialog = nullptr;

    OverlayWidget* m_alreadyRunningOverlay = nullptr;
    AlreadyRunningDialog* m_alreadyRunningDialog = nullptr;

    OverlayWidget* m_firstLaunchOverlay = nullptr;
    FirstLaunchDialog* m_firstLaunchDialog = nullptr;

    OverlayWidget* m_audioSettingsOverlay = nullptr;
    AudioSettingsDialog* m_audioSettingsDialog = nullptr;

    OverlayWidget* m_updateAvailableOverlay = nullptr;
    UpdateAvailableDialog* m_updateAvailableDialog = nullptr;

    std::shared_ptr<updater::Client> m_updaterClient = nullptr;
};
