#pragma once 

#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <vector>
#include <iostream>
#include <chrono>
#include <thread>
#include <mutex>
#include <functional>

#include "user.h"
#include "call.h"
#include "pendingCall.h"
#include "network/networkController.h"
#include "taskManager.h"
#include "packetType.h"

#include "json.hpp"

namespace server
{
	class Server {
	public:
		Server(const std::string& port);
		void run(); 
		void stop(); 

	private:
		void onReceive(const unsigned char* data, int size, uint32_t type, const asio::ip::udp::endpoint& endpointFrom);

		void handleConfirmation(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom);
		void handleAuthorization(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom);
		void handleLogout(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom);
		void handleReconnect(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom);
		void handleGetFriendInfo(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom);
	
		void handleStartOutgoingCall(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom);
		void handleStopOutgoingCall(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom);
		void handleAcceptCall(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom);
		void handleDeclineCall(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom);
		void handleEndCall(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom);

		void handleScreen(const unsigned char* data, int size, const asio::ip::udp::endpoint& endpointFrom);
		void handleCamera(const unsigned char* data, int size, const asio::ip::udp::endpoint& endpointFrom);
		void handleVoice(const unsigned char* data, int size, const asio::ip::udp::endpoint& endpointFrom);
		void redirect(const nlohmann::json& jsonObject, PacketType type);
	
		void processUserLogout(const UserPtr& user);
		bool resetOutgoingPendingCall(const UserPtr& user);
		void removeIncomingPendingCall(const UserPtr& user, const PendingCallPtr& pendingCall);

	private:
		mutable std::mutex m_mutex;
		std::unordered_map<asio::ip::udp::endpoint, UserPtr> m_endpointToUser;
		std::unordered_map<std::string, UserPtr> m_nicknameHashToUser;

		std::unordered_set<std::shared_ptr<Call>> m_calls;
		std::unordered_set<std::shared_ptr<PendingCall>> m_pendingCalls;
	
		server::network::NetworkController m_networkController;
		TaskManager<long long, std::milli> m_taskManager;

		std::unordered_map<PacketType, std::function<void(const nlohmann::json&, const asio::ip::udp::endpoint&)>> m_packetHandlers;
	};
}