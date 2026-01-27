#include "server.h"
#include "packetFactory.h"
#include "jsonType.h"
#include "utilities/crypto.h"
#include "utilities/logger.h"
#include "pendingCall.h"

using namespace std::chrono_literals;
using namespace std::placeholders;
using namespace server;
using namespace server::utilities;

namespace server
{
namespace
{
    std::vector<unsigned char> toBytes(const std::string& value) {
        return std::vector<unsigned char>(value.begin(), value.end());
    }
} 

Server::Server(const std::string& port)
    : m_packetSender(m_networkController)
    , m_mediaRelayService(m_userRepository, m_callManager, m_packetSender)
{
    m_networkController.init(port,
        [this](const unsigned char* data, int size, uint32_t type, const asio::ip::udp::endpoint& endpointFrom) { onReceive(data, size, type, endpointFrom); },
        [this](const asio::ip::udp::endpoint& endpoint) {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto user = m_userRepository.findUserByEndpoint(endpoint);
            if (user) {
                if (!user->isConnectionDown()) {
                    const std::string userPrefix = user->getNicknameHash().length() >= 5 ? user->getNicknameHash().substr(0, 5) : user->getNicknameHash();
                    LOG_INFO("Connection down with user {}", userPrefix);
                    user->setConnectionDown(true);
                    clearPendingActionsForUser(user->getNicknameHash());

                    if (user->hasOutgoingPendingCall()) {
                        auto outgoingPendingCall = user->getOutgoingPendingCall();
                        auto receiver = outgoingPendingCall->getReceiver();
                        
                        if (m_userRepository.containsUser(receiver->getNicknameHash())) {
                            std::string userPrefix = user->getNicknameHash().length() >= 5 ? user->getNicknameHash().substr(0, 5) : user->getNicknameHash();
                            std::string receiverPrefix = receiver->getNicknameHash().length() >= 5 ? receiver->getNicknameHash().substr(0, 5) : receiver->getNicknameHash();
                            LOG_INFO("Outgoing call ended due to connection down: {} -> {}", userPrefix, receiverPrefix);
                            LOG_INFO("Incoming call ended due to connection down: {} -> {}", receiverPrefix, userPrefix);

                            auto receiverInRepo = m_userRepository.findUserByNickname(receiver->getNicknameHash());
                            if (receiverInRepo && !receiverInRepo->isConnectionDown()) {
                                auto [uid, connectionDownPacket] = PacketFactory::getConnectionDownWithUserPacket(user->getNicknameHash());
                                
                                m_taskManager.createTask(uid, 1500ms, 5,
                                    std::bind(&Server::sendPacketTask, this, connectionDownPacket, PacketType::CONNECTION_DOWN_WITH_USER, receiverInRepo->getEndpoint()),
                                    std::bind(&Server::onTaskCompleted, this, _1),
                                    std::bind(&Server::onTaskFailed, this, "Connection down with user task failed", _1)
                                );
                                
                                m_taskManager.startTask(uid);
                            }

                            if (receiverInRepo) {
                                removeIncomingPendingCall(receiverInRepo, outgoingPendingCall);
                            }
                        }

                        resetOutgoingPendingCall(user);
                    }

                    auto incomingCalls = user->getIncomingPendingCalls();
                    for (auto& pendingCall : incomingCalls) {
                        auto initiator = pendingCall->getInitiator();
                        
                        if (m_userRepository.containsUser(initiator->getNicknameHash())) {
                            std::string userPrefix = user->getNicknameHash().length() >= 5 ? user->getNicknameHash().substr(0, 5) : user->getNicknameHash();
                            std::string initiatorPrefix = initiator->getNicknameHash().length() >= 5 ? initiator->getNicknameHash().substr(0, 5) : initiator->getNicknameHash();
                            LOG_INFO("Incoming call ended due to connection down: {} -> {}", initiatorPrefix, userPrefix);
                            LOG_INFO("Outgoing call ended due to connection down: {} -> {}", initiatorPrefix, userPrefix);

                            auto initiatorInRepo = m_userRepository.findUserByNickname(initiator->getNicknameHash());
                            if (initiatorInRepo && !initiatorInRepo->isConnectionDown()) {
                                auto [uid, connectionDownPacket] = PacketFactory::getConnectionDownWithUserPacket(user->getNicknameHash());
                                
                                m_taskManager.createTask(uid, 1500ms, 5,
                                    std::bind(&Server::sendPacketTask, this, connectionDownPacket, PacketType::CONNECTION_DOWN_WITH_USER, initiatorInRepo->getEndpoint()),
                                    std::bind(&Server::onTaskCompleted, this, _1),
                                    std::bind(&Server::onTaskFailed, this, "Connection down with user task failed", _1)
                                );
                                
                                m_taskManager.startTask(uid);
                            }

                            if (initiatorInRepo) {
                                resetOutgoingPendingCall(initiatorInRepo);
                            }
                        }

                        removeIncomingPendingCall(user, pendingCall);
                    }

                    if (user->isInCall()) {
                        auto callPartner = m_callManager.getCallPartner(user->getNicknameHash());
                        if (callPartner && m_userRepository.containsUser(callPartner->getNicknameHash())) {
                            auto callPartnerInRepo = m_userRepository.findUserByNickname(callPartner->getNicknameHash());
                            if (callPartnerInRepo && !callPartnerInRepo->isConnectionDown()) {
                                auto [uid, connectionDownPacket] = PacketFactory::getConnectionDownWithUserPacket(user->getNicknameHash());
                                
                                m_taskManager.createTask(uid, 1500ms, 5,
                                    std::bind(&Server::sendPacketTask, this, connectionDownPacket, PacketType::CONNECTION_DOWN_WITH_USER, callPartnerInRepo->getEndpoint()),
                                    std::bind(&Server::onTaskCompleted, this, _1),
                                    std::bind(&Server::onTaskFailed, this, "Connection down with user task failed", _1)
                                );
                                
                                m_taskManager.startTask(uid);
                            }
                        }
                    }

                    {
                        auto [uidDown, connectionDownToSelf] = PacketFactory::getConnectionDownWithUserPacket(user->getNicknameHash());
                        m_taskManager.createTask(uidDown, 1500ms, 5,
                            std::bind(&Server::sendPacketTask, this, connectionDownToSelf, PacketType::CONNECTION_DOWN_WITH_USER, user->getEndpoint()),
                            std::bind(&Server::onTaskCompleted, this, _1),
                            std::bind(&Server::onTaskFailed, this, "Connection down notify self task failed", _1)
                        );
                        m_taskManager.startTask(uidDown);
                    }
                }
            }
            else {
                m_networkController.removePingMonitoring(endpoint);
            }
        },
        [this](const asio::ip::udp::endpoint& endpoint) {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto user = m_userRepository.findUserByEndpoint(endpoint);
            if (user) {
                const std::string userPrefix = user->getNicknameHash().length() >= 5 ? user->getNicknameHash().substr(0, 5) : user->getNicknameHash();
                LOG_INFO("Connection restored with user {}", userPrefix);
                user->setConnectionDown(false);

                // Tell the restored user (A) they are restored. This gives A a path out of
                // connection-down that does not depend solely on RECONNECT_RESULT, which can
                // be lost on an asymmetric link; the server already knows A is up from the pong.
                auto [uidRestored, packetRestored] = PacketFactory::getConnectionRestoredWithUserPacket(user->getNicknameHash());
                m_taskManager.createTask(uidRestored, 1500ms, 5,
                    std::bind(&Server::sendPacketTask, this, packetRestored, PacketType::CONNECTION_RESTORED_WITH_USER, endpoint),
                    std::bind(&Server::onTaskCompleted, this, _1),
                    std::bind(&Server::onTaskFailed, this, "Connection restored notify self task failed", _1)
                );
                m_taskManager.startTask(uidRestored);

                // If in a call, also notify the partner (B) that A is back.
                if (user->isInCall()) {
                    auto partner = m_callManager.getCallPartner(user->getNicknameHash());
                    if (partner && m_userRepository.containsUser(partner->getNicknameHash())) {
                        auto partnerInRepo = m_userRepository.findUserByNickname(partner->getNicknameHash());
                        if (partnerInRepo && !partnerInRepo->isConnectionDown()) {
                            auto [uidPartner, packetPartner] = PacketFactory::getConnectionRestoredWithUserPacket(user->getNicknameHash());
                            m_taskManager.createTask(uidPartner, 1500ms, 5,
                                std::bind(&Server::sendPacketTask, this, packetPartner, PacketType::CONNECTION_RESTORED_WITH_USER, partnerInRepo->getEndpoint()),
                                std::bind(&Server::onTaskCompleted, this, _1),
                                std::bind(&Server::onTaskFailed, this, "Connection restored with user task failed", _1)
                            );
                            m_taskManager.startTask(uidPartner);
                        }
                    }
                }
            } else {
                LOG_INFO("Connection restored from endpoint {}:{}", endpoint.address().to_string(), endpoint.port());
            }
        }
    );

    m_packetHandlers.emplace(PacketType::AUTHORIZATION, [this](const nlohmann::json& json, const asio::ip::udp::endpoint& endpoint) {handleAuthorization(json, endpoint);});
    m_packetHandlers.emplace(PacketType::LOGOUT, [this](const nlohmann::json& json, const asio::ip::udp::endpoint& endpoint) {handleLogout(json, endpoint);});
    m_packetHandlers.emplace(PacketType::RECONNECT, [this](const nlohmann::json& json, const asio::ip::udp::endpoint& endpoint) {handleReconnect(json, endpoint);});
    m_packetHandlers.emplace(PacketType::GET_USER_INFO, [this](const nlohmann::json& json, const asio::ip::udp::endpoint& endpoint) {handleGetFriendInfo(json, endpoint);});
    m_packetHandlers.emplace(PacketType::CONFIRMATION, [this](const nlohmann::json& json, const asio::ip::udp::endpoint& endpoint) {handleConfirmation(json, endpoint);});
    m_packetHandlers.emplace(PacketType::CALLING_BEGIN, [this](const nlohmann::json& json, const asio::ip::udp::endpoint& endpoint) {handleStartOutgoingCall(json, endpoint);});
    m_packetHandlers.emplace(PacketType::CALLING_END, [this](const nlohmann::json& json, const asio::ip::udp::endpoint& endpoint) {handleStopOutgoingCall(json, endpoint);});
    m_packetHandlers.emplace(PacketType::CALL_ACCEPT, [this](const nlohmann::json& json, const asio::ip::udp::endpoint& endpoint) {handleAcceptCall(json, endpoint);});
    m_packetHandlers.emplace(PacketType::CALL_DECLINE, [this](const nlohmann::json& json, const asio::ip::udp::endpoint& endpoint) {handleDeclineCall(json, endpoint);});
    m_packetHandlers.emplace(PacketType::CALL_END, [this](const nlohmann::json& json, const asio::ip::udp::endpoint& endpoint) {handleEndCall(json, endpoint);});
}

void Server::onReceive(const unsigned char* data, int size, uint32_t rawType, const asio::ip::udp::endpoint& endpointFrom) {
    PacketType type = static_cast<PacketType>(rawType);
    
    // Отметить получение пакета для пользователя (для ping monitoring)
    // Note: ping monitoring is handled by NetworkController
    
    if (type == PacketType::VOICE) {
        handleVoice(data, size, endpointFrom);
        return; 
    }
    else if (type == PacketType::SCREEN) {
        handleScreen(data, size, endpointFrom);
        return;
    }
    else if (type == PacketType::CAMERA) {
        handleCamera(data, size, endpointFrom);
        return;
    }

    try {
        std::string jsonStr = std::string(reinterpret_cast<const char*>(data), size);
        nlohmann::json jsonObject = nlohmann::json::parse(jsonStr);

        std::function<void(const nlohmann::json&, const asio::ip::udp::endpoint&)> handler;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_packetHandlers.contains(type)) {
                handler = m_packetHandlers[type];
            }
        }

