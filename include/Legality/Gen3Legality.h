/**
 * Gen3Legality.h - Gen III legality for PK3 records (Ruby, Sapphire, Emerald, FireRed,
 * LeafGreen and Colosseum/XD origins).
 *
 * Part of the shared legality analyzer: it returns the same Legality::Report,
 * Issue, Severity and CoverageSummary as analyze(), so callers read one verdict
 * model. The input is a decrypted PK3 record (80-byte box or 100-byte party).
 * The record is only read, never modified, and nothing is written back.
 *
 * Rules and data follow PKHeX.Core at the commit pinned in Gen3LegalityData.h and
 * Gen3LegalityDataCxd.h (see docs/legality/GEN3_PKHEX_RULE_INVENTORY.md).
 * A source-backed contradiction is reported as Invalid; anything this analyzer
 * cannot decide (unknown game versions, search limits) is reported as a Warning with
 * partial coverage. Search uncertainty does not negate independent native bounds.
 * A Colosseum/XD origin (version 15) is judged
 * with PKHeX's GameCube templates, team locks and RNG (EncounterGenerator3GC); the
 * same rules serve later formats through Gen3Cxd.h.
 */
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "Legality/Legality.h"

namespace Pokemon { class Pokemon; }

namespace Legality {

namespace Gen3Pk3 {

/// Decrypted, bounds-checked snapshot of every PK3 field the analyzer reads.
struct Pk3 {
    bool party = false;
    uint32_t pid = 0;
    uint16_t tid = 0, sid = 0;
    std::array<uint8_t, 10> nicknameRaw{};
    uint8_t language = 0, flags = 0;
    std::array<uint8_t, 7> otRaw{};
    uint8_t markings = 0;
    uint16_t checksum = 0, speciesInternal = 0, species = 0, heldItem = 0;
    uint32_t exp = 0;
    uint8_t ppUps = 0, friendship = 0;
    std::array<uint16_t, 4> moves{};
    std::array<uint8_t, 4> pp{};
    std::array<uint8_t, 6> evs{};      // HP Atk Def Spe SpA SpD
    std::array<uint8_t, 6> contest{};  // Cool Beauty Cute Smart Tough Sheen
    uint8_t pokerus = 0, metLocation = 0, metLevel = 0, version = 0, ball = 0;
    uint8_t otGender = 0;
    uint32_t iv32 = 0;
    std::array<uint8_t, 6> ivs{};      // HP Atk Def Spe SpA SpD
    bool isEgg = false;
    uint8_t abilityBit = 0;
    uint32_t ribbons = 0;
    bool fateful = false;
    uint8_t partyLevel = 0;
    // Knowledge flags for Gen3Cxd::judge, which builds a Pk3 from a later format's fields.
    // read() leaves both true: a PK3 in a Gen III save carries its own met fields and moves.
    bool metKnown = true;    // false: met level/location are a transfer's (PKHeX Format != 3 rules)
    bool movesKnown = true;  // false: the moves are not Gen III moves; move checks are skipped

    // Derived exactly like PKHeX's PK3 accessors.
    uint8_t form = 0, gender = 0, nature = 0, currentLevel = 1;
    bool shiny = false;
    std::u16string nickname, ot; // decoded with the record's language table
    bool isNicknamed = false;
};

/// PK3 checksum: 16-bit sum of the decrypted words at 0x20..0x4F.
uint16_t checksum(std::span<const uint8_t> decrypted) noexcept;

/// Reads a decrypted 80/100-byte record. Returns false for any other size.
bool read(std::span<const uint8_t> decrypted, Pk3& out);

/// PKHeX StringConverter3.LoadString: decodes a Gen III string buffer.
std::u16string decodeString(std::span<const uint8_t> raw, uint8_t language);

/// PKHeX MethodFinder.Analyze result (PIDType index as in Gen3LegalityData.h).
struct PidIv {
    uint8_t type = 0;
    uint8_t mutated = 0;
    uint32_t originSeed = 0;
};
PidIv analyzePidIv(const Pk3& pk);

/// WordFilter.IsFiltered for a Gen III name (Switch list, then 3DS list; species names exempt).
bool isWordFiltered(std::u16string_view message);

/// Number of pinned word-filter patterns the matcher cannot evaluate (0 for the pinned lists).
std::size_t unsupportedWordFilterPatterns();

/// The species itself, then every Gen III pre-evolution (EvolutionTree3, form-0 sources),
/// nearest first. Unknown species ids return just the id.
std::vector<uint16_t> lineage(uint16_t species);

/// One run of the analyzer on a stored record, made once by Legality::analyze so its
/// older Gen III checks (the L1 encounter table, the pret move pools) can yield to it.
struct Verification {
    bool ran = false;        ///< the record is an 80/100-byte PK3 in the Gen III save group
    bool emptySlot = false;  ///< internal species index 0: an empty slot, nothing to validate
    Report report;           ///< analyzePk3's report (empty when !ran or emptySlot)
    /// Every coverage area Complete with no Warning or Invalid: the analyzer matched an
    /// encounter for this exact record, verified its moves and every other rule, and the
    /// source is a GBA game.
    bool certified = false;
    /// The matched encounter, e.g. "in-game trade Nidoran F at met location 254, level 5";
    /// empty when the analyzer matched none.
    std::string encounter;
    /// The match is an in-game trade. PKHeX's EncounterTrade3 does not check the met
    /// location, which the games always set to 254 (see GEN3_PKHEX_RULE_INVENTORY.md 4b).
    bool inGameTrade = false;
};

/// Runs the analyzer on pk's stored bytes. Not a PK3 (or not decryptable): ran == false.
Verification verify(const Pokemon::Pokemon& pk, std::string_view exactSourceGameId,
                    const Gen3BerryContext* berryContext = nullptr);

} // namespace Gen3Pk3

/// Analyzes a decrypted PK3 record. exactSourceGameId names the save the record
/// was read from (ruby_gba, sapphire_gba, emerald_gba, firered_gba or
/// leafgreen_gba); without one the report can never reach No problems found.
Report analyzePk3(std::span<const uint8_t> decrypted, std::string_view exactSourceGameId,
                  const Gen3BerryContext* berryContext = nullptr);

/// Production hook for Legality::analyze: merges the PK3 analysis into r.
/// Adds the analyzer's Invalid and Warning issues (deduplicated by text) and
/// marks every coverage area Complete only when the analyzer verified a
/// legitimate origin with every area checked, nothing is wrong, r itself has
/// no Invalid or Warning, and the source is a GBA game. Anything else leaves
/// r's coverage untouched. Records that are not PK3, and empty PK3 slots, are ignored.
void applyGen3Analyzer(Report& r, const Gen3Pk3::Verification& verified, std::string_view exactSourceGameId);

/// Same as above, running verify() first.
void applyGen3Analyzer(Report& r, const Pokemon::Pokemon& pk, std::string_view exactSourceGameId);

} // namespace Legality
