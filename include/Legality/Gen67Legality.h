/**
 * Gen67Legality.h - Generation VI (PK6) and Generation VII (PK7) native legality.
 *
 * Reached through Legality::Modern::analyze (ModernLegality.h). The record codec is the
 * existing Gen VI PK6 decoder (Integration/Gen6/Gen6PK6.h; PK7 shares its 232/260-byte
 * layout, crypto and checksum) and base stats come from the existing generated personal
 * tables (Gen6PersonalData.h, Gen7PersonalData.h). Rule data the codec does not carry
 * (abilities, gender ratios, form counts, base PP, released held items, form items,
 * battle-only forms, Vivillon console regions and ID limits) is generated from the pinned
 * PKHeX.Core by tools/pkhex_gen67_diff/oracle ("dump") into Gen67LegalityData.inc.
 *
 * Every Invalid rule mirrors a PKHeX.Core verifier at the pinned commit and depends only
 * on fields the record itself fixes (its format, origin game, species/form and slot);
 * a rule that needs the encounter, an evolution chain, a learnset or a distribution is
 * not implemented and keeps the report Incomplete. See
 * docs/legality/gen69-foundation/README.md for the rule-by-rule source map.
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

#include "Legality/ModernLegality.h"

namespace Legality::Gen67 {

/// Generation of a PKHeX GameVersion origin id as it can appear in a PK6/PK7 record:
/// 1-2 Virtual Console, 3 GBA/GameCube, 4 DS, 5 Gen V, 6 3DS Gen VI, 7 3DS Gen VII.
/// 0 for an id no game writes.
uint8_t originGeneration(uint8_t version) noexcept;

/// PK5.GetTransferPID: the PID Poke Transporter writes for a Gen III-V origin (the
/// original PID, with bit 31 flipped when it would become shiny under Gen VI odds).
constexpr uint32_t transferPid(uint32_t ec, uint32_t id32) noexcept {
    const uint32_t tmp = ec ^ id32;
    const uint32_t x = tmp ^ (tmp >> 16);
    return (x & 0xFFF8u) == 8 ? (ec ^ 0x80000000u) : ec;
}

/// EntityPID.GetUnownForm3: the Unown letter a Gen III PID fixes.
constexpr uint8_t unownForm3(uint32_t pid) noexcept {
    const uint32_t v = ((pid & 0x3000000u) >> 18) | ((pid & 0x30000u) >> 12) | ((pid & 0x300u) >> 6) | (pid & 0x3u);
    return static_cast<uint8_t>(v % 28);
}

/// Generated PKHeX data (Gen67LegalityData.cpp).
namespace Data {
struct Personal {
    uint16_t ability1, ability2, abilityHidden;
    uint8_t gender, growth, formCount;
};
/// PersonalTable.AO (format 6) or PersonalTable.USUM (format 7) entry; nullopt for a
/// species above the table or a form outside its form count.
std::optional<Personal> personal(uint8_t format, uint16_t species, uint8_t form) noexcept;
/// Form count of the species' base entry (at least 1); 0 when the species is outside.
uint8_t formCount(uint8_t format, uint16_t species) noexcept;
/// MoveInfo.GetPP for the format's context; 0 above the format's last move.
uint8_t basePP(uint8_t format, uint16_t move) noexcept;
uint16_t maxSpecies(uint8_t format) noexcept;
uint16_t maxMove(uint8_t format) noexcept;
uint8_t maxBall(uint8_t format) noexcept;
/// ItemRestrictions.IsHeldItemAllowed for the format's context.
bool heldItemAllowed(uint8_t format, uint16_t item) noexcept;
/// FormInfo.IsBattleOnlyForm.
bool battleOnlyForm(uint8_t format, uint16_t species, uint8_t form) noexcept;
/// FormItem.GetFormArceus / GetFormGenesect / GetFormSilvally.
uint8_t arceusForm(uint8_t format, uint16_t item) noexcept;
uint8_t genesectForm(uint16_t item) noexcept;
uint8_t silvallyForm(uint16_t item) noexcept;
/// Vivillon3DS.MaxWildFormID and IsPatternValid.
uint8_t vivillonMaxWildForm() noexcept;
bool vivillonPatternValid(uint8_t form, uint8_t consoleRegion) noexcept;
/// Locale3DS.IsConsoleRegionCountryValid (regions 0-6; anything else is false).
bool consoleRegionCountryValid(uint8_t consoleRegion, uint8_t country) noexcept;
/// Locale3DS.IsRegionLockedLanguageValidVC.
bool vcLanguageValid(uint8_t consoleRegion, uint8_t language) noexcept;
/// Experience.IsValidNatureMetLevel2.
bool vcNatureValidAtMetLevel2(uint8_t growth, uint8_t nature) noexcept;
/// Per-game limits below the context's (X/Y below OR/AS, Sun/Moon below USUM).
uint16_t gameMaxSpecies(Modern::Game game) noexcept;
uint16_t gameMaxMove(Modern::Game game) noexcept;
uint16_t gameMaxItem(Modern::Game game) noexcept;
} // namespace Data

/// Analyze one PK6 (format 6) or PK7 (format 7) record. Called by Modern::analyze with
/// a validated format; see ModernLegality.h for the contract.
Report analyze(uint8_t format, std::span<const uint8_t> record, const Modern::Input& input);

} // namespace Legality::Gen67
