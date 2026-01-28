#include "AuthorizationService.h"
#include "jsonType.h"
#include "utilities/logger.h"
#include <chrono>
#include <thread>

using namespace core::utilities;
using namespace std::chrono_literals;
using namespace std::placeholders;

namespace core
{
    namespace services
    {
        AuthorizationService::AuthorizationService(
            ClientStateManager& stateManager,
            KeyManager& keyManager,
            UserOperationManager& operationManager,
            PendingRequests& pendingRequests,
            core::network::udp::MediaController& mediaController,
            std::unique_ptr<core::network::tcp::ControlController>& controlController,
            std::shared_ptr<EventListener> eventListener,
            std::string host,
            std::string controlPort)
            : m_stateManager(stateManager)
            , m_keyManager(keyManager)
            , m_operationManager(operationManager)
            , m_pendingRequests(pendingRequests)
            , m_mediaController(mediaController)
            , m_controlController(controlController)
            , m_eventListener(eventListener)
            , m_host(std::move(host))
            , m_controlPort(std::move(controlPort))
        {
        }

        AuthorizationService::~AuthorizationService() {
            stopReconnectRetry();
        }

        bool AuthorizationService::sendControl(uint32_t type, const std::vector<unsigned char>& body,
            std::function<void(std::optional<nlohmann::json>)> onComplete,
            std::function<void(std::optional<nlohmann::json>)> onFail,
            const std::string& uid)
        {
            if (!m_controlController || !m_controlController->isConnected())
                return false;
            m_pendingRequests.add(uid, std::move(onComplete), std::move(onFail));
            if (!m_controlController->send(type, body.data(), body.size())) {
                m_pendingRequests.fail(uid, std::nullopt);
                return false;
            }
            return true;
        }

        bool AuthorizationService::sendControlFireAndForget(uint32_t type, const std::vector<unsigned char>& body,
            std::function<void(std::optional<nlohmann::json>)> onComplete,
            std::function<void(std::optional<nlohmann::json>)> onFail,
            const std::string& uid)
        {
            if (!sendControl(type, body, std::move(onComplete), std::move(onFail), uid))
                return false;
            m_pendingRequests.complete(uid, std::nullopt);
            return true;
        }

        std::error_code AuthorizationService::authorize(const std::string& nickname) {
            if (m_stateManager.isConnectionDown()) return make_error_code(ErrorCode::connection_down);
            if (m_stateManager.isAuthorized()) return make_error_code(ErrorCode::already_authorized);
            if (m_operationManager.isOperation(UserOperationType::AUTHORIZE, nickname)) return make_error_code(ErrorCode::operation_in_progress);

            if (!m_keyManager.isKeys()) {
                if (m_keyManager.isGeneratingKeys()) { }
                else { m_keyManager.generateKeys(); }
            }
            m_keyManager.awaitKeysGeneration();

            m_operationManager.addOperation(UserOperationType::AUTHORIZE, nickname);
            uint16_t mediaPort = m_mediaController.getLocalMediaPort();
            auto [uid, packet] = PacketFactory::getAuthorizationPacket(nickname, m_keyManager.getMyPublicKey(), mediaPort);

            if (!sendControl(static_cast<uint32_t>(PacketType::AUTHORIZATION), packet,
                    std::bind(&AuthorizationService::onAuthorizeCompleted, this, nickname, _1),
                    std::bind(&AuthorizationService::onAuthorizeFailed, this, nickname, _1),
                    uid)) {
                m_operationManager.removeOperation(UserOperationType::AUTHORIZE, nickname);
                return make_error_code(ErrorCode::connection_down);
            }
            return {};
        }

        std::error_code AuthorizationService::logout() {
            if (m_stateManager.isConnectionDown()) return make_error_code(ErrorCode::connection_down);
            if (!m_stateManager.isAuthorized()) return make_error_code(ErrorCode::not_authorized);
            if (m_operationManager.isOperation(UserOperationType::LOGOUT, m_stateManager.getMyNickname())) return make_error_code(ErrorCode::operation_in_progress);

            m_operationManager.addOperation(UserOperationType::LOGOUT, m_stateManager.getMyNickname());
            auto [uid, packet] = PacketFactory::getLogoutPacket(m_stateManager.getMyNickname());

            if (!sendControlFireAndForget(static_cast<uint32_t>(PacketType::LOGOUT), packet,
                    std::bind(&AuthorizationService::onLogoutCompleted, this, _1),
                    std::bind(&AuthorizationService::onLogoutFailed, this, _1),
                    uid)) {
                m_operationManager.removeOperation(UserOperationType::LOGOUT, m_stateManager.getMyNickname());
                m_eventListener->onLogoutCompleted();
                return make_error_code(ErrorCode::connection_down);
            }
            return {};
        }

