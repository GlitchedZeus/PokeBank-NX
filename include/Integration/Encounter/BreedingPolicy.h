#pragma once
// Round 13 owner decision 5 ("Add proper egg-group and breeding compatibility data. No generic egg
// histories presented as verified."). A pure breeding / egg-eligibility policy over the generated
// egg-group table (include/Data/EggGroups.h, tools/gen_egg_groups.py; PKHeX personal tables
// cross-checked against pret/pokecrystal and pret/pokeemerald).
//
// A species is HATCHABLE in a generation when some Pokémon of that generation's National Dex can
// parent a Day-Care egg that hatches as it:
//   * the parent is not in the Undiscovered group (legendaries, Unown, Nidorina / Nidoqueen and the
//     baby species themselves) and is not Ditto (Ditto only partners another parent);
//   * the parent has a compatible partner: Ditto (any parent, genderless ones only with Ditto), or a
//     Pokémon of the opposite gender sharing an egg group (compatible());
//   * the egg hatches as the first stage of the parent's family in that generation (baseOf()), so
//     Pichu, Cleffa, Igglybuff, Togepi, Tyrogue, Smoochum, Elekid, Magby (Gen II+) and Riolu (Gen IV+)
//     come from their evolved parents;
//   * incense babies (PKHeX 6501f0ab Legality/Breeding.cs IsSplitBreedNotBabySpecies3 / 4): without
//     the incense the egg hatches as the stage after the baby. Gen III: Marill (Sea Incense ->
//     Azurill), Wobbuffet (Lax Incense -> Wynaut). Gen IV / V add Chansey (Happiny), Mr. Mime
//     (Mime Jr.), Snorlax (Munchlax), Sudowoodo (Bonsly), Mantine (Mantyke), Roselia (Budew),
//     Chimecho (Chingling). Both the baby and the stage after it are hatchable;
//   * Manaphy's eggs hatch as Phione (Gen IV / V); Manaphy itself only ever came from its event egg;
//   * Nidoran♀ / Nidoran♂ and Volbeat / Illumise eggs can hatch as either species of the pair
//     (PKHeX Breeding.IsGenderSpeciesDetermination).
// Hatch level (PKHeX EncounterEgg2..5 / EggStateLegality): Gen II / III eggs hatch at Lv. 5, Gen IV /
// V eggs at Lv. 1; a Pokémon below that level never hatched.
//
// This informs the editor; it never validates or rewrites a Pokémon.
#include "Data/EggGroups.h"

#include <cstdint>

