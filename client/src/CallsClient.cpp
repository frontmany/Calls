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
    std::function<void(Result)>&& authorizationResult,
    std::function<void(Result)>&& createCallResult,
    std::function<void(Result)>&& createGroupCallResult,
    std::function<void(Result)>&& joinGroupCallResult,
    std::function<void(const std::string&)>&& onJoinRequest,
    std::function<void(const std::string&)>&& onJoinRequestExpired,
    std::function<void(const std::string&)>&& onParticipantLeft,
    std::function<void(const std::string&)>&& onIncomingCall,
    std::function<void(const std::string&)>&& onIncomingCallExpired,
    std::function<void(const std::string&)>&& onCallingSomeoneWhoAlreadyCallingYou,
    std::function<void()>&& onRemoteUserEndedCall,
    std::function<void()>&& onNetworkError)
{
    m_authorizationResult = authorizationResult;
    m_createCallResult = createCallResult;
    m_createGroupCallResult = createGroupCallResult;
    m_joinGroupCallResult = joinGroupCallResult;
    m_onJoinRequest = onJoinRequest;
    m_onJoinRequestExpired = onJoinRequestExpired;
    m_onParticipantLeft = onParticipantLeft;
    m_onNetworkError = onNetworkError;
    m_onIncomingCallExpired = onIncomingCallExpired;
    m_onCallingSomeoneWhoAlreadyCallingYou = onCallingSomeoneWhoAlreadyCallingYou;
    m_onIncomingCall = onIncomingCall;
    m_onRemoteUserEndedCall = onRemoteUserEndedCall;
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
    m_pingerThread = std::thread(&CallsClient::ping, this);
    m_networkController->run();
}

