#include "packetProcessor.h"

#include "jsonTypes.h"
#include "utilities/logger.h"
#include "utilities/crypto.h"
#include "utilities/timer.h"
#include "call.h"
#include "incomingCallData.h"
#include "keysManager.h"
#include "callbacksInterface.h"
#include "audio/audioEngine.h"

#include <algorithm>
#include <chrono>

using namespace calls;
using namespace utilities;
using namespace std::chrono_literals;

PacketProcessor::PacketProcessor(
    ClientStateManager& stateManager,
    KeysManager& keysManager,
    CallbacksInterface* callbackHandler,
    audio::AudioEngine* audioEngine,
    SendPacketCallback sendPacket,
    SendConfirmationCallback sendConfirmation,
    SendDeclineCallCallback sendDeclineCall,
    SendStartCallingCallback sendStartCalling,
    OnPacketConfirmedCallback onPacketConfirmed)
    : m_stateManager(stateManager)
    , m_keysManager(keysManager)
    , m_callbackHandler(callbackHandler)
    , m_audioEngine(audioEngine)
    , m_sendPacket(sendPacket)
    , m_sendConfirmation(sendConfirmation)
    , m_sendDeclineCall(sendDeclineCall)
    , m_sendStartCalling(sendStartCalling)
    , m_onPacketConfirmed(onPacketConfirmed)
{
    m_receiveHandlers.emplace(PacketType::AUTHORIZE_SUCCESS, [this](const nlohmann::json& json) { onAuthorizationSuccess(json); });
    m_receiveHandlers.emplace(PacketType::AUTHORIZE_FAIL, [this](const nlohmann::json& json) { onAuthorizationFail(json); });
    m_receiveHandlers.emplace(PacketType::GET_FRIEND_INFO_SUCCESS, [this](const nlohmann::json& json) { onFriendInfoSuccess(json); });
    m_receiveHandlers.emplace(PacketType::GET_FRIEND_INFO_FAIL, [this](const nlohmann::json& json) { onFriendInfoFail(json); });
    m_receiveHandlers.emplace(PacketType::START_CALLING_OK, [this](const nlohmann::json& json) { onStartCallingOk(json); });
    m_receiveHandlers.emplace(PacketType::START_CALLING_FAIL, [this](const nlohmann::json& json) { onStartCallingFail(json); });
    m_receiveHandlers.emplace(PacketType::END_CALL_OK, [this](const nlohmann::json& json) { onEndCallOk(json); });
    m_receiveHandlers.emplace(PacketType::STOP_CALLING_OK, [this](const nlohmann::json& json) { onStopCallingOk(json); });
    m_receiveHandlers.emplace(PacketType::CALL_ACCEPTED_OK, [this](const nlohmann::json& json) { onCallAcceptedOk(json); });
    m_receiveHandlers.emplace(PacketType::CALL_ACCEPTED_FAIL, [this](const nlohmann::json& json) { onCallAcceptedFail(json); });
    m_receiveHandlers.emplace(PacketType::CALL_DECLINED_OK, [this](const nlohmann::json& json) { onCallDeclinedOk(json); });
    m_receiveHandlers.emplace(PacketType::LOGOUT_OK, [this](const nlohmann::json& json) { onLogoutOk(json); });
    m_receiveHandlers.emplace(PacketType::CALL_ACCEPTED, [this](const nlohmann::json& json) { onCallAccepted(json); });
    m_receiveHandlers.emplace(PacketType::CALL_DECLINED, [this](const nlohmann::json& json) { onCallDeclined(json); });
    m_receiveHandlers.emplace(PacketType::STOP_CALLING, [this](const nlohmann::json& json) { onStopCalling(json); });
    m_receiveHandlers.emplace(PacketType::END_CALL, [this](const nlohmann::json& json) { onEndCall(json); });
    m_receiveHandlers.emplace(PacketType::START_CALLING, [this](const nlohmann::json& json) { onIncomingCall(json); });
    m_receiveHandlers.emplace(PacketType::START_SCREEN_SHARING, [this](const nlohmann::json& json) { onIncomingScreenSharingStarted(json); });
    m_receiveHandlers.emplace(PacketType::STOP_SCREEN_SHARING, [this](const nlohmann::json& json) { onIncomingScreenSharingStopped(json); });
    m_receiveHandlers.emplace(PacketType::START_SCREEN_SHARING_OK, [this](const nlohmann::json& json) { onScreenSharingStartedOk(json); });
    m_receiveHandlers.emplace(PacketType::START_SCREEN_SHARING_FAIL, [this](const nlohmann::json& json) { onScreenSharingStartedFail(json); });
    m_receiveHandlers.emplace(PacketType::STOP_SCREEN_SHARING_OK, [this](const nlohmann::json& json) { onScreenSharingStoppedOk(json); });
    m_receiveHandlers.emplace(PacketType::START_CAMERA_SHARING, [this](const nlohmann::json& json) { onIncomingCameraSharingStarted(json); });
    m_receiveHandlers.emplace(PacketType::STOP_CAMERA_SHARING, [this](const nlohmann::json& json) { onIncomingCameraSharingStopped(json); });
    m_receiveHandlers.emplace(PacketType::START_CAMERA_SHARING_OK, [this](const nlohmann::json& json) { onCameraSharingStartedOk(json); });
    m_receiveHandlers.emplace(PacketType::START_CAMERA_SHARING_FAIL, [this](const nlohmann::json& json) { onCameraSharingStartedFail(json); });
    m_receiveHandlers.emplace(PacketType::STOP_CAMERA_SHARING_OK, [this](const nlohmann::json& json) { onCameraSharingStoppedOk(json); });
}

