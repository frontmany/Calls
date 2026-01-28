#pragma once

#include <unordered_map>
#include <memory>
#include <vector>
#include <chrono>
#include <thread>
#include <mutex>
#include <functional>

#include "user.h"
#include "call.h"
#include "pendingCall.h"
#include "network/networkController.h"
#include "network/tcp_control_controller.h"
#include "packetType.h"
#include "services/IUserRepository.h"
#include "services/ICallManager.h"
#include "services/IPacketSender.h"
#include "services/UserRepository.h"
#include "services/CallManager.h"
#include "services/NetworkPacketSender.h"
#include "services/MediaRelayService.h"
#include "json.hpp"

namespace server
{
    class Server {
    public:
        Server(const std::string& tcpPort, const std::string& udpPort);
        void run();
        void stop();

    private:
        void onReceiveUdp(const unsigned char* data, int size, uint32_t type, const asio::ip::udp::endpoint& endpointFrom);
        void onTcpControlPacket(network::OwnedTcpPacket&& owned);
        void onTcpDisconnect(network::TcpConnectionPtr conn);

        void sendTcp(network::TcpConnectionPtr conn, uint32_t type, const std::vector<unsigned char>& body);
        void sendTcpToUser(const std::string& receiverNicknameHash, uint32_t type, const std::string& jsonBody);
        bool sendTcpToUserIfConnected(const std::string& receiverNicknameHash, uint32_t type, const std::vector<unsigned char>& body);

        void handleAuthorizationTcp(const nlohmann::json& json, network::TcpConnectionPtr conn);
        void handleReconnectTcp(const nlohmann::json& json, network::TcpConnectionPtr conn);
        void handleLogoutTcp(const nlohmann::json& json, network::TcpConnectionPtr conn);
        void handleGetFriendInfoTcp(const nlohmann::json& json, network::TcpConnectionPtr conn);
        void handleStartOutgoingCallTcp(const nlohmann::json& json, network::TcpConnectionPtr conn);
        void handleStopOutgoingCallTcp(const nlohmann::json& json, network::TcpConnectionPtr conn);
        void handleAcceptCallTcp(const nlohmann::json& json, network::TcpConnectionPtr conn);
        void handleDeclineCallTcp(const nlohmann::json& json, network::TcpConnectionPtr conn);
        void handleEndCallTcp(const nlohmann::json& json, network::TcpConnectionPtr conn);
        void handleRedirectTcp(const nlohmann::json& json, PacketType type, network::TcpConnectionPtr conn);

        void handleScreen(const unsigned char* data, int size, const asio::ip::udp::endpoint& endpointFrom);
        void handleCamera(const unsigned char* data, int size, const asio::ip::udp::endpoint& endpointFrom);
        void handleVoice(const unsigned char* data, int size, const asio::ip::udp::endpoint& endpointFrom);

        void processUserLogout(const UserPtr& user);
        bool resetOutgoingPendingCall(const UserPtr& user);
        void removeIncomingPendingCall(const UserPtr& user, const PendingCallPtr& pendingCall);
        void processConnectionDown(const UserPtr& user);

    private:
        mutable std::mutex m_mutex;
        std::string m_tcpPort;
        std::string m_udpPort;
        server::network::NetworkController m_networkController;
        std::unique_ptr<server::network::TcpControlController> m_tcpController;
        std::thread m_udpThread;

        server::services::UserRepository m_userRepository;
        server::services::CallManager m_callManager;
        server::services::NetworkPacketSender m_packetSender;
        server::services::MediaRelayService m_mediaRelayService;

        std::unordered_map<network::TcpConnectionPtr, UserPtr> m_connToUser;
    };
}
