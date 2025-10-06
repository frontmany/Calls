#include "callsClient.h"
#include "packetsFactory.h"
#include "logger.h"
#include "jsonTypes.h"

#include "json.hpp"

using namespace calls;

CallsClient& CallsClient::get()
{
    static CallsClient s_instance;
    return s_instance;
}

CallsClient::CallsClient()
{
}

bool CallsClient::init(
    const std::string& host,
    const std::string& port,
    std::function<void(bool)> logoutResult,
    std::function<void(bool)> authorizationResult,
    std::function<void(bool)> startCallingResult,
    std::function<void(bool)> stopCallingResult,
    std::function<void(bool)> declineAllIncomingCallsResult,
    std::function<void(bool)> declineIncomingCallResult,
    std::function<void(bool)> acceptIncomingCallResult,
    std::function<void(bool)> endCallResult,
    std::function<void()> onCallingAccepted,
    std::function<void()> onCallingDeclined,
    std::function<void()> onCallingExpired,
    std::function<void(const std::string&)> onIncomingCall,
    std::function<void(const std::string&)> onIncomingCallExpired,
    std::function<void(const std::string&)> onCallingSomeoneWhoAlreadyCallingYou,
    std::function<void()> onRemoteUserEndedCall,
    std::function<void()> onNetworkError)
{
    m_authorizationResult = authorizationResult;
    m_logoutResult = logoutResult;
    m_startCallingResult = startCallingResult;
    m_stopCallingResult = stopCallingResult;
    m_declineAllIncomingCallsResult = declineAllIncomingCallsResult;
    m_declineIncomingCallResult = declineIncomingCallResult;
    m_acceptIncomingCallResult = acceptIncomingCallResult;
    m_endCallResult = endCallResult;
    m_onCallingAccepted = onCallingAccepted;
    m_onCallingDeclined = onCallingDeclined;
    m_onCallingExpired = onCallingExpired;
    m_onNetworkError = onNetworkError;
    m_onIncomingCallExpired = onIncomingCallExpired;
    m_onCallingSomeoneWhoAlreadyCallingYou = onCallingSomeoneWhoAlreadyCallingYou;
    m_onIncomingCall = onIncomingCall;
    m_onRemoteUserEndedCall = onRemoteUserEndedCall;

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
            m_callbacksQueue.push([this]() {m_onNetworkError(); });
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
    m_callbacksQueue.push([this]() {m_onNetworkError(); });
}

