#include "packetHandleController.h"

#include "constants/errorCode.h"
#include "constants/jsonType.h"
#include "logic/packetFactory.h"
#include "utilities/logger.h"
#include "logic/clientStateManager.h"
#include "logic/keyManager.h"

using namespace core::constant;
using namespace core::utilities;
using namespace std::chrono_literals;

namespace core::logic
{
    PacketHandleController::PacketHandleController(
        std::shared_ptr<ClientStateManager>& stateManager,
        std::shared_ptr<KeyManager>& keyManager,
        std::shared_ptr<media::AudioEngine> audioEngine,
        std::shared_ptr<media::MediaProcessingService> mediaProcessingService,
        std::shared_ptr<EventListener> eventListener,
        std::function<std::error_code(const std::vector<unsigned char>&, core::constant::PacketType)>&& sendPacket)
    {
        m_authorizationPacketHandler = std::make_unique<AuthorizationPacketHandler>(stateManager, keyManager, eventListener);
        m_callPacketHandler = std::make_unique<CallPacketHandler>(stateManager, keyManager, eventListener, sendPacket);
        m_mediaPacketHandler = std::make_unique<MediaPacketHandler>(stateManager, audioEngine, mediaProcessingService, eventListener);
        m_reconnectionPacketHandler = std::make_unique<ReconnectionPacketHandler>(stateManager, eventListener);

        m_packetHandlers.emplace(PacketType::AUTHORIZATION_RESULT, [this](const nlohmann::json& json) { onAuthorizationResult(json); });
        m_packetHandlers.emplace(PacketType::RECONNECT_RESULT, [this](const nlohmann::json& json) {onReconnectResult(json); });
        m_packetHandlers.emplace(PacketType::GET_USER_INFO_RESULT, [this](const nlohmann::json& json) {onUserInfoResult(json); });
        m_packetHandlers.emplace(PacketType::CALLING_BEGIN, [this](const nlohmann::json& json) {onIncomingCallBegin(json); });
        m_packetHandlers.emplace(PacketType::CALLING_END, [this](const nlohmann::json& json) {onIncomingCallEnded(json); });
        m_packetHandlers.emplace(PacketType::CALL_ACCEPT, [this](const nlohmann::json& json) {onCallAccepted(json); });
        m_packetHandlers.emplace(PacketType::CALL_DECLINE, [this](const nlohmann::json& json) {onCallDeclined(json); });
        m_packetHandlers.emplace(PacketType::CALL_END, [this](const nlohmann::json& json) {onCallEnded(json); });
        m_packetHandlers.emplace(PacketType::SCREEN_SHARING_BEGIN, [this](const nlohmann::json& json) {onScreenSharingBegin(json); });
        m_packetHandlers.emplace(PacketType::SCREEN_SHARING_END, [this](const nlohmann::json& json) {onScreenSharingEnded(json); });
        m_packetHandlers.emplace(PacketType::CAMERA_SHARING_BEGIN, [this](const nlohmann::json& json) {onCameraSharingBegin(json); });
        m_packetHandlers.emplace(PacketType::CAMERA_SHARING_END, [this](const nlohmann::json& json) {onCameraSharingEnded(json); });
        m_packetHandlers.emplace(PacketType::CONNECTION_DOWN_WITH_USER, [this](const nlohmann::json& json) {onConnectionDownWithUser(json); });
        m_packetHandlers.emplace(PacketType::CONNECTION_RESTORED_WITH_USER, [this](const nlohmann::json& json) {onConnectionRestoredWithUser(json); });
        m_packetHandlers.emplace(PacketType::USER_LOGOUT, [this](const nlohmann::json& json) {onUserLogout(json); });
    }

