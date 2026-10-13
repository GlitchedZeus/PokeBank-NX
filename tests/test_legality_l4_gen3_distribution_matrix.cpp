#include "Legality/Gen3DistributionWitnessEvidence.h"
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <utility>
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
    // An out-of-domain serialized source-version tag must be Unknown,
    // never positive evidence or a candidate exclusion from a fake row.
    // This includes the now-distinct original Bonus Disc and Channel media.
    unsigned sourceTags=0;
    for(unsigned code=0;code<256;++code) {
        auto a=c;
        a.originalSource=static_cast<W::OriginalSource>(code);
        auto altered=n;altered.ids.tid^=1;
        const auto verdict=W::evaluate(altered,a);
        const bool recognized=code>=1 && code<=9;
        assert((verdict.source==SS::ConditionalSource)==recognized);
        if(!recognized) {
            assert(verdict.mismatchMask==0);
            assert(verdict.conditionalMatchMask==0);
            assert(verdict.unknownMask==65535u);
        }
        assert(!verdict.globallyInvalid && !verdict.authenticated);
        ++sourceTags;
    }
    assert(sourceTags==256u);
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
    // A purported distribution with no corroborated historical release
    // cannot exclude a Pokémon based on claimed archived source fields.
    // Individual receipt remains independently Unknown, not authenticated.
    {auto a=c;a.historicalReleaseIndependentlyCorroborated=false;
        auto tampered=n;tampered.ids.tid^=1;
        auto r=W::evaluate(tampered,a);
        assert(r.source==SS::UnknownRelease);
        assert(r.mismatchMask==0 && r.conditionalMatchMask==0);
        assert(r.unknownMask==65535u && !r.globallyInvalid);}
    // A known language conversion means OT and nickname game-font bytes may
    // be re-encoded too. Their apparent byte mismatch must not cause a
    // source-specific exclusion without separately verified encoding history.
    {auto a=c;a.languageConversionPossible=true;
        auto changed=n;
        changed.language^=1;
        changed.nickname[2]^=std::byte{0x80};
        changed.otName[2]^=std::byte{0x80};
        const auto r=W::evaluate(changed,a);
        const uint32_t rawText=(1u<<4)|(1u<<5)|(1u<<6);
        assert((r.unknownMask&rawText)==rawText);
        assert((r.mismatchMask&rawText)==0);
        assert(!r.authenticated && !r.globallyInvalid);}
    // Distinct native OT and event-move contradictions exclude ONLY the
    // independently attested distribution candidate.
    {auto a=c;auto changed=n;
        changed.otName[0]^=std::byte{1};changed.moves[0]^=1;
        auto r=W::evaluate(changed,a);
        assert(r.mismatchMask==((1u<<6)|(1u<<11)));
        a.moves.history=H::Unknown;
        a.nicknameOrOTReencodingPossible=true;
        r=W::evaluate(changed,a);
        assert(r.mismatchMask==0);
        assert((r.unknownMask&((1u<<6)|(1u<<11)))==((1u<<6)|(1u<<11)));}
    // The original receiver title, original GC source, source digest,
    // original trainer encoding and event move history are independent facts.
    // A Pokémon from a documented GameCube transfer can retain a GC-origin
    // game value while the receiving GBA version is Sapphire/Ruby.
    {auto a=c;a.originalSource=W::OriginalSource::Colosseum;
     auto x=n;x.originGame=15;
     a.originGame.original=15;
     W::TransferHistory h{};
     h.route=W::TransferRoute::GameCubeToGba;
     h.originalToNativeRecordIndependentlyLinked=true;
     h.receivingGbaVersionIndependentlyVerified=true;
     h.receivingGbaVersion=1;
     h.gameCubeOriginalIndependentlyVerified=true;
     h.originalGameCubeVersion=W::OriginalSource::ColosseumBonusDisc;
     h.gameCubeToGbaDeliveryIndependentlyVerified=true;
     h.originalGameFontEncodingIndependentlyVerified=true;
     h.originalEventMovesHistoryIndependentlyVerified=true;
     const auto positive=W::evaluateOriginalHistory(x,a,h);
     assert(positive.status==W::JoinedStatus::ConditionalCandidate);
     assert(!positive.authenticated && !positive.globallyInvalid);
     for(unsigned mask=0;mask<32;++mask) {
         auto sparse=h;
         sparse.originalToNativeRecordIndependentlyLinked=bool(mask&1u);
         sparse.receivingGbaVersionIndependentlyVerified=bool(mask&2u);
         sparse.gameCubeOriginalIndependentlyVerified=bool(mask&4u);
         sparse.gameCubeToGbaDeliveryIndependentlyVerified=bool(mask&8u);
         sparse.originalGameFontEncodingIndependentlyVerified=bool(mask&16u);
         const auto result=W::evaluateOriginalHistory(x,a,sparse);
         assert((result.status==W::JoinedStatus::ConditionalCandidate)==
                (mask==31u));
         assert(!result.globallyInvalid);
     }
     // The source identity and the claimed GC transfer version must agree.
     auto wrongSource=a;
     wrongSource.originalSource=W::OriginalSource::XD;
     assert(W::evaluateOriginalHistory(x,wrongSource,h).status==
            W::JoinedStatus::UnknownRoute);
     wrongSource=a;wrongSource.family=W::Family::Wondercard;
     assert(W::evaluateOriginalHistory(x,wrongSource,h).status==
            W::JoinedStatus::UnknownRoute);
     // A source's original GC provenance is not a direct GBA gift route.
     auto local=h;local.route=W::TransferRoute::GbaLocal;
     local.nativeOriginVersionPreservedIndependentlyVerified=true;
     assert(W::evaluateOriginalHistory(x,a,local).status==
            W::JoinedStatus::UnknownRoute);
     // Exhaust all possible serialized source-family and GameCube-origin
     // tags. A future/unsupported family must not silently become a
     // GameCube distribution on account of plausible OT and event moves.
     unsigned routeDomain=0,conditionalGcFamilies=0;
     for(unsigned family=0;family<256;++family) {
         auto familyCandidate=a;
         familyCandidate.family=static_cast<W::Family>(family);
         const auto result=W::evaluateOriginalHistory(x,familyCandidate,h);
         const bool eligible=family==1u;
         assert((result.status==W::JoinedStatus::ConditionalCandidate)==eligible);
         if(eligible)++conditionalGcFamilies;
         ++routeDomain;
     }
     assert(routeDomain==256 && conditionalGcFamilies==1);
     for(unsigned original=0;original<256;++original) {
         auto sourceCandidate=a;
         sourceCandidate.originalSource=static_cast<W::OriginalSource>(original);
         const auto result=W::evaluateOriginalHistory(x,sourceCandidate,h);
         assert((result.status==W::JoinedStatus::ConditionalCandidate)==
                (original==static_cast<unsigned>(W::OriginalSource::ColosseumBonusDisc)));
         assert(!result.globallyInvalid && !result.authenticated);
         ++routeDomain;
     }
     assert(routeDomain==512);
     // Exact independently identified media distinguish the bonus disc,
     // Pokémon Channel and Colosseum/XD native gift route. Cross-wiring
     // their documented families cannot become conditional.
     for(auto pair:{std::pair{W::Family::Channel,W::OriginalSource::PokemonChannel},
                    std::pair{W::Family::GameCubeGift,W::OriginalSource::XD},
                    std::pair{W::Family::GameCubeGift,W::OriginalSource::Colosseum}}) {
         auto source=a;source.family=pair.first;
         source.originalSource=pair.second;
         auto route=h;route.originalGameCubeVersion=pair.second;
         assert(W::evaluateOriginalHistory(x,source,route).status==
                W::JoinedStatus::ConditionalCandidate);
         route.originalGameCubeVersion=W::OriginalSource::ColosseumBonusDisc;
         assert(W::evaluateOriginalHistory(x,source,route).status==
                W::JoinedStatus::UnknownRoute);
     }
     h.gameCubeOriginalIndependentlyVerified=false;
     assert(W::evaluateOriginalHistory(x,a,h).status==
            W::JoinedStatus::UnknownRoute);
     h.gameCubeOriginalIndependentlyVerified=true;
     h.originalGameFontEncodingIndependentlyVerified=false;
     auto raw=W::evaluateOriginalHistory(x,a,h);
     assert((raw.unknown&((1u<<4)|(1u<<5)|(1u<<6)))!=0);
     assert(raw.excluded==0);
     h.originalGameFontEncodingIndependentlyVerified=true;
     h.originalEventMovesHistoryIndependentlyVerified=false;
     auto moves=W::evaluateOriginalHistory(x,a,h);
     assert(moves.status==W::JoinedStatus::UnknownEncodingOrMoveHistory);
     h.originalEventMovesHistoryIndependentlyVerified=true;
     auto tampered=x;tampered.moves[0]^=1;
     auto contradiction=W::evaluateOriginalHistory(tampered,a,h);
     assert(contradiction.status==W::JoinedStatus::CandidateExcluded);
     assert(contradiction.excluded==(1u<<11));
     h.originalToNativeRecordIndependentlyLinked=false;
     assert(W::evaluateOriginalHistory(tampered,a,h).status==
            W::JoinedStatus::UnknownRoute);
     // An uncorroborated release may not use this stricter source join.
     h.originalToNativeRecordIndependentlyLinked=true;
     a.historicalReleaseIndependentlyCorroborated=false;
     assert(W::evaluateOriginalHistory(tampered,a,h).status==
            W::JoinedStatus::UnknownOriginalSource);}
    // An unavailable native record cannot establish field contradictions.
    {auto x=n;x.available=false;auto r=W::evaluate(x,c);
        assert(!r.nativeAvailable && r.mismatchMask==0 && !r.globallyInvalid);}
    std::cout<<"L4 Gen III distribution matrix: "<<sourceStates
             <<" source combinations, "<<positiveRoutes<<" conditional routes, "
             <<negatives<<" field negatives and mutable Unknown: PASS\n";
}
