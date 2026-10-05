#include "Legality/Gen4PressureLeadEvidence.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string_view>

namespace {
constexpr uint64_t makeRow(uint8_t type, uint8_t slot,
                           uint8_t minimum, uint8_t maximum,
                           uint8_t rate = 0) {
    return (static_cast<uint64_t>(minimum) << 17) |
           (static_cast<uint64_t>(maximum) << 24) |
           (static_cast<uint64_t>(type) << 31) |
           (static_cast<uint64_t>(slot) << 46) |
           (static_cast<uint64_t>(rate) << 50);
}

constexpr std::string_view gameId(Legality::Gen4Wild::Game game) {
    using Game = Legality::Gen4Wild::Game;
    switch (game) {
        case Game::Diamond: return "diamond_nds";
        case Game::Pearl: return "pearl_nds";
        case Game::Platinum: return "platinum_nds";
        case Game::HeartGold: return "heartgold_nds";
        case Game::SoulSilver: return "soulsilver_nds";
        case Game::Invalid: break;
    }
    return {};
}
}

int main() {
    using namespace Legality::Gen4PressureLead;
    using Legality::Gen4LeadFrame::sequentialPid;

    constexpr uint64_t grass = makeRow(0, 0, 5, 8);
    constexpr uint64_t surf = makeRow(1, 0, 5, 10);
    static_assert(exactPressureLevel(grass) == 0);
    static_assert(exactPressureLevel(surf) == 10);
    static_assert(kSourceCount == kPressureCount);

    constexpr uint32_t jFishSeed = 492u;
    constexpr uint32_t jFishPid = sequentialPid(jFishSeed);
    constexpr uint64_t jOldRod = makeRow(2, 0, 5, 10);
    constexpr auto jFish = matchRow(
        false, jOldRod, jFishSeed, jFishPid, 10);
    static_assert(jFish.matched());
    static_assert(jFish.slot == 0);
    static_assert(jFish.pressureLevel == 10);
    static_assert(!matchRow(
        false, jOldRod, jFishSeed, jFishPid, 9).matched());

    constexpr uint32_t kSeed = 53u;
    constexpr uint32_t kPid = sequentialPid(kSeed);
    constexpr uint64_t kOldRod = makeRow(2, 0, 5, 10);
    constexpr auto kFish = matchRow(true, kOldRod, kSeed, kPid, 10);
    static_assert(kFish.matched());
    static_assert(kFish.slot == 0);
    static_assert(kFish.pressureLevel == 10);

    constexpr uint64_t kRock = makeRow(5, 0, 5, 10, 20);
    constexpr auto rock = matchRow(true, kRock, kSeed, kPid, 10);
    static_assert(rock.matched());
    static_assert(rock.slot == 0);

    constexpr uint64_t kHeadbutt = makeRow(6, 0, 5, 10);
    static_assert(matchRow(
        true, kHeadbutt, kSeed, kPid, 10).matched());

    constexpr uint32_t jSurfSeed = 81u;
    constexpr uint32_t jSurfPid = sequentialPid(jSurfSeed);
    constexpr auto jSurf = matchRow(
        false, surf, jSurfSeed, jSurfPid, 10);
    static_assert(jSurf.matched());
    static_assert(jSurf.slot == 0);

    constexpr uint64_t honeyTree = makeRow(9, 3, 5, 15);
    constexpr auto honey = matchRow(
        false, honeyTree, jSurfSeed, jSurfPid, 15);
    static_assert(honey.matched());
    static_assert(honey.slot == 3);
    static_assert(honey.pressureLevel == 15);

    static_assert(!matchRow(
        false, grass, jSurfSeed, jSurfPid, 8).matched());
    static_assert(!matchRow(
        true, grass, kSeed, kPid, 8).matched());

    constexpr uint64_t contest = makeRow(8, 0, 7, 18, 25);
    constexpr uint64_t safari = makeRow(10, 0, 15, 15, 6);
    static_assert(!matchRow(
        true, contest, kSeed, kPid, 18).matched());
    static_assert(!matchRow(
        true, safari, kSeed, kPid, 15).matched());
    static_assert(!matchRow(
        false, kRock, jFishSeed, jFishPid, 10).matched());

    std::size_t boostedIndex = kSourceCount;
    for (std::size_t i = 0; i < kSourceCount; ++i) {
        const uint64_t row = Legality::Gen4Wild::kPackedGen4WildEncounters[i];
        const uint8_t pressure = sourcePressureLevel(i);
        if (Legality::Gen4Wild::method(row) == 0 &&
            pressure > Legality::Gen4Wild::maxLevel(row)) {
            boostedIndex = i;
            break;
        }
    }
    assert(boostedIndex < kSourceCount);

    const uint64_t boostedRow =
        Legality::Gen4Wild::kPackedGen4WildEncounters[boostedIndex];
    const uint8_t boostedLevel = sourcePressureLevel(boostedIndex);
    assert(boostedLevel > Legality::Gen4Wild::maxLevel(boostedRow));
    assert(!Legality::Gen4Wild::levelMatches(boostedRow, boostedLevel));

    const auto sourceGame = Legality::Gen4Wild::game(boostedRow);
    const bool sourceHgss =
        sourceGame == Legality::Gen4Wild::Game::HeartGold ||
        sourceGame == Legality::Gen4Wild::Game::SoulSilver;

    bool foundSeed = false;
    uint32_t grassSeed = 0;
    uint32_t grassPid = 0;
    for (uint32_t seed = 0; seed < 0x40000u; ++seed) {
        const uint32_t pid = sequentialPid(seed);
        if (!matchRowWithPressure(
                sourceHgss, boostedRow, boostedLevel,
                seed, pid, boostedLevel).matched())
            continue;
        foundSeed = true;
        grassSeed = seed;
        grassPid = pid;
        break;
    }
    assert(foundSeed);

    const uint8_t encounterForm = Legality::Gen4Wild::form(boostedRow);
    const uint8_t pokemonForm = encounterForm >= 30 ? 0 : encounterForm;
    const auto generatedGrass = analyzeSupported(
        gameId(sourceGame),
        Legality::Gen4Wild::species(boostedRow),
        Legality::Gen4Wild::location(boostedRow),
        boostedLevel,
        pokemonForm,
        0,
        grassSeed,
        grassPid);
    assert(generatedGrass.matched());
    assert(generatedGrass.pressureLevel == boostedLevel);
    assert(generatedGrass.slot == Legality::Gen4Wild::slot(boostedRow));

    static_assert(leadName(Lead::PressureHustleVitalSpirit)[0] == 'P');
    assert(matchRow(false, jOldRod, jFishSeed, jFishPid, 10).matched());
    assert(matchRow(true, kRock, kSeed, kPid, 10).matched());

    std::cout << "Gen IV successful pressure lead evidence: PASS\n";
}
