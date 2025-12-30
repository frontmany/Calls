#include "client.h"
#include "packetFactory.h"
#include "jsonTypes.h"

#include "utilities/logger.h"

using namespace utilities;
using namespace std::chrono_literals;

namespace calls 
{
    CallsClient::~CallsClient() {
        reset();
        m_networkController.stop();
    }

    CallsClient& CallsClient::get()
    {
        static CallsClient s_instance;
        return s_instance;
    }

    bool CallsClient::init(const std::string& host,
        const std::string& port,
        std::unique_ptr<EventListener>&& eventListener)
    {
        m_eventListener = std::move(eventListener);

        bool audioInitialized = m_audioEngine.init([this](const unsigned char* data, int length) {onInputVoice(data, length); });

        bool networkInitialized = m_networkController.init(host, port,
            [this](const unsigned char* data, int length, uint32_t type) {
                onReceive(data, length, static_cast<PacketType>(type));
            },
            [this]() {
                log::LOG_ERROR("Connection down - ping failed");
                m_stateManager.setConnectionDown(true);
                m_eventListener->onConnectionDown();
            },
            [this]() {
                log::LOG_INFO("Ping restored");

                auto [uid, packet] = PacketFactory::getReconnectPacket(m_stateManager.getMyToken());
                m_taskManager.createTask(uid, 1500ms, 5,
                    [this, packet]() {m_networkController.send(packet, static_cast<uint32_t>(PacketType::RECONNECT)); },
                    [this]() { 
                        if (m_stateManager.isAuthorized())
                            m_eventListener->onConnectionRestored();
                        else
                            m_eventListener->onConnectionRestoredAuthorizationNeeded();
                    },
                    [this]() {
                        log::LOG_ERROR("Reconnect task failed");
                    }
                );

                m_taskManager.startTask(uid);
            });

        if (audioInitialized && networkInitialized) {
            log::LOG_INFO("Calls client initialized successfully");
            return true;
        }
        else {
            log::LOG_ERROR("Calls client initialization failed - audio: {}, network: {}", audioInitialized, networkInitialized);
            return false;
        }

        m_keysManager.generateKeys();

        if (!m_networkController.isRunning()) 
            m_networkController.run();
    }

