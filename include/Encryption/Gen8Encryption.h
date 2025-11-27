/**
 * Gen8Encryption.h - Generation 8 Pokemon Encryption/Decryption Utilities
 *
 * This file contains encryption and decryption utilities for Generation 8 Pokemon data
 * (Sword/Shield, Brilliant Diamond/Shining Pearl, Legends: Arceus).
 *
 * Pokemon data in Gen 8 is stored in an encrypted format using:
 * 1. A Linear Congruential Generator (LCG) for XOR encryption
 * 2. Block shuffling based on personality value
 *
 * Encryption Process:
 * 1. Four data blocks (Growth, Attacks, EVs, Misc) are shuffled based on shuffle value
 * 2. Blocks are encrypted using XOR with an LCG-generated keystream
 * 3. Encryption constant (EC) is used as the seed
 *
 * Decryption Process (reverse):
 * 1. XOR decryption using EC as seed
 * 2. Block unshuffling to restore original order
 */

#ifndef ENCRYPTION_GEN8_ENCRYPTION_H
#define ENCRYPTION_GEN8_ENCRYPTION_H

#include <cstdint>
#include <cstddef>
#include <span>

#include "Encryption/Encryption.h"

// ========================================
// Generation 8 Constants
// ========================================

/**
 * Number of data blocks in Pokemon structure.
 * Gen 8 uses 4 blocks: Growth, Attacks, EVs/Contest, and Misc.
 */
constexpr size_t BLOCK_8COUNT = 4;

/**
 * Size of each block in bytes (Gen 8).
 * Each block contains 80 bytes (0x50) of Pokemon data.
 */
constexpr size_t SIZE_8BLOCK = 0x50;

/**
 * Size of stored Pokemon data (Gen 8).
 * Includes 8-byte header + 4 blocks = 328 bytes (0x148).
 */
constexpr size_t SIZE_8STORED = 8 + (BLOCK_8COUNT * SIZE_8BLOCK);

/**
 * Size of party Pokemon data (Gen 8).
 * Stored data + 16 bytes of party stats = 344 bytes (0x158).
 */
constexpr size_t SIZE_8PARTY = SIZE_8STORED + 0x10;

// ========================================
// Encryption/Decryption Functions
// ========================================

/**
 * Unshuffles the 4 data blocks based on the shuffle value.
 *
 * During encryption, Pokemon data blocks are shuffled in a deterministic order
 * based on the shuffle value derived from the personality value. This function
 * reverses that shuffling to restore blocks to their natural order:
 *   Block A (Growth)    - Species, items, EVs, etc.
 *   Block B (Attacks)   - Moves, IVs, nickname
 *   Block C (EVs)       - Contest stats, ribbons
 *   Block D (Misc)      - OT info, encounter data
 *
 * @param data Source encrypted data (read-only)
 * @param result Destination for unshuffled data (must be same size as data)
 * @param sv Shuffle value (0-31), derived from (PV >> 13) & 31
 */
void shuffleArray8(std::span<const std::byte> data, std::span<std::byte> result, uint32_t sv);

/**
 * Decrypts a Generation 8 Pokemon byte array.
 *
 * This is the main decryption function that combines XOR decryption and
 * block unshuffling to convert encrypted Pokemon data into readable format.
 *
 * Process:
 * 1. Extract Personality Value (PV) from first 4 bytes
 * 2. Calculate Shuffle Value (SV) from PV
 * 3. Decrypt blocks using XOR cipher (CryptPKM)
 * 4. Unshuffle blocks to restore original order
 *
 * @param encryptedData The encrypted Pokemon data (SIZE_8PARTY bytes)
 * @return Pointer to decrypted data (caller must delete[])
 */
std::byte* decryptArray8(std::span<const std::byte> encryptedData);

/**
 * Encrypts a Generation 8 Pokemon byte array.
 *
 * This is the reverse of decryptArray8, used when saving Pokemon data
 * back to the save file.
 *
 * Process:
 * 1. Calculate Shuffle Value (SV) from PV
 * 2. Shuffle blocks from normal order to encrypted positions
 * 3. Apply XOR cipher using PV as seed
 *
 * @param decryptedData The decrypted Pokemon data (read-only)
 * @param pv Personality Value (encryption seed)
 * @return Pointer to encrypted data (caller must delete[])
 */
std::byte* encryptArray8(std::span<const std::byte> decryptedData, uint32_t pv);

#endif // GEN8_ENCRYPTION_H
