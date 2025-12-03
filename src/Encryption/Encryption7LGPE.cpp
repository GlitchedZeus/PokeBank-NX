/**
 * Encryption7LGPE.cpp - Generation 7 Pokemon Let's Go Pikachu/Eevee Encryption/Decryption Implementation
 *
 * Implementation of encryption and decryption utilities.
 * See Encryption7LGPE.h for detailed documentation.
 */

#include <cstring>

#include "Utils/HelperUtilities.h"
#include "Encryption/Encryption.h"
#include "Encryption/Encryption7LGPE.h"

using namespace Utils;

namespace Encryption {
    // ========================================
    // Encryption/Decryption Functions
    // ========================================

    void cryptPokemon7LGPE(std::span<std::byte> data, uint32_t personalityValue, size_t blockSize, size_t blockCount)
    {
        /**
         * Decrypts/encrypts a Pokemon's data blocks using Gen 7 format.
         *
         * Pokemon data structure (Gen 7 Let's Go):
         * - Bytes 0-7:   Header (Encryption Constant + Checksum) - NOT encrypted
         * - Bytes 8-231: Four 56-byte blocks (Growth, Attacks, EVs, Misc) - ENCRYPTED
         * - Bytes 232-259: Additional data (varies by format) - ENCRYPTED
         *
         * All data is encrypted as one continuous block after the header.
         *
         * Block size:
         * - 56 bytes per block (SIZE_BLOCK7_LGPE)
         */

        constexpr int start = 8; // Skip first 8 bytes (encryption constant + checksum)

        // Gen 7 encrypts everything after the header as one continuous block
        // Calculate total encrypted size: data size - header size
        const size_t encryptedSize = data.size() - start;

        if (encryptedSize > 0) {
            auto encryptedSpan = data.subspan(start, encryptedSize);

            // TODO: Functions needs to be implemented
            cryptArray7LGPE(encryptedSpan, personalityValue);
        }
    }

    void shuffleArray7LGPE(std::span<const std::byte> data, std::span<std::byte> result, uint32_t shuffleValue, size_t blockSize)
    {
        /**
         * Unshuffles the 4 data blocks based on the shuffle value.
         * Gen 7 block size (56 bytes instead of 80 bytes).
         *
         * Data Structure:
         * - Bytes 0-7:   Header (unchanged)
         * - Bytes 8-63:  Block A (Growth) - 56 bytes
         * - Bytes 64-119: Block B (Attacks) - 56 bytes
         * - Bytes 120-175: Block C (EVs/Contest) - 56 bytes
         * - Bytes 176-231: Block D (Misc) - 56 bytes
         * - Bytes 232+:  Additional data (unchanged, if present)
         */

        const uint32_t index = shuffleValue * BLOCK_COUNT7_LGPE;
        constexpr uint32_t start = 8;

        // Copy first 8 bytes unchanged (encryption constant + checksum)
        std::memcpy(result.data(), data.data(), start);

        // Calculate end of shuffled region (4 blocks * blockSize)
        const auto end = start + (blockSize * BLOCK_COUNT7_LGPE);

        // Copy everything after shuffled blocks unchanged
        if (end < data.size())
        {
            const size_t remainingSize = data.size() - end;
            std::memcpy(result.data() + end, data.data() + end, remainingSize);
        }

        // Unshuffle the 4 blocks using the shared blockPosition table
        // Read from shuffled positions, write to natural positions
        for (uint32_t block = 0; block < BLOCK_COUNT7_LGPE; block++)
        {
            // blockPosition[index + block] tells us which block is in position 'block'
            const int srcBlockIndex = blockPosition[index + block];
            const size_t srcOffset = start + (blockSize * srcBlockIndex);
            const size_t destOffset = start + (blockSize * block);

            // Copy one block from shuffled position to correct position
            std::memcpy(result.data() + destOffset, data.data() + srcOffset, blockSize);
        }
    }

