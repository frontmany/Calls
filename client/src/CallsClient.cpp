#include "callsClient.h"
#include "packetsFactory.h"
#include "logger.h"
#include "jsonTypes.h"

#include "json.hpp"

using namespace calls;
using namespace std::chrono_literals;

CallsClient& CallsClient::get()
{
    static CallsClient s_instance;
    return s_instance;
}

bool CallsClient::init(
    const std::string& host,
    const std::string& port,
    std::unique_ptr<Handler>&& handler)
{
    m_handler = std::move(handler);
    m_networkController = std::make_shared<NetworkController>();
    m_audioEngine = std::make_unique<AudioEngine>();
    m_pingManager = std::make_shared<PingManager>(m_networkController, [this]() {onPingFail(); });
    m_keysManager = std::make_unique<KeysManager>();

    m_keysManager->generateKeys();

    if (m_audioEngine->init([this](const unsigned char* data, int length) {
        onInputVoice(data, length);
        }) == AudioEngine::InitializationStatus::INITIALIZED
        &&
            m_networkController->init(host, port,
                [this](const unsigned char* data, int length, PacketType type) {
                    onReceive(data, length, type);
                },
                [this]() {
                    std::lock_guard<std::mutex> lock(m_callbacksQueueMutex);
                    m_timer.stop();
                    m_callbacksQueue.push([this]() {m_handler->onNetworkError(); });
                })) {
        return true;
    }
    else {
        return false;
    }
}

void CallsClient::run() {
    m_running = true;
    m_callbacksQueueProcessingThread = std::thread(&CallsClient::processQueue, this);
    m_networkController->run();
    m_pingManager->start();
}

void CallsClient::onPingFail() {
    logout();
    std::lock_guard<std::mutex> lock(m_callbacksQueueMutex);
    m_callbacksQueue.push([this]() {m_handler->onNetworkError(); });
}

CallsClient::~CallsClient() {
    completeShutdown();
}

void CallsClient::onReceive(const unsigned char* data, int length, PacketType type) {
    if (type == PacketType::VOICE) {
        if (m_call && m_audioEngine && m_audioEngine->isStream()) {
            size_t decryptedLength = static_cast<size_t>(length) - CryptoPP::AES::BLOCKSIZE;
            std::vector<CryptoPP::byte> decryptedData(decryptedLength);

            crypto::AESDecrypt(m_call.value().getCallKey(), data, length,
                decryptedData.data(), decryptedData.size()
            );

            m_audioEngine->playAudio(decryptedData.data(), static_cast<int>(decryptedLength));
        }
        return;
    }

    std::lock_guard<std::mutex> lock(m_callbacksQueueMutex);

    switch (type) {
    case (PacketType::AUTHORIZE_SUCCESS):
        onAuthorizationSuccess(data, length);
        break;

    case (PacketType::AUTHORIZE_FAIL):
        onAuthorizationFail(data, length);
        break;

    case (PacketType::LOGOUT_OK):
        onLogoutOk(data, length);
        break;

    case (PacketType::LOGOUT_AND_STOP_OK):
        onLogoutAndStopOk(data, length);
        break;

    case (PacketType::GET_FRIEND_INFO_SUCCESS):
        onFriendInfoSuccess(data, length);
        break;

    case (PacketType::GET_FRIEND_INFO_FAIL):
        onFriendInfoFail(data, length);
        break;

    case (PacketType::CREATE_CALL_SUCCESS):
        onCreateCallSuccess(data, length);
        m_callbacksQueue.push([this]() {m_handler->onStartCallingResult(true); });
        break;

    case (PacketType::CREATE_CALL_FAIL):
        onCreateCallFail(data, length);
        break;

    case (PacketType::END_CALL_OK):
        onEndCallOk(data, length);
        break;

    case (PacketType::CALLING_END_OK):
        onCallingEndOk(data, length);
        break;

    case (PacketType::CALL_ACCEPTED_OK):
        onCallAcceptedOk(data, length);
        break;

    case (PacketType::CALL_DECLINED_OK):
        onCallDeclinedOk(data, length);
        break;

    case (PacketType::PING):
        m_networkController->send(PacketType::PING_SUCCESS);
        break;

    case (PacketType::PING_SUCCESS):
        m_pingManager->setPingSuccess();
        break;

    case (PacketType::CALL_ACCEPTED):
        onCallAccepted(data, length);
        break;

    case (PacketType::CALL_DECLINED):
        onCallDeclined(data, length);
        break;

    case (PacketType::CALLING_END):
        onCallingEnd(data, length);
        break;

    case (PacketType::END_CALL):
        onEndCall(data, length);
        break;

    case (PacketType::CREATE_CALL):
        onCreateCall(data, length);
        break;
    }
}

