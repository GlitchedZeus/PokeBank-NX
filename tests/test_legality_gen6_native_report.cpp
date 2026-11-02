#include "Legality/Legality.h"
#include "Legality/Gen6NativePK6Evidence.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <span>
#include <vector>
#ifdef NDEBUG
#error assertions disabled
#endif
using namespace Legality;
static void write16(std::array<uint8_t,232>& raw,size_t offset,uint16_t v){raw[offset]=uint8_t(v);raw[offset+1]=uint8_t(v>>8);}
static std::array<uint8_t,232> make(uint16_t species,uint8_t nature=0,uint8_t ev=0) {
    std::array<uint8_t,232> b{};
    write16(b,8,species);b[0x1c]=nature;b[0x1e]=ev;
    uint32_t sum=0;
    for(size_t i=8;i<b.size();i+=2)sum+=Gen6NativePK6::read16(b,i);
    write16(b,6,uint16_t(sum));return b;
}
int main(){
    const auto legal=make(25);const auto backup=legal;
    auto valid=analyzePk6Record(legal,false);
    assert(valid.verdict()==Verdict::Incomplete && valid.ok() && !valid.hasInvalid());
    assert(valid.coverage.structure==CoverageLevel::Complete && legal==backup);
    auto badNature=analyzePk6Record(make(25,25),false);
    assert(badNature.verdict()==Verdict::Invalid);
    auto badSpecies=analyzePk6Record(make(722),false);
    assert(badSpecies.verdict()==Verdict::Invalid);
    auto badEv=analyzePk6Record(make(25,0,253),false);
    assert(badEv.verdict()==Verdict::Invalid);
    auto badChecksum=legal;badChecksum[8]^=1;
    auto questionable=analyzePk6Record(badChecksum,false);
    assert(questionable.verdict()==Verdict::Incomplete && !questionable.hasInvalid());
    auto shortInput=analyzePk6Record(std::span<const uint8_t>(legal.data(),2),false);
    assert(shortInput.verdict()==Verdict::Incomplete && !shortInput.hasInvalid());
    auto empty=analyzePk6Record(make(0),false);
    assert(empty.verdict()==Verdict::Incomplete && !empty.hasInvalid());
    std::cout<<"PK6 production report: complete structural valid, invalid fields, unknown framing, immutable input PASS\n";
}
