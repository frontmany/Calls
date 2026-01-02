#include "user.h"
#include "call.h"

#include <chrono>

User::User(const std::string& nicknameHash, const CryptoPP::RSA::PublicKey& publicKey, asio::ip::udp::endpoint endpoint, std::function<void()> onReconnectionTimeout)
	: m_nicknameHash(nicknameHash), m_publicKey(publicKey), m_endpoint(endpoint), m_onReconnectionTimeout(std::move(onReconnectionTimeout))
{
}

bool User::isConnectionDown() {
	return m_connectionDown;
}

void User::setConnectionDown(bool value) {
	m_connectionDown = value;
	
	if (value) {
		using namespace std::chrono_literals;
		m_reconnectionTimeoutTimer.start(2min, [this]() {
			m_onReconnectionTimeout();
		});
	}
	else {
		m_reconnectionTimeoutTimer.stop();
	}
}

const CryptoPP::RSA::PublicKey& User::getPublicKey() const {
	return m_publicKey;
}

const std::string& User::getNicknameHash() const {
	return m_nicknameHash;
}

const asio::ip::udp::endpoint& User::getEndpoint() const {
	return m_endpoint;
}

bool User::isInCall() const {
	return !m_call.expired();
}

CallPtr User::getCall() const {
	return m_call.lock();
}

void User::setCall(CallPtr call) {
	m_call = call;
}

void User::resetCall() {
	m_call.reset();
}