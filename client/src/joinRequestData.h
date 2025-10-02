#pragma once

#include <string>

#include "crypto.h"

namespace calls {

	struct JoinRequestData {
		std::string friendNickname;
		CryptoPP::RSA::PublicKey friendPublicKey;
	};

}