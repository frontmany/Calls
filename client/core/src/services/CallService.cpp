#include "CallService.h"
#include "jsonType.h"
#include "utilities/crypto.h"
#include "utilities/logger.h"
#include <chrono>

using namespace core::utilities;
using namespace std::chrono_literals;
using namespace std::placeholders;

namespace core
{
    namespace services
    {
        CallService::CallService(
            ClientStateManager& stateManager,
            KeyManager& keyManager,
            UserOperationManager& operationManager,
            PendingRequests& pendingRequests,
            core::network::NetworkController& networkController,
            std::unique_ptr<core::network::TcpControlClient>& tcpControl,
            core::audio::AudioEngine& audioEngine,
            std::shared_ptr<EventListener> eventListener)
            : m_stateManager(stateManager)
            , m_keyManager(keyManager)
            , m_operationManager(operationManager)
            , m_pendingRequests(pendingRequests)
            , m_networkController(networkController)
            , m_tcpControl(tcpControl)
            , m_audioEngine(audioEngine)
            , m_eventListener(eventListener)
        {
        }

        bool CallService::sendControl(uint32_t type, const std::vector<unsigned char>& body,
            std::function<void(std::optional<nlohmann::json>)> onComplete,
            std::function<void(std::optional<nlohmann::json>)> onFail,
            const std::string& uid)
        {
            if (!m_tcpControl || !m_tcpControl->isConnected()) return false;
            m_pendingRequests.add(uid, std::move(onComplete), std::move(onFail));
            if (!m_tcpControl->send(type, body.data(), body.size())) {
                m_pendingRequests.fail(uid, std::nullopt);
                return false;
            }
            return true;
        }

        std::error_code CallService::startOutgoingCall(const std::string& userNickname) {
            if (m_stateManager.isConnectionDown()) return make_error_code(ErrorCode::connection_down);
            if (!m_stateManager.isAuthorized()) return make_error_code(ErrorCode::not_authorized);
            if (m_stateManager.isActiveCall()) return make_error_code(ErrorCode::active_call_exists);
            if (m_operationManager.isOperation(UserOperationType::START_OUTGOING_CALL, userNickname)) return make_error_code(ErrorCode::operation_in_progress);

            m_operationManager.addOperation(UserOperationType::START_OUTGOING_CALL, userNickname);

            auto [uid, packet] = PacketFactory::getRequestUserInfoPacket(m_stateManager.getMyNickname(), userNickname);

            if (!sendControl(static_cast<uint32_t>(PacketType::GET_USER_INFO), packet,
                    std::bind(&CallService::onRequestUserInfoCompleted, this, userNickname, _1),
                    std::bind(&CallService::onRequestUserInfoFailed, this, userNickname, _1), uid)) {
                m_operationManager.removeOperation(UserOperationType::START_OUTGOING_CALL, userNickname);
                return make_error_code(ErrorCode::connection_down);
            }
            return {};
        }

        std::error_code CallService::stopOutgoingCall() {
            if (m_stateManager.isConnectionDown()) return make_error_code(ErrorCode::connection_down);
            if (!m_stateManager.isAuthorized()) return make_error_code(ErrorCode::not_authorized);
            if (!m_stateManager.isOutgoingCall()) return make_error_code(ErrorCode::no_outgoing_call);
            const std::string& nickname = m_stateManager.getOutgoingCall().getNickname();
            if (m_operationManager.isOperation(UserOperationType::STOP_OUTGOING_CALL, nickname)) return make_error_code(ErrorCode::operation_in_progress);

            m_operationManager.addOperation(UserOperationType::STOP_OUTGOING_CALL, nickname);

            auto [uid, packet] = PacketFactory::getStopOutgoingCallPacket(m_stateManager.getMyNickname(), nickname);

            if (!sendControl(static_cast<uint32_t>(PacketType::CALLING_END), packet,
                    std::bind(&CallService::onStopOutgoingCallCompleted, this, nickname, _1),
                    std::bind(&CallService::onStopOutgoingCallFailed, this, nickname, _1), uid)) {
                m_operationManager.removeOperation(UserOperationType::STOP_OUTGOING_CALL, nickname);
                return make_error_code(ErrorCode::connection_down);
            }
            return {};
        }

