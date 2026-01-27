#pragma once

#include <vector>
#include <cstdint>
#include "aes.h"

namespace core
{
    namespace services
    {
        // Интерфейс для шифрования и дешифрования медиа-данных
        class IMediaEncryptionService {
        public:
            virtual ~IMediaEncryptionService() = default;

            // Шифрование медиа-данных
            virtual std::vector<unsigned char> encryptMedia(
                const unsigned char* data,
                int size,
                const CryptoPP::SecByteBlock& key) = 0;

            // Дешифрование медиа-данных
            virtual std::vector<unsigned char> decryptMedia(
                const unsigned char* data,
                int size,
                const CryptoPP::SecByteBlock& key) = 0;
        };
    }
}