void CallsClient::onCallAccepted(const unsigned char* data, int length) {
    if (m_state != State::CALLING) return;

    m_timer.stop();
    m_nicknameWhomCalling.clear();
    m_state = State::BUSY;

    if (m_call && m_audioEngine) {
        m_callbacksQueue.push([this]() {m_handler->onCallingAccepted(); m_audioEngine->startStream(); });
    }
}

void CallsClient::onCallDeclined(const unsigned char* data, int length) {
    if (m_state != State::CALLING) return;

    m_timer.stop();
    m_state = State::FREE;
    m_call = std::nullopt;
    m_nicknameWhomCalling.clear();
    m_callbacksQueue.push([this]() {m_handler->onCallingDeclined(); });
}

void CallsClient::onCallingEnd(const unsigned char* data, int length) {
    std::lock_guard<std::mutex> lock(m_incomingCallsMutex);
    if (m_state == State::UNAUTHORIZED || m_incomingCalls.size() == 0) return;

    nlohmann::json jsonObject = nlohmann::json::parse(data, data + length);
    std::string friendNicknameHash = jsonObject[NICKNAME_HASH];

    auto it = std::find_if(m_incomingCalls.begin(), m_incomingCalls.end(), [&friendNicknameHash](const auto& pair) {
        return crypto::calculateHash(pair.second.friendNickname) == friendNicknameHash;
        });

    if (it == m_incomingCalls.end()) return;

    it->first->stop();
    m_callbacksQueue.push([this, nickname = std::move(it->second.friendNickname)]() {m_handler->onIncomingCallExpired(nickname); });
    m_incomingCalls.erase(it);
}

void CallsClient::onEndCall(const unsigned char* data, int length) {
    if (m_state != State::BUSY) return;

    m_audioEngine->stopStream();
    m_state = State::FREE;
    m_call = std::nullopt;
    m_callbacksQueue.push([this]() {m_handler->onRemoteUserEndedCall(); });
}

void CallsClient::onCreateCall(const unsigned char* data, int length) {
    if (m_state == State::UNAUTHORIZED) return;

    nlohmann::json jsonObject = nlohmann::json::parse(data, data + length);

    auto packetAesKey = crypto::RSADecryptAESKey(m_keysManager->getPrivateKey(), jsonObject[PACKET_KEY]);
    std::string nickname = crypto::AESDecrypt(packetAesKey, jsonObject[NICKNAME]);
    auto callKey = crypto::RSADecryptAESKey(m_keysManager->getPrivateKey(), jsonObject[CALL_KEY]);

    IncomingCallData incomingCallData(nickname, crypto::deserializePublicKey(jsonObject[PUBLIC_KEY]), callKey);
    auto timer = std::make_unique<Timer>();

    timer->start(std::chrono::seconds(32), [this, nickname]() {
        std::lock_guard<std::mutex> lock(m_incomingCallsMutex);

        auto it = std::find_if(m_incomingCalls.begin(), m_incomingCalls.end(), [&nickname](const auto& pair) {
            return pair.second.friendNickname == nickname;
            });

        if (it != m_incomingCalls.end()) {
            m_callbacksQueue.push([this, nickname]() {
                m_handler->onIncomingCallExpired(nickname);
                });

            m_incomingCalls.erase(it);
        }
        });

    {
        std::lock_guard<std::mutex> lock(m_incomingCallsMutex);
        m_incomingCalls.emplace_back(std::move(timer), std::move(incomingCallData));
    }

    m_callbacksQueue.push([this, nickname]() {m_handler->onIncomingCall(nickname); });
}

State CallsClient::getState() const {
    return m_state;
}

void CallsClient::mute(bool isMute) {
    m_audioEngine->mute(isMute);
}

bool CallsClient::isMuted() {
    return m_audioEngine->isMuted();
}

void CallsClient::refreshAudioDevices() {
    m_audioEngine->refreshAudioDevices();
}

int CallsClient::getInputVolume() const {
    return m_audioEngine->getInputVolume();
}

int CallsClient::getOutputVolume() const {
    return m_audioEngine->getOutputVolume();
}

void CallsClient::setInputVolume(int volume) {
    m_audioEngine->setInputVolume(volume);
}