CallsClient::~CallsClient() {
    stop();
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
        m_state = State::FREE;
        m_callbacksQueue.push([this]() {m_authorizationResult(true); });
        break;

    case (PacketType::AUTHORIZE_FAIL):
        m_myNickname = "";
        m_callbacksQueue.push([this]() {m_authorizationResult(false); });
        break;

    case (PacketType::LOGOUT_OK):
        onLogoutOk(data, length);
        break;

    case (PacketType::GET_FRIEND_INFO_SUCCESS):
        onFriendInfoSuccess(data, length);
        break;

    case (PacketType::GET_FRIEND_INFO_FAIL):
        m_nicknameWhomCalling.clear();
        m_callbacksQueue.push([this]() {m_startCallingResult(false); });
        break;

    case (PacketType::CREATE_CALL_SUCCESS):
        onCreateCallSuccess(data, length);
        m_callbacksQueue.push([this]() {m_startCallingResult(true); });
        break;

    case (PacketType::CREATE_CALL_FAIL):
        m_nicknameWhomCalling.clear();
        m_callbacksQueue.push([this]() {m_startCallingResult(false); });
        break;

    case (PacketType::END_CALL_OK):
        -
        break;

    case (PacketType::CALLING_END_OK):
        -
        break;

    case (PacketType::CALL_ACCEPTED_OK):
        -
        break;

    case (PacketType::CALL_DECLINED_OK):
        -
        break;

    case (PacketType::ALL_CALLS_DECLINED_OK):
        -
        break;




    case (PacketType::PING):
        m_networkController->send(PacketType::PING_SUCCESS);
        break;

    case (PacketType::PING_SUCCESS):
        m_pingManager->setPingSuccess();
        break;




    case (PacketType::CALL_ACCEPTED):
        if (m_state == State::BUSY) break;
        m_timer.stop();
        m_nicknameWhomCalling.clear();
        m_state = State::BUSY;
        if (m_call && m_audioEngine) {
            m_callbacksQueue.push([this]() {m_onCallingAccepted(); m_audioEngine->startStream(); });
        }
        break;

    case (PacketType::CALL_DECLINED):
        m_timer.stop();
        m_state = State::FREE;
        m_call = std::nullopt;
        m_nicknameWhomCalling.clear();
        m_callbacksQueue.push([this]() {m_onCallingDeclined(); });
        break;

    case (PacketType::CALLING_END):
        onCallingEnd(data, length);
        break;

    case (PacketType::END_CALL):
        if (m_state == State::BUSY && m_audioEngine) {
            m_audioEngine->stopStream();
            m_call = std::nullopt;
            m_callbacksQueue.push([this]() {m_onRemoteUserEndedCall(); });
            m_state = State::FREE;
        }
        break;

    case (PacketType::CREATE_CALL):
        if (bool parsed = onIncomingCall(data, length); parsed && m_state != State::UNAUTHORIZED) {
            m_callbacksQueue.push([this]() {
                auto& [timer, incomingCallData] = m_incomingCalls.back();
                m_onIncomingCall(incomingCallData.friendNickname); 
            });
        }
        break;
    }
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

void CallsClient::stop() {
    m_running = false;

    m_pingManager->stop();

    logout();

    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    m_myNickname.clear();
    m_keysManager->resetKeys();
    m_incomingCalls.clear();
    m_call = std::nullopt;

    if (m_audioEngine && m_audioEngine->isStream()) {
        m_audioEngine->stopStream();
    }

    if (m_networkController && !m_networkController->stopped()) {
        m_networkController->stop();
    }

    if (m_callbacksQueueProcessingThread.joinable()) {
        m_callbacksQueueProcessingThread.join();
    }
    
    std::queue<std::function<void()>> empty;
    std::swap(m_callbacksQueue, empty);
    m_state = State::UNAUTHORIZED;
}

bool CallsClient::logout() {
    if (m_state == State::UNAUTHORIZED || !m_waitingForConfirmationOnPacketUUID.empty()) return false;

    std::vector<std::string> nicknames;
    {
        std::lock_guard<std::mutex> lock(m_incomingCallsMutex);
        nicknames.reserve(m_incomingCalls.size());

        for (auto& [timer, incomingCallData] : m_incomingCalls) {
            timer->stop();
            nicknames.push_back(incomingCallData.friendNickname);
        }

        m_incomingCalls.clear();
    }

    m_networkController->send(
        PacketsFactory::getDeclineAllCallsPacket(m_myNickname, nicknames, true),
        PacketType::ALL_CALLS_DECLINED
    );

    using namespace std::chrono_literals;
    m_timer.start(4s, [this]() {
        std::lock_guard<std::mutex> lock(m_callbacksQueueMutex);
        m_callbacksQueue.push([this]() {m_logoutResult(false); });
    });

    return true;
}

bool CallsClient::authorize(const std::string& nickname) {
    if (m_state != State::UNAUTHORIZED || !m_waitingForConfirmationOnPacketUUID.empty()) return false;

    m_myNickname = nickname;

    if (m_networkController)
        m_networkController->send(
        PacketsFactory::getAuthorizationPacket(nickname, m_keysManager->getPublicKey()),
        PacketType::AUTHORIZE
    );

    using namespace std::chrono_literals;
    m_timer.start(4s, [this]() {
        std::lock_guard<std::mutex> lock(m_callbacksQueueMutex);
        m_myNickname = ""; 
        m_callbacksQueue.push([this]() {m_authorizationResult(false); });
    });

    return true;
}

