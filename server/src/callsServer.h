#pragma once 

#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <vector>
#include <iostream>
#include <chrono>
#include <thread>

#include "user.h"
#include "call.h"
#include "networkController.h"

#include "asio.hpp"
#include "json.hpp"

typedef std::shared_ptr<User> UserPtr;

class CallsServer {
public:
	CallsServer();
	void run();

private:
	void onReceive(const unsigned char* data, int size, PacketType type, const asio::ip::udp::endpoint& endpointFrom);
	void onNetworkError();

	void handleAuthorizationPacket(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom);
	void handleLogout(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom, bool logoutAndStop = false);
	void handleGetFriendInfoPacket(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom);

	void handleStartCallingPacket(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom);
	void handleEndCallPacket(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom);
	void handleStopCallingPacket(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom);
	void handleCallAcceptedPacket(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom);
	void handleCallDeclinedPacket(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom);
	void handleCallAcceptedOkPacket(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom);


	void handleVoicePacket(const unsigned char* data, int size, const asio::ip::udp::endpoint& endpointFrom);
	void handlePingSuccess(const asio::ip::udp::endpoint& endpointFrom);
	void handlePing(const asio::ip::udp::endpoint& endpointFrom);
	void broadcastPing();
	void checkPing();
	void redirectPacket(const nlohmann::json& jsonObject, PacketType type);

private:
	std::mutex m_endpointToUserMutex;
	std::mutex m_pingResultsMutex;
	std::mutex m_nicknameHashToUserAndCallsMutex;
	std::atomic_bool m_running = false;
	std::unordered_map<asio::ip::udp::endpoint, bool> m_pingResults;
	std::unordered_map<asio::ip::udp::endpoint, UserPtr> m_endpointToUser;
	std::unordered_map<std::string, UserPtr> m_nicknameHashToUser;
	std::unordered_set<std::shared_ptr<Call>> m_calls;
	NetworkController m_networkController;
};