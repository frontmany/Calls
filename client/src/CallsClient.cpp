#include "callsClient.h"
#include "packetsFactory.h"
#include "logger.h"
#include "jsonTypes.h"

#include "json.hpp"

using namespace calls;
using namespace std::chrono_literals;


CallsClient::~CallsClient() {
    completeShutdown();
}

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
    std::lock_guard<std::mutex> lock(m_dataMutex);

    m_handler = std::move(handler);
    m_networkController = std::make_shared<NetworkController>();
    m_audioEngine = std::make_unique<AudioEngine>();
    m_pingManager = std::make_shared<PingManager>(m_networkController, [this]() {onPingFail(); }, [this]() {onConnectionRestored(); });
    m_keysManager = std::make_unique<KeysManager>();

    bool audioInitialized = m_audioEngine->init([this](const unsigned char* data, int length) {
        onInputVoice(data, length);
    });

    bool networkInitialized = m_networkController->init(host, port,
        [this](const unsigned char* data, int length, PacketType type) {
            onReceive(data, length, type);
        },
        [this]() {
            std::lock_guard<std::mutex> lock(m_dataMutex);
            m_callbacksQueue.push([this]() {m_handler->onNetworkError(); });
        });

    if (audioInitialized && networkInitialized) {
        return true;
    }
    else {
        return false;
    }
}

void CallsClient::run() {
    std::lock_guard<std::mutex> lock(m_dataMutex);

    m_running = true;
    m_keysManager->generateKeys();
    m_callbacksQueueProcessingThread = std::thread(&CallsClient::processQueue, this);
    m_networkController->run();
    m_pingManager->start();
}

void CallsClient::onPingFail() {
    std::lock_guard<std::mutex> lock(m_dataMutex);
    m_callbacksQueue.push([this]() {m_handler->onNetworkError(); });
}

void CallsClient::onConnectionRestored() {
    std::lock_guard<std::mutex> lock(m_dataMutex);
    m_callbacksQueue.push([this]() {m_handler->onConnectionRestored(); });
}

