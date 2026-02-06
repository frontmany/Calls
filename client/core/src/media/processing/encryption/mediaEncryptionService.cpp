#include "mediaEncryptionService.h"
#include "utilities/crypto.h"
#include "utilities/logger.h"

using namespace core::utilities;

namespace core
{
    namespace media
    {
        std::vector<unsigned char> MediaEncryptionService::encryptMedia(
            const unsigned char* data,
            int size,
            const CryptoPP::SecByteBlock& key)
        {
            if (!data || size <= 0) {
                LOG_WARN("MediaEncryption::encryptMedia: invalid input");
                return {};
            }

            try {
                size_t cipherDataLength = static_cast<size_t>(size) + CryptoPP::AES::BLOCKSIZE;
                std::vector<unsigned char> cipherData(cipherDataLength);

                crypto::AESEncrypt(key,
                    data,
                    size,
                    reinterpret_cast<CryptoPP::byte*>(cipherData.data()),
                    cipherDataLength);

                return cipherData;
            }
            catch (const std::exception& e) {
                LOG_ERROR("MediaEncryption::encryptMedia failed: {}", e.what());
                return {};
            }
        }

        std::vector<unsigned char> MediaEncryptionService::decryptMedia(
            const unsigned char* data,
            int size,
            const CryptoPP::SecByteBlock& key)
        {
            if (!data || size <= 0) {
                LOG_WARN("MediaEncryption::decryptMedia: invalid input");
                return {};
            }

            if (size <= CryptoPP::AES::BLOCKSIZE) {
                LOG_WARN("MediaEncryption::decryptMedia: data too small to decrypt: {} bytes", size);
                return {};
            }

            try {
                size_t decryptedLength = static_cast<size_t>(size) - CryptoPP::AES::BLOCKSIZE;
                std::vector<unsigned char> decryptedData(decryptedLength);

                crypto::AESDecrypt(key,
                    data,
                    size,
                    reinterpret_cast<CryptoPP::byte*>(decryptedData.data()),
                    decryptedData.size());

                return decryptedData;
            }
            catch (const std::exception& e) {
                LOG_ERROR("MediaEncryption::decryptMedia failed: {}", e.what());
                return {};
            }
        }
    }
}
