#pragma once
#include <QMainWindow>
#include <QWidget>
#include <QStackedLayout>
#include <QHBoxLayout>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QFile>

#include "calls.h"

class AuthorizationWidget;
class MainMenuWidget;
class CallWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent, const std::string& host, const std::string& port);
    ~MainWindow();

private slots:
    void onAuthorizationResult(calls::Result authorizationResult);
    void onCreateCallResult(calls::Result createCallResult);
    void onIncomingCall(const std::string& friendNickName);
    void onIncomingCallExpired(const std::string& friendNickName);
    void onSimultaneousCalling(const std::string& friendNickName);
    void onRemoteUserEndedCall();
    void onNetworkError();

    void onIncomingCallAccepted(const QString& friendNickname);
    void onIncomingCallDeclined(const QString& friendNickname);
    void onCreateCallButtonClicked(const QString& friendNickname);
    void onStopCallingButtonClicked();
    void onHangupClicked();
    void onAuthorizationButtonClicked(const QString& friendNickname);
    void onRefreshAudioDevicesButtonClicked();
    void onInputVolumeChanged(int newVolume);
    void onOutputVolumeChanged(int newVolume);
    void onMuteButtonClicked(bool mute);


private:
    void setupUI();
    void switchToAuthorizationWidget();
    void switchToMainMenuWidget();
    void switchToCallWidget(const QString& friendNickname);
    void loadFonts();
    void playRingtone();
    void stopRingtone();

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    QMediaPlayer* m_ringtonePlayer;
    QAudioOutput* m_audioOutput;

    calls::Result m_authorizationResult = calls::Result::EMPTY;
    QWidget* m_centralWidget;
    QHBoxLayout* m_mainLayout;
    QStackedLayout* m_stackedLayout;
    AuthorizationWidget* m_authorizationWidget;
    MainMenuWidget* m_mainMenuWidget;
    CallWidget* m_callWidget;
};