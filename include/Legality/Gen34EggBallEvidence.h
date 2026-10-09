#pragma once
#include "Legality/Gen34EggState.h"
#include <cstdint>

namespace Legality::Gen34EggBallEvidence {

// Pinned PKHeX BallVerifier.VerifyBallEgg: generations before VI do not
// inherit their parent's ball. Native PK3/PK4 egg origins have a compatible
// Poké Ball (ID4). This proves one compatible historical ball, not that
// every other persisted record is impossible; do not hard-Invalid on a
// mismatch while gift/transfer/event alternatives remain unresolved.
enum class Kind : uint8_t { Unresolved, NativeEggPokeBall };

constexpr Kind analyze(uint8_t nativeGeneration,
                       bool isEgg,
                       uint16_t eggLocation,
                       uint8_t metLevel,
                       uint8_t persistedBall) noexcept {
    if (persistedBall != 4)
        return Kind::Unresolved;
    const auto egg=Gen34EggState::analyze(
        nativeGeneration,isEgg,eggLocation,metLevel);
    if (!egg.eggOrigin || egg.invalid())
        return Kind::Unresolved;
    return Kind::NativeEggPokeBall;
}
} // namespace Legality::Gen34EggBallEvidence
