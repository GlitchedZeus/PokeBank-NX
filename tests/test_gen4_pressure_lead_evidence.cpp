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
    using Legality::Gen4WildRng::sequentialPid;

    // Pinned EncounterSlot4.PressureLevel is exact from LevelMax for every
    // non-Grass family. The row-only API remains conservative for Grass because
    // parent-area identity lives only in the generated aligned source metadata.
    constexpr uint64_t grass = makeRow(0, 0, 5, 8);
    constexpr uint64_t surf = makeRow(1, 0, 5, 10);
    static_assert(exactPressureLevel(grass) == 0);
    static_assert(exactPressureLevel(surf) == 10);
    static_assert(kSourceCount == kPressureCount);

    // Method J Old Rod: seed 492 has a regular nature frame, Prev1=55173 with
    // the high bit set (Pressure/Hustle/Vital Spirit passes), Prev3=35488 which
    // selects slot 0, and Prev4=12631 which gives a normal 19% Old Rod hook.
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

    // Method K Old Rod uses low-bit parity for the lead predicate. Seed 53 has
    // Prev1=30387 (odd => pressure passes), Prev3=14714 selecting slot 0, and
    // Prev4=64904 => roll 4, a normal HG/SS Old Rod activation.
    constexpr uint32_t kSeed = 53u;
    constexpr uint32_t kPid = sequentialPid(kSeed);
    constexpr uint64_t kOldRod = makeRow(2, 0, 5, 10);
    constexpr auto kFish = matchRow(true, kOldRod, kSeed, kPid, 10);
    static_assert(kFish.matched());
    static_assert(kFish.slot == 0);
    static_assert(kFish.pressureLevel == 10);

    // The same Method K frame proves Rock Smash only through the normal
    // activation branch; the active Pressure-family lead cannot also be Illuminate.
    constexpr uint64_t kRock = makeRow(5, 0, 5, 10, 20);
    constexpr auto rock = matchRow(true, kRock, kSeed, kPid, 10);
    static_assert(rock.matched());
    static_assert(rock.slot == 0);

    // Headbutt has no separate encounter activation check after slot selection.
    constexpr uint64_t kHeadbutt = makeRow(6, 0, 5, 10);
    static_assert(matchRow(
        true, kHeadbutt, kSeed, kPid, 10).matched());

    // Method J Surf uses the same successful-pressure frame family. Seed 81 has
    // Prev1=37640 (high bit set) and Prev3=3946, selecting Surf slot 0.
    constexpr uint32_t jSurfSeed = 81u;
    constexpr uint32_t jSurfPid = sequentialPid(jSurfSeed);
    constexpr auto jSurf = matchRow(
        false, surf, jSurfSeed, jSurfPid, 10);
    static_assert(jSurf.matched());
    static_assert(jSurf.slot == 0);

    // Honey Tree species/slot is selected outside the normal ESV routine, but
    // pinned Method J still permits the successful pressure lead branch and uses
    // the source LevelMax as PressureLevel for this non-Grass SlotType4 family.
    constexpr uint64_t honeyTree = makeRow(9, 3, 5, 15);
    constexpr auto honey = matchRow(
        false, honeyTree, jSurfSeed, jSurfPid, 15);
    static_assert(honey.matched());
    static_assert(honey.slot == 3);
    static_assert(honey.pressureLevel == 15);

    // Preserve the prior row-only contract: without an indexed source alias,
    // Grass parent-area PressureLevel is not guessed.
    static_assert(!matchRow(
        false, grass, jSurfSeed, jSurfPid, 8).matched());
    static_assert(!matchRow(
        true, grass, kSeed, kPid, 8).matched());

    // HG/SS Bug Contest and Safari remain isolated until their minimum-31 rerolls
    // and lead-dependent activation/deadlock histories are reconstructed.
    constexpr uint64_t contest = makeRow(8, 0, 7, 18, 25);
    constexpr uint64_t safari = makeRow(10, 0, 15, 15, 6);
    static_assert(!matchRow(
        true, contest, kSeed, kPid, 18).matched());
    static_assert(!matchRow(
        true, safari, kSeed, kPid, 15).matched());

    // D/P/Pt Rock Smash is not a Method J family and must not be invented.
    static_assert(!matchRow(
        false, kRock, jFishSeed, jFishPid, 10).matched());

    // Exercise the generated source data itself, not just synthetic rows. Find an
    // exact Grass alias where parent-area PressureLevel is above this row's own
    // LevelMax. Such an encounter is rejected by ordinary levelMatches(), but a
    // successful Pressure lead can legally produce the generated higher level.
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

    // Derive a deterministic legal seed against the real generated source row.
    // The bound is intentionally finite; failure remains a test failure, never a
    // runtime legality assertion about arbitrary user data.
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

    // Runtime assertions keep both retail methods exercised under ASan/UBSan.
    assert(matchRow(false, jOldRod, jFishSeed, jFishPid, 10).matched());
    assert(matchRow(true, kRock, kSeed, kPid, 10).matched());

    std::cout << "Gen IV successful pressure lead evidence: PASS\n";
}
