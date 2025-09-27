#include "callsClient.h"
#include "packetsFactory.h"
#include "logger.h"

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
    std::function<void(Result)> authorizationResult,
    std::function<void(Result)> createCallResult,
    std::function<void(const std::string&)> onIncomingCall,
    std::function<void(const std::string&)> onIncomingCallExpired,
    std::function<void(const std::string&)> onCallingSomeoneWhoAlreadyCallingYou,
    std::function<void()> onCallHangUp,
    std::function<void()> onNetworkError)
{
    m_authorizationResult = authorizationResult;
    m_createCallResult = createCallResult;
    m_onNetworkError = onNetworkError;
    m_onIncomingCallExpired = onIncomingCallExpired;
    m_onCallingSomeoneWhoAlreadyCallingYou = onCallingSomeoneWhoAlreadyCallingYou;
    m_onIncomingCall = onIncomingCall;
    m_onCallHangUp = onCallHangUp;
    m_running = true;

    m_keysFuture = std::async(std::launch::async, [this]() {
        crypto::generateRSAKeyPair(m_myPrivateKey, m_myPublicKey);
    });

    m_networkController = std::make_unique<NetworkController>();
    m_audioEngine = std::make_unique<AudioEngine>();

    if (m_audioEngine->init([this](const unsigned char* data, int length) {
        onInputVoice(data, length);
        }) == AudioEngine::InitializationStatus::INITIALIZED
        && m_networkController->init(host, port,
            [this](const unsigned char* data, int length, PacketType type) {
                onReceive(data, length, type);
            },
            [this]() {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_timer.stop();
                m_queue.push([this]() {m_onNetworkError(); });
            })) {
        return true;
    }
    else {
        return false;
    }
}

void CallsClient::run() {
    m_thread = std::thread(&CallsClient::processQueue, this);
    m_networkController->run();
}

CallsClient::~CallsClient() {
    logout();

    if (m_thread.joinable()) {
        m_thread.join();
    }

    if (m_networkController) m_networkController->stop();

    if (m_audioEngine && m_audioEngine->initialized() && m_audioEngine->isStream()) {
        m_audioEngine->stopStream();
    }

    for (auto& pair : m_incomingCalls) {
        pair.first->stop();
    }
    m_incomingCalls.clear();
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
    
    std::lock_guard<std::mutex> lock(m_mutex);

    switch (type) {
    case (PacketType::AUTHORIZATION_SUCCESS):
        m_timer.stop();
        m_state = State::FREE;
        m_queue.push([this]() {m_authorizationResult(Result::SUCCESS); });
        break;

    case (PacketType::AUTHORIZATION_FAIL):
        m_timer.stop();
        m_myNickname = "";
        m_queue.push([this]() {m_authorizationResult(Result::FAIL); });
        break;

    case (PacketType::FRIEND_INFO_SUCCESS):
        m_timer.stop();
        if (bool parsed = onFriendInfoSuccess(data, length); parsed) {
            m_call.value().createCallKey();
            startCalling();
        }
        break;

    case (PacketType::FRIEND_INFO_FAIL):
        m_timer.stop();
        m_nicknameWhomCalling.clear();
        m_queue.push([this]() {m_createCallResult(Result::WRONG_FRIEND_NICKNAME); });
        break;

    case (PacketType::CALL_ACCEPTED):
        if (m_state == State::BUSY) break;

        m_timer.stop();
        m_state = State::BUSY;
        if (m_call && m_audioEngine) {
            m_queue.push([this]() {m_createCallResult(Result::CALL_ACCEPTED); m_audioEngine->startStream(); });
        }
        break;

    case (PacketType::CALL_DECLINED):
        m_timer.stop();
        m_state = State::FREE;
        m_call = std::nullopt;
        m_nicknameWhomCalling.clear();
        m_queue.push([this]() {m_createCallResult(Result::CALL_DECLINED); });
        break;

    case (PacketType::CALLING_END):
        onIncomingCallingEnd(data, length);
        break;

    case (PacketType::END_CALL):
        if (m_state == State::BUSY && m_audioEngine) {
            m_audioEngine->stopStream();
            m_queue.push([this]() {m_onCallHangUp(); });
            m_state = State::FREE;
        }
        break;

    case (PacketType::INCOMING_CALL):
        if (bool parsed = onIncomingCall(data, length); parsed && m_state == State::FREE) {
            m_queue.push([this]() {
                auto& [timer, incomingCallData] = m_incomingCalls.back();
                m_onIncomingCall(incomingCallData.friendNickname); 
            });
        }
        break;
    }
}

