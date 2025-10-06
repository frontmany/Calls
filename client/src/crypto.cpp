#include "crypto.h"
#include <stdexcept>
#include <sstream>
#include <cstring>
#include <iomanip>

namespace calls { namespace crypto {
    void generateRSAKeyPair(CryptoPP::RSA::PrivateKey& privateKey, CryptoPP::RSA::PublicKey& publicKey) {
        CryptoPP::AutoSeededRandomPool rng;
        privateKey.Initialize(rng, 3072);
        publicKey = CryptoPP::RSA::PublicKey(privateKey);
    }

    void generateAESKey(CryptoPP::SecByteBlock& key) {
        CryptoPP::AutoSeededRandomPool rng;
        key = CryptoPP::SecByteBlock(32);
        rng.GenerateBlock(key, key.size());
    }

    std::string RSAEncryptAESKey(const CryptoPP::RSA::PublicKey& publicKey, const CryptoPP::SecByteBlock& data) {
        try {
            CryptoPP::AutoSeededRandomPool rng;
            CryptoPP::RSAES<CryptoPP::OAEP<CryptoPP::SHA256>>::Encryptor encryptor(publicKey);

            std::string cipher;
            CryptoPP::StringSource(
                data.data(),
                data.size(),
                true,
                new CryptoPP::PK_EncryptorFilter(
                    rng,
                    encryptor,
                    new CryptoPP::StringSink(cipher)
                )
            );

            std::string base64Cipher;
            CryptoPP::StringSource(
                cipher,
                true,
                new CryptoPP::Base64Encoder(
                    new CryptoPP::StringSink(base64Cipher),
                    false
                )
            );

            return base64Cipher;
        }
        catch (const CryptoPP::Exception& e) {
            throw std::runtime_error(std::string("RSA encryption error: ") + e.what());
        }
    }

    CryptoPP::SecByteBlock RSADecryptAESKey(const CryptoPP::RSA::PrivateKey& privateKey, const std::string& base64Cipher) {
        try {
            CryptoPP::AutoSeededRandomPool rng;
            CryptoPP::RSAES<CryptoPP::OAEP<CryptoPP::SHA256>>::Decryptor decryptor(privateKey);

            std::string cipher;
            CryptoPP::StringSource(
                base64Cipher,
                true,
                new CryptoPP::Base64Decoder(
                    new CryptoPP::StringSink(cipher)
                )
            );

            if (cipher.size() != privateKey.GetModulus().ByteCount()) {
                throw std::runtime_error("Invalid cipher size after Base64 decoding");
            }

            CryptoPP::SecByteBlock decrypted(decryptor.MaxPlaintextLength(cipher.size()));
            CryptoPP::DecodingResult result = decryptor.Decrypt(
                rng,
                reinterpret_cast<const CryptoPP::byte*>(cipher.data()),
                cipher.size(),
                decrypted.data()
            );

            if (!result.isValidCoding) {
                throw std::runtime_error("Failed to decrypt RSA data");
            }

            decrypted.resize(result.messageLength);
            return decrypted;
        }
        catch (const CryptoPP::Exception& e) {
            throw std::runtime_error(std::string("RSA decryption error: ") + e.what());
        }
    }

    void AESEncrypt(const CryptoPP::SecByteBlock& key, const unsigned char* data, int length, CryptoPP::byte* out_ciphertext, size_t out_len) {
        if (out_len < length + CryptoPP::AES::BLOCKSIZE) {
            throw std::invalid_argument("Insufficient output buffer size");
        }

        CryptoPP::AutoSeededRandomPool prng;
        CryptoPP::byte iv[CryptoPP::AES::BLOCKSIZE];
        prng.GenerateBlock(iv, sizeof(iv));

        std::memcpy(out_ciphertext, iv, CryptoPP::AES::BLOCKSIZE);

        CryptoPP::CTR_Mode<CryptoPP::AES>::Encryption encryptor;
        encryptor.SetKeyWithIV(key, key.size(), iv);

        encryptor.ProcessData(out_ciphertext + CryptoPP::AES::BLOCKSIZE, data, length);
    }

    void AESDecrypt(const CryptoPP::SecByteBlock& key, const unsigned char* data, int length, CryptoPP::byte* out, size_t out_len) {
        if (length < CryptoPP::AES::BLOCKSIZE || out_len < static_cast<size_t>(length) - CryptoPP::AES::BLOCKSIZE) {
            throw std::invalid_argument("Invalid input or output buffer size");
        }

        const CryptoPP::byte* iv = data;
        CryptoPP::CTR_Mode<CryptoPP::AES>::Decryption decryptor;
        decryptor.SetKeyWithIV(key, key.size(), iv);

        decryptor.ProcessData(out, data + CryptoPP::AES::BLOCKSIZE, length - CryptoPP::AES::BLOCKSIZE);
    }

