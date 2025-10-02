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
#include "groupCall.h"
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
	void onUserDisconnected(const asio::ip::udp::endpoint& endpointFrom);

	void handleAuthorizationPacket(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom);
	void handleGetFriendInfoPacket(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom);
	void handleCreateCallPacket(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom);
	void handleCreateGroupCallPacket(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom);
	void handleGroupCallCheckPacket(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom);
	void handleJoinRequestPacket(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom);
	void handleJoinAllowedPacket(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom);
	void handleGroupCallNewParticipantPacket(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom);
	void handleJoinDeclinedPacket(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom);
	void handleJoinRequestCanceledByRequesterSidePacket(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom);
	void handleLeaveGroupCallPacket(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom);
	void handleCallingEndPacket(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom);
	void handleEndCallPacket(const asio::ip::udp::endpoint& endpointFrom);
	void handleEndGroupCallPacket(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom);
	void handleCallAcceptedPacket(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom);
	void handleCallDeclinedPacket(const nlohmann::json& jsonObject, const asio::ip::udp::endpoint& endpointFrom);
	void handleVoicePacket(const unsigned char* data, int size, const asio::ip::udp::endpoint& endpointFrom);
	void handleLogout(const asio::ip::udp::endpoint& endpointFrom);
	void handlePingSuccess(const asio::ip::udp::endpoint& endpointFrom);

	void broadcastPing();
	void checkPing();

private:
	std::mutex m_endpointToUserMutex;
	std::mutex m_pingResultsMutex;
	std::atomic_bool m_running = false;
	std::unordered_map<asio::ip::udp::endpoint, bool> m_pingResults;
	std::unordered_map<asio::ip::udp::endpoint, UserPtr> m_endpointToUser;
	std::unordered_map<std::string, UserPtr> m_nicknameHashToUser;
	std::unordered_set<CallPtr> m_calls;
	std::unordered_map<std::string, GroupCallPtr> m_groupCalls;
	NetworkController m_networkController;

	static constexpr const char* PUBLIC_KEY = "publicKey";
	static constexpr const char* NICKNAME_HASH = "nicknameHash";
	static constexpr const char* NICKNAME_HASH_TO = "nicknameHashTo";
	static constexpr const char* GROUP_CALL_NAME_HASH = "groupCallNameHash";
};