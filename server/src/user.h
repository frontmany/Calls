#pragma once
#include <optional>
#include <memory>
#include <unordered_set>
#include <chrono>

#include "callRole.h"
#include "groupCallRole.h"

#include "crypto.h"
#include "asio.hpp"

class Call;
class GroupCall;

using CallPtr = std::shared_ptr<Call>;
using GroupCallPtr = std::shared_ptr<GroupCall>;

class User {
public:
	User(const std::string& nicknameHash, const CryptoPP::RSA::PublicKey& publicKey, asio::ip::udp::endpoint endpoint);
	const CryptoPP::RSA::PublicKey& getPublicKey() const;
	const std::string& getNicknameHash() const;
	const asio::ip::udp::endpoint& getEndpoint() const;

	void setCall(CallPtr callPtr, CallRole role);
	bool inCall() const;
	CallPtr getCall();
	CallRole getCallRole();
	void resetCall();
	const std::string& inCallWith() const;

	void setGroupCall(GroupCallPtr groupCall, GroupCallRole role);
	bool inGroupCall() const;
	GroupCallPtr getGroupCall();
	GroupCallRole getGroupCallRole();
	void resetGroupCall();
	std::unordered_set<std::string> inGroupCallWith() const;

private:
	std::string m_nicknameHash;
	CryptoPP::RSA::PublicKey m_publicKey;
	asio::ip::udp::endpoint m_endpoint;

	CallPtr m_call;
	CallRole m_callRole;

	GroupCallPtr m_groupCall;
	GroupCallRole m_groupCallRole;
};