bool CallsClient::endCall() {
    if (m_state == State::UNAUTHORIZED || !m_call || !m_waitingForConfirmationOnPacketUUID.empty()) return false;
    m_state = State::FREE;
    if (m_audioEngine) m_audioEngine->stopStream();
    m_networkController->send(PacketType::END_CALL);
    m_call = std::nullopt;
    m_audioEngine->refreshAudioDevices();

    return true;
}

bool CallsClient::startCalling(const std::string& friendNickname) {
    if (m_state == State::UNAUTHORIZED || m_call || !m_waitingForConfirmationOnPacketUUID.empty()) return false;

    {
        std::lock_guard<std::mutex> lock(m_incomingCallsMutex);
        for (auto& [timer, incomingCall] : m_incomingCalls) {
            if (crypto::calculateHash(incomingCall.friendNickname) == crypto::calculateHash(friendNickname)) {
                m_callbacksQueue.push([this, friendNickname]() {m_onCallingSomeoneWhoAlreadyCallingYou(friendNickname); });
                return false;
            }
        }
    }

    
    m_networkController->send(
        PacketsFactory::getRequestFriendInfoPacket(m_myNickname, friendNickname),
        PacketType::GET_FRIEND_INFO
    );

    using namespace std::chrono_literals;
    m_timer.start(4s, [this]() {
        std::lock_guard<std::mutex> lock(m_callbacksQueueMutex);
        m_nicknameWhomCalling.clear();
        m_callbacksQueue.push([this]() {m_startCallingResult(false); });
    });

    m_nicknameWhomCalling = friendNickname;

    return true;
}

bool CallsClient::stopCalling() {
    if (m_state != State::CALLING || !m_waitingForConfirmationOnPacketUUID.empty()) return false;
    
    m_timer.stop();
    m_state = State::FREE;

    if (m_call) {
        m_networkController->send(
            PacketsFactory::getCallingEndPacket(m_myNickname, m_call.value().getFriendNicknameHash()),
            PacketType::CALLING_END
        );

        m_call = std::nullopt;
    }

    return true;
}

bool CallsClient::declineIncomingCall(const std::string& friendNickname) {
    if (m_state == State::UNAUTHORIZED || m_incomingCalls.empty() || !m_waitingForConfirmationOnPacketUUID.empty()) return false;

    std::lock_guard<std::mutex> lock(m_incomingCallsMutex);

    auto it = std::find_if(m_incomingCalls.begin(), m_incomingCalls.end(),
        [&friendNickname](const auto& pair) {
            return pair.second.friendNickname == friendNickname;
        });

    if (it != m_incomingCalls.end()) {
        it->first->stop();
        m_incomingCalls.erase(it);
        m_networkController->send(
            PacketsFactory::getDeclineCallPacket(m_myNickname, friendNickname),
            PacketType::CALL_DECLINED
        );
    }

    return true;
}

bool CallsClient::declineAllIncomingCalls() {
    if (m_state == State::UNAUTHORIZED || m_incomingCalls.empty() || !m_waitingForConfirmationOnPacketUUID.empty()) return false;

    std::lock_guard<std::mutex> lock(m_incomingCallsMutex);

    for (auto& [timer, incomingCallData] : m_incomingCalls) {
        timer->stop();
        m_networkController->send(
            PacketsFactory::getDeclineCallPacket(m_myNickname, incomingCallData.friendNickname),
            PacketType::CALL_DECLINED
        );
    }

    m_incomingCalls.clear();

    return true;
}

