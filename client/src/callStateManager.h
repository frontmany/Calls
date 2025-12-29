#pragma once

#include <optional>
#include <string>
#include <chrono>

#include "call.h"
#include "outgoingCall.h"
#include "utilities/crypto.h"

namespace calls
{
    class CallStateManager {
    public:
        CallStateManager() = default;
        bool isOutgoingCall() const;
        bool isActiveCall() const;
        const OutgoingCall& getOutgoingCall() const;
        const Call& getActiveCall() const;

        template <typename Rep, typename Period, typename Callable>
        void setOutgoingCall(const std::string& nickname,
            const std::chrono::duration<Rep, Period>& timeout,
            Callable&& onTimeout)
        {
            if (m_outgoingCall.has_value())
            {
                m_outgoingCall->stop();
            }
            m_activeCall = std::nullopt;
            m_outgoingCall.emplace(nickname, timeout, std::forward<Callable>(onTimeout));
        }

        void setActiveCall(const std::string& friendNickname,
            const CryptoPP::RSA::PublicKey& friendPublicKey);

        void setActiveCall(const IncomingCall& incomingCall);

        void clear();

    private:
        std::optional<OutgoingCall> m_outgoingCall;
        std::optional<Call> m_activeCall;
    };
}

