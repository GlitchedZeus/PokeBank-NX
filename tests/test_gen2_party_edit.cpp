#include "fixtures/gsc_pokemon_fixture.h"
#include "Integration/Gen2/Gen2BattleStats.h"

int main() {
    for(const auto game:{SourceGame::Gold,SourceGame::Silver,SourceGame::Crystal}) {
        const auto& layout=game==SourceGame::Crystal?C:GS;
        auto source=fixture(layout,true);
        const auto body=layout.party+8;
        source[body+32]=8;source[body+33]=0xA5; // status + native unused byte
        const auto stats=calculateBattleStats(25,15,{7,10,10,10},{}).asArray();
        be16(source,body+34,stats[0]-7);
        for(size_t i=0;i<stats.size();++i)be16(source,body+36+i*2,stats[i]);
        checksum(source,layout);const auto immutable=source;
        auto parsed=parse(source,game);assert(parsed);
        std::string error;auto editor=StagedEditor::create(*parsed.save,error);assert(editor);
        const auto before=editor->partyPokemon(0,error);assert(before);
        BoxPokemonEdit nick;nick.nickname="PARTY";
        assert(editor->stagePartyPokemonEdit(0,nick,error));
        auto after=editor->partyPokemon(0,error);assert(after && after->rawBody==before->rawBody);
        BoxPokemonEdit edit;edit.level=50;edit.dvs=std::array<uint8_t,4>{1,2,3,4};
        edit.moves=std::array<uint8_t,4>{85,0,0,0};edit.heldItem=0;
        assert(editor->stagePartyPokemonEdit(0,edit,error));
        after=editor->partyPokemon(0,error);assert(after && after->level==50 && after->moves[0]==85);
        assert(after->status==8 && after->rawBody[33]==0xA5 && after->currentHP==before->currentHP);
        const auto expected=calculateBattleStats(25,50,{1,2,3,4},{}).asArray();
        assert((std::array<uint16_t,6>{after->maxHP,after->attack,after->defense,after->speed,
            after->specialAttack,after->specialDefense})==expected);
        bool moveNamed=false;for(const auto& change:editor->pendingChanges())
            if(change.label=="Party 1 Move 1") {assert(change.afterValue=="Thunderbolt");moveNamed=true;}
        assert(moveNamed);
        const auto candidate=editor->finalizedBytes(error);assert(!candidate.empty());
        auto reload=parse(candidate,game);assert(reload && reload.save->party()[0].rawBody==after->rawBody);
        for(size_t i=0x4000;i<0x7E00;++i)assert(candidate[i]==source[i]); // all PC records untouched
        assert(std::equal(source.begin()+0x8000,source.end(),candidate.begin()+0x8000)); // RTC exact
        const auto staged=std::vector<uint8_t>(editor->stagedBytes().begin(),editor->stagedBytes().end());
        edit.level=0;assert(!editor->stagePartyPokemonEdit(0,edit,error));
        assert(!editor->stagePartyPokemonEdit(1,{},error));
        assert(std::equal(staged.begin(),staged.end(),editor->stagedBytes().begin()));
        assert(source==immutable && std::equal(source.begin(),source.end(),editor->originalBytes().begin()));
        editor->discard();assert(!editor->hasPendingChanges());
        assert(std::equal(source.begin(),source.end(),editor->stagedBytes().begin()));
    }
    std::cout<<"Gen II native Party editing: G/S/C caches, damage/status, named Review, RTC, rejection and immutable export PASS\n";
}