void CallsClient::setOutputVolume(int volume) {
    m_audioEngine->setOutputVolume(volume);
}

bool CallsClient::isRunning() const {
    return m_running;
}

int CallsClient::getIncomingCallsCount() const {
    std::lock_guard<std::mutex> lock(m_incomingCallsMutex);
    return m_incomingCalls.size();
}

const std::string& CallsClient::getNickname() const {
    static const std::string emptyNickname;
    if (m_state == State::UNAUTHORIZED) return emptyNickname;
    return m_myNickname;
}

const std::string& CallsClient::getNicknameWhomCalling() const {
    static const std::string emptyNickname;
    if (m_state == State::UNAUTHORIZED || m_state != State::CALLING) return emptyNickname;
    return m_nicknameWhomCalling;
}

const std::string& CallsClient::getNicknameInCallWith() const {
    static const std::string emptyNickname;
    if (m_state == State::UNAUTHORIZED || m_state != State::BUSY || !m_call) return emptyNickname;
    return m_call.value().getFriendNickname();
}

bool CallsClient::initiateShutdown() {
    if (m_state == State::UNAUTHORIZED || !m_waitingForConfirmationOnPacketUUID.empty()) return false;
    bool result = resolveLogout(PacketType::LOGOUT_AND_STOP);
    return result;
}

void CallsClient::completeShutdown() {
    if (!m_running) return;

    m_running = false;

    if (m_pingManager) {
        m_pingManager->stop();
    }

    if (m_audioEngine && m_audioEngine->isStream()) {
        m_audioEngine->stopStream();
    }

    if (m_networkController && !m_networkController->stopped()) {
        m_networkController->stop();
    }

    if (m_callbacksQueueProcessingThread.joinable()) {
        m_callbacksQueueProcessingThread.join();
    }

    m_myNickname.clear();
    m_nicknameWhomCalling.clear();
    if (m_keysManager) m_keysManager->resetKeys();

    {
        std::lock_guard<std::mutex> lock(m_incomingCallsMutex);
        m_incomingCalls.clear();
    }

    m_call = std::nullopt;

    {
        std::lock_guard<std::mutex> lock(m_callbacksQueueMutex);
        std::queue<std::function<void()>> empty;
        std::swap(m_callbacksQueue, empty);
    }

    m_state = State::UNAUTHORIZED;
}

bool CallsClient::logout() {
    if (m_state == State::UNAUTHORIZED || !m_waitingForConfirmationOnPacketUUID.empty()) return false;
    bool result = resolveLogout(PacketType::LOGOUT);
    return result;
}

bool CallsClient::resolveLogout(PacketType type) {
    if (m_state == State::UNAUTHORIZED || !m_waitingForConfirmationOnPacketUUID.empty()) return false;
    if (type != PacketType::LOGOUT && type != PacketType::LOGOUT_AND_STOP) return false;

    std::lock_guard<std::mutex> lock(m_incomingCallsMutex);

    std::vector<std::string> nicknamesToDecline;
    nicknamesToDecline.reserve(m_incomingCalls.size());
    for (auto& [timer, incomingCallData] : m_incomingCalls) {
        timer->stop();
        nicknamesToDecline.push_back(incomingCallData.friendNickname);
    }
    m_incomingCalls.clear();

    std::pair<std::string, std::string> packetPair;
    if (m_state == State::CALLING)
        packetPair = PacketsFactory::getLogoutPacket(m_myNickname, nicknamesToDecline, m_nicknameWhomCalling);
    else
        packetPair = PacketsFactory::getLogoutPacket(m_myNickname, nicknamesToDecline);

    m_waitingForConfirmationOnPacketUUID = packetPair.first;
    m_networkController->send(std::move(packetPair.second), type);

    m_timer.start(4s, [this, type]() {
        std::lock_guard<std::mutex> lock(m_callbacksQueueMutex);
        if (type == PacketType::LOGOUT)
            m_callbacksQueue.push([this]() {m_handler->onLogoutResult(false); });
        else
            m_callbacksQueue.push([this]() {m_handler->onStopResult(false); });
        });

    return true;
}

