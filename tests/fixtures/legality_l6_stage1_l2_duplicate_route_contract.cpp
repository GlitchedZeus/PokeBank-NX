// L6 Stage 1: regression contract for duplicate DP/Pearl daycare-route rows.
// This is audit-owned coverage. It uses the production L2 evaluator but does
// not modify the lane-owned suite that still contains the stale assertion.
#include "Legality/Gen4DiamondPearlEggAlternativeHistories.h"

#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <utility>

namespace A = Legality::Gen4DiamondPearlEggAlternativeHistories;
namespace IV = Legality::Gen4DiamondPearlEggIVStage;

namespace {
constexpr std::array<uint8_t, 6> kInitial{0, 3, 6, 9, 12, 15};
constexpr std::array<uint8_t, 6> kParent0{7, 12, 17, 22, 27, 0};
constexpr std::array<uint8_t, 6> kParent1{13, 24, 3, 14, 25, 4};
constexpr std::array<uint32_t, 3> kMt{0, 1, 32};

A::Input known(const char* game, uint32_t seed) {
    A::Input in{};
    in.game = game;
    in.originalEggEventIdentity = 0xD1A01234u;
    in.originalUnhatchedEggIdentityIndependentlyVerified = true;
    in.originalEggPIDIndependentlyVerified = true;
    in.originalEggIVsIndependentlyVerified = true;
    in.originalEggPID = 32;
    const auto replay = IV::replay(game, seed, kInitial, kParent0, kParent1);
    assert(replay.status == IV::Status::Replayed);
    in.originalEggIVs = replay.child;
    in.count = 1;
    in.allDaycareStageHypothesesIndependentlyEnumerated = true;

    auto& candidate = in.alternatives[0];
    candidate.routeIdentity = 1;
    candidate.originalEggEventIdentity = in.originalEggEventIdentity;
    candidate.independentlyAttestedHypothesis = true;
    candidate.allMethodsForThisCandidateIndependentlyCovered = true;
    candidate.sharedPhysicalParentPairIndependentlyVerified = true;
    candidate.ivPhysicalParentPairIdentity = 0xBEEFB001u;
    candidate.pidPhysicalParentPairIdentity = 0xBEEFB001u;
    candidate.iv.game = game;
    candidate.iv.initialChild = kInitial;
    candidate.iv.physicalSlot0 = kParent0;
    candidate.iv.physicalSlot1 = kParent1;
    candidate.iv.originalEggIVs = in.originalEggIVs;
    candidate.iv.finalSeed = replay.finalSeed;
    candidate.iv.historicalParentIVsVerified = true;
    candidate.iv.physicalDepositOrderVerified = true;
    candidate.iv.preInheritanceChildIVsVerified = true;
    candidate.iv.originalUnhatchedEggIVsVerified = true;
    candidate.iv.finalSeedIndependentlyVerified = true;
    candidate.pid.game = game;
    candidate.pid.originalDaycarePairEligibilityVerified = true;
    candidate.pid.eligibleOrdinaryDaycarePair = true;
    candidate.pid.originalParentSnapshotsVerified = true;
    candidate.pid.originalDaycareSlotOrderVerified = true;
    candidate.pid.creationTimeItemsIndependentlyVerified = true;
    candidate.pid.parents[0] = {25, 1, 7, 229, true};
    candidate.pid.parents[1] = {25, 0, 11, 0, true};
    candidate.pid.creationStageIndependentlyVerified = true;
    candidate.pid.collectionStageIndependentlyVerified = true;
    candidate.pid.parentStageToken = in.originalEggEventIdentity;
    candidate.pid.mtStageToken = in.originalEggEventIdentity;
    candidate.pid.collectionStageToken = in.originalEggEventIdentity;
    candidate.pid.creationLCRNGStateIndependentlyVerified = true;
    candidate.pid.consecutiveMTOutputsIndependentlyVerified = true;
    candidate.pid.exactMTConsumptionIndependentlyVerified = true;
    candidate.pid.mtOutputs = kMt;
    candidate.pid.originalParentLanguagesIndependentlyVerified = true;
    candidate.pid.originalParentLanguages = {2, 2};
    candidate.pid.originalCollectionTrainerIdIndependentlyVerified = true;
    candidate.pid.originalCollectionTrainerId = 0;
    candidate.pid.originalUnhatchedEggPidIndependentlyVerified = true;
    candidate.pid.originalUnhatchedEggPid = 32;
    return in;
}

void requireOne(const A::Input& input) {
    const auto result = A::evaluate(input);
    assert(result.status == A::Status::OneConditionallyConsistentHistory);
    assert(result.consistent == 1 && !result.contradicted);
}
} // namespace

int main() {
    unsigned cases = 0;
    for (const char* game : {"diamond_nds", "pearl_nds"}) {
        for (uint32_t seed = 0; seed != 64; ++seed) {
            const auto original = known(game, seed);
            requireOne(original);

            // Duplicate archive identity is not an exhaustive negative. A
            // verified matching row must survive whether it comes before or
            // after a malformed sibling.
            auto lateMatch = original;
            lateMatch.count = 2;
            lateMatch.alternatives[1] = lateMatch.alternatives[0];
            lateMatch.alternatives[0].independentlyAttestedHypothesis = false;
            requireOne(lateMatch);

            auto earlyMatch = lateMatch;
            std::swap(earlyMatch.alternatives[0], earlyMatch.alternatives[1]);
            requireOne(earlyMatch);

            // Two malformed colliding rows prove neither a valid route nor a
            // global contradiction; the evaluator must remain non-authenticating.
            auto malformedOnly = lateMatch;
            malformedOnly.alternatives[1] = malformedOnly.alternatives[0];
            const auto missing = A::evaluate(malformedOnly);
            assert(missing.status == A::Status::MissingOrDuplicateCandidateIdentity);
            assert(missing.consistent == 0 && missing.unverified >= 1);
            ++cases;
        }
    }
    std::cout << "L6 Stage 1 L2 duplicate-route native contracts: " << cases
              << " PASS (order-independent conditional evidence)\n";
}
