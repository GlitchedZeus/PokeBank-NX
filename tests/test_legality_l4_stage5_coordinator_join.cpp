#include "Legality/Gen14EventCoordinatorJoinEvidence.h"
#include "Legality/Gen14EventCoordinatorRoutes.h"
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <span>
namespace J=Legality::Gen14EventCoordinatorJoinEvidence;
namespace P=Legality::Gen14HistoricalEventPathEvidence;
namespace R=Legality::Gen14EventCoordinatorRoutes;
constexpr J::Digest hash(){J::Digest d{};d[0]=std::byte{0xa5};d[31]=std::byte{0x5a};return d;}
J::Input fixture() {
    J::Input v{};
    v.nativeRecordChecksumValidated=true;
    v.original.key={J::Family::Gen4MysteryGift,0,67,4,7200,58364,7,1};
    v.original.sourceKeyIndependentlyIdentified=true;
    v.original.originalArchiveIndependentlyAuthenticated=true;
    v.original.digestIndependentlyVerified=true;
    v.original.originalDigest=hash();
    v.original.releaseIndependentlyCorroborated=true;
    v.original.individualReceiptIndependentlyCorroborated=true;
    v.original.originalFieldsIndependentlyExtracted=true;
    v.crossLane.key=v.original.key;
    v.crossLane.pinnedStructuralProjectionIndependentlyChecked=true;
    v.crossLane.originalArchiveIndependentlyAuthenticated=true;
    v.crossLane.originalDigestIndependentlyVerified=true;
    v.crossLane.originalDigest=hash();
    v.nativeGeneration=P::Generation::IV;
    v.originalHistory.exactOriginalSourceIndependentlyAttested=true;
    v.originalHistory.originalFieldsIndependentlyExtracted=true;
    v.originalHistory.originalGeneration=P::Generation::IV;
    v.originalHistory.exhaustiveHistoricalChainVerified=true;
    for(size_t i=0;i<P::kFields;++i) {
        v.originalHistory.original.value[i]=100+i;
        v.observed.value[i]=100+i;
    }
    v.originalHistory.original.known=P::kAllFields;
    v.observed.known=P::kAllFields;
    return v;
}
int main(){
    unsigned cases=0;
    auto b=fixture();
    const auto pass=J::evaluate(b);
    assert(pass.outcome==J::Outcome::ConditionalCandidate);
    assert(pass.sourceIdentityAgreed&&pass.originalPayloadAgreed);
    assert(!pass.authenticated&&!pass.globallyInvalid&&!pass.globallyLegal);
    ++cases;
    // Exhaustive 15-field mismatch submasks, not only one-bit defects.
    for(uint32_t mask=1;mask<=P::kAllFields;++mask){
        auto f=b;
        for(size_t i=0;i<P::kFields;++i)
            if((mask>>i)&1u)f.observed.value[i]^=0x1000u;
        const auto r=J::evaluate(f);
        assert(r.outcome==J::Outcome::CandidateFieldExcluded);
        assert(r.excludedFieldMask==mask);
        assert(!r.globallyInvalid&&!r.globallyLegal&&!r.authenticated);
        ++cases;
    }
    // 247 distinct source-index claims; card-ID duplication does not
    // make the index optional. The same card ID with another index drifts.
    for(uint32_t i=0;i<247;++i) {
        auto f=b;f.original.key.sourceIndex=i;f.crossLane.key=f.original.key;
        assert(J::evaluate(f).outcome==J::Outcome::ConditionalCandidate);
        f.crossLane.key.sourceIndex=(i+1)%247;
        const auto r=J::evaluate(f);
        assert(r.outcome==J::Outcome::CrossLaneIdentityDrift);
        assert(!r.globallyInvalid);
        ++cases;
    }
    { auto f=b;f.original.key.cardId=0;f.crossLane.key.cardId=0;
      assert(J::evaluate(f).outcome==J::Outcome::ConditionalCandidate);
      f.crossLane.key.cardId=67;
      assert(J::evaluate(f).outcome==J::Outcome::CrossLaneIdentityDrift);
      cases+=2; }
    for(uint32_t i=0;i<32;++i){
        auto f=b;f.crossLane.originalDigest[i]^=std::byte{0x01};
        const auto r=J::evaluate(f);
        assert(r.outcome==J::Outcome::IncoherentArchive);
        assert(!r.authenticated&&!r.globallyInvalid);
        ++cases;
    }
    // Completely separate gates: empty proof, incompatible source family,
    // mismatched trainer, no original, release, receipt, fields, native.
    { auto f=b;f.nativeRecordChecksumValidated=false;assert(J::evaluate(f).outcome==J::Outcome::NativeUnavailable);++cases; }
    { auto f=b;f.original.key.sourceIndex=247;assert(J::evaluate(f).outcome==J::Outcome::UnknownSourceIdentity);++cases; }
    { auto f=b;f.original.key.family=J::Family::Gen3Distribution;assert(J::evaluate(f).outcome==J::Outcome::UnknownSourceIdentity);++cases; }
    { auto f=b;f.crossLane.key.tid++;assert(J::evaluate(f).outcome==J::Outcome::CrossLaneIdentityDrift);++cases; }
    { auto f=b;f.original.originalArchiveIndependentlyAuthenticated=false;assert(J::evaluate(f).outcome==J::Outcome::UnknownOriginalArchive);++cases; }
    { auto f=b;f.original.digestIndependentlyVerified=false;assert(J::evaluate(f).outcome==J::Outcome::UnknownOriginalArchive);++cases; }
    { auto f=b;f.original.releaseIndependentlyCorroborated=false;assert(J::evaluate(f).outcome==J::Outcome::UnknownRelease);++cases; }
    { auto f=b;f.original.individualReceiptIndependentlyCorroborated=false;assert(J::evaluate(f).outcome==J::Outcome::UnknownIndividualReceipt);++cases; }
    { auto f=b;f.original.originalFieldsIndependentlyExtracted=false;assert(J::evaluate(f).outcome==J::Outcome::UnknownOriginalFields);++cases; }
    { auto f=b;f.originalHistory.exhaustiveHistoricalChainVerified=false;assert(J::evaluate(f).outcome==J::Outcome::UnknownTransferHistory);++cases; }
    { auto f=b;f.observed.known=0;assert(J::evaluate(f).outcome==J::Outcome::UnknownTransferHistory);++cases; }
    { auto f=b;f.nativeGeneration=P::Generation::III;assert(J::evaluate(f).outcome==J::Outcome::UnknownTransferHistory);++cases; }
    // Actual verified evolution changes the immutable source-species field.
    // Raw nickname bytes require explicitly attested post-evolution values.
    {
        auto f=b;
        P::Step step{};step.action=P::Action::Evolution;step.from=P::Generation::IV;
        step.to=P::Generation::IV;step.independentlyVerified=true;
        step.mutated=P::Species;step.verifiedAfter.known=P::Species;
        step.verifiedAfter.value[0]=999;
        f.verifiedSteps=std::span<const P::Step>(&step,1);
        f.observed.value[0]=999;
        const auto r=J::evaluate(f);
        assert(r.outcome==J::Outcome::UnknownTransferHistory);
        assert(r.unknownFieldMask&P::NicknameRaw);
        ++cases;
        f.observed.known&=~P::NicknameRaw;
        assert(J::evaluate(f).outcome==J::Outcome::ConditionalCandidate);
        ++cases;
    }
    // Interlocked source-gate precedence: native mismatch is NOT admissible
    // when original release/receipt/provenance is unavailable.
    for(unsigned mask=0;mask<1024;++mask){
        auto f=b;f.original.sourceKeyIndependentlyIdentified=(mask&1)!=0;
        f.crossLane.pinnedStructuralProjectionIndependentlyChecked=(mask&2)!=0;
        f.original.originalArchiveIndependentlyAuthenticated=(mask&4)!=0;
        f.crossLane.originalArchiveIndependentlyAuthenticated=(mask&8)!=0;
        f.original.digestIndependentlyVerified=(mask&16)!=0;
        f.crossLane.originalDigestIndependentlyVerified=(mask&32)!=0;
        f.original.releaseIndependentlyCorroborated=(mask&64)!=0;
        f.original.individualReceiptIndependentlyCorroborated=(mask&128)!=0;
        f.original.originalFieldsIndependentlyExtracted=(mask&256)!=0;
        f.originalHistory.exhaustiveHistoricalChainVerified=(mask&512)!=0;
        f.observed.value[0]^=1;
        const auto r=J::evaluate(f);
        assert(!r.authenticated&&!r.globallyInvalid&&!r.globallyLegal);
        if(mask!=1023)assert(r.outcome!=J::Outcome::CandidateFieldExcluded);
        else assert(r.outcome==J::Outcome::CandidateFieldExcluded);
        ++cases;
    }
    assert(R::reconcile(pass,{})==J::Outcome::UnknownTransferHistory);
    R::Gate lane{};lane.state=R::LaneState::CandidateExcluded;
    assert(R::reconcile(pass,lane)==J::Outcome::CandidateFieldExcluded);
    lane.state=R::LaneState::ConditionalEvidence;
    assert(R::reconcile(pass,lane)==J::Outcome::ConditionalCandidate);
    std::cout<<"L4 Stage 5 source identity + per-candidate field/history joins: "<<cases<<" PASS\n";
}
