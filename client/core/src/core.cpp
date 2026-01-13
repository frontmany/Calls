#include "core.h"
#include "jsonType.h"
#include "packetFactory.h"
#include "utilities/crypto.h"
#include "json.hpp"

#include "utilities/logger.h"

using namespace core::utilities;
using namespace std::chrono_literals;

namespace core
{
    Client::Client()
    {
    }

    Client::~Client() {
        reset();
        m_networkController.stop();
    }

    bool Client::start(const std::string& host,
        const std::string& port,
        std::shared_ptr<EventListener> eventListener)
    {
        m_eventListener = std::move(eventListener);
        m_packetProcessor = std::make_unique<PacketProcessor>(m_stateManager, m_keyManager, m_taskManager, m_networkController, m_audioEngine, m_eventListener);

        bool audioInitialized = m_audioEngine.init([this](const unsigned char* data, int length) {onInputVoice(data, length); });

        bool networkInitialized = m_networkController.init(host, port,
            [this](const unsigned char* data, int length, uint32_t type) {
                onReceive(data, length, static_cast<PacketType>(type));
            },
            [this]() {
                LOG_ERROR("Connection down");
                m_stateManager.setConnectionDown(true);
                m_operationManager.clearAllOperations();

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
                LOG_INFO("Connection restored");

                if (m_stateManager.isAuthorized()) {
                    auto [uid, packet] = PacketFactory::getReconnectPacket(m_stateManager.getMyNickname(), m_stateManager.getMyToken());

                    createAndStartTask(uid, packet, PacketType::RECONNECT,
                        [this](std::optional<nlohmann::json> completionContext) {
                            auto& context = completionContext.value();

                            m_stateManager.setConnectionDown(false);

                            bool reconnected = context[RESULT].get<bool>();
                            if (reconnected) {
                                bool activeCall = context[IS_ACTIVE_CALL].get<bool>();

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
                }
                else {
                    m_stateManager.setConnectionDown(false);
                    m_eventListener->onConnectionRestoredAuthorizationNeeded();
                }
            }
        );

        m_keyManager.generateKeys();

        if (!m_networkController.isRunning())
            m_networkController.start();

        if (audioInitialized && networkInitialized) {
            return true;
        }
        else {
            LOG_ERROR("Calls client initialization failed - audio: {}, network: {}", audioInitialized, networkInitialized);
            return false;
        }
    }

    void Client::stop() {
        reset();
        m_networkController.stop();
    }

    void Client::reset() {
        m_stateManager.reset();
        m_keyManager.resetKeys();
        m_taskManager.cancelAllTasks();
        m_operationManager.clearAllOperations();

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
        if (m_packetProcessor) {
            m_packetProcessor->processPacket(data, length, type);
        }
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

    bool Client::setInputDevice(int deviceIndex) {
        return m_audioEngine.setInputDevice(deviceIndex);
    }

    bool Client::setOutputDevice(int deviceIndex) {
        return m_audioEngine.setOutputDevice(deviceIndex);
    }

    int Client::getCurrentInputDevice() const {
        return m_audioEngine.getCurrentInputDevice();
    }

    int Client::getCurrentOutputDevice() const {
        return m_audioEngine.getCurrentOutputDevice();
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

    std::error_code Client::authorize(const std::string& nickname) {
        if (m_stateManager.isConnectionDown()) return make_error_code(ErrorCode::connection_down);
        if (m_stateManager.isAuthorized()) return make_error_code(ErrorCode::already_authorized);
        if (m_operationManager.isOperation(UserOperationType::AUTHORIZE)) return make_error_code(ErrorCode::operation_in_progress);

        if (!m_keyManager.isKeys()) {
            if (m_keyManager.isGeneratingKeys()) {
            }
            else {
                m_keyManager.generateKeys();
            }
        }

        m_keyManager.awaitKeysGeneration();

        m_operationManager.addOperation(UserOperationType::AUTHORIZE);

        auto [uid, packet] = PacketFactory::getAuthorizationPacket(nickname, m_keyManager.getMyPublicKey());

        createAndStartTask(uid, packet, PacketType::AUTHORIZATION,
            [this, nickname](std::optional<nlohmann::json> completionContext) {
                m_operationManager.removeOperation(UserOperationType::AUTHORIZE);

                if (completionContext.has_value()) {
                    auto& context = completionContext.value();

                    bool successfullyAuthorized = context[RESULT].get<bool>();

                    if (successfullyAuthorized) {
                        std::string token = context[TOKEN];

                        m_stateManager.setMyNickname(nickname);
                        m_stateManager.setMyToken(token);
                        m_stateManager.setAuthorized(true);
                        m_eventListener->onAuthorizationResult({});
                    }
                    else {
                        m_eventListener->onAuthorizationResult(ErrorCode::taken_nickname);
                    }
                }
            },
            [this](std::optional<nlohmann::json> failureContext) {
                m_operationManager.removeOperation(UserOperationType::AUTHORIZE);

                if (!m_stateManager.isConnectionDown()) {
                    LOG_ERROR("Authorization task failed");
                    m_eventListener->onAuthorizationResult(ErrorCode::network_error);
                }
            }
        );

        return {};
    }

    std::error_code Client::logout() {
        if (isConnectionDown()) return make_error_code(ErrorCode::connection_down);
        if (!m_stateManager.isAuthorized()) return make_error_code(ErrorCode::not_authorized);
        if (m_operationManager.isOperation(UserOperationType::LOGOUT)) return make_error_code(ErrorCode::operation_in_progress);

        m_operationManager.addOperation(UserOperationType::LOGOUT);

        auto [uid, packet] = PacketFactory::getLogoutPacket(m_stateManager.getMyNickname());

        createAndStartTask(uid, packet, PacketType::LOGOUT,
            [this](std::optional<nlohmann::json> completionContext) {
                m_operationManager.removeOperation(UserOperationType::LOGOUT);
                reset();
                m_eventListener->onLogoutCompleted();
            },
            [this](std::optional<nlohmann::json> failureContext) {
                m_operationManager.removeOperation(UserOperationType::LOGOUT);
                LOG_ERROR("Logout task failed");

                reset();
                m_eventListener->onLogoutCompleted();
            }
        );

        return {};
    }

    std::error_code Client::startOutgoingCall(const std::string& userNickname) {
        if (m_stateManager.isConnectionDown()) return make_error_code(ErrorCode::connection_down);
        if (!m_stateManager.isAuthorized()) return make_error_code(ErrorCode::not_authorized);
        if (m_stateManager.isActiveCall()) return make_error_code(ErrorCode::active_call_exists);
        if (m_operationManager.isOperationType(UserOperationType::START_OUTGOING_CALL)) return make_error_code(ErrorCode::operation_in_progress);

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
            m_operationManager.addOperation(UserOperationType::START_OUTGOING_CALL, userNickname);

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
                                    m_operationManager.removeOperation(UserOperationType::START_OUTGOING_CALL, userNickname);

                                    m_stateManager.setOutgoingCall(userNickname, 32s, [this]() {
                                        m_stateManager.clearCallState();
                                        m_eventListener->onOutgoingCallTimeout({});
                                    });

                                    m_eventListener->onStartOutgoingCallResult({});
                                },
                                [this, userNickname](std::optional<nlohmann::json> failureContext) {
                                    m_operationManager.removeOperation(UserOperationType::START_OUTGOING_CALL, userNickname);
                                    LOG_ERROR("Start outgoing call task failed");
                                    m_eventListener->onStartOutgoingCallResult(ErrorCode::network_error);
                                }
                            );
                        }
                        else {
                            m_operationManager.removeOperation(UserOperationType::START_OUTGOING_CALL, userNickname);
                            m_eventListener->onStartOutgoingCallResult(ErrorCode::unexisting_user);
                        }
                    }
                },
                [this, userNickname](std::optional<nlohmann::json> failureContext) {
                    m_operationManager.removeOperation(UserOperationType::START_OUTGOING_CALL, userNickname);
                    LOG_ERROR("Request user info task failed");
                    m_eventListener->onStartOutgoingCallResult(ErrorCode::network_error);
                }
            );
        }

        return {};
    }

    std::error_code Client::stopOutgoingCall() {
        if (m_stateManager.isConnectionDown()) return make_error_code(ErrorCode::connection_down);
        if (!m_stateManager.isAuthorized()) return make_error_code(ErrorCode::not_authorized);
        if (!m_stateManager.isOutgoingCall()) return make_error_code(ErrorCode::no_outgoing_call);
        if (m_operationManager.isOperationType(UserOperationType::STOP_OUTGOING_CALL)) return make_error_code(ErrorCode::operation_in_progress);

        const std::string& nickname = m_stateManager.getOutgoingCall().getNickname();

        m_operationManager.addOperation(UserOperationType::STOP_OUTGOING_CALL, nickname);

        auto [uid, packet] = PacketFactory::getStopOutgoingCallPacket(m_stateManager.getMyNickname(), nickname);

        createAndStartTask(uid, packet, PacketType::CALLING_END,
            [this, nickname](std::optional<nlohmann::json> completionContext) {
                m_operationManager.removeOperation(UserOperationType::STOP_OUTGOING_CALL, nickname);
                m_eventListener->onStopOutgoingCallResult({});
            },
            [this, nickname](std::optional<nlohmann::json> failureContext) {
                m_operationManager.removeOperation(UserOperationType::STOP_OUTGOING_CALL, nickname);
                LOG_ERROR("Stop outgoing call task failed");
                m_eventListener->onStopOutgoingCallResult(ErrorCode::network_error);
            }
        );

        return {};
    }

    std::error_code Client::acceptCall(const std::string& userNickname) {
        if (m_stateManager.isConnectionDown()) return make_error_code(ErrorCode::connection_down);
        if (!m_stateManager.isAuthorized()) return make_error_code(ErrorCode::not_authorized);
        if (!m_stateManager.isIncomingCalls()) return make_error_code(ErrorCode::no_incoming_call);
        if (m_operationManager.isOperationType(UserOperationType::ACCEPT_CALL)) return make_error_code(ErrorCode::operation_in_progress);

        auto& incomingCalls = m_stateManager.getIncomingCalls();
        if (!incomingCalls.contains(userNickname)) return make_error_code(ErrorCode::no_incoming_call);

        m_operationManager.addOperation(UserOperationType::ACCEPT_CALL, userNickname);

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
                            m_operationManager.removeOperation(UserOperationType::ACCEPT_CALL, userNickname);

                            auto& incomingCalls = m_stateManager.getIncomingCalls();
                            auto& incomingCall = incomingCalls.at(userNickname);
                            m_stateManager.setActiveCall(incomingCall.getNickname(), incomingCall.getPublicKey(), incomingCall.getCallKey());
                            m_stateManager.removeIncomingCall(userNickname);

                            m_audioEngine.startStream();
                            m_eventListener->onAcceptCallResult({}, userNickname);
                        },
                        [this, userNickname](std::optional<nlohmann::json> failureContext) {
                            m_operationManager.removeOperation(UserOperationType::ACCEPT_CALL, userNickname);
                            LOG_ERROR("Accept call task failed");
                            m_eventListener->onAcceptCallResult(ErrorCode::network_error, userNickname);
                        }
                    );
                },
                [this, userNickname](std::optional<nlohmann::json> failureContext) {
                    m_operationManager.removeOperation(UserOperationType::ACCEPT_CALL, userNickname);
                    LOG_ERROR("Stop outgoing call task failed (as part of accept call operation)");
                    m_eventListener->onAcceptCallResult(ErrorCode::network_error, userNickname);
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
                            m_operationManager.removeOperation(UserOperationType::ACCEPT_CALL, userNickname);

                            auto& incomingCalls = m_stateManager.getIncomingCalls();
                            auto& incomingCall = incomingCalls.at(userNickname);
                            m_stateManager.setActiveCall(incomingCall.getNickname(), incomingCall.getPublicKey(), incomingCall.getCallKey());
                            m_stateManager.removeIncomingCall(userNickname);

                            m_audioEngine.startStream();
                            m_eventListener->onAcceptCallResult({}, userNickname);
                        },
                        [this, userNickname](std::optional<nlohmann::json> failureContext) {
                            m_operationManager.removeOperation(UserOperationType::ACCEPT_CALL, userNickname);
                            LOG_ERROR("Accept call task failed");
                            m_eventListener->onEndCallResult({});
                            m_eventListener->onAcceptCallResult(ErrorCode::network_error, userNickname);
                        }
                    );
                },
                [this, userNickname](std::optional<nlohmann::json> failureContext) {
                    m_operationManager.removeOperation(UserOperationType::ACCEPT_CALL, userNickname);
                    LOG_ERROR("End active call task failed (as part of accept new call operation)");
                    m_eventListener->onAcceptCallResult(ErrorCode::network_error, userNickname);
                }
            );
        }
        else {
            auto& incomingCalls = m_stateManager.getIncomingCalls();
            auto& incomingCall = incomingCalls.at(userNickname);
            auto [uid, packet] = PacketFactory::getAcceptCallPacket(m_stateManager.getMyNickname(), userNickname, m_keyManager.getMyPublicKey(), incomingCall.getPublicKey(), incomingCall.getCallKey());

            createAndStartTask(uid, packet, PacketType::CALL_ACCEPT,
                [this, userNickname](std::optional<nlohmann::json> completionContext) {
                    m_operationManager.removeOperation(UserOperationType::ACCEPT_CALL, userNickname);

                    auto& incomingCalls = m_stateManager.getIncomingCalls();
                    auto& incomingCall = incomingCalls.at(userNickname);
                    m_stateManager.setActiveCall(incomingCall.getNickname(), incomingCall.getPublicKey(), incomingCall.getCallKey());
                    m_stateManager.removeIncomingCall(userNickname);

                    m_audioEngine.startStream();
                    m_eventListener->onAcceptCallResult({}, userNickname);
                },
                [this, userNickname](std::optional<nlohmann::json> failureContext) {
                    m_operationManager.removeOperation(UserOperationType::ACCEPT_CALL, userNickname);
                    LOG_ERROR("Accept call task failed");
                    m_eventListener->onAcceptCallResult(ErrorCode::network_error, userNickname);
                }
            );
        }

        return {};
    } 

    std::error_code Client::declineCall(const std::string& userNickname) {
        if (m_stateManager.isConnectionDown()) return make_error_code(ErrorCode::connection_down);
        if (!m_stateManager.isAuthorized()) return make_error_code(ErrorCode::not_authorized);
        if (!m_stateManager.isIncomingCalls()) return make_error_code(ErrorCode::no_incoming_call);
        if (m_operationManager.isOperation(UserOperationType::DECLINE_CALL, userNickname)) return make_error_code(ErrorCode::operation_in_progress);

        auto& incomingCalls = m_stateManager.getIncomingCalls();
        if (!incomingCalls.contains(userNickname)) return make_error_code(ErrorCode::no_incoming_call);

        m_operationManager.addOperation(UserOperationType::DECLINE_CALL, userNickname);

        auto [uid, packet] = PacketFactory::getDeclineCallPacket(m_stateManager.getMyNickname(), userNickname);

        createAndStartTask(uid, packet, PacketType::CALL_DECLINE,
            [this, userNickname](std::optional<nlohmann::json> completionContext) {
                m_operationManager.removeOperation(UserOperationType::DECLINE_CALL, userNickname);

                m_stateManager.removeIncomingCall(userNickname);

                m_eventListener->onDeclineCallResult({}, userNickname);
            },
            [this, userNickname](std::optional<nlohmann::json> failureContext) {
                m_operationManager.removeOperation(UserOperationType::DECLINE_CALL, userNickname);
                LOG_ERROR("Decline incoming call task failed");
                m_eventListener->onDeclineCallResult(make_error_code(ErrorCode::network_error), userNickname);
            }
        );

        return {};
    }

    std::error_code Client::endCall() {
        if (m_stateManager.isConnectionDown()) return make_error_code(ErrorCode::connection_down);
        if (!m_stateManager.isAuthorized()) return make_error_code(ErrorCode::not_authorized);
        if (!m_stateManager.isActiveCall()) return make_error_code(ErrorCode::no_active_call);
        if (m_operationManager.isOperationType(UserOperationType::END_CALL)) return make_error_code(ErrorCode::operation_in_progress);

        const std::string& nickname = m_stateManager.getActiveCall().getNickname();

        m_operationManager.addOperation(UserOperationType::END_CALL, nickname);

        auto [uid, packet] = PacketFactory::getEndCallPacket(m_stateManager.getMyNickname(), nickname);

        createAndStartTask(uid, packet, PacketType::CALL_END,
            [this, nickname](std::optional<nlohmann::json> completionContext) {
                m_operationManager.removeOperation(UserOperationType::END_CALL, nickname);

                m_stateManager.setScreenSharing(false);
                m_stateManager.setCameraSharing(false);
                m_stateManager.setViewingRemoteScreen(false);
                m_stateManager.setViewingRemoteCamera(false);
                m_stateManager.clearCallState();
                m_audioEngine.stopStream();

                m_eventListener->onEndCallResult({});
            },
            [this, nickname](std::optional<nlohmann::json> failureContext) {
                m_operationManager.removeOperation(UserOperationType::END_CALL, nickname);
                LOG_ERROR("Stop outgoing call task failed");
                m_eventListener->onEndCallResult(ErrorCode::network_error);
            }
        );

        return {};
    }

    std::error_code Client::startScreenSharing() {
        if (m_stateManager.isConnectionDown()) return make_error_code(ErrorCode::connection_down);
        if (!m_stateManager.isAuthorized()) return make_error_code(ErrorCode::not_authorized);
        if (!m_stateManager.isActiveCall()) return make_error_code(ErrorCode::no_active_call);
        if (m_stateManager.isScreenSharing()) return make_error_code(ErrorCode::screen_sharing_already_active);
        if (m_stateManager.isViewingRemoteScreen()) return make_error_code(ErrorCode::viewing_remote_screen);
        if (m_operationManager.isOperationType(UserOperationType::START_SCREEN_SHARING)) return make_error_code(ErrorCode::operation_in_progress);

        m_operationManager.addOperation(UserOperationType::START_SCREEN_SHARING);

        const std::string& friendNickname = m_stateManager.getActiveCall().getNickname();
        auto [uid, packet] = PacketFactory::getStartScreenSharingPacket(m_stateManager.getMyNickname(), friendNickname);

        createAndStartTask(uid, packet, PacketType::SCREEN_SHARING_BEGIN,
            [this](std::optional<nlohmann::json> completionContext) {
                m_operationManager.removeOperation(UserOperationType::START_SCREEN_SHARING);
                m_stateManager.setScreenSharing(true);
                m_eventListener->onStartScreenSharingResult({});
            },
            [this](std::optional<nlohmann::json> failureContext) {
                m_operationManager.removeOperation(UserOperationType::START_SCREEN_SHARING);
                LOG_ERROR("Start screen sharing task failed");
                m_eventListener->onStartScreenSharingResult(ErrorCode::network_error);
            }
        );

        return {};
    }

    std::error_code Client::stopScreenSharing() {
        if (m_stateManager.isConnectionDown()) return make_error_code(ErrorCode::connection_down);
        if (!m_stateManager.isAuthorized()) return make_error_code(ErrorCode::not_authorized);
        if (!m_stateManager.isActiveCall()) return make_error_code(ErrorCode::no_active_call);
        if (!m_stateManager.isScreenSharing()) return make_error_code(ErrorCode::screen_sharing_not_active);
        if (m_operationManager.isOperationType(UserOperationType::STOP_SCREEN_SHARING)) return make_error_code(ErrorCode::operation_in_progress);

        m_operationManager.addOperation(UserOperationType::STOP_SCREEN_SHARING);

        const std::string& friendNickname = m_stateManager.getActiveCall().getNickname();
        auto [uid, packet] = PacketFactory::getStopScreenSharingPacket(m_stateManager.getMyNickname(), friendNickname);

        createAndStartTask(uid, packet, PacketType::SCREEN_SHARING_END,
            [this](std::optional<nlohmann::json> completionContext) {
                m_operationManager.removeOperation(UserOperationType::STOP_SCREEN_SHARING);
                m_stateManager.setScreenSharing(false);
                m_eventListener->onStopScreenSharingResult({});
            },
            [this](std::optional<nlohmann::json> failureContext) {
                m_operationManager.removeOperation(UserOperationType::STOP_SCREEN_SHARING);
                LOG_ERROR("Stop screen sharing task failed");
                m_eventListener->onStopScreenSharingResult(ErrorCode::network_error);
            }
        );

        return {};
    }

    std::error_code Client::sendScreen(const std::vector<unsigned char>& data) {
        if (m_stateManager.isConnectionDown()) return make_error_code(ErrorCode::connection_down);
        if (!m_stateManager.isAuthorized()) return make_error_code(ErrorCode::not_authorized);
        if (!m_stateManager.isActiveCall()) return make_error_code(ErrorCode::no_active_call);
        if (!m_stateManager.isScreenSharing()) return make_error_code(ErrorCode::screen_sharing_not_active);

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
            return {};
        }
        catch (const std::exception& e) {
            LOG_ERROR("Screen sending error");
            return make_error_code(ErrorCode::encryption_error);
        }
    }

    std::error_code Client::startCameraSharing() {
        if (m_stateManager.isConnectionDown()) return make_error_code(ErrorCode::connection_down);
        if (!m_stateManager.isAuthorized()) return make_error_code(ErrorCode::not_authorized);
        if (!m_stateManager.isActiveCall()) return make_error_code(ErrorCode::no_active_call);
        if (m_stateManager.isCameraSharing()) return make_error_code(ErrorCode::camera_sharing_already_active);
        if (m_operationManager.isOperationType(UserOperationType::START_CAMERA_SHARING)) return make_error_code(ErrorCode::operation_in_progress);

        m_operationManager.addOperation(UserOperationType::START_CAMERA_SHARING);

        const std::string& friendNickname = m_stateManager.getActiveCall().getNickname();
        auto [uid, packet] = PacketFactory::getStartCameraSharingPacket(m_stateManager.getMyNickname(), friendNickname);

        createAndStartTask(uid, packet, PacketType::CAMERA_SHARING_BEGIN,
            [this](std::optional<nlohmann::json> completionContext) {
                m_operationManager.removeOperation(UserOperationType::START_CAMERA_SHARING);
                m_eventListener->onStartCameraSharingResult({});
            },
            [this](std::optional<nlohmann::json> failureContext) {
                m_operationManager.removeOperation(UserOperationType::START_CAMERA_SHARING);
                LOG_ERROR("Start camera sharing task failed");
                m_eventListener->onStartCameraSharingResult(ErrorCode::network_error);
            }
        );

        return {};
    }

    std::error_code Client::stopCameraSharing() {
        if (m_stateManager.isConnectionDown()) return make_error_code(ErrorCode::connection_down);
        if (!m_stateManager.isAuthorized()) return make_error_code(ErrorCode::not_authorized);
        if (!m_stateManager.isActiveCall()) return make_error_code(ErrorCode::no_active_call);
        if (!m_stateManager.isCameraSharing()) return make_error_code(ErrorCode::camera_sharing_not_active);
        if (m_operationManager.isOperationType(UserOperationType::STOP_CAMERA_SHARING)) return make_error_code(ErrorCode::operation_in_progress);

        m_operationManager.addOperation(UserOperationType::STOP_CAMERA_SHARING);

        const std::string& friendNickname = m_stateManager.getActiveCall().getNickname();
        std::string friendNicknameHash = crypto::calculateHash(friendNickname);
        auto [uid, packet] = PacketFactory::getStopCameraSharingPacket(m_stateManager.getMyNickname(), friendNicknameHash);

        createAndStartTask(uid, packet, PacketType::CAMERA_SHARING_END,
            [this](std::optional<nlohmann::json> completionContext) {
                m_operationManager.removeOperation(UserOperationType::STOP_CAMERA_SHARING);
                m_eventListener->onStopCameraSharingResult({});
            },
            [this](std::optional<nlohmann::json> failureContext) {
                m_operationManager.removeOperation(UserOperationType::STOP_CAMERA_SHARING);
                LOG_ERROR("Stop camera sharing task failed");
                m_eventListener->onStopCameraSharingResult(ErrorCode::network_error);
            }
        );

        return {};
    }

    std::error_code Client::sendCamera(const std::vector<unsigned char>& data) {
        if (m_stateManager.isConnectionDown()) return make_error_code(ErrorCode::connection_down);
        if (!m_stateManager.isAuthorized()) return make_error_code(ErrorCode::not_authorized);
        if (!m_stateManager.isActiveCall()) return make_error_code(ErrorCode::no_active_call);
        if (!m_stateManager.isCameraSharing()) return make_error_code(ErrorCode::camera_sharing_not_active);

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
            return {};
        }
        catch (const std::exception& e) {
            LOG_ERROR("Camera sending error");
            return make_error_code(ErrorCode::encryption_error);
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