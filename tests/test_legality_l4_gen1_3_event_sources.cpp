#include "Legality/Gen14EventDistributionSourceEvidence.h"
#include <cassert>
#include <cstdint>
#include <iostream>
namespace E=Legality::Gen14EventDistributionSourceEvidence;
constexpr E::Source hypothetical(E::Platform platform,E::Family family) {
    E::Source source{};
    source.platform=platform;source.family=family;
    source.grade=E::ArchiveGrade::OriginalAuthenticated;
    source.exactSourceIdentityIndependentlyProven=true;
    source.sourceMechanismIndependentlyProven=true;
    source.originalPayloadDigestVerified=true;
    source.originalPayloadSha256[0]=std::byte{0x3f};
    source.originalPayloadFieldsIndependentlyExtracted=true;
    source.distributionReleaseIndependentlyCorroborated=true;
    source.individualOriginalReceiptHistoryKnown=true;
    return source;
}
static_assert(E::evaluate(hypothetical(E::Platform::Gen2,E::Family::PCNYGift)).status==E::Status::ConditionalOriginalSource);
static_assert(E::evaluate(hypothetical(E::Platform::GameCube,E::Family::ShadowEncounter)).status==E::Status::UnsupportedHistoricalRoute);
int main() {
    using P=E::Platform;using F=E::Family;using G=E::ArchiveGrade;using S=E::Status;
    const E::Family families[] = {F::Unknown,F::StadiumGift,F::PCNYGift,F::MystryMew,F::WishmakerJirachi,F::ChannelJirachi,F::BerryFix,F::GBAWondercard,F::GameCubeGift,F::ShadowEncounter};
    uint64_t checked=0, positive=0;
    for(unsigned p=0;p<=4;++p)
    for(E::Family f:families)
    for(unsigned g=0;g<=5;++g)
    for(unsigned mask=0;mask<128;++mask) {
        auto x=hypothetical(static_cast<P>(p),f);
        x.grade=static_cast<G>(g);
        x.exactSourceIdentityIndependentlyProven=(mask&1u)!=0;
        x.sourceMechanismIndependentlyProven=(mask&2u)!=0;
        x.originalPayloadDigestVerified=(mask&4u)!=0;
        x.originalPayloadFieldsIndependentlyExtracted=(mask&8u)!=0;
        x.distributionReleaseIndependentlyCorroborated=(mask&16u)!=0;
        x.individualOriginalReceiptHistoryKnown=(mask&32u)!=0;
        if(!(mask&64u))x.originalPayloadSha256={};
        const bool expected=p!=0 && f!=F::Unknown && f!=F::ShadowEncounter &&
             E::platformFamilyCompatible(x.platform,x.family) &&
             (g==1||g==2) && mask==127u;
        const auto result=E::evaluate(x);
        assert((result.status==S::ConditionalOriginalSource)==expected);
        assert(result.distributionConditional==expected);
        assert(!result.payloadEmbeddingPermitted);
        ++checked;if(expected)++positive;
    }
    assert(checked==38400);
    // Even an authentic source is not necessarily licensed to redistribute.
    auto a=hypothetical(P::Gen3GBA,F::MystryMew);
    assert(E::evaluate(a).status==S::ConditionalOriginalSource);
    assert(!E::evaluate(a).payloadEmbeddingPermitted);
    a.rightsToRedistributeEstablished=true;
    assert(E::evaluate(a).payloadEmbeddingPermitted);
    // A distribution could have existed, but a source-free PKM proves no
    // particular player's historical receipt. Never elevate those records.
    a.individualOriginalReceiptHistoryKnown=false;
    assert(E::evaluate(a).status==S::UnknownHistoricalReceipt);
    // Reconstructed/altered/rumored event records remain untrusted even
    // if somebody supplies a fake checksum and plausible release date.
    for(auto grade:{G::Unknown,G::ReconstructedOrPlaceholder,G::Altered,G::Rumor}) {
        a=hypothetical(P::Gen3GBA,F::ChannelJirachi);a.grade=grade;
        assert(E::evaluate(a).status==S::UntrustedArchive);
    }
    std::cout<<"L4 V01-V06 original Gen I-III event sources: "<<checked
             <<" candidate states, "<<positive<<" strictly conditional: PASS\n";
}
