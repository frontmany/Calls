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
	}

	void UserRepository::removeUser(const std::string& nicknameHash) {
		std::lock_guard<std::mutex> lock(m_mutex);
		m_nicknameHashToUser.erase(nicknameHash);
	}

	bool UserRepository::containsUser(const std::string& nicknameHash) const {
		std::lock_guard<std::mutex> lock(m_mutex);
		return m_nicknameHashToUser.find(nicknameHash) != m_nicknameHashToUser.end();
	}

	void UserRepository::updateUserUdpEndpoint(const std::string& nicknameHash,
		const asio::ip::udp::endpoint& newEndpoint) {
		std::lock_guard<std::mutex> lock(m_mutex);
		auto it = m_nicknameHashToUser.find(nicknameHash);
		if (it != m_nicknameHashToUser.end()) {
			it->second->setEndpoint(newEndpoint);
		}
	}

	size_t UserRepository::getActiveUsersCount() const {
		std::lock_guard<std::mutex> lock(m_mutex);
		size_t count = 0;
		for (const auto& [_, user] : m_nicknameHashToUser) {
			if (user && !user->isConnectionDown())
				++count;
		}
		return count;
	}
}