void PacketProcessor::processPacket(PacketType type, const nlohmann::json& jsonObject)
{
    auto& handlers = m_stateManager.getReceiveHandlers();
    if (handlers.contains(type))
    {
        handlers[type](jsonObject);
    }
}

bool PacketProcessor::validatePacket(const nlohmann::json& jsonObject)
{
    if (!jsonObject.contains(UUID))
    {
        LOG_WARN("Packet validation failed: missing UUID");
        return false;
    }
    return true;
}

void PacketProcessor::onAuthorizationSuccess(const nlohmann::json& jsonObject)
{
    if (!validatePacket(jsonObject)) return;

    std::string uuid = jsonObject[UUID].get<std::string>();
    if (m_onPacketConfirmed)
    {
        m_onPacketConfirmed(uuid);
    }

    log::LOG_INFO("User authorized successfully");
    std::lock_guard<std::mutex> lock(m_stateManager.getMutex());
    m_stateManager.setState(ClientState::FREE);
    m_stateManager.getCallbacksQueue().push([this]() {
        if (m_callbackHandler)
        {
            m_callbackHandler->onAuthorizationResult(ErrorCode::OK);
        }
    });
}

void PacketProcessor::onAuthorizationFail(const nlohmann::json& jsonObject)
{
    if (!validatePacket(jsonObject)) return;

    std::string uuid = jsonObject[UUID].get<std::string>();
    if (m_onPacketConfirmed)
    {
        m_onPacketConfirmed(uuid);
    }

    LOG_WARN("Authorization failed - nickname already taken");
    std::lock_guard<std::mutex> lock(m_stateManager.getMutex());
    m_stateManager.clearMyNickname();
    m_stateManager.getCallbacksQueue().push([this]() {
        if (m_callbackHandler)
        {
            m_callbackHandler->onAuthorizationResult(ErrorCode::TAKEN_NICKNAME);
        }
    });
}

void PacketProcessor::onLogoutOk(const nlohmann::json& jsonObject)
{
    if (!validatePacket(jsonObject)) return;

    std::string uuid = jsonObject[UUID].get<std::string>();
    if (m_onPacketConfirmed)
    {
        m_onPacketConfirmed(uuid);
    }
}

void PacketProcessor::onCallAccepted(const nlohmann::json& jsonObject)
{
    std::lock_guard<std::mutex> lock(m_stateManager.getMutex());

    if (m_stateManager.getState() != ClientState::CALLING) return;

    std::string nicknameHash = jsonObject[NICKNAME_HASH_SENDER].get<std::string>();
    if (calculateHash(m_stateManager.getNicknameWhomCalling()) != nicknameHash) return;

    log::LOG_INFO("Call accepted by {}", m_stateManager.getOutgoingCallNickname());
    m_stateManager.getCallState().getOutgoingCall().stop();
    m_stateManager.clearCallState();
    m_stateManager.setState(ClientState::BUSY);

    for (auto& [timer, incomingCallData] : m_stateManager.getIncomingCalls())
    {
        timer->stop();
        m_sendDeclineCall(incomingCallData.nickname, true);
    }

    m_sendConfirmation(jsonObject, PacketType::CALL_ACCEPTED_OK);

    m_stateManager.getCallbacksQueue().push([this]() {
        if (m_callbackHandler)
        {
            m_callbackHandler->onCallingAccepted();
        }
        if (m_audioEngine)
        {
            m_audioEngine->startStream();
        }
    });
}

