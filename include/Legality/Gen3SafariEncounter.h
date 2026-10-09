#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace Legality::Gen3Safari {

inline constexpr uint8_t kSafariBall = 5;
inline constexpr uint8_t kSafariLocationRSE = 57;
inline constexpr uint8_t kSafariLocationFRLG = 136;

enum class Game : uint8_t {
    Ruby = 0,
    Sapphire = 1,
    Emerald = 2,
    FireRed = 3,
    LeafGreen = 4,
    Invalid = 0xFF,
};

constexpr Game gameForId(std::string_view id) noexcept {
    if (id == "ruby_gba") return Game::Ruby;
    if (id == "sapphire_gba") return Game::Sapphire;
    if (id == "emerald_gba") return Game::Emerald;
    if (id == "firered_gba") return Game::FireRed;
    if (id == "leafgreen_gba") return Game::LeafGreen;
    return Game::Invalid;
}

constexpr uint8_t safariLocation(Game game) noexcept {
    switch (game) {
        case Game::Ruby:
        case Game::Sapphire:
        case Game::Emerald:
            return kSafariLocationRSE;
        case Game::FireRed:
        case Game::LeafGreen:
            return kSafariLocationFRLG;
        case Game::Invalid:
            return 0;
    }
    return 0;
}

constexpr bool isSafariLocation(Game game, uint16_t location) noexcept {
    const uint8_t expected = safariLocation(game);
    return expected != 0 && location == expected;
}

// Mirrors the fields retained by pinned EncounterArea3.ReadRegularSlot.
// This table contains only source areas that PKHeX classifies as Safari.
struct Entry {
    uint16_t species;
    uint8_t location;
    uint8_t minLevel;
    uint8_t maxLevel;
    uint8_t method;
    uint8_t form;
    uint8_t slot;
    uint8_t rate;
    uint8_t magnetIndex;
    uint8_t magnetCount;
    uint8_t staticIndex;
    uint8_t staticCount;
    uint8_t game;
    // Original BinLinker encounter-area index. Area identity is not location:
    // one Safari location can contain separate grass/rod/region areas.
    uint16_t areaIndex;
};

#include "Legality/Gen3SafariEncounterData.inc"

constexpr bool levelMatches(const Entry& row, uint8_t level) noexcept {
    return level >= row.minLevel && level <= row.maxLevel;
}

struct Evidence {
    bool matched = false;
    uint8_t requiredBall = 0;
    std::size_t candidateRows = 0;
};

// Pinned PKHeX evidence:
// - Locations.IsSafariZoneLocation3 => RSE 57 / FRLG 136.
// - EncounterSlot3.FixedBall calls GetRequiredBall(), which returns Safari
//   for a Safari location.
// - Ball.Safari = 5.
//
// This helper proves direct wild Safari compatibility only. Static/gift/event,
// evolution, transfer, and other competing histories remain separate. Callers
// must not turn absence or a mismatched ball into Invalid without resolving
// those histories first.
inline Evidence directWildEvidence(std::string_view exactGameId,
                                   uint16_t speciesId,
                                   uint16_t metLocation,
                                   uint8_t metLevel,
                                   uint8_t pokemonForm) noexcept {
    const Game wanted = gameForId(exactGameId);
    if (wanted == Game::Invalid || speciesId == 0 || metLevel == 0 ||
        !isSafariLocation(wanted, metLocation))
        return {};

    std::size_t matches = 0;
    for (const Entry& row : kGen3SafariEntries) {
        if (row.game != static_cast<uint8_t>(wanted) ||
            row.species != speciesId ||
            row.location != metLocation ||
            !levelMatches(row, metLevel) ||
            row.form != pokemonForm)
            continue;
        ++matches;
    }

    if (matches == 0)
        return {};
    return {true, kSafariBall, matches};
}

inline std::size_t countForGame(std::string_view exactGameId) noexcept {
    const Game wanted = gameForId(exactGameId);
    if (wanted == Game::Invalid)
        return 0;

    std::size_t count = 0;
    for (const Entry& row : kGen3SafariEntries)
        if (row.game == static_cast<uint8_t>(wanted))
            ++count;
    return count;
}

} // namespace Legality::Gen3Safari
