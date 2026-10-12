#pragma once
// TRANSFER-002 / MOVE-001: independently attested alternative full
// Emerald -> Pal Park -> Gen IV post-learning -> native PK4 histories.
// Exhausting THIS bounded Emerald-path universe cannot prove global
// Pokémon Invalid: other source games, event/gift histories and missing
// archives remain outside the coverage.
#include "Legality/Gen34EmeraldPalParkCompleteHistory.h"
#include <algorithm>
#include <array>
#include <cstdint>

namespace Legality::Gen34EmeraldPalParkCandidateUniverse {
namespace Full=Gen34EmeraldPalParkCompleteHistory;
enum class Status : uint8_t {
    NoCandidates, CapacityExceededUnknown, UnknownObservedNativeSource,
    UnknownRouteIdentity, OneConditionalFullHistory,
    UnknownHistoryProvenance, UnknownMethodCoverage,
    UnknownRouteEnumeration, AllAttestedEmeraldRoutesContradicted
};
struct Entry {
    uint64_t routeIdentity=0;
    bool independentlyGrounded=false;
    // Only an independently exhaustive negative acquisition-method
    // witness permits a contradiction to enter the exhaustion count.
    bool allMethodsIndependentlyCovered=false;
    Full::Input history{};
};
struct Input {
    // Single immutable final native PK4 which all candidates MUST
    // explain; not a list of unrelated species or recycled babies.
    const Pokemon::Pokemon4ReadOnly* observedNative=nullptr;
    bool observedNativeSourceIndependentlyVerified=false;
    bool allEmeraldPalParkRoutesIndependentlyEnumerated=false;
    std::array<Entry,8> histories{};
    uint8_t count=0;
};
struct Result {
    Status status=Status::NoCandidates;
    uint8_t considered=0,compatible=0,contradicted=0,unresolved=0,unverified=0;
    std::array<Full::Status,8> individual{};
};
constexpr bool candidateContradiction(Full::Status status) noexcept {
    switch(status) {
        case Full::Status::IncoherentArrivalStage:
        case Full::Status::IncoherentMigrationSnapshot:
        case Full::Status::IncoherentObservedStage:
        case Full::Status::IncoherentNativePokemon:
        case Full::Status::OriginalTrainingContradicted:
        case Full::Status::PostArrivalTrainingContradicted:
            return true;
        default:return false;
    }
}
static_assert(candidateContradiction(Full::Status::IncoherentNativePokemon));
static_assert(!candidateContradiction(Full::Status::UnknownSharedCreature));
inline Result evaluate(const Input& in) noexcept {
    Result out{};
    if(!in.count)return out;
    if(in.count>in.histories.size()) {
        out.status=Status::CapacityExceededUnknown;return out;
    }
    if(!in.observedNative || !in.observedNativeSourceIndependentlyVerified ||
       !in.observedNative->valid() || in.observedNative->empty() ||
       in.observedNative->isEgg()) {
        out.status=Status::UnknownObservedNativeSource;return out;
    }
    for(uint8_t i=0;i<in.count;++i) {
        if(!in.histories[i].routeIdentity) {
            out.status=Status::UnknownRouteIdentity;return out;
        }
        for(uint8_t j=0;j<i;++j)
            if(in.histories[i].routeIdentity==in.histories[j].routeIdentity) {
                out.status=Status::UnknownRouteIdentity;return out;
            }
    }
    const auto root=in.observedNative->originalEncryptedBytes();
    for(uint8_t i=0;i<in.count;++i) {
        ++out.considered;
        const auto& route=in.histories[i];
        const auto* other=route.history.nativeObserved;
        if(!route.independentlyGrounded || !other || !other->valid() ||
           other->sourceGroup()!=in.observedNative->sourceGroup()) {
            ++out.unverified;continue;
        }
        const auto bytes=other->originalEncryptedBytes();
        if(bytes.size()!=root.size() ||
           !std::equal(root.begin(),root.end(),bytes.begin())) {
            ++out.unverified;continue;
        }
        const auto candidate=Full::evaluate(route.history);
        out.individual[i]=candidate.status;
        if(candidate.status==Full::Status::KnownConditionalFullPath) {
            ++out.compatible;continue;
        }
        if(candidateContradiction(candidate.status) &&
           route.allMethodsIndependentlyCovered) {
            ++out.contradicted;continue;
        }
        ++out.unresolved;
    }
    if(out.compatible)out.status=Status::OneConditionalFullHistory;
    else if(out.unverified)out.status=Status::UnknownHistoryProvenance;
    else if(out.unresolved)out.status=Status::UnknownMethodCoverage;
    else if(!in.allEmeraldPalParkRoutesIndependentlyEnumerated)
        out.status=Status::UnknownRouteEnumeration;
    else if(out.contradicted==out.considered)
        out.status=Status::AllAttestedEmeraldRoutesContradicted;
    else out.status=Status::UnknownMethodCoverage;
    return out;
}
} // namespace Legality::Gen34EmeraldPalParkCandidateUniverse