void PacketProcessor::onCallDeclined(const nlohmann::json& jsonObject)
{
    bool needConfirmation = jsonObject[NEED_CONFIRMATION].get<bool>();
    if (needConfirmation)
    {
        m_sendConfirmation(jsonObject, PacketType::CALL_DECLINED_OK);
    }

    std::lock_guard<std::mutex> lock(m_stateManager.getMutex());

    if (!m_stateManager.isOutgoingCall()) return;

    m_stateManager.getCallState().getOutgoingCall().stop();
    m_stateManager.clearCallState();

    m_stateManager.getCallbacksQueue().push([this]() {
        if (m_callbackHandler)
        {
            m_callbackHandler->onCallingDeclined();
        }
    });
}

void PacketProcessor::onStopCalling(const nlohmann::json& jsonObject)
{
    std::lock_guard<std::mutex> lock(m_stateManager.getMutex());

    if (m_stateManager.getState() == ClientState::UNAUTHORIZED || m_stateManager.getIncomingCalls().size() == 0) return;

    bool needConfirmation = jsonObject[NEED_CONFIRMATION].get<bool>();
    if (needConfirmation)
    {
        m_sendConfirmation(jsonObject, PacketType::STOP_CALLING_OK);
    }

    const std::string& friendNicknameHash = jsonObject[NICKNAME_HASH_SENDER];
    auto& incomingCalls = m_stateManager.getIncomingCalls();
    auto it = std::find_if(incomingCalls.begin(), incomingCalls.end(), [&friendNicknameHash](const auto& pair) {
        return calculateHash(pair.second.nickname) == friendNicknameHash;
    });

    if (it == incomingCalls.end()) return;

    it->first->stop();
    std::string nickname = it->second.nickname;
    m_stateManager.getCallbacksQueue().push([this, nickname]() {
        if (m_callbackHandler)
        {
            m_callbackHandler->onIncomingCallExpired(nickname);
        }
    });
    incomingCalls.erase(it);
}

void PacketProcessor::onEndCall(const nlohmann::json& jsonObject)
{
    bool needConfirmation = jsonObject[NEED_CONFIRMATION].get<bool>();
    if (needConfirmation)
    {
        m_sendConfirmation(jsonObject, PacketType::END_CALL_OK);
    }

    std::lock_guard<std::mutex> lock(m_stateManager.getMutex());

    if (m_stateManager.getState() != ClientState::BUSY) return;

    const std::string& senderNicknameHash = jsonObject[NICKNAME_HASH_SENDER].get<std::string>();
    if (!m_stateManager.getCall() || senderNicknameHash != m_stateManager.getCall()->getFriendNicknameHash()) return;

    m_stateManager.setViewingRemoteScreen(false);

    log::LOG_INFO("Call ended by remote user");
    m_stateManager.setState(ClientState::FREE);
    m_stateManager.clearCall();
    m_stateManager.getCallbacksQueue().push([this]() {
        if (m_audioEngine)
        {
            m_audioEngine->stopStream();
        }
        if (m_callbackHandler)
        {
            m_callbackHandler->onRemoteUserEndedCall();
        }
    });
}

