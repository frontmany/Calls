#include "callStateManager.h"

namespace callifornia
{
    bool CallStateManager::isOutgoingCall() const {
        return m_outgoingCall.has_value();
    }

    bool CallStateManager::isActiveCall() const {
        return m_activeCall.has_value();
    }

    bool CallStateManager::isCallParticipantConnectionDown() const {
        if (!m_activeCall.has_value()) {
            return false;
        }
        return m_activeCall.value().isParticipantConnectionDown();
    }


    const OutgoingCall& CallStateManager::getOutgoingCall() const {
        if (!m_outgoingCall.has_value()) {
            throw std::runtime_error("OutgoingCall is not set");
        }
        return m_outgoingCall.value();
    }

    const Call& CallStateManager::getActiveCall() const {
        if (!m_activeCall.has_value()) {
            throw std::runtime_error("ActiveCall is not set");
        }
        return m_activeCall.value();
    }

    void CallStateManager::setActiveCall(const std::string& nickname,
        const CryptoPP::RSA::PublicKey& publicKey, const CryptoPP::SecByteBlock& callKey)
    {
        if (m_outgoingCall.has_value()) {
            m_outgoingCall->stop();
        }

        m_outgoingCall = std::nullopt;
        m_activeCall.emplace(nickname, publicKey, callKey);
    }

    void CallStateManager::setCallParticipantConnectionDown(bool value) {
        if (m_activeCall.has_value()) {
            m_activeCall.value().setParticipantConnectionDown(value);
        }
    }

    void CallStateManager::clear()
    {
        if (m_outgoingCall.has_value()) {
            m_outgoingCall->stop();
        }

        m_outgoingCall = std::nullopt;
        m_activeCall = std::nullopt;
    }
}
