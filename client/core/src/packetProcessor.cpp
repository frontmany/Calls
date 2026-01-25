#include "packetProcessor.h"

#include "jsonType.h"
#include "packetFactory.h"
#include "utilities/logger.h"
#include "utilities/crypto.h"
#include "network/networkController.h"
#include "audio/audioEngine.h"

#include "clientStateManager.h"
#include "errorCode.h"
#include "keyManager.h"
#include "eventListener.h"

using namespace core;
using namespace core::utilities;
using namespace std::chrono_literals;

PacketProcessor::PacketProcessor(ClientStateManager& stateManager,
    KeyManager& keyManager,
    TaskManager<long long, std::milli>& taskManager,
    core::network::NetworkController& networkController,
    core::audio::AudioEngine& audioEngine,
    std::shared_ptr<EventListener> eventListener)
    : m_stateManager(stateManager),
    m_keysManager(keyManager),
    m_taskManager(taskManager),
    m_networkController(networkController),
    m_audioEngine(audioEngine),
    m_eventListener(eventListener)
{
    m_packetHandlers.emplace(PacketType::CONFIRMATION, [this](const nlohmann::json& json) {onConfirmation(json); });
    m_packetHandlers.emplace(PacketType::AUTHORIZATION_RESULT, [this](const nlohmann::json& json) {onAuthorizationResult(json); });
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

void PacketProcessor::processPacket(const unsigned char* data, int length, PacketType type)
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

void PacketProcessor::sendConfirmation(const std::string& userNicknameHash, const std::string& uid) {
    auto myNicknameHash = crypto::calculateHash(m_stateManager.getMyNickname());
    auto packet = PacketFactory::getConfirmationPacket(myNicknameHash, userNicknameHash, uid);

    m_networkController.send(packet, static_cast<uint32_t>(PacketType::CONFIRMATION));
}

void PacketProcessor::onVoice(const unsigned char* data, int length) {
    if (m_stateManager.isActiveCall() && m_audioEngine.isStream()) {
        size_t decryptedLength = static_cast<size_t>(length) - CryptoPP::AES::BLOCKSIZE;
        std::vector<CryptoPP::byte> decryptedData(decryptedLength);

        crypto::AESDecrypt(m_stateManager.getActiveCall().getCallKey(), data, length,
            decryptedData.data(), decryptedData.size()
        );

        m_audioEngine.playAudio(decryptedData.data(), static_cast<int>(decryptedLength));
    }
}

void PacketProcessor::onScreen(const unsigned char* data, int length) {
    if (!m_stateManager.isActiveCall() || !m_stateManager.isViewingRemoteScreen() || !data || length <= 0) return;

    if (length <= CryptoPP::AES::BLOCKSIZE) {
        LOG_WARN("Screen frame too small to decrypt: {} bytes", length);
        return;
    }

    std::vector<unsigned char> decrypted(static_cast<std::size_t>(length) - CryptoPP::AES::BLOCKSIZE);
    auto& callKey = m_stateManager.getActiveCall().getCallKey();

    try {
        crypto::AESDecrypt(callKey,
            data,
            length,
            reinterpret_cast<CryptoPP::byte*>(decrypted.data()),
            decrypted.size());
    }
    catch (const std::exception& e) {
        LOG_ERROR("Failed to decrypt screen frame: {}", e.what());
        return;
    }

    m_eventListener->onIncomingScreen(decrypted);
}

void PacketProcessor::onCamera(const unsigned char* data, int length) {
    if (!m_stateManager.isActiveCall() || !m_stateManager.isViewingRemoteCamera() || !data || length <= 0) return;

    if (length <= CryptoPP::AES::BLOCKSIZE) {
        LOG_WARN("Camera frame too small to decrypt: {} bytes", length);
        return;
    }

    std::vector<unsigned char> decrypted(static_cast<std::size_t>(length) - CryptoPP::AES::BLOCKSIZE);
    auto& callKey = m_stateManager.getActiveCall().getCallKey();

    try {
        crypto::AESDecrypt(callKey,
            data,
            length,
            reinterpret_cast<CryptoPP::byte*>(decrypted.data()),
            decrypted.size());
    }
    catch (const std::exception& e) {
        LOG_ERROR("Failed to decrypt camera frame: {}", e.what());
        return;
    }

    m_eventListener->onIncomingCamera(decrypted);
}

void PacketProcessor::onConfirmation(const nlohmann::json& jsonObject) {
    const std::string& uid = jsonObject[UID];

    if (m_taskManager.hasTask(uid)) {
        m_taskManager.completeTask(uid, jsonObject);
    }
}

void PacketProcessor::onAuthorizationResult(const nlohmann::json& jsonObject)
{
    const std::string& uid = jsonObject[UID];

    if (m_taskManager.hasTask(uid)) {
        m_taskManager.completeTask(uid, jsonObject);
    }
}

void PacketProcessor::onReconnectResult(const nlohmann::json& jsonObject)
{
    if (!jsonObject.contains(UID)) {
        LOG_WARN("RECONNECT_RESULT packet missing UID, ignoring");
        return;
    }
    std::string uid = jsonObject[UID].get<std::string>();

    if (m_taskManager.hasTask(uid)) {
        m_taskManager.completeTask(uid, jsonObject);
    }
    else {
        LOG_WARN("RECONNECT_RESULT for unknown task uid, task may have already timed out");
    }
}

void PacketProcessor::setOrphanReconnectSuccessHandler(std::function<void(std::optional<nlohmann::json>)> f)
{
    m_onOrphanReconnectSuccess = std::move(f);
}

void PacketProcessor::onUserInfoResult(const nlohmann::json& jsonObject)
{
    const std::string& uid = jsonObject[UID];

    if (m_taskManager.hasTask(uid)) {
        m_taskManager.completeTask(uid, jsonObject);
    }
}

void PacketProcessor::onIncomingCallBegin(const nlohmann::json& jsonObject)
{
    auto packetKey = crypto::RSADecryptAESKey(m_keysManager.getMyPrivateKey(), jsonObject[PACKET_KEY]);
    std::string senderNickname = crypto::AESDecrypt(packetKey, jsonObject[SENDER_ENCRYPTED_NICKNAME]);
    std::string senderNicknameHash = jsonObject[SENDER_NICKNAME_HASH];
    std::string uid = jsonObject[UID];
    auto senderPublicKey = crypto::deserializePublicKey(jsonObject[SENDER_PUBLIC_KEY]);
    auto callKey = crypto::RSADecryptAESKey(m_keysManager.getMyPrivateKey(), jsonObject[ENCRYPTED_CALL_KEY]);

    sendConfirmation(senderNicknameHash, uid);

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

void PacketProcessor::onIncomingCallEnded(const nlohmann::json& jsonObject)
{
    std::string uid = jsonObject[UID];
    std::string senderNicknameHash = jsonObject[SENDER_NICKNAME_HASH];
    std::string myNicknameHash = jsonObject[RECEIVER_NICKNAME_HASH];

    sendConfirmation(senderNicknameHash, uid);

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

void PacketProcessor::onCallAccepted(const nlohmann::json& jsonObject)
{
    std::string uid = jsonObject[UID];
    std::string senderNicknameHash = jsonObject[SENDER_NICKNAME_HASH];
    std::string myNicknameHash = jsonObject[RECEIVER_NICKNAME_HASH];

    auto packetKey = crypto::RSADecryptAESKey(m_keysManager.getMyPrivateKey(), jsonObject[PACKET_KEY]);
    std::string senderNickname = crypto::AESDecrypt(packetKey, jsonObject[SENDER_ENCRYPTED_NICKNAME]);
    auto senderPublicKey = crypto::deserializePublicKey(jsonObject[SENDER_PUBLIC_KEY]);
    auto callKey = crypto::RSADecryptAESKey(m_keysManager.getMyPrivateKey(), jsonObject[ENCRYPTED_CALL_KEY]);


    sendConfirmation(senderNicknameHash, uid);

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

void PacketProcessor::onCallDeclined(const nlohmann::json& jsonObject)
{
    std::string uid = jsonObject[UID];
    std::string senderNicknameHash = jsonObject[SENDER_NICKNAME_HASH];
    std::string myNicknameHash = jsonObject[RECEIVER_NICKNAME_HASH];

    sendConfirmation(senderNicknameHash, uid);

    if (!m_stateManager.isAuthorized() || m_stateManager.isConnectionDown() || !m_stateManager.isOutgoingCall()) return;

    m_stateManager.clearCallState();

    m_eventListener->onOutgoingCallDeclined();
}


void PacketProcessor::onCallEnded(const nlohmann::json& jsonObject)
{
    std::string uid = jsonObject[UID];
    std::string senderNicknameHash = jsonObject[SENDER_NICKNAME_HASH];
    std::string myNicknameHash = jsonObject[RECEIVER_NICKNAME_HASH];

    sendConfirmation(senderNicknameHash, uid);

    if (!m_stateManager.isAuthorized() || m_stateManager.isConnectionDown() || !m_stateManager.isActiveCall() || crypto::calculateHash(m_stateManager.getActiveCall().getNickname()) != senderNicknameHash) return;

    m_stateManager.setScreenSharing(false);
    m_stateManager.setCameraSharing(false);
    m_stateManager.setViewingRemoteScreen(false);
    m_stateManager.setViewingRemoteCamera(false);
    m_stateManager.clearCallState();
    m_audioEngine.stopStream();

    m_eventListener->onCallEndedByRemote({});
}

void PacketProcessor::onScreenSharingBegin(const nlohmann::json& jsonObject)
{
    std::string uid = jsonObject[UID];
    std::string senderNicknameHash = jsonObject[SENDER_NICKNAME_HASH];
    std::string myNicknameHash = jsonObject[RECEIVER_NICKNAME_HASH];

    sendConfirmation(senderNicknameHash, uid);

    if (!m_stateManager.isAuthorized() || m_stateManager.isConnectionDown() || !m_stateManager.isActiveCall() || m_stateManager.isViewingRemoteScreen()) return;

    m_stateManager.setViewingRemoteScreen(true);
    
    m_eventListener->onIncomingScreenSharingStarted();
}

void PacketProcessor::onScreenSharingEnded(const nlohmann::json& jsonObject)
{
    std::string uid = jsonObject[UID];
    std::string senderNicknameHash = jsonObject[SENDER_NICKNAME_HASH];
    std::string myNicknameHash = jsonObject[RECEIVER_NICKNAME_HASH];

    sendConfirmation(senderNicknameHash, uid);

    if (!m_stateManager.isAuthorized() || m_stateManager.isConnectionDown() || !m_stateManager.isActiveCall() || !m_stateManager.isViewingRemoteScreen()) return;

    m_stateManager.setViewingRemoteScreen(false);

    m_eventListener->onIncomingScreenSharingStopped();
}

void PacketProcessor::onCameraSharingBegin(const nlohmann::json& jsonObject)
{
    std::string uid = jsonObject[UID];
    std::string senderNicknameHash = jsonObject[SENDER_NICKNAME_HASH];
    std::string myNicknameHash = jsonObject[RECEIVER_NICKNAME_HASH];

    sendConfirmation(senderNicknameHash, uid);

    if (!m_stateManager.isAuthorized() || m_stateManager.isConnectionDown() || !m_stateManager.isActiveCall() || m_stateManager.isViewingRemoteCamera()) return;

    m_stateManager.setViewingRemoteCamera(true);

    m_eventListener->onIncomingCameraSharingStarted();
}

void PacketProcessor::onCameraSharingEnded(const nlohmann::json& jsonObject)
{
    std::string uid = jsonObject[UID];
    std::string senderNicknameHash = jsonObject[SENDER_NICKNAME_HASH];
    std::string myNicknameHash = jsonObject[RECEIVER_NICKNAME_HASH];

    sendConfirmation(senderNicknameHash, uid);

    if (!m_stateManager.isAuthorized() || m_stateManager.isConnectionDown() || !m_stateManager.isActiveCall() || !m_stateManager.isViewingRemoteCamera()) return;

    m_stateManager.setViewingRemoteCamera(false);

    m_eventListener->onIncomingCameraSharingStopped();
}

void PacketProcessor::onConnectionDownWithUser(const nlohmann::json& jsonObject) {
    std::string uid = jsonObject[UID];
    std::string userNicknameHash = jsonObject[NICKNAME_HASH];

    sendConfirmation("server", uid);

    if (m_stateManager.isAuthorized() && crypto::calculateHash(m_stateManager.getMyNickname()) == userNicknameHash) {
        if (m_stateManager.isInReconnectGracePeriod()) {
            return;
        }
        m_networkController.notifyConnectionDown();
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

void PacketProcessor::onConnectionRestoredWithUser(const nlohmann::json& jsonObject) {
    std::string uid = jsonObject[UID];

    sendConfirmation("server", uid);

    if (m_stateManager.isActiveCall()) {
        // We received a packet from the server, so our connection is working. Clear connection-down
        // state and stop the reconnect loop; otherwise the UI can show "Connection with participant
        // restored" while "Reconnecting..." keeps spinning.
        if (m_stateManager.isConnectionDown()) {
            m_stateManager.setConnectionDown(false);
            m_networkController.notifyConnectionRestored();
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

void PacketProcessor::onUserLogout(const nlohmann::json& jsonObject) {
    std::string uid = jsonObject[UID];
    std::string userNicknameHash = jsonObject[NICKNAME_HASH];

    sendConfirmation("server", uid);

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
