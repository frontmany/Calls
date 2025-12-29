#pragma once

#include <string>
#include <memory>
#include <chrono>

#include "utilities/crypto.h"
#include "utilities/timer.h"

namespace calls 
{
	class IncomingCall {
	public:
		template <typename Rep, typename Period, typename Callable>
		explicit IncomingCall(const std::string& nickname,
			const CryptoPP::RSA::PublicKey& publicKey,
			const CryptoPP::SecByteBlock& callKey,
			const std::chrono::duration<Rep, Period>& timeout,
			Callable&& onTimeout)
			: m_nickname(nickname)
			, m_publicKey(publicKey)
			, m_callKey(callKey)
			, m_timer(std::make_unique<utilities::tic::SingleShotTimer>())
		{
			m_timer->start(timeout, std::forward<Callable>(onTimeout));
		}
		IncomingCall(const IncomingCall& other) = delete;
		IncomingCall(IncomingCall&& other) noexcept;
		~IncomingCall() = default;

		const std::string& getNickname() const;
		const CryptoPP::RSA::PublicKey& getPublicKey() const;
		const CryptoPP::SecByteBlock& getCallKey() const;
		const utilities::tic::SingleShotTimer& getTimer() const;

	private:
		std::string m_nickname;
		CryptoPP::RSA::PublicKey m_publicKey;
		CryptoPP::SecByteBlock m_callKey;
		utilities::tic::SingleShotTimer m_timer;
	};
} 