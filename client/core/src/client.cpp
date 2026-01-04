#include "client.h"
#include "jsonType.h"
#include "packetFactory.h"
#include "utilities/crypto.h"
#include "errorCode.h"
#include "json.hpp"

#include "utilities/logger.h"

using namespace core::utilities;
using namespace std::chrono_literals;

namespace core
{
    Client::Client()
        : m_packetProcessor(m_stateManager, m_keyManager, m_taskManager, m_networkController, m_audioEngine, m_eventListener)
    {
    }

    Client::~Client() {
        reset();
        m_networkController.stop();
    }

    bool Client::init(const std::string& host,
        const std::string& port,
        std::shared_ptr<EventListener> eventListener)
    {
        m_eventListener = std::move(eventListener);

        bool audioInitialized = m_audioEngine.init([this](const unsigned char* data, int length) {onInputVoice(data, length); });

        bool networkInitialized = m_networkController.init(host, port,
            [this](const unsigned char* data, int length, uint32_t type) {
                onReceive(data, length, static_cast<PacketType>(type));
            },
            [this]() {
                LOG_ERROR("Connection down - ping failed");
                m_stateManager.setConnectionDown(true);

                if (m_stateManager.isOutgoingCall()) {
                    m_stateManager.clearCallState();
                    m_eventListener->onOutgoingCallTimeout(ErrorCode::network_error);
                }

                if (m_stateManager.isIncomingCalls()) {
                    auto& incomingCalls = m_stateManager.getIncomingCalls();

                    for (auto& [nickname, incomingCall] : incomingCalls) {
                        m_eventListener->onIncomingCallExpired(ErrorCode::network_error, nickname);
                    }

                    m_stateManager.clearIncomingCalls();
                }

                m_eventListener->onConnectionDown();
            },
            [this]() {
                LOG_INFO("Ping restored");

                auto [uid, packet] = PacketFactory::getReconnectPacket(m_stateManager.getMyNickname(), m_stateManager.getMyToken());

                createAndStartTask(uid, packet, PacketType::RECONNECT,
                    [this](std::optional<nlohmann::json> completionContext) {
                        auto& context = completionContext.value();

                        bool reconnected = context[RESULT].get<bool>();
                        bool activeCall = context[IS_ACTIVE_CALL].get<bool>();
                        
                        if (reconnected) {
                            m_stateManager.setConnectionDown(false);

                            m_eventListener->onConnectionRestored();

                            if (!activeCall) {
                                m_stateManager.setScreenSharing(false);
                                m_stateManager.setCameraSharing(false);
                                m_stateManager.setViewingRemoteScreen(false);
                                m_stateManager.setViewingRemoteCamera(false);
                                m_stateManager.clearCallState();
                                m_audioEngine.stopStream();

                                m_eventListener->onCallEndedByRemote({});
                            }
                        }
                        else {
                            reset();
                            m_eventListener->onConnectionRestoredAuthorizationNeeded();
                        }
                    },
                    [this](std::optional<nlohmann::json> failureContext) {
                        LOG_ERROR("Reconnect task failed");
                    }
                );
            });

        m_keyManager.generateKeys();

        if (!m_networkController.isRunning())
            m_networkController.run();

        if (audioInitialized && networkInitialized) {
            LOG_INFO("Calls client initialized successfully");
            return true;
        }
        else {
            LOG_ERROR("Calls client initialization failed - audio: {}, network: {}", audioInitialized, networkInitialized);
            return false;
        }
    }

    void Client::reset() {
        m_stateManager.reset();
        m_keyManager.resetKeys();
        m_taskManager.cancelAllTasks();

        if (m_audioEngine.isStream())
            m_audioEngine.stopStream();
    }

    void Client::createAndStartTask(
        const std::string& uid,
        const std::vector<unsigned char>& packet,
        PacketType packetType,
        std::function<void(std::optional<nlohmann::json>)> onCompletion,
        std::function<void(std::optional<nlohmann::json>)> onFailure)
    {
        m_taskManager.createTask(uid, 1500ms, 3,
            [this, packet, packetType]() {
                m_networkController.send(packet, static_cast<uint32_t>(packetType));
            },
            std::move(onCompletion),
            std::move(onFailure)
        );

        m_taskManager.startTask(uid);
    }

    void Client::onReceive(const unsigned char* data, int length, PacketType type) {
        m_packetProcessor.processPacket(data, length, type);
    }