        std::error_code CallService::acceptCall(const std::string& userNickname) {
            if (m_stateManager.isConnectionDown()) return make_error_code(ErrorCode::connection_down);
            if (!m_stateManager.isAuthorized()) return make_error_code(ErrorCode::not_authorized);
            if (!m_stateManager.isIncomingCalls()) return make_error_code(ErrorCode::no_incoming_call);
            if (m_operationManager.isOperation(UserOperationType::ACCEPT_CALL, userNickname)) return make_error_code(ErrorCode::operation_in_progress);

            auto& incomingCalls = m_stateManager.getIncomingCalls();
            if (!incomingCalls.contains(userNickname)) return make_error_code(ErrorCode::no_incoming_call);

            m_operationManager.addOperation(UserOperationType::ACCEPT_CALL, userNickname);

            for (auto& [nickname, incomingCallData] : incomingCalls) {
                if (nickname != userNickname) {
                    auto [uid, packet] = PacketFactory::getDeclineCallPacket(m_stateManager.getMyNickname(), nickname);
                    sendControl(static_cast<uint32_t>(PacketType::CALL_DECLINE), packet,
                        std::bind(&CallService::onDeclineIncomingCallCompleted, this, nickname, _1),
                        std::bind(&CallService::onDeclineIncomingCallFailed, this, _1), uid);
                }
            }

            if (m_stateManager.isOutgoingCall()) {
                auto [uid, packet] = PacketFactory::getStopOutgoingCallPacket(m_stateManager.getMyNickname(), m_stateManager.getOutgoingCall().getNickname());
                if (!sendControl(static_cast<uint32_t>(PacketType::CALLING_END), packet,
                        std::bind(&CallService::onAcceptCallStopOutgoingCompleted, this, userNickname, _1),
                        std::bind(&CallService::onAcceptCallStopOutgoingFailed, this, userNickname, _1), uid)) {
                    m_operationManager.removeOperation(UserOperationType::ACCEPT_CALL, userNickname);
                    return make_error_code(ErrorCode::connection_down);
                }
            }
            else if (m_stateManager.isActiveCall()) {
                auto [uid, packet] = PacketFactory::getEndCallPacket(m_stateManager.getMyNickname(), m_stateManager.getActiveCall().getNickname());
                if (!sendControl(static_cast<uint32_t>(PacketType::CALL_END), packet,
                        std::bind(&CallService::onAcceptCallEndActiveCompleted, this, userNickname, _1),
                        std::bind(&CallService::onAcceptCallEndActiveFailed, this, userNickname, _1), uid)) {
                    m_operationManager.removeOperation(UserOperationType::ACCEPT_CALL, userNickname);
                    return make_error_code(ErrorCode::connection_down);
                }
            }
            else {
                auto& inc = m_stateManager.getIncomingCalls();
                auto& incomingCall = inc.at(userNickname);
                auto [uid, packet] = PacketFactory::getAcceptCallPacket(m_stateManager.getMyNickname(), userNickname, m_keyManager.getMyPublicKey(), incomingCall.getPublicKey(), incomingCall.getCallKey());
                if (!sendControl(static_cast<uint32_t>(PacketType::CALL_ACCEPT), packet,
                        std::bind(&CallService::onAcceptCallCompleted, this, userNickname, _1),
                        std::bind(&CallService::onAcceptCallFailed, this, userNickname, _1), uid)) {
                    m_operationManager.removeOperation(UserOperationType::ACCEPT_CALL, userNickname);
                    return make_error_code(ErrorCode::connection_down);
                }
            }
            return {};
        }

