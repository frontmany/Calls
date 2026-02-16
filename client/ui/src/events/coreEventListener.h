#pragma once 

#include <vector>
#include <system_error>

#include "eventListener.h"

class AuthorizationManager;
class CallManager;
class CoreNetworkErrorHandler;

class CoreEventListener : public core::EventListener {
public:
    CoreEventListener(AuthorizationManager* authorizationManager, CallManager* callManager, CoreNetworkErrorHandler* networkErrorHandler);

    void onAuthorizationResult(std::error_code ec) override;
    void onStartOutgoingCallResult(std::error_code ec) override;

    void onIncomingScreenSharingStarted() override;
    void onIncomingScreenSharingStopped() override;
    void onIncomingScreen(const std::vector<unsigned char>& data, int width, int height) override;

    void onStartScreenSharingError() override;
    void onLocalScreen(const std::vector<unsigned char>& data, int width, int height) override;

    void onIncomingCameraSharingStarted() override;
    void onIncomingCameraSharingStopped() override;
    void onIncomingCamera(const std::vector<unsigned char>& data, int width, int height) override;

    void onStartCameraSharingError() override;
    void onLocalCamera(const std::vector<unsigned char>& data, int width, int height) override;

    void onOutgoingCallAccepted(const std::string& nickname) override;
    void onOutgoingCallDeclined() override;
    void onOutgoingCallTimeout(std::error_code ec) override;
    void onIncomingCall(const std::string& friendNickname) override;
    void onIncomingCallExpired(std::error_code ec, const std::string& friendNickname) override;
    void onCallEndedByRemote(std::error_code ec) override;

    void onCallParticipantConnectionDown() override;
    void onCallParticipantConnectionRestored() override;

    void onConnectionDown() override;
    void onConnectionRestored() override;
    void onConnectionRestoredAuthorizationNeeded() override;

private:
    AuthorizationManager* m_authorizationManager;
    CallManager* m_callManager;
    CoreNetworkErrorHandler* m_coreNetworkErrorHandler;
};
