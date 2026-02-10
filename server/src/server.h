#pragma once

#include <unordered_map>
#include <memory>
#include <vector>
#include <chrono>
#include <thread>
#include <mutex>
#include <functional>

#include "models/user.h"
#include "models/call.h"
#include "models/pendingCall.h"
#include "network/networkController.h"
#include "constants/packetType.h"
#include "logic/userRepository.h"
#include "logic/callManager.h"

#include "json.hpp"

namespace server
{
    class Server {
    public:
        Server(const std::string& tcpPort, const std::string& udpPort);
        void run();
        void stop();

    private:
        using TcpPacketHandler = std::function<void(const nlohmann::json&, network::tcp::ConnectionPtr)>;

        void registerHandlers();

        void handleReceiveUdp(const unsigned char* data, int size, uint32_t type, const asio::ip::udp::endpoint& endpointFrom);
        void handleReceiveTcp(network::tcp::OwnedPacket&& owned);
        void handleConnectionWithUserDown(network::tcp::ConnectionPtr conn);

        void sendTcp(network::tcp::ConnectionPtr conn, uint32_t type, const std::vector<unsigned char>& body);
        void sendTcpToUser(const std::string& receiverNicknameHash, uint32_t type, const std::string& jsonBody);
        bool sendTcpToUserIfConnected(const std::string& receiverNicknameHash, uint32_t type, const std::vector<unsigned char>& body);

        void handleAuthorization(const nlohmann::json& json, network::tcp::ConnectionPtr conn);
        void handleReconnect(const nlohmann::json& json, network::tcp::ConnectionPtr conn);
        void handleLogout(const nlohmann::json& json, network::tcp::ConnectionPtr conn);
        void handleGetFriendInfo(const nlohmann::json& json, network::tcp::ConnectionPtr conn);
        void handleStartOutgoingCall(const nlohmann::json& json, network::tcp::ConnectionPtr conn);
        void handleStopOutgoingCall(const nlohmann::json& json, network::tcp::ConnectionPtr conn);
        void handleAcceptCall(const nlohmann::json& json, network::tcp::ConnectionPtr conn);
        void handleDeclineCall(const nlohmann::json& json, network::tcp::ConnectionPtr conn);
        void handleEndCall(const nlohmann::json& json, network::tcp::ConnectionPtr conn);
        void redirectPacket(const nlohmann::json& json, constant::PacketType type, network::tcp::ConnectionPtr conn);

        void processUserLogout(const UserPtr& user);
        bool resetOutgoingPendingCall(const UserPtr& user);
        void removeIncomingPendingCall(const UserPtr& user, const PendingCallPtr& pendingCall);
        void processConnectionDown(const UserPtr& user);

    private:
        mutable std::mutex m_mutex;

        server::network::NetworkController m_networkController;

        server::logic::UserRepository m_userRepository;
        server::logic::CallManager m_callManager;

        std::unordered_map<constant::PacketType, TcpPacketHandler> m_packetHandlers;
        std::unordered_map<network::tcp::ConnectionPtr, UserPtr> m_connToUser;
    };
}
