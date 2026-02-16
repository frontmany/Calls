#include "outgoingCall.h"

#include <stdexcept>

namespace core
{
    OutgoingCall::OutgoingCall(OutgoingCall&& other) noexcept
        : m_nickname(std::move(other.m_nickname)),
        m_publicKey(std::move(other.m_publicKey)),
        m_callKey(std::move(other.m_callKey)),
        m_timer(std::move(other.m_timer))
    {
    }

    OutgoingCall::~OutgoingCall() {
        stop();
    }

    const std::string& OutgoingCall::getNickname() const {
        return m_nickname;
    }

    const tic::SingleShotTimer& OutgoingCall::getTimer() const {
        return m_timer;
    }

    bool OutgoingCall::hasCallContext() const {
        return m_publicKey.has_value() && m_callKey.has_value();
    }

    const CryptoPP::RSA::PublicKey& OutgoingCall::getPublicKey() const {
        if (!m_publicKey.has_value()) {
            throw std::runtime_error("Outgoing call public key is not set");
        }
        return m_publicKey.value();
    }

    const CryptoPP::SecByteBlock& OutgoingCall::getCallKey() const {
        if (!m_callKey.has_value()) {
            throw std::runtime_error("Outgoing call key is not set");
        }
        return m_callKey.value();
    }

    void OutgoingCall::stop() {
        m_timer.stop();
    }
}
