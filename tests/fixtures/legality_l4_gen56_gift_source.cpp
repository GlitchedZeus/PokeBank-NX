#include "Legality/Gen56MysteryGiftSourceEvidence.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <span>
namespace G=Legality::Gen56MysteryGiftSourceEvidence;
template<size_t N>void w16(std::array<std::byte,N>& a,size_t off,uint16_t v){a[off]=std::byte(v);a[off+1]=std::byte(v>>8);}
template<size_t N>void set8(std::array<std::byte,N>& a,size_t off,uint8_t v){a[off]=std::byte(v);}
int main(){
 std::array<std::byte,0xCC> pgf{};std::array<std::byte,0x108> wc6{};std::array<std::byte,0x310> full{};
 w16(pgf,0,123);w16(pgf,2,456);w16(pgf,0x1A,25);w16(pgf,0xB0,0);
 set8(pgf,0xB3,1);set8(pgf,4,20);set8(pgf,0x1D,2);set8(pgf,0x0E,16);
 w16(wc6,0,0);w16(wc6,0x68,123);w16(wc6,0x6A,456);w16(wc6,0x82,25);
 set8(wc6,0x51,0);set8(wc6,0x6C,24);set8(wc6,0x85,2);set8(wc6,0x76,16);
 for(size_t i=0;i<wc6.size();++i)full[i+0x208]=wc6[i];
 set8(full,0,12);set8(full,0x1FF,4);
 auto p=G::inspect(pgf),w=G::inspect(wc6),f=G::inspect(full);
 assert(p.state==G::State::PokemonProjection&&p.format==G::Format::PGF);
 assert(w.state==G::State::PokemonProjection&&w.format==G::Format::WC6);
 assert(f.state==G::State::PokemonProjection&&f.format==G::Format::WC6Full);
 assert(f.card==0&&w.card==0&&f.origin==24&&f.restrictVersion==12&&f.restrictLanguage==4);
 assert(!w.versionRestrictionInWrapper&&!w.languageRestrictionInWrapper&&f.versionRestrictionInWrapper);
 assert(p.species==25&&p.tid==123&&p.sid==456&&p.origin==20&&p.ball==16);
 assert(G::inspect(std::span<const std::byte>(pgf).first(203)).state==G::State::Unavailable);
 assert(G::inspect(std::span<const std::byte>(full).first(783)).state==G::State::Unavailable);
 {auto x=pgf;set8(x,0xB3,2);assert(G::inspect(x).state==G::State::NonPokemon);
 set8(x,0xB3,3);assert(G::inspect(x).state==G::State::NonPokemon);
 set8(x,0xB3,255);assert(G::inspect(x).state==G::State::Unavailable);}
 {auto x=wc6;set8(x,0x51,1);assert(G::inspect(x).state==G::State::NonPokemon);
 set8(x,0x51,255);assert(G::inspect(x).state==G::State::Unavailable);}
 {auto x=wc6;w16(x,0x82,0);assert(G::inspect(x).state==G::State::Unavailable);}
 {auto x=pgf;set8(x,0x5C,2);assert(G::inspect(x).state==G::State::Unavailable);}
 G::Source s{};s.originalIndividualFileIdentified=true;s.originalArchiveDigestIndependentlyVerified=true;
 s.originalArchiveSha256[0]=std::byte{1};s.historicallyReleasedIndependentlyCorroborated=true;
 s.exactSourceRowLinkedToCardIndependentlyVerified=true;s.individualRecipientSaveIndependentlyVerified=true;
 s.giftReceiptLinkedToOriginalFileIndependentlyVerified=true;s.recipientToCurrentIndividualIndependentlyVerified=true;
 s.originalValuesPreservedToObservedIndependentlyVerified=true;
 s.eventBallIdentityIndependentlyVerified=true;s.eventRibbonIdentityIndependentlyVerified=true;
 s.restrictionRowIndependentlyVerified=true;s.giftHashMatchedRestrictionRowIndependentlyVerified=true;
 G::Native n{};n.validated=true;n.tid=123;n.sid=456;n.species=25;n.origin=20;n.language=2;n.ball=16;
 assert(G::evaluate(p,s,n).status==G::Finding::ConditionalCandidate);
 {auto x=p;x.ball=0;auto r=G::evaluate(x,s,n);
 assert(r.contradiction==0 && (r.unknown&G::Ball));}
 {auto x=n;x.origin=24;auto r=G::evaluate(w,s,x);
 assert(r.contradiction==0 && r.status==G::Finding::ConditionalCandidate);
 x.origin=25;r=G::evaluate(w,s,x);
 assert((r.contradiction&G::Version)!=0 && r.status==G::Finding::CandidateExcluded);
 assert(!r.globallyInvalid);}
 {auto x=n;x.tid=999;auto e=p;e.egg=1;
 assert((G::evaluate(e,s,x).contradiction&G::Trainer)==0);}

 {auto x=s;x.originalValuesPreservedToObservedIndependentlyVerified=false;
 n.species=26;assert(G::evaluate(p,x,n).status==G::Finding::UnknownHistoricalChange);n.species=25;}
 {auto x=s;x.originalArchiveDigestIndependentlyVerified=false;
 n.species=26;assert(G::evaluate(p,x,n).status==G::Finding::UnknownSource);n.species=25;}
 unsigned gates=0;
 for(unsigned mask=0;mask<4096;++mask){auto x=s;
 x.originalIndividualFileIdentified=mask&1;x.originalArchiveDigestIndependentlyVerified=mask&2;
 x.historicallyReleasedIndependentlyCorroborated=mask&4;x.exactSourceRowLinkedToCardIndependentlyVerified=mask&8;
 x.individualRecipientSaveIndependentlyVerified=mask&16;x.giftReceiptLinkedToOriginalFileIndependentlyVerified=mask&32;
 x.recipientToCurrentIndividualIndependentlyVerified=mask&64;
 x.originalValuesPreservedToObservedIndependentlyVerified=mask&128;
 x.eventBallIdentityIndependentlyVerified=mask&256;x.eventRibbonIdentityIndependentlyVerified=mask&512;
 x.restrictionRowIndependentlyVerified=mask&1024;x.giftHashMatchedRestrictionRowIndependentlyVerified=mask&2048;
 auto result=G::evaluate(p,x,n);
 assert(!result.globallyInvalid&&!result.globallyLegal&&!result.authenticated&&!result.payloadEmbeddingPermitted);
 assert((result.status==G::Finding::ConditionalCandidate)==(mask==4095));++gates;
 }
 for(unsigned k=0;k<65536;++k){auto x=s; x.allowedLanguages=uint16_t(k);
 auto result=G::evaluate(p,x,n);
 bool shouldFail=k!=0 && (k&(1u<<2))==0;
 assert((result.status==G::Finding::CandidateExcluded)==shouldFail);
 assert(!result.globallyInvalid);}
 unsigned malformed=0;
 for(unsigned value=0;value<256;++value){auto x=pgf;set8(x,0xB3,uint8_t(value));
 auto result=G::inspect(x); assert((result.state==G::State::PokemonProjection)==(value==1));
 ++malformed;}
 for(unsigned ball=0;ball<256;++ball){auto x=G::checkEggBall(5,uint8_t(ball),true);
 assert((x==G::EggBall::CompatibleDefault)==(ball==4));
 auto y=G::checkEggBall(6,uint8_t(ball),true);
 assert((y==G::EggBall::CandidateMismatch)==(ball==1||ball==16));
 assert(G::checkEggBall(6,uint8_t(ball),false)==G::EggBall::Unknown);
 assert(G::checkEggBall(5,uint8_t(ball),false)==G::EggBall::Unknown);}
 std::cout<<"Gen V/VI PGF/WC6/WC6Full source evidence: "<<gates<<" proof masks, 65536 language masks, "<<malformed<<" card-type tags, 256 ball identities PASS\n";
}
