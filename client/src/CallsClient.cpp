#include "callsClient.h"
#include "packetsFactory.h"
#include "logger.h"
#include "jsonTypes.h"



using namespace calls;
using namespace std::chrono_literals;

CallsClient::~CallsClient() {
    stop();
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

    m_handlers.emplace(PacketType::AUTHORIZE_SUCCESS, [this](const nlohmann::json& json) { onAuthorizationSuccess(json); });
    m_handlers.emplace(PacketType::AUTHORIZE_FAIL, [this](const nlohmann::json& json) { onAuthorizationFail(json); });
    m_handlers.emplace(PacketType::GET_FRIEND_INFO_SUCCESS, [this](const nlohmann::json& json) { onFriendInfoSuccess(json); });
    m_handlers.emplace(PacketType::GET_FRIEND_INFO_FAIL, [this](const nlohmann::json& json) { onFriendInfoFail(json); });
    m_handlers.emplace(PacketType::START_CALLING_OK, [this](const nlohmann::json& json) { onStartCallingOk(json); });
    m_handlers.emplace(PacketType::START_CALLING_FAIL, [this](const nlohmann::json& json) { onStartCallingFail(json); });
    m_handlers.emplace(PacketType::END_CALL_OK, [this](const nlohmann::json& json) { onEndCallOk(json); });
    m_handlers.emplace(PacketType::STOP_CALLING_OK, [this](const nlohmann::json& json) { onStopCallingOk(json); });
    m_handlers.emplace(PacketType::CALL_ACCEPTED_OK, [this](const nlohmann::json& json) { onCallAcceptedOk(json); });
    m_handlers.emplace(PacketType::CALL_DECLINED_OK, [this](const nlohmann::json& json) { onCallDeclinedOk(json); });
    m_handlers.emplace(PacketType::LOGOUT_OK, [this](const nlohmann::json& json) { onLogoutOk(json); });
    m_handlers.emplace(PacketType::CALL_ACCEPTED, [this](const nlohmann::json& json) { onCallAccepted(json); });
    m_handlers.emplace(PacketType::CALL_DECLINED, [this](const nlohmann::json& json) { onCallDeclined(json); });
    m_handlers.emplace(PacketType::STOP_CALLING, [this](const nlohmann::json& json) { onStopCalling(json); });
    m_handlers.emplace(PacketType::END_CALL, [this](const nlohmann::json& json) { onEndCall(json); });
    m_handlers.emplace(PacketType::START_CALLING, [this](const nlohmann::json& json) { onIncomingCall(json); });

    m_callbackHandler = std::move(handler);
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
            m_callbacksQueue.push([this]() {m_callbackHandler->onNetworkError(); });
        });

    if (audioInitialized && networkInitialized) 
        return true;
    else 
        return false;
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
    m_callbacksQueue.push([this]() {m_callbackHandler->onNetworkError(); });
}