    std::byte* decryptArray7LGPE(std::span<const std::byte> encryptedData)
    {
        /**
         * Main decryption function for Generation 7 Pokemon data.
         *
         * This function performs the complete decryption process:
         * 1. Extract encryption seed (Personality Value) from data
         * 2. Calculate shuffle value from Personality Value
         * 3. XOR decrypt the blocks
         * 4. Unshuffle blocks to restore natural order
         *
         * The result is Pokemon data in its natural, unencrypted block order,
         * ready for reading and modification.
         *
         * - Uses SIZE_BLOCK7_LGPE (56 bytes) for block operations
         * - Data size is always 260 bytes
         */

        // Extract Personality Value from first 4 bytes
        // Personality Value serves as the encryption seed
        const uint32_t personalityValue = readUInt32LittleEndian(reinterpret_cast<const uint8_t*>(encryptedData.data()));

        // Extract Shuffle Value from bits 13-17 of Personality Value
        // SV determines how blocks were shuffled (0-31)
        // This calculation is identical across all generations
        const uint32_t shuffleValue = (personalityValue >> 13) & 31;

        // Create a mutable copy for XOR decryption
        std::byte* decryptedData = new std::byte[encryptedData.size()];
        std::memcpy(decryptedData, encryptedData.data(), encryptedData.size());
        std::span<std::byte> mutableSpan(decryptedData, encryptedData.size());

        // Decrypt the blocks using XOR cipher
        cryptPokemon7LGPE(mutableSpan, personalityValue, SIZE_BLOCK7_LGPE);

        // Unshuffle the blocks to their correct positions (Gen 7 block size)
        std::byte* unshuffledData = new std::byte[encryptedData.size()];
        std::span<std::byte> resultSpan(unshuffledData, encryptedData.size());
        shuffleArray7LGPE(mutableSpan, resultSpan, shuffleValue, SIZE_BLOCK7_LGPE);

        // Clean up intermediate buffer
        delete[] decryptedData;

        return unshuffledData;
    }

    std::byte* encryptArray7LGPE(std::span<const std::byte> decryptedData, uint32_t personalityValue)
    {
        /**
         * Main encryption function for Generation 7 Pokemon data.
         *
         * This is the reverse of decryptArray7LGPE. It takes unencrypted, unshuffled
         * Pokemon data and converts it back to the encrypted format used in save files.
         *
         * Process:
         * 1. Calculate shuffle value from Personality Value
         * 2. Shuffle blocks from natural order to encrypted positions
         * 3. Apply XOR cipher using Personality Value as seed
         * 
         * - Uses SIZE_BLOCK7_LGPE (56 bytes) for block operations
         */

        // Extract Shuffle Value from bits 13-17 of Personality Value
        const uint32_t shuffleValue = (personalityValue >> 13) & 31;

        // Create buffer for shuffled data
        std::byte* shuffledData = new std::byte[decryptedData.size()];
        std::span<std::byte> shuffledSpan(shuffledData, decryptedData.size());

        // Shuffle the blocks from normal positions to encrypted positions
        const uint32_t index = shuffleValue * BLOCK_COUNT7_LGPE;
        constexpr uint32_t start = 8;

        // Copy first 8 bytes unchanged (encryption constant + checksum)
        std::memcpy(shuffledSpan.data(), decryptedData.data(), start);

        // Calculate end of shuffled region
        const auto end = start + (SIZE_BLOCK7_LGPE * BLOCK_COUNT7_LGPE);

        // Copy everything after shuffled blocks unchanged
        if (end < decryptedData.size())
        {
            const size_t remainingSize = decryptedData.size() - end;
            std::memcpy(shuffledSpan.data() + end, decryptedData.data() + end, remainingSize);
        }

        // Shuffle the 4 blocks (reverse of unshuffle)
        // Read from natural positions, write to shuffled positions
        for (uint32_t block = 0; block < BLOCK_COUNT7_LGPE; block++)
        {
            // blockPosition[index + block] tells us where block 'block' should go
            const int destBlockIndex = blockPosition[index + block];
            const size_t srcOffset = start + (SIZE_BLOCK7_LGPE * block);
            const size_t destOffset = start + (SIZE_BLOCK7_LGPE * destBlockIndex);

            // Copy one block from decrypted position to shuffled position
            std::memcpy(shuffledSpan.data() + destOffset, decryptedData.data() + srcOffset, SIZE_BLOCK7_LGPE);
        }

        // Apply XOR cipher (symmetric operation)
        cryptPokemon7LGPE(shuffledSpan, personalityValue, SIZE_BLOCK7_LGPE);

        return shuffledData;
    }
}
