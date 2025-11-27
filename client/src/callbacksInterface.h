#pragma once

#include <string>
#include <vector>

#include "errorCode.h"

namespace calls {
    class CallbacksInterface {
    public:
        virtual ~CallbacksInterface() = default;

        virtual void onAuthorizationResult(ErrorCode ec) = 0;
        virtual void onStartCallingResult(ErrorCode ec) = 0;
        virtual void onAcceptCallResult(ErrorCode ec, const std::string& nickname) = 0;
        
        virtual void onStartScreenSharingError() = 0;
        virtual void onIncomingScreenSharingStarted() = 0;
        virtual void onIncomingScreenSharingStopped() = 0;
        virtual void onIncomingScreen(const std::vector<unsigned char>& data) = 0;

        virtual void onStartCameraSharingError() = 0;
        virtual void onIncomingCameraSharingStarted() = 0;
        virtual void onIncomingCameraSharingStopped() = 0;
        virtual void onIncomingCamera(const std::vector<unsigned char>& data) = 0;

        virtual void onMaximumCallingTimeReached() = 0;
        virtual void onCallingAccepted() = 0;
        virtual void onCallingDeclined() = 0;
        virtual void onIncomingCall(const std::string& friendNickname) = 0;
        virtual void onIncomingCallExpired(const std::string& friendNickname) = 0;

        virtual void onNetworkError() = 0;
        virtual void onConnectionRestored() = 0;
        virtual void onRemoteUserEndedCall() = 0;
    };
}