void PacketProcessor::onIncomingCall(const nlohmann::json& jsonObject)
{
    std::lock_guard<std::mutex> lock(m_stateManager.getMutex());

    if (m_stateManager.getState() == ClientState::UNAUTHORIZED) return;

    m_sendConfirmation(jsonObject, PacketType::START_CALLING_OK);

    auto packetAesKey = RSADecryptAESKey(m_keysManager.getPrivateKey(), jsonObject[PACKET_KEY]);
    std::string nickname = AESDecrypt(packetAesKey, jsonObject[NICKNAME]);
    log::LOG_INFO("Incoming call from {}", nickname);
    auto callKey = RSADecryptAESKey(m_keysManager.getPrivateKey(), jsonObject[CALL_KEY]);

    auto& incomingCalls = m_stateManager.getIncomingCalls();
    auto it = std::find_if(incomingCalls.begin(), incomingCalls.end(), [nickname](const auto& pair) {
        return pair.second.nickname == nickname;
    });

    if (it != incomingCalls.end()) return;

    IncomingCallData incomingCallData(nickname, deserializePublicKey(jsonObject[PUBLIC_KEY]), callKey);
    auto timer = std::make_unique<utilities::tic::SingleShotTimer>();

    timer->start(32s, [this, nickname]() {
        std::lock_guard<std::mutex> lock(m_stateManager.getMutex());

        auto& incomingCalls = m_stateManager.getIncomingCalls();
        auto it = std::find_if(incomingCalls.begin(), incomingCalls.end(), [&nickname](const auto& pair) {
            return pair.second.nickname == nickname;
        });

        if (it != incomingCalls.end())
        {
            m_stateManager.getCallbacksQueue().push([this, nickname]() {
                std::lock_guard<std::mutex> lock(m_stateManager.getMutex());
                auto& incomingCalls = m_stateManager.getIncomingCalls();
                auto it = std::find_if(incomingCalls.begin(), incomingCalls.end(), [&nickname](const auto& pair) {
                    return pair.second.nickname == nickname;
                });
                if (it != incomingCalls.end())
                {
                    incomingCalls.erase(it);
                }
                if (m_callbackHandler)
                {
                    m_callbackHandler->onIncomingCallExpired(nickname);
                }
            });
        }
    });

    incomingCalls.emplace_back(std::move(timer), std::move(incomingCallData));

    m_stateManager.getCallbacksQueue().push([this, nickname]() {
        if (m_callbackHandler)
        {
            m_callbackHandler->onIncomingCall(nickname);
        }
    });
}

void PacketProcessor::onCallAcceptedOk(const nlohmann::json& jsonObject)
{
    if (!validatePacket(jsonObject)) return;

    std::string uuid = jsonObject[UUID].get<std::string>();
    if (m_onPacketConfirmed)
    {
        m_onPacketConfirmed(uuid);
    }

    std::lock_guard<std::mutex> lock(m_stateManager.getMutex());

    std::string nicknameHash = jsonObject[NICKNAME_HASH_SENDER];

    auto& incomingCalls = m_stateManager.getIncomingCalls();
    auto it = std::find_if(incomingCalls.begin(), incomingCalls.end(), [&nicknameHash](const auto& pair) {
        return calculateHash(pair.second.nickname) == nicknameHash;
    });

    if (it == incomingCalls.end()) return;

    m_stateManager.setState(ClientState::BUSY);
    m_stateManager.setCall(Call(it->second));
    m_stateManager.setViewingRemoteScreen(false);

    incomingCalls.clear();
    const std::string friendNickname = m_stateManager.getCall()->getFriendNickname();

    m_stateManager.getCallbacksQueue().push([this, friendNickname]() {
        if (m_audioEngine)
        {
            m_audioEngine->refreshAudioDevices();
            m_audioEngine->startStream();
        }
        if (m_callbackHandler)
        {
            m_callbackHandler->onAcceptCallResult(ErrorCode::OK, friendNickname);
        }
    });
}

void PacketProcessor::onCallAcceptedFail(const nlohmann::json& jsonObject)
{
    if (!validatePacket(jsonObject)) return;

    std::lock_guard<std::mutex> lock(m_stateManager.getMutex());

    std::string nicknameHash = jsonObject[NICKNAME_HASH_RECEIVER];

    auto& incomingCalls = m_stateManager.getIncomingCalls();
    auto it = std::find_if(incomingCalls.begin(), incomingCalls.end(), [&nicknameHash](const auto& pair) {
        return calculateHash(pair.second.nickname) == nicknameHash;
    });

    if (it == incomingCalls.end()) return;

    std::string nickname = it->second.nickname;
    m_stateManager.getCallbacksQueue().push([this, nickname]() {
        if (m_callbackHandler)
        {
            m_callbackHandler->onAcceptCallResult(ErrorCode::UNEXISTING_USER, nickname);
        }
    });

    m_stateManager.clearCall();
    incomingCalls.erase(it);
}

void PacketProcessor::onCallDeclinedOk(const nlohmann::json& jsonObject)
{
    if (!validatePacket(jsonObject)) return;

    std::string uuid = jsonObject[UUID].get<std::string>();
    if (m_onPacketConfirmed)
    {
        m_onPacketConfirmed(uuid);
    }
}

void PacketProcessor::onScreenSharingStartedOk(const nlohmann::json& jsonObject)
{
    if (!validatePacket(jsonObject)) return;

    std::string uuid = jsonObject[UUID].get<std::string>();
    if (m_onPacketConfirmed)
    {
        m_onPacketConfirmed(uuid);
    }

    std::lock_guard<std::mutex> lock(m_stateManager.getMutex());
    m_stateManager.setScreenSharing(true);
    log::LOG_INFO("Screen sharing started successfully");

    m_stateManager.getCallbacksQueue().push([this]() {
        if (m_callbackHandler)
        {
            m_callbackHandler->onScreenSharingStarted();
        }
    });
}

