#pragma once

#include "Legality/Gen4WildEncounter.h"

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace Legality::Gen4Static {

// Packed layout:
//   species      [0..8]   (9 bits)
//   location     [9..20]  (12 bits)
//   level        [21..27] (7 bits)
//   form         [28..35] (8 bits)
//   egg location [36..47] (12 bits)
//   roaming      [48]     (1 bit)
//   game         [49..51] (3 bits; Gen4Wild::Game)
//   fixed ball   [52..56] (5 bits; 0 = unrestricted)
struct StaticConstraint {
    uint64_t encounter = 0;
    uint8_t gender = 3; // FixedGenderUtil.GenderRandom
    uint8_t nature = 25; // Nature::Random
    uint8_t shiny = 0; // Shiny::Random / Never / Always
    bool fateful = false;
};

enum class PidCategory : uint8_t {
    None,
    Method1OrCuteCharm,
    Pokewalker,
    ChainShiny,
};

#include "Legality/Gen4StaticEncounterData.inc"

constexpr uint16_t species(uint64_t v) noexcept {
    return static_cast<uint16_t>(v & 0x1FFu);
}
constexpr uint16_t location(uint64_t v) noexcept {
    return static_cast<uint16_t>((v >> 9) & 0x0FFFu);
}
constexpr uint8_t level(uint64_t v) noexcept {
    return static_cast<uint8_t>((v >> 21) & 0x7Fu);
}
constexpr uint8_t form(uint64_t v) noexcept {
    return static_cast<uint8_t>((v >> 28) & 0xFFu);
}
constexpr uint16_t eggLocation(uint64_t v) noexcept {
    return static_cast<uint16_t>((v >> 36) & 0x0FFFu);
}
constexpr bool roaming(uint64_t v) noexcept {
    return ((v >> 48) & 1u) != 0;
}
constexpr Gen4Wild::Game game(uint64_t v) noexcept {
    return static_cast<Gen4Wild::Game>((v >> 49) & 0x07u);
}
constexpr uint8_t fixedBall(uint64_t v) noexcept {
    return static_cast<uint8_t>((v >> 52) & 0x1Fu);
}

// Pinned PKHeX FormInfo.FormChange species that can exist in Generation IV.
// A static origin form remains valid after one of these same-species form changes.
// Future-generation-only form-change species are intentionally omitted here.
constexpr bool formChangeableSpecies(uint16_t speciesId) noexcept {
    switch (speciesId) {
        case 386: // Deoxys
        case 412: // Burmy
        case 479: // Rotom
        case 483: // Dialga
        case 484: // Palkia
        case 487: // Giratina
        case 492: // Shaymin
        case 493: // Arceus
            return true;
        default:
            return false;
    }
}

constexpr bool formCompatible(uint16_t speciesId, uint8_t encounterForm,
                              uint8_t currentForm) noexcept {
    return encounterForm == currentForm || formChangeableSpecies(speciesId);
}

inline bool constraintsMatch(uint64_t encounter,
                             uint8_t pokemonGender,
                             uint8_t pokemonNature,
                             bool pokemonShiny,
                             bool pokemonFateful) noexcept {
    for (const auto& constraint : kGen4StaticConstraints) {
        if (constraint.encounter != encounter)
            continue;
        if (constraint.gender != 3 && pokemonGender != constraint.gender)
            return false;
        if (constraint.nature != 25 && pokemonNature != constraint.nature)
            return false;
        if (constraint.shiny == 1 && pokemonShiny)
            return false;
        if (constraint.shiny == 2 && !pokemonShiny)
            return false;
        return pokemonFateful == constraint.fateful;
    }
    return true;
}

constexpr bool roamerLocationAllowed(uint16_t originLocation, uint16_t metLocation) noexcept {
    // PKHeX EncounterStatic4 permitted-location masks at the pinned reference.
    // We use the union of Grass + Water routes because the current legality interface
    // does not expose PK4 GroundTile yet; this remains positive evidence only.
    if (originLocation == 16) { // Sinnoh roamers
        constexpr uint64_t permit =
            0x000000028033FFFFULL | 0x00000002803E3B9EULL;
        const int delta = static_cast<int>(metLocation) - 16;
        return delta >= 0 && delta < 64 && (permit & (uint64_t{1} << delta)) != 0;
    }
    if (originLocation == 177) { // Johto roamers
        constexpr uint32_t permit = 0x0003E7FFu | 0x0001E06Eu;
        const int delta = static_cast<int>(metLocation) - 177;
        return delta >= 0 && delta < 32 && (permit & (uint32_t{1} << delta)) != 0;
    }
    if (originLocation == 149) { // Kanto roamers
        constexpr uint32_t permit = 0x0AB3FFFFu | 0x0ABC1B28u;
        const int delta = static_cast<int>(metLocation) - 149;
        return delta >= 0 && delta < 32 && (permit & (uint32_t{1} << delta)) != 0;
    }
    return false;
}

inline bool hasSpecies(std::string_view exactGameId, uint16_t speciesId) noexcept {
    const auto wanted = Gen4Wild::gameForId(exactGameId);
    if (wanted == Gen4Wild::Game::Invalid || speciesId == 0) return false;
    for (const uint64_t row : kPackedGen4StaticEncounters)
        if (game(row) == wanted && species(row) == speciesId) return true;
    return false;
}

inline const uint64_t* findMatch(
                    std::string_view exactGameId, uint16_t speciesId,
                    uint16_t metLocation, uint8_t metLevel, uint8_t pokemonForm,
                    uint16_t pokemonEggLocation, uint8_t pokemonBall,
                    uint8_t pokemonGender, uint8_t pokemonNature,
                    bool pokemonShiny, bool pokemonFateful) noexcept {
    const auto wanted = Gen4Wild::gameForId(exactGameId);
    if (wanted == Gen4Wild::Game::Invalid || speciesId == 0)
        return nullptr;

    for (const uint64_t& row : kPackedGen4StaticEncounters) {
        if (game(row) != wanted || species(row) != speciesId ||
            !formCompatible(speciesId, form(row), pokemonForm))
            continue;

        const uint8_t requiredBall = fixedBall(row);
        if (requiredBall != 0 && pokemonBall != 0xFF && pokemonBall != requiredBall)
            continue;

        const uint16_t expectedEgg = eggLocation(row);
        if (expectedEgg != 0) {
            // Native PK4 eggs store encounter level 0 and preserve the gift egg location.
            // Hatched PK4s may have any valid hatch met location, so the static table's
            // Location=0 is not an exact met-location constraint after hatching.
            if (metLevel != 0 || pokemonEggLocation != expectedEgg)
                continue;
        } else {
            if (metLevel != level(row))
                continue;

            if (roaming(row)) {
                if (!roamerLocationAllowed(location(row), metLocation))
                    continue;
            } else if (location(row) != metLocation) {
                continue;
            }
        }

        if (!constraintsMatch(
                row, pokemonGender, pokemonNature, pokemonShiny, pokemonFateful))
            continue;
        return &row;
    }
    return nullptr;
}

inline bool matches(std::string_view exactGameId, uint16_t speciesId,
                    uint16_t metLocation, uint8_t metLevel, uint8_t pokemonForm,
                    uint16_t pokemonEggLocation, uint8_t pokemonBall,
                    uint8_t pokemonGender, uint8_t pokemonNature,
                    bool pokemonShiny, bool pokemonFateful) noexcept {
    return findMatch(
        exactGameId, speciesId, metLocation, metLevel, pokemonForm,
        pokemonEggLocation, pokemonBall, pokemonGender, pokemonNature,
        pokemonShiny, pokemonFateful) != nullptr;
}

inline PidCategory pidCategoryForRow(uint64_t row) noexcept {
    // Pinned PKHeX EncounterStatic4 correlation contract:
    // Pichu uses the Pokewalker PID formula, forced-shiny statics use
    // Chain Shiny, and ordinary statics prefer Method 1 while permitting
    // compatible Cute Charm PID surfaces as a separate class.
    if (species(row) == 172)
        return PidCategory::Pokewalker;

    for (const auto& constraint : kGen4StaticConstraints)
        if (constraint.encounter == row && constraint.shiny == 2)
            return PidCategory::ChainShiny;

    return PidCategory::Method1OrCuteCharm;
}

inline std::size_t countForGame(std::string_view exactGameId) noexcept {
    const auto wanted = Gen4Wild::gameForId(exactGameId);
    if (wanted == Gen4Wild::Game::Invalid) return 0;
    std::size_t count = 0;
    for (const uint64_t row : kPackedGen4StaticEncounters)
        if (game(row) == wanted) ++count;
    return count;
}

} // namespace Legality::Gen4Static