        if (handler) {
            handler(jsonObject, endpointFrom);
        }
        else {
            redirect(jsonObject, type);
        }
    }
    catch (const std::exception& e) {
        LOG_ERROR("Error processing packet type {}: {}", static_cast<int>(type), e.what());
    }
}

void Server::run() {
    m_networkController.run();
}

void Server::stop() {
    
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_taskManager.cancelAllTasks();
    }
    
    m_networkController.stop();
    
}

void Server::redirect(const nlohmann::json& jsonObject, PacketType type) {
    const std::string& receiverNicknameHash = jsonObject[RECEIVER_NICKNAME_HASH].get<std::string>();

    auto user = m_userRepository.findUserByNickname(receiverNicknameHash);
    if (user) {
        auto packet = toBytes(jsonObject.dump());
        m_packetSender.send(packet, static_cast<uint32_t>(type), user->getEndpoint());
    }
}

void Server::handleConfirmation(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom) {
    redirect(jsonObject, PacketType::CONFIRMATION);
    
    std::string uid = jsonObject[UID].get<std::string>();
    std::string receiverNicknameHash = jsonObject[RECEIVER_NICKNAME_HASH].get<std::string>();
    std::string senderNicknameHash = jsonObject[SENDER_NICKNAME_HASH].get<std::string>();

    m_taskManager.completeTask(uid);

    std::function<void()> actionToExecute;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto sender = m_userRepository.findUserByNickname(senderNicknameHash);
        if (sender) {
            ConfirmationKey key = ConfirmationKey(uid, sender->getNicknameHash(), sender->getToken());

            if (m_pendingActions.contains(key)) {
                actionToExecute = m_pendingActions.at(key);
                removePendingAction(key);
            }
        }
    }

    if (actionToExecute) {
        actionToExecute();
    }
}

