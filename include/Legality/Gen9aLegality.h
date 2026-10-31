/**
 * Gen9aLegality.h - Legends: Z-A (PA9) legality.
 *
 * Reached through Legality::Modern::analyze (ModernLegality.h). PA9 is a 0x148/0x158-byte record
 * with PK8's encryption (Integration/Gen8/Gen8PK8.h) and PK9's internal species ids
 * (Gen8::Data::nationalSpecies9), with its own field layout. Rule data (PersonalTable.ZA
 * abilities, gender ratios, growth rates, forms and presence, base PP, released held items,
 * battle-only forms and the ID limits) is generated from the pinned PKHeX.Core by
 * tools/pkhex_gen67_diff/oracle ("dump9a") into Gen9aLegalityData.inc.
 *
 * Every Invalid rule mirrors a PKHeX.Core verifier at the pinned commit and depends only on
 * fields the record itself fixes; a rule that needs the encounter, a learnset or HOME history
 * is not implemented and keeps the report Incomplete. See
 * docs/legality/gen69-foundation/README.md for the rule-by-rule source map.
 */
#pragma once

#include <cstdint>
#include <optional>
#include <span>

#include "Legality/ModernLegality.h"

namespace Legality::Gen9a {

namespace Data {
struct Personal {
    uint16_t ability1, ability2, abilityHidden;
    uint8_t gender, growth, formCount;
    bool present;  // PersonalTable.ZA.IsPresentInGame
};
/// PersonalTable.ZA entry; nullopt for species 0, a species above 1000 or a form outside the
/// species' form count.
std::optional<Personal> personal(uint16_t species, uint8_t form) noexcept;
uint8_t basePP(uint16_t move) noexcept;
uint16_t maxSpecies() noexcept;
uint16_t maxMove() noexcept;
uint8_t maxBall() noexcept;
bool heldItemAllowed(uint16_t item) noexcept;
bool battleOnlyForm(uint16_t species, uint8_t form) noexcept;
bool mintNature(uint8_t nature) noexcept;
} // namespace Data

/// Analyze one PA9 record. Called by Modern::analyze; see ModernLegality.h for the contract.
Report analyze(std::span<const uint8_t> record, const Modern::Input& input);

} // namespace Legality::Gen9a
