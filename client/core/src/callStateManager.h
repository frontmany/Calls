#pragma once

#include <optional>
#include <string>
#include <chrono>
#include <functional>

#include "call.h"
#include "outgoingCall.h"
#include "utilities/crypto.h"

namespace callifornia
{
    class CallStateManager {
    public:
        CallStateManager() = default;

        bool isOutgoingCall() const;
        bool isActiveCall() const;
        bool isCallParticipantConnectionDown() const;
        const OutgoingCall& getOutgoingCall() const;
        const Call& getActiveCall() const;

        void setActiveCall(const std::string& friendNickname,
            const CryptoPP::RSA::PublicKey& friendPublicKey,
            const CryptoPP::SecByteBlock& callKey
        );
        
        void setCallParticipantConnectionDown(bool value);
        void clear();

        template <typename Rep, typename Period>
        void setOutgoingCall(const std::string& nickname,
            const std::chrono::duration<Rep, Period>& timeout,
            std::function<void()> onTimeout)
        {
            if (m_outgoingCall.has_value())
            {
                m_outgoingCall->stop();
            }
            m_activeCall = std::nullopt;
            m_outgoingCall.emplace(nickname, timeout, std::move(onTimeout));
        }

    private:
        std::optional<OutgoingCall> m_outgoingCall;
        std::optional<Call> m_activeCall;
    };
}

