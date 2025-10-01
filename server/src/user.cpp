#include "user.h"
#include "call.h"
#include "groupCall.h"

User::User(const std::string& nicknameHash, const CryptoPP::RSA::PublicKey& publicKey, asio::ip::udp::endpoint endpoint)
	: m_nicknameHash(nicknameHash), m_publicKey(publicKey), m_endpoint(endpoint), m_callRole(CallRole::EMPTY), m_groupCallRole(GroupCallRole::EMPTY)
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

bool User::inCall() const {
	if (m_call) {
		return true;
	}
	else {
		return false;
	}
}

bool User::inGroupCall() const {
	if (m_groupCall) {
		return true;
	}
	else {
		return false;
	}
}

CallPtr User::getCall() {
	return m_call;
}

CallRole User::getCallRole() {
	return m_callRole;
}

GroupCallPtr User::getGroupCall() {
	return m_groupCall;
}

GroupCallRole User::getGroupCallRole() {
	return m_groupCallRole;
}

void User::resetCall() {
	m_call = nullptr;
	m_callRole = CallRole::EMPTY;
}

void User::resetGroupCall() {
	m_groupCall = nullptr;
	m_groupCallRole = GroupCallRole::EMPTY;
}

const std::string& User::inCallWith() const {
	if (!m_call) return "";

	if (m_callRole == CallRole::RESPONDER) {
		return m_call->getInitiatorNicknameHash();
	}
	else {
		return m_call->getResponderNicknameHash();
	}
}

std::unordered_set<std::string> User::inGroupCallWith() const {
	if (!m_groupCall) return {};

	std::unordered_set<std::string> participantsWithoutMe;
	participantsWithoutMe.reserve(m_groupCall->getParticipants().size() - 1);

	const std::unordered_set<std::string>& participants = m_groupCall->getParticipants();
	for (auto& participant : participants) {
		if (participant != m_nicknameHash) {
			participantsWithoutMe.insert(participant);
		}
	}

	return participantsWithoutMe;
}

void User::setCall(CallPtr call, CallRole role) {
	m_callRole = role;
	m_call = call;
}

void User::setGroupCall(GroupCallPtr groupCall, GroupCallRole role) {
	m_groupCall = groupCall;
	m_groupCallRole = role;
}