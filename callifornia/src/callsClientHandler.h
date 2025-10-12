#pragma once 

#include "calls.h"

class MainWindow;

class CallsClientHandler : public calls::Handler {
public:
	CallsClientHandler(MainWindow* mainWindow);

    void onAuthorizationResult(bool success) override;
    void onLogoutResult(bool success) override;
    void onShutdownResult(bool success) override;
    void onStartCallingResult(bool success) override;
    void onDeclineIncomingCallResult(bool success, const std::string& nickname) override;
    void onAcceptIncomingCallResult(bool success, const std::string& nickname) override;
    void onAllIncomingCallsDeclinedResult(bool success) override;
    void onEndCallResult(bool success) override;
    void onCallingStoppedResult(bool success) override;

    void onCallingAccepted() override;
    void onCallingDeclined() override;
    void onIncomingCall(const std::string& friendNickname) override;
    void onIncomingCallExpired(const std::string& friendNickname) override;

    void onNetworkError() override;
    void onConnectionRestored() override;
    void onRemoteUserEndedCall() override;

private:
	MainWindow* m_mainWindow;
};