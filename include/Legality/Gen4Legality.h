/**
 * Gen4Legality.h - Gen IV (Diamond, Pearl, Platinum, HeartGold, SoulSilver) legality for PK4 records.
 *
 * Part of the shared legality analyzer: it returns the same Legality::Report, Issue,
 * Severity and CoverageSummary as analyze(), so callers read one verdict model. The
 * input is a decrypted PK4 record (136-byte box or 236-byte party, canonical A/B/C/D
 * block order). The record is only read, never modified, and nothing is written back.
 *
 * Rules and data follow PKHeX.Core at the commit pinned in Gen4LegalityData.h: the
 * analyzer ports PKHeX's Gen IV encounter search (wild slots with the Method J/K
 * lead and frame checks, static and roaming encounters, Pokewalker, in-game trades,
 * My Pokemon Ranch, PCD/PGT gifts and eggs), its PID/IV correlation finder and the
 * verifiers PKHeX runs for a native PK4. A rule is reported Invalid only where PKHeX
 * reports it Invalid; a check this analyzer cannot decide (for example a Pal Park
 * transfer from a Gen III game) is reported as a Warning with partial coverage, never
 * as Invalid. Deliberate differences from PKHeX are lenient and documented next to
 * the rule (see docs/legality/GEN4_PKHEX_RULE_INVENTORY.md).
 */
#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <string_view>

#include "Legality/Legality.h"

namespace Pokemon {
class Pokemon;
}

namespace Legality {

namespace Gen4 {

/// Decrypted, bounds-checked snapshot of every PK4 field the analyzer reads.
struct Pk4 {
    bool party = false;
    uint32_t pid = 0;
    uint16_t sanity = 0, checksum = 0;
    uint16_t species = 0, heldItem = 0, tid = 0, sid = 0;
    uint32_t exp = 0;
    uint8_t friendship = 0, ability = 0, markings = 0, language = 0;
    std::array<uint8_t, 6> evs{};      // HP Atk Def Spe SpA SpD
    std::array<uint8_t, 6> contest{};  // Cool Beauty Cute Smart Tough Sheen
    std::array<uint16_t, 4> moves{};
    std::array<uint8_t, 4> pp{}, ppUps{};
    uint32_t iv32 = 0;                 // raw 0x38 (IVs, egg bit 30, nickname bit 31)
    std::array<uint8_t, 6> ivs{};      // HP Atk Def Spe SpA SpD
    bool isEgg = false, isNicknamed = false, fateful = false;
    uint8_t gender = 0, form = 0, shinyLeaf = 0;
    uint16_t eggLocationExtended = 0, metLocationExtended = 0;
    std::array<uint16_t, 11> nickname{};
    uint8_t version = 0;
    std::array<uint16_t, 8> ot{};
    std::array<uint8_t, 3> eggDate{}, metDate{}; // year-2000, month, day
    uint16_t eggLocationDP = 0, metLocationDP = 0;
    uint8_t pokerus = 0, ballDPPt = 0, metLevel = 0;
    uint8_t otGender = 0;
    uint8_t groundTile = 0, ballHGSS = 0;
    int8_t walkingMood = 0;
    std::array<uint8_t, 16> ribbons{}; // 0x24-0x27, 0x3C-0x3F, 0x60-0x63

    uint16_t eggLocation() const noexcept { return eggLocationExtended != 0 ? eggLocationExtended : eggLocationDP; }
    uint16_t metLocation() const noexcept { return metLocationExtended != 0 ? metLocationExtended : metLocationDP; }
    uint8_t ball() const noexcept { return ballHGSS > ballDPPt ? ballHGSS : ballDPPt; }
    uint8_t nature() const noexcept { return static_cast<uint8_t>(pid % 25u); }
    bool isShiny() const noexcept {
        return (((pid >> 16) ^ (pid & 0xFFFFu) ^ tid ^ sid) & 0xFFFFu) < 8;
    }
};

/// Add16 checksum over the decrypted storage words 0x08..0x87.
uint16_t checksum(std::span<const uint8_t> decrypted) noexcept;

/// Reads a decrypted 136/236-byte record. Returns false for any other size.
bool read(std::span<const uint8_t> decrypted, Pk4& out) noexcept;

/// Gen IV game ids PokeBank NX uses for exact-source analysis.
bool isGen4SourceGameId(std::string_view exactSourceGameId) noexcept;

} // namespace Gen4

/// Analyze one decrypted PK4 record. exactSourceGameId names the cartridge whose save
/// holds the record ("diamond_nds", "pearl_nds", "platinum_nds", "heartgold_nds",
/// "soulsilver_nds"); the origin game itself is read from the record. An all-zero or
/// species-0 record returns an empty report.
Report analyzePk4(std::span<const uint8_t> decrypted, std::string_view exactSourceGameId);

/// Folds the Gen IV analyzer into a report produced by analyze() for a native PK4
/// entity (no-op for any other entity). Invalid and Warning issues are added once each;
/// coverage becomes Complete only when the analyzer matched a legitimate origin with
/// every area checked and nothing wrong, and the existing report has no Invalid or
/// Warning issue. Otherwise the existing coverage is left as it is.
void applyGen4Analyzer(Report& report, const Pokemon::Pokemon& pk, std::string_view exactSourceGameId);

} // namespace Legality
