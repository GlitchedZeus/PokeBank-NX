#pragma once
// RNG-002: bounded Method J/K nature-rejection reversal with an explicit
// EXHAUSTED state. An RNG-stage match is not a complete wild encounter
// witness. Exhaustion is NOT a proven RNG mismatch or global Invalid.
#include "Legality/Gen3PidIvCorrelation.h"
#include <cstdint>

namespace Legality::Gen4NatureReversal {
enum class Status : uint8_t { Matched, SearchExhausted, InvalidNature };
struct Result {
    Status status=Status::SearchExhausted;
    uint16_t rejectedPidPairs=0;
    uint16_t inspectedPidPairs=0;
    constexpr bool matched() const noexcept { return status==Status::Matched; }
    constexpr bool exhausted() const noexcept {
        return status==Status::SearchExhausted;
    }
};
// The retail nature-rejection loop does not impose a 128-PID bound. The
// budget below limits *our evidence search only*; reaching it leaves the
// historical sequence unresolved rather than proving incompatibility.
constexpr Result search(uint32_t prePidSeed,uint8_t nature,
                        uint16_t budget=128) noexcept {
    if(nature>=25) return {Status::InvalidNature,0,0};
    uint32_t upper=prePidSeed>>16;
    for(uint16_t count=0;count<budget;++count) {
        prePidSeed=Gen3PidIv::Detail::prev(prePidSeed);
        const uint32_t lower=prePidSeed>>16;
        const uint32_t pid=(upper<<16)|lower;
        if(pid%25u==nature)
            return {Status::Matched,count,
                    static_cast<uint16_t>(count+1u)};
        prePidSeed=Gen3PidIv::Detail::prev(prePidSeed);
        upper=prePidSeed>>16;
    }
    return {Status::SearchExhausted,0,budget};
}
} // namespace Legality::Gen4NatureReversal
