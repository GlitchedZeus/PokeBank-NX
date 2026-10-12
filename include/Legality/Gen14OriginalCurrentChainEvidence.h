#pragma once
// L4 Stage 5: original -> independently pinned intermediate records ->
// final native Gen I-IV candidate. A checksum or SHA-256-shaped witness is
// never proof of a real gift, receipt, transfer, or distribution.
// No write operations and no bundled retail source data.
#include "Legality/Gen14HistoricalEventPathEvidence.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace Legality::Gen14OriginalCurrentChainEvidence {
namespace P=Gen14HistoricalEventPathEvidence;
using Digest=std::array<std::byte,32>;
constexpr bool digestNonzero(const Digest& digest) noexcept {
    for(const auto byte:digest)if(byte!=std::byte{0})return true;
    return false;
}
// A canonical historical value can be compared only where the original
// native generation ACTUALLY stores that field. Gen I/II have no Ball field;
// Gen I has no native Egg/Item; Gen III has no DS met calendar. Original
// source-only evidence for unsupported fields must not manufacture a match.
constexpr uint32_t nativeFields(P::Generation g) noexcept {
    constexpr uint32_t common=P::Species|P::TrainerIDs|P::Language|
        P::MoveSet|P::NicknameRaw|P::OriginalTrainerRaw;
    switch(g) {
    case P::Generation::I:return common;
    case P::Generation::II:return common|P::EggState|P::HeldItem;
    case P::Generation::III:return common|P::OriginalBall|P::OriginVersion|
        P::EggState|P::HeldItem|P::Ribbons|P::Fateful|P::Form|P::Ability;
    case P::Generation::IV:return P::kAllFields;
    case P::Generation::Unknown:return 0;
    }
    return 0;
}
enum class Status : uint8_t {
    UnknownNative, UnknownOriginalSource, UnknownRecordIdentity,
    UnknownProjection, UnknownRouteOrHistory, CandidateRouteExcluded,
    CandidateFieldExcluded, ConditionalCandidate
};
struct Hop {
    P::Step operation{};
    // Independently hashed/verified actual before and after record bytes;
    // the metadata snapshots remain independently extracted observations.
    Digest beforeRecordDigest{},afterRecordDigest{};
    bool beforeRecordDigestIndependentlyVerified=false;
    bool afterRecordDigestIndependentlyVerified=false;
    bool sameIndividualIndependentlyProven=false;
    bool beforeProjectionIndependentlyVerified=false;
    bool afterProjectionIndependentlyVerified=false;
    P::Snapshot before{},after{};
};
struct Input {
    bool finalNativeIndependentlyValidated=false;
    P::Source original{};
    bool originalRecordDigestIndependentlyVerified=false;
    Digest originalRecordDigest{};
    P::Generation finalGeneration=P::Generation::Unknown;
    P::Snapshot final{};
    bool finalRecordDigestIndependentlyVerified=false;
    Digest finalRecordDigest{};
    bool originalToFinalIndividualIndependentlyLinked=false;
    std::span<const Hop> history{};
};
struct Result {
    Status status=Status::UnknownNative;
    P::Generation destination=P::Generation::Unknown;
    size_t inspectedHops=0;
    uint32_t contradicted=0,unknown=0;
    bool authenticated=false,globallyInvalid=false,globallyLegal=false;
};
constexpr Result evaluate(const Input& input) noexcept {
    Result out{};
    if(!input.finalNativeIndependentlyValidated)return out;
    out.status=Status::UnknownOriginalSource;
    const auto& src=input.original;
    if(!src.exactOriginalSourceIndependentlyAttested ||
       !src.originalFieldsIndependentlyExtracted ||
       !src.exhaustiveHistoricalChainVerified ||
       !P::validGeneration(src.originalGeneration))return out;
    out.status=Status::UnknownRecordIdentity;
    if(!input.originalToFinalIndividualIndependentlyLinked ||
       !input.originalRecordDigestIndependentlyVerified ||
       !input.finalRecordDigestIndependentlyVerified ||
       !digestNonzero(input.originalRecordDigest) ||
       !digestNonzero(input.finalRecordDigest))return out;
    out.status=Status::UnknownProjection;
    if(!P::validGeneration(input.finalGeneration) ||
       input.history.size()>P::kMaxSteps ||
       (src.original.known & ~nativeFields(src.originalGeneration)) ||
       (input.final.known & ~nativeFields(input.finalGeneration)) ||
       src.original.known==0 || input.final.known==0)return out;
    P::Snapshot tracked=src.original;
    Digest expectedDigest=input.originalRecordDigest;
    P::Generation generation=src.originalGeneration;
    for(const Hop& hop:input.history) {
        if(!hop.sameIndividualIndependentlyProven ||
           !hop.beforeRecordDigestIndependentlyVerified ||
           !hop.afterRecordDigestIndependentlyVerified ||
           !digestNonzero(hop.beforeRecordDigest) ||
           !digestNonzero(hop.afterRecordDigest) ||
           hop.beforeRecordDigest!=expectedDigest) {
            out.status=Status::UnknownRecordIdentity;return out;
        }
        const auto& op=hop.operation;
        if(!op.independentlyVerified || op.from!=generation ||
           !P::validGeneration(op.to) || !P::validAction(op.action)) {
            out.status=Status::UnknownRouteOrHistory;return out;
        }
        if(!P::routePermitted(op.from,op.to,op.action) ||
           (op.mutated & ~P::maskFor(op.action))!=0u) {
            out.status=Status::CandidateRouteExcluded;return out;
        }
        if(!hop.beforeProjectionIndependentlyVerified ||
           !hop.afterProjectionIndependentlyVerified ||
           !hop.before.known || !hop.after.known ||
           (hop.before.known & ~nativeFields(op.from)) ||
           (hop.after.known & ~nativeFields(op.to)) ||
           (op.verifiedAfter.known & ~hop.after.known))return out;
        // If a source claims independent endpoint values but the actual
        // independently observed endpoint disagrees, evidence is incoherent.
        for(size_t f=0;f<P::kFields;++f) {
            const uint32_t bit=1u<<f;
            if((op.verifiedAfter.known&bit) &&
               op.verifiedAfter.value[f]!=hop.after.value[f])return out;
            if((tracked.known&hop.before.known&bit) &&
               tracked.value[f]!=hop.before.value[f])return out;
        }
        const auto reencoded=P::intrinsicallyReencoded(op.action);
        P::Snapshot next{};
        for(size_t f=0;f<P::kFields;++f) {
            const uint32_t bit=1u<<f;
            if(!(hop.after.known&bit))continue;
            if(op.verifiedAfter.known&bit) {
                next.known|=bit;next.value[f]=op.verifiedAfter.value[f];
            } else if(!(op.mutated&bit) && !(reencoded&bit) &&
                      (tracked.known&hop.before.known&bit) &&
                      tracked.value[f]==hop.before.value[f] &&
                      tracked.value[f]==hop.after.value[f]) {
                next.known|=bit;next.value[f]=tracked.value[f];
            }
            // Unknown transformation or missing independent observation:
            // DO NOT carry original bytes through an unwitnessed stage.
        }
        tracked=next;
        generation=op.to;
        expectedDigest=hop.afterRecordDigest;
        ++out.inspectedHops;
    }
    if(generation!=input.finalGeneration) {
        out.status=Status::UnknownRouteOrHistory;return out;
    }
    if(expectedDigest!=input.finalRecordDigest) {
        out.status=Status::UnknownRecordIdentity;return out;
    }
    for(size_t f=0;f<P::kFields;++f) {
        const uint32_t bit=1u<<f;
        if(!(input.final.known&bit))continue;
        if(!(tracked.known&bit))out.unknown|=bit;
        else if(tracked.value[f]!=input.final.value[f])out.contradicted|=bit;
    }
    out.destination=generation;
    out.status=out.contradicted?Status::CandidateFieldExcluded:
        out.unknown?Status::UnknownRouteOrHistory:Status::ConditionalCandidate;
    return out;
}
static_assert((nativeFields(P::Generation::I)&P::OriginalBall)==0);
static_assert((nativeFields(P::Generation::II)&P::Ability)==0);
static_assert((nativeFields(P::Generation::III)&P::MetDate)==0);
static_assert(nativeFields(P::Generation::IV)==P::kAllFields);
static_assert(!Result{}.authenticated && !Result{}.globallyInvalid);
} // namespace Legality::Gen14OriginalCurrentChainEvidence
