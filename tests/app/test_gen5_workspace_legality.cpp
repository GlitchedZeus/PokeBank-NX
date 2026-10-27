// Instantiated against the exact read-only #307 snapshot, never its active tree.
// Synthetic native saves are reproduction witnesses, not retail specimens.
#include "Integration/Gen5/Gen5StagedPokemonWorkspace.h"
#include "Legality/Gen5WorkspaceAdapter.h"
#include "Legality/ReportPresentation.h"
#include "legality_gen5_test_helpers.h"
#include <cassert>
#include <iostream>
namespace G=PokeVault::Integration::Gen5;
namespace C=G::Crypto;
namespace L=G::LayoutInfo;
namespace A=Legality::Gen5App;
using W=G::Gen5StagedPokemonWorkspace;
using V=Legality::Verdict;
using F=G::StagedPokemon5Record::Field;

void refresh(std::vector<uint8_t>& save,G::SaveFamily family) {
    const auto blocks=family==G::SaveFamily::BlackWhite?std::span<const L::SaveBlock>(L::BlackWhite)
        :std::span<const L::SaveBlock>(L::Black2White2);
    for(const auto& b:blocks) {
        const auto crc=Utils::crc16ccitt(save.data()+b.offset,b.length);
        C::write16(save,b.checksumOffset,crc);C::write16(save,b.mirrorOffset,crc);
    }
}
int main() {
    size_t checks=0;
    const auto pk=Gen5TestHelpers::fromHex("EB147618000094C101000000602A92F800000000784100020000000000000000000000000000000021000000000000002300000000000000FFD5F63F000000000004000000000000420075006C00620061007300610075007200FFFF000000150000000000000000540065007300740065007200FFFF00001A0A131A0A1362EA4000000401000000");
    const W::Slot box{W::Region::Box,0,0},party{W::Region::Party,0,0};
    for(const char* game:{"black_nds","white_nds","black2_nds","white2_nds"}) {
        const auto family=std::string_view(game)=="black_nds"||std::string_view(game)=="white_nds"
            ?G::SaveFamily::BlackWhite:G::SaveFamily::Black2White2;
        std::vector<uint8_t> save(L::FullSaveSize,0xFF);
        const auto size=family==G::SaveFamily::BlackWhite?L::BlackWhiteCopySize:L::Black2White2CopySize;
        std::fill_n(save.begin(),size,0);
        save[0x19400+0x1F]=A::versionFromSourceGameId(game);
        // Source-backed PlayerData5 geometry, same OT/TID/SID as this record.
        std::copy_n(pk.begin()+0x68,16,save.begin()+0x19404);
        std::copy_n(pk.begin()+0x0C,4,save.begin()+0x19414);
        save[0x1941E]=pk[0x17];save[0x19421]=pk[0x84]>>7;
        const auto encrypted=C::encryptCandidate(pk);
        std::copy(encrypted.begin(),encrypted.end(),save.begin()+L::BoxOffset);
        auto nativeParty=pk;nativeParty.resize(220);nativeParty[0x8C]=1;
        --nativeParty[0x30]; // Used PP is legal in party, invalid in a box.
        const auto encryptedParty=C::encryptCandidate(nativeParty);
        save[L::PartyOffset+4]=1;
        std::copy(encryptedParty.begin(),encryptedParty.end(),save.begin()+L::PartyOffset+8);
        refresh(save,family);
        const auto original=save;
        const auto parsed=G::Gen5ReadOnlySave::parse(save,game);assert(parsed);
        W workspace(*parsed);
        const auto before=A::analyzeWorkspace(workspace,box);assert(before);
        assert(before->knownGame&&!before->emptySlot&&before->verdict==V::NoProblemsFound);
        assert(before->source==A::RecordSource::OriginalSlot&&!before->stagedTrainer);
        assert(before->identity&&before->identity->pid==C::read32(pk,0));
        assert(before->identity->tid==C::read16(pk,0x0C)&&before->identity->sid==C::read16(pk,0x0E));
        const auto partyResult=A::analyzeWorkspace(workspace,party);assert(partyResult);
        assert(partyResult->verdict==V::NoProblemsFound&&partyResult->identity->party);
        checks+=2;
        std::string error;
        assert(workspace.stageBox(0,0,F::HeldItem,0,638,&error));
        const auto after=A::analyzeWorkspace(workspace,box);assert(after);
        assert(after->verdict==V::Invalid&&after->verdictLine.starts_with("Invalid:"));
        assert(after->source==A::RecordSource::StagedSlot&&after->identity);
        assert(after->identity->checksum!=before->identity->checksum);
        assert(after->identity->pid==before->identity->pid);
        assert(Gen5TestHelpers::hasText(after->report,Legality::Severity::Invalid,"held"));
        assert(A::analyzeWorkspace(workspace,party)->verdict==V::NoProblemsFound);
        checks+=2;
        workspace.discardAll();
        assert(A::analyzeWorkspace(workspace,box)->verdict==V::NoProblemsFound);
        auto* trainer=workspace.stagedTrainer(&error);assert(trainer);
        assert(trainer->setName("Auditor",&error));
        const auto renamed=A::analyzeWorkspace(workspace,box);assert(renamed);
        assert(renamed->verdict==V::Incomplete&&renamed->stagedTrainer);
        assert(renamed->source==A::RecordSource::OriginalSlot&&renamed->identity==before->identity);
        assert(Gen5TestHelpers::hasText(renamed->report,Legality::Severity::Warning,"suspicious provenance"));
        assert(workspace.stageBoxName(0,0,false,"Auditor",&error));
        const auto aligned=A::analyzeWorkspace(workspace,box);assert(aligned);
        assert(aligned->verdict==V::NoProblemsFound&&aligned->stagedTrainer);
        assert(aligned->source==A::RecordSource::StagedSlot);
        assert(aligned->identity->tid==before->identity->tid&&aligned->identity->sid==before->identity->sid);
        assert(A::analyzeWorkspace(workspace,party)->verdict==V::Incomplete);
        checks+=4;
        for(const W::Slot invalid: {W::Slot{W::Region::Party,0,1},W::Slot{W::Region::Party,1,0},
            W::Slot{W::Region::Box,24,0},W::Slot{W::Region::Box,0,30},W::Slot{static_cast<W::Region>(255),0,0}}) {
            assert(!A::analyzeWorkspace(workspace,invalid));++checks;
        }
        assert(save==original&&std::equal(parsed->sourceBytes().begin(),parsed->sourceBytes().end(),original.begin()));
        // Valid block CRCs do not authenticate malformed trainer context.
        // The bridge must not turn byte2 into a valid female or fall back
        // to the original trainer when the current name cannot be decoded.
        for(unsigned fault=0;fault<5;++fault) {
            auto ambiguous=original;
            if(fault==0)ambiguous[0x19421]=2;
            if(fault==1)ambiguous[0x1941E]=0;
            if(fault==2)ambiguous[0x1941E]=6;
            if(fault==3)std::fill_n(ambiguous.begin()+0x19404,16,0x58);
            if(fault==4)std::fill_n(ambiguous.begin()+0x19404,16,0);
            refresh(ambiguous,family);
            const auto unverified=G::Gen5ReadOnlySave::parse(ambiguous,game);assert(unverified);
            const W uncertain(*unverified);
            const auto report=A::analyzeWorkspace(uncertain,box);assert(report);
            assert(report->verdict==V::Incomplete&&!report->emptySlot);
            for(const auto& line:report->lines())assert(line.find("No problems found")==std::string::npos);
            assert(Gen5TestHelpers::hasText(report->report,Legality::Severity::Warning,"trainer context"));
            ++checks;
        }
        for(const char* other:{"black_nds","white_nds","black2_nds","white2_nds"})
            if(std::string_view(other)!=game){assert(!G::Gen5ReadOnlySave::parse(original,other));++checks;}
    }
    std::cout<<"Actual #307 workspace: "<<checks<<" cases across four titles; native party/box, staged Pokemon/trainer, "
        "identity, hostile contexts, canonical bounds and original-save immutability PASS\n";
}
