#include "callPacketHandler.h"
#include "logic/clientStateManager.h"
#include "logic/keyManager.h"
#include "logic/packetFactory.h"
#include "utilities/crypto.h"
#include "utilities/logger.h"
#include "constants/errorCode.h"
#include "constants/jsonType.h"


using namespace core;
using namespace core::constant;
using namespace core::utilities::crypto;
using namespace std::chrono_literals;

namespace core::logic
{
    CallPacketHandler::CallPacketHandler(
        std::shared_ptr<ClientStateManager> stateManager,
        std::shared_ptr<KeyManager> keyManager,
        std::shared_ptr<EventListener> eventListener,
        std::function<std::error_code(const std::vector<unsigned char>&, core::constant::PacketType)>&& sendPacket)
        : m_stateManager(stateManager)
        , m_keyManager(keyManager)
        , m_eventListener(eventListener)
        , m_sendPacket(sendPacket)
    {
    }

    void CallPacketHandler::handleGetUserInfoResult(const nlohmann::json& jsonObject) {
        if (!m_stateManager->isAuthorized() || m_stateManager->isConnectionDown()) return;

        bool userInfoFound = jsonObject[RESULT].get<bool>();

        if (userInfoFound) {
            const std::string& encryptedPacketKey = jsonObject[PACKET_KEY].get<std::string>();
            auto packetKey = RSADecryptAESKey(m_keyManager->getMyPrivateKey(), encryptedPacketKey);

            const std::string& encryptedUserNickname = jsonObject[ENCRYPTED_NICKNAME].get<std::string>();
            std::string userNickname = AESDecrypt(packetKey, encryptedUserNickname);

            const std::string& serializedUserPublicKey = jsonObject[PUBLIC_KEY].get<std::string>();
            auto userPublicKey = deserializePublicKey(serializedUserPublicKey);

            CryptoPP::SecByteBlock callKey;
            generateAESKey(callKey);

            auto packet = PacketFactory::getCallPacketWithKeys(m_stateManager->getMyNickname(), userNickname, m_keyManager->getMyPublicKey(), userPublicKey, callKey);
            
            auto ec = m_sendPacket(packet, PacketType::CALLING_BEGIN);

            if (!ec) {
                m_stateManager->setOutgoingCall(userNickname, 32s, [this]() {
                    if (m_stateManager->isConnectionDown()) {
                        return;
                    }
                    m_stateManager->resetOutgoingCall();
                    m_eventListener->onOutgoingCallTimeout({});
                }, userPublicKey, callKey);
            }

            if (m_eventListener) {
                m_eventListener->onStartOutgoingCallResult(ec);
            }
        }
        else {
            if (m_eventListener) {
                m_eventListener->onStartOutgoingCallResult(make_error_code(ErrorCode::unexisting_user));
            }
        }
    }

    void CallPacketHandler::handleIncomingCallBegin(const nlohmann::json& jsonObject) {
        if (!m_stateManager->isAuthorized() || m_stateManager->isConnectionDown()) return;
        
        const std::string& encryptedPacketKey = jsonObject[PACKET_KEY].get<std::string>();
        auto packetKey = RSADecryptAESKey(m_keyManager->getMyPrivateKey(), encryptedPacketKey);

        const std::string& encryptedNickname = jsonObject[SENDER_ENCRYPTED_NICKNAME].get<std::string>();
        std::string senderNickname = AESDecrypt(packetKey, encryptedNickname);

        const std::string& serializedPublicKey = jsonObject[SENDER_PUBLIC_KEY].get<std::string>();
        auto publicKey = deserializePublicKey(serializedPublicKey);

        const std::string& encryptedCallKey = jsonObject[ENCRYPTED_CALL_KEY].get<std::string>();
        auto callKey = RSADecryptAESKey(m_keyManager->getMyPrivateKey(), encryptedCallKey);

        auto& incomingCalls = m_stateManager->getIncomingCalls();
        if (incomingCalls.contains(senderNickname)) return;

        m_stateManager->addIncomingCall(senderNickname, publicKey, callKey, 32s, [this, senderNickname]() {
            m_stateManager->removeIncomingCall(senderNickname);
            m_eventListener->onIncomingCallExpired({}, senderNickname);
        });

        m_eventListener->onIncomingCall(senderNickname);
    }

