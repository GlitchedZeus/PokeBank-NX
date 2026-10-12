#include "Legality/Gen3DistributionWitnessEvidence.h"
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
namespace W=Legality::Gen3DistributionWitnessEvidence;
using F=W::Family;using M=W::Mechanism;using H=W::History;using S=W::Status;using SS=W::SourceStatus;
constexpr W::OriginalCandidate attested(F f=F::Wishmaker,M m=M::PokemonGift) {
    W::OriginalCandidate c{};
    c.family=f;c.mechanism=m;c.originalSource=W::OriginalSource::Ruby;
    c.exactRouteIndependentlyVerified=true;
    c.originalArchiveIndependentlyAuthenticated=true;
    c.originalPayloadDigestVerified=true;
    c.originalPayloadSha256[0]=std::byte{0x91};
    c.originalTemplateIndependentlyAttested=true;
    c.historicalReleaseIndependentlyCorroborated=true;
    c.individualHistoricalReceiptKnown=true;
    return c;
}
static_assert(W::sourceStatus(attested())==SS::ConditionalSource);
static_assert(!W::compatible(F::BerryFix,M::PokemonGift));
static_assert(!W::compatible(F::ShadowEncounter,M::PokemonGift));
constexpr W::Native observed() {
    W::Native n{};n.available=true;
    n.speciesInternal=385;n.pid=0xa0411082;
    n.ids={1234,5678};n.trainerGender=1;n.language=2;
    n.nickname[0]=std::byte{0xbb};n.nickname[1]=std::byte{0xff};
    n.otName[0]=std::byte{0xc1};n.otName[1]=std::byte{0xff};
    n.originGame=1;n.metLevel=5;n.metLocation=31;n.ball=4;
    n.moves={1,2,3,4};n.heldItem=175;n.egg=false;n.fateful=true;
    n.ribbonBits=0x80000001u;
    return n;
}
template<class T> constexpr void add(W::Witness<T>& w,T v) {
    w.independentlyKnown=true;w.original=v;
    w.history=H::IndependentlyVerifiedUnchanged;
}
constexpr W::OriginalCandidate allFields() {
    auto c=attested();auto n=observed();
    add(c.speciesInternal,n.speciesInternal);add(c.pid,n.pid);add(c.ids,n.ids);
    add(c.trainerGender,n.trainerGender);add(c.language,n.language);
    add(c.nickname,n.nickname);add(c.otName,n.otName);
    add(c.originGame,n.originGame);add(c.metLevel,n.metLevel);
    add(c.metLocation,n.metLocation);add(c.ball,n.ball);add(c.moves,n.moves);
    add(c.heldItem,n.heldItem);add(c.egg,n.egg);add(c.fateful,n.fateful);
    add(c.ribbonBits,n.ribbonBits);return c;
}
static_assert(W::evaluate(observed(),allFields()).conditionalMatchMask==0xffffu);
int main() {
    const auto n=observed();const auto c=allFields();
    const auto yes=W::evaluate(n,c);
    assert(yes.source==SS::ConditionalSource && yes.conditionalMatchMask==65535u);
    assert(yes.mismatchMask==0 && yes.unknownMask==0);
    assert(!yes.authenticated && !yes.globallyInvalid);
    uint64_t sourceStates=0,positiveRoutes=0;
    // Every family/mechanism/source crosses missing and attested provenance.
    for(unsigned f=0;f<=10;++f)
    for(unsigned m=0;m<=4;++m)
    for(unsigned g=0;g<=7;++g)
    for(unsigned mask=0;mask<64;++mask) {
        auto a=attested(static_cast<F>(f),static_cast<M>(m));
        a.originalSource=static_cast<W::OriginalSource>(g);
        a.exactRouteIndependentlyVerified=(mask&1u)!=0;
        a.originalArchiveIndependentlyAuthenticated=(mask&2u)!=0;
        a.originalPayloadDigestVerified=(mask&4u)!=0;
        a.originalTemplateIndependentlyAttested=(mask&8u)!=0;
        a.historicalReleaseIndependentlyCorroborated=(mask&16u)!=0;
        a.individualHistoricalReceiptKnown=(mask&32u)!=0;
        const auto eval=W::evaluate(n,a);
        const bool fully=(g!=0 && f!=0 && m!=0 && W::compatible(a.family,a.mechanism) && mask==63);
        assert((eval.source==SS::ConditionalSource)==fully);
        assert(eval.mismatchMask==0 && eval.conditionalMatchMask==0);
        assert(eval.unknownMask==65535u);
        assert(!eval.authenticated && !eval.globallyInvalid);
        if(fully)++positiveRoutes;
        ++sourceStates;
    }
    assert(sourceStates==11u*5u*8u*64u);
    // A reconstructed or digest-less alleged archive cannot validate values.
    {auto a=c;a.originalArchiveIndependentlyAuthenticated=false;
        assert(W::evaluate(n,a).source==SS::UntrustedArchive);
        assert(W::evaluate(n,a).conditionalMatchMask==0);}
    {auto a=c;a.originalPayloadSha256={};
        assert(W::evaluate(n,a).source==SS::MissingPayloadDigest);
        assert(W::evaluate(n,a).conditionalMatchMask==0);}
    // A release-corroborated distribution is not a receipt; fields may be
    // conditional while provenance remains explicitly incomplete.
    {auto a=c;a.individualHistoricalReceiptKnown=false;
        const auto r=W::evaluate(n,a);
        assert(r.source==SS::MissingReceipt && r.conditionalMatchMask==65535u);
        assert(!r.authenticated);}
    // Negative candidate-only comparisons for each of the sixteen fields,
    // including mutable ribbons, item, original moves and fateful.
    unsigned negatives=0;
    for(unsigned field=0;field<W::FIELD_COUNT;++field){
        auto changed=n;
        switch(static_cast<W::Field>(field)){
        case W::Field::Species: changed.speciesInternal^=1;break;
        case W::Field::PID: changed.pid^=1;break;
        case W::Field::TrainerIDs: changed.ids.sid^=1;break;
        case W::Field::TrainerGender: changed.trainerGender^=1;break;
        case W::Field::Language: changed.language^=1;break;
        case W::Field::Nickname: changed.nickname[0]^=std::byte{1};break;
        case W::Field::OTName: changed.otName[0]^=std::byte{1};break;
        case W::Field::OriginGame: changed.originGame^=1;break;
        case W::Field::MetLevel: changed.metLevel^=1;break;
        case W::Field::MetLocation: changed.metLocation^=1;break;
        case W::Field::Ball: changed.ball^=1;break;
        case W::Field::Moves: changed.moves[3]^=1;break;
        case W::Field::HeldItem: changed.heldItem^=1;break;
        case W::Field::Egg: changed.egg=!changed.egg;break;
        case W::Field::Fateful: changed.fateful=!changed.fateful;break;
        case W::Field::RibbonBits: changed.ribbonBits^=1;break;
        case W::Field::Count: assert(false);break;
        }
        const uint32_t bit=1u<<field;
        const auto mismatch=W::evaluate(changed,c);
        assert(mismatch.mismatchMask==bit);
        assert(mismatch.field(static_cast<W::Field>(field))==S::CandidateMismatch);
        assert(mismatch.conditionalMatchMask==(65535u^bit));
        assert(!mismatch.globallyInvalid && !mismatch.authenticated);
        ++negatives;
    }
    // Lack of original histories is Unknown even when a present PKM differs.
    {auto a=c;a.moves.history=H::Unknown;a.nickname.history=H::Unknown;
        auto x=n;x.moves[0]^=1;x.nickname[0]^=std::byte{1};
        auto r=W::evaluate(x,a);
        assert(r.mismatchMask==0 && r.unknownMask==((1u<<11)|(1u<<5)));}
    // Independently verified final (post-tutor/transfer/evolution) comparisons
    // do not compare to a historical initial move/species by mistake.
    {auto a=c;a.moves.original={10,11,12,13};a.moves.history=H::IndependentlyVerifiedFinal;
        a.moves.independentlyVerifiedFinal=n.moves;
        a.speciesInternal.original=1;a.speciesInternal.history=H::IndependentlyVerifiedFinal;
        a.speciesInternal.independentlyVerifiedFinal=n.speciesInternal;
        assert(W::evaluate(n,a).conditionalMatchMask==65535u);
        a.moves.independentlyVerifiedFinal[3]^=1;
        assert(W::evaluate(n,a).mismatchMask==(1u<<11));}
    // Egg OT/nickname game-font ambiguity and language re-encoding quarantine.
    {auto a=c;a.nicknameOrOTReencodingPossible=true;a.languageConversionPossible=true;
        auto x=n;x.otName[0]^=std::byte{1};x.nickname[0]^=std::byte{1};x.language^=1;
        auto r=W::evaluate(x,a);
        assert(r.mismatchMask==0 && r.unknownMask==((1u<<4)|(1u<<5)|(1u<<6)));}
    // An unavailable native record cannot establish field contradictions.
    {auto x=n;x.available=false;auto r=W::evaluate(x,c);
        assert(!r.nativeAvailable && r.mismatchMask==0 && !r.globallyInvalid);}
    std::cout<<"L4 Gen III distribution matrix: "<<sourceStates
             <<" source combinations, "<<positiveRoutes<<" conditional routes, "
             <<negatives<<" field negatives and mutable Unknown: PASS\n";
}
