#include "crypto.h"
#include <stdexcept>
#include <cstring>

namespace crypto {
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
}