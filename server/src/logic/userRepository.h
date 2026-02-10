#pragma once

#include <unordered_map>
#include <mutex>
#include <memory>
#include <string>
#include "models/user.h"
#include "network/tcp/connection.h"
#include "asio.hpp"

namespace server::logic 
{
	class UserRepository {
	public:
		UserRepository() = default;
		~UserRepository() = default;

		UserPtr findUserByNickname(const std::string& nicknameHash);
		UserPtr findUserByEndpoint(const asio::ip::udp::endpoint& endpoint);
		UserPtr findUserByTcpConnection(std::shared_ptr<network::tcp::Connection> conn);

		void addUser(UserPtr user);
		void removeUser(const std::string& nicknameHash);
		bool containsUser(const std::string& nicknameHash) const;
		bool containsUser(const asio::ip::udp::endpoint& endpoint) const;
		void updateUserUdpEndpoint(const std::string& nicknameHash, const asio::ip::udp::endpoint& newEndpoint);

	private:
		mutable std::mutex m_mutex;
		std::unordered_map<asio::ip::udp::endpoint, UserPtr> m_endpointToUser;
		std::unordered_map<std::string, UserPtr> m_nicknameHashToUser;
	};
}
