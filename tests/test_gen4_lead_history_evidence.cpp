#include "Legality/Gen4LeadHistoryEvidence.h"
#include "Legality/Gen4WildRngCorrelation.h"

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
    using Legality::Gen4LeadFrame::sequentialPid;

    static_assert(kSourceCount == kLeadMetaCount);
    static_assert(kSourceCount == kPressureCount);

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

    constexpr uint32_t jPressureSeed = 492u;
    constexpr uint32_t jPressurePid = sequentialPid(jPressureSeed);
    constexpr uint64_t oldRod = makeRow(2, 0, 5, 10);
    constexpr auto pressureSuccess = matchIndexedRow(
        false, oldRod, 0, 10,
        jPressureSeed, jPressurePid, 10);
    static_assert(pressureSuccess.has(Path::PressureSuccess));

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

    // HG/SS Bug Catching Contest direct-attempt lead history. Pinned Method K
    // permits a direct result only when the persisted attempt has at least one
    // 31 IV. Leads without a Sweet Scent-capable species must also prove the
    // two-call movement/rate activation path.
    static_assert(Legality::Gen4LeadFailure::supportedType(true, 8));
    static_assert(!Legality::Gen4LeadFailure::supportedType(false, 8));
    static_assert(Legality::Gen4LeadFrame::bugContestSlot(0) == 9);
    static_assert(Legality::Gen4LeadFrame::bugContestSlot(99) == 0);
    static_assert(Legality::Gen4LeadFrame::bugContestActivationAllows(
        25, 0u, false));
    static_assert(!Legality::Gen4LeadFrame::bugContestActivationAllows(
        25, 25u << 16, false));
    static_assert(Legality::Gen4LeadFrame::bugContestActivationAllows(
        25, 25u << 16, true));

    constexpr uint32_t bccSeed8 = 8u;
    constexpr uint32_t bccPid8 = sequentialPid(bccSeed8);
    static_assert(Legality::Gen4LeadFrame::directMinimum31Satisfied(bccSeed8));

    constexpr auto bccStatic = matchIndexedRow(
        true, makeRow(8, 0, 7, 18, 25), makeMeta(0, 0, 0, 1), 18,
        bccSeed8, bccPid8, 14);
    static_assert(bccStatic.has(Path::StaticSuccess));

    constexpr auto bccMagnet = matchIndexedRow(
        true, makeRow(8, 0, 7, 18, 25), makeMeta(0, 1, 0, 0), 18,
        bccSeed8, bccPid8, 14);
    static_assert(bccMagnet.has(Path::MagnetPullSuccess));

    constexpr auto bccPressure = matchIndexedRow(
        true, makeRow(8, 1, 7, 18, 25), 0, 18,
        bccSeed8, bccPid8, 18);
    static_assert(bccPressure.has(Path::PressureSuccess));

    constexpr auto bccSyncFail = matchIndexedRow(
        true, makeRow(8, 1, 7, 18, 25), 0, 18,
        bccSeed8, bccPid8, 11);
    static_assert(bccSyncFail.has(Path::SynchronizeFailure));

    constexpr uint32_t bccSeed9 = 9u;
    constexpr uint32_t bccPid9 = sequentialPid(bccSeed9);
    static_assert(Legality::Gen4LeadFrame::directMinimum31Satisfied(bccSeed9));
    constexpr auto bccPressureFail = Legality::Gen4LeadFailure::matchRow(
        true, makeRow(8, 9, 7, 18, 25), bccSeed9, bccPid9, 10,
        Legality::Gen4LeadFailure::Lead::PressureHustleVitalSpirit);
    static_assert(bccPressureFail.matched());
    constexpr uint32_t bccPressureActivation = prev(prev(prev(prev(
        bccPressureFail.encounterSeed))));
    static_assert(((bccPressureActivation >> 16) % 100u) >= 25u);
    static_assert(Legality::Gen4LeadFrame::bugContestActivationAllows(
        25, bccPressureActivation, true));
    static_assert(!Legality::Gen4LeadFrame::bugContestActivationAllows(
        25, bccPressureActivation, false));
    constexpr auto bccPressureFailMask = matchIndexedRow(
        true, makeRow(8, 9, 7, 18, 25), 0, 18,
        bccSeed9, bccPid9, 10);
    static_assert(bccPressureFailMask.has(Path::PressureFailure));
    static_assert(bccPressureFailMask.has(Path::IntimidateContinue));

    constexpr uint32_t bccSeed17 = 17u;
    constexpr uint32_t bccPid17 = sequentialPid(bccSeed17);
    static_assert(Legality::Gen4LeadFrame::directMinimum31Satisfied(bccSeed17));
    constexpr auto bccCuteFail = matchIndexedRow(
        true, makeRow(8, 5, 7, 18, 25), 0, 18,
        bccSeed17, bccPid17, 8);
    static_assert(bccCuteFail.has(Path::CuteCharmFailure));
    constexpr auto bccStaticFail = matchIndexedRow(
        true, makeRow(8, 0, 7, 18, 25), 0, 18,
        bccSeed17, bccPid17, 13);
    static_assert(bccStaticFail.has(Path::StaticMagnetFailure));

    constexpr uint32_t bccNo31Seed = 15u;
    constexpr uint32_t bccNo31Pid = sequentialPid(bccNo31Seed);
    static_assert(!Legality::Gen4LeadFrame::directMinimum31Satisfied(bccNo31Seed));
    static_assert(!Legality::Gen4PressureLead::matchRowWithPressure(
        true, makeRow(8, 6, 7, 18, 25), 18,
        bccNo31Seed, bccNo31Pid, 18).matched());

    // Method K BCC mixed Synchronize one-reroll path: original Sync proc
    // succeeds, its attempt lacks any 31 IV; persisted attempt fails Sync.
    // Pinned source allows the original locked nature 11 and final nature 0
    // to differ because the final Synchronize proc fails.
    constexpr uint32_t mixedSeed = 0x000019CBu;
    constexpr uint32_t mixedPid = 0x218385E9u;
    // Real HeartGold Kakuna BCC row: location207, slot3, level9-18,
    // retail encounter rate 25 (not a synthetic 50% rate).
    constexpr uint64_t mixedRow = 0x64D80412139E0EULL;
    static_assert(Legality::Gen4Wild::game(mixedRow) ==
                  Legality::Gen4Wild::Game::HeartGold);
    static_assert(Legality::Gen4Wild::species(mixedRow) == 14);
    static_assert(Legality::Gen4Wild::rate(mixedRow) == 25);
    constexpr auto mixed = matchIndexedRow(
        true, mixedRow, 0, 18, mixedSeed, mixedPid, 15);
    static_assert(mixed.has(Path::SynchronizeMixedSuccessThenFailure));
    static_assert(!matchIndexedRow(
        true, mixedRow, 0, 18,
        mixedSeed, mixedPid, 14).has(Path::SynchronizeMixedSuccessThenFailure));
    static_assert(!matchIndexedRow(
        false, mixedRow, 0, 18, mixedSeed, mixedPid, 15)
        .has(Path::SynchronizeMixedSuccessThenFailure));
    static_assert(!matchIndexedRow(
        true, makeRow(8, 3, 9, 18, 0), 0, 18,
        mixedSeed, mixedPid, 15).has(Path::SynchronizeMixedSuccessThenFailure));

    // The opposite direction is separately source-backed: rejected first
    // attempt failed Synchronize, retained attempt succeeded Synchronize.
    // Real HG encounter_hg.pkl Kakuna row, location 207, BCC slot3/rate25.
    constexpr uint32_t reverseSeed = 0x00000AD2u;
    constexpr uint32_t reversePid = 0xECE9B3BCu;
    constexpr auto reverse = matchIndexedRow(
        true, mixedRow, 0, 18, reverseSeed, reversePid, 11);
    static_assert(reverse.has(Path::SynchronizeMixedFailureThenSuccess));
    static_assert(!matchIndexedRow(
        true, mixedRow, 0, 18, reverseSeed, reversePid, 12)
        .has(Path::SynchronizeMixedFailureThenSuccess));
    static_assert(!matchIndexedRow(
        false, mixedRow, 0, 18, reverseSeed, reversePid, 11)
        .has(Path::SynchronizeMixedFailureThenSuccess));
    static_assert(!matchIndexedRow(
        true, makeRow(8, 3, 9, 18, 0), 0, 18,
        reverseSeed, reversePid, 11).has(Path::SynchronizeMixedFailureThenSuccess));

    constexpr uint64_t safari = makeRow(10, 0, 15, 15, 6);
    static_assert(!matchIndexedRow(
        true, safari, 0, 15,
        jStaticSeed, jStaticPid, 15).matched());

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

    const auto central = Legality::Gen4WildRng::analyzeSupported(
        gameId(sourceGame),
        Legality::Gen4Wild::species(sourceRow),
        Legality::Gen4Wild::location(sourceRow),
        sourcePressure,
        pokemonForm,
        0,
        sourceSeed,
        sourcePid);
    assert(central.matched());
    assert(central.method ==
        (sourceHgss
            ? Legality::Gen4WildRng::Method::MethodKExtendedLead
            : Legality::Gen4WildRng::Method::MethodJExtendedLead));
    assert((central.leadHistoryMask &
            static_cast<uint16_t>(Path::PressureSuccess)) != 0);
    assert(central.slot == 0xFF);

    assert(!analyzeSupported(
        "not_a_gen4_game", 1, 1, 1, 0, 0, 0, 0).matched());
    static_assert(pathName(Path::StaticSuccess)[0] == 'S');
    static_assert(pathName(Path::IntimidateContinue)[0] == 'I');

    std::cout << "Gen IV combined lead-history evidence: PASS\n";
}