void PacketProcessor::onScreenSharingStartedFail(const nlohmann::json& jsonObject)
{
    if (!validatePacket(jsonObject)) return;

    std::lock_guard<std::mutex> lock(m_stateManager.getMutex());
    m_stateManager.setScreenSharing(false);
    LOG_WARN("Screen sharing start rejected by server");

    m_stateManager.getCallbacksQueue().push([this]() {
        if (m_callbackHandler)
        {
            m_callbackHandler->onStartScreenSharingError();
        }
    });
}

void PacketProcessor::onScreenSharingStoppedOk(const nlohmann::json& jsonObject)
{
    if (!validatePacket(jsonObject)) return;

    std::string uuid = jsonObject[UUID].get<std::string>();
    if (m_onPacketConfirmed)
    {
        m_onPacketConfirmed(uuid);
    }
}

void PacketProcessor::onIncomingScreenSharingStarted(const nlohmann::json& jsonObject)
{
    std::lock_guard<std::mutex> lock(m_stateManager.getMutex());

    if (m_stateManager.getState() != ClientState::BUSY) return;

    m_sendConfirmation(jsonObject, PacketType::START_SCREEN_SHARING_OK);
    m_stateManager.setViewingRemoteScreen(true);

    m_stateManager.getCallbacksQueue().push([this]() {
        if (m_callbackHandler)
        {
            m_callbackHandler->onIncomingScreenSharingStarted();
        }
    });
}

void PacketProcessor::onIncomingScreenSharingStopped(const nlohmann::json& jsonObject)
{
    std::lock_guard<std::mutex> lock(m_stateManager.getMutex());

    if (m_stateManager.getState() != ClientState::BUSY) return;

    bool needConfirmation = jsonObject[NEED_CONFIRMATION].get<bool>();
    if (needConfirmation)
    {
        m_sendConfirmation(jsonObject, PacketType::STOP_SCREEN_SHARING_OK);
    }

    m_stateManager.setViewingRemoteScreen(false);

    m_stateManager.getCallbacksQueue().push([this]() {
        if (m_callbackHandler)
        {
            m_callbackHandler->onIncomingScreenSharingStopped();
        }
    });
}

void PacketProcessor::onCameraSharingStartedOk(const nlohmann::json& jsonObject)
{
    if (!validatePacket(jsonObject)) return;

    std::string uuid = jsonObject[UUID].get<std::string>();
    if (m_onPacketConfirmed)
    {
        m_onPacketConfirmed(uuid);
    }

    std::lock_guard<std::mutex> lock(m_stateManager.getMutex());
    m_stateManager.setCameraSharing(true);
    log::LOG_INFO("Camera sharing started successfully");

    m_stateManager.getCallbacksQueue().push([this]() {
        if (m_callbackHandler)
        {
            m_callbackHandler->onCameraSharingStarted();
        }
    });
}

void PacketProcessor::onCameraSharingStartedFail(const nlohmann::json& jsonObject)
{
    if (!validatePacket(jsonObject)) return;

    std::lock_guard<std::mutex> lock(m_stateManager.getMutex());
    m_stateManager.setCameraSharing(false);
    LOG_WARN("Camera sharing start rejected by server");

    m_stateManager.getCallbacksQueue().push([this]() {
        if (m_callbackHandler)
        {
            m_callbackHandler->onStartCameraSharingError();
        }
    });
}

void PacketProcessor::onCameraSharingStoppedOk(const nlohmann::json& jsonObject)
{
    if (!validatePacket(jsonObject)) return;

    std::string uuid = jsonObject[UUID].get<std::string>();
    if (m_onPacketConfirmed)
    {
        m_onPacketConfirmed(uuid);
    }
}

void PacketProcessor::onIncomingCameraSharingStarted(const nlohmann::json& jsonObject)
{
    std::lock_guard<std::mutex> lock(m_stateManager.getMutex());

    if (m_stateManager.getState() != ClientState::BUSY) return;

    m_sendConfirmation(jsonObject, PacketType::START_CAMERA_SHARING_OK);
    m_stateManager.setViewingRemoteCamera(true);

    m_stateManager.getCallbacksQueue().push([this]() {
        if (m_callbackHandler)
        {
            m_callbackHandler->onIncomingCameraSharingStarted();
        }
    });
}

