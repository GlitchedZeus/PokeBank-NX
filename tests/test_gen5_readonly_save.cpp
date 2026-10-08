#include "Integration/Gen5/Gen5ReadOnlySave.h"
#include "Integration/Gen5/Gen5SaveInstanceAdapter.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <tuple>
#include <string>
#include <vector>

namespace G = PokeVault::Integration::Gen5;
namespace C = PokeVault::Integration::Gen5::Crypto;
namespace L = PokeVault::Integration::Gen5::LayoutInfo;

void write32(std::vector<uint8_t>& dst,size_t off,uint32_t value) {
    C::write16(dst,off,static_cast<uint16_t>(value));
    C::write16(dst,off+2,static_cast<uint16_t>(value>>16));
}
template<size_t N>
void stamp(std::vector<uint8_t>& sav,size_t base,
           const std::array<L::SaveBlock,N>& blocks) {
    const size_t copySize = N == 70 ? L::BlackWhiteCopySize : L::Black2White2CopySize;
    auto partition = std::span<uint8_t>(sav).subspan(base,copySize);
    for(const auto& b : blocks) {
        const uint16_t crc=Utils::crc16ccitt(partition.data()+b.offset,b.length);
        C::write16(partition,b.checksumOffset,crc);
        C::write16(partition,b.mirrorOffset,crc);
    }
}
std::vector<uint8_t> make(G::SaveFamily family,uint8_t version,size_t base=0) {
    std::vector<uint8_t> sav(L::FullSaveSize,0);
    auto tr=base+0x19400;
    sav[tr+4]='N'; sav[tr+6]='X';
    C::write16(sav,tr+0x14,12345);
    C::write16(sav,tr+0x16,54321);
    sav[tr+0x1E]=2;
    sav[tr+0x1F]=version;
    sav[tr+0x21]=1;
    C::write16(sav,tr+0x24,23);
    sav[tr+0x26]=11;
    // Party Pikachu
    std::vector<uint8_t> pk(C::PartySize,0);
    write32(pk,0,0x12345678);
    C::write16(pk,8,25);
    C::write16(pk,12,12345);
    C::write16(pk,14,54321);
    const auto encrypted=C::encryptCandidate(pk);
    sav[base+L::PartyOffset+4]=1;
    std::copy(encrypted.begin(),encrypted.end(),
              sav.begin()+static_cast<std::ptrdiff_t>(base+L::PartyOffset+8));
    // Box Eevee
    std::vector<uint8_t> boxed(C::StoredSize,0);
    write32(boxed,0,0xABCDEF01);
    C::write16(boxed,8,133);
    const auto boxEncrypted=C::encryptCandidate(boxed);
    std::copy(boxEncrypted.begin(),boxEncrypted.end(),
              sav.begin()+static_cast<std::ptrdiff_t>(base+L::BoxOffset));
    const size_t dex=base+(family==G::SaveFamily::BlackWhite ? 0x21600 : 0x21400);
    sav[dex+8+3]|=1u; // species 25: bit 24
    sav[dex+8+0x54+3]|=1u;
    if(family==G::SaveFamily::BlackWhite)stamp(sav,base,L::BlackWhite);
    else stamp(sav,base,L::Black2White2);
    return sav;
}
int main() {
    for(const auto& [family,version,id] :
        {std::tuple{G::SaveFamily::BlackWhite,uint8_t{20},"white_nds"},
         std::tuple{G::SaveFamily::BlackWhite,uint8_t{21},"black_nds"},
         std::tuple{G::SaveFamily::Black2White2,uint8_t{22},"white2_nds"},
         std::tuple{G::SaveFamily::Black2White2,uint8_t{23},"black2_nds"}}) {
        const auto sav=make(family,version);
        std::string error;
        const auto parsed=G::Gen5ReadOnlySave::parse(sav,id,&error);
        assert(parsed && error.empty());
        assert(parsed->family()==family && parsed->exactGameId()==id);
        const G::SourceContext context{
            id, "retroarch", "RetroArch", "sdmc:/retroarch/cores/savefiles/test.srm",
            "sdmc:/retroarch/cores/savefiles/test.srm", "physical-save-fixture",
            "fixture-test", "test-profile", 17
        };
        auto probe = G::probeNormalizedBattery(sav,context);
        assert(probe.ready());
        assert(probe.instance.generation==5 && probe.instance.gameId==id);
        assert(probe.instance.providerId=="retroarch" && probe.instance.providerLabel=="RetroArch");
        assert(probe.instance.readOnly() && probe.instance.partyCount==1);
        assert(probe.instance.sourceIndex==17 && probe.instance.claimedProfile=="test-profile");
        assert(probe.instance.sourcePath=="sdmc:/retroarch/cores/savefiles/test.srm");
        assert(probe.instance.trainerName.empty()); // No invented Gen V text display.
        auto mismatched=context; mismatched.assignedExactGame = "platinum_nds";
        assert(!G::probeNormalizedBattery(sav,mismatched).ready());
        mismatched.assignedExactGame = id;
        mismatched.sourcePath = {};
        assert(!G::probeNormalizedBattery(sav,mismatched).ready());
        auto wrongAssignment=context;
        wrongAssignment.assignedExactGame = family==G::SaveFamily::BlackWhite ?
            "white2_nds" : "white_nds";
        const auto mismatchProbe=G::probeNormalizedBattery(sav,wrongAssignment);
        assert(!mismatchProbe.ready());
        assert(mismatchProbe.instance.validation==PokeVault::Source::ValidationStatus::AssignmentMismatch);
        assert(parsed->partyCount()==1 && parsed->selectedPartition()==0);
        assert(parsed->trainer().rawName==u"NX");
        assert(parsed->trainer().tid==12345 && parsed->trainer().sid==54321);
        assert(parsed->trainer().playedHours==23);
        assert(parsed->partyPokemon(0)->species()==25);
        assert(parsed->boxPokemon(0,0)->species()==133);
        assert(parsed->dexProgress().caught==1 && parsed->dexProgress().seen==1);
        assert(!parsed->partyPokemon(6) && !parsed->boxPokemon(24,0));
        assert(!G::Gen5ReadOnlySave::parse(sav,"wrong_nds",&error));
        assert(error.find("identity")!=std::string::npos);
        auto corrupt=sav; corrupt[L::BoxOffset+1]^=1;
        assert(!G::Gen5ReadOnlySave::parse(corrupt,id,&error));
        assert(error.find("checksum")!=std::string::npos);
        auto wrongFamily=make(family,static_cast<uint8_t>(
            family==G::SaveFamily::BlackWhite ? 22 : 20));
        assert(!G::Gen5ReadOnlySave::parse(wrongFamily,{},&error));
        assert(error.find("disagrees")!=std::string::npos);
        // Valid backup-only copy, primary is invalid.
        const size_t copySize = family == G::SaveFamily::BlackWhite ?
            L::BlackWhiteCopySize : L::Black2White2CopySize;
        const auto backup=make(family,version,copySize);
        const auto selected=G::Gen5ReadOnlySave::parse(backup,id,&error);
        assert(selected && selected->selectedBackupPartition());
        assert(selected->selectedCopyOffset()==copySize);
        auto backupProbe=G::probeNormalizedBattery(backup,context);
        assert(backupProbe.ready() && backupProbe.save->selectedBackupPartition());
        assert(!G::Gen5ReadOnlySave::parse(backup,id,&error,G::SaveCopySelection::Primary));
        assert(error.find("requested save copy")!=std::string::npos);
        // Two different valid copies must fail closed until recency is sourced.
        auto both=sav;
        auto different=backup;
        different[copySize+0x19400+6]='Y'; // valid but newer/older unknown
        if(family==G::SaveFamily::BlackWhite)stamp(different,copySize,L::BlackWhite);
        else stamp(different,copySize,L::Black2White2);
        std::copy(different.begin()+static_cast<std::ptrdiff_t>(copySize),
                  different.end(),both.begin()+static_cast<std::ptrdiff_t>(copySize));
        assert(!G::Gen5ReadOnlySave::parse(both,id,&error));
        assert(error.find("select one explicitly")!=std::string::npos);
        const auto ambiguousProbe=G::probeNormalizedBattery(both,context);
        assert(!ambiguousProbe.ready());
        assert(ambiguousProbe.instance.validation==PokeVault::Source::ValidationStatus::Unsupported);
        const auto primary = G::Gen5ReadOnlySave::parse(
            both,id,&error,G::SaveCopySelection::Primary);
        const auto secondary = G::Gen5ReadOnlySave::parse(
            both,id,&error,G::SaveCopySelection::Backup);
        assert(primary && secondary);
        assert(primary->selectedCopyOffset()==0 && secondary->selectedCopyOffset()==copySize);
        assert(primary->trainer().rawName==u"NX" && secondary->trainer().rawName==u"NY");
        // Identical valid copies are deterministic and harmless.
        std::copy(sav.begin(),sav.begin()+static_cast<std::ptrdiff_t>(copySize),
                  both.begin()+static_cast<std::ptrdiff_t>(copySize));
        assert(G::Gen5ReadOnlySave::parse(both,id,&error)->selectedPartition()==0);
        // Gen IV-sized 0x40000 "backup" must not be mistaken for a Gen V copy.
        auto wrongOffset=make(family,version,0x40000);
        assert(!G::Gen5ReadOnlySave::parse(wrongOffset,id,&error));
        // Exact title discrepancy between valid copies must fail closed.
        auto mismatch=make(family,static_cast<uint8_t>(version ^ 1),copySize);
        std::copy(mismatch.begin()+static_cast<std::ptrdiff_t>(copySize),
                  mismatch.begin()+static_cast<std::ptrdiff_t>(2*copySize),
                  both.begin()+static_cast<std::ptrdiff_t>(copySize));
        assert(!G::Gen5ReadOnlySave::parse(both,id,&error,G::SaveCopySelection::Primary));
        assert(error.find("disagree on exact game identity")!=std::string::npos);
    }
    std::vector<uint8_t> empty(L::FullSaveSize,0);
    assert(!G::Gen5ReadOnlySave::parse(empty));
    assert(!G::Gen5ReadOnlySave::parse(std::span<const uint8_t>(empty.data(),0x40000)));
    std::cout<<"Gen V strict BW/B2W2 save reader synthetic contracts PASS\n";
}