void CallsClient::processQueue() {
    while (m_running) {
        std::function<void()> callback;
        {
            std::lock_guard<std::mutex> lock(m_dataMutex);
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

bool CallsClient::validatePacket(const unsigned char* data, int length) {
    nlohmann::json jsonObject = nlohmann::json::parse(data, data + length);
    std::string uuid = jsonObject[UUID].get<std::string>();

    if (m_waitingForConfirmationOnPacketUUID != uuid) return false;

    m_timer.stop();
    m_waitingForConfirmationOnPacketUUID.clear();
    return true;
}

void CallsClient::onReceive(const unsigned char* data, int length, PacketType type) {
    std::lock_guard<std::mutex> lock(m_dataMutex);

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

    case (PacketType::SHUTDOWN_OK):
        onShutdownOk(data, length);
        break;

    case (PacketType::GET_FRIEND_INFO_SUCCESS):
        onFriendInfoSuccess(data, length);
        break;

    case (PacketType::GET_FRIEND_INFO_FAIL):
        onFriendInfoFail(data, length);
        break;

    case (PacketType::START_CALLING_SUCCESS):
        onStartCallingSuccess(data, length);
        break;

    case (PacketType::START_CALLING_FAIL):
        onStartCallingFail(data, length);
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

    case (PacketType::DECLINE_ALL_OK):
        onAllCallsDeclinedOk(data, length);
        break;

    case (PacketType::CALLING_END):
        onCallingEnd(data, length);
        break;

    case (PacketType::END_CALL):
        onEndCall(data, length);
        break;

    case (PacketType::START_CALLING):
        onIncomingCall(data, length);
        break;
    }
}

void CallsClient::onCallAccepted(const unsigned char* data, int length) {
    if (m_state != State::CALLING) return;

    m_callingTimer.stop();
    m_nicknameWhomCalling.clear();
    m_state = State::BUSY;

     m_callbacksQueue.push([this]() {m_handler->onCallingAccepted(); m_audioEngine->startStream(); });
}

void CallsClient::onCallDeclined(const unsigned char* data, int length) {
    if (m_state != State::CALLING) return;

    m_callingTimer.stop();
    m_state = State::FREE;
    m_call = std::nullopt;
    m_nicknameWhomCalling.clear();
    m_callbacksQueue.push([this]() {m_handler->onCallingDeclined(); });
}

void CallsClient::onCallingEnd(const unsigned char* data, int length) {
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

void CallsClient::onIncomingCall(const unsigned char* data, int length) {
    if (m_state == State::UNAUTHORIZED) return;

    nlohmann::json jsonObject = nlohmann::json::parse(data, data + length);

    auto packetAesKey = crypto::RSADecryptAESKey(m_keysManager->getPrivateKey(), jsonObject[PACKET_KEY]);
    std::string nickname = crypto::AESDecrypt(packetAesKey, jsonObject[NICKNAME]);
    auto callKey = crypto::RSADecryptAESKey(m_keysManager->getPrivateKey(), jsonObject[CALL_KEY]);

    IncomingCallData incomingCallData(nickname, crypto::deserializePublicKey(jsonObject[PUBLIC_KEY]), callKey);
    auto timer = std::make_unique<Timer>();

    timer->start(32s, [this, nickname]() {
        std::lock_guard<std::mutex> lock(m_dataMutex);

        auto it = std::find_if(m_incomingCalls.begin(), m_incomingCalls.end(), [&nickname](const auto& pair) {
            return pair.second.friendNickname == nickname;
        });

        if (it != m_incomingCalls.end()) {
            m_callbacksQueue.push([this, nickname]() {m_handler->onIncomingCallExpired(nickname);});
            m_incomingCalls.erase(it);
        }
    });

    m_incomingCalls.emplace_back(std::move(timer), std::move(incomingCallData));
    
    m_callbacksQueue.push([this, nickname]() {m_handler->onIncomingCall(nickname); });
}

State CallsClient::getState() const {
    return m_state;
}

std::vector<std::string> CallsClient::getCallers() {
    std::lock_guard<std::mutex> lock(m_dataMutex);

    std::vector<std::string> callersNicknames;
    callersNicknames.reserve(m_incomingCalls.size());

    for (auto& [_, incomingCallData] : m_incomingCalls) {
        callersNicknames.push_back(incomingCallData.friendNickname);
    }

    return callersNicknames;
}

void CallsClient::mute(bool isMute) {
    std::lock_guard<std::mutex> lock(m_dataMutex);
    m_audioEngine->mute(isMute);
}

bool CallsClient::isMuted() {
    std::lock_guard<std::mutex> lock(m_dataMutex);
    return m_audioEngine->isMuted();
}

void CallsClient::refreshAudioDevices() {
    std::lock_guard<std::mutex> lock(m_dataMutex);
    m_audioEngine->refreshAudioDevices();
}

int CallsClient::getInputVolume() const {
    std::lock_guard<std::mutex> lock(m_dataMutex);
    return m_audioEngine->getInputVolume();
}

int CallsClient::getOutputVolume() const {
    std::lock_guard<std::mutex> lock(m_dataMutex);
    return m_audioEngine->getOutputVolume();
}

void CallsClient::setInputVolume(int volume) {
    std::lock_guard<std::mutex> lock(m_dataMutex);
    m_audioEngine->setInputVolume(volume);
}

void CallsClient::setOutputVolume(int volume) {
    std::lock_guard<std::mutex> lock(m_dataMutex);
    m_audioEngine->setOutputVolume(volume);
}

bool CallsClient::isRunning() const {
    std::lock_guard<std::mutex> lock(m_dataMutex);
    return m_running;
}

int CallsClient::getIncomingCallsCount() const {
    std::lock_guard<std::mutex> lock(m_dataMutex);
    return m_incomingCalls.size();
}

const std::string& CallsClient::getNickname() const {
    std::lock_guard<std::mutex> lock(m_dataMutex);
    return m_myNickname;
}

const std::string& CallsClient::getNicknameWhomCalling() const {
    std::lock_guard<std::mutex> lock(m_dataMutex);
    return m_nicknameWhomCalling;
}

const std::string& CallsClient::getNicknameInCallWith() const {
    std::lock_guard<std::mutex> lock(m_dataMutex);

    static const std::string emptyNickname;
    if (m_state == State::UNAUTHORIZED || m_state != State::BUSY || !m_call) return emptyNickname;
    return m_call.value().getFriendNickname();
}

bool CallsClient::initiateShutdown() {
    std::lock_guard<std::mutex> lock(m_dataMutex);

    if (m_state == State::UNAUTHORIZED || !m_waitingForConfirmationOnPacketUUID.empty()) return false;
    bool result = resolveLogout(PacketType::SHUTDOWN);
    return result;
}

void CallsClient::completeShutdown() {
    std::lock_guard<std::mutex> lock(m_dataMutex);

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

    m_incomingCalls.clear();
    m_call = std::nullopt;

    std::queue<std::function<void()>> empty;
    std::swap(m_callbacksQueue, empty);
    
    m_state = State::UNAUTHORIZED;
}

bool CallsClient::logout() {
    if (m_state == State::UNAUTHORIZED || !m_waitingForConfirmationOnPacketUUID.empty()) return false;
    bool result = resolveLogout(PacketType::LOGOUT);
    return result;
}

void CallsClient::reset() {
    std::lock_guard<std::mutex> lock(m_dataMutex);
    m_incomingCalls.clear();
    
    m_state = State::UNAUTHORIZED;
    m_myNickname.clear();
    m_nicknameWhomCalling.clear();
    m_keysManager->resetKeys();
    m_call = std::nullopt;
}

bool CallsClient::resolveLogout(PacketType type) {
    if (type != PacketType::LOGOUT && type != PacketType::SHUTDOWN) return false;

    std::vector<std::string> nicknamesToDecline;
    nicknamesToDecline.reserve(m_incomingCalls.size());
    for (auto& [timer, incomingCallData] : m_incomingCalls) {
        nicknamesToDecline.push_back(incomingCallData.friendNickname);
    }

    std::pair<std::string, std::string> packetPair;
    if (m_state == State::CALLING)
        packetPair = PacketsFactory::getLogoutPacket(m_myNickname, nicknamesToDecline, m_nicknameWhomCalling);
    else
        packetPair = PacketsFactory::getLogoutPacket(m_myNickname, nicknamesToDecline);

    m_waitingForConfirmationOnPacketUUID = packetPair.first;
    m_networkController->send(std::move(packetPair.second), type);

    m_timer.start(2s, [this, type]() {
        std::lock_guard<std::mutex> lock(m_dataMutex);
        m_waitingForConfirmationOnPacketUUID.clear();
        if (type == PacketType::LOGOUT)
            m_callbacksQueue.push([this]() {m_handler->onLogoutResult(false); });
        else
            m_callbacksQueue.push([this]() {m_handler->onShutdownResult(false); });
    });

    return true;
}

bool CallsClient::authorize(const std::string& nickname) {
    std::lock_guard<std::mutex> lock(m_dataMutex);

    if (m_state != State::UNAUTHORIZED || !m_waitingForConfirmationOnPacketUUID.empty()) return false;

    m_myNickname = nickname;

    auto [uuid, packet] = PacketsFactory::getAuthorizationPacket(nickname, m_keysManager->getPublicKey());
    m_waitingForConfirmationOnPacketUUID = uuid;

    if (m_networkController)
        m_networkController->send(
            std::move(packet),
            PacketType::AUTHORIZE
        );

    m_timer.start(2s, [this]() {
        std::lock_guard<std::mutex> lock(m_dataMutex);
        m_waitingForConfirmationOnPacketUUID.clear();
        m_myNickname = "";
        m_callbacksQueue.push([this]() {m_handler->onAuthorizationResult(false); });
    });

    return true;
}

bool CallsClient::endCall() {
    std::lock_guard<std::mutex> lock(m_dataMutex);

    if (m_state != State::BUSY || !m_waitingForConfirmationOnPacketUUID.empty()) return false;
    m_state = State::FREE;
    if (m_audioEngine) m_audioEngine->stopStream();

    auto [uuid, packet] = PacketsFactory::getEndCallPacket(m_myNickname);
    m_waitingForConfirmationOnPacketUUID = uuid;

    m_networkController->send(std::move(packet), PacketType::END_CALL);

    m_timer.start(2s, [this]() {
        std::lock_guard<std::mutex> lock(m_dataMutex);
        m_waitingForConfirmationOnPacketUUID.clear();
        m_callbacksQueue.push([this]() {m_handler->onEndCallResult(false); });
    });

    return true;
}

bool CallsClient::startCalling(const std::string& friendNickname) {
    std::lock_guard<std::mutex> lock(m_dataMutex);

    if (m_state == State::UNAUTHORIZED || m_state == State::BUSY || !m_waitingForConfirmationOnPacketUUID.empty()) return false;

    bool callingSomeoneWhoAlreadyCallingUs = false;
    {
        for (auto& [timer, incomingCall] : m_incomingCalls) {
            if (crypto::calculateHash(incomingCall.friendNickname) == crypto::calculateHash(friendNickname)) {
                callingSomeoneWhoAlreadyCallingUs = true;
            }
        }
    }

    if (callingSomeoneWhoAlreadyCallingUs) {
        acceptIncomingCall(friendNickname);
        return true;
    }


    auto [uuid, packet] = PacketsFactory::getRequestFriendInfoPacket(m_myNickname, friendNickname);
    m_waitingForConfirmationOnPacketUUID = uuid;

    m_state = State::CALLING;
    m_nicknameWhomCalling = friendNickname;

    m_networkController->send(
        std::move(packet),
        PacketType::GET_FRIEND_INFO
    );

    m_timer.start(2s, [this]() {
        std::lock_guard<std::mutex> lock(m_dataMutex);
        m_state = State::FREE;
        m_nicknameWhomCalling.clear();
        m_waitingForConfirmationOnPacketUUID.clear();
        m_callbacksQueue.push([this]() {m_handler->onStartCallingResult(false); });
    });

    return true;
}

bool CallsClient::stopCalling() {
    std::lock_guard<std::mutex> lock(m_dataMutex);

    if (m_state != State::CALLING || !m_waitingForConfirmationOnPacketUUID.empty()) return false;

    auto [uuid, packet] = PacketsFactory::getCallingEndPacket(m_myNickname, m_call.value().getFriendNicknameHash());
    m_waitingForConfirmationOnPacketUUID = uuid;

    m_networkController->send(
        std::move(packet),
        PacketType::CALLING_END
    );

    m_timer.start(2s, [this]() {
        std::lock_guard<std::mutex> lock(m_dataMutex);
        m_waitingForConfirmationOnPacketUUID.clear();
        m_callbacksQueue.push([this]() {m_handler->onCallingStoppedResult(false); });
    });

    return true;
}

bool CallsClient::declineIncomingCall(const std::string& friendNickname) {
    std::lock_guard<std::mutex> lock(m_dataMutex);

    if (m_state == State::UNAUTHORIZED || m_incomingCalls.empty() || !m_waitingForConfirmationOnPacketUUID.empty()) return false;

    auto it = std::find_if(m_incomingCalls.begin(), m_incomingCalls.end(), [&friendNickname](const auto& pair) {
        return pair.second.friendNickname == friendNickname;
    });
    if (it == m_incomingCalls.end()) return false;

    auto [uuid, packet] = PacketsFactory::getDeclineCallPacket(m_myNickname, friendNickname);
    m_waitingForConfirmationOnPacketUUID = uuid;

    m_networkController->send(std::move(packet), PacketType::CALL_DECLINED);

    m_timer.start(2s, [this]() {
        std::lock_guard<std::mutex> lock(m_dataMutex);
        m_waitingForConfirmationOnPacketUUID.clear();
        m_callbacksQueue.push([this]() {m_handler->onDeclineIncomingCallResult(false, ""); });
    });

    return true;
}

bool CallsClient::acceptIncomingCall(const std::string& friendNickname) {
    std::lock_guard<std::mutex> lock(m_dataMutex);

    if (m_state == State::UNAUTHORIZED || m_incomingCalls.empty() || !m_waitingForConfirmationOnPacketUUID.empty()) return false;

    bool friendNicknameFound = false;
    std::vector<std::string> nicknamesToDecline;
    nicknamesToDecline.reserve(m_incomingCalls.size());
    for (auto& [_, incomingCallData] : m_incomingCalls) {
        nicknamesToDecline.push_back(incomingCallData.friendNickname);
        if (incomingCallData.friendNickname == friendNickname)
            friendNicknameFound = true;
    }

    if (!friendNicknameFound) return false;
    

    std::pair<std::string, std::string> packetPair;
    if (m_state == State::CALLING) {
        packetPair = PacketsFactory::getAcceptCallPacket(m_myNickname, friendNickname, nicknamesToDecline, m_nicknameWhomCalling);
    }
    else {
        packetPair = PacketsFactory::getAcceptCallPacket(m_myNickname, friendNickname, nicknamesToDecline);
    }

    m_waitingForConfirmationOnPacketUUID = packetPair.first;
    m_networkController->send(std::move(packetPair.second), PacketType::CALL_ACCEPTED);

    m_timer.start(2s, [this]() {
        std::lock_guard<std::mutex> lock(m_dataMutex);
        m_waitingForConfirmationOnPacketUUID.clear();
        m_callbacksQueue.push([this]() {m_handler->onAcceptIncomingCallResult(false, ""); });
    });

    return true;
}

bool CallsClient::declineAll() {
    std::lock_guard<std::mutex> lock(m_dataMutex);

    if (m_state == State::UNAUTHORIZED || m_incomingCalls.empty() || !m_waitingForConfirmationOnPacketUUID.empty()) return false;
    
    std::vector<std::string> nicknamesToDecline;
    nicknamesToDecline.reserve(m_incomingCalls.size());
    for (auto& [timer, incomingCallData] : m_incomingCalls) {
        timer->stop();
        nicknamesToDecline.push_back(incomingCallData.friendNickname);
    }

    auto [uuid, packet] = PacketsFactory::getDeclineAllCallsPacket(m_myNickname, nicknamesToDecline);
    m_waitingForConfirmationOnPacketUUID = uuid;

    m_networkController->send(std::move(packet), PacketType::DECLINE_ALL);

    m_timer.start(2s, [this]() {
        std::lock_guard<std::mutex> lock(m_dataMutex);
        m_waitingForConfirmationOnPacketUUID.clear();
        m_callbacksQueue.push([this]() {m_handler->onAllIncomingCallsDeclinedResult(false); });
    });

    return true;
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

    m_incomingCalls.clear();
    m_state = State::UNAUTHORIZED;
    m_myNickname.clear();
    m_nicknameWhomCalling.clear();
    m_keysManager->resetKeys();
    m_call = std::nullopt;

    m_callbacksQueue.push([this]() {m_handler->onLogoutResult(true); });
}

void CallsClient::onShutdownOk(const unsigned char* data, int length) {
    bool success = validatePacket(data, length);
    if (!success) return;
    m_callbacksQueue.push([this]() {m_handler->onShutdownResult(true); });
}

void CallsClient::onCallAcceptedOk(const unsigned char* data, int length) {
    bool success = validatePacket(data, length);
    if (!success) return;

    nlohmann::json jsonObject = nlohmann::json::parse(data, data + length);
    std::string nicknameHash = jsonObject[NICKNAME_HASH];

    auto it = std::find_if(m_incomingCalls.begin(), m_incomingCalls.end(), [&nicknameHash](const auto& pair) {
        return crypto::calculateHash(pair.second.friendNickname) == nicknameHash;
    });

    if (it == m_incomingCalls.end()) return;

    m_state = State::BUSY;
    m_call = Call(it->second);

    m_incomingCalls.clear();
    m_audioEngine->refreshAudioDevices();
    if (m_audioEngine) m_audioEngine->startStream();

    
    m_callbacksQueue.push([this, nickname = m_call.value().getFriendNickname()]() {m_handler->onAcceptIncomingCallResult(true, nickname); });
}

void CallsClient::onCallDeclinedOk(const unsigned char* data, int length) {
    bool success = validatePacket(data, length);
    if (!success) return;

    nlohmann::json jsonObject = nlohmann::json::parse(data, data + length);
    std::string nicknameHash = jsonObject[NICKNAME_HASH];

    auto it = std::find_if(m_incomingCalls.begin(), m_incomingCalls.end(), [&nicknameHash](const auto& pair) {
        return crypto::calculateHash(pair.second.friendNickname) == nicknameHash;
        });
    if (it == m_incomingCalls.end()) return;

    it->first->stop();
    std::string nickname = it->second.friendNickname;
    m_incomingCalls.erase(it);

    m_callbacksQueue.push([this, nickname]() {m_handler->onDeclineIncomingCallResult(true, nickname); });
}

void CallsClient::onAllCallsDeclinedOk(const unsigned char* data, int length) {
    bool success = validatePacket(data, length);
    if (!success) return;

    m_incomingCalls.clear();

    m_callbacksQueue.push([this]() {m_handler->onAllIncomingCallsDeclinedResult(true); });
}

void CallsClient::onStartCallingFail(const unsigned char* data, int length) {
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
    m_state = State::BUSY;
    m_audioEngine->refreshAudioDevices();

    m_callbacksQueue.push([this]() {m_handler->onEndCallResult(true); });
}

void CallsClient::onFriendInfoSuccess(const unsigned char* data, int length) {
    bool success = validatePacket(data, length);
    if (!success) return;

    nlohmann::json jsonObject = nlohmann::json::parse(data, data + length);

    m_call = Call(jsonObject[NICKNAME_HASH], m_nicknameWhomCalling, crypto::deserializePublicKey(jsonObject[PUBLIC_KEY]));
    m_call.value().createCallKey();

    auto [uuid, packet] = PacketsFactory::getCreateCallPacket(m_myNickname, m_keysManager->getPublicKey(), m_call->getFriendNickname(), m_call->getFriendPublicKey(), m_call->getCallKey());
    m_waitingForConfirmationOnPacketUUID = uuid;

    m_networkController->send(
        std::move(packet),
        PacketType::START_CALLING
    );

    m_timer.start(2s, [this]() {
        std::lock_guard<std::mutex> lock(m_dataMutex);

        m_state = State::FREE;
        m_call = std::nullopt;
        m_nicknameWhomCalling.clear();

        m_callbacksQueue.push([this]() {m_handler->onStartCallingResult(false); });
    });
}

void CallsClient::onFriendInfoFail(const unsigned char* data, int length) {
    bool success = validatePacket(data, length);
    if (!success) return;

    m_nicknameWhomCalling.clear();
    m_callbacksQueue.push([this]() {m_handler->onStartCallingResult(false); });
}

void CallsClient::onStartCallingSuccess(const unsigned char* data, int length) {
    bool success = validatePacket(data, length);
    if (!success) return;

    m_callingTimer.start(32s, [this]() {
        std::lock_guard<std::mutex> lock(m_dataMutex);
        m_callbacksQueue.push([this]() {m_handler->onCallingStoppedResult(true); });
    });

    m_callbacksQueue.push([this]() {m_handler->onStartCallingResult(true); });
}

void CallsClient::onCallingEndOk(const unsigned char* data, int length) {
    bool success = validatePacket(data, length);
    if (!success) return;

    m_callingTimer.stop();

    m_state = State::FREE;
    m_call = std::nullopt;
    m_nicknameWhomCalling.clear();

    m_callbacksQueue.push([this]() {m_handler->onCallingStoppedResult(true); });
}

void CallsClient::onInputVoice(const unsigned char* data, int length) {
    {
        std::lock_guard<std::mutex> lock(m_dataMutex);
        if (!m_call) return;
    }
    
    size_t cipherDataLength = static_cast<size_t>(length) + CryptoPP::AES::BLOCKSIZE;
    std::vector<CryptoPP::byte> cipherData(cipherDataLength);
    crypto::AESEncrypt(m_call.value().getCallKey(), data, length, cipherData.data(), cipherDataLength);
    m_networkController->send(std::move(cipherData), PacketType::VOICE);
}