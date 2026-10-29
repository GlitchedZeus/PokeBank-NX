#include "Legality/Gen14OriginalCurrentChainEvidence.h"
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
namespace C=Legality::Gen14OriginalCurrentChainEvidence;
namespace P=C::P;
constexpr auto I=P::Generation::I, II=P::Generation::II;
constexpr auto III=P::Generation::III, IV=P::Generation::IV;
constexpr C::Digest digest(uint8_t v){C::Digest d{};d[0]=std::byte{v};d[31]=std::byte{0xa1};return d;}
struct Data {
    C::Input input{};
    std::array<C::Hop,2> hops{};
    Data() {
        input.original.originalGeneration=III;
        input.original.exactOriginalSourceIndependentlyAttested=true;
        input.original.originalFieldsIndependentlyExtracted=true;
        input.original.exhaustiveHistoricalChainVerified=true;
        input.original.original.known=C::nativeFields(III);
        for(size_t f=0;f<P::kFields;++f)
            input.original.original.value[f]=100+f;
        input.originalRecordDigest=digest(1);
        input.originalRecordDigestIndependentlyVerified=true;
        input.originalToFinalIndividualIndependentlyLinked=true;
        input.finalNativeIndependentlyValidated=true;
        input.finalRecordDigestIndependentlyVerified=true;
        input.finalRecordDigest=digest(2);
        input.finalGeneration=IV;
        auto& h=hops[0];
        h.operation.action=P::Action::PalPark;
        h.operation.from=III;h.operation.to=IV;
        h.operation.independentlyVerified=true;
        h.beforeRecordDigest=digest(1);h.afterRecordDigest=digest(2);
        h.beforeRecordDigestIndependentlyVerified=true;
        h.afterRecordDigestIndependentlyVerified=true;
        h.sameIndividualIndependentlyProven=true;
        h.beforeProjectionIndependentlyVerified=true;
        h.afterProjectionIndependentlyVerified=true;
        h.before=input.original.original;
        h.after=h.before;
        constexpr auto reencoded=P::MoveSet|P::NicknameRaw|
            P::OriginalTrainerRaw|P::Language|P::MetDate;
        h.operation.mutated=reencoded;
        h.operation.verifiedAfter.known=reencoded;
        h.after.known|=P::MetDate;
        for(size_t f=0;f<P::kFields;++f)
            if(reencoded&(1u<<f)) {
                h.after.value[f]=1000+f;
                h.operation.verifiedAfter.value[f]=1000+f;
            }
        input.final=h.after;
        input.history=std::span<const C::Hop>(hops.data(),1);
    }
};
int main() {
    using S=C::Status;
    Data data{};
    const auto good=C::evaluate(data.input);
    assert(good.status==S::ConditionalCandidate && good.inspectedHops==1);
    assert(good.contradicted==0 && good.unknown==0 &&
           !good.authenticated && !good.globallyInvalid && !good.globallyLegal);
    // Every submask of 15 independently observed fields is excluded only
    // from this one fully identified and witnessed original record chain.
    uint64_t maskCases=0;
    for(uint32_t mask=0;mask<=P::kAllFields;++mask) {
        auto x=data.input;
        for(size_t f=0;f<P::kFields;++f)
            if(mask&(1u<<f))x.final.value[f]^=0x100000u;
        const auto r=C::evaluate(x);
        assert(r.contradicted==mask);
        assert((r.status==S::CandidateFieldExcluded)==(mask!=0));
        assert(!r.authenticated && !r.globallyInvalid && !r.globallyLegal);
        ++maskCases;
    }
    assert(maskCases==32768);
    // All 8 independent source/record/pairing proof combinations are
    // required before any candidate exclusion is admissible.
    uint64_t sourceMasks=0;
    for(unsigned mask=0;mask<256;++mask) {
        Data d{};
        d.input.finalNativeIndependentlyValidated=bool(mask&1);
        d.input.original.exactOriginalSourceIndependentlyAttested=bool(mask&2);
        d.input.originalRecordDigestIndependentlyVerified=bool(mask&4);
        d.input.finalRecordDigestIndependentlyVerified=bool(mask&8);
        d.input.originalToFinalIndividualIndependentlyLinked=bool(mask&16);
        d.hops[0].beforeRecordDigestIndependentlyVerified=bool(mask&32);
        d.hops[0].afterRecordDigestIndependentlyVerified=bool(mask&64);
        d.hops[0].sameIndividualIndependentlyProven=bool(mask&128);
        const auto r=C::evaluate(d.input);
        assert((r.status==S::ConditionalCandidate)==(mask==255));
        assert(!r.authenticated && !r.globallyInvalid);
        ++sourceMasks;
    }
    // A native field absent from the original game's format cannot be
    // independently projected just by declaring it known in a manifest.
    auto corrupt=data.input;
    corrupt.original.original.known|=P::MetDate;
    assert(C::evaluate(corrupt).status==S::UnknownProjection);
    corrupt=data.input;corrupt.finalGeneration=III;
    assert(C::evaluate(corrupt).status==S::UnknownProjection);
    // Missing intermediate Ball evidence makes that field Unknown and MUST
    // never turn a mismatch into a candidate-specific contradiction.
    Data missing{};
    missing.hops[0].before.known&=~P::OriginalBall;
    missing.input.final.value[3]++;
    const auto unknownBall=C::evaluate(missing.input);
    assert(unknownBall.status==S::UnknownRouteOrHistory);
    assert(unknownBall.unknown&P::OriginalBall);
    assert(!(unknownBall.contradicted&P::OriginalBall));
    // Pal Park can reencode original raw OT/nickname/date metadata. Only
    // independently verified canonical endpoint bytes restore comparability.
    Data missingRaw{};
    missingRaw.hops[0].operation.verifiedAfter.known&=~(P::NicknameRaw|P::OriginalTrainerRaw);
    missingRaw.input.final.value[9]++;
    const auto unknownRaw=C::evaluate(missingRaw.input);
    assert(unknownRaw.status==S::UnknownRouteOrHistory);
    assert((unknownRaw.unknown&(P::NicknameRaw|P::OriginalTrainerRaw))==
           (P::NicknameRaw|P::OriginalTrainerRaw));
    assert((unknownRaw.contradicted&P::NicknameRaw)==0);
    // The intermediate source and destination digests are independently
    // verified, but a different original/recipient record is still UNKNOWN.
    Data drift{};
    drift.hops[0].beforeRecordDigest=digest(9);
    assert(C::evaluate(drift.input).status==S::UnknownRecordIdentity);
    Data driftFinal{};driftFinal.input.finalRecordDigest=digest(9);
    assert(C::evaluate(driftFinal.input).status==S::UnknownRecordIdentity);
    Data driftBefore{};driftBefore.hops[0].before.value[3]++;
    assert(C::evaluate(driftBefore.input).status==S::UnknownProjection);
    Data driftAfter{};driftAfter.hops[0].operation.verifiedAfter.value[7]++;
    assert(C::evaluate(driftAfter.input).status==S::UnknownProjection);
    Data driftMutation{};driftMutation.hops[0].operation.mutated|=P::TrainerIDs;
    assert(C::evaluate(driftMutation.input).status==S::CandidateRouteExcluded);
    // A malicious event manifest cannot claim a trusted post-transfer OT ID
    // replacement when Pal Park mechanics do not permit changing OT IDs.
    Data immutableOverride{};
    immutableOverride.hops[0].operation.verifiedAfter.known|=P::TrainerIDs;
    immutableOverride.hops[0].operation.verifiedAfter.value[1]=99999;
    immutableOverride.hops[0].after.value[1]=99999;
    assert(C::evaluate(immutableOverride.input).status==S::UnknownProjection);
    assert(!C::evaluate(immutableOverride.input).authenticated);
    // Independent evolution after Pal Park retains the source identity,
    // requires an intermediate record digest, and binds the new species.
    Data evolved{};
    auto& evo=evolved.hops[1];
    evo.operation={P::Action::Evolution,IV,IV,true,P::Species,{}};
    evo.operation.verifiedAfter.known=P::Species;
    evo.operation.verifiedAfter.value[0]=999;
    evo.before=evolved.hops[0].after;evo.after=evo.before;
    evo.after.value[0]=999;
    evo.beforeRecordDigest=digest(2);evo.afterRecordDigest=digest(3);
    evo.beforeRecordDigestIndependentlyVerified=true;
    evo.afterRecordDigestIndependentlyVerified=true;
    evo.sameIndividualIndependentlyProven=true;
    evo.beforeProjectionIndependentlyVerified=true;
    evo.afterProjectionIndependentlyVerified=true;
    evolved.input.final=evo.after;
    evolved.input.finalRecordDigest=digest(3);
    evolved.input.history=std::span<const C::Hop>(evolved.hops.data(),2);
    assert(C::evaluate(evolved.input).status==S::UnknownRouteOrHistory);
    // Evolution naturally rewrites raw nickname bytes. Independently
    // extracted post-evolution raw bytes are required for a full match.
    evo.operation.verifiedAfter.known|=P::NicknameRaw;
    evo.operation.verifiedAfter.value[9]=evo.after.value[9];
    assert(C::evaluate(evolved.input).status==S::ConditionalCandidate);
    evo.beforeRecordDigest=digest(13);
    assert(C::evaluate(evolved.input).status==S::UnknownRecordIdentity);
    // A legitimate I -> II -> I Time Capsule round trip is representable;
    // no Gen I Ball/Egg/held-item/ability metadata is manufactured.
    Data capsule{};
    capsule.input.original.originalGeneration=I;
    capsule.input.original.original.known=C::nativeFields(I);
    capsule.input.finalGeneration=I;
    capsule.input.originalRecordDigest=digest(4);
    capsule.input.finalRecordDigest=digest(6);
    auto& go=capsule.hops[0];
    go.operation={P::Action::TimeCapsule,I,II,true,0,{}};
    go.beforeRecordDigest=digest(4);go.afterRecordDigest=digest(5);
    go.beforeRecordDigestIndependentlyVerified=true;
    go.afterRecordDigestIndependentlyVerified=true;
    go.sameIndividualIndependentlyProven=true;
    go.beforeProjectionIndependentlyVerified=true;
    go.afterProjectionIndependentlyVerified=true;
    go.before=capsule.input.original.original;
    go.after=go.before;
    go.operation.verifiedAfter.known=P::NicknameRaw|P::OriginalTrainerRaw;
    go.operation.verifiedAfter.value[9]=go.after.value[9];
    go.operation.verifiedAfter.value[10]=go.after.value[10];
    auto& back=capsule.hops[1];
    back.operation={P::Action::TimeCapsule,II,I,true,0,{}};
    back.beforeRecordDigest=digest(5);back.afterRecordDigest=digest(6);
    back.beforeRecordDigestIndependentlyVerified=true;
    back.afterRecordDigestIndependentlyVerified=true;
    back.sameIndividualIndependentlyProven=true;
    back.beforeProjectionIndependentlyVerified=true;
    back.afterProjectionIndependentlyVerified=true;
    back.before=go.after;
    back.after=back.before;
    back.operation.verifiedAfter.known=P::NicknameRaw|P::OriginalTrainerRaw;
    back.operation.verifiedAfter.value[9]=back.after.value[9];
    back.operation.verifiedAfter.value[10]=back.after.value[10];
    capsule.input.final=back.after;
    capsule.input.history=std::span<const C::Hop>(capsule.hops.data(),2);
    assert(C::evaluate(capsule.input).status==S::ConditionalCandidate);
    // Neither PK1 nor PK2 independently stores a Pokémon language byte.
    // A source manifest pretending to project one must remain Unknown.
    capsule.input.original.original.known|=P::Language;
    assert(C::evaluate(capsule.input).status==S::UnknownProjection);
    capsule.input.original.original.known&=~P::Language;
    // A direct Gen II -> Gen III or Gen I -> Gen IV jump cannot be
    // invented by a source manifest; this does not invalidate the Pokémon.
    back.operation.action=P::Action::PalPark;
    back.operation.to=III;
    assert(C::evaluate(capsule.input).status==S::CandidateRouteExcluded);
    // All 256 original action labels: only the documented Pal Park action
    // may link Gen III to Gen IV; unknown labels cannot be accepted.
    uint64_t routeCases=0;
    for(unsigned action=0;action<256;++action) {
        Data r{};r.hops[0].operation.action=static_cast<P::Action>(action);
        const auto verdict=C::evaluate(r.input);
        if(action==static_cast<unsigned>(P::Action::PalPark))
            assert(verdict.status==S::ConditionalCandidate);
        else if(action<10)assert(verdict.status==S::CandidateRouteExcluded);
        else assert(verdict.status==S::UnknownRouteOrHistory);
        assert(!verdict.globallyInvalid);
        ++routeCases;
    }
    // Exactly 16 independently proven operations fit the retention bound.
    // A 17-hop claim is Unknown rather than an unbounded read.
    std::array<C::Hop,17> huge{};
    auto longInput=data.input;longInput.history=huge;
    assert(C::evaluate(longInput).status==S::UnknownProjection);
    std::cout<<"L4 Stage 5 original/current chains: "<<maskCases
             <<" field masks, "<<sourceMasks<<" proof masks, "<<routeCases
             <<" route tags, two-transfer lineage: PASS\n";
}
