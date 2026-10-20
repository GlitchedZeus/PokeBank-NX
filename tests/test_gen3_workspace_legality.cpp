#include "Legality/Gen3WorkspaceAdapter.h"
#include "Names/ItemNames.h"
#include "Names/SpeciesNames.h"
#include "fixtures/gen3_berry_save.h"
#include <cassert>
#include <cstring>
#include <iostream>
namespace Trainer {
const char* getSpeciesName(uint16_t s){return Names::getSpeciesName(s);}
const char* getItemName(uint16_t s){return Names::getItemName(s);}
}
namespace G=PokeVault::Integration::Gen3;
using namespace Gen3BerryFixture;
bool berryIssue(const Legality::Report& report) {
    return std::any_of(report.issues.begin(),report.issues.end(),[](const auto& i){return i.text.find("Enigma Berry:")!=std::string::npos;});
}
int main() {
    const std::array games{G::SourceGame::RubyGBA,G::SourceGame::SapphireGBA,G::SourceGame::EmeraldGBA,G::SourceGame::FireRedGBA,G::SourceGame::LeafGreenGBA};
    size_t checks=0;
    for(auto game:games)for(int rotation=0;rotation<14;++rotation)for(int state:{0,1,2,3}) {
        const bool absent=state==1,malformed=state==2,japanese=state==3;
        const auto id=G::sourceGameId(game);
        auto source=makeSource(id,rotation,absent,malformed,japanese);
        const auto phys=[&](size_t logical){return ((logical+rotation)%14)*0x1000;};
        source[phys(0)+6]=japanese?0:0xFF; // explicit international save-name shape
        // Emerald's zero money is encrypted with its source security key.
        if(game==G::SourceGame::EmeraldGBA)put32(source,phys(1)+0x490,0x12345678);
        std::array<uint8_t,80> blank{};
        Pokemon::Pokemon3FRLG pk(std::as_bytes(std::span(blank)));
        pk.setPID(0x12345678);pk.setSpecies(1);pk.setLanguage(2);
        pk.setOriginGame(2);pk.setOTName(u"RED");pk.setNickname(u"BULBASAUR");
        pk.setLevel(5);pk.setMetLevel(5);pk.setBall(4);pk.setMove(0,33);pk.setMovePP(0,35);
        pk.setHeldItem(175);pk.refreshChecksum();
        auto* encrypted=Encryption::encryptArray3FRLG(pk.getData());
        std::memcpy(source.data()+phys(5)+4,encrypted,80);
        const auto logical=[&](size_t offset){return phys(1+offset/D::kSectorDataSize)+offset%D::kSectorDataSize;};
        source[logical(D::partyCountOffset(game))]=1;
        for(size_t i=0;i<80;++i)source[logical(D::partyDataOffset(game)+i)]=static_cast<uint8_t>(encrypted[i]);
        source[logical(D::partyDataOffset(game)+84)]=5;
        delete[] encrypted;
        for(size_t sector=0;sector<14;++sector)put16(source,phys(sector)+0xFF6,D::sectorChecksum(std::span<const uint8_t>(source).subspan(phys(sector),D::kChunkLengths[sector])));
        const auto before=source;
        auto parsed=G::parse(source,game);assert(parsed);
        assert(parsed.save->trainer().language==(japanese?1:2));
        const G::PokemonLocation slot{G::PokemonLocation::Kind::Box,0,0};
        const auto direct=Legality::Gen3App::analyzeSave(*parsed.save,slot);assert(direct);
        if(malformed)assert(std::any_of(direct->report.issues.begin(),direct->report.issues.end(),[](const auto& issue){return issue.severity==Legality::Severity::Warning&&issue.text.find("berry data fails its native integrity")!=std::string::npos;}));
        assert(direct->sourceGame==game&&direct->activeSlot==0&&direct->saveCounter==11);
        assert(berryIssue(direct->report)==(absent||malformed));
        assert(!Legality::Gen3App::analyzeSave(*parsed.save,{G::PokemonLocation::Kind::Box,14,0}));
        assert(!Legality::Gen3App::analyzeSave(*parsed.save,{G::PokemonLocation::Kind::Box,0,1}));
        const auto party=Legality::Gen3App::analyzeSave(*parsed.save,{G::PokemonLocation::Kind::Party,0,0});
        assert(party&&party->location.kind==G::PokemonLocation::Kind::Party);
        assert(berryIssue(party->report)==(absent||malformed));
        assert(!Legality::Gen3App::analyzeSave(*parsed.save,{G::PokemonLocation::Kind::Party,1,0}));
        std::string error;
        auto workspace=G::StagedPokemonEditor::create(source,game,error);
        if(!workspace)std::cerr<<id<<": "<<error<<'\n';
        assert(workspace);
        const auto staged=Legality::Gen3App::analyzeWorkspace(*workspace,slot);assert(staged);
        assert(berryIssue(staged->report)==(absent||malformed));
        G::BoxPokemonEdit edit;edit.heldItem=0;
        assert(workspace->stageBoxPokemonEdit(0,0,edit,error));
        const auto edited=Legality::Gen3App::analyzeWorkspace(*workspace,slot);assert(edited);
        assert(!berryIssue(edited->report));
        // Context never sticks to the record or leaks from another opened save.
        assert(berryIssue(Legality::analyze(pk,Enums::GameVersion::FRLG,id)));
        assert(source==before);++checks;
    }
    std::cout<<"Gen III actual validated/staged save -> selected PK3 -> owned berry context -> production report: "<<checks<<" title/rotation/presence/integrity/save-language cases PASS\n";
}