bool CallsClient::authorize(const std::string& nickname) {
    if (m_state != State::UNAUTHORIZED || !m_waitingForConfirmationOnPacketUUID.empty()) return false;

    m_myNickname = nickname;

    auto [uuid, packet] = PacketsFactory::getAuthorizationPacket(nickname, m_keysManager->getPublicKey());
    m_waitingForConfirmationOnPacketUUID = uuid;

    if (m_networkController)
        m_networkController->send(
            std::move(packet),
            PacketType::AUTHORIZE
        );

    m_timer.start(4s, [this]() {
        std::lock_guard<std::mutex> lock(m_callbacksQueueMutex);
        m_myNickname = "";
        m_callbacksQueue.push([this]() {m_handler->onAuthorizationResult(false); });
        });

    return true;
}

bool CallsClient::endCall() {
    if (m_state == State::UNAUTHORIZED || !m_call || !m_waitingForConfirmationOnPacketUUID.empty()) return false;
    m_state = State::FREE;
    if (m_audioEngine) m_audioEngine->stopStream();

    auto [uuid, packet] = PacketsFactory::getEndCallPacket(m_myNickname);
    m_waitingForConfirmationOnPacketUUID = uuid;

    m_networkController->send(std::move(packet), PacketType::END_CALL);

    m_timer.start(4s, [this]() {
        std::lock_guard<std::mutex> lock(m_callbacksQueueMutex);
        m_callbacksQueue.push([this]() {m_handler->onEndCallResult(false); });
        });

    return true;
}

bool CallsClient::startCalling(const std::string& friendNickname) {
    if (m_state == State::UNAUTHORIZED || m_call || !m_waitingForConfirmationOnPacketUUID.empty()) return false;

    {
        std::lock_guard<std::mutex> lock(m_incomingCallsMutex);
        for (auto& [timer, incomingCall] : m_incomingCalls) {
            if (crypto::calculateHash(incomingCall.friendNickname) == crypto::calculateHash(friendNickname)) {

                std::lock_guard<std::mutex> lock(m_callbacksQueueMutex);
                m_callbacksQueue.push([this, friendNickname]() {m_handler->onCallingSomeoneWhoAlreadyCallingYou(friendNickname); });
                return false;
            }
        }
    }
    auto [uuid, packet] = PacketsFactory::getRequestFriendInfoPacket(m_myNickname, friendNickname);
    m_waitingForConfirmationOnPacketUUID = uuid;

    m_nicknameWhomCalling = friendNickname;

    m_networkController->send(
        std::move(packet),
        PacketType::GET_FRIEND_INFO
    );

    m_timer.start(4s, [this]() {
        std::lock_guard<std::mutex> lock(m_callbacksQueueMutex);
        m_nicknameWhomCalling.clear();
        m_callbacksQueue.push([this]() {m_handler->onStartCallingResult(false); });
        });

    return true;
}

bool CallsClient::stopCalling() {
    if (m_state != State::CALLING || !m_waitingForConfirmationOnPacketUUID.empty()) return false;

    m_callingTimer.stop();

    auto [uuid, packet] = PacketsFactory::getCallingEndPacket(m_myNickname, m_call.value().getFriendNicknameHash());
    m_waitingForConfirmationOnPacketUUID = uuid;

    m_networkController->send(
        std::move(packet),
        PacketType::CALLING_END
    );

    m_timer.start(4s, [this]() {
        std::lock_guard<std::mutex> lock(m_callbacksQueueMutex);
        m_callbacksQueue.push([this]() {m_handler->onCallingStoppedResult(false); });
        });

    return true;
}

bool CallsClient::declineIncomingCall(const std::string& friendNickname) {
    if (m_state == State::UNAUTHORIZED || m_incomingCalls.empty() || !m_waitingForConfirmationOnPacketUUID.empty()) return false;

    std::lock_guard<std::mutex> lock(m_incomingCallsMutex);

    auto it = std::find_if(m_incomingCalls.begin(), m_incomingCalls.end(), [&friendNickname](const auto& pair) {
        return pair.second.friendNickname == friendNickname;
        });
    if (it == m_incomingCalls.end()) return false;

    auto [uuid, packet] = PacketsFactory::getDeclineCallPacket(m_myNickname, friendNickname);
    m_waitingForConfirmationOnPacketUUID = uuid;

    m_networkController->send(std::move(packet), PacketType::CALL_DECLINED);

    m_timer.start(4s, [this]() {
        std::lock_guard<std::mutex> lock(m_callbacksQueueMutex);
        m_callbacksQueue.push([this]() {m_handler->onDeclineIncomingCallResult(false); });
        });

    return true;
}

