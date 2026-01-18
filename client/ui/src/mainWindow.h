#pragma once
#include <QMainWindow>
#include <QWidget>
#include <QStackedLayout>
#include <QHBoxLayout>
#include <QFontDatabase>
#include <memory>

#include "updater.h"
#include "core.h"
#include "utilities/event.h"

class AuthorizationWidget;
class MainMenuWidget;
class CallWidget;
class DialogsController;
class ScreenCaptureController;
class CameraCaptureController; 
class ConfigManager;
class AudioEffectsManager;
class AudioSettingsManager;
class AudioDevicesWatcher;
class UpdateManager;
class NavigationController;
class AuthorizationManager;
class CallManager;
class ScreenSharingManager;
class CameraSharingManager;
class CoreNetworkErrorHandler;
class UpdaterNetworkErrorHandler;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow() = default;
    ~MainWindow() = default;
    void init();

protected:
    void closeEvent(QCloseEvent* event) override;
    void changeEvent(QEvent* event) override;
    void customEvent(QEvent* event) override;

private slots:
    void onWindowTitleChanged(const QString& title);
    void onWindowFullscreenRequested();
    void onWindowMaximizedRequested();
    void onStopScreenCaptureRequested();
    void onStopCameraCaptureRequested();
    void onEndCallFullscreenExitRequested(); 
    void onStopAllRingtonesRequested();

private:
    void initializeCentralStackedWidget();
    void initializeAudioManager();
    void initializeAudioDevicesWatcher();
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
    void initializeCoreNetworkErrorHandler();
    void initializeUpdaterNetworkErrorHandler();
    void initializeAuthorizationWidget();
    void initializeMainMenuWidget();
    void initializeCallWidget();
    void loadFonts();
    void applyAudioSettings();
    void connectWidgetsToManagers();

private:
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

    std::shared_ptr<core::Client> m_coreClient = nullptr;
    std::shared_ptr<updater::Client> m_updaterClient = nullptr;

    AudioEffectsManager* m_audioManager = nullptr;
    AudioDevicesWatcher* m_audioDevicesWatcher = nullptr;
    AudioSettingsManager* m_audioSettingsManager = nullptr;
    UpdateManager* m_updateManager = nullptr;
    NavigationController* m_navigationController = nullptr;
    AuthorizationManager* m_authorizationManager = nullptr;
    CallManager* m_callManager = nullptr;
    ScreenSharingManager* m_screenSharingManager = nullptr;
    CameraSharingManager* m_cameraSharingManager = nullptr;
    CoreNetworkErrorHandler* m_coreNetworkErrorHandler = nullptr;
    UpdaterNetworkErrorHandler* m_updaterNetworkErrorHandler = nullptr;
};
