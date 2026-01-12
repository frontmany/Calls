#pragma once
#include <QLabel>

class OverlayWidget;
class QGridLayout;
class QPushButton;
class QScreen;
class ScreenPreviewWidget;
class AudioSettingsDialog;

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

private slots:
    void onScreenShareButtonClicked();

private:
    QWidget* createAlreadyRunningDialog(OverlayWidget* overlay);
    QWidget* createUpdatingDialog(OverlayWidget* overlay);
    QWidget* createWaitingStatusDialog(QWidget* parent, const QString& statusText);
    QWidget* createScreenShareDialog(OverlayWidget* overlay);
    QWidget* createFirstLaunchDialog(OverlayWidget* overlay, const QString& imagePath = "", const QString& descriptionText = "");
    void refreshScreenSharePreviews();
    void handleScreenPreviewClick(int screenIndex, bool currentlySelected);
    void updateScreenShareSelectionState();

private:
    QWidget* m_parent;

    OverlayWidget* m_updatingOverlay;
    QWidget* m_updatingDialog;
	QLabel* m_updatingProgressLabel;
	QLabel* m_updatingLabel;
	QLabel* m_updatingGifLabel;

    OverlayWidget* m_waitingStatusOverlay;
    QWidget* m_waitingStatusDialog;
    QLabel* m_waitingStatusLabel;
    QLabel* m_waitingStatusGifLabel;

    OverlayWidget* m_screenShareOverlay;
    QWidget* m_screenShareDialog;
    QWidget* m_screenShareScreensContainer;
    QGridLayout* m_screenShareScreensLayout;
    QPushButton* m_screenShareButton;
    QLabel* m_screenShareStatusLabel;
    QList<ScreenPreviewWidget*> m_screenSharePreviewWidgets;
    QList<QScreen*> m_screenShareScreens;
    int m_screenShareSelectedIndex;

    OverlayWidget* m_alreadyRunningOverlay;
    QWidget* m_alreadyRunningDialog;
    QLabel* m_alreadyRunningImageLabel;
    QLabel* m_alreadyRunningTitleLabel;
    QLabel* m_alreadyRunningMessageLabel;

    OverlayWidget* m_firstLaunchOverlay;
    QWidget* m_firstLaunchDialog;
    QLabel* m_firstLaunchImageLabel;
    QLabel* m_firstLaunchDescriptionLabel;
    QPushButton* m_firstLaunchOkButton;

    OverlayWidget* m_audioSettingsOverlay = nullptr;
    AudioSettingsDialog* m_audioSettingsDialog = nullptr;
};