bool CallsClient::acceptIncomingCall(const std::string& friendNickname) {
    if (m_state == State::UNAUTHORIZED || m_incomingCalls.empty() || !m_waitingForConfirmationOnPacketUUID.empty()) return false;

    std::lock_guard<std::mutex> lock(m_incomingCallsMutex);
    auto it = std::find_if(m_incomingCalls.begin(), m_incomingCalls.end(), [&friendNickname](const auto& pair) {
        return pair.second.friendNickname == friendNickname;
        });
    if (it == m_incomingCalls.end()) return false;

    std::vector<std::string> nicknamesToDecline;
    nicknamesToDecline.reserve(m_incomingCalls.size());
    for (auto& incomingPair : m_incomingCalls) {
        incomingPair.first->stop();
        nicknamesToDecline.push_back(incomingPair.second.friendNickname);
    }
    m_incomingCalls.clear();

    std::pair<std::string, std::string> packetPair;
    if (m_state == State::CALLING) {
        packetPair = PacketsFactory::getAcceptCallPacketAndEndCalling(m_myNickname, friendNickname, nicknamesToDecline, m_call.value().getFriendNickname());
    }
    else {
        packetPair = PacketsFactory::getAcceptCallPacket(m_myNickname, friendNickname, nicknamesToDecline);
    }

    m_waitingForConfirmationOnPacketUUID = packetPair.first;
    m_networkController->send(std::move(packetPair.second), PacketType::CALL_ACCEPTED);

    m_timer.start(4s, [this]() {
        std::lock_guard<std::mutex> lock(m_callbacksQueueMutex);
        m_callbacksQueue.push([this]() {m_handler->onAcceptIncomingCallResult(false); });
        });

    return true;
}

