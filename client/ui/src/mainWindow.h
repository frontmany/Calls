#pragma once
#include <QMainWindow>
#include <QWidget>
#include <QStackedLayout>
#include <QHBoxLayout>
#include <QFontDatabase>
#include <memory>

#include "utilities/event.h"

class AuthorizationWidget;
class MainMenuWidget;
class CallWidget;
 
class ConfigManager;
class AudioEffectsManager;
class AudioSettingsManager;
class AudioDevicesWatcher;
class UpdateManager;
class NavigationController;
class AuthorizationManager;
class CallManager;
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
    void onEndCallFullscreenExitRequested(); 
    void onStopAllRingtonesRequested();

private:
    void initializeCentralStackedWidget();
    void initializeAudioManager();
    void initializeAudioDevicesWatcher();
    void initializeAudioSettingsManager();
    void initializeUpdateManager();
    void initializeDialogsController();
    void initializeNotificationController();
    void initializeNavigationController();
    void initializeAuthorizationManager();
    void initializeCallManager();
    void initializeCoreNetworkErrorHandler();
    void initializeUpdaterNetworkErrorHandler();
    void initializeAuthorizationWidget();
    void initializeMainMenuWidget();
    void initializeCallWidget();
    void initializeDiagnostics();
    void replaceUpdateApplier();
    void loadFonts();
    void applyAudioSettings();
    void connectWidgetsToManagers();
    QString getConfigFilePath() const;

private:
    QWidget* m_centralWidget = nullptr;
    QHBoxLayout* m_mainLayout = nullptr;
    QStackedLayout* m_stackedLayout = nullptr;

    AuthorizationWidget* m_authorizationWidget = nullptr;
    MainMenuWidget* m_mainMenuWidget = nullptr;
    CallWidget* m_callWidget = nullptr;
    DialogsController* m_dialogsController = nullptr;
    NotificationController* m_notificationController = nullptr;
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
    CoreNetworkErrorHandler* m_coreNetworkErrorHandler = nullptr;
    UpdaterNetworkErrorHandler* m_updaterNetworkErrorHandler = nullptr;
};
