#pragma once

#include <string>

#include "crypto.h"

namespace calls {

struct IncomingCallData {
	std::string friendNickname;
	CryptoPP::RSA::PublicKey friendPublicKey;
	CryptoPP::SecByteBlock callKey;
};

}