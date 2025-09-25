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
	enum class PingResult {
		PING_SUCCESS,
		PING_FAIL,
		PING_LOSS_1,
		PING_LOSS_2,
		PING_LOSS_3
	};

	void onReceive(const unsigned char* data, int size, PacketType type, const asio::ip::udp::endpoint& endpointFrom);
	void onNetworkError();
	void onUserDisconnected(const asio::ip::udp::endpoint& endpointFrom);

	void ping();
	void checkPingResults();
	void handleAuthorizationPacket(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom);
	void handleGetFriendInfoPacket(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom);
	void handleCreateCallPacket(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom);
	void handleCallingEndPacket(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom);
	void handleEndCallPacket(const asio::ip::udp::endpoint& endpointFrom);
	void handleCallAcceptedPacket(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom);
	void handleCallDeclinedPacket(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom);
	void handleVoicePacket(const unsigned char* data, int size, const asio::ip::udp::endpoint& endpointFrom);
	void handleLogout(const asio::ip::udp::endpoint& endpointFrom);
	void handlePingSuccess(const asio::ip::udp::endpoint& endpointFrom);

private:
	std::atomic_bool m_running = false;
	std::mutex m_mutex;

	std::unordered_map<asio::ip::udp::endpoint, UserPtr> m_endpointToUser;
	std::unordered_map<std::string, UserPtr> m_nicknameHashToUser;
	std::unordered_set<std::shared_ptr<Call>> m_calls;
	NetworkController m_networkController;

	static constexpr const char* PUBLIC_KEY = "publicKey";
	static constexpr const char* NICKNAME_HASH = "nicknameHash";
	static constexpr const char* NICKNAME_HASH_TO = "nicknameHashTo";
};