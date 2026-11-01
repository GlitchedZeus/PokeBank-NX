/**
 * ModernEncounter.h - the encounter ball rule shared by the Gen VI-IX analyzers.
 *
 * BallVerifier.VerifyBall, made encounter-independent: for each format and national species,
 * the union of the balls every encounter EncounterMovesetGenerator yields for any form of the
 * species (wild ball sets per game, fixed balls, egg inheritance, Dream World, GO restrictions,
 * plus the Shedinja and Legends: Arceus -> Sword/Shield replacements). A ball outside that union
 * has no encounter at all. The same pass records whether any encounter takes the fateful flag
 * and whether any takes it clear, and the unhatched-egg facts of each species. Generated from the pinned PKHeX.Core by
 * tools/pkhex_gen67_diff/oracle ("dumpencounters") into ModernEncounterData.inc.
 */
#pragma once

#include <array>
#include <cstdint>

#include "Legality/ModernLegality.h"

namespace Legality::ModernEncounter {

/// False only when no encounter of the species in this format can carry the ball. A species
/// without any generated encounter, or outside the table, is never decided here.
bool ballPossible(Modern::Format format, uint16_t species, uint8_t ball) noexcept;

/// False only when no encounter of the species in this format accepts the fateful flag in this
/// state (set, or clear). Undecided cases as for ballPossible.
bool fatefulPossible(Modern::Format format, uint16_t species, bool fateful) noexcept;

/// MiscVerifierPK8.VerifyGigantamax over every encounter that can reach Sword/Shield (PK8): false
/// only when no encounter of the species and form gives this Gigantamax flag state and Gigantamax
/// Soup cannot toggle it (never on an unhatched egg). Undecided species/forms return true.
bool gigantamaxPossible(uint16_t species, uint8_t form, bool flag, bool egg) noexcept;

/// A Sword/Shield record's fields that a Max Raid seed fixes (RaidRNG.Verify).
struct RaidRecord {
    uint16_t species = 0;
    uint16_t metLocation = 0;
    uint32_t ec = 0, pid = 0, id32 = 0;
    std::array<uint8_t, 6> ivs{};  // HP, Atk, Def, SpA, SpD, Spe
    uint8_t form = 0, abilityNumber = 0, gender = 0, nature = 0, height = 0, weight = 0;
    bool homeTracker = false;
};

enum class RaidResult : uint8_t { Undecided, SeedFound, NoSeed };

/// EncounterStatic8Nest.TryGetSeed over every raid of the species' line that can carry the met
/// location: the shared den location 162 (dens, Watchtower crystals, distributions) or Max Lair
/// 244. Undecided for any other location, a shiny record (PKHeX does not check its seed), a
/// species/location some non-raid encounter also uses, or a species without generated encounters.
/// NoSeed: no seed from (EC, PID) generates the record's IVs, ability, gender, nature and sizes
/// for any of those raids' parameters.
RaidResult raidSeed(const RaidRecord& record) noexcept;

/// Unhatched-egg facts of the species in a format that has eggs (PK6, PK7, PK8, PB8, PK9), over
/// the egg encounters of that format's own games only: transfer services and HOME never accept an
/// egg, so an unhatched record is always in the format of the game it was laid in.
struct EggFacts {
    bool decided = false;        // false: the format has no egg table, or the species is undecided
    bool exists = false;         // some egg encounter (bred, static gift or mystery gift)
    bool gift = false;           // a mystery gift egg exists (its met level and locations follow the gift)
    bool nicknameFlagSet = false;    // EggStateLegality.IsNicknameFlagSet may be true
    bool nicknameFlagClear = false;  // ... may be false
    bool eggLocationZero = false;    // a gift egg may carry egg location 0
    // Over every encounter of the species or a pre-evolution in the format's own games:
    bool nonEggEggLocation = false;  // a non-egg encounter carries a nonzero egg location
    bool lineHasEgg = false;         // some egg encounter (the record may have hatched)
};
EggFacts eggFacts(Modern::Format format, uint16_t species) noexcept;

/// LearnVerifierRelearn for a Generation 6/7 origin (format PK6 or PK7 = the origin games): false
/// only when no encounter of the species' line in those games can leave this move in a relearn
/// slot. `fromEgg` picks the egg encounters (an egg's level-up and egg moves, a gift egg's fixed
/// moves) or the others (fixed relearn movesets, DexNav-style bonus moves). Undecided species and
/// other formats return true.
bool relearnPossible(Modern::Format origin, uint16_t species, uint16_t move, bool fromEgg) noexcept;

/// EncounterVerifier.VerifyEncounterEgg6/7/8/8BDSP/9 hatch-location lists: whether an egg of the
/// origin game (version id) hatches at this met location. Games without a list return true.
bool hatchLocation(uint8_t version, uint16_t location) noexcept;

} // namespace Legality::ModernEncounter
