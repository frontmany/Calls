#pragma once
#include <QMainWindow>
#include <QWidget>
#include <QStackedLayout>
#include <QHBoxLayout>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QFile>
#include <QDialog>
#include <QScreen>

#include "calls.h"
#include "callsClientHandler.h"

class AuthorizationWidget;
class MainMenuWidget;
class CallWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent, const std::string& host, const std::string& port);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onAuthorizationResult(bool success);
    void onLogoutResult(bool success);
    void onShutdownResult(bool success);
    void onStartCallingResult(bool success);
    void onCallingStoppedResult(bool success);
    void onDeclineIncomingCallResult(bool success, QString nickname);
    void onAcceptIncomingCallResult(bool success, QString nickname);
    void onAllIncomingCallsDeclinedResult(bool success);
    void onEndCallResult(bool success);

    void onCallingAccepted();
    void onCallingDeclined();
    void onRemoteUserEndedCall();
    void onIncomingCall(QString friendNicname);
    void onIncomingCallEnded(QString friendNickname);
    void onNetworkError();
    void onConnectionRestored();

    void onStartCallingButtonClicked(const QString& friendNickname);
    void onStopCallingButtonClicked();
    void onAcceptCallButtonClicked(const QString& friendNickname);
    void onDeclineCallButtonClicked(const QString& friendNickname);
    void onEndCallButtonClicked();
    void onAuthorizationButtonClicked(const QString& friendNickname);
    void onRefreshAudioDevicesButtonClicked();
    void onInputVolumeChanged(int newVolume);
    void onOutputVolumeChanged(int newVolume);
    void onMuteButtonClicked(bool mute);
    void onBlurAnimationFinished();

private:
    void showInitializationErrorDialog();
    void switchToAuthorizationWidget();
    void switchToMainMenuWidget();
    void switchToCallWidget(const QString& friendNickname);

    void handleAcceptCallErrorNotificationAppearance();
    void handleDeclineCallErrorNotificationAppearance();
    void handleDeclineAllCallsErrorNotificationAppearance();
    void handleStartCallingErrorNotificationAppearance();
    void handleStopCallingErrorNotificationAppearance();
    void handleEndCallErrorNotificationAppearance();

    void setupUI();
    void loadFonts();
    void playRingtone();
    void pauseRingtone();
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
};