#include "AuthorizationService.h"
#include "jsonType.h"
#include "utilities/logger.h"
#include <chrono>

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
            TaskManager<long long, std::milli>& taskManager,
            core::network::NetworkController& networkController,
            std::shared_ptr<EventListener> eventListener)
            : m_stateManager(stateManager)
            , m_keyManager(keyManager)
            , m_operationManager(operationManager)
            , m_taskManager(taskManager)
            , m_networkController(networkController)
            , m_eventListener(eventListener)
        {
        }

        AuthorizationService::~AuthorizationService() {
            stopReconnectRetry();
        }

        void AuthorizationService::createAndStartTask(
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

        std::error_code AuthorizationService::authorize(const std::string& nickname) {
            if (m_stateManager.isConnectionDown()) return make_error_code(ErrorCode::connection_down);
            if (m_stateManager.isAuthorized()) return make_error_code(ErrorCode::already_authorized);
            if (m_operationManager.isOperation(UserOperationType::AUTHORIZE, nickname)) return make_error_code(ErrorCode::operation_in_progress);

            if (!m_keyManager.isKeys()) {
                if (m_keyManager.isGeneratingKeys()) {
                    // Ключи уже генерируются, ждем
                }
                else {
                    m_keyManager.generateKeys();
                }
            }

            m_keyManager.awaitKeysGeneration();

            m_operationManager.addOperation(UserOperationType::AUTHORIZE, nickname);

            auto [uid, packet] = PacketFactory::getAuthorizationPacket(nickname, m_keyManager.getMyPublicKey());

            createAndStartTask(uid, packet, PacketType::AUTHORIZATION,
                std::bind(&AuthorizationService::onAuthorizeCompleted, this, nickname, _1),
                std::bind(&AuthorizationService::onAuthorizeFailed, this, nickname, _1)
            );

            return {};
        }

        std::error_code AuthorizationService::logout() {
            if (m_stateManager.isConnectionDown()) return make_error_code(ErrorCode::connection_down);
            if (!m_stateManager.isAuthorized()) return make_error_code(ErrorCode::not_authorized);
            if (m_operationManager.isOperation(UserOperationType::LOGOUT, m_stateManager.getMyNickname())) return make_error_code(ErrorCode::operation_in_progress);

            m_operationManager.addOperation(UserOperationType::LOGOUT, m_stateManager.getMyNickname());

            auto [uid, packet] = PacketFactory::getLogoutPacket(m_stateManager.getMyNickname());

            createAndStartTask(uid, packet, PacketType::LOGOUT,
                std::bind(&AuthorizationService::onLogoutCompleted, this, _1),
                std::bind(&AuthorizationService::onLogoutFailed, this, _1)
            );

            return {};
        }

        void AuthorizationService::handleReconnect() {
            if (!m_stateManager.isConnectionDown()) {
                return;
            }
            if (m_reconnectInProgress.exchange(true)) {
                return;
            }

            if (m_stateManager.isAuthorized()) {
                auto [uid, packet] = PacketFactory::getReconnectPacket(m_stateManager.getMyNickname(), m_stateManager.getMyToken());

                createAndStartTask(uid, packet, PacketType::RECONNECT,
                    std::bind(&AuthorizationService::onReconnectCompleted, this, _1),
                    std::bind(&AuthorizationService::onReconnectFailed, this, _1)
                );
            }
            else {
                m_stateManager.setConnectionDown(false);
                m_reconnectInProgress = false;
                m_eventListener->onConnectionRestoredAuthorizationNeeded();
            }
        }

        void AuthorizationService::startReconnectRetry() {
            if (m_stateManager.isAuthorized() && m_stateManager.isConnectionDown()) {
                if (m_reconnectRetryThread.joinable()) {
                    m_reconnectRetryThread.join();
                }
                m_stopReconnectRetry = false;
                m_reconnectRetryThread = std::thread([this]() {
                    while (m_stateManager.isConnectionDown() && m_stateManager.isAuthorized() && !m_stopReconnectRetry.load()) {
                        if (!m_reconnectInProgress.exchange(true)) {
                            auto [uid, packet] = PacketFactory::getReconnectPacket(m_stateManager.getMyNickname(), m_stateManager.getMyToken());
                            createAndStartTask(uid, packet, PacketType::RECONNECT,
                                std::bind(&AuthorizationService::onReconnectCompleted, this, _1),
                                std::bind(&AuthorizationService::onReconnectFailed, this, _1)
                            );
                        }
                        for (int i = 0; i < 10 && !m_stopReconnectRetry.load(); ++i) {
                            std::this_thread::sleep_for(1s);
                        }
                    }
                });
            }
        }

        void AuthorizationService::stopReconnectRetry() {
            m_stopReconnectRetry = true;
            if (m_reconnectRetryThread.joinable()) {
                m_reconnectRetryThread.join();
            }
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
        }

        void AuthorizationService::onAuthorizeFailed(const std::string& nickname, std::optional<nlohmann::json> failureContext) {
            m_operationManager.removeOperation(UserOperationType::AUTHORIZE, nickname);

            if (!m_stateManager.isConnectionDown()) {
                LOG_ERROR("Authorization task failed");
                m_eventListener->onAuthorizationResult(ErrorCode::network_error);
            }
        }

        void AuthorizationService::onLogoutCompleted(std::optional<nlohmann::json> completionContext) {
            m_operationManager.removeOperation(UserOperationType::LOGOUT, m_stateManager.getMyNickname());
            // Reset будет вызван из Client
            m_eventListener->onLogoutCompleted();
        }

        void AuthorizationService::onLogoutFailed(std::optional<nlohmann::json> failureContext) {
            m_operationManager.removeOperation(UserOperationType::LOGOUT, m_stateManager.getMyNickname());
            LOG_ERROR("Logout task failed");
            // Reset будет вызван из Client
            m_eventListener->onLogoutCompleted();
        }

        void AuthorizationService::onReconnectCompleted(std::optional<nlohmann::json> completionContext) {
            m_reconnectInProgress = false;
            if (!completionContext.has_value()) {
                LOG_ERROR("onReconnectCompleted called with empty completionContext");
                return;
            }
            auto& context = completionContext.value();

            m_stateManager.setConnectionDown(false);
            m_networkController.notifyConnectionRestored();

            if (!context.contains(RESULT)) {
                LOG_ERROR("RECONNECT_RESULT missing result field, treating as failed");
                // Reset будет вызван из Client
                m_eventListener->onConnectionRestoredAuthorizationNeeded();
                return;
            }
            bool reconnected = context[RESULT].get<bool>();
            if (reconnected) {
                LOG_INFO("Connection restored successfully");
                m_stateManager.setLastReconnectSuccessTime();

                bool activeCall = context.contains(IS_ACTIVE_CALL) ? context[IS_ACTIVE_CALL].get<bool>() : false;

                m_eventListener->onConnectionRestored();

                if (!activeCall) {
                    bool hadActiveCall = m_stateManager.isActiveCall();
                    
                    m_stateManager.setScreenSharing(false);
                    m_stateManager.setCameraSharing(false);
                    m_stateManager.setViewingRemoteScreen(false);
                    m_stateManager.setViewingRemoteCamera(false);
                    m_stateManager.clearCallState();
                    // AudioEngine будет остановлен из Client

                    if (hadActiveCall) {
                        m_eventListener->onCallEndedByRemote({});
                    }
                }
            }
            else {
                LOG_INFO("Connection restored but authorization needed again");
                // Reset будет вызван из Client
                m_eventListener->onConnectionRestoredAuthorizationNeeded();
            }
        }

        void AuthorizationService::onReconnectFailed(std::optional<nlohmann::json> failureContext) {
            m_reconnectInProgress = false;
            LOG_ERROR("Reconnect task failed");
        }
    }
}
