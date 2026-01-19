#pragma once
#include <QObject>
#include <QWidget>
#include <QList>
#include <QString>
#include <QScreen>
#include <QMap>
#include <functional>
#include "userOperationType.h"

class OverlayWidget;
class AudioSettingsDialog;
class UpdatingDialog;
class NotificationDialogBase;
class ConnectionDownDialog;
class ConnectionRestoredDialog;
class ConnectionDownWithUserDialog;
class ConnectionRestoredWithUserDialog;
class PendingOperationDialog;
class UpdateErrorDialog;
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

    void showUpdateErrorDialog();
    void hideUpdateErrorDialog();

    void showScreenShareDialog(const QList<QScreen*>& screens);
    void hideScreenShareDialog();
	
    void showConnectionDownDialog();
    void hideConnectionDownDialog();

    void showConnectionRestoredDialog();
    void hideConnectionRestoredDialog();

    void showConnectionDownWithUserDialog(const QString& statusText);
    void hideConnectionDownWithUserDialog();

    void showConnectionRestoredWithUserDialog(const QString& statusText);
    void hideConnectionRestoredWithUserDialog();

    void showPendingOperationDialog(const QString& statusText, core::UserOperationType key);
    void hidePendingOperationDialog(core::UserOperationType key);
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
    enum class ManagedNotificationType
    {
        ConnectionDownWithUser,
        PendingOperation
    };

    struct ManagedNotificationState
    {
        ManagedNotificationType type;
        bool hasKey = false;
        core::UserOperationType key = core::UserOperationType::AUTHORIZE;
        QString statusText;
    };

    void addManagedNotification(ManagedNotificationType type, bool hasKey, core::UserOperationType key, const QString& statusText);
    void removeManagedNotification(ManagedNotificationType type, bool hasKey, core::UserOperationType key);
    bool isManagedNotificationActive(ManagedNotificationType type, bool hasKey, core::UserOperationType key) const;
    void showManagedNotification(const ManagedNotificationState& state);
    void showLastManagedNotification();
    void hideActiveNotificationDialog();

    QWidget* m_parent;
    QMap<QString, IncomingCallDialog*> m_incomingCallDialogs;
    QList<ManagedNotificationState> m_managedNotificationStack;
    bool m_hasActivePendingOperationKey = false;
    core::UserOperationType m_activePendingOperationKey = core::UserOperationType::AUTHORIZE;

    OverlayWidget* m_updatingOverlay = nullptr;
    UpdatingDialog* m_updatingDialog = nullptr;

    OverlayWidget* m_connectionDownOverlay = nullptr;
    ConnectionDownDialog* m_connectionDownDialog = nullptr;

    OverlayWidget* m_connectionRestoredOverlay = nullptr;
    ConnectionRestoredDialog* m_connectionRestoredDialog = nullptr;

    OverlayWidget* m_connectionDownWithUserOverlay = nullptr;
    ConnectionDownWithUserDialog* m_connectionDownWithUserDialog = nullptr;

    OverlayWidget* m_connectionRestoredWithUserOverlay = nullptr;
    ConnectionRestoredWithUserDialog* m_connectionRestoredWithUserDialog = nullptr;

    OverlayWidget* m_updateErrorOverlay = nullptr;
    UpdateErrorDialog* m_updateErrorDialog = nullptr;

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