namespace PokeVault::Integration::BreedingPolicy {

namespace EggData = PokeVault::Data::EggGroups;

inline constexpr uint8_t kEggGroupDitto = 13;
inline constexpr uint8_t kEggGroupUndiscovered = 15;
inline constexpr uint8_t kGenderMaleOnly = 0;
inline constexpr uint8_t kGenderFemaleOnly = 254;
inline constexpr uint8_t kGenderless = 255;
inline constexpr uint16_t kManaphy = 490;
inline constexpr uint16_t kPhione = 489;

constexpr bool inGeneration(uint8_t generation, uint16_t species) noexcept {
    return species != 0 && species <= EggData::maxSpecies(generation);
}
constexpr EggData::Row row(uint8_t generation, uint16_t species) noexcept {
    return inGeneration(generation, species) ? EggData::table(generation)[species] : EggData::Row{0, 0, 0, 0};
}
constexpr bool undiscovered(uint8_t generation, uint16_t species) noexcept {
    const auto r = row(generation, species);
    return r.egg1 == kEggGroupUndiscovered || r.egg2 == kEggGroupUndiscovered;
}
constexpr bool isDitto(uint8_t generation, uint16_t species) noexcept {
    return inGeneration(generation, species) && row(generation, species).egg1 == kEggGroupDitto;
}
constexpr bool canBeMale(uint8_t generation, uint16_t species) noexcept {
    const auto g = row(generation, species).gender;
    return g != kGenderFemaleOnly && g != kGenderless;
}
constexpr bool canBeFemale(uint8_t generation, uint16_t species) noexcept {
    const auto g = row(generation, species).gender;
    return g != kGenderMaleOnly && g != kGenderless;
}
constexpr bool shareEggGroup(uint8_t generation, uint16_t a, uint16_t b) noexcept {
    const auto x = row(generation, a), y = row(generation, b);
    return x.egg1 == y.egg1 || x.egg1 == y.egg2 || x.egg2 == y.egg1 || x.egg2 == y.egg2;
}

// Two Pokémon of this generation can produce an egg together at the Day-Care.
constexpr bool compatible(uint8_t generation, uint16_t a, uint16_t b) noexcept {
    if (!inGeneration(generation, a) || !inGeneration(generation, b)) return false;
    if (undiscovered(generation, a) || undiscovered(generation, b)) return false;
    const bool dittoA = isDitto(generation, a), dittoB = isDitto(generation, b);
    if (dittoA && dittoB) return false;
    if (dittoA || dittoB) return true;
    if (!shareEggGroup(generation, a, b)) return false;
    return (canBeMale(generation, a) && canBeFemale(generation, b)) ||
           (canBeFemale(generation, a) && canBeMale(generation, b));
}

// A Pokémon that can parent an egg: some partner of this generation is compatible with it (Ditto is
// always one for a non-Ditto, non-Undiscovered parent).
constexpr bool canParentEgg(uint8_t generation, uint16_t species) noexcept {
    return inGeneration(generation, species) && !undiscovered(generation, species) && !isDitto(generation, species);
}

constexpr uint16_t prevo(uint8_t generation, uint16_t species) noexcept { return row(generation, species).prevo; }

// The first stage of the species' family in this generation.
constexpr uint16_t baseOf(uint8_t generation, uint16_t species) noexcept {
    uint16_t current = species;
    for (int depth = 0; depth < 8 && prevo(generation, current) != 0; ++depth) current = prevo(generation, current);
    return current;
}

// Incense parents (PKHeX Breeding.IsSplitBreedNotBabySpecies3 / 4), by the stage after the baby.
constexpr bool incenseStage(uint8_t generation, uint16_t species) noexcept {
    if (generation == 3) return species == 183 || species == 202;   // Marill, Wobbuffet
    if (generation == 4 || generation == 5)
        return species == 183 || species == 202 || species == 113 || species == 122 || species == 143 ||
               species == 185 || species == 226 || species == 315 || species == 358;
    return false;
}

// The species an egg parented by `parent` hatches as, with or without the parent's incense held.
// 0 when the parent cannot parent an egg.
constexpr uint16_t eggSpecies(uint8_t generation, uint16_t parent, bool incense) noexcept {
    if (!canParentEgg(generation, parent)) return 0;
    if (parent == kManaphy && (generation == 4 || generation == 5)) return kPhione;
    const uint16_t base = baseOf(generation, parent);
    if (incense || base == parent) return base;
    // The stage on the parent's own line whose pre-evolution is the base.
    uint16_t stage = parent;
    for (int depth = 0; depth < 8 && prevo(generation, stage) != base; ++depth) stage = prevo(generation, stage);
    return incenseStage(generation, stage) ? stage : base;
}

// The other species of a gender-determined pair (PKHeX Breeding.IsGenderSpeciesDetermination).
constexpr uint16_t genderPair(uint16_t species) noexcept {
    return species == 29 ? 32 : species == 32 ? 29 : species == 313 ? 314 : species == 314 ? 313 : 0;
}

constexpr bool hatchable(uint8_t generation, uint16_t species) noexcept {
    if (!inGeneration(generation, species)) return false;
    const uint16_t pair = genderPair(species);
    for (uint16_t parent = 1; parent <= EggData::maxSpecies(generation); ++parent) {
        for (int incense = 0; incense < 2; ++incense) {
            const uint16_t egg = eggSpecies(generation, parent, incense != 0);
            if (egg != 0 && (egg == species || (pair != 0 && egg == pair))) return true;
        }
    }
    return false;
}

constexpr uint8_t hatchLevel(uint8_t generation) noexcept {
    return generation == 2 || generation == 3 ? 5 : generation == 4 || generation == 5 ? 1 : 0;
}

enum class Reason : uint8_t {
    Hatchable,          // the species or a pre-evolution hatches from a Day-Care egg here
    NotInGeneration,    // species is not in this generation's National Dex
    Undiscovered,       // no member of its family can parent an egg
    Ditto,              // Ditto never hatches from an egg
    HatchesAsOther,     // eggs of its family hatch as another species (Manaphy -> Phione)
    LevelBelowHatch,    // the Pokémon's level is below the hatch level
};

struct EggVerdict {
    Reason reason = Reason::NotInGeneration;
    uint16_t hatchedAs = 0;   // the species (itself or a pre-evolution) that hatched; 0 when none
    uint16_t eggsHatchAs = 0; // HatchesAsOther: what its family's eggs hatch as
    constexpr bool verified() const noexcept { return reason == Reason::Hatchable; }
};

// May this Pokémon (species, current level; 0 = level not known, not judged) have hatched from a
// Day-Care egg in a game of this generation? An evolved Pokémon qualifies through the closest
// pre-evolution that is hatchable (Raichu via Pichu, Azumarill via Marill in Gen III).
constexpr EggVerdict eggHistory(uint8_t generation, uint16_t species, uint8_t currentLevel) noexcept {
    EggVerdict verdict;
    if (!inGeneration(generation, species)) return verdict;
    uint16_t member = species;
    for (int depth = 0; depth < 8 && member != 0; ++depth, member = prevo(generation, member)) {
        if (hatchable(generation, member)) { verdict.hatchedAs = member; break; }
    }
    if (verdict.hatchedAs == 0) {
        if (isDitto(generation, species)) verdict.reason = Reason::Ditto;
        else if (species == kManaphy && (generation == 4 || generation == 5)) {
            verdict.reason = Reason::HatchesAsOther;
            verdict.eggsHatchAs = kPhione;
        } else verdict.reason = Reason::Undiscovered;
        return verdict;
    }
    verdict.reason = currentLevel != 0 && currentLevel < hatchLevel(generation) ? Reason::LevelBelowHatch
                                                                                : Reason::Hatchable;
    return verdict;
}

// Short reason text (fits beside a place name in the 620 px Met Location picker).
constexpr const char* reasonText(Reason reason) noexcept {
    switch (reason) {
        case Reason::Hatchable: return "hatchable";
        case Reason::NotInGeneration: return "not in this game";
        case Reason::Undiscovered: return "never hatches (Undiscovered)";
        case Reason::Ditto: return "Ditto never hatches";
        case Reason::HatchesAsOther: return "its eggs hatch as Phione";
        case Reason::LevelBelowHatch: return "level below hatch level";
    }
    return "unverified";
}

} // namespace PokeVault::Integration::BreedingPolicy
