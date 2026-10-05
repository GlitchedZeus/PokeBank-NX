#include "Legality/Gen4LeadHistoryEvidence.h"

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

constexpr uint32_t makeMeta(uint8_t magnetIndex, uint8_t magnetCount,
                            uint8_t staticIndex, uint8_t staticCount) {
    return static_cast<uint32_t>(magnetIndex) |
           (static_cast<uint32_t>(magnetCount) << 8) |
           (static_cast<uint32_t>(staticIndex) << 16) |
           (static_cast<uint32_t>(staticCount) << 24);
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
} // namespace

int main() {
    using namespace Legality::Gen4LeadHistory;
    using Legality::Gen3PidIv::Detail::prev;
    using Legality::Gen4WildRng::sequentialPid;

    static_assert(kSourceCount == kLeadMetaCount);
    static_assert(kSourceCount == kPressureCount);

    // Reuse the accepted discriminating Method J Grass attraction vector.
    constexpr uint64_t grass = makeRow(0, 0, 5, 5);
    constexpr uint32_t jStaticSeed = 53u;
    constexpr uint32_t jStaticPid = sequentialPid(jStaticSeed);
    constexpr auto staticSuccess = matchIndexedRow(
        false, grass, makeMeta(0, 0, 0, 3), 5,
        jStaticSeed, jStaticPid, 5);
    static_assert(staticSuccess.has(Path::StaticSuccess));

    constexpr auto magnetSuccess = matchIndexedRow(
        false, grass, makeMeta(3, 4, 0, 0), 5,
        jStaticSeed, jStaticPid, 5);
    static_assert(magnetSuccess.has(Path::MagnetPullSuccess));

    // Reuse the accepted successful Method J Old Rod Pressure vector.
    constexpr uint32_t jPressureSeed = 492u;
    constexpr uint32_t jPressurePid = sequentialPid(jPressureSeed);
    constexpr uint64_t oldRod = makeRow(2, 0, 5, 10);
    constexpr auto pressureSuccess = matchIndexedRow(
        false, oldRod, 0, 10,
        jPressureSeed, jPressurePid, 10);
    static_assert(pressureSuccess.has(Path::PressureSuccess));

    // The accepted seed-13 Method J failure frame proves several distinct lead
    // histories for the same saved encounter. The combined layer must retain all
    // of them instead of selecting a single guessed lead.
    constexpr uint32_t jFailureSeed = 13u;
    constexpr uint32_t jFailurePid = sequentialPid(jFailureSeed);
    constexpr uint64_t jSlot2 = makeRow(0, 2, 5, 5);
    constexpr auto failures = matchIndexedRow(
        false, jSlot2, 0, 5,
        jFailureSeed, jFailurePid, 5);
    static_assert(failures.has(Path::CuteCharmFailure));
    static_assert(failures.has(Path::PressureFailure));
    static_assert(failures.has(Path::IntimidateContinue));

    constexpr uint32_t jSyncSeed = 81u;
    constexpr uint32_t jSyncPid = sequentialPid(jSyncSeed);
    constexpr uint64_t jSlot0 = makeRow(0, 0, 5, 5);
    constexpr auto syncFailure = matchIndexedRow(
        false, jSlot0, 0, 5,
        jSyncSeed, jSyncPid, 5);
    static_assert(syncFailure.has(Path::SynchronizeFailure));

    constexpr uint64_t jStaticFailSlot2 = makeRow(0, 2, 5, 5);
    constexpr auto staticFailure = matchIndexedRow(
        false, jStaticFailSlot2, 0, 5,
        jStaticSeed, jStaticPid, 5);
    static_assert(staticFailure.has(Path::StaticMagnetFailure));

    // Prove the combined row matcher can accept a source-proven Grass Pressure
    // level above this individual row's ordinary maximum. Build the row's slot
    // directly from the accepted Method J pressure frame instead of guessing it.
    constexpr uint32_t boostedSeed = 81u;
    constexpr uint32_t boostedPid = sequentialPid(boostedSeed);
    constexpr uint32_t boostedPrev1 = prev(boostedSeed);
    constexpr uint32_t boostedPrev2 = prev(boostedPrev1);
    constexpr uint64_t grassShape = makeRow(0, 0, 5, 8);
    constexpr uint8_t boostedSlot = Legality::Gen4LeadFailure::rolledSlot(
        false, grassShape, static_cast<uint16_t>(boostedPrev2 >> 16));
    constexpr uint64_t boostedGrass = makeRow(0, boostedSlot, 5, 8);
    static_assert(!Legality::Gen4Wild::levelMatches(boostedGrass, 10));
    constexpr auto boostedPressure = matchIndexedRow(
        false, boostedGrass, 0, 10,
        boostedSeed, boostedPid, 10);
    static_assert(boostedPressure.has(Path::PressureSuccess));

    // Bug Contest and Safari stay intentionally outside the accepted extended
    // lead-history layer until their reroll/deadlock behavior is reconstructed.
    constexpr uint64_t contest = makeRow(8, 0, 7, 18, 25);
    constexpr uint64_t safari = makeRow(10, 0, 15, 15, 6);
    static_assert(!matchIndexedRow(
        true, contest, 0, 18,
        jStaticSeed, jStaticPid, 18).matched());
    static_assert(!matchIndexedRow(
        true, safari, 0, 15,
        jStaticSeed, jStaticPid, 15).matched());

    // Exercise exact generated source aliases end-to-end. Find a real Grass row
    // whose parent-area PressureLevel is above the row's ordinary LevelMax, derive
    // one bounded deterministic successful pressure history, then require the
    // source-aware combined analyzer to retain that positive proof.
    std::size_t boostedIndex = kSourceCount;
    for (std::size_t i = 0; i < kSourceCount; ++i) {
        const uint64_t row = Legality::Gen4Wild::kPackedGen4WildEncounters[i];
        const uint8_t pressure =
            Legality::Gen4Wild::kPackedGen4WildPressureLevel[i];
        if (Legality::Gen4Wild::method(row) == 0 &&
            pressure > Legality::Gen4Wild::maxLevel(row)) {
            boostedIndex = i;
            break;
        }
    }
    assert(boostedIndex < kSourceCount);

    const uint64_t sourceRow =
        Legality::Gen4Wild::kPackedGen4WildEncounters[boostedIndex];
    const uint8_t sourcePressure =
        Legality::Gen4Wild::kPackedGen4WildPressureLevel[boostedIndex];
    const uint32_t sourceMeta =
        Legality::Gen4Wild::kPackedGen4WildLeadMeta[boostedIndex];
    const auto sourceGame = Legality::Gen4Wild::game(sourceRow);
    const bool sourceHgss =
        sourceGame == Legality::Gen4Wild::Game::HeartGold ||
        sourceGame == Legality::Gen4Wild::Game::SoulSilver;

    bool foundSeed = false;
    uint32_t sourceSeed = 0;
    uint32_t sourcePid = 0;
    for (uint32_t seed = 0; seed < 0x40000u; ++seed) {
        const uint32_t pid = sequentialPid(seed);
        if (!matchIndexedRow(
                sourceHgss, sourceRow, sourceMeta, sourcePressure,
                seed, pid, sourcePressure).has(Path::PressureSuccess))
            continue;
        sourceSeed = seed;
        sourcePid = pid;
        foundSeed = true;
        break;
    }
    assert(foundSeed);

    const uint8_t encounterForm = Legality::Gen4Wild::form(sourceRow);
    const uint8_t pokemonForm = encounterForm >= 30 ? 0 : encounterForm;
    const auto generated = analyzeSupported(
        gameId(sourceGame),
        Legality::Gen4Wild::species(sourceRow),
        Legality::Gen4Wild::location(sourceRow),
        sourcePressure,
        pokemonForm,
        0,
        sourceSeed,
        sourcePid);
    assert(generated.has(Path::PressureSuccess));
    assert(!analyzeSupported(
        "not_a_gen4_game", 1, 1, 1, 0, 0, 0, 0).matched());

    static_assert(pathName(Path::StaticSuccess)[0] == 'S');
    static_assert(pathName(Path::IntimidateContinue)[0] == 'I');

    std::cout << "Gen IV combined lead-history evidence: PASS\n";
}
