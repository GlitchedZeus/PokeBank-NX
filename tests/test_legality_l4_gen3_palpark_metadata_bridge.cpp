#include "Legality/Gen3EventPalParkMetadataBridge.h"
#include "Encryption/Encryption3FRLG.h"
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <span>
#include <vector>
namespace B=Legality::Gen3EventPalParkMetadataBridge;
namespace P=Legality::Gen14HistoricalEventPathEvidence;
namespace N=Legality::Gen3DistributionNativePK3Evidence;
void w16(std::vector<std::byte>& b,size_t p,uint16_t v) {b[p]=std::byte(v&255);b[p+1]=std::byte(v>>8);}
void w32(std::vector<std::byte>& b,size_t p,uint32_t v) {w16(b,p,v&0xffffu);w16(b,p+2,v>>16);}
std::vector<std::byte> makePK3(size_t size,uint32_t pid){
  std::vector<std::byte> b(size);
  w32(b,0,pid);w16(b,4,0x1234);w16(b,6,0xabcd);
  b[0x12]=std::byte{2};b[0x13]=std::byte{2};
  b[0x08]=std::byte{0x21};b[0x14]=std::byte{0x45};
  w16(b,0x20,385); w16(b,0x22,155);
  w16(b,0x46,static_cast<uint16_t>(5u|(1u<<7)|(4u<<11)|(1u<<15)));
  w16(b,0x2c,85);w32(b,0x4c,0x80000000u);
  w16(b,0x1c,Encryption::checksum3FRLG(b));
  return b;
}
std::vector<std::byte> encode(std::span<const std::byte> s){
  std::unique_ptr<std::byte[]> ptr(Encryption::encryptArray3FRLG(s));
  return {ptr.get(),ptr.get()+s.size()};
}
int main(){
  size_t decoded=0,negative=0;
  for(const size_t size:{Encryption::SIZE_STORED3_FRLG,Encryption::SIZE_PARTY3_FRLG})
  for(uint32_t sv=0;sv<24;++sv){
    const auto plain=makePK3(size,sv+24u),encrypted=encode(plain);
    assert(encrypted!=plain);
    const auto untouched=encrypted;
    const auto native=N::inspectEncrypted(encrypted);
    assert(native.available && native.speciesInternal==385);
    assert(native.ids.tid==0x1234 && native.ids.sid==0xabcd);
    assert(native.ball==4 && native.originGame==1);
    auto obs=B::project(native);
    assert(!(obs.known&P::Species)); // internal Gen III ≠ National species ID
    obs.value[0]=385;obs.known|=P::Species;
    B::Source source{};
    source.independentOriginalEventIdentity=true;
    source.sourceToFinalRecordIdentityIndependentlyProven=true;
    source.palParkTransitionIndependentlyVerified=true;
    source.historicalChainExhaustivelyProven=true;
    source.originalSpeciesNationalIndependentlyVerified=true;
    source.originalSpeciesNational=385;
    auto good=B::evaluate(encrypted,obs,source);
    assert(good.originalPK3Available);
    assert(good.lineage.status==P::Status::ConditionalCandidate);
    assert(!good.authenticated && !good.lineage.authenticated);
    auto noSource=source;
    noSource.independentOriginalEventIdentity=false;
    assert(B::evaluate(encrypted,obs,noSource).lineage.status==P::Status::UnknownOriginalSource);
    noSource=source;noSource.historicalChainExhaustivelyProven=false;
    assert(B::evaluate(encrypted,obs,noSource).lineage.status==P::Status::UnknownOriginalSource);
    noSource=source;noSource.originalSpeciesNationalIndependentlyVerified=false;
    assert((B::evaluate(encrypted,obs,noSource).lineage.unknown&P::Species)!=0);
    auto badBall=obs;badBall.value[3]=16;
    const auto mismatch=B::evaluate(encrypted,badBall,source);
    assert(mismatch.lineage.status==P::Status::CandidateFieldContradicted);
    assert(mismatch.lineage.contradicted==P::OriginalBall);
    auto badOT=obs;badOT.value[1]++;
    assert(B::evaluate(encrypted,badOT,source).lineage.contradicted==P::TrainerIDs);
    // Pal Park reencodes raw text: without an original receipt's exact
    // after-transfer bytes, a native PK4 OT/name mismatch MUST be Unknown.
    auto text=obs;text.known|=P::NicknameRaw|P::OriginalTrainerRaw;
    text.value[9]=999;text.value[10]=999;
    const auto textR=B::evaluate(encrypted,text,source);
    assert(textR.lineage.status==P::Status::UnknownHistoricalTransformation);
    assert((textR.lineage.unknown&(P::NicknameRaw|P::OriginalTrainerRaw))!=0);
    auto tampered=encrypted;tampered[0x30]^=std::byte{0x20};
    assert(!B::evaluate(tampered,obs,source).originalPK3Available);
    auto badEgg=encrypted;badEgg[0x13]|=std::byte{1};
    assert(!B::evaluate(badEgg,obs,source).originalPK3Available);
    assert(encrypted==untouched);++decoded;negative+=4;
  }
  std::array<std::byte,1> tiny{};
  assert(!B::evaluate(tiny,{},{}).originalPK3Available);
  std::array<std::byte,0x88> wrongGen{};
  assert(!B::evaluate(wrongGen,{},{}).originalPK3Available);
  // The actual host application must pair a native PK4 to the source using
  // independently established provenance. We do NOT infer that from bytes.
  assert(decoded==48);
  std::cout<<"L4 Stage4 native encrypted PK3 Pal Park bridge: "<<decoded
           <<" layouts, "<<negative<<" negatives: PASS\n";
}
