#pragma once
// L4 V15 / META-001. Gen IV contest-ribbon rank-chain evidence on native,
// checksum-valid PK4. Pinned reference: PKHeX@6501f0ab,
// RibbonVerifierUnique4.AddMissingContest4 and RibbonRules.IsAllowedContest4.
// No global Invalid: observed gaps are a *candidate* contradiction only.
// Later trades allow HGSS residents to have earned Sinnoh ribbons.
#include "Legality/Gen4ContestRibbonEvidence.h"
#include "Pokemon/Pokemon4ReadOnly.h"
#include <array>
#include <cstdint>

namespace Legality::Gen4ContestRibbonNativeEvidence {
enum class Status : uint8_t {
    Unavailable, HistoryUnknown, RankPrefixConsistent,
    CandidateRankGap, CandidateCannotEnter
};
struct ContestHistory {
    // An exact provenance chain may contain multiple earlier species/forms,
    // and can visit Sinnoh even when stored in HGSS. Unknown by default.
    bool originalHistoryComplete=false;
    bool accessToSinnohContestProven=false;
    bool anyEligibleStage=false;
};
struct Result {
    Status status=Status::Unavailable;
    Gen4ContestRibbon::Result prefix{};
    std::array<uint8_t,5> nativeRankMasks{};
};
constexpr bool eligibleAtStage(uint16_t species, uint8_t form) noexcept {
    // Same specific exclusions as pinned PKHeX RibbonRules.IsAllowedContest4:
    // Ditto #132, Unown #201 and form-1 Spiky-Eared Pichu #172.
    return species != 132 && species != 201 &&
           !(species == 172 && form == 1);
}
constexpr std::array<uint8_t,5> extract(uint32_t ribbonsWordAt60) noexcept {
    std::array<uint8_t,5> ranks{};
    for (uint8_t i=0;i<5;++i)
        ranks[i]=static_cast<uint8_t>((ribbonsWordAt60>>(4u*i))&0xFu);
    return ranks;
}
constexpr Result evaluate(bool nativeAvailable, uint32_t wordAt60,
                          const ContestHistory& history={}) noexcept {
    Result result{};
    if(!nativeAvailable)return result;
    result.nativeRankMasks=extract(wordAt60);
    result.prefix=Gen4ContestRibbon::evaluate(true,result.nativeRankMasks);
    // A native logical gap is a candidate-local historical contradiction,
    // not proof that a detached specimen has no valid alternate history.
    if(result.prefix.status==Gen4ContestRibbon::Status::RankGap) {
        result.status=Status::CandidateRankGap;return result;
    }
    if(!result.prefix.presentCategories) {
        result.status=Status::RankPrefixConsistent;return result;
    }
    if(!history.originalHistoryComplete) {
        result.status=Status::HistoryUnknown;return result;
    }
    if(!history.accessToSinnohContestProven || !history.anyEligibleStage) {
        result.status=Status::CandidateCannotEnter;return result;
    }
    result.status=Status::RankPrefixConsistent;
    return result;
}
inline Result evaluateNative(const Pokemon::Pokemon4ReadOnly& native,
                             const ContestHistory& history={}) noexcept {
    if(!native.valid() || native.empty())return {};
    return evaluate(true,native.ribbons()[2],history);
}
static_assert(extract(0xFEDCBu)[0]==0xB);
static_assert(extract(0xFEDCBu)[4]==0xF);
static_assert(eligibleAtStage(25,0));
static_assert(!eligibleAtStage(132,0));
static_assert(!eligibleAtStage(201,0));
static_assert(!eligibleAtStage(172,1));
static_assert(eligibleAtStage(172,0));
static_assert(evaluate(false,0xFFFFFu).status==Status::Unavailable);
static_assert(evaluate(true,0xFFFFFu).status==Status::HistoryUnknown);
static_assert(evaluate(true,0x00008u).status==Status::CandidateRankGap);
} // namespace Legality::Gen4ContestRibbonNativeEvidence