bool CallsClient::acceptIncomingCall(const std::string& friendNickname) {
    if (m_state == State::UNAUTHORIZED || m_incomingCalls.empty() || !m_waitingForConfirmationOnPacketUUID.empty()) return false;

    std::lock_guard<std::mutex> lock(m_incomingCallsMutex);

    if (m_state == State::CALLING) {
        stopCalling();
    }

    auto it = std::find_if(m_incomingCalls.begin(), m_incomingCalls.end(),
        [&friendNickname](const auto& pair) {
            return pair.second.friendNickname == friendNickname;
        });

    if (it == m_incomingCalls.end()) 
        return false;
    

    m_state = State::BUSY;
    it->first->stop();
    m_call = Call(it->second);
    m_incomingCalls.erase(it);

    for (auto& incomingPair : m_incomingCalls) {
        incomingPair.first->stop();
        m_networkController->send(
            PacketsFactory::getDeclineCallPacket(m_myNickname, incomingPair.second.friendNickname),
            PacketType::CALL_DECLINED
        );
    }
    m_incomingCalls.clear();

    m_networkController->send(
        PacketsFactory::getAcceptCallPacket(m_myNickname, friendNickname),
        PacketType::CALL_ACCEPTED
    );

    m_audioEngine->refreshAudioDevices();
    if (m_audioEngine) m_audioEngine->startStream(); 
    return true;
}

