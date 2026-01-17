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
class NotificationDialogBase;
class ConnectionDownDialog;
class ConnectionRestoredDialog;
class PendingOperationDialog;
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
	
    void showConnectionDownDialog();
    void hideConnectionDownDialog();

    void showConnectionRestoredDialog();
    void hideConnectionRestoredDialog();

    void showPendingOperationDialog(const QString& statusText);
    void hidePendingOperationDialog();

    void showAlreadyRunningDialog();
    void hideAlreadyRunningDialog();

    void showFirstLaunchDialog(const QString& imagePath = ":/resources/welcome.jpg", const QString& descriptionText = "");
    void hideFirstLaunchDialog();

    void showAudioSettingsDialog(bool showSliders, bool micMuted, bool speakerMuted, int inputVolume, int outputVolume, int currentInputDevice = -1, int currentOutputDevice = -1);
    void hideAudioSettingsDialog();

    void showIncomingCallsDialog(const QString& friendNickname, int remainingTime);
    void hideIncomingCallsDialog(const QString& friendNickname);
    void setIncomingCallButtonsActive(const QString& friendNickname, bool active);

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
    void showNotificationDialogInternal(OverlayWidget*& overlay,
        NotificationDialogBase*& dialog,
        bool createOverlay,
        const std::function<NotificationDialogBase*(QWidget*)>& createDialog,
        const std::function<void(NotificationDialogBase*)>& updateDialog);
    void hideNotificationDialogInternal(OverlayWidget*& overlay, NotificationDialogBase*& dialog);

private:
    QWidget* m_parent;
    QMap<QString, IncomingCallDialog*> m_incomingCallDialogs;

    OverlayWidget* m_updatingOverlay = nullptr;
    UpdatingDialog* m_updatingDialog = nullptr;

    OverlayWidget* m_connectionDownOverlay = nullptr;
    ConnectionDownDialog* m_connectionDownDialog = nullptr;

    OverlayWidget* m_connectionRestoredOverlay = nullptr;
    ConnectionRestoredDialog* m_connectionRestoredDialog = nullptr;

    OverlayWidget* m_pendingOperationOverlay = nullptr;
    PendingOperationDialog* m_pendingOperationDialog = nullptr;

    OverlayWidget* m_screenShareOverlay = nullptr;
    ScreenShareDialog* m_screenShareDialog = nullptr;

    OverlayWidget* m_alreadyRunningOverlay = nullptr;
    AlreadyRunningDialog* m_alreadyRunningDialog = nullptr;

    OverlayWidget* m_firstLaunchOverlay = nullptr;
    FirstLaunchDialog* m_firstLaunchDialog = nullptr;

    OverlayWidget* m_audioSettingsOverlay = nullptr;
    AudioSettingsDialog* m_audioSettingsDialog = nullptr;
};