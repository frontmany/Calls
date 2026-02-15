#include "logic/userRepository.h"

namespace server::logic
{
	UserPtr UserRepository::findUserByNickname(const std::string& nicknameHash) {
		std::lock_guard<std::mutex> lock(m_mutex);
		auto it = m_nicknameHashToUser.find(nicknameHash);
		if (it != m_nicknameHashToUser.end()) {
			return it->second;
		}
		return nullptr;
	}

	UserPtr UserRepository::findUserByEndpoint(const asio::ip::udp::endpoint& endpoint) {
		std::lock_guard<std::mutex> lock(m_mutex);
		auto it = m_endpointToUser.find(endpoint);
		if (it != m_endpointToUser.end()) {
			return it->second;
		}
		return nullptr;
	}

	UserPtr UserRepository::findUserByTcpConnection(std::shared_ptr<network::tcp::Connection> conn) {
		if (!conn) return nullptr;
		std::lock_guard<std::mutex> lock(m_mutex);
		for (const auto& [_, user] : m_nicknameHashToUser) {
			if (user->getTcpConnection() == conn)
				return user;
		}
		return nullptr;
	}

	void UserRepository::addUser(UserPtr user) {
		if (!user) return;
		std::lock_guard<std::mutex> lock(m_mutex);
		m_nicknameHashToUser[user->getNicknameHash()] = user;
		auto ep = user->getEndpoint();
		if (ep.port() != 0)
			m_endpointToUser[ep] = user;
	}

	void UserRepository::removeUser(const std::string& nicknameHash) {
		std::lock_guard<std::mutex> lock(m_mutex);
		auto nicknameIt = m_nicknameHashToUser.find(nicknameHash);
		if (nicknameIt != m_nicknameHashToUser.end()) {
			auto endpoint = nicknameIt->second->getEndpoint();
			if (endpoint.port() != 0)
				m_endpointToUser.erase(endpoint);
			m_nicknameHashToUser.erase(nicknameIt);
		}
	}

	bool UserRepository::containsUser(const std::string& nicknameHash) const {
		std::lock_guard<std::mutex> lock(m_mutex);
		return m_nicknameHashToUser.find(nicknameHash) != m_nicknameHashToUser.end();
	}

	bool UserRepository::containsUser(const asio::ip::udp::endpoint& endpoint) const {
		std::lock_guard<std::mutex> lock(m_mutex);
		return m_endpointToUser.find(endpoint) != m_endpointToUser.end();
	}

	void UserRepository::updateUserUdpEndpoint(const std::string& nicknameHash,
		const asio::ip::udp::endpoint& newEndpoint) {
		std::lock_guard<std::mutex> lock(m_mutex);
		auto it = m_nicknameHashToUser.find(nicknameHash);
		if (it != m_nicknameHashToUser.end()) {
			auto oldEndpoint = it->second->getEndpoint();
			if (oldEndpoint.port() != 0)
				m_endpointToUser.erase(oldEndpoint);
			it->second->setEndpoint(newEndpoint);
			if (newEndpoint.port() != 0)
				m_endpointToUser[newEndpoint] = it->second;
		}
	}
}
