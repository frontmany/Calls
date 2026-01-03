#include "user.h"
#include "call.h"
#include "pendingCall.h"

#include <chrono>
#include <vector>

namespace callifornia
{
User::User(const std::string& nicknameHash, const std::string& token, const CryptoPP::RSA::PublicKey& publicKey, asio::ip::udp::endpoint endpoint, std::function<void()> onReconnectionTimeout)
	: m_nicknameHash(nicknameHash), m_token(token), m_publicKey(publicKey), m_endpoint(endpoint), m_onReconnectionTimeout(std::move(onReconnectionTimeout))
{
}

bool User::isConnectionDown()
{
	return m_connectionDown;
}

void User::setConnectionDown(bool value)
{
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

const CryptoPP::RSA::PublicKey& User::getPublicKey() const
{
	return m_publicKey;
}

const std::string& User::getNicknameHash() const
{
	return m_nicknameHash;
}

const std::string& User::getToken() const
{
	return m_token;
}

const asio::ip::udp::endpoint& User::getEndpoint() const
{
	return m_endpoint;
}

bool User::isInCall() const
{
	return !m_call.expired();
}

bool User::isPendingCall() const
{
	return !m_outgoingPendingCall.expired() || !m_incomingPendingCalls.empty();
}

bool User::hasOutgoingPendingCall() const
{
	return !m_outgoingPendingCall.expired();
}

bool User::hasIncomingPendingCall(const PendingCallPtr& pendingCall) const
{
	for (const auto& weakPtr : m_incomingPendingCalls) {
		if (auto ptr = weakPtr.lock()) {
			if (ptr == pendingCall) {
				return true;
			}
		}
	}
	return false;
}

CallPtr User::getCall() const
{
	return m_call.lock();
}

PendingCallPtr User::getOutgoingPendingCall() const
{
	return m_outgoingPendingCall.lock();
}

UserPtr User::getCallPartner() const
{
	auto call = m_call.lock();

	if (call) {
		if (m_nicknameHash == call->getInitiator()->getNicknameHash())
			return call->getResponder();
		else
			return call->getInitiator();
	}

	return nullptr;
}

UserPtr User::getOutgoingPendingCallPartner() const
{
	auto pendingCall = m_outgoingPendingCall.lock();

	if (pendingCall) {
		if (m_nicknameHash == pendingCall->getInitiator()->getNicknameHash())
			return pendingCall->getReceiver();
		else
			return pendingCall->getInitiator();
	}

	return nullptr;
}

std::vector<PendingCallPtr> User::getIncomingPendingCalls() const
{
	std::vector<PendingCallPtr> result;
	
	for (const auto& weakPtr : m_incomingPendingCalls) {
		if (auto ptr = weakPtr.lock()) {
			result.push_back(ptr);
		}
	}
	
	return result;
}

void User::setCall(CallPtr call)
{
	m_call = call;
}

void User::setOutgoingPendingCall(PendingCallPtr pendingCall)
{
	m_outgoingPendingCall = pendingCall;
}

void User::addIncomingPendingCall(PendingCallPtr pendingCall)
{
	// Check if already exists
	for (const auto& weakPtr : m_incomingPendingCalls) {
		if (auto ptr = weakPtr.lock()) {
			if (ptr == pendingCall) {
				return; // Already exists
			}
		}
	}
	m_incomingPendingCalls.push_back(pendingCall);
}

void User::resetCall()
{
	m_call.reset();
}

void User::resetOutgoingPendingCall()
{
	m_outgoingPendingCall.reset();
}

void User::removeIncomingPendingCall(PendingCallPtr pendingCall)
{
	for (auto it = m_incomingPendingCalls.begin(); it != m_incomingPendingCalls.end(); ) {
		if (auto ptr = it->lock()) {
			if (ptr == pendingCall) {
				it = m_incomingPendingCalls.erase(it);
				return;
			} else {
				++it;
			}
		} else {
			// Удаляем expired weak_ptr
			it = m_incomingPendingCalls.erase(it);
		}
	}
}

void User::resetAllPendingCalls()
{
	m_outgoingPendingCall.reset();
	m_incomingPendingCalls.clear();
}
} // namespace callifornia