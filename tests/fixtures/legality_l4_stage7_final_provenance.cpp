#include "Legality/Gen14FinalProvenanceGateEvidence.h"
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
namespace G=Legality::Gen14FinalProvenanceGateEvidence;
namespace H=G::H;
namespace R=G::R;
namespace P=H::P;
constexpr R::Blob digest20(uint8_t v){R::Blob d{};d[0]=std::byte{v};return d;}
constexpr R::Digest digest32(uint8_t v){R::Digest d{};d[0]=std::byte{v};d[31]=std::byte{0xaf};return d;}
struct Fixture {
    G::Input in{};
    std::array<H::Hop,1> steps{};
    Fixture() {
        auto& r=in.research;
        r.l4SourceProjectionIndependentlyChecked=true;
        r.q07AllIndexedRowsIndependentlyChecked=true;
        r.q08EverySourcePermissionIndependentlyChecked=true;
        r.expectedL4SourceBlob=r.verifiedL4SourceBlob=digest20(1);
        r.expectedQ07Blob=r.verifiedQ07Blob=digest20(2);
        r.expectedQ08Blob=r.verifiedQ08Blob=digest20(3);
        r.l4Rows=r.q07Rows=247;r.q08SourcesChecked=14;
        r.q08AllRetailPayloadImportsBlocked=true;
        r.q08AllThirdPartyCodeImportsBlocked=true;
        r.q08AllRetailTableImportsBlocked=true;
        auto& source=in.original;
        source.family=R::Family::MysteryGiftIV;
        source.independentOriginalSourceIndex=7;
        source.independentOriginalSourceIdentityVerified=true;
        source.originalArchiveIndependentlyAuthenticated=true;
        source.originalArchiveSha256=digest32(30);
        source.originalDigestIndependentlyVerified=true;
        source.originalFieldsIndependentlyExtracted=true;
        source.historicalReleaseIndependentlyCorroborated=true;
        source.individualReceiptIndependentlyVerified=true;
        source.originalToNativeIndividualChainIndependentlyVerified=true;
        auto& h=in.historical;
        h.finalNativeIndependentlyValidated=true;
        h.original.originalGeneration=P::Generation::IV;
        h.original.exactOriginalSourceIndependentlyAttested=true;
        h.original.originalFieldsIndependentlyExtracted=true;
        h.original.exhaustiveHistoricalChainVerified=true;
        h.original.original.known=P::kAllFields;
        for(unsigned j=0;j<P::kFields;++j)h.original.original.value[j]=300+j;
        h.originalRecordDigest=digest32(31);
        h.originalRecordDigestIndependentlyVerified=true;
        h.originalToFinalIndividualIndependentlyLinked=true;
        h.finalNativeIndependentlyValidated=true;
        h.final=h.original.original;
        h.finalGeneration=P::Generation::IV;
        h.finalRecordDigest=digest32(32);
        h.finalRecordDigestIndependentlyVerified=true;
        auto& op=steps[0];
        op.operation={P::Action::Trade,P::Generation::IV,P::Generation::IV,true,0,{}};
        op.beforeRecordDigest=h.originalRecordDigest;
        op.afterRecordDigest=h.finalRecordDigest;
        op.beforeRecordDigestIndependentlyVerified=true;
        op.afterRecordDigestIndependentlyVerified=true;
        op.sameIndividualIndependentlyProven=true;
        op.beforeProjectionIndependentlyVerified=true;
        op.afterProjectionIndependentlyVerified=true;
        op.before=h.original.original;op.after=h.final;
        h.history=std::span<const H::Hop>(steps.data(),1);
        auto& b=in.binding;
        b.originalDistributionFamily=source.family;
        b.originalDistributionIndex=source.independentOriginalSourceIndex;
        b.originalDistributionArchiveDigest=source.originalArchiveSha256;
        b.originalPokemonRecordDigest=h.originalRecordDigest;
        b.finalPokemonRecordDigest=h.finalRecordDigest;
        b.sourceRecordPairIndependentlyAttested=true;
        b.originalRecordWasExtractedFromAttestedSource=true;
        b.originalToCurrentIndividualIndependentlyAttested=true;
    }
};
int main() {
    using S=G::Status;
    Fixture f;
    auto r=G::evaluate(f.in);
    assert(r.status==S::ConditionalCandidate && r.researchCatalogsAgreed);
    assert(!r.originalEventAuthenticated && !r.individualReceiptAuthenticated);
    assert(!r.globallyInvalid && !r.globallyLegal && !r.payloadEmbeddingPermitted);
    unsigned proofMasks=0;
    for(unsigned mask=0;mask<256;++mask) {
        Fixture t;
        t.in.research.q07AllIndexedRowsIndependentlyChecked=bool(mask&1);
        t.in.research.q08EverySourcePermissionIndependentlyChecked=bool(mask&2);
        t.in.original.historicalReleaseIndependentlyCorroborated=bool(mask&4);
        t.in.original.individualReceiptIndependentlyVerified=bool(mask&8);
        t.in.binding.sourceRecordPairIndependentlyAttested=bool(mask&16);
        t.in.binding.originalRecordWasExtractedFromAttestedSource=bool(mask&32);
        t.in.binding.originalToCurrentIndividualIndependentlyAttested=bool(mask&64);
        t.in.historical.originalToFinalIndividualIndependentlyLinked=bool(mask&128);
        const auto a=G::evaluate(t.in);
        assert((a.status==S::ConditionalCandidate)==(mask==255));
        assert(!a.globallyLegal&&!a.globallyInvalid&&!a.payloadEmbeddingPermitted);
        ++proofMasks;
    }
    // Verify every observed final-record field; only an independently linked
    // candidate is excluded, NEVER the entire Pokémon.
    uint64_t masks=0;
    for(uint32_t mask=0;mask<=P::kAllFields;++mask) {
        Fixture t;
        for(unsigned bit=0;bit<P::kFields;++bit)
            if(mask&(1u<<bit))t.in.historical.final.value[bit]^=0xdeadbeefu;
        const auto a=G::evaluate(t.in);
        assert(a.candidateExcludedFields==mask);
        assert(a.status==(mask?S::CandidateFieldExcluded:S::ConditionalCandidate));
        assert(!a.globallyInvalid&&!a.globallyLegal&&!a.payloadEmbeddingPermitted);
        ++masks;
    }
    // Original archive SHA-256 is not an original PKM SHA-256, and neither
    // one can be silently replaced by the other, even if both look valid.
    for(unsigned variant=0;variant<8;++variant) {
        Fixture t;
        if(variant&1)t.in.binding.originalDistributionArchiveDigest=digest32(31);
        if(variant&2)t.in.binding.originalPokemonRecordDigest=digest32(30);
        if(variant&4)t.in.binding.finalPokemonRecordDigest=digest32(30);
        auto a=G::evaluate(t.in);
        assert(a.status==(variant?S::UnknownSourceRecordBinding:S::ConditionalCandidate));
        assert(!a.originalEventAuthenticated&&!a.individualReceiptAuthenticated);
    }
    // A purported license or receipt claim cannot turn research-only source
    // parity into permission to package copyrighted original card bytes.
    for(unsigned m=0;m<256;++m) {
        Fixture t;
        t.in.rights.purportedRights=static_cast<R::Rights>(m);
        t.in.rights.sourceSpecificWrittenGrantIndependentlyReviewed=true;
        t.in.rights.copiedMaterialWithinSpecificGrant=true;
        t.in.rights.receivingProjectLicenseCompatibilityReviewed=true;
        t.in.rights.userOwnSaveConsentPresent=true;
        const auto a=G::evaluate(t.in);
        assert(a.status==S::ConditionalCandidate);
        assert(!a.payloadEmbeddingPermitted && !a.thirdPartyImportPermitted);
    }
    // Multiple plausible sources retain Unknown if one is missing history.
    Fixture valid,excluded,unknown;
    excluded.in.historical.final.value[1]++;
    unknown.in.binding.originalRecordWasExtractedFromAttestedSource=false;
    const std::array<G::Input,3> all{valid.in,excluded.in,unknown.in};
    auto aggregate=G::evaluateAll(all);
    assert(aggregate.conditional==1&&aggregate.excluded==1&&aggregate.unknown==1);
    assert(aggregate.status==G::AggregateStatus::ConditionalCandidatesExist);
    assert(!aggregate.globallyInvalid&&!aggregate.originalEventAuthenticated);
    const std::array<G::Input,2> negative{excluded.in,excluded.in};
    aggregate=G::evaluateAll(negative);
    assert(aggregate.status==G::AggregateStatus::AllProvidedCandidatesExcluded);
    assert(!aggregate.globallyInvalid);
    const std::array<G::Input,2> ambiguous{excluded.in,unknown.in};
    aggregate=G::evaluateAll(ambiguous);
    assert(aggregate.status==G::AggregateStatus::AmbiguousOrUnknown);
    assert(G::evaluateAll(std::span<const G::Input>{}).inspected==0);
    std::array<G::Input,65> tooMany{};
    assert(G::evaluateAll(tooMany).inspected==0);
    // Unexpected version/family labels are not evidence of a source match.
    unsigned tags=0;
    for(unsigned family=0;family<256;++family) {
        Fixture t;t.in.original.family=static_cast<R::Family>(family);
        t.in.binding.originalDistributionFamily=t.in.original.family;
        const auto a=G::evaluate(t.in);
        const bool known=family>=1 && family<=6;
        assert((a.status==S::ConditionalCandidate)==known);
        assert(!a.originalEventAuthenticated&&!a.globallyInvalid);
        ++tags;
    }
    std::cout<<"L4 Stage 7 final source/record/rights gate: "<<masks
             <<" field masks, "<<proofMasks<<" proof masks, "<<tags
             <<" family labels, candidate-local multi-source verdicts: PASS\n";
}
