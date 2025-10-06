#pragma once

#include <string>
#include "rsa.h" 
#include "hex.h" 
#include "aes.h"
#include "gcm.h" 
#include "osrng.h"
#include "base64.h"  

namespace calls { namespace crypto {
    void generateRSAKeyPair(CryptoPP::RSA::PrivateKey& privateKey, CryptoPP::RSA::PublicKey& publicKey);
    std::string RSAEncryptAESKey(const CryptoPP::RSA::PublicKey& publicKey, const CryptoPP::SecByteBlock& data);
    CryptoPP::SecByteBlock RSADecryptAESKey(const CryptoPP::RSA::PrivateKey& privateKey, const std::string& base64Cipher);

    void generateAESKey(CryptoPP::SecByteBlock& key);
    void AESEncrypt(const CryptoPP::SecByteBlock& key, const unsigned char* data, int length, CryptoPP::byte* out_ciphertext, size_t out_len);
    void AESDecrypt(const CryptoPP::SecByteBlock& key, const unsigned char* data, int length, CryptoPP::byte* out_plaintext, size_t out_len);
    std::string AESEncrypt(const CryptoPP::SecByteBlock& key, const std::string& plain);
    std::string AESDecrypt(const CryptoPP::SecByteBlock& key, const std::string& cipher);

    std::string serializePublicKey(const CryptoPP::RSA::PublicKey& key);
    CryptoPP::RSA::PublicKey deserializePublicKey(const std::string& keyStr);
    std::string serializeAESKey(const CryptoPP::SecByteBlock& key);
    CryptoPP::SecByteBlock deserializeAESKey(const std::string& keyStr);

    std::string calculateHash(const std::string& text);
    std::string generateUUID();
}} 