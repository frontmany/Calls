#pragma once 

#include "core.h"
#include "errorCode.h"
#include <vector>
#include <system_error>

class MainWindow;

class CoreEventListener : public core::EventListener {
public:
    CoreEventListener(MainWindow* mainWindow);

    void onAuthorizationResult(std::error_code ec) override;
    void onLogoutCompleted() override;
    void onStartOutgoingCallResult(std::error_code ec) override;
    void onStopOutgoingCallResult(std::error_code ec) override;
    void onAcceptCallResult(std::error_code ec) override;
    void onDeclineCallResult(std::error_code ec) override;
    void onEndCallResult(std::error_code ec) override;

    void onStartScreenSharingResult(std::error_code ec) override;
    void onStopScreenSharingResult(std::error_code ec) override;
    void onIncomingScreenSharingStarted() override;
    void onIncomingScreenSharingStopped() override;
    void onIncomingScreen(const std::vector<unsigned char>& data) override;

    void onStartCameraSharingResult(std::error_code ec) override;
    void onStopCameraSharingResult(std::error_code ec) override;
    void onIncomingCameraSharingStarted() override;
    void onIncomingCameraSharingStopped() override;
    void onIncomingCamera(const std::vector<unsigned char>& data) override;

    void onOutgoingCallAccepted() override;
    void onOutgoingCallDeclined() override;
    void onOutgoingCallTimeout(std::error_code ec) override;
    void onIncomingCall(const std::string& friendNickname) override;
    void onIncomingCallExpired(std::error_code ec, const std::string& friendNickname) override;

    void onCallParticipantConnectionDown() override;
    void onCallParticipantConnectionRestored() override;

    void onConnectionDown() override;
    void onConnectionRestored() override;
    void onConnectionRestoredAuthorizationNeeded() override;
    void onCallEndedByRemote(std::error_code ec) override;

private:
    MainWindow* m_mainWindow;
};