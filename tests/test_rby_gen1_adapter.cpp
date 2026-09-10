#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include "Integration/Gen1/Gen1ReadOnlySave.h"
#include "sav/Sav1.hpp"
#include "pkx/PKX.hpp"

using namespace PokeVault::Integration::Gen1;

namespace {
struct FixtureLayout {
    RegionLayout region;
    size_t money, tid, currentBoxIndex, party, currentBox, checksum, mainLength;
    size_t starter, friendship;
    uint8_t boxCapacity, boxCount, stringLength;
    size_t boxSize, bankBoxesSize;
};

// Constants are copied into this oracle fixture from the pinned external implementations, not from
// production headers: PKHeX e15d246... SAV1Offsets/PokeList1 and PKSM-Core aa22d7... Sav1/PK1.
constexpr FixtureLayout INTL{RegionLayout::International,
    0x25F3,0x2605,0x284C,0x2F2C,0x30C0,0x3523,0x0F8B,0x29C3,0x271C,20,12,11,0x462,0x1A4C};
constexpr FixtureLayout JPN{RegionLayout::Japanese,
    0x25EE,0x25FB,0x2842,0x2ED5,0x302D,0x3594,0x0FFC,0x29B9,0x2712,30,8,6,0x566,0x1598};

uint8_t diff8(std::span<const uint8_t> s) {
    uint8_t v = 0xFF;
    for (uint8_t b : s) v = static_cast<uint8_t>(v - b);
    return v;
}

void putBE16(std::vector<uint8_t>& d, size_t o, uint16_t v) {
    d[o] = static_cast<uint8_t>(v >> 8); d[o+1] = static_cast<uint8_t>(v);
}
void putBE24(std::vector<uint8_t>& d, size_t o, uint32_t v) {
    d[o] = static_cast<uint8_t>(v >> 16); d[o+1] = static_cast<uint8_t>(v >> 8); d[o+2] = static_cast<uint8_t>(v);
}

std::vector<uint8_t> encIntl(const std::string& s, size_t length) {
    std::vector<uint8_t> out(length, 0x50);
    for (size_t i=0; i<s.size() && i<length; ++i) {
        const char c=s[i];
        if (c>='A' && c<='Z') out[i]=static_cast<uint8_t>(0x80+(c-'A'));
        else if (c>='a' && c<='z') out[i]=static_cast<uint8_t>(0xA0+(c-'a'));
        else if (c>='0' && c<='9') out[i]=static_cast<uint8_t>(0xF6+(c-'0'));
        else if (c==' ') out[i]=0x7F;
        else assert(false && "fixture encoder only supports simple Gen I glyphs");
    }
    return out;
}

std::vector<uint8_t> encJP(std::initializer_list<uint8_t> glyphs, size_t length) {
    std::vector<uint8_t> out(length, 0x50);
    size_t i=0; for (uint8_t b:glyphs) { assert(i<length); out[i++]=b; }
    return out;
}

size_t boxStart(const FixtureLayout& l, size_t box) {
    const size_t half=l.boxCount/2;
    return box<half ? 0x4000+box*l.boxSize : 0x6000+(box-half)*l.boxSize;
}

void initEmptyList(std::vector<uint8_t>& d, size_t offset, size_t capacity) {
    d[offset]=0;
    std::fill(d.begin()+offset+1,d.begin()+offset+capacity+2,0);
    d[offset+1]=0xFF;
}

void putString(std::vector<uint8_t>& d, size_t offset, const std::vector<uint8_t>& value) {
    std::copy(value.begin(), value.end(), d.begin()+offset);
}

void fillBody(std::vector<uint8_t>& d, size_t o, size_t bodySize, uint8_t rawSpecies,
              uint8_t level, uint16_t tid, uint32_t exp, uint16_t dvs=0xA987) {
    std::fill(d.begin()+o,d.begin()+o+bodySize,0);
    d[o]=rawSpecies;
    putBE16(d,o+1,45);
    d[o+3]=level;
    d[o+4]=0;
    d[o+5]=0x14; d[o+6]=0x14;
    d[o+7]=45;
    d[o+8]=33; d[o+9]=45; d[o+10]=0; d[o+11]=0;
    putBE16(d,o+0x0C,tid);
    putBE24(d,o+0x0E,exp);
    putBE16(d,o+0x11,1234); putBE16(d,o+0x13,2345); putBE16(d,o+0x15,3456);
    putBE16(d,o+0x17,4567); putBE16(d,o+0x19,5678);
    putBE16(d,o+0x1B,dvs);
    d[o+0x1D]=0xC0|20; d[o+0x1E]=15; d[o+0x1F]=0; d[o+0x20]=0;
    if (bodySize==44) {
        d[o+0x21]=level;
        putBE16(d,o+0x22,52); putBE16(d,o+0x24,31); putBE16(d,o+0x26,28);
        putBE16(d,o+0x28,35); putBE16(d,o+0x2A,30);
    }
}

void setListEntry(std::vector<uint8_t>& d, size_t offset, size_t capacity, size_t bodySize,
                  size_t strLen, size_t index, uint8_t species, uint8_t level,
                  const std::vector<uint8_t>& ot, const std::vector<uint8_t>& nick) {
    assert(index<capacity);
    const size_t bodyStart=offset+capacity+2;
    const size_t otStart=bodyStart+capacity*bodySize;
    const size_t nickStart=otStart+capacity*strLen;
    d[offset]=static_cast<uint8_t>(std::max<size_t>(d[offset],index+1));
    d[offset+1+index]=species;
    d[offset+1+d[offset]]=0xFF;
    fillBody(d,bodyStart+index*bodySize,bodySize,species,level,0x1234,12345+index);
    putString(d,otStart+index*strLen,ot);
    putString(d,nickStart+index*strLen,nick);
}

void finalizeChecksums(std::vector<uint8_t>& d, const FixtureLayout& l, bool initialized) {
    if (initialized) {
        const size_t half=l.boxCount/2;
        for (int bank=0;bank<2;++bank) {
            const size_t start=bank==0?0x4000:0x6000;
            const size_t sumOfs=start+l.bankBoxesSize;
            for (size_t i=0;i<half;++i)
                d[sumOfs+1+i]=diff8(std::span<const uint8_t>(d).subspan(start+i*l.boxSize,l.boxSize));
            d[sumOfs]=diff8(std::span<const uint8_t>(d).subspan(start,l.bankBoxesSize));
        }
    }
    d[l.checksum]=diff8(std::span<const uint8_t>(d).subspan(0x2598,l.mainLength));
}

std::vector<uint8_t> makeFixture(SourceGame game, const FixtureLayout& l, bool initialized=true) {
    std::vector<uint8_t> d(0x8000,0);
    const bool jp=l.region==RegionLayout::Japanese;
    putString(d,0x2598,jp?encJP({0x80,0x81,0x82},l.stringLength):encIntl("WILL",l.stringLength));
    d[l.money]=0x12; d[l.money+1]=0x34; d[l.money+2]=0x56;
    putBE16(d,l.tid,0x1234);
    d[l.starter]=(game==SourceGame::Yellow)?0x54:0x99; // Pikachu vs Bulbasaur internal IDs.
    d[l.friendship]=(game==SourceGame::Yellow)?90:0;
    d[l.currentBoxIndex]=static_cast<uint8_t>((initialized?0x80:0)|2);

    initEmptyList(d,l.party,6);
    const auto ot=jp?encJP({0x85,0x86},l.stringLength):encIntl("RED",l.stringLength);
    const auto pika=jp?encJP({0x8A,0x8B},l.stringLength):encIntl("SPARKY",l.stringLength);
    const auto rhydon=jp?encJP({0x90,0x91},l.stringLength):encIntl("RHYDON",l.stringLength);
    setListEntry(d,l.party,6,44,l.stringLength,0,0x54,15,ot,pika); // raw 0x54 -> Pikachu #25
    setListEntry(d,l.party,6,44,l.stringLength,1,0x01,20,ot,rhydon); // raw 0x01 -> Rhydon #112

    initEmptyList(d,l.currentBox,l.boxCapacity);
    setListEntry(d,l.currentBox,l.boxCapacity,33,l.stringLength,0,0x99,8,ot,
                 jp?encJP({0x94,0x95},l.stringLength):encIntl("BULBA",l.stringLength));

    for (size_t box=0;box<l.boxCount;++box) initEmptyList(d,boxStart(l,box),l.boxCapacity);
    if (initialized) {
        setListEntry(d,boxStart(l,0),l.boxCapacity,33,l.stringLength,0,0xB0,12,ot,
                     jp?encJP({0x96,0x97},l.stringLength):encIntl("CHAR",l.stringLength));
        const size_t last=l.boxCount-1;
        for (size_t slot=0;slot<l.boxCapacity;++slot)
            setListEntry(d,boxStart(l,last),l.boxCapacity,33,l.stringLength,slot,0x15,
                         static_cast<uint8_t>(5+(slot%20)),ot,
                         jp?encJP({0x98},l.stringLength):encIntl("MEW",l.stringLength));
    } else {
        std::fill(d.begin()+boxStart(l,0),d.begin()+boxStart(l,0)+l.boxSize,0xA5);
    }

    if (!jp) d[0x2ED5]=0xFF;
    finalizeChecksums(d,l,initialized);
    return d;
}

void oracleCheckPKSM(const std::vector<uint8_t>& fixture, const FixtureLayout& l) {
    auto bytes=std::shared_ptr<uint8_t[]>(new uint8_t[fixture.size()]);
    std::copy(fixture.begin(),fixture.end(),bytes.get());
    assert(pksm::Sav1::isValid(bytes));
    pksm::Sav1 sav(bytes,static_cast<uint32_t>(fixture.size()));
    assert(sav.TID()==0x1234);
    // Do not use Sav1::money() as an oracle at this pinned revision. Its BigEndian BCD helper has
    // both a non-advancing multiplier and off-by-one reverse-iterator bounds, so it reads outside
    // the three-byte money field. PKHeX e15d246 independently confirms Gen I money is three-byte
    // big-endian packed BCD; the production assertion below must still equal 123456.
    assert(sav.currentBox()==2);
    assert(sav.partyCount()==2);
    assert(sav.maxBoxes()==l.boxCount);
    auto first=sav.pkm(0);
    assert(first && static_cast<uint16_t>(first->species())==25);
    auto boxed=sav.pkm(2,0);
    assert(boxed && static_cast<uint16_t>(boxed->species())==1);
}

void assertCore(SourceGame game, const FixtureLayout& l) {
    auto f=makeFixture(game,l,true);
    const auto before=f;
    auto r=parse(f,game);
    assert(r && r.error==SaveError::None);
    assert(f==before);
    assert(r.save->sourceBytes().size()==f.size());
    assert(std::equal(r.save->sourceBytes().begin(),r.save->sourceBytes().end(),f.begin()));
    assert(r.save->metadata().sourceGameId==sourceGameId(game));
    assert(r.save->metadata().region==l.region);
    assert(r.save->metadata().currentBox==2);
    assert(r.save->metadata().boxesInitialized);
    assert(r.save->trainer().trainerId==0x1234);
    assert(r.save->trainer().money==123456);
    assert(r.save->party().size()==2);
    assert(r.save->party()[0].species==25);
    assert(r.save->party()[0].level==15);
    assert(r.save->party()[0].dvs[1]==10);
    assert(r.save->party()[0].statExperience[0]==1234);
    assert(r.save->party()[0].pp[0]==20 && r.save->party()[0].ppUps[0]==3);
    assert(r.save->party()[1].species==112);
    assert(r.save->boxes().size()==l.boxCount);
    assert(r.save->boxes()[2].slots.size()==l.boxCapacity);
    assert(r.save->boxes()[2].slots[0] && r.save->boxes()[2].slots[0]->species==1);
    assert(r.save->boxes()[0].slots[0] && r.save->boxes()[0].slots[0]->species==4);
    assert(r.save->boxes().back().slots.back() && r.save->boxes().back().slots.back()->species==151);
    if (l.region==RegionLayout::International) {
        assert(r.save->trainer().name=="WILL");
        assert(r.save->party()[0].nickname=="SPARKY");
        assert(r.save->party()[0].originalTrainer=="RED");
    } else {
        assert(!r.save->trainer().name.empty());
        assert(!r.save->party()[0].nickname.empty());
    }
    oracleCheckPKSM(f,l);
}
}