        void AuthorizationService::handleReconnect() {
            if (!m_stateManager.isConnectionDown()) return;
            if (m_reconnectInProgress.exchange(true)) return;

            if (m_stateManager.isAuthorized()) {
                m_controlController->disconnect();
                m_controlController->connect(m_host, m_controlPort);
                for (int i = 0; i < 30; ++i) {
                    std::this_thread::sleep_for(200ms);
                    if (m_controlController->isConnected())
                        break;
                }
                if (!m_controlController->isConnected()) {
                    m_reconnectInProgress = false;
                    return;
                }
                uint16_t mediaPort = m_mediaController.getLocalMediaPort();
                auto [uid, packet] = PacketFactory::getReconnectPacket(m_stateManager.getMyNickname(), m_stateManager.getMyToken(), mediaPort);
                sendControl(static_cast<uint32_t>(PacketType::RECONNECT), packet,
                    std::bind(&AuthorizationService::onReconnectCompleted, this, _1),
                    std::bind(&AuthorizationService::onReconnectFailed, this, _1),
                    uid);
            }
            else {
                m_stateManager.setConnectionDown(false);
                m_reconnectInProgress = false;
                m_stateManager.setAuthorized(false);
                m_stateManager.clearMyNickname();
                m_stateManager.clearMyToken();
                m_eventListener->onConnectionRestoredAuthorizationNeeded();
            }
        }

        void AuthorizationService::startReconnectRetry() {
            if (!m_stateManager.isConnectionDown()) {
                LOG_DEBUG("Reconnect retry skipped: connectionDown={}", m_stateManager.isConnectionDown());
                return;
            }
            if (m_retryThreadRunning.load()) {
                LOG_DEBUG("Reconnect retry already running, skipping");
                return;
            }
            if (m_reconnectRetryThread.joinable()) {
                m_reconnectRetryThread.join();
            }

            m_stopReconnectRetry = false;
            LOG_INFO("Reconnect retry started (authorized={})", m_stateManager.isAuthorized());
            m_reconnectRetryThread = std::thread([this]() {
                m_retryThreadRunning = true;
                while (m_stateManager.isConnectionDown() && !m_stopReconnectRetry.load()) {
                    if (!m_reconnectInProgress.exchange(true)) {
                        m_controlController->disconnect();
                        m_controlController->connect(m_host, m_controlPort);
                        for (int i = 0; i < 30; ++i) {
                            std::this_thread::sleep_for(200ms);
                            if (m_controlController->isConnected()) break;
                        }
                        if (m_controlController->isConnected()) {
                            if (!m_stateManager.isAuthorized()) {
                                LOG_INFO("Reconnect: connected, not authorized -> need to authorize");
                                m_stateManager.setConnectionDown(false);
                                m_mediaController.notifyConnectionRestored();
                                m_reconnectInProgress = false;
                                m_stateManager.setAuthorized(false);
                                m_stateManager.clearMyNickname();
                                m_stateManager.clearMyToken();
                                m_eventListener->onConnectionRestoredAuthorizationNeeded();
                                break;
                            }
                            uint16_t mediaPort = m_mediaController.getLocalMediaPort();
                            auto [uid, packet] = PacketFactory::getReconnectPacket(m_stateManager.getMyNickname(), m_stateManager.getMyToken(), mediaPort);
                            LOG_INFO("Reconnect: sending RECONNECT, waiting for result");
                            sendControl(static_cast<uint32_t>(PacketType::RECONNECT), packet,
                                std::bind(&AuthorizationService::onReconnectCompleted, this, _1),
                                std::bind(&AuthorizationService::onReconnectFailed, this, _1),
                                uid);
                        }
                        else {
                            LOG_WARN("Reconnect: connect failed, will retry");
                            m_reconnectInProgress = false;
                        }
                    }
                    for (int i = 0; i < 4 && !m_stopReconnectRetry.load(); ++i)
                        std::this_thread::sleep_for(1s);
                    m_reconnectInProgress = false;
                }
                m_retryThreadRunning = false;
            });
        }