void Server::handleAuthorization(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom) {
    try {
        std::string uid = jsonObject[UID].get<std::string>();
        std::string nicknameHash = jsonObject[SENDER_NICKNAME_HASH].get<std::string>();
        CryptoPP::RSA::PublicKey publicKey = crypto::deserializePublicKey(jsonObject[PUBLIC_KEY]);
        
        bool authorized = false;
        std::string token = "";
        
        if (m_userRepository.containsUser(nicknameHash)) {
            std::string nicknameHashPrefix = nicknameHash.length() >= 5 ? nicknameHash.substr(0, 5) : nicknameHash;
            LOG_WARN("Authorization failed - nickname already taken: {}", nicknameHashPrefix);
        }
        else {
            token = crypto::generateUID();

            UserPtr user = std::make_shared<User>(nicknameHash, token, publicKey, endpointFrom,
                [this, nicknameHash]() {
                    auto user = m_userRepository.findUserByNickname(nicknameHash);
                    if (user) {
                        processUserLogout(user);
                    }
                }
            );

            m_userRepository.addUser(user);
            authorized = true;
            
            std::string nicknameHashPrefix = nicknameHash.length() >= 5 ? nicknameHash.substr(0, 5) : nicknameHash;
            LOG_INFO("User authorized: {}", nicknameHashPrefix);
        }

        std::vector<unsigned char> packet;
        if (authorized)
            packet = PacketFactory::getAuthorizationResultPacket(true, uid, nicknameHash, token);
        else
            packet = PacketFactory::getAuthorizationResultPacket(false, uid, nicknameHash);

        m_packetSender.send(packet, static_cast<uint32_t>(PacketType::AUTHORIZATION_RESULT), endpointFrom);
    }
    catch (const std::exception& e) {
        LOG_ERROR("Error in authorization packet: {}", e.what());
    }
}

void Server::handleLogout(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom) {
    try {
        std::string uid = jsonObject[UID].get<std::string>();
        std::string senderNicknameHash = jsonObject[SENDER_NICKNAME_HASH].get<std::string>();

        auto packet = PacketFactory::getConfirmationPacket(uid, senderNicknameHash);
        m_packetSender.send(packet, static_cast<uint32_t>(PacketType::CONFIRMATION), endpointFrom);

        auto user = m_userRepository.findUserByNickname(senderNicknameHash);
        if (user) {
            processUserLogout(user);
        }
    }
    catch (const std::exception& e) {
        LOG_ERROR("Error during logout: {}", e.what());
    }
}