    void CallPacketHandler::handleIncomingCallEnd(const nlohmann::json& jsonObject) {
        if (!m_stateManager->isAuthorized() || m_stateManager->isConnectionDown() || !m_stateManager->isIncomingCalls()) return;

        const std::string& senderNicknameHash = jsonObject[SENDER_NICKNAME_HASH].get<std::string>();

        auto& incomingCalls = m_stateManager->getIncomingCalls();
        
        const auto it = std::find_if(incomingCalls.begin(), incomingCalls.end(), [&senderNicknameHash](const auto& pair) {
            return calculateHash(pair.first) == senderNicknameHash;
        });

        if (it == incomingCalls.end()) return;

        std::string userNickname = it->second.getNickname();

        m_stateManager->removeIncomingCall(userNickname);
        m_eventListener->onIncomingCallExpired({}, userNickname);
    }

    void CallPacketHandler::handleOutgoingCallAccepted(const nlohmann::json& jsonObject) {
        if (!m_stateManager->isAuthorized() || m_stateManager->isConnectionDown() || !m_stateManager->isOutgoingCall()) return;

        const std::string& senderNicknameHash = jsonObject[SENDER_NICKNAME_HASH].get<std::string>();

        const std::string& encryptedPacketKey = jsonObject[PACKET_KEY].get<std::string>();
        auto packetKey = RSADecryptAESKey(m_keyManager->getMyPrivateKey(), encryptedPacketKey);
        
        const std::string& encryptedNickname = jsonObject[SENDER_ENCRYPTED_NICKNAME].get<std::string>();
        std::string senderNickname = AESDecrypt(packetKey, encryptedNickname);
        
        const std::string& serializedPublicKey = jsonObject[SENDER_PUBLIC_KEY].get<std::string>();
        auto senderPublicKey = deserializePublicKey(serializedPublicKey);

        const std::string& encryptedCallKey = jsonObject[ENCRYPTED_CALL_KEY].get<std::string>();
        auto callKey = RSADecryptAESKey(m_keyManager->getMyPrivateKey(), encryptedCallKey);

        if (calculateHash(m_stateManager->getOutgoingCall().getNickname()) != senderNicknameHash) return;

        if (m_stateManager->isIncomingCalls()) {
            m_stateManager->resetIncomingCalls();
        }

        auto& incomingCalls = m_stateManager->getIncomingCalls();

        for (auto& [nickname, incomingCallData] : incomingCalls) {
            auto packet = PacketFactory::getTwoNicknamesPacket(m_stateManager->getMyNickname(), nickname);

            auto ec = m_sendPacket(packet, PacketType::CALL_DECLINE);

            if (ec) {
                return;
            }
            else {
                m_stateManager->removeIncomingCall(nickname);
            }
        }

        if (m_stateManager->isActiveCall()) {
            auto packet = PacketFactory::getTwoNicknamesPacket(m_stateManager->getMyNickname(), m_stateManager->getActiveCall().getNickname());

            auto ec = m_sendPacket(packet, PacketType::CALL_END);

            if (ec) {
                return;
            }
            else {
                changeStateOnEndCall();
            }
        }

        m_stateManager->setActiveCall(senderNickname, senderPublicKey, callKey);

        m_eventListener->onOutgoingCallAccepted();
    }

    void CallPacketHandler::handleOutgoingCallDeclined(const nlohmann::json& jsonObject) {
        if (!m_stateManager->isAuthorized() || m_stateManager->isConnectionDown()) return;
        
        const std::string& senderNicknameHash = jsonObject[SENDER_NICKNAME_HASH].get<std::string>();

        if (calculateHash(m_stateManager->getOutgoingCall().getNickname()) != senderNicknameHash) return;

        m_stateManager->resetOutgoingCall();
        m_eventListener->onOutgoingCallDeclined();
    }

    void CallPacketHandler::handleCallEndedByRemote(const nlohmann::json& jsonObject) {
        if (!m_stateManager->isAuthorized() || m_stateManager->isConnectionDown() || !m_stateManager->isActiveCall()) return;

        const std::string& senderNicknameHash = jsonObject[SENDER_NICKNAME_HASH].get<std::string>();

        if (calculateHash(m_stateManager->getActiveCall().getNickname()) != senderNicknameHash) return;
        
        changeStateOnEndCall();

        m_eventListener->onCallEndedByRemote({});
    }

