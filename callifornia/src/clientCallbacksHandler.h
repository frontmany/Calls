#pragma once 

#include "calls.h"

class MainWindow;

class ClientCallbacksHandler : public calls::CallbacksInterface {
public:
    ClientCallbacksHandler(MainWindow* mainWindow);

    void onAuthorizationResult(calls::ErrorCode ec) override;
    void onStartCallingResult(calls::ErrorCode ec) override;
    void onAcceptCallResult(calls::ErrorCode ec, const std::string& nickname) override;

    void onMaximumCallingTimeReached() override;
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