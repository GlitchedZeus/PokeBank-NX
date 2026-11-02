#include "fuzz_synthetic_saves.h"
#include "Integration/Common/NativeBadges.h"
#include "Integration/Gen1/Gen1ReadOnlySave.h"
#include "Integration/Gen2/Gen2ReadOnlySave.h"
#include "Integration/Gen4/Gen4ReadOnlySave.h"
#include <cassert>
#include <iostream>

int main() {
    namespace G1=PokeVault::Integration::Gen1;namespace G2=PokeVault::Integration::Gen2;
    namespace G4=PokeVault::Integration::Gen4;
    using PokeVault::Integration::Common::NativeBadges;
    // Independent pinned PKHeX SAV1Offsets / SAV2Offsets bytes; checksums are
    // forged by the existing synthetic-save builders before the real parsers.
    auto red=qasynth::gen1::save();red[0x2602]=0xa5;qasynth::gen1::forge(red);
    const auto redBefore=red;
    const auto r=G1::parse(red,G1::SourceGame::Red);assert(r && r.save->trainer().badges==0xa5);
    assert((NativeBadges{r.save->trainer().badges,8}.text()=="4 / 8"));
    assert(red==redBefore);
    for(bool crystal:{false,true}) {
        const auto l=crystal?qasynth::gen2::IC:qasynth::gen2::IGS;
        auto bytes=qasynth::gen2::save(l);
        const auto offset=crystal?0x23e5:0x23e4;
        bytes[offset]=0xff;bytes[offset+1]=0xff;qasynth::gen2::checksum(bytes,l);
        const auto before=bytes;
        const auto parsed=G2::parse(bytes,crystal?G2::SourceGame::Crystal:G2::SourceGame::Gold);
        assert(parsed && parsed.save->trainer().badges==0xffff);
        assert((NativeBadges{parsed.save->trainer().badges,16}.count()==16));
        assert((NativeBadges{parsed.save->trainer().badges,16}.text()=="Johto 8/8 / Kanto 8/8"));
        assert(bytes==before);
    }
    namespace F=qasynth::gen4;
    for(auto kind:{F::Kind::DiamondPearl,F::Kind::Platinum,F::Kind::HeartGoldSoulSilver}) {
        const bool hgss=kind==F::Kind::HeartGoldSoulSilver;
        const auto sp=F::spec(kind);
        auto bytes=F::save(kind,hgss?7:kind==F::Kind::Platinum?12:10);
        for(size_t base:{size_t{0},size_t{0x40000}}) {
            bytes[base+sp.trainer+0x1a]=0xff;
            bytes[base+sp.trainer+0x1f]=hgss?0xff:0;
        }
        F::forge(bytes);const auto before=bytes;
        const auto layout=hgss?G4::Layout::HeartGoldSoulSilver:
            kind==F::Kind::Platinum?G4::Layout::Platinum:G4::Layout::DiamondPearl;
        std::string error;const auto parsed=G4::Gen4ReadOnlySave::parse(bytes,layout,{},&error);
        assert(parsed);
        const auto& t=parsed->trainer();
        assert(t.badges==0xff && t.kantoBadges==(hgss?0xff:0));
        const NativeBadges badges{static_cast<uint16_t>(t.badges|(uint16_t(t.kantoBadges)<<8)),
            static_cast<uint8_t>(hgss?16:8)};
        assert(badges.count()==(hgss?16u:8u));assert(bytes==before);
    }
    std::cout<<"Native Trainer badges: Gen I eight, Gen II sixteen, D/P/Pt eight, HGSS Johto+Kanto sixteen; source bytes unchanged PASS\n";
}
