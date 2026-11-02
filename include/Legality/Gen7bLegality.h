/**
 * Gen7bLegality.h - Let's Go Pikachu/Eevee (PB7) native legality.
 *
 * Reached through Legality::Modern::analyze (ModernLegality.h). PB7 is the Gen VI/VII record
 * layout (same encryption, checksum and most offsets), always 0x104 bytes, so the record codec
 * is the existing Gen VI PK6 decoder (Integration/Gen6/Gen6PK6.h). Rule data (PersonalTable.GG
 * abilities, gender ratios, form counts, growth rates, base stats, heights and weights,
 * presence in the game, base PP, released held items, battle-only forms and ID limits) is
 * generated from the pinned PKHeX.Core by tools/pkhex_gen67_diff/oracle ("dump7b") into
 * Gen7bLegalityData.inc.
 *
 * Every Invalid rule mirrors a PKHeX.Core verifier at the pinned commit and depends only on
 * fields the record itself fixes; a rule that needs the encounter, a learnset or a
 * distribution is not implemented and keeps the report Incomplete. See
 * docs/legality/gen69-foundation/README.md for the rule-by-rule source map.
 */
#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <span>

#include "Legality/ModernLegality.h"

namespace Legality::Gen7b {

namespace Data {
struct Personal {
    uint16_t ability1, ability2, abilityHidden;
    uint8_t gender, growth, formCount;
    bool present;                      // PersonalTable.GG.IsPresentInGame
    std::array<uint8_t, 6> baseStats;  // HP Atk Def Spe SpA SpD
    uint16_t height, weight;           // PersonalInfo.Height / Weight (x100)
};
/// PersonalTable.GG entry; nullopt for species 0, a species above 809 or a form outside the
/// species' form count.
std::optional<Personal> personal(uint16_t species, uint8_t form) noexcept;
/// MoveInfo.GetPP(EntityContext.Gen7b, move); 0 above the last move.
uint8_t basePP(uint16_t move) noexcept;
uint16_t maxSpecies() noexcept;
uint16_t maxMove() noexcept;
uint8_t maxBall() noexcept;
/// ItemRestrictions.IsHeldItemAllowed(item, EntityContext.Gen7b).
bool heldItemAllowed(uint16_t item) noexcept;
/// FormInfo.IsBattleOnlyForm(species, form, 7).
bool battleOnlyForm(uint16_t species, uint8_t form) noexcept;
} // namespace Data

/// PB7.CalcCP: the combat power Let's Go stores for the record's species, level, IVs (Hyper
/// Training counted as 31), Awakening Values, nature and current friendship. Single-precision
/// steps exactly as PKHeX computes them.
uint16_t combatPower(const Data::Personal& p, uint8_t level, const std::array<uint8_t, 6>& ivs,
                     const std::array<uint8_t, 6>& avs, uint8_t nature, uint8_t friendship) noexcept;
/// PB7.GetHeightAbsolute / GetWeightAbsolute (no fused multiply-add, as PKHeX).
float heightAbsolute(uint16_t height, uint8_t heightScalar) noexcept;
float weightAbsolute(uint16_t weight, uint8_t heightScalar, uint8_t weightScalar) noexcept;

/// Analyze one PB7 record. Called by Modern::analyze; see ModernLegality.h for the contract.
Report analyze(std::span<const uint8_t> record, const Modern::Input& input);

} // namespace Legality::Gen7b