void CallsClient::ping() {
    using namespace std::chrono_literals;

    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    std::chrono::seconds pingGap = 2s;
    std::chrono::seconds checkPingGap = 6s;

    std::chrono::steady_clock::time_point lastPing = begin;
    std::chrono::steady_clock::time_point lastCheck = begin;

    while (m_running) {
        std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();

        auto timeSinceLastCheck = now - lastCheck;
        auto timeSinceLastPing = now - lastPing;

        // Check ping takes priority over ping
        if (timeSinceLastCheck >= checkPingGap) {
            checkPing();
            lastCheck = now;
            // After check, also reset ping timer to avoid immediate ping
            lastPing = now;
        }
        else if (timeSinceLastPing >= pingGap) {
            m_networkController->send(PacketType::PING);
            lastPing = now;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

void CallsClient::checkPing() {
    if (m_pingResult) {
        m_pingResult = false;
        DEBUG_LOG("ping success");
    }
    else {
        logout();
        DEBUG_LOG("ping fail");
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push([this]() {m_onNetworkError(); });
    }
}

CallsClient::~CallsClient() {
    logout();

    m_running = false;

    if (m_pingerThread.joinable()) {
        m_pingerThread.join();
    }

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

    if (m_keysFuture.valid()) {
        m_keysFuture.wait();
    }
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

    case (PacketType::PING):
        m_networkController->send(PacketType::PING_SUCCESS);
        break;

    case (PacketType::PING_SUCCESS):
        m_pingResult = true;
        break;

    case (PacketType::FRIEND_INFO_SUCCESS):
        m_timer.stop();
        if (bool parsed = onFriendInfoSuccess(data, length); parsed) {
            startCalling();
        }
        break;

    case (PacketType::FRIEND_INFO_FAIL):
        m_timer.stop();
        m_nicknameWhomCalling.clear();
        m_queue.push([this]() {m_createCallResult(Result::WRONG_FRIEND_NICKNAME); });
        break;

    case (PacketType::CREATE_GROUP_CALL_SUCCESS):
        m_timer.stop();
        onCreateGroupCallSuccess();
        m_queue.push([this]() {m_createGroupCallResult(Result::SUCCESS); m_audioEngine->startStream(); });
        break;

    case (PacketType::CREATE_GROUP_CALL_FAIL):
        m_timer.stop();
        m_groupCall = std::nullopt;
        m_queue.push([this]() {m_createGroupCallResult(Result::TAKEN_GROUP_CALL_NAME); });
        break;

    case (PacketType::GROUP_CALL_EXISTENCE_CHECK_SUCESS):
        m_timer.stop();
        onGroupCallExistenceCheckSuccess(data, length);
        break;

    case (PacketType::GROUP_CALL_EXISTENCE_CHECK_FAIL):
        m_timer.stop();
        m_queue.push([this]() {m_joinGroupCallResult(Result::UNEXISTING_GROUP_CALL); });
        break;


    case (PacketType::JOIN_ALLOWED):
        m_timer.stop();
        onJoinAllowed(data, length);
        m_queue.push([this]() {m_joinGroupCallResult(Result::JOIN_ALLOWED); m_audioEngine->startStream(); });
        break;

    case (PacketType::JOIN_DECLINED):
        m_timer.stop();
        m_queue.push([this]() {m_joinGroupCallResult(Result::JOIN_DECLINED); });
        break;

    case (PacketType::JOIN_REQUEST):
        onJoinRequest(data, length);
        m_queue.push([this]() {
            auto& [_, incomingCallData] = m_joinRequests.back();
            m_onJoinRequest(incomingCallData.friendNickname);
        });
        break;
        
    case (PacketType::GROUP_CALL_PARTICIPANT_QUIT):
        if (const std::string& nickname = onParticipantLeft(data, length); !nickname.empty()) {
            m_queue.push([this, nickname]() {m_onParticipantLeft(nickname); });
        }
        break;

    case (PacketType::CALL_ACCEPTED):
        if (m_state == State::BUSY) break;
        m_timer.stop();
        m_nicknameWhomCalling.clear();
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
            m_call = std::nullopt;
            m_queue.push([this]() {m_onRemoteUserEndedCall(); });
            m_state = State::FREE;
        }
        break;

    case (PacketType::INCOMING_CALL):
        if (bool parsed = onIncomingCall(data, length); parsed && m_state != State::UNAUTHORIZED) {
            m_queue.push([this]() {
                auto& [_, incomingCallData] = m_incomingCalls.back();
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

int CallsClient::getIncomingCallsCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
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

    if (m_pingerThread.joinable()) {
        m_pingerThread.join();
    }

    logout();

    std::this_thread::sleep_for(std::chrono::milliseconds(300));

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

    if (m_thread.joinable()) {
        m_thread.join();
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    std::queue<std::function<void()>> empty;
    std::swap(m_queue, empty);
    m_state = State::UNAUTHORIZED;
}

void CallsClient::logout() {
    if (m_state == State::UNAUTHORIZED) return;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto& [timer, incomingCall] : m_incomingCalls) {
            timer->stop();
            m_networkController->send(
                PacketsFactory::getDeclineCallPacket(incomingCall.friendNickname),
                PacketType::CALL_DECLINED
            );
        }

        m_incomingCalls.clear();
    }

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
    if (m_state == State::UNAUTHORIZED || m_state == State::IN_GROUP_CALL || !m_call) return false;
    m_state = State::FREE;
    if (m_audioEngine) m_audioEngine->stopStream();
    m_networkController->send(PacketType::END_CALL);
    m_call = std::nullopt;
    m_audioEngine->refreshAudioDevices();

    return true;
}

bool CallsClient::createCall(const std::string& friendNickname) {
    if (m_state == State::UNAUTHORIZED || m_state == State::IN_GROUP_CALL || m_call) return false;

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

bool CallsClient::createGroupCall(const std::string& groupCallName) {
    if (m_state == State::UNAUTHORIZED || m_state == State::CALLING || m_state == State::IN_GROUP_CALL || m_call) return false;

    m_networkController->send(
        PacketsFactory::getCreateGroupCallPacket(m_myNickname, groupCallName),
        PacketType::CREATE_GROUP_CALL
    );

    using namespace std::chrono_literals;
    m_timer.start(4s, [this]() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push([this]() {m_createGroupCallResult(Result::TIMEOUT); });
    });

    m_groupCall = GroupCall(m_myNickname, m_myPublicKey, groupCallName, GroupCall::GroupCallRole::INITIATOR);

    return true;
}

bool CallsClient::endGroupCall(const std::string& groupCallName) {
    if (m_state == State::UNAUTHORIZED || m_state == State::CALLING || m_state != State::IN_GROUP_CALL || m_call) return false;
    if (groupCallName == m_groupCall.value().getGroupCallName() || m_groupCall.value().getGroupCallRole() == GroupCall::GroupCallRole::PARTICIPANT) return false;
    
    m_networkController->send(PacketsFactory::getEndGroupCallPacket(m_myNickname, groupCallName), PacketType::END_GROUP_CALL);
    m_groupCall = std::nullopt;
    m_state = State::FREE;
    m_audioEngine->stopStream();

    return true;
}

bool CallsClient::leaveGroupCall(const std::string& groupCallName) {
    if (m_state == State::UNAUTHORIZED || m_state == State::CALLING || m_state != State::IN_GROUP_CALL || m_call) return false;
    if (groupCallName == m_groupCall.value().getGroupCallName() || m_groupCall.value().getGroupCallRole() == GroupCall::GroupCallRole::INITIATOR) return false;

    m_networkController->send(PacketsFactory::getLeaveGroupCallPacket(m_myNickname, groupCallName), PacketType::LEAVE_GROUP_CALL);
    m_groupCall = std::nullopt;
    m_state = State::FREE;
    m_audioEngine->stopStream();

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

bool CallsClient::declineAllIncomingCalls() {
    if (m_state == State::UNAUTHORIZED || m_incomingCalls.empty()) return false;

    std::lock_guard<std::mutex> lock(m_mutex);

    for (auto& [timer, incomingCallData] : m_incomingCalls) {
        timer->stop();
        m_networkController->send(
            PacketsFactory::getDeclineCallPacket(incomingCallData.friendNickname),
            PacketType::CALL_DECLINED
        );
    }

    m_incomingCalls.clear();

    return true;
}

bool CallsClient::acceptIncomingCall(const std::string& friendNickname) {
    if (m_state == State::UNAUTHORIZED || m_incomingCalls.empty()) return false;

    std::lock_guard<std::mutex> lock(m_mutex);

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
            PacketsFactory::getDeclineCallPacket(incomingPair.second.friendNickname),
            PacketType::CALL_DECLINED
        );
    }
    m_incomingCalls.clear();

    m_networkController->send(
        PacketsFactory::getAcceptCallPacket(friendNickname),
        PacketType::CALL_ACCEPTED
    );

    m_audioEngine->refreshAudioDevices();
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
            m_nicknameWhomCalling,
            crypto::deserializePublicKey(jsonObject[PUBLIC_KEY])
        );
        m_call.value().createCallKey();

        return true;
    }
    catch (const std::exception& e) {
        DEBUG_LOG("FriendInfoSuccess parsing error: " << e.what() << std::endl);
        return false;
    }
}

void CallsClient::onGroupCallExistenceCheckSuccess(const unsigned char* data, int length) {
    nlohmann::json jsonObject = nlohmann::json::parse(data, data + length);
    std::string initiatorPublicKeyStr = jsonObject[PUBLIC_KEY].get<std::string>();
    CryptoPP::RSA::PublicKey initiatorPublicKey = crypto::deserializePublicKey(initiatorPublicKeyStr);
    std::string initiatorNicknameHash = jsonObject[NICKNAME_HASH].get<std::string>();

    m_networkController->send(
        PacketsFactory::getJoinGroupCallPacket(m_myNickname, m_myPublicKey, initiatorNicknameHash, initiatorPublicKey),
        PacketType::JOIN_REQUEST
    );

    using namespace std::chrono_literals;
    m_timer.start(32s, [this]() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push([this]() {m_joinGroupCallResult(Result::TIMEOUT); });
    });
}