void CallsClient::processQueue() {
    while (m_running) {
        std::function<void()> callback;
        {
            std::lock_guard<std::mutex> lock(m_callbacksQueueMutex);
            if (!m_callbacksQueue.empty()) {
                callback = std::move(m_callbacksQueue.front());
                m_callbacksQueue.pop();
            }
        }

        if (callback) {
            callback();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void CallsClient::onAuthorizationSuccess(const unsigned char* data, int length) {
    bool success = validatePacket(data, length);
    if (!success) return;

    m_state = State::FREE;
    m_callbacksQueue.push([this]() {m_handler->onAuthorizationResult(true); });
}

void CallsClient::onAuthorizationFail(const unsigned char* data, int length) {
    bool success = validatePacket(data, length);
    if (!success) return;

    m_myNickname = "";
    m_callbacksQueue.push([this]() {m_handler->onAuthorizationResult(false); });
}

void CallsClient::onLogoutOk(const unsigned char* data, int length) {
    bool success = validatePacket(data, length);
    if (!success) return;

    m_state = State::UNAUTHORIZED;
    m_myNickname.clear();
    m_nicknameWhomCalling.clear();
    m_keysManager->resetKeys();
    m_incomingCalls.clear();
    m_call = std::nullopt;

    m_callbacksQueue.push([this]() {m_handler->onLogoutResult(true); });
}

void CallsClient::onLogoutAndStopOk(const unsigned char* data, int length) {
    bool success = validatePacket(data, length);
    if (!success) return;
    m_callbacksQueue.push([this]() {m_handler->onStopResult(true); });
}

void CallsClient::onCallAcceptedOk(const unsigned char* data, int length) {
    bool success = validatePacket(data, length);
    if (!success) return;

    nlohmann::json jsonObject = nlohmann::json::parse(data, data + length);
    std::string nicknameHash = jsonObject[NICKNAME_HASH];

    std::lock_guard<std::mutex> lock(m_incomingCallsMutex);
    auto it = std::find_if(m_incomingCalls.begin(), m_incomingCalls.end(), [&nicknameHash](const auto& pair) {
        return crypto::calculateHash(pair.second.friendNickname) == nicknameHash;
        });

    if (it == m_incomingCalls.end()) return;

    it->first->stop();
    m_state = State::BUSY;
    m_call = Call(it->second);
    m_incomingCalls.erase(it);

    m_audioEngine->refreshAudioDevices();
    if (m_audioEngine) m_audioEngine->startStream();
}

void CallsClient::onCallDeclinedOk(const unsigned char* data, int length) {
    bool success = validatePacket(data, length);
    if (!success) return;

    nlohmann::json jsonObject = nlohmann::json::parse(data, data + length);
    std::string nicknameHash = jsonObject[NICKNAME_HASH];

    std::lock_guard<std::mutex> lock(m_incomingCallsMutex);
    auto it = std::find_if(m_incomingCalls.begin(), m_incomingCalls.end(), [&nicknameHash](const auto& pair) {
        return crypto::calculateHash(pair.second.friendNickname) == nicknameHash;
        });
    if (it == m_incomingCalls.end()) return;

    it->first->stop();
    m_incomingCalls.erase(it);
}

void CallsClient::onCreateCallFail(const unsigned char* data, int length) {
    bool success = validatePacket(data, length);
    if (!success) return;

    m_state = State::FREE;
    m_call = std::nullopt;
    m_nicknameWhomCalling.clear();

    m_callbacksQueue.push([this]() {m_handler->onStartCallingResult(false); });
}

void CallsClient::onEndCallOk(const unsigned char* data, int length) {
    bool success = validatePacket(data, length);
    if (!success) return;

    m_call = std::nullopt;
    m_audioEngine->refreshAudioDevices();

    std::lock_guard<std::mutex> lock(m_callbacksQueueMutex);
    m_callbacksQueue.push([this]() {m_handler->onEndCallResult(true); });
}

void CallsClient::onFriendInfoSuccess(const unsigned char* data, int length) {
    bool success = validatePacket(data, length);
    if (!success) return;

    nlohmann::json jsonObject = nlohmann::json::parse(data, data + length);

    m_call = Call(jsonObject[NICKNAME_HASH], m_nicknameWhomCalling, crypto::deserializePublicKey(jsonObject[PUBLIC_KEY]));
    m_call.value().createCallKey();
    m_state = State::CALLING;

    auto [uuid, packet] = PacketsFactory::getCreateCallPacket(m_myNickname, m_keysManager->getPublicKey(), m_call->getFriendNickname(), m_call->getFriendPublicKey(), m_call->getCallKey());
    m_waitingForConfirmationOnPacketUUID = uuid;

    m_networkController->send(
        std::move(packet),
        PacketType::CREATE_CALL
    );

    m_timer.start(4s, [this]() {
        m_state = State::FREE;
        m_call = std::nullopt;
        m_nicknameWhomCalling.clear();

        std::lock_guard<std::mutex> lock(m_callbacksQueueMutex);
        m_callbacksQueue.push([this]() {m_handler->onStartCallingResult(false); });
        });
}

void CallsClient::onFriendInfoFail(const unsigned char* data, int length) {
    bool success = validatePacket(data, length);
    if (!success) return;

    m_nicknameWhomCalling.clear();
    m_callbacksQueue.push([this]() {m_handler->onStartCallingResult(false); });
}

void CallsClient::onCreateCallSuccess(const unsigned char* data, int length) {
    bool success = validatePacket(data, length);
    if (!success) return;

    m_callingTimer.start(32s, [this]() {
        auto [uuid, packet] = PacketsFactory::getCallingEndPacket(m_myNickname, m_call.value().getFriendNickname());
        m_waitingForConfirmationOnPacketUUID = uuid;

        m_networkController->send(
            std::move(packet),
            PacketType::CALLING_END
        );

        m_timer.start(4s, [this]() {
            std::lock_guard<std::mutex> lock(m_callbacksQueueMutex);
            m_callbacksQueue.push([this]() {m_handler->onCallingStoppedResult(false); });
            });
        });
}

void CallsClient::onCallingEndOk(const unsigned char* data, int length) {
    bool success = validatePacket(data, length);
    if (!success) return;

    m_state = State::FREE;
    m_call = std::nullopt;
    m_nicknameWhomCalling.clear();

    std::lock_guard<std::mutex> lock(m_callbacksQueueMutex);
    m_callbacksQueue.push([this]() {m_handler->onCallingStoppedResult(true); });
}

void CallsClient::onInputVoice(const unsigned char* data, int length) {
    if (m_call) {
        size_t cipherDataLength = static_cast<size_t>(length) + CryptoPP::AES::BLOCKSIZE;
        std::vector<CryptoPP::byte> cipherData(cipherDataLength);
        crypto::AESEncrypt(m_call.value().getCallKey(), data, length, cipherData.data(), cipherDataLength);
        m_networkController->send(std::move(cipherData), PacketType::VOICE);
    }
}

bool CallsClient::validatePacket(const unsigned char* data, int length) {
    nlohmann::json jsonObject = nlohmann::json::parse(data, data + length);
    std::string uuid = jsonObject[UUID].get<std::string>();

    if (m_waitingForConfirmationOnPacketUUID != uuid) return false;

    m_timer.stop();
    m_waitingForConfirmationOnPacketUUID.clear();
    return true;
}