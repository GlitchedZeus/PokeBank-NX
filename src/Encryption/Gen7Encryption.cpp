/**
 * Gen7Encryption.cpp - Generation 7 Pokemon Encryption/Decryption Implementation
 *
 * Implementation of encryption and decryption utilities for Generation 7 Pokemon data
 * (Pokemon Let's Go Pikachu/Eevee - PK7 format).
 *
 * See Gen7Encryption.h for detailed documentation.
 */

#include <cstring>

#include "Utils/Utilities.h"
#include "Encryption/Encryption.h"
#include "Encryption/Gen7Encryption.h"

// ========================================
// Encryption/Decryption Functions
// ========================================

void cryptPKM7(std::span<std::byte> data, uint32_t partyValue, size_t blockSize, size_t blockCount)
{
    /**
     * Decrypts/encrypts a Pokemon's data blocks using Gen 6/7 format.
     *
     * Pokemon data structure (Gen 7 Let's Go):
     * - Bytes 0-7:   Header (Encryption Constant + Checksum) - NOT encrypted
     * - Bytes 8-231: Four 56-byte blocks (Growth, Attacks, EVs, Misc) - ENCRYPTED
     * - Bytes 232-259: Additional data (varies by format) - ENCRYPTED
     *
     * Unlike Gen 8, there is no separate party stats section. All data is
     * encrypted as one continuous block after the header.
     *
     * The key difference from Gen 8 is the block size:
     * - Gen 7: 56 bytes per block (SIZE_6BLOCK)
     * - Gen 8: 80 bytes per block (SIZE_8BLOCK)
     */

    constexpr int start = 8; // Skip first 8 bytes (encryption constant + checksum)

    // Gen 7 encrypts everything after the header as one continuous block
    // Calculate total encrypted size: data size - header size
    const size_t encryptedSize = data.size() - start;

    if (encryptedSize > 0) {
        auto encryptedSpan = data.subspan(start, encryptedSize);

        // TODO: Functions needs to be implemented
        cryptArray7(encryptedSpan, partyValue);
    }
}

void shuffleArray7(std::span<const std::byte> data, std::span<std::byte> result, uint32_t sv, size_t blockSize)
{
    /**
     * Unshuffles the 4 data blocks based on the shuffle value (Gen 6/7 format).
     *
     * This function is nearly identical to Gen 8's shuffleArray, but uses
     * the Gen 6/7 block size (56 bytes instead of 80 bytes).
     *
     * Data Structure:
     * - Bytes 0-7:   Header (unchanged)
     * - Bytes 8-63:  Block A (Growth) - 56 bytes
     * - Bytes 64-119: Block B (Attacks) - 56 bytes
     * - Bytes 120-175: Block C (EVs/Contest) - 56 bytes
     * - Bytes 176-231: Block D (Misc) - 56 bytes
     * - Bytes 232+:  Additional data (unchanged, if present)
     */

    const uint32_t index = sv * BLOCK_7COUNT;
    constexpr uint32_t start = 8;

    // Copy first 8 bytes unchanged (encryption constant + checksum)
    std::memcpy(result.data(), data.data(), start);

    // Calculate end of shuffled region (4 blocks * blockSize)
    const auto end = start + (blockSize * BLOCK_7COUNT);

    // Copy everything after shuffled blocks unchanged
    if (end < data.size())
    {
        const size_t remainingSize = data.size() - end;
        std::memcpy(result.data() + end, data.data() + end, remainingSize);
    }

    // Unshuffle the 4 blocks using the shared blockPosition table
    // Read from shuffled positions, write to natural positions
    for (uint32_t block = 0; block < BLOCK_7COUNT; block++)
    {
        // blockPosition[index + block] tells us which block is in position 'block'
        const int srcBlockIndex = blockPosition[index + block];
        const size_t srcOffset = start + (blockSize * srcBlockIndex);
        const size_t destOffset = start + (blockSize * block);

        // Copy one block from shuffled position to correct position
        std::memcpy(result.data() + destOffset, data.data() + srcOffset, blockSize);
    }
}

