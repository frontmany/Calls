#include "groupCall.h"

namespace calls {
	GroupCall::GroupCall(const std::string& initiatorNickname, const CryptoPP::RSA::PublicKey& initiatorPublicKey, const std::string& groupCallName)
		: m_groupCallName(groupCallName), m_initiatorNickname(initiatorNickname), m_initiatorPublicKey(initiatorPublicKey)
	{
		m_participants.emplace(initiatorNickname, initiatorPublicKey);
	}

	void GroupCall::createGroupCallKey() {
		crypto::generateAESKey(m_groupCallKey);
	}

	const CryptoPP::SecByteBlock& GroupCall::getGroupCallKey() const {
		return m_groupCallKey;
	}

	const std::string& GroupCall::getGroupCallName() const {
		return m_groupCallName;
	}

	const std::string& GroupCall::getInitiatorNickname() const {
		return m_initiatorNickname;
	}

	const CryptoPP::RSA::PublicKey& GroupCall::getInitiatorPublicKey() const {
		return m_initiatorPublicKey;
	}

	const std::unordered_map<std::string, CryptoPP::RSA::PublicKey>& GroupCall::getParticipants() {
		return m_participants;
	}

	void GroupCall::addParticipant(const std::string& nickname, const CryptoPP::RSA::PublicKey& publicKey) {
		m_participants.emplace(nickname, publicKey);
	}

	void GroupCall::removeParticipant(const std::string& nickname) {
		m_participants.erase(nickname);
	}
}
