/**
 * Gen8aLegality.h - Legends: Arceus (PA8) legality.
 *
 * Reached through Legality::Modern::analyze (ModernLegality.h). PA8 keeps the Gen VIII
 * encryption (the same LCG and block order as PK8, Integration/Gen8/Gen8PK8.h) with 0x58-byte
 * blocks: 0x168 bytes stored, 0x178 in the party. The record codec is here; the rule data
 * (PersonalTable.LA abilities, gender ratios, growth rates, forms, presence, heights, weights
 * and move shop permissions, the move shop list, base PP, released held items, battle-only
 * forms and the ID limits) is generated from the pinned PKHeX.Core by
 * tools/pkhex_gen67_diff/oracle ("dump8a") into Gen8aLegalityData.inc.
 *
 * Every Invalid rule mirrors a PKHeX.Core verifier at the pinned commit and depends only on
 * fields the record itself fixes; a rule that needs the encounter, a learnset or HOME history
 * is not implemented and keeps the report Incomplete. See
 * docs/legality/gen69-foundation/README.md for the rule-by-rule source map.
 */
#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

#include "Legality/ModernLegality.h"

namespace Legality::Gen8a {

namespace Codec {
inline constexpr size_t StoredSize = 0x168;
inline constexpr size_t PartySize = 0x178;
inline constexpr size_t BlockSize = 0x58;
/// PokeCrypto.Decrypt8A / Encrypt8A. Empty on a size other than 0x168 or 0x178.
std::vector<uint8_t> decrypt(std::span<const uint8_t> encrypted);
std::vector<uint8_t> encrypt(std::span<const uint8_t> plain);
/// PA8.CalculateChecksum: 16-bit sum of little-endian words over [0x08, 0x168).
uint16_t checksum(std::span<const uint8_t> plain) noexcept;
} // namespace Codec

namespace Data {
struct Personal {
    uint16_t ability1, ability2, abilityHidden;
    uint8_t gender, growth, formCount;
    bool present;        // PersonalTable.LA.IsPresentInGame
    uint16_t height, weight;
    uint64_t moveShop;   // PersonalInfo8LA.IsRecordPermitted, bit per move shop index
};
/// PersonalTable.LA entry; nullopt for species 0, a species above 905 or a form outside the
/// species' form count.
std::optional<Personal> personal(uint16_t species, uint8_t form) noexcept;
/// PersonalInfo8LA.RecordPermitIndexes.
std::span<const uint16_t> moveShop() noexcept;
uint8_t basePP(uint16_t move) noexcept;
uint16_t maxSpecies() noexcept;
uint16_t maxMove() noexcept;
uint8_t maxBall() noexcept;
bool heldItemAllowed(uint16_t item) noexcept;
bool battleOnlyForm(uint16_t species, uint8_t form) noexcept;
/// Nature.IsMint: the natures a stat-nature byte may hold when it differs from the nature.
bool mintNature(uint8_t nature) noexcept;
} // namespace Data

/// PA8.GetHeightAbsolute / GetWeightAbsolute (in-game single-precision steps, no FMA).
float heightAbsolute(uint16_t height, uint8_t heightScalar) noexcept;
float weightAbsolute(uint16_t weight, uint8_t heightScalar, uint8_t weightScalar) noexcept;

/// Analyze one PA8 record. Called by Modern::analyze; see ModernLegality.h for the contract.
Report analyze(std::span<const uint8_t> record, const Modern::Input& input);

} // namespace Legality::Gen8a
