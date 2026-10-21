#include "Legality/Gen14OriginalSourceRightsEvidence.h"
#include <cassert>
#include <cstdint>
#include <iostream>
namespace G=Legality::Gen14OriginalSourceRightsEvidence;
constexpr G::Blob hash(uint8_t b){G::Blob a{};a[0]=std::byte{b};return a;}
struct Fixture {
    G::Research r{};
    G::Original o{};
    G::DistributionRights rights{};
    Fixture() {
        r.l4SourceProjectionIndependentlyChecked=true;
        r.q07AllIndexedRowsIndependentlyChecked=true;
        r.q08EverySourcePermissionIndependentlyChecked=true;
        r.l4Rows=r.q07Rows=247;r.q08SourcesChecked=14;
        r.q08AllRetailPayloadImportsBlocked=true;
        r.q08AllThirdPartyCodeImportsBlocked=true;
        r.q08AllRetailTableImportsBlocked=true;
        r.expectedL4SourceBlob=r.verifiedL4SourceBlob=hash(1);
        r.expectedQ07Blob=r.verifiedQ07Blob=hash(2);
        r.expectedQ08Blob=r.verifiedQ08Blob=hash(3);
        o.family=G::Family::MysteryGiftIV;
        o.independentOriginalSourceIndex=23;
        o.independentOriginalSourceIdentityVerified=true;
        o.originalArchiveIndependentlyAuthenticated=true;
        o.originalArchiveSha256[0]=std::byte{4};
        o.originalDigestIndependentlyVerified=true;
        o.originalFieldsIndependentlyExtracted=true;
        o.historicalReleaseIndependentlyCorroborated=true;
        o.individualReceiptIndependentlyVerified=true;
        o.originalToNativeIndividualChainIndependentlyVerified=true;
    }
};
int main() {
    using S=G::Status;
    Fixture f{};
    auto baseline=G::evaluate(f.r,f.o);
    assert(baseline.status==S::ConditionalSourceCandidate);
    assert(baseline.researchOnly && baseline.sourceCandidateConditional);
    assert(!baseline.originalArchiveAuthenticated && !baseline.individualReceiptAuthenticated);
    // If the source/receipt claim lacks independent proof, Q07 research cannot
    // substitute the missing original, however complete its structural row.
    for(unsigned mask=0;mask<256;++mask) {
        auto x=f;
        x.r.l4SourceProjectionIndependentlyChecked=bool(mask&1);
        x.r.q07AllIndexedRowsIndependentlyChecked=bool(mask&2);
        x.r.q08EverySourcePermissionIndependentlyChecked=bool(mask&4);
        x.r.q08AllRetailPayloadImportsBlocked=bool(mask&8);
        x.r.q08AllThirdPartyCodeImportsBlocked=bool(mask&16);
        x.r.q08AllRetailTableImportsBlocked=bool(mask&32);
        x.o.independentOriginalSourceIdentityVerified=bool(mask&64);
        x.o.originalArchiveIndependentlyAuthenticated=bool(mask&128);
        const auto v=G::evaluate(x.r,x.o,x.rights);
        assert((v.status==S::ConditionalSourceCandidate)==(mask==255));
        assert(!v.payloadEmbeddingPermitted&&!v.thirdPartyCodeImportPermitted);
        assert(!v.bulkRetailTableImportPermitted&&!v.globallyValid&&!v.globallyInvalid);
    }
    // Exhaust complete 16-bit source permission claims: even false future
    // "specific rights" or own-save consent never overrides Q08 blocked.
    uint64_t rightsMasks=0;
    for(unsigned mask=0;mask<65536;++mask) {
        auto x=f;
        x.rights.purportedRights=static_cast<G::Rights>(mask&255);
        x.rights.sourceSpecificWrittenGrantIndependentlyReviewed=bool(mask&256);
        x.rights.copiedMaterialWithinSpecificGrant=bool(mask&512);
        x.rights.receivingProjectLicenseCompatibilityReviewed=bool(mask&1024);
        x.rights.userOwnSaveConsentPresent=bool(mask&2048);
        x.o.historicalReleaseIndependentlyCorroborated=bool(mask&4096);
        x.o.individualReceiptIndependentlyVerified=bool(mask&8192);
        x.o.originalToNativeIndividualChainIndependentlyVerified=bool(mask&16384);
        x.r.q08SourcesChecked=mask&32768?14:13;
        const auto result=G::evaluate(x.r,x.o,x.rights);
        assert(!result.payloadEmbeddingPermitted&&!result.thirdPartyCodeImportPermitted);
        assert(!result.bulkRetailTableImportPermitted&&!result.globallyValid&&!result.globallyInvalid);
        assert(result.status!=S::ConditionalSourceCandidate ||
               ((mask&(4096|8192|16384|32768))==(4096|8192|16384|32768)));
        ++rightsMasks;
    }
    // Drift in ANY of the three pinned blobs must prevent L5 row admission.
    for(unsigned i=0;i<3;++i) {
        auto x=f;
        if(i==0)x.r.verifiedL4SourceBlob[0]^=std::byte{1};
        if(i==1)x.r.verifiedQ07Blob[0]^=std::byte{1};
        if(i==2)x.r.verifiedQ08Blob[0]^=std::byte{1};
        assert(G::evaluate(x.r,x.o).status==S::CrossLaneSourceDrift);
    }
    for(unsigned family=0;family<256;++family) {
        auto x=f;x.o.family=static_cast<G::Family>(family);
        const auto result=G::evaluate(x.r,x.o);
        const bool supported=family>=1 && family<=6;
        assert((result.status==S::ConditionalSourceCandidate)==supported);
        assert(!result.payloadEmbeddingPermitted);
    }
    for(unsigned n=0;n<=255;++n) {
        auto x=f;x.r.q08SourcesChecked=static_cast<uint8_t>(n);
        const auto result=G::evaluate(x.r,x.o);
        assert((result.status==S::ConditionalSourceCandidate)==(n==14));
        assert(!result.originalArchiveAuthenticated);
    }
    std::cout<<"L4 Stage6 research/provenance/rights: 256 source masks, "
             <<rightsMasks<<" rights combinations, 256 family and 256 Q08 count tags: PASS\n";
}