    std::vector<std::string> Client::getCallers() const {
        auto& incomingCalls = m_stateManager.getIncomingCalls();

        std::vector<std::string> callersNicknames;
        for (auto& [nickname, incomingCall] : incomingCalls) {
            callersNicknames.push_back(nickname);
        }

        return callersNicknames;
    }

    void Client::muteMicrophone(bool isMute) {
        m_audioEngine.muteMicrophone(isMute);
    }

    void Client::muteSpeaker(bool isMute) {
        m_audioEngine.muteSpeaker(isMute);
    }

    bool Client::isScreenSharing() const {
        return m_stateManager.isScreenSharing();
    }

    bool Client::isViewingRemoteScreen() const {
        return m_stateManager.isViewingRemoteScreen();
    }

    bool Client::isCameraSharing() const {
        return m_stateManager.isCameraSharing();
    }

    bool Client::isViewingRemoteCamera() const {
        return m_stateManager.isViewingRemoteCamera();
    }

    bool Client::isMicrophoneMuted() const {
        return m_audioEngine.isMicrophoneMuted();
    }

    bool Client::isSpeakerMuted() const {
        return m_audioEngine.isSpeakerMuted();
    }

    void Client::refreshAudioDevices() {
        m_audioEngine.refreshAudioDevices();
    }

    int Client::getInputVolume() const {
        return m_audioEngine.getInputVolume();
    }

    int Client::getOutputVolume() const {
        return m_audioEngine.getOutputVolume();
    }

    void Client::setInputVolume(int volume) {
        m_audioEngine.setInputVolume(volume);
    }

    void Client::setOutputVolume(int volume) {
        m_audioEngine.setOutputVolume(volume);
    }

    bool Client::isAuthorized() const {
        return m_stateManager.isAuthorized();
    }

    bool Client::isOutgoingCall() const {
        return m_stateManager.isOutgoingCall();
    }

    bool Client::isActiveCall() const {
        return m_stateManager.isActiveCall();
    }

    bool Client::isConnectionDown() const {
        return m_stateManager.isConnectionDown();
    }

    int Client::getIncomingCallsCount() const {
        return m_stateManager.getIncomingCallsCount();
    }

    const std::string& Client::getMyNickname() const {
        return m_stateManager.getMyNickname();
    }

    const std::string& Client::getNicknameWhomCalling() const {
        return m_stateManager.getOutgoingCall().getNickname();
    }

    const std::string& Client::getNicknameInCallWith() const {
        return m_stateManager.getActiveCall().getNickname();
    }

    bool Client::authorize(const std::string& nickname) {
        if (m_stateManager.isAuthorized()) return false;

        if (!m_keyManager.isKeys()) {
            if (m_keyManager.isGeneratingKeys()) {
            }
            else {
                m_keyManager.generateKeys();
            }
        }

        m_keyManager.awaitKeysGeneration();

        auto [uid, packet] = PacketFactory::getAuthorizationPacket(nickname, m_keyManager.getMyPublicKey());

        createAndStartTask(uid, packet, PacketType::AUTHORIZATION,
            [this, nickname](std::optional<nlohmann::json> completionContext) {
                if (completionContext.has_value()) {
                    auto& context = completionContext.value();

                    bool successfullyAuthorized = context[RESULT].get<bool>();

                    if (successfullyAuthorized) {
                        std::string token = context[TOKEN];

                        m_stateManager.setMyNickname(nickname);
                        m_stateManager.setMyToken(token);
                        m_eventListener->onAuthorizationResult({});
                    }
                    else {
                        m_eventListener->onAuthorizationResult(ErrorCode::taken_nickname);
                    }
                }
            },
            [this](std::optional<nlohmann::json> failureContext) {
                if (!m_stateManager.isConnectionDown()) {
                    LOG_ERROR("Authorization task failed");
                    m_eventListener->onAuthorizationResult(ErrorCode::network_error);
                }
            }
        );

        return true;
    }

    bool Client::logout() {
        if (!m_stateManager.isAuthorized()) return false;

        auto [uid, packet] = PacketFactory::getLogoutPacket(m_stateManager.getMyNickname());

        createAndStartTask(uid, packet, PacketType::LOGOUT,
            [this](std::optional<nlohmann::json> completionContext) {
                reset();
                m_eventListener->onLogoutCompleted();
            },
            [this](std::optional<nlohmann::json> failureContext) {
                LOG_ERROR("Logout task failed");

                reset();
                m_eventListener->onLogoutCompleted();
            }
        );

        return true;
    }