    std::string AESEncrypt(const CryptoPP::SecByteBlock& key, const std::string& plain) {
        CryptoPP::AutoSeededRandomPool rng;

        const size_t ivSize = 12;
        CryptoPP::byte iv[ivSize];
        rng.GenerateBlock(iv, ivSize);

        CryptoPP::GCM<CryptoPP::AES>::Encryption enc;
        enc.SetKeyWithIV(key, key.size(), iv, ivSize);

        std::string cipher;
        CryptoPP::AuthenticatedEncryptionFilter ef(enc, new CryptoPP::StringSink(cipher));
        CryptoPP::StringSource ss(plain, true, new CryptoPP::Redirector(ef));

        std::string binaryResult;
        binaryResult.assign(reinterpret_cast<const char*>(iv), ivSize);
        binaryResult += cipher;

        std::string base64Result;
        CryptoPP::StringSource ss2(
            binaryResult, true,
            new CryptoPP::Base64Encoder(
                new CryptoPP::StringSink(base64Result),
                false
            )
        );

        return base64Result;
    }

    std::string AESDecrypt(const CryptoPP::SecByteBlock& key, const std::string& cipher) {
        std::string cipherDecoded;
        CryptoPP::StringSource ss1(
            cipher, true,
            new CryptoPP::Base64Decoder(
                new CryptoPP::StringSink(cipherDecoded)
            )
        );

        const size_t ivSize = 12;
        if (cipherDecoded.size() < ivSize + 16 + 1)
            throw std::runtime_error("Invalid ciphertext");

        CryptoPP::byte iv[ivSize];
        std::memcpy(iv, cipherDecoded.data(), ivSize);

        std::string encrypted = cipherDecoded.substr(ivSize);

        CryptoPP::GCM<CryptoPP::AES>::Decryption dec;
        dec.SetKeyWithIV(key, key.size(), iv, ivSize);

        std::string plain;
        CryptoPP::AuthenticatedDecryptionFilter df(dec, new CryptoPP::StringSink(plain));

        try {
            CryptoPP::StringSource ss2(encrypted, true, new CryptoPP::Redirector(df));
        }
        catch (const CryptoPP::Exception& e) {
            throw std::runtime_error("Decryption failed: " + std::string(e.what()));
        }

        return plain;
    }

    std::string serializePublicKey(const CryptoPP::RSA::PublicKey& key) {
        std::string encoded;
        CryptoPP::ByteQueue queue;
        key.Save(queue);

        CryptoPP::StringSink sink(encoded);
        CryptoPP::Base64Encoder encoder(new CryptoPP::Redirector(sink), false);
        queue.CopyTo(encoder);
        encoder.MessageEnd();

        return encoded;
    }

    CryptoPP::RSA::PublicKey deserializePublicKey(const std::string& keyStr) {
        try {
            CryptoPP::ByteQueue queue;
            CryptoPP::StringSource ss(keyStr, true,
                new CryptoPP::Base64Decoder(
                    new CryptoPP::Redirector(queue)
                ));

            CryptoPP::RSA::PublicKey key;
            key.Load(queue);

            return key;
        }
        catch (const CryptoPP::Exception& e) {
            throw std::runtime_error("Failed to deserialize public key: " + std::string(e.what()));
        }
    }

    std::string serializeAESKey(const CryptoPP::SecByteBlock& key) {
        std::string encoded;

        CryptoPP::StringSource ss(
            key.data(),
            key.size(),
            true,
            new CryptoPP::Base64Encoder(
                new CryptoPP::StringSink(encoded),
                false
            )
        );

        return encoded;
    }

    CryptoPP::SecByteBlock deserializeAESKey(const std::string& keyStr) {
        try {
            std::string decoded;

            CryptoPP::StringSource ss(
                keyStr,
                true,
                new CryptoPP::Base64Decoder(
                    new CryptoPP::StringSink(decoded)
                )
            );

            CryptoPP::SecByteBlock key(
                reinterpret_cast<const CryptoPP::byte*>(decoded.data()),
                decoded.size()
            );

            return key;
        }
        catch (const CryptoPP::Exception& e) {
            throw std::runtime_error("Failed to deserialize AES key: " + std::string(e.what()));
        }
    }

    std::string calculateHash(const std::string& text) {
        CryptoPP::SHA256 hash;

        std::string digest;
        CryptoPP::StringSource ss(
            text,
            true,
            new CryptoPP::HashFilter(
                hash,
                new CryptoPP::HexEncoder(
                    new CryptoPP::StringSink(digest)
                )
            )
        );

        return digest;
    }

    std::string generateUUID() {
        try {
            CryptoPP::AutoSeededRandomPool rng;
            CryptoPP::byte uuid[16];
            rng.GenerateBlock(uuid, sizeof(uuid));

            uuid[6] = (uuid[6] & 0x0F) | 0x40; 
            uuid[8] = (uuid[8] & 0x3F) | 0x80;

            std::stringstream ss;
            ss << std::hex << std::setfill('0');

            for (int i = 0; i < 16; ++i) {
                if (i == 4 || i == 6 || i == 8 || i == 10) {
                    ss << '-';
                }
                ss << std::setw(2) << static_cast<unsigned int>(uuid[i]);
            }

            return ss.str();
        }
        catch (const std::exception& e) {
            throw std::runtime_error(std::string("UUID generation error: ") + e.what());
        }
    }
}}