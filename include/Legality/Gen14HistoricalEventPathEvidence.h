#pragma once
// L4 Stage 4: original event -> optional historical operations -> stored PKM.
// A field mismatch excludes at most ONE independently identified candidate
// history. No result authenticates a distribution or establishes global Invalid.
// Source/route completeness and the values after every mutation are external
// historical evidence, never inferred from an extracted PKM or game clock.
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace Legality::Gen14HistoricalEventPathEvidence {
enum class Generation : uint8_t { Unknown, I, II, III, IV };
enum class Action : uint8_t {
    Trade, Evolution, Hatch, PalPark, TimeCapsule,
    MoveChange, Renaming, Contest, ItemChange, ClockChange
};
enum Field : uint32_t {
    Species=1u<<0, TrainerIDs=1u<<1, Language=1u<<2,
    OriginalBall=1u<<3, OriginVersion=1u<<4, EggState=1u<<5,
    HeldItem=1u<<6, MoveSet=1u<<7, Ribbons=1u<<8,
    NicknameRaw=1u<<9, OriginalTrainerRaw=1u<<10,
    MetDate=1u<<11, Fateful=1u<<12, Form=1u<<13,
    Ability=1u<<14
};
inline constexpr size_t kFields=15;
inline constexpr uint32_t kAllFields=(1u<<kFields)-1u;
inline constexpr size_t kMaxSteps=16;
enum class Status : uint8_t {
    UnknownNative, UnknownOriginalSource, UnknownRoute,
    UnknownHistoricalTransformation, CandidateRouteContradicted,
    CandidateFieldContradicted, ConditionalCandidate
};
// Each value is a CANONICAL source-supplied value, NOT a native PK3/PK4
// layout assumption. In particular, raw nickname/OT values are only
// comparable within their proven encoding history, and MetDate is a display
// date rather than an authenticated DS receipt time.
struct Snapshot {
    std::array<uint64_t,kFields> value{};
    uint32_t known=0;
};
struct Step {
    Action action=Action::Trade;
    Generation from=Generation::Unknown, to=Generation::Unknown;
    bool independentlyVerified=false;
    uint32_t mutated=0;
    Snapshot verifiedAfter{};
};
struct Source {
    bool exactOriginalSourceIndependentlyAttested=false;
    bool originalFieldsIndependentlyExtracted=false;
    Generation originalGeneration=Generation::Unknown;
    Snapshot original{};
    // An independently exhaustive chain is necessary to claim that an
    // unlisted rename/evolution/transfer could not have happened.
    bool exhaustiveHistoricalChainVerified=false;
};
struct Result {
    Status status=Status::UnknownNative;
    Generation destination=Generation::Unknown;
    uint32_t contradicted=0, unknown=0;
    size_t inspectedSteps=0;
    bool authenticated=false; // never infer an original distribution
};
constexpr uint32_t maskFor(Action a) noexcept {
    switch(a) {
    case Action::Trade: return NicknameRaw|OriginalTrainerRaw|Language;
    case Action::Evolution: return Species|MoveSet|Form|Ability|NicknameRaw;
    case Action::Hatch: return EggState|NicknameRaw|OriginalTrainerRaw|MetDate|Language;
    case Action::PalPark: return MoveSet|NicknameRaw|OriginalTrainerRaw|MetDate|Language;
    case Action::TimeCapsule: return MoveSet|NicknameRaw|OriginalTrainerRaw|Language;
    case Action::MoveChange: return MoveSet;
    case Action::Renaming: return NicknameRaw;
    case Action::Contest: return Ribbons;
    case Action::ItemChange: return HeldItem;
    case Action::ClockChange: return MetDate;
    }
    return 0;
}
constexpr uint32_t intrinsicallyReencoded(Action a) noexcept {
    // These historical transitions can rewrite raw strings/date even when
    // no logical species, trainer or date change is visible on the final PKM.
    switch(a) {
    case Action::PalPark: case Action::TimeCapsule:
        return NicknameRaw|OriginalTrainerRaw|MetDate;
    case Action::Hatch: return NicknameRaw|OriginalTrainerRaw|MetDate;
    case Action::Evolution: return NicknameRaw;
    default: return 0;
    }
}
constexpr bool routePermitted(Generation from,Generation to,Action a) noexcept {
    if(from==Generation::Unknown || to==Generation::Unknown)return false;
    if(a==Action::PalPark) return from==Generation::III && to==Generation::IV;
    if(a==Action::TimeCapsule)
        return (from==Generation::I && to==Generation::II) ||
               (from==Generation::II && to==Generation::I);
    if(from!=to)return false;
    if(a==Action::Contest && to!=Generation::III && to!=Generation::IV)return false;
    return true;
}
constexpr Result evaluate(bool nativeAvailable,Generation nativeGeneration,
    const Snapshot& final, const Source& source,
    std::span<const Step> steps={}) noexcept {
    Result out{};
    if(!nativeAvailable)return out;
    if(!source.exactOriginalSourceIndependentlyAttested ||
       !source.originalFieldsIndependentlyExtracted ||
       source.originalGeneration==Generation::Unknown) {
        out.status=Status::UnknownOriginalSource;return out;
    }
    out.destination=source.originalGeneration;
    if(!source.exhaustiveHistoricalChainVerified || steps.size()>kMaxSteps) {
        out.status=Status::UnknownHistoricalTransformation;
        out.unknown=final.known&kAllFields;return out;
    }
    auto state=source.original;
    state.known &= kAllFields;
    for(const Step& step:steps) {
        if(!step.independentlyVerified || step.from!=out.destination ||
           step.from==Generation::Unknown || step.to==Generation::Unknown) {
            out.status=Status::UnknownHistoricalTransformation;
            out.unknown=final.known&kAllFields;return out;
        }
        if(!routePermitted(step.from,step.to,step.action) ||
           (step.mutated & ~(maskFor(step.action)))!=0u) {
            out.status=Status::CandidateRouteContradicted;
            out.contradicted=step.mutated & ~maskFor(step.action);
            return out;
        }
        // The native original Ball, OT IDs, fateful flag and origin code are
        // intentionally NOT declared mutable by historical game transitions.
        const auto lost=intrinsicallyReencoded(step.action) & ~step.verifiedAfter.known;
        state.known &= ~(step.mutated|lost);
        for(size_t f=0;f<kFields;++f) {
            const uint32_t bit=1u<<f;
            if((step.mutated&bit)==0 && (lost&bit)==0)continue;
            if((step.verifiedAfter.known&bit)!=0) {
                state.value[f]=step.verifiedAfter.value[f];
                state.known|=bit;
            }
        }
        out.destination=step.to;
        ++out.inspectedSteps;
    }
    if(nativeGeneration!=out.destination) {
        out.status=Status::UnknownRoute;return out;
    }
    out.unknown=(final.known&kAllFields)&~state.known;
    if((final.known&kAllFields)==0) {
        out.status=Status::UnknownHistoricalTransformation;return out;
    }
    for(size_t f=0;f<kFields;++f) {
        const uint32_t bit=1u<<f;
        if((final.known&state.known&bit)!=0 && state.value[f]!=final.value[f])
            out.contradicted|=bit;
    }
    out.status=out.contradicted?Status::CandidateFieldContradicted:
        out.unknown?Status::UnknownHistoricalTransformation:
        Status::ConditionalCandidate;
    return out;
}
static_assert(routePermitted(Generation::III,Generation::IV,Action::PalPark));
static_assert(!routePermitted(Generation::II,Generation::IV,Action::PalPark));
static_assert(!(maskFor(Action::Trade)&TrainerIDs));
static_assert(!(maskFor(Action::PalPark)&OriginalBall));
static_assert(kAllFields==0x7fffu);
} // namespace Legality::Gen14HistoricalEventPathEvidence