        void AuthorizationService::stopReconnectRetry() {
            m_stopReconnectRetry = true;
            if (m_reconnectRetryThread.joinable())
                m_reconnectRetryThread.join();
        }

        void AuthorizationService::onConnectionDown() {
            m_stateManager.setConnectionDown(true);
            m_operationManager.clearAllOperations();

            startReconnectRetry();
        }

        void AuthorizationService::onConnectionRestored() {
            handleReconnect();
        }

        void AuthorizationService::onAuthorizeCompleted(const std::string& nickname, std::optional<nlohmann::json> completionContext) {
            m_operationManager.removeOperation(UserOperationType::AUTHORIZE, nickname);
            if (completionContext.has_value()) {
                auto& context = completionContext.value();
                bool ok = context[RESULT].get<bool>();
                if (ok) {
                    m_stateManager.setMyNickname(nickname);
                    m_stateManager.setMyToken(context[TOKEN].get<std::string>());
                    m_stateManager.setAuthorized(true);
                    m_eventListener->onAuthorizationResult({});
                }
                else
                    m_eventListener->onAuthorizationResult(ErrorCode::taken_nickname);
            }
        }

        void AuthorizationService::onAuthorizeFailed(const std::string& nickname, std::optional<nlohmann::json>) {
            m_operationManager.removeOperation(UserOperationType::AUTHORIZE, nickname);
            if (!m_stateManager.isConnectionDown()) {
                LOG_ERROR("Authorization failed");
                m_eventListener->onAuthorizationResult(ErrorCode::network_error);
            }
        }

        void AuthorizationService::onLogoutCompleted(std::optional<nlohmann::json>) {
            m_operationManager.removeOperation(UserOperationType::LOGOUT, m_stateManager.getMyNickname());
            m_eventListener->onLogoutCompleted();
        }

        void AuthorizationService::onLogoutFailed(std::optional<nlohmann::json>) {
            m_operationManager.removeOperation(UserOperationType::LOGOUT, m_stateManager.getMyNickname());
            m_eventListener->onLogoutCompleted();
        }

        void AuthorizationService::onReconnectCompleted(std::optional<nlohmann::json> completionContext) {
            m_reconnectInProgress = false;
            if (!completionContext.has_value()) {
                LOG_ERROR("onReconnectCompleted empty");
                return;
            }
            auto& context = completionContext.value();
            m_stateManager.setConnectionDown(false);
            m_mediaController.notifyConnectionRestored();

            if (!context.contains(RESULT)) {
                m_stateManager.setAuthorized(false);
                m_stateManager.clearMyNickname();
                m_stateManager.clearMyToken();
                m_eventListener->onConnectionRestoredAuthorizationNeeded();
                return;
            }
            bool ok = context[RESULT].get<bool>();
            if (ok) {
                LOG_INFO("Reconnect OK");
                m_stateManager.setLastReconnectSuccessTime();
                bool activeCall = context.contains(IS_ACTIVE_CALL) && context[IS_ACTIVE_CALL].get<bool>();
                m_eventListener->onConnectionRestored();
                if (!activeCall) {
                    bool had = m_stateManager.isActiveCall();
                    m_stateManager.setScreenSharing(false);
                    m_stateManager.setCameraSharing(false);
                    m_stateManager.setViewingRemoteScreen(false);
                    m_stateManager.setViewingRemoteCamera(false);
                    m_stateManager.clearCallState();
                    if (had)
                        m_eventListener->onCallEndedByRemote({});
                }
            }
            else {
                m_stateManager.setAuthorized(false);
                m_stateManager.clearMyNickname();
                m_stateManager.clearMyToken();
                m_eventListener->onConnectionRestoredAuthorizationNeeded();
            }
        }

        void AuthorizationService::onReconnectFailed(std::optional<nlohmann::json>) {
            m_reconnectInProgress = false;
            LOG_ERROR("Reconnect failed");
        }
    }
}
