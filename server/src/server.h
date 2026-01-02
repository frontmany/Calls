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
#include "network/networkController.h"
#include "taskManager.h"
#include "packetType.h"

#include "json.hpp"

typedef std::shared_ptr<User> UserPtr;

class CallsServer {
public:
	CallsServer(const std::string& port);
	void run(); 
	void stop(); 

private:
	void onReceive(const unsigned char* data, int size, uint32_t type, const asio::ip::udp::endpoint& endpointFrom);
	std::vector<unsigned char> toBytes(const std::string& value);

	void handleConfirmation(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom);
	void handleAuthorization(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom);
	void handleLogout(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom, bool logoutAndStop = false);
	void handleReconnect(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom, bool logoutAndStop = false);
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

private:
	// Fast lookup by endpoint (for connectionDown/ConnectionRestored callbacks from networkController)
	std::unordered_map<asio::ip::udp::endpoint, UserPtr> m_endpointToUser;
	std::mutex m_endpointToUserMutex;
	
	// Fast lookup by nicknameHash (for processing packets with nicknameHash)
	std::unordered_map<std::string, UserPtr> m_nicknameHashToUser;
	std::mutex m_nicknameHashToUserAndCallsMutex;
	
	// Set of all active calls (optional - can iterate through users if needed)
	// Note: Call is also stored in User via weak_ptr for fast access
	std::unordered_set<std::shared_ptr<Call>> m_calls;
	
	network::NetworkController m_networkController;
	
	// Packet handlers map (similar to client implementation)
	std::unordered_map<PacketType, std::function<void(const nlohmann::json&, const asio::ip::udp::endpoint&)>> m_packetHandlers;

	TaskManager<long long, std::milli> m_taskManager;
	network::NetworkController m_networkController;
};