void Server::handleReconnect(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom) {
    std::string uid = jsonObject[UID].get<std::string>();
    std::string token = jsonObject[TOKEN].get<std::string>();
    std::string senderNicknameHash = jsonObject[SENDER_NICKNAME_HASH].get<std::string>();

    auto user = m_userRepository.findUserByNickname(senderNicknameHash);
    if (!user) {
        // Пользователь не найден - отправляем отрицательный результат
        auto packet = PacketFactory::getReconnectionResultPacket(false, uid, senderNicknameHash, token);
        m_taskManager.createTask(uid, 1500ms, 5,
            std::bind(&Server::sendPacketTask, this, packet, PacketType::RECONNECT_RESULT, endpointFrom),
            std::bind(&Server::onTaskCompleted, this, _1),
            std::bind(&Server::onTaskFailed, this, "Reconnect result task failed", _1)
        );
        m_taskManager.startTask(uid);
        return;
    }

    bool reconnectionAllowed = false;
    if (user->getToken() == token) {
        user->setConnectionDown(false);
        reconnectionAllowed = true;

        // После WiFi off/on endpoint клиента часто меняется. Обновляем endpoint
        asio::ip::udp::endpoint oldEndpoint = user->getEndpoint();
        if (oldEndpoint != endpointFrom) {
            m_networkController.removePingMonitoring(oldEndpoint);
            m_userRepository.updateUserEndpoint(senderNicknameHash, endpointFrom);
            m_networkController.addEndpointForPing(endpointFrom);
        }
    }

    std::vector<unsigned char> packet;
    if (reconnectionAllowed)
        packet = PacketFactory::getReconnectionResultPacket(true, uid, senderNicknameHash, token, user->isInCall());
    else
        packet = PacketFactory::getReconnectionResultPacket(false, uid, senderNicknameHash, token);
        
    // Используем TaskManager для гарантированной доставки RECONNECT_RESULT
    m_taskManager.createTask(uid, 1500ms, 5,
        std::bind(&Server::sendPacketTask, this, packet, PacketType::RECONNECT_RESULT, endpointFrom),
        std::bind(&Server::onTaskCompleted, this, _1),
        std::bind(&Server::onTaskFailed, this, "Reconnect result task failed", _1)
    );
    m_taskManager.startTask(uid);

        // Сразу шлём CONNECTION_RESTORED_WITH_USER на endpointFrom (мы только что получили RECONNECT
        // оттуда). Не полагаемся только на pong: при WiFi off/on pong может прийти с другого
        // endpoint или после лишних ретраев. Дубликат уйдёт ещё из connection restored (pong) — ок.
        if (reconnectionAllowed) {
            auto [uidRestored, packetRestored] = PacketFactory::getConnectionRestoredWithUserPacket(user->getNicknameHash());
            m_taskManager.createTask(uidRestored, 1500ms, 5,
                std::bind(&Server::sendPacketTask, this, packetRestored, PacketType::CONNECTION_RESTORED_WITH_USER, endpointFrom),
                std::bind(&Server::onTaskCompleted, this, _1),
                std::bind(&Server::onTaskFailed, this, "Connection restored notify self task failed", _1)
            );
            m_taskManager.startTask(uidRestored);
        }

    // Сразу шлём CONNECTION_RESTORED_WITH_USER на endpointFrom (мы только что получили RECONNECT
    // оттуда). Не полагаемся только на pong: при WiFi off/on pong может прийти с другого
    // endpoint или после лишних ретраев. Дубликат уйдёт ещё из connection restored (pong) — ок.
    if (reconnectionAllowed) {
        auto [uidRestored, packetRestored] = PacketFactory::getConnectionRestoredWithUserPacket(user->getNicknameHash());
        m_taskManager.createTask(uidRestored, 1500ms, 5,
            std::bind(&Server::sendPacketTask, this, packetRestored, PacketType::CONNECTION_RESTORED_WITH_USER, endpointFrom),
            std::bind(&Server::onTaskCompleted, this, _1),
            std::bind(&Server::onTaskFailed, this, "Connection restored notify self task failed", _1)
        );
        m_taskManager.startTask(uidRestored);
    }

    if (reconnectionAllowed && user->isInCall()) {
        auto partner = m_callManager.getCallPartner(user->getNicknameHash());
        if (partner && m_userRepository.containsUser(partner->getNicknameHash())) {
            auto partnerInRepo = m_userRepository.findUserByNickname(partner->getNicknameHash());
            if (!partnerInRepo->isConnectionDown()) {
                auto [uid, connectionRestoredWithUserPacket] = PacketFactory::getConnectionRestoredWithUserPacket(user->getNicknameHash());

                m_taskManager.createTask(uid, 1500ms, 5,
                    std::bind(&Server::sendPacketTask, this, connectionRestoredWithUserPacket, PacketType::CONNECTION_RESTORED_WITH_USER, partnerInRepo->getEndpoint()),
                    std::bind(&Server::onTaskCompleted, this, _1),
                    std::bind(&Server::onTaskFailed, this, "Connection restored with user task failed", _1)
                );

                m_taskManager.startTask(uid);
            }
            else {
                auto [uid, connectionDownWithUserPacket] = PacketFactory::getConnectionDownWithUserPacket(partner->getNicknameHash());

                m_taskManager.createTask(uid, 1500ms, 5,
                    std::bind(&Server::sendPacketTask, this, connectionDownWithUserPacket, PacketType::CONNECTION_DOWN_WITH_USER, user->getEndpoint()),
                    std::bind(&Server::onTaskCompleted, this, _1),
                    std::bind(&Server::onTaskFailed, this, "Connection restored with user task failed", _1)
                );

                m_taskManager.startTask(uid);
            }
        }
    }
}

void Server::handleGetFriendInfo(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom) {
    try {
        std::string uid = jsonObject[UID];
        std::string nicknameHash = jsonObject[NICKNAME_HASH];
        std::string senderNicknameHash = jsonObject[SENDER_NICKNAME_HASH];

        auto requestedUser = m_userRepository.findUserByNickname(nicknameHash);
        if (requestedUser && m_userRepository.containsUser(senderNicknameHash)) {
            auto packet = PacketFactory::getUserInfoResultPacket(true, uid, requestedUser->getNicknameHash(), requestedUser->getPublicKey());
            m_packetSender.send(packet, static_cast<uint32_t>(PacketType::GET_USER_INFO_RESULT), endpointFrom);
        }
        else {
            auto packet = PacketFactory::getUserInfoResultPacket(false, uid, nicknameHash);
            m_packetSender.send(packet, static_cast<uint32_t>(PacketType::GET_USER_INFO_RESULT), endpointFrom);
        }
    }
    catch (const std::exception& e) {
        LOG_ERROR("Error in get friend info packet: {}", e.what());
    }
}

