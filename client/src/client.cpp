#include "client.h"
#include "jsonType.h"
#include "packetFactory.h"
#include "utilities/crypto.h"
#include "errorCode.h"
#include "json.hpp"

#include "utilities/logger.h"

using namespace utilities;
using namespace std::chrono_literals;

namespace calls
{
    CallsClient::CallsClient()
        : m_packetProcessor(m_stateManager, m_keyManager, m_taskManager, m_networkController, m_audioEngine, m_eventListener)
    {
    }

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
                    m_eventListener->onOutgoingCallTimeout();
                }

                if (m_stateManager.isIncomingCalls()) {
                    auto& incomingCalls = m_stateManager.getIncomingCalls();

                    for (auto& [nickname, incomingCall] : incomingCalls) {
                        m_eventListener->onIncomingCallExpired(nickname);
                    }

                    m_stateManager.clearIncomingCalls();
                }

                m_eventListener->onConnectionDown();
            },
            [this]() {
                LOG_INFO("Ping restored");

                auto [uid, packet] = PacketFactory::getReconnectPacket(m_stateManager.getMyNickname(), m_stateManager.getMyToken());

                m_taskManager.createTask(uid, 1500ms, 5,
                    [this, packet]() {
                        m_networkController.send(packet, static_cast<uint32_t>(PacketType::RECONNECT));
                    },
                    [this](std::optional<nlohmann::json> completionContext) {
                        if (m_stateManager.isAuthorized())
                            m_eventListener->onConnectionRestored();
                        else
                            m_eventListener->onConnectionRestoredAuthorizationNeeded();
                    },
                    [this](std::optional<nlohmann::json> failureContext) {
                        LOG_ERROR("Reconnect task failed");
                    }
                );

                m_taskManager.startTask(uid);
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

    void CallsClient::reset() {
        m_stateManager.reset();
        m_keyManager.resetKeys();
        m_taskManager.cancelAllTasks();

        if (m_audioEngine.isStream())
            m_audioEngine.stopStream();
    }

    void CallsClient::onReceive(const unsigned char* data, int length, PacketType type) {
        m_packetProcessor.processPacket(data, length, type);
    }

    std::vector<std::string> CallsClient::getCallers() const {
        auto& incomingCalls = m_stateManager.getIncomingCalls();

        std::vector<std::string> callersNicknames;
        for (auto& [nickname, incomingCall] : incomingCalls) {
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

    bool CallsClient::isScreenSharing() const {
        return m_stateManager.isScreenSharing();
    }

    bool CallsClient::isViewingRemoteScreen() const {
        return m_stateManager.isViewingRemoteScreen();
    }

    bool CallsClient::isCameraSharing() const {
        return m_stateManager.isCameraSharing();
    }

    bool CallsClient::isViewingRemoteCamera() const {
        return m_stateManager.isViewingRemoteCamera();
    }

    bool CallsClient::isMicrophoneMuted() const {
        return m_audioEngine.isMicrophoneMuted();
    }

    bool CallsClient::isSpeakerMuted() const {
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
        return m_stateManager.isOutgoingCall();
    }

    bool CallsClient::isActiveCall() const {
        return m_stateManager.isActiveCall();
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
        return m_stateManager.getOutgoingCall().getNickname();
    }

    const std::string& CallsClient::getNicknameInCallWith() const {
        return m_stateManager.getActiveCall().getNickname();
    }

    bool CallsClient::authorize(const std::string& nickname) {
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

        m_taskManager.createTask(uid, 1500ms, 5,
            [this, packet]() {
                m_networkController.send(packet, static_cast<uint32_t>(PacketType::AUTHORIZATION));
            },
            [this, nickname](std::optional<nlohmann::json> completionContext) {
                if (completionContext.has_value()) {
                    auto& context = completionContext.value();

                    bool successfullyAuthorized = context[RESULT].get<bool>();

                    if (successfullyAuthorized) {
                        m_stateManager.setMyNickname(nickname);
                        m_eventListener->onAuthorizationResult({});
                    }
                    else {
                        m_eventListener->onAuthorizationResult(CallsErrorCode::taken_nickname);
                    }
                }
            },
            [this](std::optional<nlohmann::json> failureContext) {
                if (!m_stateManager.isConnectionDown()) {
                    LOG_ERROR("Authorization task failed");
                    m_eventListener->onAuthorizationResult(CallsErrorCode::network_error);
                }
            }
        );

        m_taskManager.startTask(uid);

        return true;
    }

    bool CallsClient::logout() {
        if (!m_stateManager.isAuthorized()) return false;

        auto [uid, packet] = PacketFactory::getLogoutPacket(m_stateManager.getMyNickname());

        m_taskManager.createTask(uid, 1500ms, 5,
            [this, packet]() {
                m_networkController.send(packet, static_cast<uint32_t>(PacketType::LOGOUT));
            },
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

        m_taskManager.startTask(uid);

        return true;
    }

    bool CallsClient::startOutgoingCall(const std::string& userNickname) {
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

            m_taskManager.createTask(uid, 1500ms, 5,
                [this, packet]() {
                    m_networkController.send(packet, static_cast<uint32_t>(PacketType::GET_USER_INFO));
                },
                [this, userNickname](std::optional<nlohmann::json> completionContext) {
                    if (completionContext.has_value()) {
                        auto& context = completionContext.value();

                        bool userInfoFound = context[RESULT].get<bool>();

                        if (userInfoFound) {
                            const std::string& userNicknameHash = context[NICKNAME_HASH];
                            const std::string& userPublicKeyString = context[PUBLIC_KEY];
                            auto userPublicKey = crypto::deserializePublicKey(userPublicKeyString);

                            CryptoPP::SecByteBlock callKey;
                            crypto::generateAESKey(callKey);

                            auto [uid, packet] = PacketFactory::getStartOutgoingCallPacket(m_stateManager.getMyNickname(), userNickname, m_keyManager.getMyPublicKey(), userPublicKey, callKey);

                            m_taskManager.createTask(uid, 1500ms, 5,
                                [this, packet]() {
                                    m_networkController.send(packet, static_cast<uint32_t>(PacketType::CALLING_BEGIN));
                                },
                                [this, userNickname](std::optional<nlohmann::json> completionContext) {
                                    m_stateManager.setOutgoingCall(userNickname, 32s, [this]() {
                                        m_stateManager.clearCallState();
                                        m_eventListener->onOutgoingCallTimeout();
                                        });

                                    m_eventListener->onStartOutgoingCallResult({});
                                },
                                [this](std::optional<nlohmann::json> failureContext) {
                                    LOG_ERROR("Start outgoing call task failed");
                                    m_eventListener->onStartOutgoingCallResult(CallsErrorCode::network_error);
                                }
                            );

                            m_taskManager.startTask(uid);
                        }
                        else {
                            m_eventListener->onStartOutgoingCallResult(CallsErrorCode::unexisting_user);
                        }
                    }
                },
                [this](std::optional<nlohmann::json> failureContext) {
                    LOG_ERROR("Request user info task failed");
                    m_eventListener->onStartOutgoingCallResult(CallsErrorCode::network_error);
                }
            );

            m_taskManager.startTask(uid);
        }

        return true;
    }

    bool CallsClient::stopOutgoingCall() {
        if (!m_stateManager.isAuthorized() || !m_stateManager.isOutgoingCall()) return false;

        auto [uid, packet] = PacketFactory::getStopOutgoingCallPacket(m_stateManager.getMyNickname(), m_stateManager.getOutgoingCall().getNickname());

        m_taskManager.createTask(uid, 1500ms, 5,
            [this, packet]() {
                m_networkController.send(packet, static_cast<uint32_t>(PacketType::CALLING_END));
            },
            [this](std::optional<nlohmann::json> completionContext) {
                m_eventListener->onStopOutgoingCallResult({});
            },
            [this](std::optional<nlohmann::json> failureContext) {
                LOG_ERROR("Stop outgoing call task failed");
                m_eventListener->onStopOutgoingCallResult(CallsErrorCode::network_error);
            }
        );

        m_taskManager.startTask(uid);

        return true;
    }

    bool CallsClient::acceptCall(const std::string& userNickname) {
        if (!m_stateManager.isAuthorized() || !m_stateManager.isIncomingCalls()) return false;

        auto& incomingCalls = m_stateManager.getIncomingCalls();
        if (!incomingCalls.contains(userNickname)) return false;

        for (auto& [nickname, incomingCallData] : incomingCalls) {
            if (nickname != userNickname) {
                auto [uid, packet] = PacketFactory::getDeclineCallPacket(m_stateManager.getMyNickname(), nickname);

                m_taskManager.createTask(uid, 1500ms, 5,
                    [this, packet]() {
                        m_networkController.send(packet, static_cast<uint32_t>(PacketType::CALL_DECLINE));
                    },
                    [this, nickname](std::optional<nlohmann::json> completionContext) {
                        auto& incomingCalls = m_stateManager.getIncomingCalls();
                        m_stateManager.removeIncomingCall(nickname);
                    },
                    [this](std::optional<nlohmann::json> failureContext) {
                        LOG_ERROR("Decline incoming call task failed (as part of accept call operation)");
                    }
                );

                m_taskManager.startTask(uid);
            }
        }

        if (m_stateManager.isOutgoingCall()) {
            auto [uid, packet] = PacketFactory::getStopOutgoingCallPacket(m_stateManager.getMyNickname(), m_stateManager.getOutgoingCall().getNickname());

            m_taskManager.createTask(uid, 1500ms, 5,
                [this, packet]() {
                    m_networkController.send(packet, static_cast<uint32_t>(PacketType::CALLING_END));
                },
                [this, userNickname](std::optional<nlohmann::json> completionContext) {
                    auto& incomingCalls = m_stateManager.getIncomingCalls();
                    auto& incomingCall = incomingCalls.at(userNickname);
                    auto [uid, packet] = PacketFactory::getAcceptCallPacket(m_stateManager.getMyNickname(), userNickname, m_keyManager.getMyPublicKey(), incomingCall.getPublicKey(), incomingCall.getCallKey());

                    m_taskManager.createTask(uid, 1500ms, 5,
                        [this, packet]() {
                            m_networkController.send(packet, static_cast<uint32_t>(PacketType::CALL_ACCEPT));
                        },
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
                            m_eventListener->onAcceptCallResult(CallsErrorCode::network_error);
                        }
                    );

                    m_taskManager.startTask(uid);
                },
                [this](std::optional<nlohmann::json> failureContext) {
                    LOG_ERROR("Stop outgoing call task failed (as part of accept call operation)");
                    m_eventListener->onAcceptCallResult(CallsErrorCode::network_error);
                }
            );

            m_taskManager.startTask(uid);
        }
        else if (m_stateManager.isActiveCall()) {
            auto [uid, packet] = PacketFactory::getEndCallPacket(m_stateManager.getMyNickname(), m_stateManager.getActiveCall().getNickname());

            m_taskManager.createTask(uid, 1500ms, 5,
                [this, packet]() {
                    m_networkController.send(packet, static_cast<uint32_t>(PacketType::CALL_END));
                },
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

                    m_taskManager.createTask(uid, 1500ms, 5,
                        [this, packet]() {
                            m_networkController.send(packet, static_cast<uint32_t>(PacketType::CALL_ACCEPT));
                        },
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
                            m_eventListener->onAcceptCallResult(CallsErrorCode::network_error);
                        }
                    );

                    m_taskManager.startTask(uid);
                },
                [this](std::optional<nlohmann::json> failureContext) {
                    LOG_ERROR("End active call task failed (as part of accept new call operation)");
                    m_eventListener->onAcceptCallResult(CallsErrorCode::network_error);
                }
            );

            m_taskManager.startTask(uid);
        }
        else {
            auto& incomingCalls = m_stateManager.getIncomingCalls();
            auto& incomingCall = incomingCalls.at(userNickname);
            auto [uid, packet] = PacketFactory::getAcceptCallPacket(m_stateManager.getMyNickname(), userNickname, m_keyManager.getMyPublicKey(), incomingCall.getPublicKey(), incomingCall.getCallKey());

            m_taskManager.createTask(uid, 1500ms, 5,
                [this, packet]() {
                    m_networkController.send(packet, static_cast<uint32_t>(PacketType::CALL_ACCEPT));
                },
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
                    m_eventListener->onAcceptCallResult(CallsErrorCode::network_error);
                }
            );

            m_taskManager.startTask(uid);
        }

        return true;
    } 

    bool CallsClient::declineCall(const std::string& userNickname) {
        if (!m_stateManager.isAuthorized() || !m_stateManager.isIncomingCalls()) return false;

        auto& incomingCalls = m_stateManager.getIncomingCalls();
        if (!incomingCalls.contains(userNickname)) return false;

        auto [uid, packet] = PacketFactory::getDeclineCallPacket(m_stateManager.getMyNickname(), userNickname);

        m_taskManager.createTask(uid, 1500ms, 5,
            [this, packet]() {
                m_networkController.send(packet, static_cast<uint32_t>(PacketType::CALL_DECLINE));
            },
            [this, userNickname](std::optional<nlohmann::json> completionContext) {
                auto& incomingCalls = m_stateManager.getIncomingCalls();
                m_stateManager.removeIncomingCall(userNickname);

                m_eventListener->onDeclineCallResult({});
            },
            [this](std::optional<nlohmann::json> failureContext) {
                LOG_ERROR("Decline incoming call task failed");
                m_eventListener->onDeclineCallResult(CallsErrorCode::network_error);
            }
        );

        m_taskManager.startTask(uid);

        return true;
    }

    bool CallsClient::endCall() {
        if (!m_stateManager.isAuthorized() || !m_stateManager.isActiveCall()) return false;

        auto [uid, packet] = PacketFactory::getEndCallPacket(m_stateManager.getMyNickname(), m_stateManager.getActiveCall().getNickname());

        m_taskManager.createTask(uid, 1500ms, 5,
            [this, packet]() {
                m_networkController.send(packet, static_cast<uint32_t>(PacketType::CALL_END));
            },
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
                m_eventListener->onEndCallResult(CallsErrorCode::network_error);
            }
        );

        m_taskManager.startTask(uid);

        return true;
    }

    bool CallsClient::startScreenSharing() {
        if (!m_stateManager.isAuthorized() || !m_stateManager.isActiveCall() || m_stateManager.isScreenSharing() || m_stateManager.isViewingRemoteScreen()) return false;

        const std::string& friendNickname = m_stateManager.getActiveCall().getNickname();
        std::string friendNicknameHash = crypto::calculateHash(friendNickname);
        auto [uid, packet] = PacketFactory::getStartScreenSharingPacket(m_stateManager.getMyNickname(), friendNicknameHash);

        m_taskManager.createTask(uid, 1500ms, 5,
            [this, packet]() {
                m_networkController.send(packet, static_cast<uint32_t>(PacketType::SCREEN_SHARING_BEGIN));
            },
            [this](std::optional<nlohmann::json> completionContext) {
                m_eventListener->onStartScreenSharingResult({});
            },
            [this](std::optional<nlohmann::json> failureContext) {
                LOG_ERROR("Start screen sharing task failed");
                m_eventListener->onStartScreenSharingResult(CallsErrorCode::network_error);
            }
        );

        m_taskManager.startTask(uid);

        return true;
    }

    bool CallsClient::stopScreenSharing() {
        if (!m_stateManager.isAuthorized() || !m_stateManager.isActiveCall() || !m_stateManager.isScreenSharing()) return false;

        const std::string& friendNickname = m_stateManager.getActiveCall().getNickname();
        std::string friendNicknameHash = crypto::calculateHash(friendNickname);
        auto [uid, packet] = PacketFactory::getStopScreenSharingPacket(m_stateManager.getMyNickname(), friendNicknameHash);

        m_taskManager.createTask(uid, 1500ms, 5,
            [this, packet]() {
                m_networkController.send(packet, static_cast<uint32_t>(PacketType::SCREEN_SHARING_END));
            },
            [this](std::optional<nlohmann::json> completionContext) {
                m_eventListener->onStopScreenSharingResult({});
            },
            [this](std::optional<nlohmann::json> failureContext) {
                LOG_ERROR("Stop screen sharing task failed");
                m_eventListener->onStopScreenSharingResult(CallsErrorCode::network_error);
            }
        );

        m_taskManager.startTask(uid);

        return true;
    }

    bool CallsClient::sendScreen(const std::vector<unsigned char>& data) {
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

    bool CallsClient::startCameraSharing() {
        if (!m_stateManager.isAuthorized() || !m_stateManager.isActiveCall() || m_stateManager.isCameraSharing()) return false;

        const std::string& friendNickname = m_stateManager.getActiveCall().getNickname();
        std::string friendNicknameHash = crypto::calculateHash(friendNickname);
        auto [uid, packet] = PacketFactory::getStartCameraSharingPacket(m_stateManager.getMyNickname(), friendNicknameHash);

        m_taskManager.createTask(uid, 1500ms, 5,
            [this, packet]() {
                m_networkController.send(packet, static_cast<uint32_t>(PacketType::CAMERA_SHARING_BEGIN));
            },
            [this](std::optional<nlohmann::json> completionContext) {
                m_eventListener->onStartCameraSharingResult({});
            },
            [this](std::optional<nlohmann::json> failureContext) {
                LOG_ERROR("Start camera sharing task failed");
                m_eventListener->onStartCameraSharingResult(CallsErrorCode::network_error);
            }
        );

        m_taskManager.startTask(uid);

        return true;
    }

    bool CallsClient::stopCameraSharing() {
        if (!m_stateManager.isAuthorized() || !m_stateManager.isActiveCall() || !m_stateManager.isCameraSharing()) return false;

        const std::string& friendNickname = m_stateManager.getActiveCall().getNickname();
        std::string friendNicknameHash = crypto::calculateHash(friendNickname);
        auto [uid, packet] = PacketFactory::getStopCameraSharingPacket(m_stateManager.getMyNickname(), friendNicknameHash);

        m_taskManager.createTask(uid, 1500ms, 5,
            [this, packet]() {
                m_networkController.send(packet, static_cast<uint32_t>(PacketType::CAMERA_SHARING_END));
            },
            [this](std::optional<nlohmann::json> completionContext) {
                m_eventListener->onStopCameraSharingResult({});
            },
            [this](std::optional<nlohmann::json> failureContext) {
                LOG_ERROR("Stop camera sharing task failed");
                m_eventListener->onStopCameraSharingResult(CallsErrorCode::network_error);
            }
        );

        m_taskManager.startTask(uid);

        return true;
    }

    bool CallsClient::sendCamera(const std::vector<unsigned char>& data) {
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

    void CallsClient::onInputVoice(const unsigned char* data, int length) {
        if (!m_stateManager.isActiveCall() || m_stateManager.isConnectionDown()) return;

        const CryptoPP::SecByteBlock& callKey = m_stateManager.getActiveCall().getCallKey();

        size_t cipherDataLength = static_cast<size_t>(length) + CryptoPP::AES::BLOCKSIZE;
        std::vector<CryptoPP::byte> cipherData(cipherDataLength);
        crypto::AESEncrypt(callKey, data, length, cipherData.data(), cipherDataLength);
        
        m_networkController.send(std::move(cipherData), static_cast<uint32_t>(PacketType::VOICE));
    }
}