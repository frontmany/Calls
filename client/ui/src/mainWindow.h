#pragma once
#include <QMainWindow>
#include <QWidget>
#include <QStackedLayout>
#include <QHBoxLayout>
#include <QFontDatabase>
#include <QTimer>
#include <memory>

#include "utilities/startupEvent.h"
#include "core.h"
#include "updater.h"

namespace core {
class EventListener;
}

class AuthorizationWidget;
class MainMenuWidget;
class CallWidget;
class MeetingWidget;
 
class ConfigManager;
class AudioEffectsManager;
class DeviceSettingsManager;
class AudioDevicesWatcher;
class UpdateManager;
class NavigationController;
class AuthorizationManager;
class CallManager;
class MeetingManager;
class DialogsController;
class NotificationController;
class CoreNetworkErrorHandler;
class UpdaterNetworkErrorHandler;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow() = default;
    ~MainWindow() = default;
    void init();
    bool isFirstLaunch() const;

protected:
    void closeEvent(QCloseEvent* event) override;
    void changeEvent(QEvent* event) override;
    void customEvent(QEvent* event) override;

private slots:
    void onWindowTitleChanged(const QString& title);
    void onWindowFullscreenRequested();
    void onWindowMaximizedRequested();
    void onEndCallFullscreenExitRequested();
    void onEndMeetingFullscreenExitRequested();
    void onStopAllRingtonesRequested();
    void onCoreRestartRequested();
    void onEndMeetingRequested();

private:
    void initializeCentralStackedWidget();
    void initializeAudioManager();
    void initializeAudioDevicesWatcher();
    void initializeDeviceSettingsManager();
    void initializeUpdateManager();
    void initializeDialogsController();
    void initializeNotificationController();
    void initializeNavigationController();
    void initializeAuthorizationManager();
    void initializeCallManager();
    void initializeMeetingManager();
    void initializeCoreNetworkErrorHandler();
    void initializeUpdaterNetworkErrorHandler();
    void initializeAuthorizationWidget();
    void initializeMainMenuWidget();
    void initializeCallWidget();
    void initializeMeetingWidget();
    void initializeDiagnostics();
    void replaceUpdateApplier();
    void loadFonts();
    void applyAudioSettings();
    void connectWidgetsToManagers();
    QString getConfigFilePath() const;
    std::shared_ptr<core::EventListener> createCoreEventListener();

private:
    QWidget* m_centralWidget = nullptr;
    QHBoxLayout* m_mainLayout = nullptr;
    QStackedLayout* m_stackedLayout = nullptr;

    AuthorizationWidget* m_authorizationWidget = nullptr;
    MainMenuWidget* m_mainMenuWidget = nullptr;
    CallWidget* m_callWidget = nullptr;
    MeetingWidget* m_meetingWidget = nullptr;
    DialogsController* m_dialogsController = nullptr;
    NotificationController* m_notificationController = nullptr;
    ConfigManager* m_configManager = nullptr;

    std::shared_ptr<core::Core> m_coreClient = nullptr;
    std::shared_ptr<updater::Client> m_updaterClient = nullptr;

    AudioEffectsManager* m_audioManager = nullptr;
    AudioDevicesWatcher* m_audioDevicesWatcher = nullptr;
    DeviceSettingsManager* m_deviceSettingsManager = nullptr;
    UpdateManager* m_updateManager = nullptr;
    NavigationController* m_navigationController = nullptr;
    AuthorizationManager* m_authorizationManager = nullptr;
    CallManager* m_callManager = nullptr;
    MeetingManager* m_meetingManager = nullptr;
    CoreNetworkErrorHandler* m_coreNetworkErrorHandler = nullptr;
    UpdaterNetworkErrorHandler* m_updaterNetworkErrorHandler = nullptr;
};
