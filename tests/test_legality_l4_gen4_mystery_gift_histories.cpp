#include "Legality/Gen4MysteryGiftHistoryEvidence.h"
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <vector>
namespace C=Legality::Gen4MysteryGiftContainerEvidence;
namespace H=Legality::Gen4MysteryGiftHistoryEvidence;
void w16(std::vector<std::byte>& b,std::size_t i,uint16_t v) {
    b[i]=static_cast<std::byte>(v);b[i+1]=static_cast<std::byte>(v>>8);
}
C::Projection card(C::GiftType type) {
    std::vector<std::byte> raw(C::PCDSize);w16(raw,0,static_cast<uint16_t>(type));
    w16(raw,C::PCDCardId,123);
    return C::inspect(raw);
}
C::Attestation attested() {
    C::Attestation a{};a.grade=C::ArchiveGrade::OriginalPCD;
    a.exactSourceIndependentlyIdentified=true;
    a.sourceDigest[0]=std::byte{0x99};
    a.digestIndependentlyVerified=true;a.originalFieldsIndependentlyExtracted=true;
    a.releasedIndependentlyCorroborated=true;a.recipientHistoryIndependentlyKnown=true;
    return a;
}
H::Observed observed() {
    H::Observed n{};n.nativeValid=true;n.species=25;n.tid=2345;n.sid=9876;
    n.language=2;n.originVersion=10;n.ball=4;n.egg=false;n.fateful=true;
    n.moves={10,20,30,40};n.ribbons={1,0x20u,0};
    n.rawOT[0]=std::byte{0x41};n.rawNickname[0]=std::byte{0x52};return n;
}
H::Source source(const H::Observed& n) {
    H::Source s{};s.originalFieldsIndependentlyVerified=true;
    s.giftType=C::GiftType::Pokemon;
    s.originalSpecies={true,n.species};s.originalTid={true,n.tid};
    s.originalSid={true,n.sid};s.originalLanguage={true,n.language};
    s.originalOriginVersion={true,n.originVersion};
    s.originalBall={true,n.ball};s.originalEgg={true,n.egg};
    s.originalFateful={true,n.fateful};s.originalMoves={true,n.moves};
    s.originalRawOT={true,n.rawOT};s.originalRawNickname={true,n.rawNickname};
    s.requiredAtReceiptRibbons={true,{1,0,0}};
    s.speciesHistory=s.trainerHistory=s.languageHistory=s.originHistory=
    s.eggHistory=s.ballHistory=s.movesHistory=s.rawOTHistory=
    s.nicknameHistory=H::History::Unchanged;
    s.ribbonRetentionIndependentlyVerified=true;
    return s;
}
int main() {
    auto n=observed();auto s=source(n);
    const auto p=card(C::GiftType::Pokemon);
    const auto a=attested();
    assert(H::evaluate(n,p,a,s).status==H::Status::ConditionalCandidate);
    uint32_t negatives=0, unknowns=0;
    // Individually mutate every original-stage comparison domain: a
    // candidate-only contradiction, never global Invalid.
    for(unsigned i=0;i<12;++i) {
        auto m=n;
        switch(i) {
        case 0:m.species=26;break;
        case 1:m.tid^=1;break;
        case 2:m.language=3;break;
        case 3:m.originVersion=11;break;
        case 4:m.egg=true;break;
        case 5:m.ball=2;break;
        case 6:m.moves[0]=99;break;
        case 7:m.ribbons[0]=0;break;
        case 8:m.rawOT[0]=std::byte{0x46};break;
        case 9:m.rawNickname[0]=std::byte{0x53};break;
        case 10:m.fateful=false;break;
        case 11:break;
        }
        auto checked=H::evaluate(m,p,a,s);
        if(i==10) {
            assert(checked.status==H::Status::UnresolvedHistory);
            assert((checked.unknown&H::Fateful)!=0);++unknowns;
        } else if(i<10) {
            assert(checked.status==H::Status::CandidateContradicted);
            assert(checked.contradicted==(1u<<i));++negatives;
        } else {
            auto mismatch=s;mismatch.giftType=C::GiftType::PokemonEgg;
            checked=H::evaluate(m,p,a,mismatch);
            assert(checked.status==H::Status::CandidateContradicted);
            assert(checked.contradicted==H::GiftType);++negatives;
        }
    }
    // Unknown transformation is not a contradiction even when data differ;
    // a separately verified final endpoint may restore a conditional path.
    auto evolved=n;evolved.species=26;
    auto history=s;history.speciesHistory=H::History::Unknown;
    assert(H::evaluate(evolved,p,a,history).status==H::Status::UnresolvedHistory);
    history.speciesHistory=H::History::VerifiedChangedFinal;
    assert(H::evaluate(evolved,p,a,history).status==H::Status::UnresolvedHistory);
    history.verifiedFinalSpecies={true,26};
    assert(H::evaluate(evolved,p,a,history).status==H::Status::ConditionalCandidate);
    auto changed=n;changed.moves={40,30,20,10};
    history=s;history.movesHistory=H::History::VerifiedChangedFinal;
    assert(H::evaluate(changed,p,a,history).status==H::Status::UnresolvedHistory);
    history.verifiedFinalMoves={true,changed.moves};
    assert(H::evaluate(changed,p,a,history).status==H::Status::ConditionalCandidate);
    auto traded=n;traded.tid=444;traded.sid=333;
    history=s;history.trainerHistory=H::History::Unknown;
    assert(H::evaluate(traded,p,a,history).status==H::Status::UnresolvedHistory);
    history.trainerHistory=H::History::VerifiedChangedFinal;
    history.verifiedFinalTrainer={true,{444,333}};
    assert(H::evaluate(traded,p,a,history).status==H::Status::ConditionalCandidate);
    // Egg hatching requires an independently attested later endpoint;
    // an intact present-day hatch must not exclude the original egg gift.
    auto eggSource=s;eggSource.giftType=C::GiftType::PokemonEgg;
    eggSource.originalEgg={true,true};eggSource.eggHistory=H::History::Unknown;
    assert(H::evaluate(n,card(C::GiftType::PokemonEgg),a,eggSource).status==H::Status::UnresolvedHistory);
    eggSource.eggHistory=H::History::VerifiedChangedFinal;
    eggSource.verifiedFinalEgg={true,false};
    assert(H::evaluate(n,card(C::GiftType::PokemonEgg),a,eggSource).status==H::Status::ConditionalCandidate);
    auto ranger=eggSource;ranger.giftType=C::GiftType::ManaphyEgg;
    assert(H::evaluate(n,card(C::GiftType::ManaphyEgg),a,ranger).status==H::Status::UnknownRangerReceipt);
    ranger.rangerGiftProofIndependentlyVerified=true;
    assert(H::evaluate(n,card(C::GiftType::ManaphyEgg),a,ranger).status==H::Status::ConditionalCandidate);
    assert(H::evaluate(n,card(C::GiftType::MemberCard),a,s).status==H::Status::NonPokemonGift);
    auto uncertain=a;uncertain.recipientHistoryIndependentlyKnown=false;
    assert(H::evaluate(n,p,uncertain,s).status==H::Status::UnknownOriginalSource);
    auto missing=n;missing.nativeValid=false;
    assert(H::evaluate(missing,p,a,s).status==H::Status::UnavailableNative);
    // Regression lattice across all original game codes and all meaningful
    // mutable history flags: no unjustified unconditional positive.
    uint64_t combinations=0, conditional=0;
    for(unsigned game=0;game<256;++game)
    for(unsigned flagMask=0;flagMask<256;++flagMask) {
        auto m=n;m.originVersion=static_cast<uint8_t>(game);
        auto t=s;
        if(flagMask&1) t.speciesHistory=H::History::Unknown;
        if(flagMask&2) t.trainerHistory=H::History::Unknown;
        if(flagMask&4) t.languageHistory=H::History::Unknown;
        if(flagMask&8) t.originHistory=H::History::Unknown;
        if(flagMask&16)t.eggHistory=H::History::Unknown;
        if(flagMask&32)t.ballHistory=H::History::Unknown;
        if(flagMask&64)t.movesHistory=H::History::Unknown;
        if(flagMask&128)t.nicknameHistory=H::History::Unknown;
        const auto r=H::evaluate(m,p,a,t);
        const bool exact=game==10 && flagMask==0;
        assert((r.status==H::Status::ConditionalCandidate)==exact);
        if(exact)++conditional;
        ++combinations;
    }
    assert(combinations==65536 && conditional==1 && negatives==11 && unknowns==1);
    std::cout<<"L4 Gen IV gift histories: "<<combinations
             <<" source/game states, "<<negatives<<" field negatives, mutable Unknown: PASS\n";
}
