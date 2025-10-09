#pragma once

#include <string>

namespace calls {
    class Handler {
    public:
        virtual ~Handler() = default;

        virtual void onAuthorizationResult(bool success) = 0;
        virtual void onLogoutResult(bool success) = 0;
        virtual void onStopResult(bool success) = 0;
        virtual void onStartCallingResult(bool success) = 0;
        virtual void onStopCallingResult(bool success) = 0;
        virtual void onDeclineAllIncomingCallsResult(bool success) = 0;
        virtual void onDeclineIncomingCallResult(bool success) = 0;
        virtual void onAcceptIncomingCallResult(bool success) = 0;
        virtual void onEndCallResult(bool success) = 0;
        virtual void onCallingStoppedResult(bool success) = 0;

        virtual void onCallingAccepted() = 0;
        virtual void onCallingDeclined() = 0;
        virtual void onIncomingCall(const std::string& callId) = 0;
        virtual void onIncomingCallExpired(const std::string& friendNickname) = 0;
        virtual void onCallingSomeoneWhoAlreadyCallingYou(const std::string& friendNickname) = 0;

        virtual void onNetworkError() = 0;
        virtual void onRemoteUserEndedCall() = 0;
    };
}
