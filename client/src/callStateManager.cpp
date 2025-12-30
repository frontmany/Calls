#include "callStateManager.h"

namespace calls
{
    bool CallStateManager::isOutgoingCall() const {
        return m_outgoingCall.has_value();
    }

    bool CallStateManager::isActiveCall() const {
        return m_activeCall.has_value();
    }

    bool CallStateManager::isCallParticipantConnectionLost() const {
        m_activeCall.value().isCallParticipantConnectionLost();
    }


    const OutgoingCall& CallStateManager::getOutgoingCall() const {
        return m_outgoingCall.value();
    }

    const Call& CallStateManager::getActiveCall() const {
        return m_activeCall.value();
    }

    void CallStateManager::setActiveCall(const std::string& nickname,
        const CryptoPP::RSA::PublicKey& publicKey)
    {
        if (m_outgoingCall.has_value()) {
            m_outgoingCall->stop();
        }

        m_outgoingCall = std::nullopt;
        m_activeCall.emplace(nickname, publicKey);
    }

    void CallStateManager::setActiveCall(const IncomingCall& incomingCall) {
        if (m_outgoingCall.has_value()) {
            m_outgoingCall->stop();
        }

        m_outgoingCall = std::nullopt;
        m_activeCall.emplace(incomingCall);
    }

    void CallStateManager::setCallParticipantConnectionLost(bool value) {
        m_activeCall.value().setCallParticipantConnectionLost(value);
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