void Server::handleStartOutgoingCall(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom) {
    try {
        std::string uid = jsonObject[UID].get<std::string>();
        std::string receiverNicknameHash = jsonObject[RECEIVER_NICKNAME_HASH].get<std::string>();
        std::string senderNicknameHash = jsonObject[SENDER_NICKNAME_HASH].get<std::string>();

        auto receiver = m_userRepository.findUserByNickname(receiverNicknameHash);
        if (receiver) {
            ConfirmationKey key(uid, receiver->getNicknameHash(), receiver->getToken());

            addPendingAction(key, [this, receiverNicknameHash, senderNicknameHash]() {
                    auto receiver = m_userRepository.findUserByNickname(receiverNicknameHash);
                    auto sender = m_userRepository.findUserByNickname(senderNicknameHash);

                    if (receiver && sender) {
                        std::shared_ptr<PendingCall> pendingCall = std::make_shared<PendingCall>(sender, receiver,
                            [this, receiver, sender]() {
                                auto outgoingPendingCall = sender->getOutgoingPendingCall();
                                if (outgoingPendingCall && outgoingPendingCall->getReceiver()->getNicknameHash() == receiver->getNicknameHash()) {
                                    resetOutgoingPendingCall(sender);
                                    removeIncomingPendingCall(receiver, outgoingPendingCall);
                                }
                            }
                        );

                        m_callManager.addPendingCall(pendingCall);
                        sender->setOutgoingPendingCall(pendingCall);
                        receiver->addIncomingPendingCall(pendingCall);

                        std::string senderPrefix = senderNicknameHash.length() >= 5 ? senderNicknameHash.substr(0, 5) : senderNicknameHash;
                        std::string receiverPrefix = receiverNicknameHash.length() >= 5 ? receiverNicknameHash.substr(0, 5) : receiverNicknameHash;
                        LOG_INFO("Call initiated from {} to {}", senderPrefix, receiverPrefix);
                    }
                }
            );

            if (!receiver->isConnectionDown()) {
                m_packetSender.send(toBytes(jsonObject.dump()),
                    static_cast<uint32_t>(PacketType::CALLING_BEGIN),
                    receiver->getEndpoint()
                );
            }
        }
    }
    catch (const std::exception& e) {
        LOG_ERROR("Error in start outgoing call packet: {}", e.what());
    }
}

void Server::handleStopOutgoingCall(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom) {
    try {
        std::string uid = jsonObject[UID].get<std::string>();
        std::string receiverNicknameHash = jsonObject[RECEIVER_NICKNAME_HASH].get<std::string>();
        std::string senderNicknameHash = jsonObject[SENDER_NICKNAME_HASH].get<std::string>();

        auto receiver = m_userRepository.findUserByNickname(receiverNicknameHash);
        if (receiver) {

            ConfirmationKey key(uid, receiver->getNicknameHash(), receiver->getToken());

            addPendingAction(key, [this, receiverNicknameHash, senderNicknameHash]() {
                std::lock_guard<std::mutex> lock(m_mutex);

                auto receiver = m_userRepository.findUserByNickname(receiverNicknameHash);
                auto sender = m_userRepository.findUserByNickname(senderNicknameHash);

                if (receiver && sender) {
                    auto outgoingPendingCall = sender->getOutgoingPendingCall();
                    if (outgoingPendingCall && outgoingPendingCall->getReceiver()->getNicknameHash() == receiverNicknameHash) {
                        resetOutgoingPendingCall(sender);
                        removeIncomingPendingCall(receiver, outgoingPendingCall);

                        std::string senderPrefix = senderNicknameHash.length() >= 5 ? senderNicknameHash.substr(0, 5) : senderNicknameHash;
                        std::string receiverPrefix = receiverNicknameHash.length() >= 5 ? receiverNicknameHash.substr(0, 5) : receiverNicknameHash;
                        LOG_INFO("Calling stopped: {} -> {}", senderPrefix, receiverPrefix);
                    }
                }
            });

            if (receiver->isConnectionDown()) {
                auto packet = PacketFactory::getConfirmationPacket(uid, senderNicknameHash);

                m_packetSender.send(packet,
                    static_cast<uint32_t>(PacketType::CONFIRMATION),
                    endpointFrom
                );
            }
            else {
                m_packetSender.send(toBytes(jsonObject.dump()),
                    static_cast<uint32_t>(PacketType::CALLING_END),
                    receiver->getEndpoint()
                );
            }
        }
        else {
            auto packet = PacketFactory::getConfirmationPacket(uid, senderNicknameHash);

            m_packetSender.send(packet,
                static_cast<uint32_t>(PacketType::CONFIRMATION),
                endpointFrom
            );
        }
    }
    catch (const std::exception& e) {
        LOG_ERROR("Error in stop calling packet: {}", e.what());
    }
}

