#pragma once
#include <QMainWindow>
#include <QWidget>
#include <QStackedLayout>
#include <QHBoxLayout>

#include "calls.h"

class AuthorizationWidget;
class MainMenuWidget;
class CallWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr, const std::string& host = "192.168.1.45");
    ~MainWindow() = default;

public slots:
    void onAuthorizationResult(calls::Result authorizationResult);
    void onCreateCallResult(calls::Result createCallResult);
    void onIncomingCall(const std::string& friendNickName);
    void onIncomingCallExpired(const std::string& friendNickName);
    void onSimultaneousCalling(const std::string& friendNickName);
    void onCallHangUp();
    void onNetworkError();

private slots:
    void onIncomingCallAccepted(const QString& friendNickname);
    void onHangupClicked();
    void onMuteMicClicked();
    void onMuteSpeakerClicked();

private:
    void setupUI();
    void switchToAuthorizationWidget();
    void switchToMainMenuWidget();
    void switchToCallWidget(const QString& friendNickname);
    void loadFonts();

    QWidget* m_centralWidget;
    QHBoxLayout* m_mainLayout;
    QStackedLayout* m_stackedLayout;
    AuthorizationWidget* m_authorizationWidget;
    MainMenuWidget* m_mainMenuWidget;
    CallWidget* m_callWidget;
};