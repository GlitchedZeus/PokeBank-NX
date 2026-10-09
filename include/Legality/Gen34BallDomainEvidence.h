#ifndef LEGALITY_GEN34_BALL_DOMAIN_EVIDENCE_H
#define LEGALITY_GEN34_BALL_DOMAIN_EVIDENCE_H

#include <cstdint>

namespace Legality::Gen34BallDomain {

    enum class Result : uint8_t {
        Unresolved,
        WithinGenerationDomain,
        Invalid,
    };

    // PKHeX reference: Legal.MaxBallID_3 / Legal.MaxBallID_4 at
    // 6501f0ab46e8f8ca048539dbaf8cae8cb104e722.
    //
    // This helper proves only that a stored nonzero Ball ID can exist in the
    // generation's format. It deliberately does not claim encounter legality:
    // Safari/Cherish/Sport, eggs, fixed gifts, Shedinja and other histories need
    // encounter-specific evidence before they may be called valid or invalid.
    constexpr Result classify(uint8_t generation, uint8_t ball) noexcept {
        if (generation != 3 && generation != 4)
            return Result::Unresolved;
        if (ball == 0)
            return Result::Unresolved;

        const uint8_t maxBall = generation == 3 ? 12 : 24;
        return ball <= maxBall ? Result::WithinGenerationDomain : Result::Invalid;
    }

    constexpr bool isInvalid(uint8_t generation, uint8_t ball) noexcept {
        return classify(generation, ball) == Result::Invalid;
    }

} // namespace Legality::Gen34BallDomain

#endif // LEGALITY_GEN34_BALL_DOMAIN_EVIDENCE_H
