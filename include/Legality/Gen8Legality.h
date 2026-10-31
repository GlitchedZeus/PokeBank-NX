/**
 * Gen8Legality.h - Generation VIII Sword/Shield (PK8) native legality.
 *
 * Reached through Legality::Modern::analyze (ModernLegality.h). The record codec is the
 * existing Gen VIII PK8 decoder (Integration/Gen8/Gen8PK8.h). Rule data (PersonalTable.SWSH
 * abilities, gender ratios, form counts, growth rates, base stats, presence in the game and
 * TR permissions, base PP, released held items, form items, battle-only forms, mint natures
 * and ID limits) is generated from the pinned PKHeX.Core by tools/pkhex_gen67_diff/oracle
 * ("dump8") into Gen8LegalityData.inc.
 *
 * Every Invalid rule mirrors a PKHeX.Core verifier at the pinned commit and depends only on
 * fields the record itself fixes; a rule that needs the encounter, an evolution chain, a
 * learnset, a distribution or the HOME server is not implemented and keeps the report
 * Incomplete. See docs/legality/gen69-foundation/README.md for the rule-by-rule source map.
 */
#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <span>

#include "Legality/ModernLegality.h"

namespace Legality::Gen8 {

/// PKHeX PKM.Generation of a PK8 record: the generation its version byte (and, for
/// Pokemon GO, its met location) names. 1-2 Virtual Console, 3-7 the handheld games and
/// Let's Go (7), 8 Sword/Shield, Legends: Arceus, Brilliant Diamond/Shining Pearl and GO
/// via HOME, 9 Legends: Z-A. 0 for a version no game writes into a PK8 (including Scarlet/
/// Violet, whose records carry Sword/Shield's version and a HOME met location in a PK8).
uint8_t originGeneration(uint8_t version, uint16_t metLocation) noexcept;

/// The game a HOME side transfer came from, by its LocationsHOME met location (59994-60000):
/// PLA, BD, SP, SL, VL, FR or LG (Switch Virtual Console). 0 for any other met location.
uint8_t sideTransferOrigin(uint16_t metLocation) noexcept;

namespace Data {
struct Personal {
    uint16_t ability1, ability2, abilityHidden;
    uint8_t gender, growth, formCount;
    bool present;                      // PersonalTable.SWSH.IsPresentInGame
    std::array<uint8_t, 6> baseStats;  // HP Atk Def Spe SpA SpD
};
/// PersonalTable.SWSH entry; nullopt for species 0, a species above 898 or a form outside
/// the species' form count.
std::optional<Personal> personal(uint16_t species, uint8_t form) noexcept;
/// Form count of the species' base entry (at least 1); 0 when the species is outside.
uint8_t formCount(uint16_t species) noexcept;
/// PersonalTable.SWSH.IsPresentInGame(species, form).
bool presentInGame(uint16_t species, uint8_t form) noexcept;
/// PersonalInfo8SWSH.GetIsLearnTR(index) for the species/form; false outside the table.
bool canLearnTechRecord(uint16_t species, uint8_t form, uint8_t index) noexcept;
uint8_t techRecordCount() noexcept;
/// MoveInfo.GetPP(EntityContext.Gen8, move); 0 above the last move.
uint8_t basePP(uint16_t move) noexcept;
uint16_t maxSpecies() noexcept;
uint16_t maxMove() noexcept;
uint16_t maxItem() noexcept;
uint8_t maxBall() noexcept;
/// ItemRestrictions.IsHeldItemAllowed(item, EntityContext.Gen8).
bool heldItemAllowed(uint16_t item) noexcept;
/// FormInfo.IsBattleOnlyForm(species, form, 8).
bool battleOnlyForm(uint16_t species, uint8_t form) noexcept;
/// FormItem.GetFormArceus(item, 8) / GetFormGenesect / GetFormSilvally.
uint8_t arceusForm(uint16_t item) noexcept;
uint8_t genesectForm(uint16_t item) noexcept;
uint8_t silvallyForm(uint16_t item) noexcept;
/// Nature.IsMint.
bool mintNature(uint8_t nature) noexcept;
} // namespace Data

/// Analyze one PK8 record. Called by Modern::analyze; see ModernLegality.h for the contract.
Report analyze(std::span<const uint8_t> record, const Modern::Input& input);

} // namespace Legality::Gen8