    void CallPacketHandler::handleRemoteUserConnectionDown(const nlohmann::json& jsonObject) {
        if (!m_stateManager->isAuthorized() || m_stateManager->isConnectionDown()) return;
        
        const std::string& userNicknameHash = jsonObject[NICKNAME_HASH].get<std::string>();

        if (m_stateManager->isOutgoingCall()) {
            if (calculateHash(m_stateManager->getOutgoingCall().getNickname()) == userNicknameHash) {
                m_stateManager->resetOutgoingCall();

                m_eventListener->onOutgoingCallTimeout(make_error_code(ErrorCode::connection_down_with_user));
            }
        }

        if (m_stateManager->isIncomingCalls()) {
            auto& incomingCalls = m_stateManager->getIncomingCalls();

            auto it = std::find_if(incomingCalls.begin(), incomingCalls.end(), [&userNicknameHash](const auto& pair) {
                return calculateHash(pair.first) == userNicknameHash;
            });

            if (it != incomingCalls.end()) {
                auto& [nickname, _] = *it;
                std::string nicknameCopy = nickname;
                m_stateManager->removeIncomingCall(nicknameCopy);
                m_eventListener->onIncomingCallExpired(make_error_code(ErrorCode::connection_down_with_user), nicknameCopy);
            }
        }

        if (m_stateManager->isActiveCall() && calculateHash(m_stateManager->getActiveCall().getNickname()) == userNicknameHash) {
            m_stateManager->setViewingRemoteScreen(false);
            m_stateManager->setViewingRemoteCamera(false);
            m_stateManager->setCallParticipantConnectionDown(true);

            m_eventListener->onCallParticipantConnectionDown();
        }

    }

    void CallPacketHandler::handleRemoteUserConnectionRestored(const nlohmann::json& jsonObject) {
        if (!m_stateManager->isAuthorized() || m_stateManager->isConnectionDown()) return;

        const std::string& userNicknameHash = jsonObject[NICKNAME_HASH].get<std::string>();

        if (m_stateManager->isActiveCall()
            && calculateHash(m_stateManager->getActiveCall().getNickname()) == userNicknameHash) {
            m_stateManager->setCallParticipantConnectionDown(false);

            m_eventListener->onCallParticipantConnectionRestored();
        }
    }

    void CallPacketHandler::handleRemoteUserLogout(const nlohmann::json& jsonObject) {
        if (!m_stateManager->isAuthorized() || m_stateManager->isConnectionDown()) return;
        
        std::string userNicknameHash = jsonObject[NICKNAME_HASH];

        if (m_stateManager->isOutgoingCall()) {
            const auto& outgoingCall = m_stateManager->getOutgoingCall();
            if (calculateHash(outgoingCall.getNickname()) == userNicknameHash) {
                m_stateManager->resetOutgoingCall();

                m_eventListener->onOutgoingCallTimeout(make_error_code(ErrorCode::user_logout));
            }
        }

        if (m_stateManager->isIncomingCalls()) {
            const auto& incomingCalls = m_stateManager->getIncomingCalls();

            auto it = std::find_if(incomingCalls.begin(), incomingCalls.end(), [&userNicknameHash](const auto& pair) {
                return calculateHash(pair.first) == userNicknameHash;
            });

            if (it != incomingCalls.end()) {
                const auto& [nickname, _] = *it;
                std::string nicknameCopy = nickname;
                m_stateManager->removeIncomingCall(nicknameCopy);

                m_eventListener->onIncomingCallExpired(make_error_code(ErrorCode::user_logout), nicknameCopy);
            }
        }

        if (m_stateManager->isActiveCall()) {
            if (calculateHash(m_stateManager->getActiveCall().getNickname()) == userNicknameHash) {
                m_stateManager->resetActiveCall();

                m_eventListener->onCallEndedByRemote(make_error_code(ErrorCode::user_logout));
            }
        }
    }

    void CallPacketHandler::changeStateOnEndCall() {
        m_stateManager->resetActiveCall();
        m_stateManager->setViewingRemoteScreen(false);
        m_stateManager->setViewingRemoteCamera(false);
    }
}