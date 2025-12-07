#pragma once
#include <QMainWindow>
#include <QWidget>
#include <QStackedLayout>
#include <QHBoxLayout>
#include <QFontDatabase>
#include <memory>

#include "updater.h"
#include "calls.h"

class AuthorizationWidget;
class MainMenuWidget;
class CallWidget;
class DialogsController;
class ScreenCaptureController;
class CameraCaptureController; 
class ConfigManager;
class AudioEffectsManager;
class AudioSettingsManager;
class UpdateManager;
class NavigationController;
class AuthorizationManager;
class CallManager;
class ScreenSharingManager;
class CameraSharingManager;
class NetworkErrorHandler;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow();
    ~MainWindow();
    void init();
    void executePrerequisites();

protected:
    void closeEvent(QCloseEvent* event) override;
    void changeEvent(QEvent* event) override;

private slots:
    // Эти слоты остаются для совместимости с ClientCallbacksHandler
    void onAuthorizationResult(calls::ErrorCode ec);
    void onStartCallingResult(calls::ErrorCode ec);
    void onAcceptCallResult(calls::ErrorCode ec, const QString& nickname);
    void onMaximumCallingTimeReached();
    void onCallingAccepted();
    void onCallingDeclined();
    void onRemoteUserEndedCall();
    void onIncomingCall(const QString& friendNickname);
    void onIncomingCallExpired(const QString& friendNickname);
    void onClientNetworkError();
    void onUpdaterNetworkError();
    void onConnectionRestored();

    // Остальные слоты делегируются менеджерам
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
    void onScreenSharingStarted();
    void onStartScreenSharingError();
    void onIncomingScreenSharingStarted();
    void onIncomingScreenSharingStopped();
    void onIncomingScreen(const std::vector<unsigned char>& data);

    void onCameraSharingStarted();
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

    // Signals from managers
    void onWindowTitleChanged(const QString& title);
    void onWindowFullscreenRequested();
    void onWindowMaximizedRequested();
    void onCallWidgetActivated(const QString& friendNickname);
    void onStopScreenCaptureRequested();
    void onStopCameraCaptureRequested();
    void onEndCallFullscreenExitRequested();
    void onStopAllRingtonesRequested();

private:
    void setupUI();
    void loadFonts();
    void setSettingsFromConfig();
    void setupManagerConnections();

private:
    // UI components
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

    // Managers
    AudioEffectsManager* m_audioManager = nullptr;
    AudioSettingsManager* m_audioSettingsManager = nullptr;
    UpdateManager* m_updateManager = nullptr;
    NavigationController* m_navigationController = nullptr;
    AuthorizationManager* m_authorizationManager = nullptr;
    CallManager* m_callManager = nullptr;
    ScreenSharingManager* m_screenSharingManager = nullptr;
    CameraSharingManager* m_cameraSharingManager = nullptr;
    NetworkErrorHandler* m_networkErrorHandler = nullptr;
};