void CallsClient::onConnectionRestored() {
    std::lock_guard<std::mutex> lock(m_dataMutex);
    m_callbacksQueue.push([this]() {m_callbackHandler->onConnectionRestored(); });
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

bool CallsClient::validatePacket(const nlohmann::json& jsonObject) {
    const std::string& uuid = jsonObject[UUID].get<std::string>();
    if (!m_tasks.contains(uuid)) return false;
    m_tasks.erase(uuid);
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
    else if (type == PacketType::PING){
        m_networkController->send(PacketType::PING_SUCCESS);
    }
    else if (type == PacketType::PING_SUCCESS) {
        m_pingManager->setPingSuccess();
    }

    if (m_handlers.contains(type)) {
        nlohmann::json jsonObject = nlohmann::json::parse(data, data + length);

        auto& handler = m_handlers.at(type);
        handler(jsonObject);
    }
}

void CallsClient::onCallAccepted(const nlohmann::json& jsonObject) {
    if (m_state != State::CALLING) return;

    m_callingTimer.stop();
    m_nicknameWhomCalling.clear();
    m_state = State::BUSY;

    for (auto& [timer, incomingCallData] : m_incomingCalls) {
        timer->stop();
        sendDeclineCallPacket(incomingCallData.friendNickname, true);
    }

    sendConfirmationPacket(jsonObject, PacketType::CALL_ACCEPTED_OK);

    m_callbacksQueue.push([this]() {m_callbackHandler->onCallingAccepted(); m_audioEngine->startStream(); });
}

void CallsClient::onCallDeclined(const nlohmann::json& jsonObject) {
    if (m_state != State::CALLING) return;

    m_callingTimer.stop();
    m_state = State::FREE;
    m_call = std::nullopt;
    m_nicknameWhomCalling.clear();

    bool needConfirmation = jsonObject[NEED_CONFIRMATION].get<bool>();
    if (needConfirmation) 
        sendConfirmationPacket(jsonObject, PacketType::CALL_DECLINED_OK);

    m_callbacksQueue.push([this]() {m_callbackHandler->onCallingDeclined(); });
}

void CallsClient::onStopCalling(const nlohmann::json& jsonObject) {
    if (m_state == State::UNAUTHORIZED || m_incomingCalls.size() == 0) return;

    bool needConfirmation = jsonObject[NEED_CONFIRMATION].get<bool>();
    if (needConfirmation)
        sendConfirmationPacket(jsonObject, PacketType::STOP_CALLING_OK);

    const std::string& friendNicknameHash = jsonObject[NICKNAME_HASH_SENDER];
    auto it = std::find_if(m_incomingCalls.begin(), m_incomingCalls.end(), [&friendNicknameHash](const auto& pair) {
        return crypto::calculateHash(pair.second.friendNickname) == friendNicknameHash;
    });

    if (it == m_incomingCalls.end()) return;

    it->first->stop();
    m_callbacksQueue.push([this, nickname = std::move(it->second.friendNickname)]() {m_callbackHandler->onIncomingCallExpired(nickname); });
    m_incomingCalls.erase(it);
}

void CallsClient::onEndCall(const nlohmann::json& jsonObject) {
    if (m_state != State::BUSY) return;

    bool needConfirmation = jsonObject[NEED_CONFIRMATION].get<bool>();
    if (needConfirmation)
        sendConfirmationPacket(jsonObject, PacketType::END_CALL_OK);

    const std::string& senderNicknameHash = jsonObject[NICKNAME_HASH_SENDER].get<std::string>();
    if (senderNicknameHash != m_call->getFriendNicknameHash()) return;

    m_audioEngine->stopStream();
    m_state = State::FREE;
    m_call = std::nullopt;
    m_callbacksQueue.push([this]() {m_callbackHandler->onRemoteUserEndedCall(); });
}

void CallsClient::onIncomingCall(const nlohmann::json& jsonObject) {
    if (m_state == State::UNAUTHORIZED) return;

    sendConfirmationPacket(jsonObject, PacketType::START_CALLING_OK);

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
            m_callbacksQueue.push([this, nickname]() {m_callbackHandler->onIncomingCallExpired(nickname);});
            m_incomingCalls.erase(it);
        }
    });

    m_incomingCalls.emplace_back(std::move(timer), std::move(incomingCallData));
    
    m_callbacksQueue.push([this, nickname]() {m_callbackHandler->onIncomingCall(nickname); });
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

bool CallsClient::isAuthorized() const {
    std::lock_guard<std::mutex> lock(m_dataMutex);
    return m_state != State::UNAUTHORIZED;
}

bool CallsClient::isCalling() const {
    std::lock_guard<std::mutex> lock(m_dataMutex);
    return m_state == State::CALLING;
}