void CallsClient::processQueue() {
    while (m_running) {
        {
            std::lock_guard<std::mutex> lock(m_callbacksQueueMutex);
            if (!m_callbacksQueue.empty()) {
                auto& callback = m_callbacksQueue.front();
                callback();
                m_callbacksQueue.pop();
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void CallsClient::onAllCallsDeclinedOk(const unsigned char* data, int length) {
    nlohmann::json jsonObject = nlohmann::json::parse(data, data + length);
    std::string uuid = jsonObject[UUID].get<std::string>();
    bool forLogout = jsonObject[PURPOSE].get<bool>();
    
    if (m_waitingForConfirmationOnPacketUUID == uuid) {
        m_timer.stop();
        m_waitingForConfirmationOnPacketUUID.clear();
    }

    if (forLogout) {
        m_networkController->send(
            PacketsFactory::getLogoutPacket(m_myNickname),
            PacketType::LOGOUT);
   

        using namespace std::chrono_literals;
        m_timer.start(4s, [this]() {
            std::lock_guard<std::mutex> lock(m_callbacksQueueMutex);
            m_callbacksQueue.push([this]() {m_logoutResult(false); });
        });

        if (m_state == State::CALLING) {
            stopCalling();
        }

        if (m_state == State::BUSY) {
            endCall();
        }

        m_state = State::UNAUTHORIZED;
        m_myNickname.clear();
    }
    else {

    }
}

void CallsClient::onLogoutOk(const unsigned char* data, int length) {
    nlohmann::json jsonObject = nlohmann::json::parse(data, data + length);
    std::string uuid = jsonObject[UUID].get<std::string>();

    if (m_waitingForConfirmationOnPacketUUID == uuid) {
        m_timer.stop();
        m_waitingForConfirmationOnPacketUUID.clear();
    }

    std::lock_guard<std::mutex> lock(m_callbacksQueueMutex);
    m_callbacksQueue.push([this]() {m_logoutResult(true); });
}

void CallsClient::onFriendInfoSuccess(const unsigned char* data, int length) {
    nlohmann::json jsonObject = nlohmann::json::parse(data, data + length);
    m_state = State::CALLING;
    m_call = Call(
        jsonObject[NICKNAME_HASH],
        m_nicknameWhomCalling,
        crypto::deserializePublicKey(jsonObject[PUBLIC_KEY])
    );
    m_call.value().createCallKey();

    m_networkController->send(
        PacketsFactory::getCreateCallPacket(m_myNickname, m_keysManager->getPublicKey(), m_call->getFriendNickname(), m_call->getFriendPublicKey(), m_call->getCallKey()),
        PacketType::CREATE_CALL
    );

    using namespace std::chrono_literals;
    m_timer.start(4s, [this]() {
        m_state = State::FREE;
        m_call = std::nullopt;
        m_nicknameWhomCalling.clear();
        m_callbacksQueue.push([this]() {m_startCallingResult(false); });
    });
}

bool CallsClient::onIncomingCall(const unsigned char* data, int length) {
    try {
        nlohmann::json jsonObject = nlohmann::json::parse(data, data + length);

        auto packetAesKey = crypto::RSADecryptAESKey(m_keysManager->getPrivateKey(), jsonObject[PACKET_KEY]);
        std::string nickname = crypto::AESDecrypt(packetAesKey, jsonObject[NICKNAME]);
        auto callKey = crypto::RSADecryptAESKey(m_keysManager->getPrivateKey(), jsonObject[CALL_KEY]);

        // in case we are already calling this user (unlikely to happen)
        if (m_call && m_state == State::CALLING) {
            if (m_call.value().getFriendNicknameHash() == crypto::calculateHash(nickname)) {
                declineIncomingCall(nickname);
            }
        }

        IncomingCallData data(
            nickname,
            crypto::deserializePublicKey(jsonObject[PUBLIC_KEY]),
            callKey);

        auto timer = std::make_unique<Timer>();
        timer->start(std::chrono::seconds(32), [this, nickname]() {
            std::lock_guard<std::mutex> lock(m_incomingCallsMutex);

            auto it = std::find_if(m_incomingCalls.begin(), m_incomingCalls.end(),
                [&nickname](const auto& pair) {
                    return pair.second.friendNickname == nickname;
                });

            if (it != m_incomingCalls.end()) {
                m_callbacksQueue.push([this, nickname]() {
                    m_onIncomingCallExpired(nickname);
                    });
                m_incomingCalls.erase(it);
            }
            });

        m_incomingCalls.emplace_back(std::move(timer), std::move(data));

        return true;
    }
    catch (const std::exception& e) {
        DEBUG_LOG("IncomingCall parsing error: " << e.what() << std::endl);
        return false;
    }
}

void CallsClient::onIncomingCallingEnd(const unsigned char* data, int length) {
    try {
        nlohmann::json jsonObject = nlohmann::json::parse(data, data + length);
        std::string friendNicknameHash = jsonObject[NICKNAME_HASH];

        auto it = std::find_if(m_incomingCalls.begin(), m_incomingCalls.end(),
            [&friendNicknameHash](const auto& pair) {
                return crypto::calculateHash(pair.second.friendNickname) == friendNicknameHash;
            });

        if (it != m_incomingCalls.end()) {
            it->first->stop();
            std::string nickname = it->second.friendNickname;
            m_incomingCalls.erase(it);

            m_callbacksQueue.push([this, nickname = std::move(nickname)]() {
                m_onIncomingCallExpired(nickname);
            });
        }
    }
    catch (const std::exception& e) {
        DEBUG_LOG("CallingEnd parsing error: " << e.what() << std::endl);
    }
}

void CallsClient::onCreateCallSuccess(const unsigned char* data, int length) {
    if (m_call) {
        m_networkController->send(
            PacketsFactory::getCreateCallPacket(m_myNickname, m_keysManager->getPublicKey(), m_call->getFriendNickname(), m_call->getFriendPublicKey(), m_call->getCallKey()),
            PacketType::CREATE_CALL
        );

        using namespace std::chrono_literals;
        m_timer.start(32s, [this]() {
            m_networkController->send(
                PacketsFactory::getCallingEndPacket(m_myNickname, m_call.value().getFriendNickname()),
                PacketType::CALLING_END
            );

            m_state = State::FREE;
            m_call = std::nullopt;
            m_nicknameWhomCalling.clear();
            m_callbacksQueue.push([this]() {m_onCallingExpired(); });
        });
    }
}

void CallsClient::onInputVoice(const unsigned char* data, int length) {
    if (m_call) {
        size_t cipherDataLength = static_cast<size_t>(length) + CryptoPP::AES::BLOCKSIZE;
        std::vector<CryptoPP::byte> cipherData(cipherDataLength);
        crypto::AESEncrypt(m_call.value().getCallKey(), data, length, cipherData.data(), cipherDataLength);
        m_networkController->send(std::move(cipherData), PacketType::VOICE);

    }
}