    bool Client::startOutgoingCall(const std::string& userNickname) {
        if (!m_stateManager.isAuthorized() || m_stateManager.isActiveCall()) return false;

        auto& icomingCalls = m_stateManager.getIncomingCalls();

        bool acceptExistingCall = false;
        for (auto& [nickname, incomingCall] : icomingCalls) {
            if (nickname == userNickname) {
                acceptExistingCall = true;
            }
        }

        if (acceptExistingCall) {
            acceptCall(userNickname);
        }
        else {
            auto [uid, packet] = PacketFactory::getRequestUserInfoPacket(m_stateManager.getMyNickname(), userNickname);

            createAndStartTask(uid, packet, PacketType::GET_USER_INFO,
                [this, userNickname](std::optional<nlohmann::json> completionContext) {
                    if (completionContext.has_value()) {
                        auto& context = completionContext.value();

                        bool userInfoFound = context[RESULT].get<bool>();

                        if (userInfoFound) {
                            const std::string& userPublicKeyString = context[PUBLIC_KEY];
                            auto userPublicKey = crypto::deserializePublicKey(userPublicKeyString);

                            CryptoPP::SecByteBlock callKey;
                            crypto::generateAESKey(callKey);

                            auto [uid, packet] = PacketFactory::getStartOutgoingCallPacket(m_stateManager.getMyNickname(), userNickname, m_keyManager.getMyPublicKey(), userPublicKey, callKey);

                            createAndStartTask(uid, packet, PacketType::CALLING_BEGIN,
                                [this, userNickname](std::optional<nlohmann::json> completionContext) {
                                    m_stateManager.setOutgoingCall(userNickname, 32s, [this]() {
                                        m_stateManager.clearCallState();
                                        m_eventListener->onOutgoingCallTimeout({});
                                    });

                                    m_eventListener->onStartOutgoingCallResult({});
                                },
                                [this](std::optional<nlohmann::json> failureContext) {
                                    LOG_ERROR("Start outgoing call task failed");
                                    m_eventListener->onStartOutgoingCallResult(ErrorCode::network_error);
                                }
                            );
                        }
                        else {
                            m_eventListener->onStartOutgoingCallResult(ErrorCode::unexisting_user);
                        }
                    }
                },
                [this](std::optional<nlohmann::json> failureContext) {
                    LOG_ERROR("Request user info task failed");
                    m_eventListener->onStartOutgoingCallResult(ErrorCode::network_error);
                }
            );
        }

        return true;
    }

    bool Client::stopOutgoingCall() {
        if (!m_stateManager.isAuthorized() || !m_stateManager.isOutgoingCall()) return false;

        auto [uid, packet] = PacketFactory::getStopOutgoingCallPacket(m_stateManager.getMyNickname(), m_stateManager.getOutgoingCall().getNickname());

        createAndStartTask(uid, packet, PacketType::CALLING_END,
            [this](std::optional<nlohmann::json> completionContext) {
                m_eventListener->onStopOutgoingCallResult({});
            },
            [this](std::optional<nlohmann::json> failureContext) {
                LOG_ERROR("Stop outgoing call task failed");
                m_eventListener->onStopOutgoingCallResult(ErrorCode::network_error);
            }
        );

        return true;
    }

