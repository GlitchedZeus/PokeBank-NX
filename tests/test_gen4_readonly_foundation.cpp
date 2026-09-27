#include "Encryption/Encryption4.h"
#include "Enums/LanguageID.h"
#include "Integration/Gen4/Gen4ReadOnlySave.h"
#include "Pokemon/Pokemon4ReadOnly.h"
#include "Utils/CRC16.h"
#include "Utils/Gen4TextCodec.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

using PokeVault::Integration::Gen4::AssignmentStatus;
using PokeVault::Integration::Gen4::Gen4ReadOnlySave;
using PokeVault::Integration::Gen4::Layout;

namespace {
constexpr size_t SAVE_SIZE = 0x80000;
constexpr size_t PARTITION = 0x40000;
constexpr uint32_t MAGIC_INTL = 0x20060623;
constexpr uint32_t MAGIC_KOR = 0x20070903;

struct Spec {
    size_t generalSize, storageStart, storageSize, footerSize;
    size_t trainer, party, boxData, boxStride, currentBox, boxNames;
};
Spec spec(Layout layout) {
    if (layout == Layout::DiamondPearl)
        return {0xC100,0xC100,0x121E0,0x14,0x64,0x98,4,0xFF0,0,4 + 18*0xFF0};
    if (layout == Layout::Platinum)
        return {0xCF2C,0xCF2C,0x121E4,0x14,0x68,0xA0,4,0xFF0,0,4 + 18*0xFF0};
    return {0xF628,0xF700,0x12310,0x10,0x64,0x98,0,0x1000,0x12000,0x12008};
}

void w16(std::vector<uint8_t>& b, size_t o, uint16_t v) {
    b[o]=static_cast<uint8_t>(v); b[o+1]=static_cast<uint8_t>(v>>8);
}
void w32(std::vector<uint8_t>& b, size_t o, uint32_t v) {
    b[o]=static_cast<uint8_t>(v); b[o+1]=static_cast<uint8_t>(v>>8);
    b[o+2]=static_cast<uint8_t>(v>>16); b[o+3]=static_cast<uint8_t>(v>>24);
}
void wb16(std::vector<std::byte>& b, size_t o, uint16_t v) {
    b[o]=static_cast<std::byte>(v); b[o+1]=static_cast<std::byte>(v>>8);
}
void wb32(std::vector<std::byte>& b, size_t o, uint32_t v) {
    b[o]=static_cast<std::byte>(v); b[o+1]=static_cast<std::byte>(v>>8);
    b[o+2]=static_cast<std::byte>(v>>16); b[o+3]=static_cast<std::byte>(v>>24);
}
void copy(std::vector<uint8_t>& dst, size_t off, std::span<const std::byte> src) {
    for(size_t i=0;i<src.size();++i) dst[off+i]=static_cast<uint8_t>(src[i]);
}
void copy(std::vector<std::byte>& dst, size_t off, std::span<const std::byte> src) {
    std::copy(src.begin(), src.end(), dst.begin()+static_cast<std::ptrdiff_t>(off));
}

std::vector<std::byte> makeEntity(bool party, uint32_t pid=0x12345678u,
                                  uint8_t language=static_cast<uint8_t>(Enums::LanguageID::English)) {
    std::vector<std::byte> d(party ? Encryption::SIZE_PARTY4 : Encryption::SIZE_STORED4, std::byte{0});
    wb32(d,0,pid);
    wb16(d,0x08,25); // Pikachu
    wb16(d,0x0A,1);
    wb16(d,0x0C,12345);
    wb16(d,0x0E,54321);
    wb32(d,0x10,10000);
    d[0x14]=std::byte{70};
    d[0x15]=std::byte{9}; // Static
    d[0x17]=static_cast<std::byte>(language);
    d[0x18]=std::byte{1}; d[0x19]=std::byte{2}; d[0x1A]=std::byte{3};
    d[0x1B]=std::byte{4}; d[0x1C]=std::byte{5}; d[0x1D]=std::byte{6};
    wb16(d,0x28,85); wb16(d,0x2A,98); wb16(d,0x2C,86); wb16(d,0x2E,104);
    d[0x30]=std::byte{15}; d[0x31]=std::byte{30}; d[0x32]=std::byte{20}; d[0x33]=std::byte{15};
    d[0x34]=std::byte{1}; d[0x35]=std::byte{2}; d[0x36]=std::byte{3};
    wb32(d,0x38,31u | (30u<<5) | (29u<<10) | (28u<<15) | (27u<<20) | (26u<<25));
    d[0x40]=std::byte{1}; // fateful
    wb16(d,0x44,3001); wb16(d,0x46,2001);
    auto nick=Utils::encodeGen4Field(u"PIKA",11,10,language);
    copy(d,0x48,nick);
    d[0x5F]=std::byte{10};
    auto ot=Utils::encodeGen4Field(u"ASH",8,7,language);
    copy(d,0x68,ot);
    wb16(d,0x7E,1); wb16(d,0x80,2);
    d[0x82]=std::byte{0x21}; d[0x83]=std::byte{4}; d[0x84]=std::byte{25}; d[0x86]=std::byte{17};
    if(party) {
        wb32(d,0x88,0x40);
        d[0x8C]=std::byte{25}; d[0x8D]=std::byte{2};
        wb16(d,0x8E,7); wb16(d,0x90,60);
        wb16(d,0x92,35); wb16(d,0x94,30); wb16(d,0x96,55); wb16(d,0x98,40); wb16(d,0x9A,40);
        for(size_t i=0x9C;i<d.size();++i) d[i]=static_cast<std::byte>((i*7)&0xFF);
    }
    wb16(d,0x06,Encryption::checksum4(d));
    return Encryption::encryptArray4(d);
}

void stamp(std::vector<uint8_t>& save,size_t off,size_t len,size_t footer,
           uint32_t major,uint32_t minor,uint32_t magic) {
    const size_t end=off+len;
    w32(save,end-0x14,major); w32(save,end-0x10,minor);
    w32(save,end-0x0C,static_cast<uint32_t>(len)); w32(save,end-0x08,magic);
    const uint16_t crc=Utils::crc16ccitt(save.data()+off,len-footer);
    w16(save,end-2,crc);
}

void fillBlockPair(std::vector<uint8_t>& save, Layout layout, uint8_t rom,
                   uint8_t language, int newerGeneral, int newerStorage) {
    const Spec s=spec(layout);
    const auto blank=Encryption::blankRecord4(Encryption::SIZE_STORED4);
    const auto party=makeEntity(true,0x12345678u,language);
    const auto stored=makeEntity(false,0x12345678u,language);
    const uint32_t magic=language==static_cast<uint8_t>(Enums::LanguageID::Korean)?MAGIC_KOR:MAGIC_INTL;

    for(int p=0;p<2;++p) {
        const size_t base=static_cast<size_t>(p)*PARTITION;
        std::fill(save.begin()+base,save.begin()+base+s.generalSize,0);
        const size_t st=base+s.storageStart;
        std::fill(save.begin()+st,save.begin()+st+s.storageSize,0);

        const size_t tr=base+s.trainer;
        auto name=Utils::encodeGen4Field(u"ASH",8,7,language);
        copy(save,tr,name);
        w16(save,tr+0x10,12345); w16(save,tr+0x12,54321); w32(save,tr+0x14,999999);
        save[tr+0x18]=1; save[tr+0x19]=language; save[tr+0x1A]=8; save[tr+0x1C]=rom;
        w16(save,tr+0x22,321); save[tr+0x24]=45; save[tr+0x25]=12;

        save[base+s.party-4]=1;
        copy(save,base+s.party,party);

        for(size_t box=0;box<18;++box) {
            const size_t boxBase=st+s.boxData+box*s.boxStride;
            for(size_t slot=0;slot<30;++slot)
                copy(save,boxBase+slot*Encryption::SIZE_STORED4,blank);
            if(layout==Layout::HeartGoldSoulSilver)
                std::fill(save.begin()+boxBase+0xFF0,save.begin()+boxBase+0x1000,
                          static_cast<uint8_t>(0xA0+box));
        }
        copy(save,st+s.boxData,stored);
        save[st+s.currentBox]=3;
        for(size_t box=0;box<18;++box) {
            auto bn=Utils::encodeGen4Field(u"BOX",20,8,language);
            copy(save,st+s.boxNames+box*40,bn);
        }
        stamp(save,base,s.generalSize,s.footerSize,
              p==newerGeneral?20:10,1,magic);
        stamp(save,st,s.storageSize,s.footerSize,
              p==newerStorage?30:15,2,magic);
    }
}

std::vector<uint8_t> makeSave(Layout layout,int newerGeneral=0,int newerStorage=0,
                              uint8_t rom=7,uint8_t language=2) {
    std::vector<uint8_t> save(SAVE_SIZE,0xFF);
    fillBlockPair(save,layout,rom,language,newerGeneral,newerStorage);
    return save;
}

void restampCounter(std::vector<uint8_t>& save, Layout layout, bool storage, int partition,
                    uint32_t major,uint32_t minor) {
    const Spec s=spec(layout);
    const size_t off=static_cast<size_t>(partition)*PARTITION+(storage?s.storageStart:0);
    const size_t len=storage?s.storageSize:s.generalSize;
    const size_t end=off+len;
    const uint32_t magic=(static_cast<uint32_t>(save[end-8]) |
        (static_cast<uint32_t>(save[end-7])<<8) |
        (static_cast<uint32_t>(save[end-6])<<16) |
        (static_cast<uint32_t>(save[end-5])<<24));
    stamp(save,off,len,s.footerSize,major,minor,magic);
}

void testCryptoAndEntity() {
    for(uint32_t shuffle=0;shuffle<32;++shuffle) {
        auto d=std::vector<std::byte>(Encryption::SIZE_STORED4,std::byte{0});
        wb32(d,0,(shuffle<<13)|0x123u);
        wb16(d,0x08,25);
        wb16(d,0x06,Encryption::checksum4(d));
        auto e=Encryption::encryptArray4(d);
        assert(e.size()==d.size());
        auto round=Encryption::decryptArray4(e);
        assert(round==d);
        assert(Encryption::encryptArray4(round)==e);
    }
    auto partyRaw=makeEntity(true);
    Pokemon::Pokemon4ReadOnly p(partyRaw,Enums::GameVersion::DP);
    assert(p.valid() && p.isParty() && p.species()==25);
    assert(p.currentHP()==7 && p.partyStatus()==0x40 && p.partyLevel()==25);
    assert(p.battleStats()[2]==55);
    assert(p.nickname()==u"PIKA" && p.originalTrainerName()==u"ASH");
    assert(p.personal().hp==35);

    auto bad=Encryption::decryptArray4(partyRaw);
    wb16(bad,0x06,static_cast<uint16_t>(Encryption::checksum4(bad)+1));
    auto badRaw=Encryption::encryptArray4(bad);
    Pokemon::Pokemon4ReadOnly invalid(badRaw,Enums::GameVersion::DP);
    assert(!invalid.checksumValid() && !invalid.valid());

    for(size_t size:{Encryption::SIZE_STORED4,Encryption::SIZE_PARTY4}) {
        auto blank=Encryption::blankRecord4(size);
        assert(std::any_of(blank.begin(),blank.end(),[](std::byte b){return b!=std::byte{0};}));
        Pokemon::Pokemon4ReadOnly empty(blank,Enums::GameVersion::DP);
        assert(empty.valid() && empty.empty());
        assert(Encryption::encryptArray4(empty.decryptedBytes())==blank);
    }
}

void testText() {
    for(const auto& sample: {std::u16string(u"ABC♂"),std::u16string(u"ピカチュウ"),std::u16string(u"피카츄")}) {
        const uint8_t lang=sample==u"피카츄" ? static_cast<uint8_t>(Enums::LanguageID::Korean)
                                             : static_cast<uint8_t>(Enums::LanguageID::Japanese);
        auto bytes=Utils::encodeGen4Field(sample,12,10,lang);
        assert(Utils::decodeGen4Field(bytes)==sample);
    }
    std::vector<std::byte> zeroTerm(8,std::byte{0});
    const uint16_t a=Utils::encodeGen4CodePoint(u'A',true);
    zeroTerm[0]=static_cast<std::byte>(a); zeroTerm[1]=static_cast<std::byte>(a>>8);
    assert(Utils::decodeGen4Field(zeroTerm)==u"A");
    std::vector<std::byte> unmapped={std::byte{0x00},std::byte{0x03},std::byte{0xFF},std::byte{0xFF}};
    assert(Utils::decodeGen4Field(unmapped).empty());
}

void testLayoutsAndAssignments() {
    for(Layout layout:{Layout::DiamondPearl,Layout::Platinum,Layout::HeartGoldSoulSilver}) {
        const uint8_t rom=layout==Layout::HeartGoldSoulSilver?7:0;
        auto save=makeSave(layout,0,1,rom,2);
        const auto before=save;
        std::string error;
        auto parsed=Gen4ReadOnlySave::parse(save,layout,{},&error);
        assert(parsed && error.empty() && save==before);
        assert(parsed->generalSelection().partition==0);
        assert(parsed->storageSelection().partition==1);
        assert(parsed->party().size()==1);
        assert(parsed->party()[0].currentHP()==7);
        assert(parsed->box(0,0).species()==25);
        assert(parsed->currentBox()==3);
        assert(parsed->boxNames().size()==18 && parsed->boxNames()[0]==u"BOX");
        assert(parsed->trainer().tid==12345 && parsed->trainer().sid==54321);
        assert(parsed->trainer().language==2);
        if(layout==Layout::HeartGoldSoulSilver) {
            assert(parsed->exactGameFromSave()==Enums::GameVersion::HG);
            const auto pad=parsed->hgssBoxPadding(0);
            assert(pad.size()==0x10 && pad[0]==0xA0);
        }
    }

    auto dp=makeSave(Layout::DiamondPearl);
    auto d=Gen4ReadOnlySave::parse(dp,Layout::DiamondPearl,"diamond_nds");
    auto p=Gen4ReadOnlySave::parse(dp,Layout::DiamondPearl,"pearl_nds");
    assert(d && p);
    assert(d->rawFamily()==Enums::GameVersion::DP && d->exactGameFromSave()==Enums::GameVersion::Invalid);
    assert(d->assignedExactGame()==Enums::GameVersion::D && d->assignmentStatus()==AssignmentStatus::Match);
    assert(p->assignedExactGame()==Enums::GameVersion::P && p->assignmentStatus()==AssignmentStatus::Match);
    assert(!Gen4ReadOnlySave::parse(dp,Layout::Platinum));

    auto pt=makeSave(Layout::Platinum);
    auto wrongPt=Gen4ReadOnlySave::parse(pt,Layout::Platinum,"diamond_nds");
    assert(wrongPt && wrongPt->assignmentStatus()==AssignmentStatus::Mismatch);

    auto hg=makeSave(Layout::HeartGoldSoulSilver,0,0,7);
    auto ss=makeSave(Layout::HeartGoldSoulSilver,0,0,8);
    auto hgOk=Gen4ReadOnlySave::parse(hg,Layout::HeartGoldSoulSilver,"heartgold_nds");
    auto hgBad=Gen4ReadOnlySave::parse(hg,Layout::HeartGoldSoulSilver,"soulsilver_nds");
    auto ssOk=Gen4ReadOnlySave::parse(ss,Layout::HeartGoldSoulSilver,"soulsilver_nds");
    assert(hgOk && hgOk->assignmentStatus()==AssignmentStatus::Match);
    assert(hgBad && hgBad->assignmentStatus()==AssignmentStatus::Mismatch);
    assert(ssOk && ssOk->exactGameFromSave()==Enums::GameVersion::SS &&
           ssOk->assignmentStatus()==AssignmentStatus::Match);

    auto kor=makeSave(Layout::Platinum,0,0,0,static_cast<uint8_t>(Enums::LanguageID::Korean));
    auto korParsed=Gen4ReadOnlySave::parse(kor,Layout::Platinum);
    assert(korParsed && korParsed->trainer().language==static_cast<uint8_t>(Enums::LanguageID::Korean));
}

void testSelectionAndDamage() {
    auto save=makeSave(Layout::DiamondPearl,0,0);
    const Spec s=spec(Layout::DiamondPearl);

    // Equal major: minor is the tiebreak.
    restampCounter(save,Layout::DiamondPearl,false,0,100,1);
    restampCounter(save,Layout::DiamondPearl,false,1,100,2);
    auto minor=Gen4ReadOnlySave::parse(save,Layout::DiamondPearl);
    assert(minor && minor->generalSelection().partition==1);

    // 0xFFFFFFFF is normally never-written, not newest.
    restampCounter(save,Layout::DiamondPearl,false,0,0xFFFFFFFFu,0xFFFFFFFFu);
    restampCounter(save,Layout::DiamondPearl,false,1,5,1);
    auto uninit=Gen4ReadOnlySave::parse(save,Layout::DiamondPearl);
    assert(uninit && uninit->generalSelection().partition==1);

    // Rollover edge: FFFFFFFF after FFFFFFFE is treated as newer.
    restampCounter(save,Layout::DiamondPearl,false,0,0xFFFFFFFEu,1);
    restampCounter(save,Layout::DiamondPearl,false,1,0xFFFFFFFFu,1);
    auto rollover=Gen4ReadOnlySave::parse(save,Layout::DiamondPearl);
    assert(rollover && rollover->generalSelection().partition==1);

    // Newest CRC-bad -> explicit older-copy recovery.
    auto recovery=makeSave(Layout::DiamondPearl,0,0);
    recovery[1]^=0x5A;
    auto rec=Gen4ReadOnlySave::parse(recovery,Layout::DiamondPearl);
    assert(rec && rec->generalSelection().partition==1 && rec->generalSelection().recoveredOlderCopy);

    auto storageRecovery=makeSave(Layout::DiamondPearl,0,0);
    storageRecovery[s.storageStart+1]^=0x33;
    auto sr=Gen4ReadOnlySave::parse(storageRecovery,Layout::DiamondPearl);
    assert(sr && sr->storageSelection().partition==1 && sr->storageSelection().recoveredOlderCopy);

    auto bothBad=makeSave(Layout::DiamondPearl);
    bothBad[1]^=1; bothBad[PARTITION+1]^=1;
    assert(!Gen4ReadOnlySave::parse(bothBad,Layout::DiamondPearl));

    auto bothStorageBad=makeSave(Layout::DiamondPearl);
    bothStorageBad[s.storageStart+1]^=1; bothStorageBad[PARTITION+s.storageStart+1]^=1;
    assert(!Gen4ReadOnlySave::parse(bothStorageBad,Layout::DiamondPearl));

    auto badSize=makeSave(Layout::DiamondPearl);
    for(int part=0;part<2;++part) {
        const size_t end=static_cast<size_t>(part)*PARTITION+s.generalSize;
        w32(badSize,end-0x0C,0x1234);
    }
    assert(!Gen4ReadOnlySave::parse(badSize,Layout::DiamondPearl));

    auto badMagic=makeSave(Layout::DiamondPearl);
    for(int part=0;part<2;++part) {
        const size_t end=static_cast<size_t>(part)*PARTITION+s.generalSize;
        w32(badMagic,end-0x08,0xDEADBEEF);
    }
    assert(!Gen4ReadOnlySave::parse(badMagic,Layout::DiamondPearl));

    auto shortSave=makeSave(Layout::DiamondPearl); shortSave.pop_back();
    assert(!Gen4ReadOnlySave::parse(shortSave,Layout::DiamondPearl));
    auto longSave=makeSave(Layout::DiamondPearl); longSave.push_back(0);
    assert(!Gen4ReadOnlySave::parse(longSave,Layout::DiamondPearl));

    // Invalid HGSS ROMCode fails closed even when the layout/CRCs are otherwise good.
    auto badRom=makeSave(Layout::HeartGoldSoulSilver,0,0,9);
    assert(!Gen4ReadOnlySave::parse(badRom,Layout::HeartGoldSoulSilver));
}
}

int main() {
    testCryptoAndEntity();
    testText();
    testLayoutsAndAssignments();
    testSelectionAndDamage();
    std::cout << "G4-01 shared PK4 + strict read-only DP/Pt/HGSS foundation PASS\n";
}
