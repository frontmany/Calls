#include "groupCall.h"


GroupCall::GroupCall(const std::string& initiatorNicknameHash, const std::string& groupCallNameHash) 
	: m_initiatorNicknameHash(initiatorNicknameHash), m_groupCallNameHash(groupCallNameHash) 
{
	m_participants.insert(initiatorNicknameHash);
}

void GroupCall::addParticipant(const std::string& userNicknameHash) {
	m_participants.insert(userNicknameHash);
}

void GroupCall::removeParticipant(const std::string& userNicknameHashToRemove) {
	if (m_participants.contains(userNicknameHashToRemove)) {
		m_participants.erase(userNicknameHashToRemove);
	}
}

const std::unordered_set<std::string>& GroupCall::getParticipants() const {
	return m_participants;
}

const std::string& GroupCall::getGroupCallNameHash() const {
	return m_groupCallNameHash;
}

const std::string& GroupCall::getInitiatorNicknameHash() const {
	return m_initiatorNicknameHash;
}