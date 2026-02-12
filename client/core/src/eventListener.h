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
        virtual void onStartOutgoingCallResult(std::error_code ec) = 0;

        virtual void onIncomingScreenSharingStarted() = 0;
        virtual void onIncomingScreenSharingStopped() = 0;
        virtual void onIncomingScreen(const std::vector<unsigned char>& data, int width, int height) = 0;

        virtual void onStartScreenSharingError() = 0;
        virtual void onLocalScreen(const std::vector<unsigned char>& data, int width, int height) = 0;

        virtual void onIncomingCameraSharingStarted() = 0;
        virtual void onIncomingCameraSharingStopped() = 0;
        virtual void onIncomingCamera(const std::vector<unsigned char>& data, int width, int height) = 0;

        virtual void onStartCameraSharingError() = 0;
        virtual void onLocalCamera(const std::vector<unsigned char>& data, int width, int height) = 0;

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