        std::error_code CallService::declineCall(const std::string& userNickname) {
            if (m_stateManager.isConnectionDown()) return make_error_code(ErrorCode::connection_down);
            if (!m_stateManager.isAuthorized()) return make_error_code(ErrorCode::not_authorized);
            if (!m_stateManager.isIncomingCalls()) return make_error_code(ErrorCode::no_incoming_call);
            if (m_operationManager.isOperation(UserOperationType::DECLINE_CALL, userNickname)) return make_error_code(ErrorCode::operation_in_progress);

            auto& incomingCalls = m_stateManager.getIncomingCalls();
            if (!incomingCalls.contains(userNickname)) return make_error_code(ErrorCode::no_incoming_call);

            m_operationManager.addOperation(UserOperationType::DECLINE_CALL, userNickname);

            auto [uid, packet] = PacketFactory::getDeclineCallPacket(m_stateManager.getMyNickname(), userNickname);

            if (!sendControl(static_cast<uint32_t>(PacketType::CALL_DECLINE), packet,
                    std::bind(&CallService::onDeclineCallCompleted, this, userNickname, _1),
                    std::bind(&CallService::onDeclineCallFailed, this, userNickname, _1), uid)) {
                m_operationManager.removeOperation(UserOperationType::DECLINE_CALL, userNickname);
                return make_error_code(ErrorCode::connection_down);
            }
            return {};
        }

        std::error_code CallService::endCall() {
            if (m_stateManager.isConnectionDown()) return make_error_code(ErrorCode::connection_down);
            if (!m_stateManager.isAuthorized()) return make_error_code(ErrorCode::not_authorized);
            if (!m_stateManager.isActiveCall()) return make_error_code(ErrorCode::no_active_call);
            const std::string& nickname = m_stateManager.getActiveCall().getNickname();
            if (m_operationManager.isOperation(UserOperationType::END_CALL, nickname)) return make_error_code(ErrorCode::operation_in_progress);

            m_operationManager.addOperation(UserOperationType::END_CALL, nickname);

            auto [uid, packet] = PacketFactory::getEndCallPacket(m_stateManager.getMyNickname(), nickname);

            if (!sendControl(static_cast<uint32_t>(PacketType::CALL_END), packet,
                    std::bind(&CallService::onEndCallCompleted, this, nickname, _1),
                    std::bind(&CallService::onEndCallFailed, this, nickname, _1), uid)) {
                m_operationManager.removeOperation(UserOperationType::END_CALL, nickname);
                return make_error_code(ErrorCode::connection_down);
            }
            return {};
        }

        void CallService::onRequestUserInfoCompleted(const std::string& userNickname, std::optional<nlohmann::json> completionContext) {
            if (completionContext.has_value()) {
                auto& context = completionContext.value();

                bool userInfoFound = context[RESULT].get<bool>();

                if (userInfoFound) {
                    const std::string& userPublicKeyString = context[PUBLIC_KEY];
                    auto userPublicKey = crypto::deserializePublicKey(userPublicKeyString);

                    CryptoPP::SecByteBlock callKey;
                    crypto::generateAESKey(callKey);

                    auto [uid, packet] = PacketFactory::getStartOutgoingCallPacket(m_stateManager.getMyNickname(), userNickname, m_keyManager.getMyPublicKey(), userPublicKey, callKey);

                    sendControl(static_cast<uint32_t>(PacketType::CALLING_BEGIN), packet,
                        std::bind(&CallService::onStartOutgoingCallCompleted, this, userNickname, _1),
                        std::bind(&CallService::onStartOutgoingCallFailed, this, userNickname, _1), uid);
                }
                else {
                    m_operationManager.removeOperation(UserOperationType::START_OUTGOING_CALL, userNickname);
                    m_eventListener->onStartOutgoingCallResult(ErrorCode::unexisting_user);
                }
            }
        }

