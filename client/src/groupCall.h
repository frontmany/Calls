#pragma once
#include <unordered_map>
#include <string>

#include "crypto.h"

namespace calls 
{
	class GroupCall {
	public:
		enum GroupCallRole {
			INITIATOR,
			PARTICIPANT
		};

		GroupCall(const std::string& initiatorNickname, const CryptoPP::RSA::PublicKey& initiatorPublicKey, const std::string& groupCallName, GroupCallRole role);
		GroupCall(const std::unordered_map<std::string, CryptoPP::RSA::PublicKey>& participants, const CryptoPP::SecByteBlock& groupCallKey, const std::string& initiatorNickname, const CryptoPP::RSA::PublicKey& initiatorPublicKey, const std::string& groupCallName, GroupCallRole role);
		~GroupCall() = default;
		void addParticipant(const std::string& nickname, const CryptoPP::RSA::PublicKey& publicKey);
		void removeParticipant(const std::string& nickname);
		void createGroupCallKey();
		const CryptoPP::SecByteBlock& getGroupCallKey() const;
		const std::string& getGroupCallName() const;
		GroupCallRole getGroupCallRole() const;
		const std::string& getInitiatorNickname() const;
		const CryptoPP::RSA::PublicKey& getInitiatorPublicKey() const;
		const std::unordered_map<std::string, CryptoPP::RSA::PublicKey>& getParticipants();

	private:
		std::unordered_map<std::string, CryptoPP::RSA::PublicKey> m_participants;
		std::string m_initiatorNickname;
		CryptoPP::RSA::PublicKey m_initiatorPublicKey;
		std::string m_groupCallName;
		CryptoPP::SecByteBlock m_groupCallKey;
		GroupCallRole m_role;
	};
}