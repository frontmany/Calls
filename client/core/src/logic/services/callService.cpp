#include "CallService.h"
#include "utilities/logger.h"
#include "constants/errorCode.h"
#include "constants/packetType.h"
#include "logic/packetFactory.h"

using namespace core::utilities;
using namespace core::constant;
using namespace std::chrono_literals;
using namespace std::placeholders;

namespace core::logic
{
    CallService::CallService(
        std::shared_ptr<ClientStateManager> stateManager,
        std::shared_ptr<KeyManager> keyManager,
        std::function<std::error_code(const std::vector<unsigned char>&, PacketType)>&& sendPacket)
        : m_stateManager(stateManager)
        , m_keyManager(keyManager)
        , m_sendPacket(std::move(sendPacket))
    {
    }

    std::error_code CallService::startOutgoingCall(const std::string& userNickname) {
        if (m_stateManager->isConnectionDown()) return make_error_code(ErrorCode::connection_down);
        if (!m_stateManager->isAuthorized()) return make_error_code(ErrorCode::not_authorized);
        if (m_stateManager->isActiveCall()) return make_error_code(ErrorCode::active_call_exists);

        auto packet = PacketFactory::getRequestUserInfoPacket(m_stateManager->getMyNickname(), m_keyManager->getMyPublicKey(), userNickname);

        return m_sendPacket(packet, PacketType::GET_USER_INFO);
    }

    std::error_code CallService::stopOutgoingCall() {
        if (m_stateManager->isConnectionDown()) return make_error_code(ErrorCode::connection_down);
        if (!m_stateManager->isAuthorized()) return make_error_code(ErrorCode::not_authorized);
        if (!m_stateManager->isOutgoingCall()) return make_error_code(ErrorCode::no_outgoing_call);

        auto packet = PacketFactory::getTwoNicknamesPacket(m_stateManager->getMyNickname(), m_stateManager->getOutgoingCall().getNickname());
        
        auto ec = m_sendPacket(packet, PacketType::CALLING_END);

        if (ec) {
            return ec;
        }
        else {
            m_stateManager->resetOutgoingCall();
        }

        return {};
    }

    std::error_code CallService::acceptCall(const std::string& userNickname) {
        if (m_stateManager->isConnectionDown()) return make_error_code(ErrorCode::connection_down);
        if (!m_stateManager->isAuthorized()) return make_error_code(ErrorCode::not_authorized);
        if (!m_stateManager->isIncomingCalls()) return make_error_code(ErrorCode::no_incoming_call);

        auto& incomingCalls = m_stateManager->getIncomingCalls();

        if (!incomingCalls.contains(userNickname)) {
            return make_error_code(ErrorCode::no_incoming_call);
        }

        for (auto& [nickname, incomingCallData] : incomingCalls) {
            if (nickname != userNickname) {
                auto packet = PacketFactory::getTwoNicknamesPacket(m_stateManager->getMyNickname(), nickname);
                
                auto ec = m_sendPacket(packet, PacketType::CALL_DECLINE);

                if (ec) {
                    return ec;
                }
                else {
                    m_stateManager->removeIncomingCall(nickname);
                }
            }
        }

        if (m_stateManager->isOutgoingCall()) {
            auto packet = PacketFactory::getTwoNicknamesPacket(m_stateManager->getMyNickname(), m_stateManager->getOutgoingCall().getNickname());
            
            auto ec = m_sendPacket(packet, PacketType::CALLING_END);

            if (ec) {
                return ec;
            }
            else {
                m_stateManager->resetOutgoingCall();
            }
        }
        else if (m_stateManager->isActiveCall()) {
            auto packet = PacketFactory::getTwoNicknamesPacket(m_stateManager->getMyNickname(), m_stateManager->getActiveCall().getNickname());
            
            auto ec = m_sendPacket(packet, PacketType::CALL_END);

            if (ec) {
                return ec;
            }
            else {
                changeStateOnEndCall();
            }
        }

        auto& inc = m_stateManager->getIncomingCalls();
        auto& incomingCall = inc.at(userNickname);
        auto packet = PacketFactory::getCallPacketWithKeys(m_stateManager->getMyNickname(), userNickname, m_keyManager->getMyPublicKey(), incomingCall.getPublicKey(), incomingCall.getCallKey());
            
        auto ec = m_sendPacket(packet, PacketType::CALL_ACCEPT);

        if (ec) {
            return ec;
        }
        else {
            auto& incomingCalls = m_stateManager->getIncomingCalls();
            auto& incomingCall = incomingCalls.at(userNickname);
            m_stateManager->setActiveCall(incomingCall.getNickname(), incomingCall.getPublicKey(), incomingCall.getCallKey());
            m_stateManager->removeIncomingCall(userNickname);
        }

        return {};
    }

    std::error_code CallService::declineCall(const std::string& userNickname) {
        if (m_stateManager->isConnectionDown()) return make_error_code(ErrorCode::connection_down);
        if (!m_stateManager->isAuthorized()) return make_error_code(ErrorCode::not_authorized);
        if (!m_stateManager->isIncomingCalls()) return make_error_code(ErrorCode::no_incoming_call);

        auto& incomingCalls = m_stateManager->getIncomingCalls();
        if (!incomingCalls.contains(userNickname)) {
            return make_error_code(ErrorCode::no_incoming_call);
        }

        auto packet = PacketFactory::getTwoNicknamesPacket(m_stateManager->getMyNickname(), userNickname);

        auto ec = m_sendPacket(packet, PacketType::CALL_DECLINE);

        if (ec) {
            return ec;
        }
        else {
            m_stateManager->removeIncomingCall(userNickname);
        }

        return {};
    }

    std::error_code CallService::endCall() {
        if (m_stateManager->isConnectionDown()) return make_error_code(ErrorCode::connection_down);
        if (!m_stateManager->isAuthorized()) return make_error_code(ErrorCode::not_authorized);
        if (!m_stateManager->isActiveCall()) return make_error_code(ErrorCode::no_active_call);

        auto packet = PacketFactory::getTwoNicknamesPacket(m_stateManager->getMyNickname(), m_stateManager->getActiveCall().getNickname());

        auto ec = m_sendPacket(packet, PacketType::CALL_END);

        if (ec) {
            return ec;
        }
        else {
            changeStateOnEndCall();
        }

        return {};
    }


    void CallService::changeStateOnEndCall() {
        m_stateManager->resetActiveCall();
        m_stateManager->setViewingRemoteScreen(false);
        m_stateManager->setViewingRemoteCamera(false);
    }
}