std::byte* decryptArray7(std::span<const std::byte> encryptedData)
{
    /**
     * Main decryption function for Generation 6/7 Pokemon data (including PK7).
     *
     * This function performs the complete decryption process:
     * 1. Extract encryption seed (Personality Value) from data
     * 2. Calculate shuffle value from PV
     * 3. XOR decrypt the blocks
     * 4. Unshuffle blocks to restore natural order
     *
     * The result is Pokemon data in its natural, unencrypted block order,
     * ready for reading and modification.
     *
     * Key Difference from Gen 8:
     * - Uses SIZE_6BLOCK (56 bytes) for block operations
     * - Data size is always 260 bytes for PK7
     */

    // Extract Personality Value (PV) from first 4 bytes
    // PV serves as the encryption seed
    const uint32_t pv = readUInt32LittleEndian(reinterpret_cast<const uint8_t*>(encryptedData.data()));

    // Extract Shuffle Value (SV) from bits 13-17 of PV
    // SV determines how blocks were shuffled (0-31)
    // This calculation is identical across all generations
    const uint32_t sv = (pv >> 13) & 31;

    // Step 1: Create a mutable copy for XOR decryption
    std::byte* decryptedData = new std::byte[encryptedData.size()];
    std::memcpy(decryptedData, encryptedData.data(), encryptedData.size());
    std::span<std::byte> mutableSpan(decryptedData, encryptedData.size());

    // Step 2: Decrypt the blocks using XOR cipher (Gen 6/7 format)
    cryptPKM7(mutableSpan, pv, SIZE_7BLOCK);

    // Step 3: Unshuffle the blocks to their correct positions (Gen 6/7 block size)
    std::byte* unshuffledData = new std::byte[encryptedData.size()];
    std::span<std::byte> resultSpan(unshuffledData, encryptedData.size());
    shuffleArray7(mutableSpan, resultSpan, sv, SIZE_7BLOCK);

    // Clean up intermediate buffer
    delete[] decryptedData;

    return unshuffledData;
}

std::byte* encryptArray7(std::span<const std::byte> decryptedData, uint32_t pv)
{
    /**
     * Main encryption function for Generation 6/7 Pokemon data (including PK7).
     *
     * This is the reverse of decryptArray67. It takes unencrypted, unshuffled
     * Pokemon data and converts it back to the encrypted format used in save files.
     *
     * Process:
     * 1. Calculate shuffle value from PV
     * 2. Shuffle blocks from natural order to encrypted positions
     * 3. Apply XOR cipher using PV as seed
     *
     * Key Difference from Gen 8:
     * - Uses SIZE_6BLOCK (56 bytes) for block operations
     */

    // Extract Shuffle Value (SV) from bits 13-17 of PV
    const uint32_t sv = (pv >> 13) & 31;

    // Create buffer for shuffled data
    std::byte* shuffledData = new std::byte[decryptedData.size()];
    std::span<std::byte> shuffledSpan(shuffledData, decryptedData.size());

    // Step 1: Shuffle the blocks from normal positions to encrypted positions
    const uint32_t index = sv * BLOCK_7COUNT;
    constexpr uint32_t start = 8;

    // Copy first 8 bytes unchanged (encryption constant + checksum)
    std::memcpy(shuffledSpan.data(), decryptedData.data(), start);

    // Calculate end of shuffled region
    const auto end = start + (SIZE_7BLOCK * BLOCK_7COUNT);

    // Copy everything after shuffled blocks unchanged
    if (end < decryptedData.size())
    {
        const size_t remainingSize = decryptedData.size() - end;
        std::memcpy(shuffledSpan.data() + end, decryptedData.data() + end, remainingSize);
    }

    // Shuffle the 4 blocks (reverse of unshuffle)
    // Read from natural positions, write to shuffled positions
    for (uint32_t block = 0; block < BLOCK_7COUNT; block++)
    {
        // blockPosition[index + block] tells us where block 'block' should go
        const int destBlockIndex = blockPosition[index + block];
        const size_t srcOffset = start + (SIZE_7BLOCK * block);
        const size_t destOffset = start + (SIZE_7BLOCK * destBlockIndex);

        // Copy one block from decrypted position to shuffled position
        std::memcpy(shuffledSpan.data() + destOffset, decryptedData.data() + srcOffset, SIZE_7BLOCK);
    }

    // Step 2: Apply XOR cipher (symmetric operation)
    cryptPKM7(shuffledSpan, pv, SIZE_7BLOCK);

    return shuffledData;
}
