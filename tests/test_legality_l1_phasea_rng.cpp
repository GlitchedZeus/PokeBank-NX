#include "Legality/Gen5PkhexLcrngStage.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
namespace R=Legality::Gen5PkhexLcrngStage;
static R::Evidence original(uint64_t seed,uint16_t frames,uint32_t pid,bool wild=false) {
 R::Evidence e{};
 e.originalSaveIdentity=0x12345678;e.originalCreatureIdentity=0x87654321;
 e.originalSeed=seed;
 e.independentlyAttestedPhysicalOriginal=true;
 e.independentlyAttestedOriginalGame=true;e.originalGameVersion=21;
 e.independentlyAttestedOriginalSeed=true;
 e.independentlyAttestedOriginalPid=true;
 e.independentlyAttestedOriginalTrainer=true;
 e.independentlyAttestedWildXorRule=true;
 e.tid=12345;e.sid=54321;
 e.originalPid=pid;e.searchFrames=frames;e.applyWildXor=wild;
 return e;
}
int main(){
 uint64_t checked=0;
 constexpr std::array<uint64_t,5> seeds{{
 0ULL,1ULL,0x123456789ABCDEF0ULL,
 0xFFFFFFFFFFFFFFFFULL,0xDEADBEEF00112233ULL}};
 constexpr std::array<uint64_t,5> nextKnown{{
 0x0000000000269EC3ULL,0x5D588B656C2E2828ULL,
 0xDB90D64B78890373ULL,0xA2A7749A941F155EULL,
 0x7F26A558A59B67E2ULL}};
 for(unsigned i=0;i<seeds.size();++i) {
    assert(R::next(seeds[i])==nextKnown[i]);
    assert(R::prev(nextKnown[i])==seeds[i]);
    uint64_t step=seeds[i];
    assert(R::next32(step)==static_cast<uint32_t>(nextKnown[i]>>32));
    checked+=3;
 }
 for(uint32_t i=0;i<65536u;++i){
    uint64_t seed=0x123456789ABCDEF0ULL+i*0x100010001ULL;
    auto next=R::next(seed);
    assert(R::prev(next)==seed);
    assert(R::next(R::prev(seed))==seed);
    uint64_t stepped=seed;const uint32_t pid=R::next32(stepped);
    assert(pid==static_cast<uint32_t>(next>>32));
    auto witness=original(seed,1,pid);
    assert(R::evaluate(witness).status==R::Status::ConditionalRawSeedPidStageOnly);
    witness.originalPid^=1u;
    witness.exactOriginalAdvanceIndependentlyAttested=true;
    assert(R::evaluate(witness).status==R::Status::CandidateLocalRawPidMismatch);
    witness.exactOriginalAdvanceIndependentlyAttested=false;
    assert(R::evaluate(witness).status==R::Status::UnknownSearchBound);
    checked+=6;
 }
 for(uint32_t i=0;i<65536u;++i){
    const uint16_t tid=static_cast<uint16_t>(i),sid=static_cast<uint16_t>(i^0x55AA);
    const uint32_t pid=(i*0x9E3779B9u);
    const auto forced=R::wildXor(pid,tid,sid);
    assert(((forced>>31)^(forced&1u)^R::trainerBitXor(tid,sid))%2==0);
    assert(R::wildXor(forced,tid,sid)==forced);
    auto witness=original(0xF00DBAAF00000000ULL+i,1,0,true);
    uint64_t seed=witness.originalSeed;
    witness.tid=tid;witness.sid=sid;
    witness.originalPid=R::wildXor(R::next32(seed),tid,sid);
    assert(R::evaluate(witness).status==R::Status::ConditionalRawSeedPidStageOnly);
    witness.independentlyAttestedOriginalTrainer=false;
    assert(R::evaluate(witness).status==R::Status::UnknownTrainer);
    checked+=4;
 }
 auto missing=original(0,1,0);
 missing.independentlyAttestedOriginalGame=false;
 assert(R::evaluate(missing).status==R::Status::UnknownOriginalGame);
 missing=original(0,1,0);
 missing.originalGameVersion=27;
 assert(R::evaluate(missing).status==R::Status::UnknownOriginalGame);
 missing=original(0,1,0);
 missing.independentlyAttestedPhysicalOriginal=false;
 assert(R::evaluate(missing).status==R::Status::UnknownOriginalCapture);
 missing=original(0,1,0);
 missing.independentlyAttestedOriginalSeed=false;
 assert(R::evaluate(missing).status==R::Status::UnknownOriginalSeed);
 missing=original(0,1,0);
 missing.independentlyAttestedOriginalPid=false;
 assert(R::evaluate(missing).status==R::Status::UnknownOriginalPID);
 missing=original(0,0,0);
 assert(R::evaluate(missing).status==R::Status::UnknownSearchBound);
 missing=original(0,4097,0);
 assert(R::evaluate(missing).status==R::Status::UnknownSearchBound);
 missing=original(0,2,0);
 missing.exactOriginalAdvanceIndependentlyAttested=true;
 missing.advance=3;
 assert(R::evaluate(missing).status==R::Status::UnknownSearchBound);
 checked+=8;
 // Historic frame is an independently witnessed bounded candidate; even
 // matching the exact upstream 64-bit raw RNG does not certify source game.
 const uint64_t start=0x123456789ABCDEF0ULL;
 uint64_t seed=start;
 for(unsigned frame=0;frame<32;++frame){
    const uint32_t pid=R::next32(seed);
    auto v=original(start,32,pid);
    v.exactOriginalAdvanceIndependentlyAttested=true;
    v.advance=frame;
    const auto answer=R::evaluate(v);
    assert(answer.status==R::Status::ConditionalRawSeedPidStageOnly);
    assert(answer.checkedFrames==frame+1);
    assert(!answer.globalInvalidSupported&&!answer.globallyLegalSupported);
    checked+=3;
 }
 std::cout<<"L1 PKHeX LCRNG64 original seed, reverse/forward, 131072 inputs, "
          <<"bounded source PID stages "<<checked<<" checks PASS\n";
}
