#pragma once

#include <vector>
#include <cstdint>
#include "aes.h"

namespace core::media
{
    class MediaEncryptionService {
    public:
        MediaEncryptionService() = default;
        ~MediaEncryptionService() = default;

        std::vector<unsigned char> encryptMedia(
            const unsigned char* data,
            int size,
            const CryptoPP::SecByteBlock& key);

        std::vector<unsigned char> decryptMedia(
            const unsigned char* data,
            int size,
            const CryptoPP::SecByteBlock& key);
    };
}
