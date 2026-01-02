#pragma once

#include <memory>
#include <string>
#include <functional>

#include "utilities/crypto.h"
#include "utilities/timer.h"
#include "asio.hpp"

class Call;
typedef std::shared_ptr<Call> CallPtr;

class User {
public:
	User(const std::string& nicknameHash,
		const CryptoPP::RSA::PublicKey& publicKey,
		asio::ip::udp::endpoint endpoint,
		std::function<void()> onReconnectionTimeout
	);

	bool isInCall() const;
	bool isConnectionDown();
	
	const CryptoPP::RSA::PublicKey& getPublicKey() const;
	const std::string& getNicknameHash() const;
	const asio::ip::udp::endpoint& getEndpoint() const;
	CallPtr getCall() const;

	void setConnectionDown(bool value);
	void setCall(CallPtr call);
	void resetCall();

private:
	bool m_connectionDown = false;
	std::string m_nicknameHash;
	std::weak_ptr<Call> m_call;
	CryptoPP::RSA::PublicKey m_publicKey;
	asio::ip::udp::endpoint m_endpoint;

	std::function<void()> m_onReconnectionTimeout;
	utilities::tic::SingleShotTimer m_reconnectionTimeoutTimer;
};