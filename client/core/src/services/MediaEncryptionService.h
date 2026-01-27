#pragma once

#include "IMediaEncryptionService.h"
#include "utilities/crypto.h"
#include "utilities/logger.h"
#include <vector>
#include <cstdint>
#include "aes.h"

namespace core
{
    namespace services
    {
        // Сервис для шифрования и дешифрования медиа-данных (voice, screen, camera)
        class MediaEncryptionService : public IMediaEncryptionService {
        public:
            MediaEncryptionService() = default;
            ~MediaEncryptionService() = default;

            std::vector<unsigned char> encryptMedia(
                const unsigned char* data,
                int size,
                const CryptoPP::SecByteBlock& key) override;

            std::vector<unsigned char> decryptMedia(
                const unsigned char* data,
                int size,
                const CryptoPP::SecByteBlock& key) override;
        };
    }
}
