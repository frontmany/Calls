#pragma once

#include <string>
#include <memory>
#include <chrono>
#include <functional>

#include "utilities/crypto.h"
#include "ticTimer.h"

namespace core 
{
	class IncomingCall {
	public:
		template <typename Rep, typename Period>
		explicit IncomingCall(const std::string& nickname,
			const CryptoPP::RSA::PublicKey& publicKey,
			const CryptoPP::SecByteBlock& callKey,
			const std::chrono::duration<Rep, Period>& timeout,
			std::function<void()> onTimeout)
			: m_nickname(nickname)
			, m_publicKey(publicKey)
			, m_callKey(callKey)
			, m_timer()
		{
			m_timer.start(timeout, std::move(onTimeout));
		}
		IncomingCall(const IncomingCall& other) = delete;
		IncomingCall(IncomingCall&& other) noexcept;
		~IncomingCall() = default;

		const std::string& getNickname() const;
		const CryptoPP::RSA::PublicKey& getPublicKey() const;
		const CryptoPP::SecByteBlock& getCallKey() const;
		const tic::SingleShotTimer& getTimer() const;

	private:
		std::string m_nickname;
		CryptoPP::RSA::PublicKey m_publicKey;
		CryptoPP::SecByteBlock m_callKey;
		tic::SingleShotTimer m_timer;
	};
} 