#pragma once

#include <string>

#include "crypto.h"

struct IncomingCallData {
	std::string friendNickname;
	CryptoPP::RSA::PublicKey m_friendPublicKey;
	CryptoPP::SecByteBlock m_callKey;
};