void CallsClient::onCreateGroupCallSuccess() {
    m_state = State::IN_GROUP_CALL;
    m_groupCall.value().createGroupCallKey();
}

void CallsClient::onJoinAllowed(const unsigned char* data, int length) {
    nlohmann::json jsonObject = nlohmann::json::parse(data, data + length);

    try {
        std::string encryptedPacketKey = jsonObject[PACKET_KEY];
        CryptoPP::SecByteBlock thisPacketKey = crypto::RSADecryptAESKey(m_myPrivateKey, encryptedPacketKey);

        std::string encryptedGroupCallKey = jsonObject[GROUP_CALL_KEY];
        std::string encryptedGroupCallName = jsonObject[GROUP_CALL_NAME];
        std::string encryptedInitiatorNickname = jsonObject[INITIATOR_NICKNAME];
        std::string initiatorPublicKeyStr = jsonObject[PUBLIC_KEY];
        CryptoPP::RSA::PublicKey initiatorPublicKey = crypto::deserializePublicKey(initiatorPublicKeyStr);
        std::string nicknameHashTo = jsonObject[NICKNAME_HASH_TO];

        // Verify this packet is for us
        std::string myNicknameHash = crypto::calculateHash(m_myNickname);
        if (nicknameHashTo != myNicknameHash) {
            std::cerr << "Join allowed packet not intended for this user" << std::endl;
            return;
        }

        CryptoPP::SecByteBlock groupCallKey = crypto::RSADecryptAESKey(m_myPrivateKey, encryptedGroupCallKey);
        std::string groupCallName = crypto::AESDecrypt(thisPacketKey, encryptedGroupCallName);
        std::string initiatorNickname = crypto::AESDecrypt(thisPacketKey, encryptedInitiatorNickname);


        std::unordered_map<std::string, CryptoPP::RSA::PublicKey> participants;
        auto participantsArray = jsonObject[PARTICIPANTS_ARRAY];
        for (const auto& participant : participantsArray) {
            std::string nickname = participant[NICKNAME];
            std::string publicKeyStr = participant[PUBLIC_KEY];
            CryptoPP::RSA::PublicKey publicKey = crypto::deserializePublicKey(publicKeyStr);
            participants[nickname] = publicKey;
        }

        m_groupCall = GroupCall(participants, groupCallKey, initiatorNickname, initiatorPublicKey, groupCallName, GroupCall::GroupCallRole::PARTICIPANT);
        m_groupCall.value().addParticipant(m_myNickname, m_myPublicKey);
        m_state = State::IN_GROUP_CALL;
    }
    catch (const std::exception& e) {
        std::cerr << "Error parsing join allowed packet: " << e.what() << std::endl;
    }
}

