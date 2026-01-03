#pragma once 

#include "eventListener.h"

#include <vector>

class MainWindow;

class ClientCallbacksHandler : public callifornia::EventListener {
public:
    ClientCallbacksHandler(MainWindow* mainWindow);

    void onAuthorizationResult(callifornia::ErrorCode ec) override;
    void onStartCallingResult(callifornia::ErrorCode ec) override;
    void onAcceptCallResult(callifornia::ErrorCode ec, const std::string& nickname) override;

    void onScreenSharingStarted() override;
    void onStartScreenSharingError() override;
    void onIncomingScreenSharingStarted() override;
    void onIncomingScreenSharingStopped() override;
    void onIncomingScreen(const std::vector<unsigned char>& data) override;

    void onCameraSharingStarted() override;
    void onStartCameraSharingError() override;
    void onIncomingCameraSharingStarted() override;
    void onIncomingCameraSharingStopped() override;
    void onIncomingCamera(const std::vector<unsigned char>& data) override;

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