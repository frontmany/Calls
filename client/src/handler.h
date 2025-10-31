#pragma once

#include <string>

#include "errorCode.h"

namespace calls {
    class Handler {
    public:
        virtual ~Handler() = default;

        virtual void onAuthorizationResult(ErrorCode ec) = 0;
        virtual void onStartCallingResult(ErrorCode ec) = 0;
        virtual void onAcceptCallResult(ErrorCode ec, const std::string& nickname) = 0;
        
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