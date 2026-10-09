#include "Legality/Gen4BugContestMixedSyncReverseEvidence.h"

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
    namespace Mixed = Gen4BugContestMixedSyncReverse;

    // Real pinned encounter_hg.pkl row: HeartGold Kakuna #14, location 207,
    // BCC slot 3, level 9..18, encounter rate 25 (not a synthetic rate 50).
    constexpr uint64_t row = 0x64D80412139E0EULL;
    static_assert(Gen4Wild::game(row) == Gen4Wild::Game::HeartGold);
    static_assert(Gen4Wild::species(row) == 14);
    static_assert(Gen4Wild::location(row) == 207);
    static_assert(Gen4Wild::method(row) == 8);
    static_assert(Gen4Wild::slot(row) == 3);
    static_assert(Gen4Wild::minLevel(row) == 9);
    static_assert(Gen4Wild::maxLevel(row) == 18);
    static_assert(Gen4Wild::rate(row) == 25);

    // First failed-Sync attempt: nature roll 0x2BCF0327 -> nature15,
    // preceding failed Sync 0xE3F56D04 (odd), rejected PID 0xF5FA1144
    // nature15, IV1=23594/IV2=8682 lack all 31s. Movement rate roll13,
    // source slot3/level11. Persisted attempt: Sync pass at 0x00000AD2
    // (even), PID0xECE9B3BC nature1; original rolled nature is different.
    constexpr uint32_t finalSeed = 0x00000AD2u;
    constexpr uint32_t persistedPid = 0xECE9B3BCu;
    static_assert(Gen4LeadFrame::sequentialPid(finalSeed) == persistedPid);
    static_assert((finalSeed >> 16) % 25u != (persistedPid % 25u));
    static_assert(Gen4LeadFrame::directMinimum31Satisfied(finalSeed));
    static_assert(Gen4LeadFrame::previousRerollAttemptRejected(finalSeed));
    constexpr uint32_t oldNature =
        Gen4LeadFrame::previousRerollNatureSeed(finalSeed);
    static_assert(oldNature == 0x2BCF0327u);
    static_assert(Gen4LeadFrame::sequentialPid(oldNature) == 0xF5FA1144u);
    static_assert((oldNature >> 16) % 25u == 15u);
    static_assert(Gen3PidIv::Detail::prev(oldNature) == 0xE3F56D04u);

    constexpr auto first = Mixed::matchFailureThenSuccess(
        row, finalSeed, persistedPid, 11);
    static_assert(first.matched());
    static_assert(first.encounterSeed == 0xB6154713u);
    static_assert(first.rejectedPid == 0xF5FA1144u);
    static_assert(first.rejectedNature == 15u);
    static_assert(first.slot == 3);
    static_assert(first.level == 11);
    static_assert(first.rerollDepth == 1);

    // Existing all-successful proof cannot explain the first failed proc.
    static_assert(!Gen4BugContestSynchronize::matchReroll(
        row, finalSeed, persistedPid, 11, 1).matched());

    // All unmatched outcomes are unresolved, never proof of an impossible PK4.
    static_assert(!Mixed::matchFailureThenSuccess(
        row, finalSeed, persistedPid, 12).matched()); // wrong level
    static_assert(!Mixed::matchFailureThenSuccess(
        makeRow(8, 2, 9, 18, 25), finalSeed, persistedPid, 11).matched());
    static_assert(!Mixed::matchFailureThenSuccess(
        makeRow(8, 3, 9, 18, 0), finalSeed, persistedPid, 11).matched());
    static_assert(!Mixed::matchFailureThenSuccess(
        makeRow(10, 3, 9, 18, 25), finalSeed, persistedPid, 11).matched());
    static_assert(!Mixed::matchFailureThenSuccess(
        row, finalSeed, persistedPid ^ 1u, 11).matched());

    // Second independently enumerated source-valid BCC history:
    // first failed Sync at 0x6695C43A; first PID 0xA6C135AA
    // nature17; persisted successful Sync at 0x00003858,
    // PID 0x632AFD53 nature22, level10, activation roll6 (<25).
    constexpr uint32_t secondSeed = 0x00003858u;
    constexpr uint32_t secondPid = 0x632AFD53u;
    constexpr auto second = Mixed::matchFailureThenSuccess(
        row, secondSeed, secondPid, 10);
    static_assert(second.matched());
    static_assert(second.encounterSeed == 0x3B66D931u);
    static_assert(second.rejectedPid == 0xA6C135AAu);
    static_assert(second.rejectedNature == 17u);
    static_assert(second.rerollDepth == 1);
    static_assert(!Mixed::matchFailureThenSuccess(
        row, secondSeed, secondPid, 11).matched());

    assert(first.matched() && second.matched());
    std::cout << "Gen IV BCC failed-then-successful Synchronize: PASS\n";
}