void CallsClient::onJoinRequest(const unsigned char* data, int length) {
    nlohmann::json jsonObject = nlohmann::json::parse(data, data + length);
    auto packetAesKey = crypto::RSADecryptAESKey(m_myPrivateKey, jsonObject[PACKET_KEY]);
    std::string requesterNickname = crypto::AESDecrypt(packetAesKey, jsonObject[NICKNAME]);
    std::string requesterPublicKeyStr = jsonObject[PUBLIC_KEY].get<std::string>();
    CryptoPP::RSA::PublicKey requesterPublicKey = crypto::deserializePublicKey(requesterPublicKeyStr);
    
    auto timer = std::make_unique<Timer>();
    timer->start(std::chrono::seconds(32), [this, requesterNickname]() {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = std::find_if(m_joinRequests.begin(), m_joinRequests.end(),
            [&requesterNickname](const auto& pair) {
                return pair.second.friendNickname == requesterNickname;
            });

        if (it != m_joinRequests.end()) {
            m_queue.push([this, requesterNickname]() {
                m_onJoinRequestExpired(requesterNickname);
            });
            m_joinRequests.erase(it);
        }
    });

    JoinRequestData joinRequestData{ requesterNickname, requesterPublicKey };

    std::lock_guard<std::mutex> lock(m_mutex);
    m_joinRequests.emplace_back(std::move(timer), std::move(joinRequestData));
}

