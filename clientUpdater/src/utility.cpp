#include "utility.h"

#include <fstream>
#include <sstream>
#include <iomanip>

namespace updater {

std::string calculateFileHash(std::filesystem::path filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        return "";
    }

    uint32_t h0 = 0x67452301;
    uint32_t h1 = 0xEFCDAB89;
    uint32_t h2 = 0x98BADCFE;
    uint32_t h3 = 0x10325476;
    uint32_t h4 = 0xC3D2E1F0;

    uint64_t totalLength = 0;
    const size_t CHUNK_SIZE = 64;

    while (!file.eof()) {
        char buffer[CHUNK_SIZE] = { 0 };
        file.read(buffer, CHUNK_SIZE);
        size_t bytesRead = file.gcount();

        if (bytesRead == 0) break;

        totalLength += bytesRead * 8;

        if (bytesRead < CHUNK_SIZE) {
            buffer[bytesRead] = 0x80;
            for (size_t i = bytesRead + 1; i < CHUNK_SIZE - 8; i++) {
                buffer[i] = 0x00;
            }

            uint64_t bitLength = totalLength;
            for (int i = 0; i < 8; i++) {
                buffer[CHUNK_SIZE - 1 - i] = (bitLength >> (i * 8)) & 0xFF;
            }
        }

        uint32_t w[80] = { 0 };
        for (int i = 0; i < 16; i++) {
            w[i] = (static_cast<uint32_t>(buffer[i * 4] & 0xFF) << 24) |
                (static_cast<uint32_t>(buffer[i * 4 + 1] & 0xFF) << 16) |
                (static_cast<uint32_t>(buffer[i * 4 + 2] & 0xFF) << 8) |
                (static_cast<uint32_t>(buffer[i * 4 + 3] & 0xFF));
        }

        for (int i = 16; i < 80; i++) {
            w[i] = w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16];
            w[i] = (w[i] << 1) | (w[i] >> 31);
        }

        uint32_t a = h0;
        uint32_t b = h1;
        uint32_t c = h2;
        uint32_t d = h3;
        uint32_t e = h4;

        for (int i = 0; i < 80; i++) {
            uint32_t f, k;

            if (i < 20) {
                f = (b & c) | ((~b) & d);
                k = 0x5A827999;
            }
            else if (i < 40) {
                f = b ^ c ^ d;
                k = 0x6ED9EBA1;
            }
            else if (i < 60) {
                f = (b & c) | (b & d) | (c & d);
                k = 0x8F1BBCDC;
            }
            else {
                f = b ^ c ^ d;
                k = 0xCA62C1D6;
            }

            uint32_t temp = ((a << 5) | (a >> 27)) + f + e + k + w[i];
            e = d;
            d = c;
            c = (b << 30) | (b >> 2);
            b = a;
            a = temp;
        }

        h0 += a;
        h1 += b;
        h2 += c;
        h3 += d;
        h4 += e;
    }

    std::stringstream ss;
    ss << std::hex << std::setw(8) << std::setfill('0') << h0
        << std::setw(8) << h1
        << std::setw(8) << h2
        << std::setw(8) << h3
        << std::setw(8) << h4;

    return ss.str();
}

uint64_t scramble(uint64_t inputNumber) {
	uint64_t out = inputNumber ^ 0xDEADBEEFC;
	out = (out & 0xF0F0F0F0F) >> 4 | (out & 0x0F0F0F0F0F) << 4;
	return out ^ 0xC0DEFACE12345678;
}

}