/**
 * Gen5Legality.h - Gen V (Black, White, Black 2, White 2) legality for PK5 records.
 *
 * Part of the shared legality analyzer: it returns the same Legality::Report,
 * Issue, Severity and CoverageSummary as analyze() for Gen I-IV, so callers read
 * one verdict model. The input is a raw PK5 record (136-byte box or 220-byte
 * party). The record is only read, never modified, and nothing is written back.
 *
 * Rules and data follow PKHeX.Core at the commit pinned in
 * Gen5LegalityData.h (see docs/legality/GEN5_PKHEX_RULE_INVENTORY.md).
 * A check this analyzer cannot decide is reported as a Warning with partial
 * coverage, never as Invalid.
 */
#pragma once

#include <array>
#include <cstdint>
#include <span>

#include "Legality/Legality.h"

namespace Legality {

namespace Gen5 {

/// Decrypted, bounds-checked snapshot of every PK5 field the analyzer reads.
struct Pk5 {
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
    std::array<uint8_t, 6> ivs{};      // HP Atk Def Spe SpA SpD
    bool isEgg = false, isNicknamed = false;
    bool fateful = false;
    uint8_t gender = 0, form = 0, nature = 0;
    bool hiddenAbility = false, nSparkle = false;
    uint8_t version = 0;
    std::array<uint8_t, 3> eggDate{}, metDate{};
    uint16_t eggLocation = 0, metLocation = 0;
    uint8_t pokerus = 0, ball = 0, metLevel = 0;
    bool otGender = false;
    uint8_t groundTile = 0, pokeStarFame = 0;
    uint8_t partyLevel = 0;
    std::array<uint16_t, 11> nickname{};
    std::array<uint16_t, 8> ot{};
    std::array<uint8_t, 12> ribbons{}; // 0x24-0x27, 0x3C-0x3F, 0x60-0x63
};

/// Checksum over the decrypted storage words 0x08..0x87.
uint16_t checksum(std::span<const uint8_t> decrypted) noexcept;

/// Reads a decrypted 136/220-byte record. Returns false for any other size.
bool read(std::span<const uint8_t> decrypted, Pk5& out) noexcept;

/// PKHeX MonochromeRNG.GetShinyPID: the PID a Gen V "always shiny" gift uses.
constexpr uint32_t shinyPid(uint32_t low, uint32_t abilityBit, uint16_t tid, uint16_t sid) noexcept {
    uint32_t pid = ((low ^ tid ^ sid) << 16) | low;
    if ((pid & 0x10000u) != ((abilityBit & 1u) << 16)) pid ^= 0x10000u;
    return pid;
}

constexpr bool isShiny(uint32_t pid, uint16_t tid, uint16_t sid) noexcept {
    return ((tid ^ sid ^ (pid >> 16) ^ (pid & 0xFFFFu)) & 0xFFFFu) < 8;
}

/// PKHeX MonochromeRNG.GetBitXor == 0: the Gen V wild PID correlation.
constexpr bool wildPidCorrelated(uint32_t pid, uint16_t tid, uint16_t sid) noexcept {
    return ((((tid ^ sid) & 1u) ^ (pid & 1u) ^ (pid >> 31)) & 1u) == 0;
}

/// PKHeX EggHatchLocation5: may an egg hatch at `location` in `version` (20-23)?
bool validHatchLocation(uint16_t location, uint8_t version) noexcept;

} // namespace Gen5

struct Pk5Input {
    /// True for a record as stored in a save (encrypted and shuffled); false for
    /// an already-decrypted record.
    bool encrypted = true;
};

/// Analyze one PK5 record. An all-zero or species-0 record returns an empty report.
Report analyzePk5(std::span<const uint8_t> record, const Pk5Input& input = {});

} // namespace Legality
