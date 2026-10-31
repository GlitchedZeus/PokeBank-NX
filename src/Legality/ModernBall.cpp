// ModernBall.cpp - BallVerifier's encounter ball sets, as a per-species union.
// The tables are include/Legality/ModernBallData.inc, written by
// tools/pkhex_gen67_diff/oracle ("dumpballs") and checked against the pinned PKHeX in CI.
#include "Legality/ModernBall.h"

#include <array>
#include <span>

namespace Legality::ModernBall {
namespace {

#include "Legality/ModernBallData.inc"

std::span<const uint64_t> table(Modern::Format format) noexcept {
    using F = Modern::Format;
    switch (format) {
    case F::PK6: return kBalls6;
    case F::PK7: return kBalls7;
    case F::PB7: return kBalls7B;
    case F::PK8: return kBalls8;
    case F::PB8: return kBalls8B;
    case F::PA8: return kBalls8A;
    case F::PK9: return kBalls9;
    case F::PA9: return kBalls9A;
    case F::Unknown: break;
    }
    return {};
}

// The species' ball union in one format; 0 when it has no generated encounter there.
uint64_t mask(Modern::Format format, uint16_t species) noexcept {
    const auto masks = table(format);
    return species < masks.size() ? masks[species] : 0;
}

} // namespace

bool possible(Modern::Format format, uint16_t species, uint8_t ball) noexcept {
    using F = Modern::Format;
    if (ball >= 64) return true;
    uint64_t allowed = mask(format, species);
    if (allowed == 0) return true;
    // HOME moves records between every Switch format, including back to an older game (a
    // Scarlet/Violet gift in Sword/Shield), which the generator for the older format does not
    // yield: a Switch format takes the union over all of them and Let's Go.
    if (format == F::PK8 || format == F::PB8 || format == F::PA8 || format == F::PK9 || format == F::PA9) {
        for (F other : {F::PB7, F::PK8, F::PB8, F::PA8, F::PK9, F::PA9})
            allowed |= mask(other, species);
    }
    return ((allowed >> ball) & 1) != 0;
}

} // namespace Legality::ModernBall
