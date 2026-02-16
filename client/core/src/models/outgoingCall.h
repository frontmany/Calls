#pragma once

#include <string>
#include <functional>
#include <optional>

#include "utilities/crypto.h"
#include "ticTimer.h"

namespace core
{
    class OutgoingCall
    {
    public:
        template <typename Rep, typename Period>
        explicit OutgoingCall(const std::string& nickname,
            const std::chrono::duration<Rep, Period>& timeout,
            std::function<void()> onTimeout,
            std::optional<CryptoPP::RSA::PublicKey> publicKey = std::nullopt,
            std::optional<CryptoPP::SecByteBlock> callKey = std::nullopt)
            : m_nickname(nickname)
            , m_publicKey(std::move(publicKey))
            , m_callKey(std::move(callKey))
        {
            m_timer.start(timeout, std::move(onTimeout));
        }
        OutgoingCall(const OutgoingCall& other) = delete;
        OutgoingCall(OutgoingCall&& other) noexcept;
        ~OutgoingCall();

        const std::string& getNickname() const;
        const tic::SingleShotTimer& getTimer() const;
        bool hasCallContext() const;
        const CryptoPP::RSA::PublicKey& getPublicKey() const;
        const CryptoPP::SecByteBlock& getCallKey() const;
        void stop();

    private:
        std::string m_nickname;
        std::optional<CryptoPP::RSA::PublicKey> m_publicKey;
        std::optional<CryptoPP::SecByteBlock> m_callKey;
        tic::SingleShotTimer m_timer;
    };
}