void PacketProcessor::onIncomingCameraSharingStopped(const nlohmann::json& jsonObject)
{
    std::lock_guard<std::mutex> lock(m_stateManager.getMutex());

    if (m_stateManager.getState() != ClientState::BUSY) return;

    bool needConfirmation = jsonObject[NEED_CONFIRMATION].get<bool>();
    if (needConfirmation)
    {
        m_sendConfirmation(jsonObject, PacketType::STOP_CAMERA_SHARING_OK);
    }

    m_stateManager.setViewingRemoteCamera(false);

    m_stateManager.getCallbacksQueue().push([this]() {
        if (m_callbackHandler)
        {
            m_callbackHandler->onIncomingCameraSharingStopped();
        }
    });
}

void PacketProcessor::onFriendInfoSuccess(const nlohmann::json& jsonObject)
{
    if (!validatePacket(jsonObject)) return;

    std::string uuid = jsonObject[UUID].get<std::string>();
    if (m_onPacketConfirmed)
    {
        m_onPacketConfirmed(uuid);
    }

    std::lock_guard<std::mutex> lock(m_stateManager.getMutex());

    m_stateManager.setCall(Call(jsonObject[NICKNAME_HASH], m_stateManager.getNicknameWhomCalling(), deserializePublicKey(jsonObject[PUBLIC_KEY])));
    m_stateManager.getCall()->createCallKey();

    m_sendStartCalling(m_stateManager.getCall()->getFriendNickname(), m_stateManager.getCall()->getFriendPublicKey(), m_stateManager.getCall()->getCallKey());
}

void PacketProcessor::onFriendInfoFail(const nlohmann::json& jsonObject)
{
    if (!validatePacket(jsonObject)) return;

    std::string uuid = jsonObject[UUID].get<std::string>();
    if (m_onPacketConfirmed)
    {
        m_onPacketConfirmed(uuid);
    }

    std::lock_guard<std::mutex> lock(m_stateManager.getMutex());

    m_stateManager.setState(ClientState::FREE);
    m_stateManager.clearNicknameWhomCalling();

    m_stateManager.getCallbacksQueue().push([this]() {
        if (m_callbackHandler)
        {
            m_callbackHandler->onStartCallingResult(ErrorCode::UNEXISTING_USER);
        }
    });
}

void PacketProcessor::onStartCallingOk(const nlohmann::json& jsonObject)
{
    if (!validatePacket(jsonObject)) return;

    std::string uuid = jsonObject[UUID].get<std::string>();
    if (m_onPacketConfirmed)
    {
        m_onPacketConfirmed(uuid);
    }

    std::lock_guard<std::mutex> lock(m_stateManager.getMutex());

    m_stateManager.getCallState().getOutgoingCall().getTimer().start(32s, [this]() {
        std::lock_guard<std::mutex> lock(m_stateManager.getMutex());
        m_stateManager.getCallbacksQueue().push([this]() {
            if (m_callbackHandler)
            {
                m_callbackHandler->onMaximumCallingTimeReached();
            }
        });
    });

    m_stateManager.getCallbacksQueue().push([this]() {
        if (m_callbackHandler)
        {
            m_callbackHandler->onStartCallingResult(ErrorCode::OK);
        }
    });
}

void PacketProcessor::onStartCallingFail(const nlohmann::json& jsonObject)
{
    if (!validatePacket(jsonObject)) return;

    std::lock_guard<std::mutex> lock(m_stateManager.getMutex());

    m_stateManager.setState(ClientState::FREE);
    m_stateManager.clearCall();
    m_stateManager.clearNicknameWhomCalling();

    m_stateManager.getCallbacksQueue().push([this]() {
        if (m_callbackHandler)
        {
            m_callbackHandler->onStartCallingResult(ErrorCode::UNEXISTING_USER);
        }
    });
}

void PacketProcessor::onStopCallingOk(const nlohmann::json& jsonObject)
{
    if (!validatePacket(jsonObject)) return;

    std::string uuid = jsonObject[UUID].get<std::string>();
    if (m_onPacketConfirmed)
    {
        m_onPacketConfirmed(uuid);
    }
}

void PacketProcessor::onEndCallOk(const nlohmann::json& jsonObject)
{
    if (!validatePacket(jsonObject)) return;

    std::string uuid = jsonObject[UUID].get<std::string>();
    if (m_onPacketConfirmed)
    {
        m_onPacketConfirmed(uuid);
    }
}