    bool Client::acceptCall(const std::string& userNickname) {
        if (!m_stateManager.isAuthorized() || !m_stateManager.isIncomingCalls()) return false;

        auto& incomingCalls = m_stateManager.getIncomingCalls();
        if (!incomingCalls.contains(userNickname)) return false;

        for (auto& [nickname, incomingCallData] : incomingCalls) {
            if (nickname != userNickname) {
                auto [uid, packet] = PacketFactory::getDeclineCallPacket(m_stateManager.getMyNickname(), nickname);

                createAndStartTask(uid, packet, PacketType::CALL_DECLINE,
                    [this, nickname](std::optional<nlohmann::json> completionContext) {
                        auto& incomingCalls = m_stateManager.getIncomingCalls();
                        m_stateManager.removeIncomingCall(nickname);
                    },
                    [this](std::optional<nlohmann::json> failureContext) {
                        LOG_ERROR("Decline incoming call task failed (as part of accept call operation)");
                    }
                );
            }
        }

        if (m_stateManager.isOutgoingCall()) {
            auto [uid, packet] = PacketFactory::getStopOutgoingCallPacket(m_stateManager.getMyNickname(), m_stateManager.getOutgoingCall().getNickname());

            createAndStartTask(uid, packet, PacketType::CALLING_END,
                [this, userNickname](std::optional<nlohmann::json> completionContext) {
                    auto& incomingCalls = m_stateManager.getIncomingCalls();
                    auto& incomingCall = incomingCalls.at(userNickname);
                    auto [uid, packet] = PacketFactory::getAcceptCallPacket(m_stateManager.getMyNickname(), userNickname, m_keyManager.getMyPublicKey(), incomingCall.getPublicKey(), incomingCall.getCallKey());

                    createAndStartTask(uid, packet, PacketType::CALL_ACCEPT,
                        [this, userNickname](std::optional<nlohmann::json> completionContext) {
                            auto& incomingCalls = m_stateManager.getIncomingCalls();
                            auto& incomingCall = incomingCalls.at(userNickname);
                            m_stateManager.setActiveCall(incomingCall.getNickname(), incomingCall.getPublicKey(), incomingCall.getCallKey());
                            m_stateManager.removeIncomingCall(userNickname);

                            m_audioEngine.startStream();
                            m_eventListener->onAcceptCallResult({});
                        },
                        [this](std::optional<nlohmann::json> failureContext) {
                            LOG_ERROR("Accept call task failed");
                            m_eventListener->onAcceptCallResult(ErrorCode::network_error);
                        }
                    );
                },
                [this](std::optional<nlohmann::json> failureContext) {
                    LOG_ERROR("Stop outgoing call task failed (as part of accept call operation)");
                    m_eventListener->onAcceptCallResult(ErrorCode::network_error);
                }
            );
        }
        else if (m_stateManager.isActiveCall()) {
            auto [uid, packet] = PacketFactory::getEndCallPacket(m_stateManager.getMyNickname(), m_stateManager.getActiveCall().getNickname());

            createAndStartTask(uid, packet, PacketType::CALL_END,
                [this, userNickname](std::optional<nlohmann::json> completionContext) {
                    m_stateManager.setScreenSharing(false);
                    m_stateManager.setCameraSharing(false);
                    m_stateManager.setViewingRemoteScreen(false);
                    m_stateManager.setViewingRemoteCamera(false);
                    m_stateManager.clearCallState();
                    m_audioEngine.stopStream();

                    auto& incomingCalls = m_stateManager.getIncomingCalls();
                    auto& incomingCall = incomingCalls.at(userNickname);
                    auto [uid, packet] = PacketFactory::getAcceptCallPacket(m_stateManager.getMyNickname(), userNickname, m_keyManager.getMyPublicKey(), incomingCall.getPublicKey(), incomingCall.getCallKey());

                    createAndStartTask(uid, packet, PacketType::CALL_ACCEPT,
                        [this, userNickname](std::optional<nlohmann::json> completionContext) {
                            auto& incomingCalls = m_stateManager.getIncomingCalls();
                            auto& incomingCall = incomingCalls.at(userNickname);
                            m_stateManager.setActiveCall(incomingCall.getNickname(), incomingCall.getPublicKey(), incomingCall.getCallKey());
                            m_stateManager.removeIncomingCall(userNickname);

                            m_audioEngine.startStream();
                            m_eventListener->onAcceptCallResult({});
                        },
                        [this](std::optional<nlohmann::json> failureContext) {
                            LOG_ERROR("Accept call task failed");
                            m_eventListener->onEndCallResult({});
                            m_eventListener->onAcceptCallResult(ErrorCode::network_error);
                        }
                    );
                },
                [this](std::optional<nlohmann::json> failureContext) {
                    LOG_ERROR("End active call task failed (as part of accept new call operation)");
                    m_eventListener->onAcceptCallResult(ErrorCode::network_error);
                }
            );
        }
        else {
            auto& incomingCalls = m_stateManager.getIncomingCalls();
            auto& incomingCall = incomingCalls.at(userNickname);
            auto [uid, packet] = PacketFactory::getAcceptCallPacket(m_stateManager.getMyNickname(), userNickname, m_keyManager.getMyPublicKey(), incomingCall.getPublicKey(), incomingCall.getCallKey());

            createAndStartTask(uid, packet, PacketType::CALL_ACCEPT,
                [this, userNickname](std::optional<nlohmann::json> completionContext) {
                    auto& incomingCalls = m_stateManager.getIncomingCalls();
                    auto& incomingCall = incomingCalls.at(userNickname);
                    m_stateManager.setActiveCall(incomingCall.getNickname(), incomingCall.getPublicKey(), incomingCall.getCallKey());
                    m_stateManager.removeIncomingCall(userNickname);

                    m_audioEngine.startStream();
                    m_eventListener->onAcceptCallResult({});
                },
                [this](std::optional<nlohmann::json> failureContext) {
                    LOG_ERROR("Accept call task failed");
                    m_eventListener->onAcceptCallResult(ErrorCode::network_error);
                }
            );
        }

        return true;
    } 

