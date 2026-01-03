#include "incomingCall.h"

namespace callifornia
{
	IncomingCall::IncomingCall(IncomingCall&& other) noexcept
		: m_nickname(std::move(other.m_nickname))
		, m_publicKey(std::move(other.m_publicKey))
		, m_callKey(std::move(other.m_callKey))
		, m_timer(std::move(other.m_timer))
	{
	}

	const std::string& IncomingCall::getNickname() const
	{
		return m_nickname;
	}

	const CryptoPP::RSA::PublicKey& IncomingCall::getPublicKey() const
	{
		return m_publicKey;
	}

	const CryptoPP::SecByteBlock& IncomingCall::getCallKey() const
	{
		return m_callKey;
	}

	const utilities::tic::SingleShotTimer& IncomingCall::getTimer() const
	{
		return m_timer;
	}
}