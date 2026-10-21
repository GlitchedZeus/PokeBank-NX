// Compile against #307's exact workspace plus the reviewed legality sources.
// Synthetic native saves; never presented as authenticated retail specimens.
#include "Integration/Gen5/Gen5StagedPokemonWorkspace.h"
#include "Legality/Gen5WorkspaceAdapter.h"
#include "Legality/ReportPresentation.h"
#include "legality_gen5_test_helpers.h"
#include <cassert>
#include <iostream>
namespace G=PokeVault::Integration::Gen5;
namespace C=G::Crypto;
namespace L=G::LayoutInfo;
int main() {
    using W=G::Gen5StagedPokemonWorkspace;
    using F=G::StagedPokemon5Record::Field;
    size_t checks=0;
    for(const auto game:{"black_nds","white_nds","black2_nds","white2_nds"}) {
        const auto family=std::string_view(game)=="black_nds"||std::string_view(game)=="white_nds"
            ? G::SaveFamily::BlackWhite:G::SaveFamily::Black2White2;
        std::vector<uint8_t> save(L::FullSaveSize,0xFF);
        const auto blocks=family==G::SaveFamily::BlackWhite?std::span<const L::SaveBlock>(L::BlackWhite)
            :std::span<const L::SaveBlock>(L::Black2White2);
        const auto size=family==G::SaveFamily::BlackWhite?L::BlackWhiteCopySize:L::Black2White2CopySize;
        std::fill_n(save.begin(),size,0);
        save[0x19400+0x1F]=Legality::Gen5App::versionFromSourceGameId(game);
        // Fixture source: existing adapter regression's PKHeX-synthetic Black egg.
        auto pk=Gen5TestHelpers::fromHex("EB147618000094C101000000602A92F800000000784100020000000000000000000000000000000021000000000000002300000000000000FFD5F63F000000000004000000000000420075006C00620061007300610075007200FFFF000000150000000000000000540065007300740065007200FFFF00001A0A131A0A1362EA4000000401000000");
        const auto encrypted=C::encryptCandidate(pk);
        std::copy(encrypted.begin(),encrypted.end(),save.begin()+0x400);
        for(const auto& b:blocks) {
            const auto crc=Utils::crc16ccitt(save.data()+b.offset,b.length);
            C::write16(save,b.checksumOffset,crc);C::write16(save,b.mirrorOffset,crc);
        }
        const auto original=save;
        const auto parsed=G::Gen5ReadOnlySave::parse(save,game);assert(parsed);
        W workspace(*parsed);const W::Slot slot{W::Region::Box,0,0};
        const auto before=Legality::Gen5App::analyzeWorkspace(workspace,slot);assert(before);
        assert(before->knownGame && !before->emptySlot);
        assert(before->verdict==Legality::Verdict::NoProblemsFound);
        std::string error;
        assert(workspace.stageBox(0,0,F::HeldItem,0,638,&error)); // Key item cannot be held.
        assert(workspace.viewBox(0,0)->heldItem()==638);
        const auto after=Legality::Gen5App::analyzeWorkspace(workspace,slot);assert(after);
        assert(after->verdict==Legality::Verdict::Invalid);
        assert(after->verdictLine.starts_with("Invalid:"));
        assert(std::any_of(after->report.issues.begin(),after->report.issues.end(),[](const auto& i){return i.identifier==Legality::CheckIdentifier::Items && i.severity==Legality::Severity::Invalid;}));
        assert(!Legality::Gen5App::analyzeWorkspace(workspace,W::Slot{W::Region::Party,1,0}));
        assert(!Legality::Gen5App::analyzeWorkspace(workspace,W::Slot{W::Region::Box,24,0}));
        assert(save==original && std::equal(parsed->sourceBytes().begin(),parsed->sourceBytes().end(),original.begin()));
        assert(workspace.changedRecordCount()==1);
        ++checks;
    }
    std::cout<<"Actual #307 workspace -> production Gen V adapter: "<<checks<<" titles, staged Invalid, exact identity, bounds and source immutability PASS\n";
}