bool CallsClient::allowJoin(const std::string& friendNickname) {
    if (m_state == State::UNAUTHORIZED || m_state != State::IN_GROUP_CALL || m_joinRequests.empty()) return false;

    
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = std::find_if(m_joinRequests.begin(), m_joinRequests.end(),
        [&friendNickname](const auto& pair) {
            return pair.second.friendNickname == friendNickname;
        });

    if (it != m_joinRequests.end()) {
        auto& [timer, data] = *it;
        timer->stop();
        
        m_networkController->send(PacketsFactory::getJoinAllowedPacket(m_groupCall.value().getGroupCallKey(),
            m_groupCall.value().getParticipants(),
            m_myNickname,
            m_myPublicKey,
            m_groupCall.value().getGroupCallName(),
            data.friendNickname,
            data.friendPublicKey),
            PacketType::JOIN_ALLOWED
        );

        m_groupCall.value().addParticipant(data.friendNickname, data.friendPublicKey);


        if (m_groupCall.value().getParticipants().size() >= 2) {
            auto& participants = m_groupCall.value().getParticipants();
            for (auto& [nickname, publicKey] : participants) {
                if (nickname != friendNickname && nickname != m_myNickname) {
                    m_networkController->send(PacketsFactory::getGroupCallNewParticipantPacket(nickname, publicKey, data.friendNickname), PacketType::GROUP_CALL_NEW_PARTICIPANT);
                }
            }
        }

        m_joinRequests.erase(it);
    }

    return true;

}

bool CallsClient::joinGroupCall(const std::string& groupCallName) {
    if (m_state == State::UNAUTHORIZED || m_state == State::CALLING || m_state == State::IN_GROUP_CALL || m_call) return false;

    m_networkController->send(
        PacketsFactory::getCheckGroupCallExistencePacket(groupCallName),
        PacketType::GROUP_CALL_EXISTENCE_CHECK
    );

    using namespace std::chrono_literals;
    m_timer.start(4s, [this]() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push([this]() {m_joinGroupCallResult(Result::TIMEOUT); });
    });

    return true;
}

bool CallsClient::declineJoin(const std::string& friendNickname) {
    if (m_state == State::UNAUTHORIZED || m_state != State::IN_GROUP_CALL || m_joinRequests.empty()) return false;

    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = std::find_if(m_joinRequests.begin(), m_joinRequests.end(),
        [&friendNickname](const auto& pair) {
            return pair.second.friendNickname == friendNickname;
        });
     
    if (it != m_joinRequests.end()) {
        auto& [timer, data] = *it;
        timer->stop();
        m_networkController->send(PacketsFactory::getJoinDeclinedPacket(data.friendNickname, data.friendPublicKey, m_groupCall.value().getGroupCallName()), PacketType::JOIN_DECLINED);
        m_joinRequests.erase(it);
    }

    return true;
}

const std::string& CallsClient::onParticipantLeft(const unsigned char* data, int length) const {
    nlohmann::json jsonObject = nlohmann::json::parse(data, data + length);
    auto packetAesKey = crypto::RSADecryptAESKey(m_myPrivateKey, jsonObject[PACKET_KEY]);
    std::string nickname = crypto::AESDecrypt(packetAesKey, jsonObject[NICKNAME]);
    return nickname;
}

bool CallsClient::onIncomingCall(const unsigned char* data, int length) {
    try {
        nlohmann::json jsonObject = nlohmann::json::parse(data, data + length);

        auto packetAesKey = crypto::RSADecryptAESKey(m_myPrivateKey, jsonObject[PACKET_KEY]);
        std::string nickname = crypto::AESDecrypt(packetAesKey, jsonObject[NICKNAME]);
        auto callKey = crypto::RSADecryptAESKey(m_myPrivateKey, jsonObject[CALL_KEY]);

        // in case we are already calling this user (unlikely to happen)
        if (m_call && m_state == State::CALLING) {
            if (m_call.value().getFriendNicknameHash() == crypto::calculateHash(nickname)) {
                declineIncomingCall(nickname);
            }
        }

        IncomingCallData incomingCallData(
            nickname,
            crypto::deserializePublicKey(jsonObject[PUBLIC_KEY]),
            callKey);

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

        m_incomingCalls.emplace_back(std::move(timer), std::move(incomingCallData));

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