#include "user.h"

namespace core
{
	User::User(const std::string& nickname, const CryptoPP::RSA::PublicKey& publicKey)
		: m_nickname(nickname)
		, m_publicKey(publicKey)
	{
	}

	bool User::isConnected() const {
		return m_connected;
	}

	const std::string& User::getNickname() const {
		return m_nickname;
	}

	const CryptoPP::PublicKey& User::getPublicKey() const {
		return m_publicKey;
	}

	void User::setConnectionDown(bool value) {
		m_connected = value;
	}
}