void Server::handleAcceptCall(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom) {
    try {
        std::string uid = jsonObject[UID].get<std::string>();
        std::string senderNicknameHash = jsonObject[SENDER_NICKNAME_HASH].get<std::string>();
        std::string receiverNicknameHash = jsonObject[RECEIVER_NICKNAME_HASH].get<std::string>();

        auto receiver = m_userRepository.findUserByNickname(receiverNicknameHash);
        if (receiver) {

            ConfirmationKey key(uid, receiver->getNicknameHash(), receiver->getToken());

            addPendingAction(key, [this, receiverNicknameHash, senderNicknameHash]() {
                std::lock_guard<std::mutex> lock(m_mutex);

                auto receiver = m_userRepository.findUserByNickname(receiverNicknameHash);
                auto sender = m_userRepository.findUserByNickname(senderNicknameHash);

                if (receiver && sender) {
                    auto foundPendingCall = receiver->getOutgoingPendingCall();

                    if (foundPendingCall && foundPendingCall->getReceiver()->getNicknameHash() == senderNicknameHash) {
                        resetOutgoingPendingCall(receiver);
                        removeIncomingPendingCall(sender, foundPendingCall);

                        auto call = m_callManager.createCall(receiver, sender);
                        receiver->setCall(call);
                        sender->setCall(call);

                        auto declineIncomingPendingCalls = [this](const UserPtr& busyUser) {
                            if (!busyUser) {
                                return;
                            }

                            auto incomingCalls = busyUser->getIncomingPendingCalls();
                            for (const auto& pendingCall : incomingCalls) {
                                auto initiator = pendingCall->getInitiator();
                                if (!initiator) {
                                    removeIncomingPendingCall(busyUser, pendingCall);
                                    continue;
                                }

                                resetOutgoingPendingCall(initiator);
                                removeIncomingPendingCall(busyUser, pendingCall);

                                if (m_userRepository.containsUser(initiator->getNicknameHash())) {
                                    auto initiatorInRepo = m_userRepository.findUserByNickname(initiator->getNicknameHash());
                                    if (initiatorInRepo && !initiatorInRepo->isConnectionDown()) {
                                        auto [uid, declinePacket] = PacketFactory::getCallDeclinedPacket(busyUser->getNicknameHash(), initiator->getNicknameHash());
                                        m_taskManager.createTask(uid, 1500ms, 3,
                                            std::bind(&Server::sendPacketTask, this, declinePacket, PacketType::CALL_DECLINE, initiatorInRepo->getEndpoint()),
                                            std::bind(&Server::onTaskCompleted, this, _1),
                                            std::bind(&Server::onTaskFailed, this, "Call decline notification task failed", _1)
                                        );
                                        m_taskManager.startTask(uid);
                                    }
                                }
                            }
                        };

                        declineIncomingPendingCalls(receiver);
                        declineIncomingPendingCalls(sender);

                        std::string senderPrefix = senderNicknameHash.length() >= 5 ? senderNicknameHash.substr(0, 5) : senderNicknameHash;
                        std::string receiverPrefix = receiverNicknameHash.length() >= 5 ? receiverNicknameHash.substr(0, 5) : receiverNicknameHash;
                        LOG_INFO("Call accepted: {} -> {}", receiverPrefix, senderPrefix);
                    }
                }
            });

            if (!receiver->isConnectionDown()) {
                m_packetSender.send(toBytes(jsonObject.dump()),
                    static_cast<uint32_t>(PacketType::CALL_ACCEPT),
                    receiver->getEndpoint()
                );
            }
        }
    }
    catch (const std::exception& e) {
        LOG_ERROR("Error in call accepted packet: {}", e.what());
    }
}

void Server::handleDeclineCall(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom) {
    try {
        std::string uid = jsonObject[UID].get<std::string>();
        std::string receiverNicknameHash = jsonObject[RECEIVER_NICKNAME_HASH].get<std::string>();
        std::string senderNicknameHash = jsonObject[SENDER_NICKNAME_HASH].get<std::string>();

        std::unique_lock<std::mutex> lock(m_mutex);

        auto onConfirmationReceived = [this, receiverNicknameHash, senderNicknameHash]() {
            auto receiver = m_userRepository.findUserByNickname(receiverNicknameHash);
            auto sender = m_userRepository.findUserByNickname(senderNicknameHash);

            if (receiver && sender) {
                PendingCallPtr foundPendingCall = nullptr;
                auto incomingCalls = receiver->getIncomingPendingCalls();
                for (auto& pendingCall : incomingCalls) {
                    if (pendingCall->getInitiator()->getNicknameHash() == senderNicknameHash) {
                        foundPendingCall = pendingCall;
                        break;
                    }
                }

                if (foundPendingCall) {
                    resetOutgoingPendingCall(sender);
                    removeIncomingPendingCall(receiver, foundPendingCall);

                    std::string senderPrefix = senderNicknameHash.length() >= 5 ? senderNicknameHash.substr(0, 5) : senderNicknameHash;
                    std::string receiverPrefix = receiverNicknameHash.length() >= 5 ? receiverNicknameHash.substr(0, 5) : receiverNicknameHash;
                    LOG_INFO("Call declined: {} -> {}", senderPrefix, receiverPrefix);
                }
            }
        };

        auto receiver = m_userRepository.findUserByNickname(receiverNicknameHash);
        if (receiver) {
            if (receiver->isConnectionDown()) {
                lock.unlock();
                onConfirmationReceived();
                lock.lock();

                auto packet = PacketFactory::getConfirmationPacket(uid, senderNicknameHash);

                m_packetSender.send(packet,
                    static_cast<uint32_t>(PacketType::CONFIRMATION),
                    endpointFrom
                );
            }
            else {
                ConfirmationKey key(uid, receiver->getNicknameHash(), receiver->getToken());
                addPendingAction(key, onConfirmationReceived);

                m_packetSender.send(
                    toBytes(jsonObject.dump()),
                    static_cast<uint32_t>(PacketType::CALL_DECLINE),
                    receiver->getEndpoint()
                );
            }
        }
        else {
            lock.unlock();
            onConfirmationReceived();
            lock.lock();

            auto packet = PacketFactory::getConfirmationPacket(uid, senderNicknameHash);

            m_packetSender.send(packet,
                static_cast<uint32_t>(PacketType::CONFIRMATION),
                endpointFrom
            );
        }
    }
    catch (const std::exception& e) {
        LOG_ERROR("Error in call declined packet: {}", e.what());
    }
}

