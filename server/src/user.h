#pragma once

#include <optional>
#include <memory>
#include <chrono>

#include "crypto.h"
#include "asio.hpp"

class Call;

class User {
public:
	enum class CallRole {
		EMPTY,
		INITIATOR,
		RESPONDER
	};

	User(const std::string& nicknameHash, const CryptoPP::RSA::PublicKey& publicKey, asio::ip::udp::endpoint endpoint);
	const CryptoPP::RSA::PublicKey& getPublicKey() const;
	const std::string& getNicknameHash() const;
	const asio::ip::udp::endpoint& getEndpoint() const;


	void setCall(std::shared_ptr<Call> callPtr, CallRole role);
	bool inCall() const;
	std::shared_ptr<Call> getCall();
	void resetCall();
	void setCallAccepted(bool accepted);
	std::string inCallWith() const;

private:
	std::string m_nicknameHash;
	CryptoPP::RSA::PublicKey m_publicKey;
	asio::ip::udp::endpoint m_endpoint;

	std::shared_ptr<Call> m_call;
	CallRole m_role = CallRole::EMPTY;
};