        void CallService::onRequestUserInfoFailed(const std::string& userNickname, std::optional<nlohmann::json> failureContext) {
            m_operationManager.removeOperation(UserOperationType::START_OUTGOING_CALL, userNickname);
            LOG_ERROR("Request user info task failed");
            m_eventListener->onStartOutgoingCallResult(ErrorCode::network_error);
        }

        void CallService::onStartOutgoingCallCompleted(const std::string& userNickname, std::optional<nlohmann::json> completionContext) {
            m_operationManager.removeOperation(UserOperationType::START_OUTGOING_CALL, userNickname);

            m_stateManager.setOutgoingCall(userNickname, 32s, [this]() {
                onOutgoingCallTimeout();
            });

            m_eventListener->onStartOutgoingCallResult({});
        }

        void CallService::onStartOutgoingCallFailed(const std::string& userNickname, std::optional<nlohmann::json> failureContext) {
            m_operationManager.removeOperation(UserOperationType::START_OUTGOING_CALL, userNickname);
            LOG_ERROR("Start outgoing call task failed");
            m_eventListener->onStartOutgoingCallResult(ErrorCode::network_error);
        }

        void CallService::onOutgoingCallTimeout() {
            m_stateManager.clearCallState();
            m_eventListener->onOutgoingCallTimeout({});
        }

        void CallService::onStopOutgoingCallCompleted(const std::string& nickname, std::optional<nlohmann::json> completionContext) {
            m_operationManager.removeOperation(UserOperationType::STOP_OUTGOING_CALL, nickname);
            m_eventListener->onStopOutgoingCallResult({});
        }

        void CallService::onStopOutgoingCallFailed(const std::string& nickname, std::optional<nlohmann::json> failureContext) {
            m_operationManager.removeOperation(UserOperationType::STOP_OUTGOING_CALL, nickname);
            LOG_ERROR("Stop outgoing call task failed");
            m_eventListener->onStopOutgoingCallResult(ErrorCode::network_error);
        }

        void CallService::onDeclineIncomingCallCompleted(const std::string& nickname, std::optional<nlohmann::json> completionContext) {
            m_stateManager.removeIncomingCall(nickname);
        }

        void CallService::onDeclineIncomingCallFailed(std::optional<nlohmann::json> failureContext) {
            LOG_ERROR("Decline incoming call task failed (as part of accept call operation)");
        }

        void CallService::onAcceptCallStopOutgoingCompleted(const std::string& userNickname, std::optional<nlohmann::json> completionContext) {
            auto& incomingCalls = m_stateManager.getIncomingCalls();
            auto& incomingCall = incomingCalls.at(userNickname);
            auto [uid, packet] = PacketFactory::getAcceptCallPacket(m_stateManager.getMyNickname(), userNickname, m_keyManager.getMyPublicKey(), incomingCall.getPublicKey(), incomingCall.getCallKey());

            sendControl(static_cast<uint32_t>(PacketType::CALL_ACCEPT), packet,
                std::bind(&CallService::onAcceptCallCompleted, this, userNickname, _1),
                std::bind(&CallService::onAcceptCallFailed, this, userNickname, _1), uid);
        }

        void CallService::onAcceptCallStopOutgoingFailed(const std::string& userNickname, std::optional<nlohmann::json> failureContext) {
            m_operationManager.removeOperation(UserOperationType::ACCEPT_CALL, userNickname);
            LOG_ERROR("Stop outgoing call task failed (as part of accept call operation)");
            m_eventListener->onAcceptCallResult(ErrorCode::network_error, userNickname);
        }

        void CallService::onAcceptCallEndActiveCompleted(const std::string& userNickname, std::optional<nlohmann::json> completionContext) {
            m_stateManager.setScreenSharing(false);
            m_stateManager.setCameraSharing(false);
            m_stateManager.setViewingRemoteScreen(false);
            m_stateManager.setViewingRemoteCamera(false);
            m_stateManager.clearCallState();
            m_audioEngine.stopStream();

            auto& incomingCalls = m_stateManager.getIncomingCalls();
            auto& incomingCall = incomingCalls.at(userNickname);
            auto [uid, packet] = PacketFactory::getAcceptCallPacket(m_stateManager.getMyNickname(), userNickname, m_keyManager.getMyPublicKey(), incomingCall.getPublicKey(), incomingCall.getCallKey());

            sendControl(static_cast<uint32_t>(PacketType::CALL_ACCEPT), packet,
                std::bind(&CallService::onAcceptCallCompleted, this, userNickname, _1),
                std::bind(&CallService::onAcceptCallFailedAfterEndCall, this, userNickname, _1), uid);
        }

