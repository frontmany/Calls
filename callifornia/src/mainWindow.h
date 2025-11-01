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
#include <QProcess>
#include <QDialog>
#include <QScreen>

#include "updater.h"
#include "calls.h"

class AuthorizationWidget;
class MainMenuWidget;
class CallWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent);
    ~MainWindow();
    void init();
    void connectCallifornia(const std::string& host, const std::string& port);

protected:
    void closeEvent(QCloseEvent* event) override;

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
    void onBlurAnimationFinished();

private:
    void showConnectionErrorDialog();
    void hideUpdatingErrorDialog();
    void showUpdatingDialog();
    void hideUpdatingDialog();
    void switchToAuthorizationWidget();
    void switchToMainMenuWidget();
    void switchToCallWidget(const QString& friendNickname);

    void handleAcceptCallErrorNotificationAppearance();
    void handleDeclineCallErrorNotificationAppearance();
    void handleStartCallingErrorNotificationAppearance();
    void handleStopCallingErrorNotificationAppearance();
    void handleEndCallErrorNotificationAppearance();

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

private:
    QMediaPlayer* m_ringtonePlayer;
    QAudioOutput* m_audioOutput;

    QWidget* m_centralWidget;
    QHBoxLayout* m_mainLayout;
    QStackedLayout* m_stackedLayout;

    AuthorizationWidget* m_authorizationWidget;
    MainMenuWidget* m_mainMenuWidget;
    CallWidget* m_callWidget;

    QLabel* m_updatingProgressLabel = nullptr;
};