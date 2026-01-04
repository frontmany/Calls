#pragma once

#include <string>
#include <vector>
#include <system_error>

namespace core 
{
    class EventListener {
    public:
        virtual ~EventListener() = default;

        virtual void onAuthorizationResult(std::error_code ec) = 0;
        virtual void onLogoutCompleted() = 0;
        virtual void onStartOutgoingCallResult(std::error_code ec) = 0;
        virtual void onStopOutgoingCallResult(std::error_code ec) = 0;
        virtual void onAcceptCallResult(std::error_code ec) = 0;
        virtual void onDeclineCallResult(std::error_code ec) = 0;
        virtual void onEndCallResult(std::error_code ec) = 0;
        virtual void onStartScreenSharingResult(std::error_code ec) = 0;
        virtual void onStopScreenSharingResult(std::error_code ec) = 0;
        virtual void onStartCameraSharingResult(std::error_code ec) = 0;
        virtual void onStopCameraSharingResult(std::error_code ec) = 0;

        virtual void onIncomingScreenSharingStarted() = 0;
        virtual void onIncomingScreenSharingStopped() = 0;
        virtual void onIncomingScreen(const std::vector<unsigned char>& data) = 0;

        virtual void onIncomingCameraSharingStarted() = 0;
        virtual void onIncomingCameraSharingStopped() = 0;
        virtual void onIncomingCamera(const std::vector<unsigned char>& data) = 0;

        virtual void onOutgoingCallAccepted() = 0;
        virtual void onOutgoingCallDeclined() = 0;
        virtual void onOutgoingCallTimeout(std::error_code ec) = 0;

        virtual void onIncomingCall(const std::string& nickname) = 0;
        virtual void onIncomingCallExpired(std::error_code ec, const std::string& nickname) = 0;
        virtual void onCallEndedByRemote(std::error_code ec) = 0;

        virtual void onCallParticipantConnectionDown() = 0;
        virtual void onCallParticipantConnectionRestored() = 0;

        virtual void onConnectionDown() = 0;
        virtual void onConnectionRestored() = 0;
        virtual void onConnectionRestoredAuthorizationNeeded() = 0;
    };
}