        void CallService::onAcceptCallEndActiveFailed(const std::string& userNickname, std::optional<nlohmann::json> failureContext) {
            m_operationManager.removeOperation(UserOperationType::ACCEPT_CALL, userNickname);
            LOG_ERROR("End active call task failed (as part of accept new call operation)");
            m_eventListener->onAcceptCallResult(ErrorCode::network_error, userNickname);
        }

        void CallService::onAcceptCallCompleted(const std::string& userNickname, std::optional<nlohmann::json> completionContext) {
            m_operationManager.removeOperation(UserOperationType::ACCEPT_CALL, userNickname);

            auto& incomingCalls = m_stateManager.getIncomingCalls();
            auto& incomingCall = incomingCalls.at(userNickname);
            m_stateManager.setActiveCall(incomingCall.getNickname(), incomingCall.getPublicKey(), incomingCall.getCallKey());
            m_stateManager.removeIncomingCall(userNickname);

            m_audioEngine.startStream();
            m_eventListener->onAcceptCallResult({}, userNickname);
        }

        void CallService::onAcceptCallFailed(const std::string& userNickname, std::optional<nlohmann::json> failureContext) {
            m_operationManager.removeOperation(UserOperationType::ACCEPT_CALL, userNickname);
            LOG_ERROR("Accept call task failed");
            m_eventListener->onAcceptCallResult(ErrorCode::network_error, userNickname);
        }

        void CallService::onAcceptCallFailedAfterEndCall(const std::string& userNickname, std::optional<nlohmann::json> failureContext) {
            m_operationManager.removeOperation(UserOperationType::ACCEPT_CALL, userNickname);
            LOG_ERROR("Accept call task failed");
            m_eventListener->onEndCallResult({});
            m_eventListener->onAcceptCallResult(ErrorCode::network_error, userNickname);
        }

        void CallService::onDeclineCallCompleted(const std::string& userNickname, std::optional<nlohmann::json> completionContext) {
            m_operationManager.removeOperation(UserOperationType::DECLINE_CALL, userNickname);

            m_stateManager.removeIncomingCall(userNickname);

            m_eventListener->onDeclineCallResult({}, userNickname);
        }

        void CallService::onDeclineCallFailed(const std::string& userNickname, std::optional<nlohmann::json> failureContext) {
            m_operationManager.removeOperation(UserOperationType::DECLINE_CALL, userNickname);
            LOG_ERROR("Decline incoming call task failed");
            m_eventListener->onDeclineCallResult(make_error_code(ErrorCode::network_error), userNickname);
        }

        void CallService::onEndCallCompleted(const std::string& nickname, std::optional<nlohmann::json> completionContext) {
            m_operationManager.removeOperation(UserOperationType::END_CALL, nickname);

            m_stateManager.setScreenSharing(false);
            m_stateManager.setCameraSharing(false);
            m_stateManager.setViewingRemoteScreen(false);
            m_stateManager.setViewingRemoteCamera(false);
            m_stateManager.clearCallState();
            m_audioEngine.stopStream();

            m_eventListener->onEndCallResult({});
        }

        void CallService::onEndCallFailed(const std::string& nickname, std::optional<nlohmann::json> failureContext) {
            m_operationManager.removeOperation(UserOperationType::END_CALL, nickname);
            LOG_ERROR("End call task failed");
            m_eventListener->onEndCallResult(ErrorCode::network_error);
        }
    }
}
