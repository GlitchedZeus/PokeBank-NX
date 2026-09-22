#include "fixtures/gsc_pokemon_fixture.h"
#include "UI/Gen2PokemonSession.h"
#include "UI/Gen2PokemonPickerModel.h"
#include "UI/PokemonViewActions.h"
#include "Integration/Gen2/Gen2BattleStats.h"
#include "UI/BattleStatRadarModel.h"
#include "UI/SpeciesChangeLevelPolicy.h"
namespace Rules = PokeBank::UIModel::Gen2PokemonEditor;
namespace Picker = PokeBank::UIModel::Gen2PokemonPicker;
namespace Gen2Picker = PokeBank::UIModel::Gen2PokemonPicker;
using Rules::Session;
using Rules::SessionMode;

void runSession(const L& layout,SourceGame game) {
    const std::string_view sourceId = game == SourceGame::Gold ? "gold_gbc" :
                                      game == SourceGame::Silver ? "silver_gbc" : "crystal_gbc";
    const auto raw=fixture(layout,true);
    auto parsed=parse(raw,game);assert(parsed);
    std::string error;auto editor=StagedEditor::create(*parsed.save,error);assert(editor);
    BoxPokemonEdit level5;level5.level=5;
    assert(editor->stageBoxPokemonEdit(0,0,level5,error));
    auto entry=*editor->boxedPokemon(0,0,error);
    for (auto mode : {SessionMode::Create, SessionMode::Edit}) {
        Session names; names.begin(entry, mode);
        names.working.nickname = "Pikachu";
        assert(names.setSpecies(6, sourceId)); assert(names.working.nickname == "Charizard");
        names.working.nickname = "FlameBoy";
        assert(names.setSpecies(9, sourceId)); assert(names.working.nickname == "FlameBoy");
        names.working.nickname = "BLASTOISE";
        assert(names.setSpecies(25, sourceId)); assert(names.working.nickname == "PIKACHU");
    }
    if (game == SourceGame::Crystal) {
        Session caughtSession; caughtSession.begin(entry, SessionMode::Edit);
        caughtSession.working.moves = {}; caughtSession.working.pp = {}; caughtSession.working.ppUps = {};
        auto caught = PokeBank::UIModel::Gen2Native::decodeCrystalCaughtData(0);
        caught.timeOfDay = 3; caught.levelCode = 20; caught.location = 16; caught.originalTrainerFemale = true;
        caughtSession.working.caughtData = PokeBank::UIModel::Gen2Native::encodeCrystalCaughtData(caught);
        assert(caughtSession.keep(*editor, 0, 0, error));
        const auto saved = editor->boxedPokemon(0, 0, error);
        assert(saved && saved->caughtData == 0xD490);
        assert(std::equal(raw.begin(), raw.end(), editor->originalBytes().begin()));
        editor->discard();
        assert(editor->stageBoxPokemonEdit(0,0,level5,error));
    }
    for (auto mode : {SessionMode::Create, SessionMode::Edit}) {
        Session species; species.begin(entry, mode); assert(species.setLevel(50));
        species.working.moves = {}; species.working.pp = {}; species.working.ppUps = {};
        const auto stagedBefore = std::vector<uint8_t>(editor->stagedBytes().begin(), editor->stagedBytes().end());
        const auto expectedLevel =
            PokeBank::UIModel::SpeciesChangeLevelPolicy::defaultLevel(sourceId, 16);
        assert(species.setSpecies(16, sourceId));
        assert(species.working.level == expectedLevel);
        assert(species.working.level != 50);
        assert(expectedLevel == (game == SourceGame::Crystal ? 2 : 5));
        assert(species.working.experience == Pokemon::getExpForLevel(species.working.level, personalRecord(16)->experienceGrowth));
        assert(std::equal(stagedBefore.begin(), stagedBefore.end(), editor->stagedBytes().begin()));
        assert(std::equal(raw.begin(), raw.end(), editor->originalBytes().begin()));
        // Serialize the same visible result into a separate staged workspace.
        auto committed = StagedEditor::create(*parsed.save, error); assert(committed);
        size_t changedSlot = 0;
        if (mode == SessionMode::Create) assert(species.add(*committed, 2, changedSlot, error));
        else assert(species.keep(*committed, 0, 0, error));
        const auto result = committed->boxedPokemon(mode == SessionMode::Create ? 2 : 0, changedSlot, error);
        assert(result && result->species == 16 && result->level ==
               PokeBank::UIModel::SpeciesChangeLevelPolicy::defaultLevel(sourceId, 16));
        assert(result->experience == Pokemon::getExpForLevel(result->level, personalRecord(16)->experienceGrowth));
        assert(std::equal(raw.begin(), raw.end(), committed->originalBytes().begin()));
        // Discard a fresh local change, preserving the pre-existing staged bytes.
        species.begin(entry, mode); assert(species.setLevel(50)); assert(species.setSpecies(16, sourceId));
        species.discard();
        assert(std::equal(stagedBefore.begin(), stagedBefore.end(), editor->stagedBytes().begin()));
    }
    // Held-item browse/cancel and staged selection preserve source bytes for G/S/C.
    {
        Session held; held.begin(entry, SessionMode::Edit);
        const auto before = std::vector<uint8_t>(editor->stagedBytes().begin(), editor->stagedBytes().end());
        const auto choices = Rules::heldItemChoices();
        for (const auto item : choices) {
            held.working.heldItem = item;
            assert(std::equal(before.begin(), before.end(), editor->stagedBytes().begin()));
        }
        held.discard(); assert(held.working.heldItem == entry.heldItem);
        held.begin(entry, SessionMode::Edit); held.working.heldItem = 218;
        auto committed = StagedEditor::create(*parsed.save, error); assert(committed);
        assert(held.keep(*committed, 0, 0, error));
        assert(committed->boxedPokemon(0, 0, error)->heldItem == 218);
        assert(std::equal(raw.begin(), raw.end(), committed->originalBytes().begin()));
        assert(std::equal(before.begin(), before.end(), editor->stagedBytes().begin()));
    }
    Picker::Model locations;
    locations.openLocation("crystal_gbc", 25, 71, 25);
    assert(locations.locationCount() > 0);
    const auto* selectedLocation = locations.locationChoice();
    assert(selectedLocation);
    assert(selectedLocation->sourceGameId == "crystal_gbc");
    assert(selectedLocation->species == 25);
    assert(selectedLocation->location == 71);
    assert(selectedLocation->minLevel == 25 && selectedLocation->maxLevel == 25);
    assert(selectedLocation->containsLevel(25));
    assert(!selectedLocation->containsLevel(83));
    locations.stepList(1);
    selectedLocation = locations.locationChoice();
    assert(selectedLocation);
    assert(selectedLocation->sourceGameId == "crystal_gbc");
    assert(selectedLocation->species == 25);

    // Graveler's repeated-looking rows are genuine Morning/Day/Night variants.
    Picker::Model gravelerLocations;
    gravelerLocations.openLocation("crystal_gbc", 75, 35, 31);
    int mtMortar31 = 0;
    bool sawMorning = false, sawDay = false, sawNight = false, sawRoute45NightRange = false;
    for (const auto& encounter : gravelerLocations.encounterChoices) {
        assert(encounter.minLevel >= 2 ||
               encounter.method == PokeVault::Integration::EncounterGuardrails::Method::Egg);
        if (encounter.location == 35 && encounter.minLevel == 31 && encounter.maxLevel == 31) {
            ++mtMortar31;
            const auto time = Picker::encounterTimeLabel(encounter.timeMask);
            sawMorning |= time == "Morning";
            sawDay |= time == "Day";
            sawNight |= time == "Night";
        }
        if (encounter.location == 43 && encounter.minLevel == 23 && encounter.maxLevel == 27 &&
            Picker::encounterTimeLabel(encounter.timeMask) == "Night")
            sawRoute45NightRange = true;
    }
    assert(mtMortar31 == 3 && sawMorning && sawDay && sawNight);
    assert(sawRoute45NightRange);

    using EncounterTemplate = PokeVault::Integration::EncounterGuardrails::EncounterTemplate;
    using EncounterMethod = PokeVault::Integration::EncounterGuardrails::Method;
    const EncounterTemplate duplicate{"crystal_gbc", 75, 35, 31, 31, EncounterMethod::Grass, 2};
    const EncounterTemplate distinct{"crystal_gbc", 75, 35, 31, 31, EncounterMethod::Grass, 4};
    const auto deduped = PokeVault::Integration::EncounterGuardrails::deduplicateExactEncounterTemplates(
        std::vector<EncounterTemplate>{duplicate, duplicate, distinct});
    assert(deduped.size() == 2);
    assert(PokeVault::Integration::EncounterGuardrails::sameEncounterTemplate(deduped[0], duplicate));
    assert(!PokeVault::Integration::EncounterGuardrails::sameEncounterTemplate(deduped[0], distinct));

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
    assert(session.setSpecies(1, sourceId));
    const auto speciesOneLevel =
        PokeBank::UIModel::SpeciesChangeLevelPolicy::defaultLevel(sourceId, 1);
    assert(session.working.level == speciesOneLevel);
    assert(session.working.experience==Pokemon::getExpForLevel(speciesOneLevel,3));
    for (uint8_t manual : {uint8_t(1), uint8_t(50), uint8_t(100)}) {
        assert(session.setLevel(manual));
        assert(session.working.level == manual);
        assert(session.working.experience == Pokemon::getExpForLevel(manual, 3));
    }
    assert(session.working.gender==static_cast<uint8_t>(genderFromAttackDV(1,session.working.dvs[1])));
    assert(session.working.dvs[0]==StagedEditor::derivedHPDV(Rules::storedDVs(session.working)));
    assert(!session.setExperience(session.maximumExperience()+1));

    // Named picker models only browse local selection. Session state changes only on explicit Apply.
    session.begin(kept,SessionMode::Edit);
    Picker::Model picker;
    const auto beforeSpeciesBrowse=session.working;
    picker.openSpecies(session.working.species);
    picker.stepList(-24); // Pikachu #25 -> Bulbasaur #1, without touching the edit transaction.
    assert(Rules::sameEditableRecord(beforeSpeciesBrowse,session.working));
    assert(picker.speciesChoice()==1);
    assert(Picker::applySpeciesChoice(session,picker.speciesChoice(), sourceId));
    assert(session.working.species==1);
    assert(session.working.level ==
           PokeBank::UIModel::SpeciesChangeLevelPolicy::defaultLevel(sourceId, 1));
    assert(session.working.level==Pokemon::getLevelFromExp(session.working.experience,3));
    assert(session.working.gender==static_cast<uint8_t>(genderFromAttackDV(1,session.working.dvs[1])));

    session.begin(kept,SessionMode::Edit);
    const auto beforeMoveBrowse=session.working;
    picker.openMove(session.working.moves[0],0);
    picker.stepList(84-static_cast<int>(picker.moveChoice()));
    assert(Rules::sameEditableRecord(beforeMoveBrowse,session.working));
    assert(picker.moveChoice()==84);
    assert(Picker::applyMoveChoice(session,0,picker.moveChoice()));
    assert(session.working.moves[0]==84 && session.working.pp[0]==30 && session.working.ppUps[0]==0);
    picker.openMove(session.working.moves[0],0);picker.stepList(-84);
    assert(picker.moveChoice()==0 && Picker::applyMoveChoice(session,0,0));
    assert(session.working.moves[0]==0 && session.working.pp[0]==0 && session.working.ppUps[0]==0);

    // Pokérus is an inline three-state row, not a picker. Reading an unusual
    // valid raw byte only classifies it; explicit activation is what changes bytes.
    namespace Native = PokeBank::UIModel::Gen2Native;
    uint8_t unusualPokerus = Native::encodePokerus(14, 7);
    const uint8_t viewedPokerus = unusualPokerus;
    assert(Native::pokerusText(0) == "None");
    assert(Native::pokerusText(unusualPokerus) == "Infected");
    assert(Native::pokerusText(Native::encodePokerus(14, 0)) == "Cured");
    assert(unusualPokerus == viewedPokerus);
    const uint8_t infected = Native::cyclePokerusState(0);
    const auto infectedState = Native::decodePokerus(infected);
    assert(infectedState.present && infectedState.active && infectedState.strain != 0 && infectedState.days != 0);
    const uint8_t cured = Native::cyclePokerusState(unusualPokerus);
    const auto curedState = Native::decodePokerus(cured);
    assert(curedState.present && !curedState.active && curedState.strain == 14);
    assert(Native::cyclePokerusState(cured) == 0);

    session.begin(kept,SessionMode::Edit);
    session.working.pokerus = unusualPokerus;
    const auto beforePokerusView = session.working;
    assert(Native::pokerusText(session.working.pokerus) == "Infected");
    assert(Rules::sameEditableRecord(beforePokerusView, session.working));
    session.working.pokerus = Native::cyclePokerusState(session.working.pokerus);
    assert(Native::pokerusText(session.working.pokerus) == "Cured");

    // Staged work at entry is the baseline. Continue keeps the local edits; Discard
    // never calls StagedEditor and preserves prior staged changes byte-for-byte.
    const std::vector<uint8_t> prior(editor->stagedBytes().begin(),editor->stagedBytes().end());
    session.begin(kept,SessionMode::Edit);
    assert(!session.back() && session.confirmExit && session.mode==SessionMode::Edit);
    session.continueEditing();
    assert(!session.confirmExit && session.mode==SessionMode::Edit);
    session.begin(kept,SessionMode::Create);
    assert(!session.back() && session.confirmExit && session.mode==SessionMode::Create);
    session.continueEditing();
    assert(!session.confirmExit && session.mode==SessionMode::Create);
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

    // PP edits use the same session methods as the UI: decreasing Ups clamps,
    // changing a move resets, empty is zero, and Create preserves exhausted PP.
    session.begin(kept,SessionMode::Edit);
    assert(session.setMove(0,33) && session.working.pp[0]==35 && session.working.ppUps[0]==0);
    assert(session.setPPUps(0,3) && session.maximumPP(0)==56);
    assert(session.setPP(0,56) && !session.setPP(0,57));
    assert(session.setPPUps(0,0) && session.working.pp[0]==35);
    assert(session.setMove(0,84) && session.working.pp[0]==30 && session.working.ppUps[0]==0);
    assert(session.setMove(0,0) && session.working.pp[0]==0 && session.working.ppUps[0]==0);
    assert(session.setPPUps(0,3) && session.working.ppUps[0]==0 && !session.setPP(0,1));
    session.begin(kept,SessionMode::Create);session.working.heldItem=0;
    for(size_t i=0;i<4;++i)session.setMove(i,0);
    assert(session.setMove(0,84)&&session.setPP(0,0));
    assert(session.add(*editor,4,slot,error));
    assert(editor->boxedPokemon(4,slot,error)->pp[0]==0);

    // Explicit new choices reject key items; an unusual byte already in the
    // source survives an unrelated edit and strict serialization.
    auto unusualRaw=fixture(layout,true);
    const auto body=boxStart(0)+22;
    unusualRaw[body+1]=7;
    unusualRaw[body+0x17]=63; // unusual existing PP, outside the move maximum
    checksum(unusualRaw,layout);
    auto unusualParsed=parse(unusualRaw,game);assert(unusualParsed);
    auto unusualEditor=StagedEditor::create(*unusualParsed.save,error);assert(unusualEditor);
    auto unusual=*unusualEditor->boxedPokemon(0,0,error);assert(unusual.heldItem==7);
    session.begin(unusual,SessionMode::Edit);session.working.nickname="RAW";
    assert(session.keep(*unusualEditor,0,0,error));
    assert(unusualEditor->boxedPokemon(0,0,error)->heldItem==7);
    assert(unusualEditor->boxedPokemon(0,0,error)->pp[0]==63);
    BoxPokemonEdit badHeld;badHeld.heldItem=54;
    assert(!unusualEditor->stageBoxPokemonEdit(0,0,badHeld,error));
    session.begin(unusual,SessionMode::Create);session.working.moves={};
    session.working.pp={};session.working.ppUps={};
    assert(!session.add(*unusualEditor,2,slot,error));
    assert(!unusualEditor->finalizedBytes(error).empty());

    // Re-accepting an existing unusual move through the named picker is a no-op, preserving raw PP.
    session.begin(unusual,SessionMode::Edit);
    picker.openMove(unusual.moves[0],0);
    const auto unusualPP=session.working.pp[0];
    assert(Picker::applyMoveChoice(session,0,picker.moveChoice()));
    assert(session.working.moves[0]==unusual.moves[0] && session.working.pp[0]==unusualPP);

    // Passive sessions reject edit setters, Keep, Add and picker application.
    session.begin(kept,SessionMode::View);
    assert(!session.setLevel(90)&&!session.setExperience(1000)&&!session.setSpecies(6));
    assert(!session.setMove(0,33)&&!session.setPP(0,1)&&!session.setPPUps(0,1));
    assert(!Picker::applySpeciesChoice(session,6));
    assert(!Picker::applyMoveChoice(session,0,33));
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
 std::cout<<"GSC UI session: progression, named-picker browse/apply, transaction, PP and passive mutation gates PASS\n";}
