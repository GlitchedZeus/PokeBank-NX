#include <vector>
#include <codecvt>
#include <locale>
#include <cstdio>
#include <span>
#include <cstring>

#include <sys/types.h>

#include "Globals.h"
#include "Utils/Logger.h"
#include "Utils/FileUtilities.h"
#include "Utils/HelperUtilities.h"
#include "Utils/StringHelpers.h"
#include "Utils/SHA256.h"
#include "Encryption/Encryption.h"
#include "Save/Block.h"

using namespace Utils;
using namespace Save;

namespace Encryption {
    void cryptStaticXorpadBytes(std::vector<uint8_t> &data, size_t dataLength)
    {
        // Apply the xorpad over each chunk of xorpad-sized spans.
        const size_t xpLength = sizeof(StaticXorpad) / sizeof(StaticXorpad[0]);
        const size_t size = xpLength - 1; // 0x7F, not 0x80
        
        for (size_t offset = 0; offset < dataLength; offset += size)
        {
            size_t chunkSize = (dataLength - offset < xpLength) ? (dataLength - offset) : xpLength;
            for (size_t i = 0; i < chunkSize; i++)
            {
                data[offset + i] ^= StaticXorpad[i];
            }
        }
    }

    DecryptStatus tryDecrypt(const uint8_t* data, size_t dataLength,
                             std::vector<Block>& blocks) {
        blocks.clear();
        if (data == nullptr) return DecryptStatus::MissingData;
        if (dataLength <= SIZE_HASH_IN_BYTES) return DecryptStatus::TooSmall;

        const size_t payloadLength = dataLength - SIZE_HASH_IN_BYTES;
        uint8_t expectedHash[SIZE_HASH_IN_BYTES];
        computeHash(data, payloadLength, expectedHash);
        if (std::memcmp(expectedHash, data + payloadLength, SIZE_HASH_IN_BYTES) != 0)
            return DecryptStatus::HashMismatch;

        std::vector<uint8_t> payload(data, data + payloadLength);
        cryptStaticXorpadBytes(payload, payloadLength);
        size_t consumed = 0;
        blocks = parseAllBlocks(payload.data(), payloadLength, &consumed);
        if (blocks.empty() || consumed != payloadLength) {
            blocks.clear();
            return DecryptStatus::MalformedBlocks;
        }
        return DecryptStatus::Ok;
    }

    std::vector<Block> decrypt(uint8_t* data, size_t dataLength) {
        std::vector<Block> blocks;
        (void)tryDecrypt(data, dataLength, blocks);
        return blocks;
    }

    void computeHash(const uint8_t* data, size_t dataLength, uint8_t* hash) {
        // Compute SHA256 hash with intro and outro salt bytes
        SHA256 sha;
        sha.update(IntroHashBytes, sizeof(IntroHashBytes));
        sha.update(data, dataLength);
        sha.update(OutroHashBytes, sizeof(OutroHashBytes));
        sha.finalize(hash);
    }

    std::vector<uint8_t> encrypt(const std::vector<Block>& blocks) {
        // Serialize blocks to binary format
        std::vector<uint8_t> payload = serializeAllBlocks(blocks);

        // Apply XOR encryption
        cryptStaticXorpadBytes(payload, payload.size());

        // Calculate hash of encrypted payload
        uint8_t hash[SIZE_HASH_IN_BYTES];
        computeHash(payload.data(), payload.size(), hash);

        // Append the hash
        payload.insert(payload.end(), hash, hash + SIZE_HASH_IN_BYTES);

        return payload;
    }

    // ========================================
    // Encryption/Decryption Functions
    // ========================================

    void cryptArray(std::span<std::byte> data, uint32_t seed)
    {
        // Process data as 16-bit chunks (uint16_t)
        const size_t numU16 = data.size() / sizeof(uint16_t);

        for (size_t i = 0; i < numU16; ++i)
        {
            // Advance LCG state: seed = (multiplier * seed) + increment
            seed = (0x41C64E6D * seed) + 0x00006073;

            // Extract upper 16 bits for XOR mask
            // The lower 16 bits are less random, so we use upper 16 bits
            const uint16_t xorValue = static_cast<uint16_t>(seed >> 16);

            // Read current uint16 value from data (little-endian)
            const size_t byteOffset = i * sizeof(uint16_t);
            auto* bytes = reinterpret_cast<uint8_t*>(data.data() + byteOffset);
            writeUInt16LittleEndian(bytes, readUInt16LittleEndian(bytes) ^ xorValue);
        }
    }

    void cryptPokemon(std::span<std::byte> data, uint32_t partyValue, size_t blockSize, size_t blockCount)
    {
        constexpr int start = 8; // Skip first 8 bytes (encryption constant + checksum)
        const int blocksEndValue = blockSize * blockCount;
        const int blocksEnd = start + blocksEndValue; // End of the 4 shuffled blocks

        // Decrypt the 4 data blocks (Growth, Attacks, EVs, Misc)
        auto blocksSpan = data.subspan(start, blocksEndValue);
        cryptArray(blocksSpan, partyValue);

        // If there's party data, decrypt it too
        // Party data includes battle stats and is only present for party Pokemon
        if (data.size() > static_cast<size_t>(blocksEnd))
        {
            auto partySpan = data.subspan(blocksEnd);
            cryptArray(partySpan, partyValue);
        }
    }
}
