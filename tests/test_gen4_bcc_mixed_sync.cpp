#include "Legality/Gen4BugContestMixedSyncEvidence.h"

#include <cassert>
#include <cstdint>
#include <iostream>

namespace {
constexpr uint64_t makeRow(uint8_t type, uint8_t slot,
                           uint8_t min, uint8_t max, uint8_t rate) {
    return (static_cast<uint64_t>(min) << 17) |
           (static_cast<uint64_t>(max) << 24) |
           (static_cast<uint64_t>(type) << 31) |
           (static_cast<uint64_t>(slot) << 46) |
           (static_cast<uint64_t>(rate) << 50);
}
}

int main() {
    using namespace Legality;
    namespace Mixed = Gen4BugContestMixedSync;

    // REAL pinned PKHeX HeartGold Bug-Catching Contest row:
    // Kakuna (#14), location 207, slot 3, level 9-18, encounter rate 25.
    // Source: encounter_hg.pkl packed in Gen4WildEncounterData.inc.
    // Do not replace the retail encounter rate with a synthetic 50% proc!
    constexpr uint64_t row = 0x64D80412139E0EULL;
    static_assert(Gen4Wild::game(row) == Gen4Wild::Game::HeartGold);
    static_assert(Gen4Wild::species(row) == 14u);
    static_assert(Gen4Wild::location(row) == 207u);
    static_assert(Gen4Wild::method(row) == 8u);
    static_assert(Gen4Wild::slot(row) == 3u);
    static_assert(Gen4Wild::minLevel(row) == 9u);
    static_assert(Gen4Wild::maxLevel(row) == 18u);
    static_assert(Gen4Wild::rate(row) == 25u);

    // First valid Method K mixed history, found independently against the
    // actual BCC rate/level/slot, NOT a synthetic encounter probability:
    // Origin Sync 0x2D1A9C05 passes (even), PID 0xF06D40BC nature 11.
    // IVs [12793, 28486] contain no 31, forcing the BCC reroll.
    // Persisted ordinary nature at 0x000019CB (nature 0) follows failed
    // Sync 0x7F9FE7B8 (odd). PID 0x218385E9 has nature 0 and a 31 IV.
    // Origin slot 3, level 15, activation 0xEBF86BB0 -> roll 8 < rate 25.
    constexpr uint32_t finalSeed = 0x000019CBu;
    constexpr uint32_t finalPid = 0x218385E9u;
    static_assert(Gen4LeadFrame::sequentialPid(finalSeed) == finalPid);
    static_assert(Gen4LeadFrame::directMinimum31Satisfied(finalSeed));
    constexpr uint32_t failSeed = Gen3PidIv::Detail::prev(finalSeed);
    static_assert(failSeed == 0x7F9FE7B8u);
    static_assert(Gen4BugContestSynchronizeFailure::synchronizeFails(failSeed));
    static_assert(Gen4LeadFrame::previousRerollAttemptRejected(failSeed));
    constexpr uint32_t firstSeed =
        Gen4LeadFrame::previousRerollNatureSeed(failSeed);
    static_assert(firstSeed == 0x2D1A9C05u);
    static_assert(Gen4LeadFrame::sequentialPid(firstSeed) == 0xF06D40BCu);
    static_assert(((firstSeed >> 16) & 1u) == 0u);

    constexpr auto matched =
        Mixed::matchSuccessThenFailure(row, finalSeed, finalPid, 15);
    static_assert(matched.matched());
    static_assert(matched.encounterSeed == 0xEBF86BB0u);
    static_assert(matched.originPid == 0xF06D40BCu);
    static_assert(matched.originNature == 11u);
    static_assert(matched.slot == 3u);
    static_assert(matched.level == 15u);
    static_assert(matched.rerollDepth == 1u);
    static_assert((finalPid % 25u) == 0u);
    static_assert((matched.encounterSeed >> 16) % 100u == 8u);

    // Old all-failed proof must reject this mixed path; the origin Sync
    // proc actually succeeded. A non-match is Unresolved, never Invalid.
    static_assert(!Gen4BugContestSynchronizeFailure::matchReroll(
        row, finalSeed, finalPid, 15, 1).matched());

    static_assert(!Mixed::matchSuccessThenFailure(
        row, finalSeed, finalPid, 14).matched()); // wrong met level
    static_assert(!Mixed::matchSuccessThenFailure(
        makeRow(8, 2, 9, 18, 25), finalSeed, finalPid, 15).matched());
    static_assert(!Mixed::matchSuccessThenFailure(
        makeRow(8, 3, 9, 18, 0), finalSeed, finalPid, 15).matched());
    static_assert(!Mixed::matchSuccessThenFailure(
        makeRow(10, 3, 9, 18, 25), finalSeed, finalPid, 15).matched());
    static_assert(!Mixed::matchSuccessThenFailure(
        row, finalSeed, finalPid ^ 1u, 15).matched());

    // SECOND real-row positive with different nature-lock and retained PID:
    // origin successful Sync at 0xC270CC55 (PID 0x3C77C707 nature 20),
    // retained failed Sync at 0x250FEAC8 (PID 0xDAC5D2D2 nature 0).
    // HG/SS BCC slot 3, level 16, rate 25, activation roll 1.
    constexpr uint32_t secondSeed = 0x0000479Bu;
    constexpr uint32_t secondPid = 0xDAC5D2D2u;
    constexpr auto second = Mixed::matchSuccessThenFailure(
        row, secondSeed, secondPid, 16);
    static_assert(second.matched());
    static_assert(second.encounterSeed == 0xB7FD5CC0u);
    static_assert(second.originPid == 0x3C77C707u);
    static_assert(second.originNature == 20u);
    static_assert(second.rerollDepth == 1u);
    static_assert(!Mixed::matchSuccessThenFailure(
        row, secondSeed, secondPid, 15).matched());

    assert(matched.matched() && second.matched());
    std::cout << "Gen IV BCC real-row mixed Synchronize reroll: PASS\n";
}
