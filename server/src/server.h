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
#include "confirmationKey.h"
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

		void addPendingAction(const ConfirmationKey& key, std::function<void()> action);
		void removePendingAction(const ConfirmationKey& key);
		void clearPendingActionsForUser(const std::string& nicknameHash);

		void sendPacketTask(const std::vector<unsigned char>& packet, PacketType type, const asio::ip::udp::endpoint& endpoint);
		void onTaskCompleted(std::optional<nlohmann::json> completionContext);
		void onTaskFailed(const std::string& message, std::optional<nlohmann::json> failureContext);

	private:
		mutable std::mutex m_mutex;
		
		// Инфраструктура
		server::network::NetworkController m_networkController;
		TaskManager<long long, std::milli> m_taskManager;
		
		// Сервисы (порядок важен для инициализации)
		server::services::UserRepository m_userRepository;
		server::services::CallManager m_callManager;
		server::services::NetworkPacketSender m_packetSender;
		server::services::MediaRelayService m_mediaRelayService;

		std::unordered_map<PacketType, std::function<void(const nlohmann::json&, const asio::ip::udp::endpoint&)>> m_packetHandlers;
		std::unordered_map<ConfirmationKey, std::function<void()>> m_pendingActions;
		std::unordered_map<std::string, std::unordered_set<ConfirmationKey>> m_nicknameHashToConfirmationKeys;
	};
}