    bool Client::declineCall(const std::string& userNickname) {
        if (!m_stateManager.isAuthorized() || !m_stateManager.isIncomingCalls()) return false;

        auto& incomingCalls = m_stateManager.getIncomingCalls();
        if (!incomingCalls.contains(userNickname)) return false;

        auto [uid, packet] = PacketFactory::getDeclineCallPacket(m_stateManager.getMyNickname(), userNickname);

        createAndStartTask(uid, packet, PacketType::CALL_DECLINE,
            [this, userNickname](std::optional<nlohmann::json> completionContext) {
                auto& incomingCalls = m_stateManager.getIncomingCalls();
                m_stateManager.removeIncomingCall(userNickname);

                m_eventListener->onDeclineCallResult({});
            },
            [this](std::optional<nlohmann::json> failureContext) {
                LOG_ERROR("Decline incoming call task failed");
                m_eventListener->onDeclineCallResult(ErrorCode::network_error);
            }
        );

        return true;
    }

    bool Client::endCall() {
        if (!m_stateManager.isAuthorized() || !m_stateManager.isActiveCall()) return false;

        auto [uid, packet] = PacketFactory::getEndCallPacket(m_stateManager.getMyNickname(), m_stateManager.getActiveCall().getNickname());

        createAndStartTask(uid, packet, PacketType::CALL_END,
            [this](std::optional<nlohmann::json> completionContext) {
                m_stateManager.setScreenSharing(false);
                m_stateManager.setCameraSharing(false);
                m_stateManager.setViewingRemoteScreen(false);
                m_stateManager.setViewingRemoteCamera(false);
                m_stateManager.clearCallState();
                m_audioEngine.stopStream();

                m_eventListener->onEndCallResult({});
            },
            [this](std::optional<nlohmann::json> failureContext) {
                LOG_ERROR("Stop outgoing call task failed");
                m_eventListener->onEndCallResult(ErrorCode::network_error);
            }
        );

        return true;
    }

    bool Client::startScreenSharing() {
        if (!m_stateManager.isAuthorized() || !m_stateManager.isActiveCall() || m_stateManager.isScreenSharing() || m_stateManager.isViewingRemoteScreen()) return false;

        const std::string& friendNickname = m_stateManager.getActiveCall().getNickname();
        std::string friendNicknameHash = crypto::calculateHash(friendNickname);
        auto [uid, packet] = PacketFactory::getStartScreenSharingPacket(m_stateManager.getMyNickname(), friendNicknameHash);

        createAndStartTask(uid, packet, PacketType::SCREEN_SHARING_BEGIN,
            [this](std::optional<nlohmann::json> completionContext) {
                m_eventListener->onStartScreenSharingResult({});
            },
            [this](std::optional<nlohmann::json> failureContext) {
                LOG_ERROR("Start screen sharing task failed");
                m_eventListener->onStartScreenSharingResult(ErrorCode::network_error);
            }
        );

        return true;
    }

    bool Client::stopScreenSharing() {
        if (!m_stateManager.isAuthorized() || !m_stateManager.isActiveCall() || !m_stateManager.isScreenSharing()) return false;

        const std::string& friendNickname = m_stateManager.getActiveCall().getNickname();
        std::string friendNicknameHash = crypto::calculateHash(friendNickname);
        auto [uid, packet] = PacketFactory::getStopScreenSharingPacket(m_stateManager.getMyNickname(), friendNicknameHash);

        createAndStartTask(uid, packet, PacketType::SCREEN_SHARING_END,
            [this](std::optional<nlohmann::json> completionContext) {
                m_eventListener->onStopScreenSharingResult({});
            },
            [this](std::optional<nlohmann::json> failureContext) {
                LOG_ERROR("Stop screen sharing task failed");
                m_eventListener->onStopScreenSharingResult(ErrorCode::network_error);
            }
        );

        return true;
    }