State CallsClient::getStatus() const {
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

void CallsClient::stop() {
    logout();

    std::this_thread::sleep_for(std::chrono::seconds(1));

    m_myNickname.clear();
    m_myPublicKey = CryptoPP::RSA::PublicKey();
    m_myPrivateKey = CryptoPP::RSA::PrivateKey();
    m_call = std::nullopt;
    m_incomingCalls.clear();

    if (m_audioEngine && m_audioEngine->isStream()) {
        m_audioEngine->stopStream();
    }

    if (m_networkController && !m_networkController->stopped()) {
        m_networkController->stop();
    }

    m_running = false;
    if (m_thread.joinable()) {
        m_thread.join();
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    std::queue<std::function<void()>> empty;
    std::swap(m_queue, empty);
    m_state = State::UNAUTHORIZED;
}

void CallsClient::logout() {
    if (m_networkController)
        m_networkController->send(
        PacketType::LOGOUT
    );

    if (m_timer.is_active()) {
        m_timer.stop();
    }

    if (m_state == State::CALLING) {
        stopCalling();
    }

    if (m_state == State::BUSY) {
        endCall();
    }

    m_state = State::UNAUTHORIZED;
}

bool CallsClient::authorize(const std::string& nickname) {
    if (m_state != State::UNAUTHORIZED) return false;

    m_myNickname = nickname;

    m_keysFuture.wait();

    if (m_networkController)
        m_networkController->send(
        PacketsFactory::getAuthorizationPacket(m_myPublicKey, nickname),
        PacketType::AUTHORIZE
    );

    using namespace std::chrono_literals;
    
    m_timer.start(4s, [this]() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_myNickname = ""; 
        m_queue.push([this]() {m_authorizationResult(Result::TIMEOUT); });
    });

    return true;
}

bool CallsClient::endCall() {
    if (m_state == State::UNAUTHORIZED || !m_call) return false;
    m_state = State::FREE;
    if (m_audioEngine) m_audioEngine->stopStream();
    m_networkController->send(PacketType::END_CALL);
    m_call = std::nullopt;

    return true;
}

bool CallsClient::createCall(const std::string& friendNickname) {
    if (m_state == State::UNAUTHORIZED || m_call) return false;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto& [timer, incomingCall] : m_incomingCalls) {
            if (crypto::calculateHash(incomingCall.friendNickname) == crypto::calculateHash(friendNickname)) {
                m_queue.push([this, friendNickname]() {m_onCallingSomeoneWhoAlreadyCallingYou(friendNickname); });
                return false;
            }
        }
    }

    requestFriendInfo(friendNickname);
    m_nicknameWhomCalling = friendNickname;

    return true;
}

bool CallsClient::stopCalling() {
    if (m_state != State::CALLING) return false;
    
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
    if (m_state == State::UNAUTHORIZED || m_incomingCalls.empty()) return false;

    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = std::find_if(m_incomingCalls.begin(), m_incomingCalls.end(),
        [&friendNickname](const auto& pair) {
            return pair.second.friendNickname == friendNickname;
        });

    if (it != m_incomingCalls.end()) {
        it->first->stop();
        m_incomingCalls.erase(it);
        m_networkController->send(
            PacketsFactory::getDeclineCallPacket(friendNickname),
            PacketType::CALL_DECLINED
        );
    }

    return true;
}