void Server::handleEndCall(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom) {
    try {
        std::string uid = jsonObject[UID].get<std::string>();
        std::string senderNicknameHash = jsonObject[SENDER_NICKNAME_HASH].get<std::string>();
        std::string receiverNicknameHash = jsonObject[RECEIVER_NICKNAME_HASH].get<std::string>();

        auto onConfirmationReceived = [this, receiverNicknameHash, senderNicknameHash]() {
            auto receiver = m_userRepository.findUserByNickname(receiverNicknameHash);
            auto sender = m_userRepository.findUserByNickname(senderNicknameHash);

            if (receiver && sender && sender->isInCall() && receiver->isInCall()) {
                std::string senderPrefix = senderNicknameHash.length() >= 5 ? senderNicknameHash.substr(0, 5) : senderNicknameHash;
                std::string receiverPrefix = receiverNicknameHash.length() >= 5 ? receiverNicknameHash.substr(0, 5) : receiverNicknameHash;
                LOG_INFO("Call ended: {} ended call with {}", senderPrefix, receiverPrefix);

                auto call = sender->getCall();
                if (call) {
                    m_callManager.endCall(sender->getNicknameHash(), receiver->getNicknameHash());
                }
                sender->resetCall();
                receiver->resetCall();
            }
        };

        auto receiver = m_userRepository.findUserByNickname(receiverNicknameHash);
        if (receiver) {
            if (receiver->isConnectionDown()) {
                onConfirmationReceived();

                auto packet = PacketFactory::getConfirmationPacket(uid, senderNicknameHash);

                m_packetSender.send(packet,
                    static_cast<uint32_t>(PacketType::CONFIRMATION),
                    endpointFrom
                );
            }
            else {
                ConfirmationKey key(uid, receiver->getNicknameHash(), receiver->getToken());

                addPendingAction(key, onConfirmationReceived);

                m_packetSender.send(toBytes(jsonObject.dump()),
                    static_cast<uint32_t>(PacketType::CALL_END),
                    receiver->getEndpoint()
                );
            }
        }
        else {
            onConfirmationReceived();

            auto packet = PacketFactory::getConfirmationPacket(uid, senderNicknameHash);

            m_packetSender.send(packet,
                static_cast<uint32_t>(PacketType::CONFIRMATION),
                endpointFrom
            );
        }
    }
    catch (const std::exception& e) {
        LOG_ERROR("Error in end call packet: {}", e.what());
    }
}

void Server::handleVoice(const unsigned char* data, int size, const asio::ip::udp::endpoint& endpointFrom) {
    m_mediaRelayService.relayMedia(data, size, PacketType::VOICE, endpointFrom);
}

void Server::handleScreen(const unsigned char* data, int size, const asio::ip::udp::endpoint& endpointFrom) {
    m_mediaRelayService.relayMedia(data, size, PacketType::SCREEN, endpointFrom);
}

void Server::handleCamera(const unsigned char* data, int size, const asio::ip::udp::endpoint& endpointFrom) {
    m_mediaRelayService.relayMedia(data, size, PacketType::CAMERA, endpointFrom);
}