int main() {
    assert(gen1InternalToNational(0x01)==112);
    assert(gen1InternalToNational(0x54)==25);
    assert(gen1InternalToNational(0x99)==1);
    assert(gen1InternalToNational(0x15)==151);

    assertCore(SourceGame::Red,INTL);
    assertCore(SourceGame::Blue,INTL);
    assertCore(SourceGame::Yellow,INTL);
    assertCore(SourceGame::Red,JPN);
    assertCore(SourceGame::Yellow,JPN);

    auto rb=makeFixture(SourceGame::Red,INTL,true);
    assert(parse(rb,SourceGame::Blue));

    auto yellow=makeFixture(SourceGame::Yellow,INTL,true);
    auto mismatch=parse(yellow,SourceGame::Red);
    assert(!mismatch && mismatch.error==SaveError::GameHintMismatch);
    auto red=makeFixture(SourceGame::Red,INTL,true);
    mismatch=parse(red,SourceGame::Yellow);
    assert(!mismatch && mismatch.error==SaveError::GameHintMismatch);

    auto checksumBad=red;
    checksumBad[0x2608]^=0x01;
    auto bad=parse(checksumBad,SourceGame::Red);
    assert(!bad && bad.error==SaveError::ChecksumMismatch);

    auto boxChecksumBad=red;
    boxChecksumBad[0x4000+0x30]^=0x80;
    bad=parse(boxChecksumBad,SourceGame::Red);
    assert(!bad && bad.error==SaveError::ChecksumMismatch);

    auto early=makeFixture(SourceGame::Red,INTL,false);
    auto earlyBefore=early;
    auto earlyResult=parse(early,SourceGame::Red);
    assert(earlyResult);
    assert(early==earlyBefore);
    assert(!earlyResult.save->metadata().boxesInitialized);
    assert(!earlyResult.save->boxes()[0].slots[0]);
    assert(earlyResult.save->boxes()[2].slots[0] && earlyResult.save->boxes()[2].slots[0]->species==1);

    auto badParty=red;
    badParty[INTL.party]=7;
    badParty[INTL.checksum]=diff8(std::span<const uint8_t>(badParty).subspan(0x2598,INTL.mainLength));
    bad=parse(badParty,SourceGame::Red);
    assert(!bad);

    auto badMon=red;
    const size_t partyBody=INTL.party+8;
    badMon[partyBody]=0x02;
    badMon[INTL.checksum]=diff8(std::span<const uint8_t>(badMon).subspan(0x2598,INTL.mainLength));
    bad=parse(badMon,SourceGame::Red);
    assert(!bad && bad.error==SaveError::InvalidParty);

    auto invalidSpecies=red;
    invalidSpecies[INTL.party+1]=0x1F;
    invalidSpecies[partyBody]=0x1F;
    invalidSpecies[INTL.checksum]=diff8(std::span<const uint8_t>(invalidSpecies).subspan(0x2598,INTL.mainLength));
    bad=parse(invalidSpecies,SourceGame::Red);
    assert(!bad);

    auto badMoney=red;
    badMoney[INTL.money]=0xFA;
    badMoney[INTL.checksum]=diff8(std::span<const uint8_t>(badMoney).subspan(0x2598,INTL.mainLength));
    bad=parse(badMoney,SourceGame::Red);
    assert(!bad && bad.error==SaveError::InvalidTrainerData);
    bad=parse(std::span<const uint8_t>(red.data(),red.size()-1),SourceGame::Red);
    assert(!bad && bad.error==SaveError::WrongSize);

    std::cout << "Gen I RBY strict read-only adapter/oracle tests passed\n";
    return 0;
}
