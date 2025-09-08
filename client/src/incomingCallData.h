#pragma once

#include <string>

#include "crypto.h"

struct IncomingCallData {
	std::string friendNickname;
	CryptoPP::RSA::PublicKey friendPublicKey;
	CryptoPP::SecByteBlock callKey;
};