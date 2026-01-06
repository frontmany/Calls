#pragma once
#include <QMainWindow>
#include <QWidget>
#include <QStackedLayout>
#include <QHBoxLayout>
#include <QFontDatabase>
#include <memory>

#include "updater.h"
#include "core.h"

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
    MainWindow() = default;
    ~MainWindow() = default;
    void init();

protected:
    void showEvent(QShowEvent* event) override;
    void closeEvent(QCloseEvent* event) override;
    void changeEvent(QEvent* event) override;

private slots:
    // Эти слоты остаются для совместимости с ClientCallbacksHandler
    void onAuthorizationResult(std::error_code ec);
    void onStartCallingResult(std::error_code ec);
    void onAcceptCallResult(std::error_code ec, const QString& nickname);
    void onMaximumCallingTimeReached();
    void onCallingAccepted();
    void onCallingDeclined();
    void onRemoteUserEndedCall();
    void onIncomingCall(const QString& friendNickname);
    void onIncomingCallExpired(const QString& friendNickname);
    void onClientNetworkError();
    void onUpdaterNetworkError();
    void onConnectionRestored();


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
    void initializeCentralStackedWidget();
    void initializeAudioManager();
    void initializeAudioSettingsManager();
    void initializeUpdateManager();
    void initializeScreenSharingManager();
    void initializeCameraSharingManager();
    void initializeDialogsController();
    void initializeNavigationController();
    void initializeScreenCaptureController();
    void initializeCameraCaptureController();
    void initializeAuthorizationManager();
    void initializeCallManager();
    void initializeNetworkErrorHandler();
    void initializeAuthorizationWidget();
    void initializeMainMenuWidget();
    void initializeCallWidget();
    void loadFonts();
    void applyAudioSettings();
    void connectWidgetsToManagers();

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

    std::shared_ptr<core::Client> m_client = nullptr;
    std::shared_ptr<updater::Client> m_updater = nullptr;

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