    void PacketHandleController::processPacket(const unsigned char* data, int length, PacketType type)
    {
        if (type == PacketType::VOICE) {
            onVoice(data, length);
        }
        else if (type == PacketType::SCREEN) {
            onScreen(data, length);
        }
        else if (type == PacketType::CAMERA) {
            onCamera(data, length);
        }
        else {
            std::function<void(const nlohmann::json&)> handler;
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                if (m_packetHandlers.contains(type)) {
                    handler = m_packetHandlers[type];
                }
            }

            if (handler) {
                try {
                    nlohmann::json jsonObject = nlohmann::json::parse(data, data + length);
                    handler(jsonObject);
                }
                catch (const nlohmann::json::exception& e) {
                    LOG_ERROR("Failed to parse JSON packet: {}", e.what());
                }
            }
            else {
                LOG_WARN("Unknown packet type");
            }
        }
    }

    void PacketHandleController::onVoice(const unsigned char* data, int length) {
        if (m_stateManager.isActiveCall() && m_audioEngine.isStream()) {
            const CryptoPP::SecByteBlock& callKey = m_stateManager.getActiveCall().getCallKey();
            auto decryptedData = m_mediaEncryptionService.decryptMedia(data, length, callKey);

            if (!decryptedData.empty()) {
                m_audioEngine.playAudio(decryptedData.data(), static_cast<int>(decryptedData.size()));
            }
        }
    }

    void PacketHandleController::onScreen(const unsigned char* data, int length) {
        if (!m_stateManager.isActiveCall() || !m_stateManager.isViewingRemoteScreen() || !data || length <= 0) return;

        const CryptoPP::SecByteBlock& callKey = m_stateManager.getActiveCall().getCallKey();
        auto decrypted = m_mediaEncryptionService.decryptMedia(data, length, callKey);

        if (!decrypted.empty()) {
            m_eventListener->onIncomingScreen(decrypted);
        }
    }

    void PacketHandleController::onCamera(const unsigned char* data, int length) {
        if (!m_stateManager.isActiveCall() || !m_stateManager.isViewingRemoteCamera() || !data || length <= 0) return;

        const CryptoPP::SecByteBlock& callKey = m_stateManager.getActiveCall().getCallKey();
        auto decrypted = m_mediaEncryptionService.decryptMedia(data, length, callKey);

        if (!decrypted.empty()) {
            m_eventListener->onIncomingCamera(decrypted);
        }
    }

    void PacketHandleController::onAuthorizationResult(const nlohmann::json& jsonObject)
    {
        const std::string& uid = jsonObject[UID];
        m_pendingRequests.complete(uid, jsonObject);
    }

    void PacketHandleController::onReconnectResult(const nlohmann::json& jsonObject)
    {
        if (!jsonObject.contains(UID)) {
            LOG_WARN("RECONNECT_RESULT packet missing UID, ignoring");
            return;
        }
        std::string uid = jsonObject[UID].get<std::string>();
        if (!m_pendingRequests.complete(uid, jsonObject))
            LOG_WARN("RECONNECT_RESULT for unknown uid");
    }

    void PacketHandleController::setOrphanReconnectSuccessHandler(std::function<void(std::optional<nlohmann::json>)> f)
    {
        m_onOrphanReconnectSuccess = std::move(f);
    }

    void PacketHandleController::onUserInfoResult(const nlohmann::json& jsonObject)
    {
        const std::string& uid = jsonObject[UID];
        m_pendingRequests.complete(uid, jsonObject);
    }

    void PacketHandleController::onIncomingCallBegin(const nlohmann::json& jsonObject)
    {
        auto packetKey = crypto::RSADecryptAESKey(m_keysManager.getMyPrivateKey(), jsonObject[PACKET_KEY]);
        std::string senderNickname = crypto::AESDecrypt(packetKey, jsonObject[SENDER_ENCRYPTED_NICKNAME]);
        std::string senderNicknameHash = jsonObject[SENDER_NICKNAME_HASH];
        std::string uid = jsonObject[UID];
        auto senderPublicKey = crypto::deserializePublicKey(jsonObject[SENDER_PUBLIC_KEY]);
        auto callKey = crypto::RSADecryptAESKey(m_keysManager.getMyPrivateKey(), jsonObject[ENCRYPTED_CALL_KEY]);

        if (!m_stateManager.isAuthorized() || m_stateManager.isConnectionDown()) return;

        auto& incomingCalls = m_stateManager.getIncomingCalls();
        if (incomingCalls.contains(senderNickname)) return;

        m_stateManager.addIncomingCall(senderNickname, senderPublicKey, callKey, 32s, [this, senderNickname]() {
            if (m_stateManager.tryRemoveIncomingCall(senderNickname)) {
                m_eventListener->onIncomingCallExpired({}, senderNickname);
            }
            });

        m_eventListener->onIncomingCall(senderNickname);
    }

    void PacketHandleController::onIncomingCallEnded(const nlohmann::json& jsonObject)
    {
        std::string uid = jsonObject[UID];
        std::string senderNicknameHash = jsonObject[SENDER_NICKNAME_HASH];
        std::string myNicknameHash = jsonObject[RECEIVER_NICKNAME_HASH];

        if (!m_stateManager.isAuthorized() || m_stateManager.isConnectionDown() || !m_stateManager.isIncomingCalls()) return;

        auto& incomingCalls = m_stateManager.getIncomingCalls();
        const auto it = std::find_if(incomingCalls.begin(), incomingCalls.end(), [&senderNicknameHash](const auto& pair) {
            return crypto::calculateHash(pair.first) == senderNicknameHash;
            });

        if (it == incomingCalls.end()) return;

        std::string userNickname = it->second.getNickname();
        if (m_stateManager.tryRemoveIncomingCall(userNickname)) {
            m_eventListener->onIncomingCallExpired({}, userNickname);
        }
    }

    void PacketHandleController::onCallAccepted(const nlohmann::json& jsonObject)
    {
        std::string uid = jsonObject[UID];
        std::string senderNicknameHash = jsonObject[SENDER_NICKNAME_HASH];
        std::string myNicknameHash = jsonObject[RECEIVER_NICKNAME_HASH];

        auto packetKey = crypto::RSADecryptAESKey(m_keysManager.getMyPrivateKey(), jsonObject[PACKET_KEY]);
        std::string senderNickname = crypto::AESDecrypt(packetKey, jsonObject[SENDER_ENCRYPTED_NICKNAME]);
        auto senderPublicKey = crypto::deserializePublicKey(jsonObject[SENDER_PUBLIC_KEY]);
        auto callKey = crypto::RSADecryptAESKey(m_keysManager.getMyPrivateKey(), jsonObject[ENCRYPTED_CALL_KEY]);


        if (!m_stateManager.isAuthorized() || m_stateManager.isConnectionDown() || !m_stateManager.isOutgoingCall()) return;

        if (crypto::calculateHash(m_stateManager.getOutgoingCall().getNickname()) != senderNicknameHash) return;

        if (m_stateManager.isIncomingCalls()) {
            m_stateManager.clearIncomingCalls();
        }

        m_stateManager.clearCallState();
        m_stateManager.setActiveCall(senderNickname, senderPublicKey, callKey);

        m_audioEngine.startStream();
        m_eventListener->onOutgoingCallAccepted();
    }

    void PacketHandleController::onCallDeclined(const nlohmann::json& jsonObject)
    {
        std::string uid = jsonObject[UID];
        std::string senderNicknameHash = jsonObject[SENDER_NICKNAME_HASH];
        std::string myNicknameHash = jsonObject[RECEIVER_NICKNAME_HASH];

        if (!m_stateManager.isAuthorized() || m_stateManager.isConnectionDown() || !m_stateManager.isOutgoingCall()) return;

        m_stateManager.clearCallState();

        m_eventListener->onOutgoingCallDeclined();
    }


    void PacketHandleController::onCallEnded(const nlohmann::json& jsonObject)
    {
        std::string uid = jsonObject[UID];
        std::string senderNicknameHash = jsonObject[SENDER_NICKNAME_HASH];
        std::string myNicknameHash = jsonObject[RECEIVER_NICKNAME_HASH];

        if (!m_stateManager.isAuthorized() || m_stateManager.isConnectionDown() || !m_stateManager.isActiveCall() || crypto::calculateHash(m_stateManager.getActiveCall().getNickname()) != senderNicknameHash) return;

        m_stateManager.setScreenSharing(false);
        m_stateManager.setCameraSharing(false);
        m_stateManager.setViewingRemoteScreen(false);
        m_stateManager.setViewingRemoteCamera(false);
        m_stateManager.clearCallState();
        m_audioEngine.stopStream();

        m_eventListener->onCallEndedByRemote({});
    }

    void PacketHandleController::onScreenSharingBegin(const nlohmann::json& jsonObject)
    {
        std::string uid = jsonObject[UID];
        std::string senderNicknameHash = jsonObject[SENDER_NICKNAME_HASH];
        std::string myNicknameHash = jsonObject[RECEIVER_NICKNAME_HASH];

        if (!m_stateManager.isAuthorized() || m_stateManager.isConnectionDown() || !m_stateManager.isActiveCall() || m_stateManager.isViewingRemoteScreen()) return;

        m_stateManager.setViewingRemoteScreen(true);

        m_eventListener->onIncomingScreenSharingStarted();
    }

    void PacketHandleController::onScreenSharingEnded(const nlohmann::json& jsonObject)
    {
        std::string uid = jsonObject[UID];
        std::string senderNicknameHash = jsonObject[SENDER_NICKNAME_HASH];
        std::string myNicknameHash = jsonObject[RECEIVER_NICKNAME_HASH];

        if (!m_stateManager.isAuthorized() || m_stateManager.isConnectionDown() || !m_stateManager.isActiveCall() || !m_stateManager.isViewingRemoteScreen()) return;

        m_stateManager.setViewingRemoteScreen(false);

        m_eventListener->onIncomingScreenSharingStopped();
    }

    void PacketHandleController::onCameraSharingBegin(const nlohmann::json& jsonObject)
    {
        std::string uid = jsonObject[UID];
        std::string senderNicknameHash = jsonObject[SENDER_NICKNAME_HASH];
        std::string myNicknameHash = jsonObject[RECEIVER_NICKNAME_HASH];

        if (!m_stateManager.isAuthorized() || m_stateManager.isConnectionDown() || !m_stateManager.isActiveCall() || m_stateManager.isViewingRemoteCamera()) return;

        m_stateManager.setViewingRemoteCamera(true);

        m_eventListener->onIncomingCameraSharingStarted();
    }

    void PacketHandleController::onCameraSharingEnded(const nlohmann::json&)
    {
        if (!m_stateManager.isAuthorized() || m_stateManager.isConnectionDown() || !m_stateManager.isActiveCall() || !m_stateManager.isViewingRemoteCamera()) return;

        m_stateManager.setViewingRemoteCamera(false);

        m_eventListener->onIncomingCameraSharingStopped();
    }

    void PacketHandleController::onConnectionDownWithUser(const nlohmann::json& jsonObject) {
        std::string userNicknameHash = jsonObject[NICKNAME_HASH];

        if (m_stateManager.isAuthorized() && crypto::calculateHash(m_stateManager.getMyNickname()) == userNicknameHash) {
            if (m_stateManager.isInReconnectGracePeriod()) {
                return;
            }
            m_mediaController.notifyConnectionDown();
            return;
        }

        if (m_stateManager.isOutgoingCall()) {
            const auto& outgoingCall = m_stateManager.getOutgoingCall();
            if (crypto::calculateHash(outgoingCall.getNickname()) == userNicknameHash) {
                m_stateManager.clearCallState();

                m_eventListener->onOutgoingCallTimeout(ErrorCode::connection_down_with_user);
            }
        }

        if (m_stateManager.isIncomingCalls()) {
            const auto& incomingCalls = m_stateManager.getIncomingCalls();
            auto it = std::find_if(incomingCalls.begin(), incomingCalls.end(), [&userNicknameHash](const auto& pair) {
                return crypto::calculateHash(pair.first) == userNicknameHash;
                });

            if (it != incomingCalls.end()) {
                const auto& [nickname, incomingCall] = *it;
                std::string nicknameCopy = nickname;
                if (m_stateManager.tryRemoveIncomingCall(nicknameCopy)) {
                    m_eventListener->onIncomingCallExpired(ErrorCode::connection_down_with_user, nicknameCopy);
                }
            }
        }

        if (m_stateManager.isActiveCall() && crypto::calculateHash(m_stateManager.getActiveCall().getNickname()) == userNicknameHash) {
            m_stateManager.setScreenSharing(false);
            m_stateManager.setCameraSharing(false);
            m_stateManager.setViewingRemoteScreen(false);
            m_stateManager.setViewingRemoteCamera(false);
            m_stateManager.setCallParticipantConnectionDown(true);

            m_eventListener->onCallParticipantConnectionDown();
        }
    }

    void PacketHandleController::onConnectionRestoredWithUser(const nlohmann::json& jsonObject) {
        std::string userNicknameHash = jsonObject[NICKNAME_HASH];

        // When the restored user is us: server tells us we're restored (e.g. after server's ping saw
        // pong). Clear connection-down even if not in a call — gives a path out that does not depend
        // solely on RECONNECT_RESULT, which can be lost on an asymmetric link.
        if (m_stateManager.isAuthorized() && crypto::calculateHash(m_stateManager.getMyNickname()) == userNicknameHash) {
            if (m_stateManager.isConnectionDown()) {
                m_stateManager.setConnectionDown(false);
                m_mediaController.notifyConnectionRestored();
                m_eventListener->onConnectionRestored();
            }
            return;
        }

        if (m_stateManager.isActiveCall()) {
            // We received a packet from the server, so our connection is working. Clear connection-down
            // state and stop the reconnect loop; otherwise the UI can show "Connection with participant
            // restored" while "Reconnecting..." keeps spinning.
            if (m_stateManager.isConnectionDown()) {
                m_stateManager.setConnectionDown(false);
                m_mediaController.notifyConnectionRestored();
                m_eventListener->onConnectionRestored();
            }

            // Reset sharing and viewing state as a safety measure (CONNECTION_DOWN_WITH_USER
            // should have done this, but it may have been lost). Mirrors onConnectionDownWithUser.
            m_stateManager.setScreenSharing(false);
            m_stateManager.setCameraSharing(false);
            m_stateManager.setViewingRemoteScreen(false);
            m_stateManager.setViewingRemoteCamera(false);
            m_stateManager.setCallParticipantConnectionDown(false);

            m_eventListener->onCallParticipantConnectionRestored();
        }
    }

    void PacketHandleController::onUserLogout(const nlohmann::json& jsonObject) {
        std::string userNicknameHash = jsonObject[NICKNAME_HASH];

        if (m_stateManager.isOutgoingCall()) {
            const auto& outgoingCall = m_stateManager.getOutgoingCall();
            if (crypto::calculateHash(outgoingCall.getNickname()) == userNicknameHash) {
                m_stateManager.clearCallState();

                m_eventListener->onOutgoingCallTimeout(ErrorCode::user_logout);
            }
        }

        if (m_stateManager.isIncomingCalls()) {
            const auto& incomingCalls = m_stateManager.getIncomingCalls();
            auto it = std::find_if(incomingCalls.begin(), incomingCalls.end(), [&userNicknameHash](const auto& pair) {
                return crypto::calculateHash(pair.first) == userNicknameHash;
                });

            if (it != incomingCalls.end()) {
                const auto& [nickname, incomingCall] = *it;
                std::string nicknameCopy = nickname;
                if (m_stateManager.tryRemoveIncomingCall(nicknameCopy)) {
                    m_eventListener->onIncomingCallExpired(ErrorCode::user_logout, nicknameCopy);
                }
            }
        }

        if (m_stateManager.isActiveCall()) {
            const auto& activeCall = m_stateManager.getActiveCall();
            if (crypto::calculateHash(activeCall.getNickname()) == userNicknameHash) {
                m_stateManager.clearCallState();

                m_eventListener->onCallEndedByRemote(ErrorCode::user_logout);
            }
        }
    }
}