void Server::processUserLogout(const UserPtr& user) {
    if (!user) return;
    
    std::string nicknameHash = user->getNicknameHash();
    std::string nicknameHashPrefix = nicknameHash.length() >= 5 ? nicknameHash.substr(0, 5) : nicknameHash;
    LOG_INFO("User logout: {}", nicknameHashPrefix);
    
    if (!m_userRepository.containsUser(nicknameHash)) return;

    if (user->isInCall()) {
        auto callPartner = m_callManager.getCallPartner(user->getNicknameHash());

        if (callPartner && m_userRepository.containsUser(callPartner->getNicknameHash())) {
            auto callPartnerInRepo = m_userRepository.findUserByNickname(callPartner->getNicknameHash());
            std::string callPartnerPrefix = callPartnerInRepo->getNicknameHash().length() >= 5 ? callPartnerInRepo->getNicknameHash().substr(0, 5) : callPartnerInRepo->getNicknameHash();
            LOG_INFO("Call ended due to logout: {} ended call with {}", nicknameHashPrefix, callPartnerPrefix);

            if (!callPartnerInRepo->isConnectionDown()) {
                auto [uid, packet] = PacketFactory::getUserLogoutPacket(user->getNicknameHash());
                m_taskManager.createTask(uid, 1500ms, 5,
                    std::bind(&Server::sendPacketTask, this, packet, PacketType::USER_LOGOUT, callPartnerInRepo->getEndpoint()),
                    std::bind(&Server::onTaskCompleted, this, _1),
                    std::bind(&Server::onTaskFailed, this, "User logout task failed", _1)
                );

                m_taskManager.startTask(uid);
            }

            callPartnerInRepo->resetCall();
        }

        if (auto call = user->getCall(); call) {
            auto partner = m_callManager.getCallPartner(user->getNicknameHash());
            if (partner) {
                m_callManager.endCall(user->getNicknameHash(), partner->getNicknameHash());
            }
        }

        user->resetCall();
    }

    if (user->hasOutgoingPendingCall()) {
        auto outgoingPendingCall = user->getOutgoingPendingCall();
        auto pendingCallPartner = user->getOutgoingPendingCallPartner();

        if (pendingCallPartner && m_userRepository.containsUser(pendingCallPartner->getNicknameHash())) {
            auto pendingCallPartnerInRepo = m_userRepository.findUserByNickname(pendingCallPartner->getNicknameHash());
            std::string pendingCallPartnerPrefix = pendingCallPartnerInRepo->getNicknameHash().length() >= 5 ? pendingCallPartnerInRepo->getNicknameHash().substr(0, 5) : pendingCallPartnerInRepo->getNicknameHash();
            LOG_INFO("Outgoing call ended due to logout: {} -> {}", nicknameHashPrefix, pendingCallPartnerPrefix);
            LOG_INFO("Incoming call ended due to logout: {} -> {}", pendingCallPartnerPrefix, nicknameHashPrefix);

            if (!pendingCallPartnerInRepo->isConnectionDown()) {
                auto [uid, packet] = PacketFactory::getUserLogoutPacket(user->getNicknameHash());
                m_taskManager.createTask(uid, 1500ms, 5,
                    std::bind(&Server::sendPacketTask, this, packet, PacketType::USER_LOGOUT, pendingCallPartnerInRepo->getEndpoint()),
                    std::bind(&Server::onTaskCompleted, this, _1),
                    std::bind(&Server::onTaskFailed, this, "User logout task failed", _1)
                );

                m_taskManager.startTask(uid);
            }

            removeIncomingPendingCall(pendingCallPartnerInRepo, outgoingPendingCall);
        }

        resetOutgoingPendingCall(user);
    }

    auto incomingCalls = user->getIncomingPendingCalls();
    for (auto& pendingCall : incomingCalls) {
        auto pendingCallPartner = pendingCall->getInitiator();

        if (pendingCallPartner && m_userRepository.containsUser(pendingCallPartner->getNicknameHash())) {
            auto pendingCallPartnerInRepo = m_userRepository.findUserByNickname(pendingCallPartner->getNicknameHash());
            std::string pendingCallPartnerPrefix = pendingCallPartnerInRepo->getNicknameHash().length() >= 5 ? pendingCallPartnerInRepo->getNicknameHash().substr(0, 5) : pendingCallPartnerInRepo->getNicknameHash();
            LOG_INFO("Incoming call ended due to logout: {} -> {}", pendingCallPartnerPrefix, nicknameHashPrefix);
            LOG_INFO("Outgoing call ended due to logout: {} -> {}", pendingCallPartnerPrefix, nicknameHashPrefix);

            if (!pendingCallPartnerInRepo->isConnectionDown()) {
                auto [uid, packet] = PacketFactory::getUserLogoutPacket(user->getNicknameHash());
                m_taskManager.createTask(uid, 1500ms, 5,
                    std::bind(&Server::sendPacketTask, this, packet, PacketType::USER_LOGOUT, pendingCallPartnerInRepo->getEndpoint()),
                    std::bind(&Server::onTaskCompleted, this, _1),
                    std::bind(&Server::onTaskFailed, this, "User logout task failed", _1)
                );

                m_taskManager.startTask(uid);
            }

            resetOutgoingPendingCall(pendingCallPartnerInRepo);
        }

        removeIncomingPendingCall(user, pendingCall);
    }
    user->resetAllPendingCalls();

    clearPendingActionsForUser(user->getNicknameHash());

    m_networkController.removePingMonitoring(user->getEndpoint());
    m_userRepository.removeUser(nicknameHash);
}

bool Server::resetOutgoingPendingCall(const UserPtr& user) {
    if (user->hasOutgoingPendingCall()) {
        auto pendingCall = user->getOutgoingPendingCall();

        if (m_callManager.hasPendingCall(pendingCall)) 
            m_callManager.removePendingCall(pendingCall);
        
        user->resetOutgoingPendingCall();

        return true;
    }

    return false;
}

void Server::removeIncomingPendingCall(const UserPtr& user, const PendingCallPtr& pendingCall) {
    if (user->hasIncomingPendingCall(pendingCall)) {
        if (m_callManager.hasPendingCall(pendingCall))
            m_callManager.removePendingCall(pendingCall);
        
        user->removeIncomingPendingCall(pendingCall);
    }
}

void Server::addPendingAction(const ConfirmationKey& key, std::function<void()> action)
{
    if (!m_pendingActions.contains(key)) {
        m_pendingActions.emplace(key, std::move(action));
        m_nicknameHashToConfirmationKeys[key.nicknameHash].insert(key);
    }
}

void Server::removePendingAction(const ConfirmationKey& key)
{
    if (m_pendingActions.contains(key)) {
        m_pendingActions.erase(key);
        auto it = m_nicknameHashToConfirmationKeys.find(key.nicknameHash);
        if (it != m_nicknameHashToConfirmationKeys.end()) {
            it->second.erase(key);
            if (it->second.empty()) {
                m_nicknameHashToConfirmationKeys.erase(it);
            }
        }
    }
}

void Server::clearPendingActionsForUser(const std::string& nicknameHash)
{
    auto it = m_nicknameHashToConfirmationKeys.find(nicknameHash);
    if (it != m_nicknameHashToConfirmationKeys.end()) {
        for (const auto& key : it->second) {
            m_pendingActions.erase(key);
        }
        m_nicknameHashToConfirmationKeys.erase(it);
    }
}

void Server::sendPacketTask(const std::vector<unsigned char>& packet, PacketType type, const asio::ip::udp::endpoint& endpoint) {
    m_packetSender.send(packet, static_cast<uint32_t>(type), endpoint);
}

void Server::onTaskCompleted(std::optional<nlohmann::json> completionContext) {
}

void Server::onTaskFailed(const std::string& message, std::optional<nlohmann::json> failureContext) {
    LOG_ERROR(message);
}
} // namespace server