bool CallsClient::acceptIncomingCall(const std::string& friendNickname) {
    if (m_state == State::UNAUTHORIZED || m_incomingCalls.empty()) return false;

    std::lock_guard<std::mutex> lock(m_mutex);

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
            PacketsFactory::getDeclineCallPacket(incomingPair.second.friendNickname),
            PacketType::CALL_DECLINED
        );
    }
    m_incomingCalls.clear();

    m_networkController->send(
        PacketsFactory::getAcceptCallPacket(friendNickname),
        PacketType::CALL_ACCEPTED
    );

    if (m_audioEngine) m_audioEngine->startStream();
    return true;
}

void CallsClient::processQueue() {
    while (m_running) {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (!m_queue.empty()) {
                auto& callback = m_queue.front();
                callback();
                m_queue.pop();
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void CallsClient::requestFriendInfo(const std::string& friendNickname) {
    m_networkController->send(
        PacketsFactory::getRequestFriendInfoPacket(friendNickname),
        PacketType::GET_FRIEND_INFO
    );

    using namespace std::chrono_literals;
    m_timer.start(4s, [this]() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_nicknameWhomCalling.clear();
        m_queue.push([this]() {m_createCallResult(Result::TIMEOUT); });
    });
}

bool CallsClient::onFriendInfoSuccess(const unsigned char* data, int length) {
    try {
        nlohmann::json jsonObject = nlohmann::json::parse(data, data + length);
        m_state = State::CALLING;

        m_call = Call(
            jsonObject[NICKNAME_HASH],
            crypto::deserializePublicKey(jsonObject[PUBLIC_KEY])
        );

        return true;
    }
    catch (const std::exception& e) {
        DEBUG_LOG("FriendInfoSuccess parsing error: " << e.what() << std::endl);
        return false;
    }
}

bool CallsClient::onIncomingCall(const unsigned char* data, int length) {
    try {
        nlohmann::json jsonObject = nlohmann::json::parse(data, data + length);

        auto packetAesKey = crypto::RSADecryptAESKey(m_myPrivateKey, jsonObject[PACKET_KEY]);
        std::string nickname = crypto::AESDecrypt(packetAesKey, jsonObject[NICKNAME]);
        auto callKey = crypto::RSADecryptAESKey(m_myPrivateKey, jsonObject[CALL_KEY]);

        if (m_call && m_state == State::CALLING) {
            if (m_call.value().getFriendNicknameHash() == crypto::calculateHash(nickname)) {
                declineIncomingCall(nickname);
            }
        }

        auto timer = std::make_unique<Timer>();
        timer->start(std::chrono::seconds(32), [this, nickname]() {
            std::lock_guard<std::mutex> lock(m_mutex);

            auto it = std::find_if(m_incomingCalls.begin(), m_incomingCalls.end(),
                [&nickname](const auto& pair) {
                    return pair.second.friendNickname == nickname;
                });

            if (it != m_incomingCalls.end()) {
                m_queue.push([this, nickname]() {
                    m_onIncomingCallExpired(nickname);
                });
                m_incomingCalls.erase(it);
            }
        });

        IncomingCallData data(
            nickname,
            crypto::deserializePublicKey(jsonObject[PUBLIC_KEY]),
            callKey);

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

            m_queue.push([this, nickname = std::move(nickname)]() {
                m_onIncomingCallExpired(nickname);
                });
        }
    }
    catch (const std::exception& e) {
        DEBUG_LOG("CallingEnd parsing error: " << e.what() << std::endl);
    }
}

void CallsClient::startCalling() {
    if (m_call) {
        m_networkController->send(
            PacketsFactory::getCreateCallPacket(m_myPublicKey, m_myNickname, *m_call),
            PacketType::CREATE_CALL
        );

        m_queue.push([this]() {m_createCallResult(Result::CALLING); });

        using namespace std::chrono_literals;
        m_timer.start(32s, [this]() {
            std::lock_guard<std::mutex> lock(m_mutex);

            m_networkController->send(
                PacketsFactory::getCallingEndPacket(m_myNickname, m_call.value().getFriendNicknameHash()),
                PacketType::CALLING_END
            );

            m_state = State::FREE;
            m_call = std::nullopt;
            m_nicknameWhomCalling.clear();
            m_queue.push([this]() {m_createCallResult(Result::TIMEOUT); });
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