#pragma once

#include <memory>
#include <string>
#include <functional>
#include <vector>
#include <mutex>

#include "utilities/crypto.h"
#include "ticTimer.h"
#include "asio.hpp"
#include "network/tcp/connection.h"

namespace server
{
    namespace network { using TcpConnection = tcp::Connection; }

    class Call;
    class PendingCall;
    class User;
    typedef std::shared_ptr<Call> CallPtr;
    typedef std::shared_ptr<PendingCall> PendingCallPtr;
    typedef std::shared_ptr<User> UserPtr;

    class User {
public:
	User(const std::string& nicknameHash, const std::string& token,
		const CryptoPP::RSA::PublicKey& publicKey,
		asio::ip::udp::endpoint endpoint,
		std::function<void()> onReconnectionTimeout
	);

	bool isInCall() const;
	bool isPendingCall() const;
	bool hasOutgoingPendingCall() const;
	bool hasIncomingPendingCall(const PendingCallPtr& pendingCall) const;
	bool isConnectionDown();
	
	const CryptoPP::RSA::PublicKey& getPublicKey() const;
	const std::string& getNicknameHash() const;
	const std::string& getToken() const;
	const asio::ip::udp::endpoint& getEndpoint() const;
	CallPtr getCall() const;
	PendingCallPtr getOutgoingPendingCall() const;
	UserPtr getCallPartner() const;
	UserPtr getOutgoingPendingCallPartner() const;
	std::vector<PendingCallPtr> getIncomingPendingCalls() const;

	void setConnectionDown(bool value);
	void setEndpoint(asio::ip::udp::endpoint endpoint);
	void setTcpConnection(std::shared_ptr<network::TcpConnection> conn);
	std::shared_ptr<network::TcpConnection> getTcpConnection() const;
	void clearTcpConnection();
	void setCall(CallPtr call);
	void setOutgoingPendingCall(PendingCallPtr pendingCall);
	void addIncomingPendingCall(PendingCallPtr pendingCall);
	void resetCall();
	void resetOutgoingPendingCall();
	void removeIncomingPendingCall(PendingCallPtr pendingCall);
	void resetAllPendingCalls();

private:
	mutable std::mutex m_mutex;
	bool m_connectionDown = false;
	std::string m_nicknameHash;
	std::string m_token;
	std::weak_ptr<Call> m_call;
	std::weak_ptr<PendingCall> m_outgoingPendingCall;
	std::vector<std::weak_ptr<PendingCall>> m_incomingPendingCalls;
	CryptoPP::RSA::PublicKey m_publicKey;
	asio::ip::udp::endpoint m_endpoint;
	std::weak_ptr<network::TcpConnection> m_tcpConnection;

	std::function<void()> m_onReconnectionTimeout;
	tic::SingleShotTimer m_reconnectionTimeoutTimer;
    };
}