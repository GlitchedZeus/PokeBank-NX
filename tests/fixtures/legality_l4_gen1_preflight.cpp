#include "Legality/Gen1OriginalGiftNativePreflight.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
namespace N=Legality::Gen1OriginalGiftNativePreflight;
int main() {
    using S=N::RateStatus;
    for(unsigned tag=0;tag<256;++tag) {
        assert(N::supportedOriginalSourceFamily(static_cast<uint8_t>(tag),false)==(tag==1));
        assert(N::supportedOriginalSourceFamily(static_cast<uint8_t>(tag),true)==(tag==2));
    }
    for(unsigned tag=0;tag<8;++tag) {
        const bool stadium=bool(tag&1),cart=bool(tag&2),media=bool(tag&4);
        assert(N::verifiedOriginalMechanism(stadium,cart,media)==
               (stadium?media:cart));
    }
    N::RateHistory h{};
    uint64_t checked=0;
    for(unsigned value=0;value<=255;++value)
    for(unsigned rate=0;rate<=255;++rate) {
        h.originalAcquisitionByteIndependentlyAttested=true;
        h.originalAcquisitionByte=static_cast<uint8_t>(rate);
        h.exhaustiveUnchangedByteHistoryIndependentlyProven=true;
        h.independentTimeCapsuleReturnWitness=false;
        const auto result=N::compareRate(static_cast<uint8_t>(value),h);
        assert((result==S::OriginalRateMatches)==(value==rate));
        h.independentTimeCapsuleReturnWitness=true;
        h.exhaustiveUnchangedByteHistoryIndependentlyProven=false;
        h.independentReturnItemWitness=true;
        h.actualCrossingSnapshotIndependentlyVerified=true;
        h.returnByteVerifiedAsGen2HeldItem=true;
        h.returnItemByte=static_cast<uint8_t>(rate);
        assert((N::compareRate(static_cast<uint8_t>(value),h)==S::TradebackItemMatches)==(value==rate));
        h.actualCrossingSnapshotIndependentlyVerified=false;
        assert(N::compareRate(static_cast<uint8_t>(value),h)==S::Unknown);
        ++checked;
    }
    assert(checked==65536);
    {
        N::RateHistory forged{};
        forged.originalAcquisitionByteIndependentlyAttested=true;
        forged.originalAcquisitionByte=45;
        forged.independentTimeCapsuleReturnWitness=true;
        forged.independentReturnItemWitness=true;
        forged.actualCrossingSnapshotIndependentlyVerified=true;
        forged.returnItemByte=45;
        assert(N::compareRate(45,forged)==S::Unknown);
        forged.returnByteVerifiedAsGen2HeldItem=true;
        assert(N::compareRate(45,forged)==S::TradebackItemMatches);
    }
    for(unsigned mask=0;mask<32;++mask) {
        h.originalAcquisitionByteIndependentlyAttested=(mask&1)!=0;
        h.independentTimeCapsuleReturnWitness=(mask&2)!=0;
        h.independentReturnItemWitness=(mask&4)!=0;
        h.actualCrossingSnapshotIndependentlyVerified=(mask&8)!=0;
        h.exhaustiveUnchangedByteHistoryIndependentlyProven=(mask&16)!=0;
        h.returnByteVerifiedAsGen2HeldItem=true;
        if(!h.originalAcquisitionByteIndependentlyAttested)
            assert(N::compareRate(h.originalAcquisitionByte,h)==S::Unknown);
    }
    std::array<uint8_t,4> moves{1,2,3,0};
    std::array<uint8_t,5> dvs{0,1,2,3,15};
    std::array<uint8_t,4> ups{0,1,2,3};
    unsigned native=0;
    for(unsigned m=0;m<256;++m) {
        moves[2]=static_cast<uint8_t>(m);
        assert(N::bounded(25,20,moves,dvs,ups)==(m<=165));
        for(unsigned d=0;d<16;++d) {
            dvs[3]=static_cast<uint8_t>(d);
            assert(N::bounded(25,20,moves,dvs,ups)==(m<=165));
            ++native;
        }
    }
    assert(native==4096);
    moves[2]=165;dvs[3]=15;
    for(unsigned pp=0;pp<256;++pp) {
        ups[3]=static_cast<uint8_t>(pp);
        assert(N::bounded(25,20,moves,dvs,ups)==(pp<=3));
    }
    assert(!N::bounded(152,20,moves,dvs,{0,0,0,0}));
    assert(!N::bounded(25,0,moves,dvs,{0,0,0,0}));
    std::cout<<"Gen I native catch/tradeback byte witnesses "<<checked<<" plus "<<native<<" raw move/DV bounds: PASS\n";
}
