#include "fixtures/gsc_pokemon_fixture.h"
#include "UI/Gen2PokemonSession.h"
#include "UI/PokemonViewActions.h"
#include "Integration/Gen2/Gen2BattleStats.h"
#include "UI/BattleStatRadarModel.h"
namespace Rules = PokeBank::UIModel::Gen2PokemonEditor;
using Rules::Session;
using Rules::SessionMode;

void runSession(const L& layout,SourceGame game) {
    const auto raw=fixture(layout,true);
    auto parsed=parse(raw,game);assert(parsed);
    std::string error;auto editor=StagedEditor::create(*parsed.save,error);assert(editor);
    BoxPokemonEdit level5;level5.level=5;
    assert(editor->stageBoxPokemonEdit(0,0,level5,error));
    auto entry=*editor->boxedPokemon(0,0,error);
    Session session;session.begin(entry,SessionMode::Edit);
    assert(session.setLevel(20));
    assert(session.working.level==20 && session.working.experience==8000);
    auto edit=session.editRequest();assert(edit.level==20 && !edit.experience);
    assert(!session.back() && session.confirmExit);
    assert(session.keep(*editor,0,0,error));
    auto kept=*editor->boxedPokemon(0,0,error);assert(kept.level==20 && kept.experience==8000);

    session.begin(kept,SessionMode::Edit);
    assert(session.setExperience(15630)); // level 25 plus progress within that level
    assert(session.working.level==25 && session.working.experience==15630);
    assert(session.experienceToNext()==Pokemon::getExpForLevel(26,0)-15630);
    edit=session.editRequest();assert(!edit.level && edit.experience==15630);
    const auto preview=calculateBattleStats(25,session.working.level,Rules::storedDVs(session.working),session.working.statExperience).asArray();
    assert(session.keep(*editor,0,0,error));
    kept=*editor->boxedPokemon(0,0,error);assert(kept.level==25&&kept.experience==15630);
    assert(preview==calculateBattleStats(kept.species,kept.level,Rules::storedDVs(kept),kept.statExperience).asArray());

    // Last explicit progression action wins, even if both values differ from baseline.
    session.begin(kept,SessionMode::Edit);assert(session.setLevel(20));assert(session.setExperience(27001));
    assert(!session.editRequest().level && session.editRequest().experience==27001);
    assert(session.setLevel(40));assert(session.editRequest().level==40&&!session.editRequest().experience);
    session.setSpecies(1);assert(session.working.experience==Pokemon::getExpForLevel(40,3));
    assert(!session.setExperience(session.maximumExperience()+1));

    // Staged work at entry is the baseline. Continue keeps the local edits; Discard
    // never calls StagedEditor and preserves prior staged changes byte-for-byte.
    const std::vector<uint8_t> prior(editor->stagedBytes().begin(),editor->stagedBytes().end());
    session.begin(kept,SessionMode::Edit);assert(session.back()&&session.mode==SessionMode::None);
    session.begin(kept,SessionMode::Edit);session.working.nickname="LOCAL";
    assert(!session.back()&&session.confirmExit);session.continueEditing();
    assert(!session.confirmExit&&session.working.nickname=="LOCAL");
    assert(!session.back());session.discard();assert(session.working.nickname==kept.nickname);
    assert(std::equal(prior.begin(),prior.end(),editor->stagedBytes().begin()));

    session.begin(kept,SessionMode::Create);session.working.moves={};session.working.pp={};session.working.ppUps={};
    session.working.heldItem=0;session.working.caughtData=0;
    assert(session.setExperience(9267));assert(session.working.level==21);
    const auto visible=session.working;
    auto create=session.createRequest();assert(create.experience==9267);
    size_t slot=0;assert(session.add(*editor,2,slot,error));
    auto added=*editor->boxedPokemon(2,slot,error);
    assert(added.level==visible.level&&added.experience==visible.experience&&added.moves==visible.moves&&added.pp==visible.pp);
    assert(added.nickname==visible.nickname&&added.statExperience==visible.statExperience);
    auto finalized=editor->finalizedBytes(error);assert(!finalized.empty());auto reparsed=parse(finalized,game);assert(reparsed);
    assert(reparsed.save->boxes()[2].slots[slot]->experience==9267);

    // A new incompatible move is blocked before staged bytes change. Existing
    // unusual moves remain preservable during unrelated edits.
    uint8_t incompatible=1;
    while(incompatible<251&&MoveCompatibility::canLearnMove(game,25,incompatible))++incompatible;
    session.begin(kept,SessionMode::Create);session.working.moves={incompatible,0,0,0};
    const auto preInvalid=editor->finalizedBytes(error);
    assert(!session.add(*editor,3,slot,error));assert(editor->finalizedBytes(error)==preInvalid);
    session.begin(kept,SessionMode::Edit);session.working.nickname="KEPT";
    assert(session.keep(*editor,0,0,error));

    // Passive sessions reject edit setters, Keep and Add, even if A/Y/etc is
    // accidentally routed here. Back is the only exit effect.
    session.begin(kept,SessionMode::View);
    assert(!session.setLevel(90)&&!session.setExperience(1000)&&!session.setSpecies(6));
    assert(!session.keep(*editor,0,0,error)&&!session.add(*editor,3,slot,error));
    assert(session.back());
    assert(std::equal(raw.begin(),raw.end(),editor->originalBytes().begin()));
    assert(std::equal(raw.begin(),raw.end(),parsed.save->sourceBytes().begin()));
}
int main(){
 using namespace PokeBank::UIModel;
 assert(Rules::slotActionCount(true)==7 && Rules::slotActionCount(false)==3);
 const std::array expected{Rules::SlotAction::View,Rules::SlotAction::Edit,Rules::SlotAction::Clone,
  Rules::SlotAction::Shiny,Rules::SlotAction::Add,Rules::SlotAction::Review,Rules::SlotAction::Close};
 for(size_t i=0;i<expected.size();++i) assert(Rules::slotActionAt(true,i)==expected[i]);
 assert(Rules::slotActionAt(false,0)==Rules::SlotAction::Add);
 assert(Rules::slotActionAt(true,7)==Rules::SlotAction::None);
 assert(passiveViewAction(false)==PassiveViewAction::None);
 assert(passiveViewAction(true)==PassiveViewAction::Back);
 assert(passiveViewAction(false,passiveBackTarget)==PassiveViewAction::Back);
 assert(passiveViewAction(false,42)==PassiveViewAction::None);
runSession(GS,SourceGame::Gold);runSession(GS,SourceGame::Silver);runSession(C,SourceGame::Crystal);
 std::cout<<"GSC UI session: canonical Level/EXP Keep, Create serialization, preview, transaction and passive mutation gates PASS\n";}
