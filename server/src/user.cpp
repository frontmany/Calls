#include "user.h"
#include "call.h"

User::User(const std::string& nicknameHash, const CryptoPP::RSA::PublicKey& publicKey, asio::ip::udp::endpoint endpoint)
	: m_nicknameHash(nicknameHash), m_publicKey(publicKey), m_endpoint(endpoint)
{
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

void User::setCallAccepted(bool accepted) {
	if (!m_call) return;

	if (accepted) {
		m_call->setActive();
	}
	else {
		m_call->setEnded();
		m_call = nullptr;
		m_role = CallRole::EMPTY;
	}
}

bool User::isCall() const {
	if (m_call) {
		return true;
	}
	else {
		return false;
	}
}

std::shared_ptr<Call> User::getCall() {
	return m_call;
}

void User::resetCall() {
	m_call = nullptr;
	m_role = CallRole::EMPTY;
}

std::string User::inCallWith() const {
	if (!m_call) return "";

	if (m_role == CallRole::RESPONDER) {
		return m_call->getInitiatorNicknameHash();
	}
	else {
		return m_call->getResponderNicknameHash();
	}
}

void User::setCall(std::shared_ptr<Call> callPtr, CallRole role) {
	m_role = role;
	m_call = callPtr;
}