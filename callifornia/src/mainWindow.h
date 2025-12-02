#pragma once
#include <QMainWindow>
#include <QWidget>
#include <QStackedLayout>
#include <QHBoxLayout>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QFile>
#include <QLabel>
#include <QMovie>
#include <QDir>
#include <QProcess>
#include <QRect>
#include <QDialog>
#include <QScreen>
#include <QPixmap>
#include <memory>
#include <vector>

#include "updater.h"
#include "calls.h"

class AuthorizationWidget;
class MainMenuWidget;
class CallWidget;
class DialogsController;
class ScreenCaptureController;
class CameraCaptureController; 
class ConfigManager;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow();
    ~MainWindow();
    void init();
    void executePrerequisites();
    void checkUpdates();

protected:
    void closeEvent(QCloseEvent* event) override;
    void changeEvent(QEvent* event) override;

private slots:
    void onAuthorizationResult(calls::ErrorCode ec);
    void onStartCallingResult(calls::ErrorCode ec);
    void onAcceptCallResult(calls::ErrorCode ec, const QString& nickname);

    void onMaximumCallingTimeReached();
    void onCallingAccepted();
    void onCallingDeclined();
    void onRemoteUserEndedCall();
    void onIncomingCall(const QString& friendNicname);
    void onIncomingCallExpired(const QString& friendNickname);
    void onClientNetworkError();
    void onUpdaterNetworkError();
    void onConnectionRestored();

    void onUpdaterCheckResult(updater::UpdatesCheckResult checkResult);
    void onUpdateLoaded(bool emptyUpdate);
    void onLoadingProgress(double progress);

    void onUpdateButtonClicked();
    void onStartCallingButtonClicked(const QString& friendNickname);
    void onStopCallingButtonClicked();
    void onAcceptCallButtonClicked(const QString& friendNickname);
    void onDeclineCallButtonClicked(const QString& friendNickname);
    void onEndCallButtonClicked();
    void onAuthorizationButtonClicked(const QString& friendNickname);
    void onRefreshAudioDevicesButtonClicked();
    void onInputVolumeChanged(int newVolume);
    void onOutputVolumeChanged(int newVolume);
    void onMuteMicrophoneButtonClicked(bool mute);
    void onMuteSpeakerButtonClicked(bool mute);
    void onActivateCameraButtonClicked(bool activated);
    void onBlurAnimationFinished();

    void onScreenSelected(int screenIndex);
    void onScreenShareButtonClicked(bool toggled);
    void onScreenCaptureStarted();
    void onScreenCaptureStopped();
    void onScreenCaptured(const QPixmap& pixmap, const std::vector<unsigned char>& imageData);
    
    void onStartScreenSharingError();
    void onIncomingScreenSharingStarted();
    void onIncomingScreenSharingStopped();
    void onIncomingScreen(const std::vector<unsigned char>& data);

    void onStartCameraSharingError();
    void onIncomingCameraSharingStarted();
    void onIncomingCameraSharingStopped();
    void onIncomingCamera(const std::vector<unsigned char>& data);
    void onCameraButtonClicked(bool toggled);

    void onCameraCaptured(const QPixmap& pixmap, const std::vector<unsigned char>& imageData);
    void onCameraCaptureStarted();
    void onCameraCaptureStopped();
    void onCameraErrorOccurred(const QString& errorMessage);

    void onCallWidgetEnterFullscreenRequested();
    void onCallWidgetExitFullscreenRequested();

private:
    void switchToAuthorizationWidget();
    void switchToMainMenuWidget();
    void switchToCallWidget(const QString& friendNickname);

    void handleAcceptCallErrorNotificationAppearance();
    void handleDeclineCallErrorNotificationAppearance();
    void handleStartCallingErrorNotificationAppearance();
    void handleStopCallingErrorNotificationAppearance();
    void handleEndCallErrorNotificationAppearance();

    void setSettingsFromConfig();
    void launchUpdateApplier();
    void setupUI();
    void loadFonts();
    std::string parseVersionFromConfig();
    updater::OperationSystemType resolveOperationSystemType();
    void playRingtone(const QUrl& ringtoneUrl);
    void stopRingtone();
    void stopIncomingCallRingtone();
    void playIncomingCallRingtone();
    void playCallingRingtone();
    void stopCallingRingtone();
    void playSoundEffect(const QString& soundPath); 
    void stopLocalScreenCapture();
    void stopLocalCameraCapture();
    void showTransientStatusMessage(const QString& message, int durationMs);

private:
    QMediaPlayer* m_ringtonePlayer = nullptr;
    QAudioOutput* m_audioOutput = nullptr;

    QWidget* m_centralWidget = nullptr;
    QHBoxLayout* m_mainLayout = nullptr;
    QStackedLayout* m_stackedLayout = nullptr;

    AuthorizationWidget* m_authorizationWidget = nullptr;
    MainMenuWidget* m_mainMenuWidget = nullptr;
    CallWidget* m_callWidget = nullptr;
    DialogsController* m_dialogsController = nullptr;
    ScreenCaptureController* m_screenCaptureController = nullptr;
    CameraCaptureController* m_CameraCaptureController = nullptr;
    ConfigManager* m_configManager = nullptr;

    bool m_isCameraInAdditionalScreen = false;
    bool m_isRemoteCameraInAdditionalScreen = false;
};