bool CallsClient::isBusy() const {
    std::lock_guard<std::mutex> lock(m_dataMutex);
    return m_state == State::BUSY;
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

void CallsClient::stop() {
    std::lock_guard<std::mutex> lock(m_dataMutex);

    if (!m_running) return;

    if (m_state == State::CALLING)
        sendStopCallingPacket(false);

    if (m_state == State::BUSY)
        sendEndCallPacket(false);

    for (auto& [timer, incomingCallData] : m_incomingCalls) {
        timer->stop();
        sendDeclineCallPacket(incomingCallData.friendNickname, false);
    }

    sendLogoutPacket(false);

    std::this_thread::sleep_for(250ms);

    m_tasks.clear();

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
    std::lock_guard<std::mutex> lock(m_dataMutex);
    if (m_state == State::UNAUTHORIZED) return false;
    
    m_tasks.clear();

    if (m_state == State::CALLING)
        sendStopCallingPacket(true);

    if (m_state == State::BUSY)
        sendEndCallPacket(true);

    for (auto& [timer, incomingCallData] : m_incomingCalls) {
        timer->stop();
        sendDeclineCallPacket(incomingCallData.friendNickname, true);
    }

    sendLogoutPacket(true);

    return true;
}

bool CallsClient::authorize(const std::string& nickname) {
    std::lock_guard<std::mutex> lock(m_dataMutex);

    if (m_state != State::UNAUTHORIZED) return false;

    m_myNickname = nickname;

    sendAuthorizationPacket();

    return true;
}

bool CallsClient::endCall() {
    std::lock_guard<std::mutex> lock(m_dataMutex);

    if (m_state != State::BUSY) return false;

    m_state = State::FREE;
    m_call = std::nullopt;
    m_audioEngine->stopStream();
    m_audioEngine->refreshAudioDevices();

    sendEndCallPacket(true);

    return true;
}

bool CallsClient::startCalling(const std::string& friendNickname) {
    std::lock_guard<std::mutex> lock(m_dataMutex);

    if (m_state == State::UNAUTHORIZED || m_state == State::BUSY) return false;

    // calling the person who are alredy call us
    for (auto& [timer, incomingCall] : m_incomingCalls) {
        if (incomingCall.friendNickname == friendNickname) {
            acceptCall(friendNickname);
            return true;
        }
    }

    m_state = State::CALLING;
    m_nicknameWhomCalling = friendNickname;

    sendRequestFriendInfoPacket(friendNickname);

    return true;
}

bool CallsClient::stopCalling() {
    std::lock_guard<std::mutex> lock(m_dataMutex);

    if (m_state != State::CALLING) return false;

    m_callingTimer.stop();
    m_state = State::FREE;
    m_call = std::nullopt;
    m_nicknameWhomCalling.clear();

    sendStopCallingPacket(true);

    return true;
}

bool CallsClient::declineCall(const std::string& friendNickname) {
    std::lock_guard<std::mutex> lock(m_dataMutex);

    if (m_state == State::UNAUTHORIZED || m_incomingCalls.empty()) return false;

    auto it = std::find_if(m_incomingCalls.begin(), m_incomingCalls.end(), [&friendNickname](const auto& pair) {
        return pair.second.friendNickname == friendNickname;
    });

    if (it == m_incomingCalls.end()) return false;

    m_incomingCalls.erase(it);

    sendDeclineCallPacket(friendNickname, true);

    return true;
}

bool CallsClient::acceptCall(const std::string& friendNickname) {
    std::lock_guard<std::mutex> lock(m_dataMutex);

    if (m_state == State::UNAUTHORIZED || m_incomingCalls.empty()) return false;

    auto it = std::find_if(m_incomingCalls.begin(), m_incomingCalls.end(), [&friendNickname](const auto& pair) {
        return pair.second.friendNickname == friendNickname;
    });

    if (it == m_incomingCalls.end()) return false;

    if (m_state == State::CALLING)
        sendStopCallingPacket(false);

    if (m_state == State::BUSY)
        sendEndCallPacket(false);

    for (auto& [timer, incomingCallData] : m_incomingCalls) {
        timer->stop();
        sendDeclineCallPacket(incomingCallData.friendNickname, false);
    }

    sendAcceptCallPacket(friendNickname);

    return true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void CallsClient::onAuthorizationSuccess(const nlohmann::json& jsonObject) {
    auto packetValid = validatePacket(jsonObject);
    if (!packetValid) return;
    m_state = State::FREE;
    m_callbacksQueue.push([this]() {m_callbackHandler->onAuthorizationResult(ErrorCode::OK); });
}

void CallsClient::onAuthorizationFail(const nlohmann::json& jsonObject) {
    auto packetValid = validatePacket(jsonObject);
    if (!packetValid) return;
    m_myNickname.clear();
    m_callbacksQueue.push([this]() {m_callbackHandler->onAuthorizationResult(ErrorCode::TAKEN_NICKNAME); });
}

void CallsClient::onLogoutOk(const nlohmann::json& jsonObject) {
    auto packetValid = validatePacket(jsonObject);
    if (!packetValid) return;
}

void CallsClient::onCallAcceptedOk(const nlohmann::json& jsonObject) {
    auto packetValid = validatePacket(jsonObject);
    if (!packetValid) return;

    std::string nicknameHash = jsonObject[NICKNAME_HASH];

    auto it = std::find_if(m_incomingCalls.begin(), m_incomingCalls.end(), [&nicknameHash](const auto& pair) {
        return crypto::calculateHash(pair.second.friendNickname) == nicknameHash;
    });

    if (it == m_incomingCalls.end()) return;

    m_state = State::BUSY;
    m_call = Call(it->second);

    m_incomingCalls.clear();
    m_audioEngine->refreshAudioDevices();
    m_audioEngine->startStream();
    
    m_callbacksQueue.push([this]() {m_callbackHandler->onAcceptCallResult(ErrorCode::OK, m_call.value().getFriendNickname()); });
}

void CallsClient::onCallDeclinedOk(const nlohmann::json& jsonObject) {
    auto packetValid = validatePacket(jsonObject);
    if (!packetValid) return;
}

void CallsClient::onStartCallingFail(const nlohmann::json& jsonObject) {
    auto packetValid = validatePacket(jsonObject);
    if (!packetValid) return;

    m_state = State::FREE;
    m_call = std::nullopt;
    m_nicknameWhomCalling.clear();

    m_callbacksQueue.push([this]() {m_callbackHandler->onStartCallingResult(ErrorCode::UNEXISTING_USER); });
}

void CallsClient::onEndCallOk(const nlohmann::json& jsonObject) {
    auto packetValid = validatePacket(jsonObject);
    if (!packetValid) return;
}

void CallsClient::onFriendInfoSuccess(const nlohmann::json& jsonObject) {
    auto packetValid = validatePacket(jsonObject);
    if (!packetValid) return;

    m_call = Call(jsonObject[NICKNAME_HASH], m_nicknameWhomCalling, crypto::deserializePublicKey(jsonObject[PUBLIC_KEY]));
    m_call.value().createCallKey();

    sendStartCallingPacket(m_call->getFriendNickname(), m_call->getFriendPublicKey(), m_call->getCallKey());
}

void CallsClient::onFriendInfoFail(const nlohmann::json& jsonObject) {
    auto packetValid = validatePacket(jsonObject);
    if (!packetValid) return;

    m_nicknameWhomCalling.clear();
    m_callbacksQueue.push([this]() {m_callbackHandler->onStartCallingResult(ErrorCode::UNEXISTING_USER); });
}

void CallsClient::onStartCallingOk(const nlohmann::json& jsonObject) {
    auto packetValid = validatePacket(jsonObject);
    if (!packetValid) return;

    m_callingTimer.start(32s, [this]() {
        std::lock_guard<std::mutex> lock(m_dataMutex);
        m_callbacksQueue.push([this]() {m_callbackHandler->onMaximumCallingTimeReached(); });
    });

    m_callbacksQueue.push([this]() {m_callbackHandler->onStartCallingResult(ErrorCode::OK); });
}

void CallsClient::onStopCallingOk(const nlohmann::json& jsonObject) {
    auto packetValid = validatePacket(jsonObject);
    if (!packetValid) return;
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











void CallsClient::sendAuthorizationPacket() {
    auto [uuid, packet] = PacketsFactory::getAuthorizationPacket(m_myNickname, m_keysManager->getPublicKey());
    m_networkController->send(std::move(packet), PacketType::AUTHORIZE);

    std::shared_ptr<Task> task = std::make_shared<Task>(m_networkController, uuid, packet, PacketType::AUTHORIZE,
        [this, uuid]() { 
            std::lock_guard<std::mutex> lock(m_dataMutex);

            if (m_tasks.contains(uuid)) {
                auto& task = m_tasks.at(uuid);
                m_callbacksQueue.push([this, task]() {task->retry(); });
            }
        },
        [this, uuid]() {
            std::lock_guard<std::mutex> lock(m_dataMutex);

            if (m_tasks.contains(uuid)) {
                m_callbacksQueue.push([this, uuid]() {m_tasks.erase(uuid); });
            }

            m_callbacksQueue.push([this]() {m_callbackHandler->onAuthorizationResult(ErrorCode::TIMEOUT); });
        },
        1
    );

    m_tasks.emplace(uuid, task);
}

void CallsClient::sendLogoutPacket(bool createTask) {
    auto [uuid, packet] = PacketsFactory::getLogoutPacket(m_myNickname);
    m_networkController->send(std::move(packet), PacketType::LOGOUT);

    if (createTask) {
        std::shared_ptr<Task> task = std::make_shared<Task>(m_networkController, uuid, packet, PacketType::LOGOUT,
            [this, uuid]() {
                std::lock_guard<std::mutex> lock(m_dataMutex);

                if (m_tasks.contains(uuid)) {
                    auto& task = m_tasks.at(uuid);
                    m_callbacksQueue.push([this, task]() {task->retry(); });
                }
            },
            [this, uuid]() {
                std::lock_guard<std::mutex> lock(m_dataMutex);

                if (m_tasks.contains(uuid)) {
                    m_callbacksQueue.push([this, uuid]() {m_tasks.erase(uuid); });
                }
            },
            3
        );

        m_tasks.emplace(uuid, task);
    }
}

void CallsClient::sendRequestFriendInfoPacket(const std::string& friendNickname) {
    auto [uuid, packet] = PacketsFactory::getRequestFriendInfoPacket(m_myNickname, friendNickname);

    std::shared_ptr<Task> task = std::make_shared<Task>(m_networkController, uuid, packet, PacketType::GET_FRIEND_INFO,
        [this, uuid]() {
            std::lock_guard<std::mutex> lock(m_dataMutex);

            if (m_tasks.contains(uuid)) {
                auto& task = m_tasks.at(uuid);
                m_callbacksQueue.push([this, task]() {task->retry(); });
            }
        },
        [this, uuid]() {
            std::lock_guard<std::mutex> lock(m_dataMutex);

            if (m_tasks.contains(uuid)) {
                m_callbacksQueue.push([this, uuid]() {m_tasks.erase(uuid); });
            }

            std::string emptyNickname;
            m_callbacksQueue.push([this, nickname = emptyNickname]() {
                m_state = State::FREE;
                m_nicknameWhomCalling.clear();
                m_callbackHandler->onStartCallingResult(ErrorCode::TIMEOUT);
            });
        },
        3
    );

    m_tasks.emplace(uuid, task);
}

void CallsClient::sendAcceptCallPacket(const std::string& friendNickname) {
    auto [uuid, packet] = PacketsFactory::getAcceptCallPacket(m_myNickname, friendNickname);
    m_networkController->send(std::move(packet), PacketType::CALL_ACCEPTED);

    std::shared_ptr<Task> task = std::make_shared<Task>(m_networkController, uuid, packet, PacketType::CALL_ACCEPTED,
        [this, uuid]() {
            std::lock_guard<std::mutex> lock(m_dataMutex);

            if (m_tasks.contains(uuid)) {
                auto& task = m_tasks.at(uuid);
                m_callbacksQueue.push([this, task]() {task->retry(); });
            }
        },
        [this, uuid, nickname = friendNickname]() {
            std::lock_guard<std::mutex> lock(m_dataMutex);

            if (m_tasks.contains(uuid)) {
                m_callbacksQueue.push([this, uuid]() {m_tasks.erase(uuid); });
            }

            m_callbacksQueue.push([this, nickname]() {
                m_callbackHandler->onAcceptCallResult(ErrorCode::TIMEOUT, nickname);
            });
        },
        3
    );

    m_tasks.emplace(uuid, task);
}

void CallsClient::sendDeclineCallPacket(const std::string& friendNickname, bool createTask) {
    auto [uuid, packet] = PacketsFactory::getDeclineCallPacket(m_myNickname, friendNickname, createTask);
    m_networkController->send(std::move(packet), PacketType::CALL_DECLINED);

    if (createTask) {
        std::shared_ptr<Task> task = std::make_shared<Task>(m_networkController, uuid, packet, PacketType::CALL_DECLINED,
            [this, uuid]() {
                std::lock_guard<std::mutex> lock(m_dataMutex);

                if (m_tasks.contains(uuid)) {
                    auto& task = m_tasks.at(uuid);
                    m_callbacksQueue.push([this, task]() {task->retry(); });
                }
            },
            [this, uuid]() {
                std::lock_guard<std::mutex> lock(m_dataMutex);

                if (m_tasks.contains(uuid)) {
                    m_callbacksQueue.push([this, uuid]() {m_tasks.erase(uuid); });
                }
            },
            3
        );
    }
}

void CallsClient::sendStartCallingPacket(const std::string& friendNickname, const CryptoPP::RSA::PublicKey& friendPublicKey, const CryptoPP::SecByteBlock& callKey) {
    auto [uuid, packet] = PacketsFactory::getStartCallingPacket(m_myNickname, m_keysManager->getPublicKey(), friendNickname, friendPublicKey, callKey);
    m_networkController->send(std::move(packet), PacketType::START_CALLING);

    std::shared_ptr<Task> task = std::make_shared<Task>(m_networkController, uuid, packet, PacketType::START_CALLING,
        [this, uuid]() {
            std::lock_guard<std::mutex> lock(m_dataMutex);

            if (m_tasks.contains(uuid)) {
                auto& task = m_tasks.at(uuid);
                m_callbacksQueue.push([this, task]() {task->retry(); });
            }
        },
        [this, uuid]() {
            std::lock_guard<std::mutex> lock(m_dataMutex);

            if (m_tasks.contains(uuid)) {
                m_callbacksQueue.push([this, uuid]() {m_tasks.erase(uuid); });
            }

            m_state = State::FREE;
            m_call = std::nullopt;
            m_nicknameWhomCalling.clear();

            m_callbacksQueue.push([this]() {m_callbackHandler->onStartCallingResult(ErrorCode::TIMEOUT); });
        },
        3
    );
}

void CallsClient::sendStopCallingPacket(bool createTask) {
    auto [uuid, packet] = PacketsFactory::getDeclineCallPacket(m_myNickname, m_nicknameWhomCalling, createTask);
    m_networkController->send(std::move(packet), PacketType::STOP_CALLING);

    if (createTask) {
        std::shared_ptr<Task> task = std::make_shared<Task>(m_networkController, uuid, packet, PacketType::STOP_CALLING,
            [this, uuid]() {
                std::lock_guard<std::mutex> lock(m_dataMutex);

                if (m_tasks.contains(uuid)) {
                    auto& task = m_tasks.at(uuid);
                    m_callbacksQueue.push([this, task]() {task->retry(); });
                }
            },
            [this, uuid]() {
                std::lock_guard<std::mutex> lock(m_dataMutex);

                if (m_tasks.contains(uuid)) {
                    m_callbacksQueue.push([this, uuid]() {m_tasks.erase(uuid); });
                }
            },
            3
        );
    }
}

void CallsClient::sendEndCallPacket(bool createTask) {
    auto [uuid, packet] = PacketsFactory::getEndCallPacket(m_myNickname, m_call->getFriendNickname(), createTask);
    m_networkController->send(std::move(packet), PacketType::END_CALL);

    if (createTask) {
        std::shared_ptr<Task> task = std::make_shared<Task>(m_networkController, uuid, packet, PacketType::END_CALL,
            [this, uuid]() {
                std::lock_guard<std::mutex> lock(m_dataMutex);

                if (m_tasks.contains(uuid)) {
                    auto& task = m_tasks.at(uuid);
                    m_callbacksQueue.push([this, task]() {task->retry(); });
                }
            },
            [this, uuid]() {
                std::lock_guard<std::mutex> lock(m_dataMutex);

                if (m_tasks.contains(uuid)) {
                    m_callbacksQueue.push([this, uuid]() {m_tasks.erase(uuid); });
                }
            },
            3
        );
    }
}

void CallsClient::sendConfirmationPacket(const nlohmann::json& jsonObject, PacketType type) {
    std::string senderNicknameHash = jsonObject[NICKNAME_HASH_SENDER];
    std::string uuid = jsonObject[UUID];

    auto packet = PacketsFactory::getConfirmationPacket(m_myNickname, senderNicknameHash, uuid);
    m_networkController->send(std::move(packet), type);
}