    bool Client::sendScreen(const std::vector<unsigned char>& data) {
        if (!m_stateManager.isAuthorized() || !m_stateManager.isActiveCall() || !m_stateManager.isScreenSharing()) return false;

        const CryptoPP::SecByteBlock& callKey = m_stateManager.getActiveCall().getCallKey();

        try {
            size_t cipherDataLength = data.size() + CryptoPP::AES::BLOCKSIZE;
            std::vector<CryptoPP::byte> cipherData(cipherDataLength);

            crypto::AESEncrypt(callKey,
                reinterpret_cast<const CryptoPP::byte*>(data.data()),
                data.size(),
                cipherData.data(),
                cipherDataLength);

            m_networkController.send(std::move(cipherData), static_cast<uint32_t>(PacketType::SCREEN));
            return true;
        }
        catch (const std::exception& e) {
            LOG_ERROR("Screen sending error");
            return false;
        }
    }

    bool Client::startCameraSharing() {
        if (!m_stateManager.isAuthorized() || !m_stateManager.isActiveCall() || m_stateManager.isCameraSharing()) return false;

        const std::string& friendNickname = m_stateManager.getActiveCall().getNickname();
        std::string friendNicknameHash = crypto::calculateHash(friendNickname);
        auto [uid, packet] = PacketFactory::getStartCameraSharingPacket(m_stateManager.getMyNickname(), friendNicknameHash);

        createAndStartTask(uid, packet, PacketType::CAMERA_SHARING_BEGIN,
            [this](std::optional<nlohmann::json> completionContext) {
                m_eventListener->onStartCameraSharingResult({});
            },
            [this](std::optional<nlohmann::json> failureContext) {
                LOG_ERROR("Start camera sharing task failed");
                m_eventListener->onStartCameraSharingResult(ErrorCode::network_error);
            }
        );

        return true;
    }

    bool Client::stopCameraSharing() {
        if (!m_stateManager.isAuthorized() || !m_stateManager.isActiveCall() || !m_stateManager.isCameraSharing()) return false;

        const std::string& friendNickname = m_stateManager.getActiveCall().getNickname();
        std::string friendNicknameHash = crypto::calculateHash(friendNickname);
        auto [uid, packet] = PacketFactory::getStopCameraSharingPacket(m_stateManager.getMyNickname(), friendNicknameHash);

        createAndStartTask(uid, packet, PacketType::CAMERA_SHARING_END,
            [this](std::optional<nlohmann::json> completionContext) {
                m_eventListener->onStopCameraSharingResult({});
            },
            [this](std::optional<nlohmann::json> failureContext) {
                LOG_ERROR("Stop camera sharing task failed");
                m_eventListener->onStopCameraSharingResult(ErrorCode::network_error);
            }
        );

        return true;
    }

    bool Client::sendCamera(const std::vector<unsigned char>& data) {
        if (!m_stateManager.isAuthorized() || !m_stateManager.isActiveCall() || !m_stateManager.isCameraSharing()) return false;

        const CryptoPP::SecByteBlock& callKey = m_stateManager.getActiveCall().getCallKey();

        try {
            size_t cipherDataLength = data.size() + CryptoPP::AES::BLOCKSIZE;
            std::vector<CryptoPP::byte> cipherData(cipherDataLength);

            crypto::AESEncrypt(callKey,
                reinterpret_cast<const CryptoPP::byte*>(data.data()),
                data.size(),
                cipherData.data(),
                cipherDataLength);

            m_networkController.send(std::move(cipherData), static_cast<uint32_t>(PacketType::CAMERA));
            return true;
        }
        catch (const std::exception& e) {
            LOG_ERROR("Camera sending error");
            return false;
        }
    }

    void Client::onInputVoice(const unsigned char* data, int length) {
        if (!m_stateManager.isActiveCall() || m_stateManager.isConnectionDown()) return;

        const CryptoPP::SecByteBlock& callKey = m_stateManager.getActiveCall().getCallKey();

        size_t cipherDataLength = static_cast<size_t>(length) + CryptoPP::AES::BLOCKSIZE;
        std::vector<CryptoPP::byte> cipherData(cipherDataLength);
        crypto::AESEncrypt(callKey, data, length, cipherData.data(), cipherDataLength);
        
        m_networkController.send(std::move(cipherData), static_cast<uint32_t>(PacketType::VOICE));
    }
}