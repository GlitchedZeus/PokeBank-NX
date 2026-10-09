#include "Legality/Gen4LeadHistoryEvidence.h"
#include "Legality/Gen4LeadReportingEvidence.h"
#include "Legality/Gen4WildEncounter.h"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>

namespace {
using Legality::Gen4LeadHistory::Path;
using Legality::Gen4LeadHistory::Result;

constexpr bool hasPath(const Result& result, Path path) noexcept {
    return result.has(path);
}

constexpr uint16_t forwardBit =
    static_cast<uint16_t>(Path::SynchronizeMixedSuccessThenFailure);
constexpr uint16_t reverseBit =
    static_cast<uint16_t>(Path::SynchronizeMixedFailureThenSuccess);
}

int main() {
    using namespace Legality;

    // Pinned PKHeX Gen IV encounter_hg.pkl: real HeartGold Kakuna (#14)
    // Bug-Catching Contest area, met location 207, slot 3, level 9..18,
    // retail encounter rate 25, not the earlier discarded synthetic rate 50.
    constexpr uint64_t source = 0x64D80412139E0EULL;
    static_assert(Gen4Wild::game(source) == Gen4Wild::Game::HeartGold);
    static_assert(Gen4Wild::method(source) == 8);
    static_assert(Gen4Wild::species(source) == 14);
    static_assert(Gen4Wild::location(source) == 207);
    static_assert(Gen4Wild::slot(source) == 3);
    static_assert(Gen4Wild::minLevel(source) == 9);
    static_assert(Gen4Wild::maxLevel(source) == 18);
    static_assert(Gen4Wild::rate(source) == 25);

    // The actual source-indexed matcher must recover both one-reroll
    // mixed-Synchronize histories for their distinct supported seed frames.
    // Direct synthetic matchIndexedRow proof is not sufficient to show the
    // real species/game/met-location lookup ever finds the evidence.
    constexpr uint32_t forwardSeed = 0x000019CBu;
    constexpr uint32_t forwardPid = 0x218385E9u;
    constexpr uint32_t reverseSeed = 0x00000AD2u;
    constexpr uint32_t reversePid = 0xECE9B3BCu;

    const auto forward = Gen4LeadHistory::analyzeSupported(
        "heartgold_nds", 14, 207, 15, 0, 0, forwardSeed, forwardPid);
    const auto reverse = Gen4LeadHistory::analyzeSupported(
        "heartgold_nds", 14, 207, 11, 0, 0, reverseSeed, reversePid);
    assert(hasPath(forward, Path::SynchronizeMixedSuccessThenFailure));
    assert(hasPath(reverse, Path::SynchronizeMixedFailureThenSuccess));
    assert((forward.mask & forwardBit) != 0);
    assert((reverse.mask & reverseBit) != 0);

    // Source-aware reporting carries the proof without declaring every
    // other possible lead history reconstructed or claiming unique provenance.
    const auto forwardReport = Gen4LeadReporting::analyzeSupported(
        "heartgold_nds", 14, 207, 15, 0, 0, forwardSeed, forwardPid);
    const auto reverseReport = Gen4LeadReporting::analyzeSupported(
        "heartgold_nds", 14, 207, 11, 0, 0, reverseSeed, reversePid);
    assert(forwardReport.history.has(Path::SynchronizeMixedSuccessThenFailure));
    assert(reverseReport.history.has(Path::SynchronizeMixedFailureThenSuccess));
    const std::string forwardText = Gen4LeadReporting::describe(forwardReport);
    const std::string reverseText = Gen4LeadReporting::describe(reverseReport);
    assert(forwardText.find("Synchronize success then failure (BCC reroll)")
           != std::string::npos);
    assert(reverseText.find("Synchronize failure then success (BCC reroll)")
           != std::string::npos);
    assert(forwardText.find("other unproven lead histories remain incomplete")
           != std::string::npos);
    assert(reverseText.find("other unproven lead histories remain incomplete")
           != std::string::npos);

    // Negative controls: source identity has to be real. A Method J source
    // cannot produce a HeartGold BCC history; absent/malformed locations and
    // species likewise never establish these positive proof bits.
    const auto diamond = Gen4LeadHistory::analyzeSupported(
        "diamond_nds", 14, 207, 15, 0, 0, forwardSeed, forwardPid);
    const auto unknownGame = Gen4LeadHistory::analyzeSupported(
        "unknown_provider", 14, 207, 15, 0, 0, forwardSeed, forwardPid);
    const auto invalidLocation = Gen4LeadHistory::analyzeSupported(
        "heartgold_nds", 14, 999, 15, 0, 0, forwardSeed, forwardPid);
    const auto wrongSpecies = Gen4LeadHistory::analyzeSupported(
        "heartgold_nds", 1, 207, 15, 0, 0, forwardSeed, forwardPid);
    assert(!hasPath(diamond, Path::SynchronizeMixedSuccessThenFailure));
    assert(!unknownGame.matched());
    assert(!invalidLocation.matched());
    assert(!hasPath(wrongSpecies, Path::SynchronizeMixedSuccessThenFailure));

    // A failed proof is NOT a hard-invalid verdict: the stored record does
    // not retain the lead ability, the full history can be unresolvable.
    std::cout << "Gen IV real-source BCC mixed-Sync reporting: PASS\n";
}
