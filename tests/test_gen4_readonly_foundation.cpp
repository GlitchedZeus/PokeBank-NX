#include "Encryption/Encryption4.h"
#include "Integration/Gen4/Gen4AssignedSource.h"
#include "Integration/Gen4/Gen4SourceDiscovery.h"
#include "Utils/SHA256.h"
#include "utils/crypto.hpp"
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <unistd.h>
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
    // Independent pinned PKSM-Core CRC oracle, not the implementation under test.
    const uint16_t crc=pksm::crypto::ccitt16(std::span<const uint8_t>(save).subspan(off,len-footer));
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

std::vector<std::byte> oracleEncrypt(std::vector<std::byte> d) {
    // PKSM-Core aa22d7a: PK4::encrypt, invoked without its heuristic isEncrypted check.
    auto* b = reinterpret_cast<uint8_t*>(d.data());
    uint16_t sum = 0;
    for (size_t i=8; i<0x88; i+=2) sum += static_cast<uint16_t>(b[i] | (b[i+1]<<8));
    wb16(d,6,sum);
    const uint32_t pid = b[0] | (uint32_t(b[1])<<8) | (uint32_t(b[2])<<16) | (uint32_t(b[3])<<24);
    const uint8_t shuffle = (pid>>13)&31;
    pksm::crypto::pkm::blockShuffle<0x20>(b+8,pksm::crypto::pkm::InvertedBlockPositions[shuffle]);
    pksm::crypto::pkm::crypt<0x80>(b+8,sum);
    if (d.size()==0xEC) pksm::crypto::pkm::crypt<0x64>(b+0x88,pid);
    return d;
}
std::string hex(std::span<const std::byte> bytes) {
    std::ostringstream out;
    for (auto b:bytes) out << std::hex << std::setw(2) << std::setfill('0') << unsigned(b);
    return out.str();
}
std::string digest(std::span<const uint8_t> bytes) {
    Utils::SHA256 hash;
    hash.update(bytes.data(),bytes.size());
    std::array<uint8_t,32> result{};
    hash.finalize(result.data());
    return hex(std::as_bytes(std::span(result)));
}
void testOracle() {
    for (size_t size : {size_t(0x88),size_t(0xEC)}) {
        for (uint32_t shuffle=0;shuffle<32;++shuffle) {
            std::vector<std::byte> d(size);
            for(size_t i=0;i<size;++i) d[i]=static_cast<std::byte>((i*71+shuffle*3)&255);
            wb32(d,0,(shuffle<<13)|0x523); wb16(d,4,0);
            const auto oracle=oracleEncrypt(d);
            assert(Encryption::encryptArray4(d)==oracle);
            wb16(d,6,Encryption::checksum4(d));
            assert(Encryption::decryptArray4(oracle)==d);
            assert(Encryption::encryptArray4(Encryption::decryptArray4(oracle))==oracle);
        }
        const auto blank=oracleEncrypt(std::vector<std::byte>(size));
        std::ifstream in(size==0x88 ? "tests/fixtures/gen4/blank-stored.hex" : "tests/fixtures/gen4/blank-party.hex");
        std::string pinned; in>>pinned;
        assert(in && pinned==hex(blank));
        assert(Encryption::blankRecord4(size)==blank);
    }
    for(size_t badSize : {size_t(0),size_t(1),size_t(0x87),size_t(0x89),size_t(0xEB),size_t(0xED)}) {
        std::vector<std::byte> bad(badSize);
        assert(Encryption::decryptArray4(bad).empty() && Encryption::encryptArray4(bad).empty());
        Pokemon::Pokemon4ReadOnly p(bad);
        assert(!p.valid() && !p.empty() && p.species()==0 && p.nickname().empty());
    }
    auto badSanity=Encryption::decryptArray4(makeEntity(false));wb16(badSanity,4,1);
    Pokemon::Pokemon4ReadOnly bad(oracleEncrypt(badSanity));
    assert(bad.checksumValid() && !bad.valid() && bad.species()==0 && !bad.empty());
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
    // Caller-provided stale checksum must be refreshed on encryption.
    auto stale = Encryption::decryptArray4(makeEntity(false));
    wb16(stale, 0x08, 133);
    Pokemon::Pokemon4ReadOnly refreshed(Encryption::encryptArray4(stale), Enums::GameVersion::DP);
    assert(refreshed.valid() && refreshed.species() == 133);
    auto partyRaw=makeEntity(true);
    Pokemon::Pokemon4ReadOnly p(partyRaw,Enums::GameVersion::DP);
    assert(p.valid() && p.isParty() && p.species()==25);
    assert(p.currentHP()==7 && p.partyStatus()==0x40 && p.partyLevel()==25);
    assert(p.battleStats()[2]==55);
    assert(p.nickname()==u"PIKA" && p.originalTrainerName()==u"ASH");
    assert(p.personal().hp==35);

    auto badRaw = partyRaw;
    badRaw[0x08] ^= std::byte{1}; // Corrupt ciphertext, not the encryptor's input checksum.
    Pokemon::Pokemon4ReadOnly invalid(badRaw,Enums::GameVersion::DP);
    assert(!invalid.checksumValid() && !invalid.valid());
    assert(!invalid.empty() && invalid.species() == 0 && invalid.nickname().empty());
    assert(invalid.currentHP() == 0 && invalid.heldItem() == 0);
    assert(std::equal(invalid.originalEncryptedBytes().begin(), invalid.originalEncryptedBytes().end(), badRaw.begin()));

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
        assert(parsed->party().size()==1 && parsed->partyCount()==1);
        assert(parsed->nativePartySlots().size()==6);
        const auto inactive = parsed->nativePartySlots()[5].originalEncryptedBytes();
        assert(inactive.size()==0xEC);
        for (size_t i=0;i<inactive.size();++i)
            assert(static_cast<uint8_t>(inactive[i]) == save[spec(layout).party+5*0xEC+i]);
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
void testEveryLayoutMatrix() {
    for(Layout layout:{Layout::DiamondPearl,Layout::Platinum,Layout::HeartGoldSoulSilver}) {
        const auto sp=spec(layout);
        for (int general=0;general<2;++general) for(int storage=0;storage<2;++storage) {
            auto bytes=makeSave(layout,general,storage);
            const auto before=digest(bytes);
            auto parsed=Gen4ReadOnlySave::parse(bytes,layout);
            assert(parsed && parsed->generalSelection().partition==general && parsed->storageSelection().partition==storage);
            assert(!parsed->recovered() && digest(parsed->sourceBytes())==before && digest(bytes)==before);
            // Last slot of last box proves packed DP/Pt vs HGSS padded geometry.
            auto last=makeEntity(false,0x67890123);
            const size_t off=storage*PARTITION+sp.storageStart+sp.boxData+17*sp.boxStride+29*0x88;
            copy(bytes,off,last); restampCounter(bytes,layout,true,storage,50,0);
            parsed=Gen4ReadOnlySave::parse(bytes,layout);
            assert(parsed && parsed->box(17,29).pid()==0x67890123);
            assert(hex(parsed->box(17,29).originalEncryptedBytes())==hex(last));
            assert(!parsed->box(18,0).valid() && !parsed->box(0,30).valid());
        }
        for(bool storage:{false,true}) {
            const size_t offset=storage?sp.storageStart:0;
            const size_t len=storage?sp.storageSize:sp.generalSize;
            auto selection=[&](const Gen4ReadOnlySave& p){return storage?p.storageSelection():p.generalSelection();};
            for(int newest:{0,1}) for(int damage:{0,1,2}) {
                auto bytes=makeSave(layout,newest,newest);
                const size_t base=newest*PARTITION+offset;
                if(damage==0) bytes[base+1]^=1;
                if(damage==1) w32(bytes,base+len-12,1);
                if(damage==2) w32(bytes,base+len-8,0xBAD);
                const auto before=digest(bytes);
                auto parsed=Gen4ReadOnlySave::parse(bytes,layout);
                assert(parsed && selection(*parsed).partition==1-newest && selection(*parsed).recoveredOlderCopy);
                assert(digest(bytes)==before && digest(parsed->sourceBytes())==before);
                bytes[(1-newest)*PARTITION+offset+1]^=1;
                const auto failedHash=digest(bytes);
                assert(!Gen4ReadOnlySave::parse(bytes,layout));
                assert(digest(bytes)==failedHash);
            }
            auto olderBad=makeSave(layout,1,1);olderBad[offset+1]^=1;
            auto parsed=Gen4ReadOnlySave::parse(olderBad,layout);
            assert(parsed && !selection(*parsed).recoveredOlderCopy && selection(*parsed).partition==1);
            // Exact PKHeX CompareCounters edge behavior, applied to General and Storage.
            struct Counters {uint32_t a,b;int expected;};
            for(auto c:{Counters{100,101,1},Counters{101,100,0},Counters{0xFFFFFFFF,5,1},
                        Counters{5,0xFFFFFFFF,0},Counters{0xFFFFFFFE,0xFFFFFFFF,1},
                        Counters{0xFFFFFFFF,0xFFFFFFFE,0},Counters{0xFFFFFFFF,0xFFFFFFFF,1}}) {
                auto bytes=makeSave(layout);
                restampCounter(bytes,layout,storage,0,c.a,1);
                restampCounter(bytes,layout,storage,1,c.b,1);
                parsed=Gen4ReadOnlySave::parse(bytes,layout);
                assert(parsed && selection(*parsed).partition==c.expected);
                restampCounter(bytes,layout,storage,0,25,c.a);
                restampCounter(bytes,layout,storage,1,25,c.b);
                parsed=Gen4ReadOnlySave::parse(bytes,layout);
                assert(parsed && selection(*parsed).partition==c.expected);
            }
            auto tied=makeSave(layout);
            for(int i:{0,1})restampCounter(tied,layout,storage,i,123,456);
            parsed=Gen4ReadOnlySave::parse(tied,layout);
            assert(parsed && selection(*parsed).partition==0);
        }
        for(uint8_t lang:{1,2,3,4,5,7,8}) {
            auto bytes=makeSave(layout,0,1,7,lang);
            const std::u16string name=lang==1?u"ヒカリ":lang==8?u"빛나":u"ÉLISE";
            copy(bytes,sp.trainer,Utils::encodeGen4Field(name,8,7,lang));
            restampCounter(bytes,layout,false,0,100,0);
            auto parsed=Gen4ReadOnlySave::parse(bytes,layout);
            assert(parsed && parsed->trainer().language==lang && parsed->trainer().name==name);
        }
        for (bool count : {false,true}) {
            auto bytes=makeSave(layout);
            if(count)bytes[sp.party-4]=7;else bytes[sp.storageStart+sp.currentBox]=18;
            restampCounter(bytes,layout,!count,0,100,0);
            auto hash=digest(bytes);
            assert(!Gen4ReadOnlySave::parse(bytes,layout) && digest(bytes)==hash);
        }
        auto bytes=makeSave(layout);
        for(auto wrong:{Layout::DiamondPearl,Layout::Platinum,Layout::HeartGoldSoulSilver})
            if(wrong!=layout)assert(!Gen4ReadOnlySave::parse(bytes,wrong));
        for(size_t size : {size_t(0),size_t(1),size_t(0x7FFFF),size_t(0x80001)}) {
            auto malformed=bytes;malformed.resize(size);
            const auto before=digest(malformed);
            assert(!Gen4ReadOnlySave::parse(malformed,layout) && digest(malformed)==before);
        }
    }
}

void testFieldFidelity() {
    auto d=Encryption::decryptArray4(makeEntity(true));
    wb32(d,0x24,0x12345678);wb32(d,0x3C,0x87654321);wb32(d,0x60,0xAABBCCDD);
    wb32(d,0x38,0xC000001F); d[0x16]=std::byte{0x2A};d[0x40]=std::byte{0x1B};
    // Native trash and reserved bytes, including after both text terminators.
    d[0x5D]=std::byte{0xAD};d[0x77]=std::byte{0xBE};d[0x64]=std::byte{0xCA};
    for(uint16_t hp:{0,1,7}) {
        wb16(d,0x8E,hp);
        auto raw=oracleEncrypt(d);Pokemon::Pokemon4ReadOnly p(raw,Enums::GameVersion::PT);
        assert(p.valid() && p.currentHP()==hp && p.partyStatus()==0x40 && p.maxHP()==60);
        assert(p.isEgg() && p.isNicknamed() && p.fatefulEncounter() && p.gender()==1 && p.form()==3);
        assert(p.markings()==0x2A && p.ribbons()[2]==0xAABBCCDD);
        assert(p.tid()==12345 && p.sid()==54321 && p.experience()==10000 && p.friendship()==70);
        assert(p.ability()==9 && p.language()==2 && p.evs()[5]==6 && p.ivs()[0]==31);
        assert(p.moves()[0]==85 && p.pp()[1]==30 && p.ppUps()[2]==3);
        assert(p.eggLocationExtended()==3001 && p.metLocationExtended()==2001);
        assert(p.eggLocationDP()==1 && p.metLocationDP()==2 && p.originVersion()==10);
        assert(p.ballDPPt()==4 && p.ballHGSS()==17 && p.pokerusState()==0x21 && p.metLevel()==25);
        assert(p.ballCapsuleIndex()==2 && p.battleStats()[0]==35);
        assert(Encryption::encryptArray4(p.decryptedBytes())==raw);
    }
    // PID-correlated identity is observed without rerolling either PID or IDs.
    for(uint32_t pid:{0u,1u,0x12345678u}) {
        wb32(d,0,pid);wb16(d,0x0C,0);wb16(d,0x0E,0);
        const auto raw=oracleEncrypt(d);Pokemon::Pokemon4ReadOnly p(raw,Enums::GameVersion::DP);
        assert(p.valid() && p.pid()==pid && p.tid()==0 && p.sid()==0);
        assert(Encryption::encryptArray4(p.decryptedBytes())==raw);
    }
    // Current container group, not origin Version, selects form/base-stat policy.
    for (auto species : {386,413,479,487,492}) {
        wb16(d,0x08,species);d[0x40]=std::byte{8};
        auto raw=oracleEncrypt(d);
        Pokemon::Pokemon4ReadOnly dp(raw,Enums::GameVersion::DP),pt(raw,Enums::GameVersion::PT),hg(raw,Enums::GameVersion::HGSS);
        assert(dp.personal().hp>0 && pt.personal().hp>0 && hg.personal().hp>0);
        if(species==479)assert(dp.personal().spa==95 && pt.personal().spa==105 && hg.personal().spa==105);
        if(species==487)assert(dp.personal().atk==100 && pt.personal().atk==120 && hg.personal().atk==120);
        if(species==492)assert(dp.personal().spe==100 && pt.personal().spe==127 && hg.personal().spe==127);
        if(species==386)assert(dp.personal().atk==180 && pt.personal().atk==180 && hg.personal().atk==180);
    }
    auto code=[](const std::vector<std::byte>& b,size_t i){return unsigned(b[i*2])|(unsigned(b[i*2+1])<<8);};
    auto jp=Utils::encodeGen4Field(u"ピ♂",8,7,1),ko=Utils::encodeGen4Field(u"피♂",8,7,8);
    assert(code(jp,1)==0xEE && code(ko,1)==0x1BB);
    assert(Utils::decodeGen4Field(jp)==u"ピ♂" && Utils::decodeGen4Field(ko)==u"피♂");
    assert(code(Utils::encodeGen4Field(u"A♂ピ",8,2,2),1)==0x1BB);
    assert(code(Utils::encodeGen4Field(u"’",8,7,2),0)==0x1B3);
    assert(code(Utils::encodeGen4Field(u"\u0378",8,7,2),0)==0x1AC);
}

void testAssignmentsOnDisk() {
    using namespace PokeVault::Integration::Gen4;
    namespace fs=std::filesystem;
    const auto root=fs::temp_directory_path()/("pokebank-g4-"+std::to_string(getpid()));
    fs::create_directories(root);
    const auto savePath=root/"assigned.sav";
    const auto otherPath=root/"another.sav";
    const auto database=root/"bindings.cfg";
    auto bytes=makeSave(Layout::DiamondPearl);
    auto write=[&](const fs::path& path,const auto& data){std::ofstream f(path,std::ios::binary);f.write(reinterpret_cast<const char*>(data.data()),data.size());assert(f);};
    write(savePath,bytes);write(otherPath,bytes);
    PokeVault::Legacy::LegacySourceBindings bindings(database.string());
    assert(bindings.load());
    assert(!bindings.assignFileAndSave("metadata-alias",{"user","diamond_nds",database.string()+".tmp","manual","DP"}));
    assert(openAssignedSource(bindings,"user","diamond_nds").status==OpenStatus::Unassigned);
    assert(bindings.assignFileAndSave("explicit-source",{"user","diamond_nds",savePath.string(),"manual","DP"}));
    PokeVault::Legacy::LegacySourceBindings restarted(database.string());assert(restarted.load());
    auto opened=openAssignedSource(restarted,"user","diamond_nds");
    assert(opened.status==OpenStatus::Ready && opened.save);
    assert(opened.save->assignedExactGame()==Enums::GameVersion::D && opened.save->exactGameFromSave()==Enums::GameVersion::Invalid);
    assert(digest(opened.save->sourceBytes())==digest(bytes));
    std::ifstream original(savePath,std::ios::binary);
    std::vector<uint8_t> stillOriginal((std::istreambuf_iterator<char>(original)),{});
    assert(digest(stillOriginal)==digest(bytes));
    original.close();
    fs::remove(savePath);
    assert(fs::exists(otherPath));
    assert(openAssignedSource(restarted,"user","diamond_nds").status==OpenStatus::Missing);
    assert(openAssignedSource(restarted,"other-user","diamond_nds").status==OpenStatus::Unassigned);
    write(savePath,bytes);
    assert(restarted.assignFileAndSave("explicit-source",{"user","pearl_nds",savePath.string(),"melonDS","DP"}));
    opened=openAssignedSource(restarted,"user","pearl_nds");
    assert(opened.status==OpenStatus::Ready && opened.save->assignedExactGame()==Enums::GameVersion::P);
    assert(restarted.assignFileAndSave("explicit-source",{"user","platinum_nds",savePath.string(),"manual","PT"}));
    assert(openAssignedSource(restarted,"user","platinum_nds").status==OpenStatus::AssignmentMismatch);
    for(uint8_t rom:{7,8})for(const std::string game:{"heartgold_nds","soulsilver_nds"}) {
        write(savePath,makeSave(Layout::HeartGoldSoulSilver,0,0,rom));
        assert(restarted.assignFileAndSave("explicit-source",{"user",game,savePath.string(),"DraStic","HGSS"}));
        opened=openAssignedSource(restarted,"user",game);
        const bool matches=(rom==7)==(game=="heartgold_nds");
        assert(opened.status==(matches?OpenStatus::Ready:OpenStatus::AssignmentMismatch));
    }
    assert(restarted.assignFileAndSave("explicit-source",{"user","diamond_nds",savePath.string(),"manual","DP"}));
    assert(restarted.assignFileAndSave("second-source",{"user","diamond_nds",otherPath.string(),"manual","DP"}));
    assert(openAssignedSource(restarted,"user","diamond_nds").status==OpenStatus::Ambiguous);
    fs::remove_all(root);
}

void testSourceDiscovery() {
    using namespace PokeVault::Integration::Gen4;
    namespace fs=std::filesystem;
    const auto root=fs::temp_directory_path()/("pokebank-g4-discovery-"+std::to_string(getpid()));
    const auto retro=root/"retroarch";
    const auto drastic=root/"drastic";
    fs::create_directories(retro);
    fs::create_directories(drastic);
    auto write=[&](const fs::path& path,const auto& data){
        std::ofstream f(path,std::ios::binary);
        f.write(reinterpret_cast<const char*>(data.data()),static_cast<std::streamsize>(data.size()));
        assert(f);
    };
    const auto dp=makeSave(Layout::DiamondPearl);
    const auto pt=makeSave(Layout::Platinum);
    const auto hg=makeSave(Layout::HeartGoldSoulSilver,0,0,7);
    write(retro/"Diamond.srm",dp);
    write(retro/"Platinum.sav",pt);
    write(drastic/"HeartGold.dsv",hg);

    auto wrapped=pt;
    const std::string marker="|-DESMUME SAVE-|";
    wrapped.insert(wrapped.end(),marker.begin(),marker.end());
    write(retro/"Wrapped.dsv",wrapped);

    const std::array<DiscoveryRoot,2> roots{{
        {retro.string(),"RetroArch",1},
        {drastic.string(),"DraStic",1},
    }};
    const auto beforeDp=digest(dp),beforePt=digest(pt),beforeHg=digest(hg);
    auto found=discoverSources(roots,{64});
    assert(!found.limitReached && found.filesExamined==4);
    size_t ready=0,wrappedCount=0;
    bool sawDP=false,sawPT=false,sawHG=false;
    for(const auto& candidate:found.candidates) {
        if(candidate.ready()) {
            ++ready;
            if(candidate.layout==Layout::DiamondPearl) {
                sawDP=true;
                assert(candidateMatchesGame(candidate,"diamond_nds"));
                assert(candidateMatchesGame(candidate,"pearl_nds"));
                assert(!candidateMatchesGame(candidate,"platinum_nds"));
            }
            if(candidate.layout==Layout::Platinum) {
                sawPT=true;
                assert(candidateMatchesGame(candidate,"platinum_nds"));
            }
            if(candidate.layout==Layout::HeartGoldSoulSilver) {
                sawHG=true;
                assert(candidate.exactGameFromSave==Enums::GameVersion::HG);
                assert(candidateMatchesGame(candidate,"heartgold_nds"));
                assert(!candidateMatchesGame(candidate,"soulsilver_nds"));
            }
        } else if(candidate.status==CandidateStatus::UnsupportedWrapper) {
            ++wrappedCount;
            assert(candidate.diagnostic.find("does not trim")!=std::string::npos);
        }
    }
    assert(ready==3 && wrappedCount==1 && sawDP && sawPT && sawHG);

    auto manual=inspectSourceFile((retro/"Diamond.srm").string(),"Manual","diamond_nds");
    assert(manual.ready() && manual.expectedRawFamily=="DP");
    auto mismatch=inspectSourceFile((retro/"Platinum.sav").string(),"Manual","diamond_nds");
    assert(mismatch.status==CandidateStatus::AssignmentMismatch);

    auto read=[&](const fs::path& path){
        std::ifstream f(path,std::ios::binary);
        return std::vector<uint8_t>((std::istreambuf_iterator<char>(f)),{});
    };
    assert(digest(read(retro/"Diamond.srm"))==beforeDp);
    assert(digest(read(retro/"Platinum.sav"))==beforePt);
    assert(digest(read(drastic/"HeartGold.dsv"))==beforeHg);
    assert(inspectSourceFile((retro/"Diamond.srm").string(),"Manual").sourceIdentity==
           inspectSourceFile((retro/"Diamond.srm").string(),"RetroArch").sourceIdentity);

    fs::remove_all(root);
}

}

int main(int argc,char** argv) {
    if(argc==2 && std::string_view(argv[1])=="--generate-oracles") {
        std::filesystem::create_directories("tests/fixtures/gen4");
        for(size_t size:{size_t(0x88),size_t(0xEC)}) {
            const auto blank=oracleEncrypt(std::vector<std::byte>(size));
            std::ofstream out(size==0x88?"tests/fixtures/gen4/blank-stored.hex":"tests/fixtures/gen4/blank-party.hex");
            out<<hex(blank)<<"\n";
        }
        return 0;
    }
    testFieldFidelity();
    testOracle();
    testEveryLayoutMatrix();
    testAssignmentsOnDisk();
    testSourceDiscovery();
    testCryptoAndEntity();
    testText();
    testLayoutsAndAssignments();
    testSelectionAndDamage();
    std::cout << "G4-02 strict Gen IV foundation + bounded source discovery PASS\n";
}