    void CallsClient::onReceive(const unsigned char* data, int length, PacketType type) {
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
            nlohmann::json jsonObject = nlohmann::json::parse(data, data + length);
            m_packetProcessor.processPacket(jsonObject, type);
        }
    }

    std::vector<std::string> CallsClient::getCallers() {
        auto incomingCalls = m_stateManager.getIncomingCalls();

        std::vector<std::string> callersNicknames;
        for (auto& [nickname, incomingCallData] : incomingCalls) {
            callersNicknames.push_back(nickname);
        }
        
        return callersNicknames;
    }

    void CallsClient::muteMicrophone(bool isMute) {
        m_audioEngine.muteMicrophone(isMute);
    }

    void CallsClient::muteSpeaker(bool isMute) {
        m_audioEngine.muteSpeaker(isMute);
    }

    bool CallsClient::isScreenSharing() {
        return m_stateManager.isScreenSharing();
    }

    bool CallsClient::isViewingRemoteScreen() {
        return m_stateManager.isViewingRemoteScreen();
    }

    bool CallsClient::isCameraSharing() {
        m_stateManager.isCameraSharing();
    }

    bool CallsClient::isViewingRemoteCamera() {
        m_stateManager.isViewingRemoteCamera();
    }

    bool CallsClient::isMicrophoneMuted() {
        return m_audioEngine.isMicrophoneMuted();
    }

    bool CallsClient::isSpeakerMuted() {
        return m_audioEngine.isSpeakerMuted();
    }

    void CallsClient::refreshAudioDevices() {
        m_audioEngine.refreshAudioDevices();
    }

    int CallsClient::getInputVolume() const {
        return m_audioEngine.getInputVolume();
    }

    int CallsClient::getOutputVolume() const {
        return m_audioEngine.getOutputVolume();
    }

    void CallsClient::setInputVolume(int volume) {
        m_audioEngine.setInputVolume(volume);
    }

    void CallsClient::setOutputVolume(int volume) {
        m_audioEngine.setOutputVolume(volume);
    }

    bool CallsClient::isAuthorized() const {
        return m_stateManager.isAuthorized();
    }

    bool CallsClient::isOutgoingCall() const {
        return m_stateManager.getCallStateManager().isOutgoingCall();
    }

    bool CallsClient::isActiveCall() const {
        return m_stateManager.getCallStateManager().isActiveCall();
    }

    bool CallsClient::isConnectionDown() const {
        return m_stateManager.isConnectionDown();
    }

    int CallsClient::getIncomingCallsCount() const {
        return m_stateManager.getIncomingCallsCount();
    }

    const std::string& CallsClient::getMyNickname() const {
        return m_stateManager.getMyNickname();
    }

    const std::string& CallsClient::getNicknameWhomCalling() const {
        return m_stateManager.getCallStateManager().getOutgoingCall().getNickname();
    }

    const std::string& CallsClient::getNicknameInCallWith() const {
        return m_stateManager.getCallStateManager().getActiveCall().getNickname();
    }

    void CallsClient::reset() {
        m_stateManager.reset();
        m_keysManager.resetKeys();
        m_taskManager.cancelAllTasks();

        if (m_audioEngine.isStream()) 
            m_audioEngine.stopStream();
    }

    bool CallsClient::logout() {
        if (!m_stateManager.isAuthorized()) return false;

        auto [uid, packet] = PacketFactory::getLogoutPacket(m_stateManager.getMyNickname());
        m_taskManager.createTask(uid, 1500ms, 5,
            [this, packet]() {
                m_networkController.send(packet, static_cast<uint32_t>(PacketType::LOGOUT));
            },
            [this]() {
                reset();
                m_eventListener->onLogoutCompleted();
            },
            [this]() {
                reset();
                m_eventListener->onLogoutCompleted();
            }
        );

        m_taskManager.startTask(uid);

        return true;
    }

    bool CallsClient::authorize(const std::string& nickname) {
        if (m_stateManager.isAuthorized()) return false;

        auto [uid, packet] = PacketFactory::getAuthorizationPacket(m_stateManager.getMyNickname());
        m_taskManager.createTask(uid, 1500ms, 5,
            [this, packet]() {
                m_networkController.send(packet, static_cast<uint32_t>(PacketType::LOGOUT));
            },
            [this]() {
                if (m_stateManager.isAuthorized())
                    m_eventListener->onAuthorizationResult();
                else
                    m_eventListener->onAuthorizationResult();
            },
            [this]() {
                if (!m_stateManager.isConnectionDown())
                    m_eventListener->onAuthorizationResult();
            }
        );



        {
            std::lock_guard<std::mutex> lock(m_dataMutex);
            if (m_state != ClientState::UNAUTHORIZED) return false;
            m_myNickname = nickname;

            if (!m_keysManager.isKeys()) {
                if (m_keysManager.isGeneratingKeys()) {
                }
                else {
                    m_keysManager.generateKeys();
                }
            }
        }

        m_keysManager.awaitKeysGeneration();
        sendAuthorizationPacket();

        return true;
    }

    bool CallsClient::endCall() {
        std::lock_guard<std::mutex> lock(m_dataMutex);

        if (m_state != ClientState::BUSY) return false;

        sendEndCallPacket(true);

        m_viewingRemoteScreen = false;
        m_state = ClientState::FREE;
        m_call = std::nullopt;
        m_audioEngine.stopStream();
        m_audioEngine.refreshAudioDevices();

        return true;
    }

    bool CallsClient::startCalling(const std::string& friendNickname) {
        bool acceptExistingCall = false;
        {
            std::lock_guard<std::mutex> lock(m_dataMutex);

            if (m_state == ClientState::UNAUTHORIZED || m_state == ClientState::BUSY) return false;

            // calling the person who are already calling us
            for (auto& [timer, incomingCall] : m_incomingCalls) {
                if (incomingCall.friendNickname == friendNickname) {
                    acceptExistingCall = true;
                }
            }

            if (!acceptExistingCall) {
                m_state = ClientState::CALLING;
                m_nicknameWhomCalling = friendNickname;

                sendRequestFriendInfoPacket(friendNickname);

                return true;
            }
        }

        acceptCall(friendNickname);
        return true;
    }

    bool CallsClient::stopCalling() {
        std::lock_guard<std::mutex> lock(m_dataMutex);

        if (m_state != ClientState::CALLING) return false;

        sendStopCallingPacket(true);

        m_callingTimer.stop();
        m_state = ClientState::FREE;
        m_call = std::nullopt;
        m_nicknameWhomCalling.clear();

        return true;
    }

    bool CallsClient::declineCall(const std::string& friendNickname) {
        std::lock_guard<std::mutex> lock(m_dataMutex);

        if (m_state == ClientState::UNAUTHORIZED || m_incomingCalls.empty()) return false;

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

        if (m_state == ClientState::UNAUTHORIZED || m_incomingCalls.empty()) return false;

        auto it = std::find_if(m_incomingCalls.begin(), m_incomingCalls.end(), [&friendNickname](const auto& pair) {
            return pair.second.friendNickname == friendNickname;
            });

        if (it == m_incomingCalls.end()) return false;

        if (m_state == ClientState::CALLING)
            sendStopCallingPacket(true);

        if (m_state == ClientState::BUSY)
            sendEndCallPacket(true);

        for (auto& [timer, incomingCallData] : m_incomingCalls) {
            timer->stop();
            if (incomingCallData.friendNickname != friendNickname)
                sendDeclineCallPacket(incomingCallData.friendNickname, true);
        }

        sendAcceptCallPacket(friendNickname);

        return true;
    }

    bool CallsClient::startScreenSharing() {
        std::lock_guard<std::mutex> lock(m_dataMutex);

        if (m_state != ClientState::BUSY || m_screenSharing || m_viewingRemoteScreen) return false;
        if (!m_call) {
            LOG_WARN("Attempted to start screen sharing without active call context");
            return false;
        }

        sendStartScreenSharingPacket();

        return true;
    }

    bool CallsClient::stopScreenSharing() {
        std::lock_guard<std::mutex> lock(m_dataMutex);

        if (!m_screenSharing) return false;

        m_screenSharing = false;

        if (m_state != ClientState::BUSY) return false;
        if (!m_call) {
            LOG_WARN("Screen sharing stop requested but call data is missing");
            return false;
        }

        sendStopScreenSharingPacket(true);

        return true;
    }

    bool CallsClient::sendScreen(const std::vector<unsigned char>& data) {
        if (!m_screenSharing) return false;
        if (m_state != ClientState::BUSY) return false;

        CryptoPP::SecByteBlock callKey;
        {
            std::lock_guard<std::mutex> lock(m_dataMutex);
            if (!m_call) return false;

            callKey = m_call.value().getCallKey();
        }

        try {
            size_t cipherDataLength = data.size() + CryptoPP::AES::BLOCKSIZE;
            std::vector<CryptoPP::byte> cipherData(cipherDataLength);

            utilities::AESEncrypt(callKey,
                reinterpret_cast<const CryptoPP::byte*>(data.data()),
                data.size(),
                cipherData.data(),
                cipherDataLength);

            m_networkController.send(std::move(cipherData), PacketType::SCREEN);
            return true;
        }
        catch (const std::exception& e) {
            log::LOG_ERROR("Error during screen sending");
            return false;
        }
    }

    bool CallsClient::startCameraSharing() {
        std::lock_guard<std::mutex> lock(m_dataMutex);

        if (m_state != ClientState::BUSY || m_cameraSharing) return false;

        if (!m_call) {
            LOG_WARN("Attempted to start camera sharing without active call context");
            return false;
        }

        sendStartCameraSharingPacket();

        return true;
    }

    bool CallsClient::stopCameraSharing() {
        std::lock_guard<std::mutex> lock(m_dataMutex);

        if (!m_cameraSharing) return false;

        m_cameraSharing = false;

        if (m_state != ClientState::BUSY) return false;
        if (!m_call) {
            LOG_WARN("Camera sharing stop requested but call data is missing");
            return false;
        }

        sendStopCameraSharingPacket(true);

        return true;
    }

    bool CallsClient::sendCamera(const std::vector<unsigned char>& data) {
        if (!m_cameraSharing) return false;
        if (m_state != ClientState::BUSY) return false;

        CryptoPP::SecByteBlock callKey;
        {
            std::lock_guard<std::mutex> lock(m_dataMutex);
            if (!m_call) return false;

            callKey = m_call.value().getCallKey();
        }

        try {
            size_t cipherDataLength = data.size() + CryptoPP::AES::BLOCKSIZE;
            std::vector<CryptoPP::byte> cipherData(cipherDataLength);

            utilities::AESEncrypt(callKey,
                reinterpret_cast<const CryptoPP::byte*>(data.data()),
                data.size(),
                cipherData.data(),
                cipherDataLength);

            m_networkController.send(std::move(cipherData), PacketType::CAMERA);
            return true;
        }
        catch (const std::exception& e) {
            log::LOG_ERROR("Error during screen sending");
            return false;
        }
    }



    //----------------------------------------------------------------------------
    //----------------------------------------------------------------------------
    //----------------------------------------------------------------------------

    void CallsClient::onVoice(const unsigned char* data, int length) {
        if (m_call && m_audioEngine && m_audioEngine.isStream()) {
            size_t decryptedLength = static_cast<size_t>(length) - CryptoPP::AES::BLOCKSIZE;
            std::vector<CryptoPP::byte> decryptedData(decryptedLength);

            utilities::AESDecrypt(m_call.value().getCallKey(), data, length,
                decryptedData.data(), decryptedData.size()
            );

            m_audioEngine.playAudio(decryptedData.data(), static_cast<int>(decryptedLength));
        }
    }

    void CallsClient::onAuthorizationSuccess(const nlohmann::json& jsonObject) {
        auto packetValid = validatePacket(jsonObject);
        if (!packetValid) return;

        std::string uuid = jsonObject[UUID].get<std::string>();
        onPacketConfirmed(uuid);

        log::LOG_INFO("User authorized successfully");
        m_state = ClientState::FREE;
        m_callbacksQueue.push([this]() {m_eventListener->onAuthorizationResult(ErrorCode::OK); });
    }

    void CallsClient::onAuthorizationFail(const nlohmann::json& jsonObject) {
        auto packetValid = validatePacket(jsonObject);
        if (!packetValid) return;

        std::string uuid = jsonObject[UUID].get<std::string>();
        onPacketConfirmed(uuid);

        LOG_WARN("Authorization failed - nickname already taken");
        m_myNickname.clear();
        m_callbacksQueue.push([this]() {m_eventListener->onAuthorizationResult(ErrorCode::TAKEN_NICKNAME); });
    }

    void CallsClient::onLogoutOk(const nlohmann::json& jsonObject) {
        auto packetValid = validatePacket(jsonObject);
        if (!packetValid) return;

        std::string uuid = jsonObject[UUID].get<std::string>();
        onPacketConfirmed(uuid);
    }

    void CallsClient::onCallAcceptedOk(const nlohmann::json& jsonObject) {
        auto packetValid = validatePacket(jsonObject);
        if (!packetValid) return;

        std::string uuid = jsonObject[UUID].get<std::string>();
        onPacketConfirmed(uuid);

        std::string nicknameHash = jsonObject[NICKNAME_HASH_SENDER];

        auto it = std::find_if(m_incomingCalls.begin(), m_incomingCalls.end(), [&nicknameHash](const auto& pair) {
            return utilities::calculateHash(pair.second.friendNickname) == nicknameHash;
            });

        if (it == m_incomingCalls.end()) return;

        m_state = ClientState::BUSY;
        m_call = Call(it->second);
        m_viewingRemoteScreen = false;

        m_incomingCalls.clear();
        const std::string friendNickname = m_call.value().getFriendNickname();

        m_callbacksQueue.push([this, friendNickname]() {
            if (m_audioEngine) {
                m_audioEngine.refreshAudioDevices();
                m_audioEngine.startStream();
            }
            m_eventListener->onAcceptCallResult(ErrorCode::OK, friendNickname);
            });
    }

    void CallsClient::onCallAcceptedFail(const nlohmann::json& jsonObject) {
        auto packetValid = validatePacket(jsonObject);
        if (!packetValid) return;

        std::string nicknameHash = jsonObject[NICKNAME_HASH_RECEIVER];

        auto it = std::find_if(m_incomingCalls.begin(), m_incomingCalls.end(), [&nicknameHash](const auto& pair) {
            return utilities::calculateHash(pair.second.friendNickname) == nicknameHash;
            });

        if (it == m_incomingCalls.end()) return;

        m_callbacksQueue.push([this, nickname = it->second.friendNickname]() {m_eventListener->onAcceptCallResult(ErrorCode::UNEXISTING_USER, nickname); });

        m_call = std::nullopt;
        m_incomingCalls.erase(it);
    }

    void CallsClient::onCallDeclinedOk(const nlohmann::json& jsonObject) {
        auto packetValid = validatePacket(jsonObject);
        if (!packetValid) return;

        std::string uuid = jsonObject[UUID].get<std::string>();
        onPacketConfirmed(uuid);
    }

    void CallsClient::onScreenSharingStartedFail(const nlohmann::json& jsonObject) {
        auto packetValid = validatePacket(jsonObject);
        if (!packetValid) return;

        m_screenSharing = false;
        LOG_WARN("Screen sharing start rejected by server");

        m_callbacksQueue.push([this]() {m_eventListener->onStartScreenSharingError(); });
    }

    void CallsClient::onScreenSharingStartedOk(const nlohmann::json& jsonObject) {
        auto packetValid = validatePacket(jsonObject);
        if (!packetValid) return;

        std::string uuid = jsonObject[UUID].get<std::string>();
        onPacketConfirmed(uuid);

        m_screenSharing = true;
        log::LOG_INFO("Screen sharing started successfully");

        m_callbacksQueue.push([this]() {m_eventListener->onScreenSharingStarted(); });
    }

    void CallsClient::onScreenSharingStoppedOk(const nlohmann::json& jsonObject) {
        auto packetValid = validatePacket(jsonObject);
        if (!packetValid) return;

        std::string uuid = jsonObject[UUID].get<std::string>();
        onPacketConfirmed(uuid);
    }

    void CallsClient::onScreen(const unsigned char* data, int length) {
        if (!m_viewingRemoteScreen) {
            return;
        }

        if (!data || length <= 0) {
            return;
        }

        if (!m_call) {
            LOG_WARN("Screen frame received but call state is empty");
            return;
        }

        if (length <= CryptoPP::AES::BLOCKSIZE) {
            LOG_WARN("Screen frame too small to decrypt: {} bytes", length);
            return;
        }

        std::vector<CryptoPP::byte> decrypted(static_cast<std::size_t>(length) - CryptoPP::AES::BLOCKSIZE);

        try {
            utilities::AESDecrypt(m_call.value().getCallKey(),
                data,
                length,
                decrypted.data(),
                decrypted.size());
        }
        catch (const std::exception& e) {
            log::LOG_ERROR("Failed to decrypt screen frame: {}", e.what());
            return;
        }

        std::vector<unsigned char> screenData(decrypted.begin(), decrypted.end());

        log::LOG_INFO("Screen frame : {} bytes", screenData.size());

        m_callbacksQueue.push([this, screenData = std::move(screenData)]() mutable {
            if (m_eventListener) {
                m_eventListener->onIncomingScreen(screenData);
            }
            });
    }

    void CallsClient::onIncomingScreenSharingStarted(const nlohmann::json& jsonObject) {
        if (m_state != ClientState::BUSY) return;

        sendConfirmationPacket(jsonObject, PacketType::START_SCREEN_SHARING_OK);
        m_viewingRemoteScreen = true;

        m_callbacksQueue.push([this]() {m_eventListener->onIncomingScreenSharingStarted(); });
    }

    void CallsClient::onIncomingScreenSharingStopped(const nlohmann::json& jsonObject) {
        if (m_state != ClientState::BUSY) return;

        bool needConfirmation = jsonObject[NEED_CONFIRMATION].get<bool>();
        if (needConfirmation)
            sendConfirmationPacket(jsonObject, PacketType::STOP_SCREEN_SHARING_OK);

        m_viewingRemoteScreen = false;

        m_callbacksQueue.push([this]() {m_eventListener->onIncomingScreenSharingStopped(); });
    }

    void CallsClient::onCameraSharingStartedFail(const nlohmann::json& jsonObject) {
        auto packetValid = validatePacket(jsonObject);
        if (!packetValid) return;

        m_cameraSharing = false;
        LOG_WARN("Camera sharing start rejected by server");

        m_callbacksQueue.push([this]() {m_eventListener->onStartCameraSharingError(); });
    }

    void CallsClient::onCameraSharingStartedOk(const nlohmann::json& jsonObject) {
        auto packetValid = validatePacket(jsonObject);
        if (!packetValid) return;

        std::string uuid = jsonObject[UUID].get<std::string>();
        onPacketConfirmed(uuid);

        m_cameraSharing = true;
        log::LOG_INFO("Camera sharing started successfully");

        m_callbacksQueue.push([this]() {m_eventListener->onCameraSharingStarted(); });
    }

    void CallsClient::onCameraSharingStoppedOk(const nlohmann::json& jsonObject) {
        auto packetValid = validatePacket(jsonObject);
        if (!packetValid) return;

        std::string uuid = jsonObject[UUID].get<std::string>();
        onPacketConfirmed(uuid);
    }

    void CallsClient::onCamera(const unsigned char* data, int length) {
        if (!m_viewingRemoteCamera) return;

        if (!data || length <= 0) return;

        if (!m_call) {
            LOG_WARN("Camera frame received but call state is empty");
            return;
        }

        if (length <= CryptoPP::AES::BLOCKSIZE) {
            LOG_WARN("Camera frame too small to decrypt: {} bytes", length);
            return;
        }

        std::vector<CryptoPP::byte> decrypted(static_cast<std::size_t>(length) - CryptoPP::AES::BLOCKSIZE);

        try {
            utilities::AESDecrypt(m_call.value().getCallKey(),
                data,
                length,
                decrypted.data(),
                decrypted.size());
        }
        catch (const std::exception& e) {
            log::LOG_ERROR("Failed to decrypt camera frame: {}", e.what());
            return;
        }

        std::vector<unsigned char> cameraData(decrypted.begin(), decrypted.end());

        log::LOG_INFO("Camera frame : {} bytes", cameraData.size());

        m_callbacksQueue.push([this, cameraData = std::move(cameraData)]() mutable {
            if (m_eventListener) {
                m_eventListener->onIncomingCamera(cameraData);
            }
            });
    }

    void CallsClient::onIncomingCameraSharingStarted(const nlohmann::json& jsonObject) {
        if (m_state != ClientState::BUSY) return;

        sendConfirmationPacket(jsonObject, PacketType::START_CAMERA_SHARING_OK);
        m_viewingRemoteCamera = true;

        m_callbacksQueue.push([this]() {m_eventListener->onIncomingCameraSharingStarted(); });
    }

    void CallsClient::onIncomingCameraSharingStopped(const nlohmann::json& jsonObject) {
        if (m_state != ClientState::BUSY) return;

        bool needConfirmation = jsonObject[NEED_CONFIRMATION].get<bool>();
        if (needConfirmation)
            sendConfirmationPacket(jsonObject, PacketType::STOP_CAMERA_SHARING_OK);

        m_viewingRemoteCamera = false;

        m_callbacksQueue.push([this]() {m_eventListener->onIncomingCameraSharingStopped(); });
    }

    void CallsClient::onStartCallingFail(const nlohmann::json& jsonObject) {
        auto packetValid = validatePacket(jsonObject);
        if (!packetValid) return;

        m_state = ClientState::FREE;
        m_call = std::nullopt;
        m_nicknameWhomCalling.clear();

        m_callbacksQueue.push([this]() {m_eventListener->onStartCallingResult(ErrorCode::UNEXISTING_USER); });
    }

    void CallsClient::onEndCallOk(const nlohmann::json& jsonObject) {
        auto packetValid = validatePacket(jsonObject);
        if (!packetValid) return;

        std::string uuid = jsonObject[UUID].get<std::string>();
        onPacketConfirmed(uuid);
    }

    void CallsClient::onFriendInfoSuccess(const nlohmann::json& jsonObject) {
        auto packetValid = validatePacket(jsonObject);
        if (!packetValid) return;

        std::string uuid = jsonObject[UUID].get<std::string>();
        onPacketConfirmed(uuid);

        m_call = Call(jsonObject[NICKNAME_HASH], m_nicknameWhomCalling, utilities::deserializePublicKey(jsonObject[PUBLIC_KEY]));
        m_call.value().createCallKey();

        sendStartCallingPacket(m_call->getFriendNickname(), m_call->getFriendPublicKey(), m_call->getCallKey());
    }

    void CallsClient::onFriendInfoFail(const nlohmann::json& jsonObject) {
        auto packetValid = validatePacket(jsonObject);
        if (!packetValid) return;

        std::string uuid = jsonObject[UUID].get<std::string>();
        onPacketConfirmed(uuid);

        m_state = ClientState::FREE;
        m_nicknameWhomCalling.clear();

        m_callbacksQueue.push([this]() {m_eventListener->onStartCallingResult(ErrorCode::UNEXISTING_USER); });
    }

    void CallsClient::onStartCallingOk(const nlohmann::json& jsonObject) {
        auto packetValid = validatePacket(jsonObject);
        if (!packetValid) return;

        std::string uuid = jsonObject[UUID].get<std::string>();
        onPacketConfirmed(uuid);

        m_callingTimer.start(32s, [this]() {
            std::lock_guard<std::mutex> lock(m_dataMutex);
            m_callbacksQueue.push([this]() {m_eventListener->onMaximumCallingTimeReached(); });
            });

        m_callbacksQueue.push([this]() {m_eventListener->onStartCallingResult(ErrorCode::OK); });
    }

    void CallsClient::onStopCallingOk(const nlohmann::json& jsonObject) {
        auto packetValid = validatePacket(jsonObject);
        if (!packetValid) return;

        std::string uuid = jsonObject[UUID].get<std::string>();
        onPacketConfirmed(uuid);
    }

    void CallsClient::onInputVoice(const unsigned char* data, int length) {
        const CryptoPP::SecByteBlock* callKey;
        {
            std::lock_guard<std::mutex> lock(m_dataMutex);
            if (!m_call) return;

            size_t cipherDataLength = static_cast<size_t>(length) + CryptoPP::AES::BLOCKSIZE;
            std::vector<CryptoPP::byte> cipherData(cipherDataLength);
            utilities::AESEncrypt(m_call.value().getCallKey(), data, length, cipherData.data(), cipherDataLength);
            m_networkController.send(std::move(cipherData), PacketType::VOICE);
        }
    }











    void CallsClient::sendAuthorizationPacket() {
        auto [uuid, packet] = PacketsFactory::getAuthorizationPacket(m_myNickname, m_keysManager.getPublicKey());

        std::shared_ptr<Task> task = std::make_shared<Task>(m_networkController, uuid, std::move(packet), PacketType::AUTHORIZE,
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

                m_callbacksQueue.push([this]() {m_eventListener->onAuthorizationResult(ErrorCode::TIMEOUT); });
            },
            1
        );

        m_tasks.emplace(uuid, task);
    }

    void CallsClient::sendLogoutPacket(bool createTask) {
        auto [uuid, packet] = PacketsFactory::getLogoutPacket(m_myNickname, createTask);

        if (createTask) {
            std::shared_ptr<Task> task = std::make_shared<Task>(m_networkController, uuid, std::move(packet), PacketType::LOGOUT,
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
            return;
        }

        m_networkController.send(std::move(packet), PacketType::LOGOUT);
    }

    void CallsClient::sendRequestFriendInfoPacket(const std::string& friendNickname) {
        auto [uuid, packet] = PacketsFactory::getRequestFriendInfoPacket(m_myNickname, friendNickname);

        std::shared_ptr<Task> task = std::make_shared<Task>(m_networkController, uuid, std::move(packet), PacketType::GET_FRIEND_INFO,
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
                    m_state = ClientState::FREE;
                    m_nicknameWhomCalling.clear();
                    m_eventListener->onStartCallingResult(ErrorCode::TIMEOUT);
                    });
            },
            3
        );

        m_tasks.emplace(uuid, task);
    }

    void CallsClient::sendAcceptCallPacket(const std::string& friendNickname) {
        auto [uuid, packet] = PacketsFactory::getAcceptCallPacket(m_myNickname, friendNickname);

        std::shared_ptr<Task> task = std::make_shared<Task>(m_networkController, uuid, std::move(packet), PacketType::CALL_ACCEPTED,
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
                    m_eventListener->onAcceptCallResult(ErrorCode::TIMEOUT, nickname);
                    });
            },
            3
        );

        m_tasks.emplace(uuid, task);
    }

    void CallsClient::sendDeclineCallPacket(const std::string& friendNickname, bool createTask) {
        auto [uuid, packet] = PacketsFactory::getDeclineCallPacket(m_myNickname, friendNickname, createTask);

        if (createTask) {
            std::shared_ptr<Task> task = std::make_shared<Task>(m_networkController, uuid, std::move(packet), PacketType::CALL_DECLINED,
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
            return;
        }

        m_networkController.send(std::move(packet), PacketType::CALL_DECLINED);
    }

    void CallsClient::sendStartCallingPacket(const std::string& friendNickname, const CryptoPP::RSA::PublicKey& friendPublicKey, const CryptoPP::SecByteBlock& callKey) {
        auto [uuid, packet] = PacketsFactory::getStartCallingPacket(m_myNickname, m_keysManager.getPublicKey(), friendNickname, friendPublicKey, callKey);

        std::shared_ptr<Task> task = std::make_shared<Task>(m_networkController, uuid, std::move(packet), PacketType::START_CALLING,
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

                m_state = ClientState::FREE;
                m_call = std::nullopt;
                m_nicknameWhomCalling.clear();

                m_callbacksQueue.push([this]() {m_eventListener->onStartCallingResult(ErrorCode::TIMEOUT); });
            },
            3
        );

        m_tasks.emplace(uuid, task);
    }

    void CallsClient::sendStartScreenSharingPacket() {
        if (!m_call) {
            LOG_WARN("Cannot send start screen sharing packet without active call");
            return;
        }

        auto [uuid, packet] = PacketsFactory::getStartScreenSharingPacket(m_myNickname, m_call->getFriendNicknameHash());

        std::shared_ptr<Task> task = std::make_shared<Task>(m_networkController, uuid, std::move(packet), PacketType::START_SCREEN_SHARING,
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

                m_callbacksQueue.push([this]() {
                    m_eventListener->onStartScreenSharingError();
                    });
            },
            3
        );

        m_tasks.emplace(uuid, task);
    }

    void CallsClient::sendStopScreenSharingPacket(bool createTask) {
        if (!m_call) {
            LOG_WARN("Cannot send stop screen sharing packet without active call");
            return;
        }

        auto [uuid, packet] = PacketsFactory::getStopScreenSharingPacket(m_myNickname, m_call->getFriendNicknameHash(), createTask);

        if (createTask) {
            std::shared_ptr<Task> task = std::make_shared<Task>(m_networkController, uuid, std::move(packet), PacketType::STOP_SCREEN_SHARING,
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
            return;
        }

        m_networkController.send(std::move(packet), PacketType::STOP_SCREEN_SHARING);
    }

    void CallsClient::sendStartCameraSharingPacket() {
        if (!m_call) {
            LOG_WARN("Cannot send start screen sharing packet without active call");
            return;
        }

        auto [uuid, packet] = PacketsFactory::getStartCameraSharingPacket(m_myNickname, m_call->getFriendNicknameHash());

        std::shared_ptr<Task> task = std::make_shared<Task>(m_networkController, uuid, std::move(packet), PacketType::START_CAMERA_SHARING,
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

                m_callbacksQueue.push([this]() {
                    m_eventListener->onStartCameraSharingError();
                    });
            },
            3
        );

        m_tasks.emplace(uuid, task);
    }

    void CallsClient::sendStopCameraSharingPacket(bool createTask) {
        if (!m_call) {
            LOG_WARN("Cannot send stop screen sharing packet without active call");
            return;
        }

        auto [uuid, packet] = PacketsFactory::getStopCameraSharingPacket(m_myNickname, m_call->getFriendNicknameHash(), createTask);

        if (createTask) {
            std::shared_ptr<Task> task = std::make_shared<Task>(m_networkController, uuid, std::move(packet), PacketType::STOP_CAMERA_SHARING,
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
            return;
        }

        m_networkController.send(std::move(packet), PacketType::STOP_CAMERA_SHARING);
    }

    void CallsClient::sendStopCallingPacket(bool createTask) {
        auto [uuid, packet] = PacketsFactory::getStopCallingPacket(m_myNickname, m_nicknameWhomCalling, createTask);

        if (createTask) {
            std::shared_ptr<Task> task = std::make_shared<Task>(m_networkController, uuid, std::move(packet), PacketType::STOP_CALLING,
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
            return;
        }

        m_networkController.send(std::move(packet), PacketType::STOP_CALLING);
    }

    void CallsClient::sendEndCallPacket(bool createTask) {
        auto [uuid, packet] = PacketsFactory::getEndCallPacket(m_myNickname, createTask);

        if (createTask) {
            std::shared_ptr<Task> task = std::make_shared<Task>(m_networkController, uuid, std::move(packet), PacketType::END_CALL,
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
            return;
        }

        m_networkController.send(std::move(packet), PacketType::END_CALL);
    }

    void CallsClient::sendConfirmation(const std::string& uid, const std::string& nicknameHashTo) {
        auto packet = PacketsFactory::getConfirmationPacket(m_myNickname, nicknameHashTo, uid);
        m_networkController.send(std::move(packet), static_cast<uint32_t>(PacketType::CALL_ACCEPTED_OK));
    }

    void CallsClient::sendConfirmationPacket(const nlohmann::json& jsonObject, PacketType confirmationType) {
        if (!jsonObject.contains(UUID)) {
            LOG_WARN("Cannot send confirmation: packet missing UUID");
            return;
        }

        std::string uuid = jsonObject[UUID].get<std::string>();
        std::string nicknameHashTo = jsonObject[NICKNAME_HASH_SENDER].get<std::string>();

        sendConfirmation(uuid, nicknameHashTo);
    }

    bool CallsClient::validatePacket(const nlohmann::json& jsonObject) {
        if (!jsonObject.contains(UUID)) {
            LOG_WARN("Packet validation failed: missing UUID");
            return false;
        }
        return true;
    }

    void CallsClient::sendPacketWithConfirmation(const std::string& uuid, std::vector<unsigned char> packet,
        PacketType packetType, int maxRetries)
    {
        std::lock_guard<std::mutex> lock(m_dataMutex);

        PendingPacket pendingPacket;
        pendingPacket.data = packet;
        pendingPacket.type = packetType;
        pendingPacket.uuid = uuid;

        m_pendingPackets.emplace(uuid, pendingPacket);

        m_tasksManager.createTask(uuid, maxRetries,
            [this, uuid, packetType]() {
                std::lock_guard<std::mutex> lock(m_dataMutex);

                auto it = m_pendingPackets.find(uuid);
                if (it != m_pendingPackets.end()) {
                    m_networkController.send(it->second.data, static_cast<uint32_t>(it->second.type));
                    log::LOG_INFO("Retrying packet with UUID: {}", uuid);
                }
            },
            [this, uuid]() {
                std::lock_guard<std::mutex> lock(m_dataMutex);

                m_pendingPackets.erase(uuid);
                LOG_WARN("Packet delivery failed after max retries: {}", uuid);
            }
        );

        m_networkController.send(std::move(packet), static_cast<uint32_t>(packetType));
    }

    void CallsClient::onPacketConfirmed(const std::string& uuid) {
        std::lock_guard<std::mutex> lock(m_dataMutex);

        if (m_tasksManager.hasTask(uuid)) {
            m_tasksManager.finishTask(uuid);
            m_pendingPackets.erase(uuid);
            log::LOG_INFO("Packet confirmed and task finished: {}", uuid);
        }
        else if (m_pendingPackets.find(uuid) != m_pendingPackets.end()) {
            m_pendingPackets.erase(uuid);
            LOG_WARN("Received confirmation for packet without active task: {}", uuid);
        }
    }
}