#pragma once

#include <string>
#include "rsa.h" 
#include "hex.h" 
#include "aes.h"
#include "gcm.h"
#include "osrng.h"
#include "base64.h"  

namespace crypto {
    std::string serializePublicKey(const CryptoPP::RSA::PublicKey& key);
    CryptoPP::RSA::PublicKey deserializePublicKey(const std::string& keyStr);
    std::string calculateHash(const std::string& text);
}