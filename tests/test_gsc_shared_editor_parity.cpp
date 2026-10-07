#include "fixtures/gsc_pokemon_fixture.h"
#include "UI/SharedPokemonEditorContract.h"
#include "UI/Gen2PokemonSession.h"
#include "UI/Gen2StagedAccess.h"
#include "UI/Gen2TrainerSession.h"
#include "UI/Gen2PokemonPickerModel.h"
#include "UI/Gen2NativePresentation.h"
#include "UI/BattleStatRadarModel.h"
#include "UI/ExactSaveCapabilities.h"
#include "Integration/Gen2/Gen2BattleStats.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <vector>

namespace Shared = PokeBank::UIModel::SharedPokemonEditor;
namespace SessionRules = PokeBank::UIModel::Gen2PokemonEditor;
namespace Picker = PokeBank::UIModel::Gen2PokemonPicker;
namespace Native = PokeBank::UIModel::Gen2Native;
using SessionRules::Session;
using SessionRules::SessionMode;

void runCreateParity(const L& layout, SourceGame game) {
    const auto raw = fixture(layout, true);
    auto parsed = parse(raw, game);
    assert(parsed);
    std::string error;
    auto editor = StagedEditor::create(*parsed.save, error);
    assert(editor);
    const auto seed = *editor->boxedPokemon(0, 0, error);
    namespace Access = PokeBank::UIModel::Gen2StagedAccess;
    const auto caps = Access::capabilities(true, editor != nullptr);
    assert(!caps.canWriteOriginalSource && caps.canEditStagedCopy);
    for (auto surface : {Access::Surface::Boxes, Access::Surface::StorageSave}) {
        assert(Access::entry(caps, surface, true, false, true, true) == Access::Entry::PokemonActions);
        assert(Access::entry(caps, surface, true, true, true, true) == Access::Entry::None);
        assert(Access::entry(caps, surface, true, false, false, true) == Access::Entry::None);
    }
    assert(Access::entry(Access::capabilities(false, true), Access::Surface::Boxes,
                         true, false, true, true) == Access::Entry::None);
    assert(Access::entry(caps, Access::Surface::Other, true, false, true, true) == Access::Entry::None);

    // Existing unusual moves are orange/preserved in View and Edit, but a newly
    // introduced incompatible draft is red and cannot reach either commit path.
    auto unusual = seed;
    uint16_t incompatible = 1;
    while (MoveCompatibility::canLearnMove(game, seed.species, incompatible)) ++incompatible;
    unusual.moves = {static_cast<uint8_t>(incompatible),0,0,0};
    Session statusSession;
    statusSession.begin(unusual, SessionMode::View);
    assert(statusSession.moveSummary(game) == SessionRules::CompatibilityState::UnusualPreserved);
    statusSession.begin(unusual, SessionMode::Edit);
    assert(statusSession.moveSummary(game) == SessionRules::CompatibilityState::UnusualPreserved);
    statusSession.begin(seed, SessionMode::Edit);
    statusSession.setMove(0, incompatible);
    assert(statusSession.moveSummary(game) == SessionRules::CompatibilityState::NeedsCorrection);
    const auto beforeInvalid = std::vector<uint8_t>(editor->stagedBytes().begin(), editor->stagedBytes().end());
    assert(!statusSession.keep(*editor, 0, 0, error));
    statusSession.begin(unusual, SessionMode::Create);
    size_t rejectedSlot = 0;
    assert(!statusSession.add(*editor, 2, rejectedSlot, error));
    assert(std::equal(beforeInvalid.begin(), beforeInvalid.end(), editor->stagedBytes().begin()));

    // Create starts as a local draft. Browsing from the fixture's Pikachu to
    // Chikorita must not mutate staged or source bytes.
    const std::vector<uint8_t> stagedBefore(editor->stagedBytes().begin(), editor->stagedBytes().end());
    Session session;
    session.begin(seed, SessionMode::Create);
    Picker::Model picker;
    picker.openSpecies(session.working.species);
    while (picker.speciesChoice() != 152) picker.stepList(1);
    assert(session.working.species == seed.species);
    assert(std::equal(stagedBefore.begin(), stagedBefore.end(), editor->stagedBytes().begin()));
    assert(std::equal(raw.begin(), raw.end(), editor->originalBytes().begin()));

    // Explicit selection updates only the draft preview/session. Chikorita must
    // remain Chikorita through the real Stage Add serialization path.
    assert(Picker::applySpeciesChoice(session, picker.speciesChoice()));
    assert(session.working.species == 152);
    session.working.nickname = "CHIKORITA";
    session.working.moves = {0, 0, 0, 0};
    session.working.pp = {0, 0, 0, 0};
    session.working.ppUps = {0, 0, 0, 0};
    session.working.heldItem = 0;
    session.working.caughtData = 0;
    assert(session.setLevel(18));
    const auto previewStats = calculateBattleStats(
        session.working.species, session.working.level,
        SessionRules::storedDVs(session.working), session.working.statExperience).asArray();
    const auto radar = PokeBank::UIModel::gen2RadarModel(previewStats);
    assert(radar.scale > 0.0f);
    assert(std::any_of(radar.normalized.begin(), radar.normalized.end(), [](float value) { return value > 0.0f; }));
    static_assert(PokeBank::UIModel::gen2RadarLabels.size() == 6);
    static_assert(PokeBank::UIModel::gen2RadarLabels[0][0] == 'H');
    static_assert(PokeBank::UIModel::gen2RadarLabels[3][0] == 'S'); // Speed
    assert((PokeBank::UIModel::canonicalGen2RadarStats(
        std::array<uint16_t,6>{1,2,3,4,5,6}) == std::array<uint16_t,6>{1,2,3,4,5,6}));

    size_t slot = 0;
    assert(session.add(*editor, 2, slot, error));
    const auto added = editor->boxedPokemon(2, slot, error);
    assert(added);
    assert(added->species == 152);
    assert(added->nickname == "CHIKORITA");
    assert(added->level == 18);
    assert(std::equal(raw.begin(), raw.end(), editor->originalBytes().begin()));
    assert(std::equal(raw.begin(), raw.end(), parsed.save->sourceBytes().begin()));

    // The same immutable adapter permits an Edit; only Keep reaches staged bytes.
    session.begin(*added, SessionMode::Edit);
    assert(session.setLevel(19));
    assert(session.keep(*editor, 2, slot, error));
    assert(editor->boxedPokemon(2, slot, error)->level == 19);
    assert(std::equal(raw.begin(), raw.end(), editor->originalBytes().begin()));

    // Cancelling a second Create draft restores no bytes because nothing from
    // browsing or local edits ever reached StagedEditor.
    const std::vector<uint8_t> afterAdd(editor->stagedBytes().begin(), editor->stagedBytes().end());
    session.begin(seed, SessionMode::Create);
    picker.openSpecies(session.working.species);
    while (picker.speciesChoice() != 155) picker.stepList(1); // Cyndaquil
    assert(Picker::applySpeciesChoice(session, picker.speciesChoice()));
    assert(session.working.species == 155);
    assert(!session.back());
    assert(session.confirmExit);
    assert(session.mode == SessionMode::Create);
    session.discard();
    assert(session.mode == SessionMode::None);
    assert(std::equal(afterAdd.begin(), afterAdd.end(), editor->stagedBytes().begin()));
}

void runSpeciesAppearance() {
    Session session;
    PokemonRecord seed{};
    seed.species = 25; seed.level = 5; seed.dvs = {0, 9, 9, 9, 9};
    session.begin(seed, SessionMode::Create);
    Picker::Model picker;
    picker.openSpecies(1, false);
    picker.stepList(-10); assert(picker.speciesChoice() == 1);
    picker.stepList(999); assert(picker.speciesChoice() == 251);
    // Every selectable species translates shiny intent into authentic DV/gender state.
    for (uint16_t species = 1; species <= 251; ++species) {
        assert(Picker::applySpeciesAppearance(session, species, true));
        auto dvs = SessionRules::storedDVs(session.working);
        assert(StagedEditor::isShinyDVs(dvs) && session.working.shiny);
        assert(session.working.gender == static_cast<uint8_t>(genderFromAttackDV(species, dvs[0])));
        assert(session.working.dvs[0] == StagedEditor::derivedHPDV(dvs));
        const auto attack = dvs[0];
        assert(Picker::applySpeciesAppearance(session, species, false));
        assert(!session.working.shiny && session.working.dvs[1] == attack);
    }
    const auto committed = session.working;
    picker.openSpecies(committed.species, committed.shiny);
    picker.previewShiny = !picker.previewShiny;
    picker.stepList(-10);
    picker.close();
    assert(SessionRules::sameEditableRecord(committed, session.working));
    session.begin(seed, SessionMode::View);
    assert(!Picker::applySpeciesAppearance(session, 152, true));
}

void runTrainerSession(const L& layout, SourceGame game) {
    const auto raw = fixture(layout, true);
    auto parsed = parse(raw, game);
    std::string error;
    auto editor = StagedEditor::create(*parsed.save, error);
    assert(editor);
    namespace Access = PokeBank::UIModel::Gen2StagedAccess;
    assert(Access::entry(Access::capabilities(true, true), Access::Surface::Trainer,
        true, false, false, true) == Access::Entry::TrainerEdit);
    // Preserve earlier staged Pokemon and Trainer changes across Discard this Edit.
    BoxPokemonEdit edit;
    edit.nickname = "EARLIER";
    assert(editor->stageBoxPokemonEdit(0, 0, edit, error));
    assert(editor->stageMoney(1234, error));
    const auto before = std::vector<uint8_t>(editor->stagedBytes().begin(), editor->stagedBytes().end());
    PokeBank::UIModel::Gen2Trainer::Session session;
    session.begin(*editor);
    session.name = "SILVER";
    session.money = 5678;
    assert(!session.back() && session.confirmExit);
    session.continueEditing();
    assert(session.active && !session.confirmExit && session.name == "SILVER");
    assert(std::equal(before.begin(), before.end(), editor->stagedBytes().begin()));
    session.discard();
    assert(!session.active && session.money == 1234);
    assert(std::equal(before.begin(), before.end(), editor->stagedBytes().begin()));
    session.begin(*editor);
    session.name = "GOLD";
    session.money = 1000000;
    assert(!session.keep(*editor, error));
    assert(std::equal(before.begin(), before.end(), editor->stagedBytes().begin()));
    session.money = 999999;
    session.name = "TOOLONGNAME";
    assert(!session.keep(*editor, error));
    assert(std::equal(before.begin(), before.end(), editor->stagedBytes().begin()));
    session.name = "GOLD";
    assert(session.keep(*editor, error));
    assert(editor->trainerName() == "GOLD" && editor->money() == 999999);
    assert(editor->boxedPokemon(0, 0, error)->nickname == "EARLIER");
    assert(parse(editor->finalizedBytes(error), game));
    assert(std::equal(raw.begin(), raw.end(), editor->originalBytes().begin()));
    assert(std::equal(raw.begin(), raw.end(), parsed.save->sourceBytes().begin()));
}

int main() {
    runSpeciesAppearance();
    // Same conceptual surfaces for occupied and empty slots across generations.
    const auto empty = Shared::actionsForSlot(false);
    assert(Shared::surfaceForAction(empty[0]) == Shared::Surface::CreateDraft);
    const auto occupied = Shared::actionsForSlot(true, {.canClone = true});
    assert(Shared::surfaceForAction(occupied[0]) == Shared::Surface::View);
    assert(Shared::surfaceForAction(occupied[1]) == Shared::Surface::Edit);

    // Passive View is one no-focus/no-edit surface. Generation II extends the
    // presentation with real party state and Crystal-only caught/met capability.
    static_assert(Shared::passiveViewHasFieldCursor());
    static_assert(!Shared::passiveViewAllowsEditing());
    static_assert(Shared::passiveViewAllowsPanelSwitching());

    const auto goldCaps = PokeBank::UIModel::PokemonEditorFoundation::capabilitiesForSourceId("gold_gbc");
    const auto silverCaps = PokeBank::UIModel::PokemonEditorFoundation::capabilitiesForSourceId("silver_gbc");
    const auto crystalCaps = PokeBank::UIModel::PokemonEditorFoundation::capabilitiesForSourceId("crystal_gbc");
    assert(goldCaps && !goldCaps->supportsCrystalCaughtData);
    assert(silverCaps && !silverCaps->supportsCrystalCaughtData);
    assert(crystalCaps && crystalCaps->supportsCrystalCaughtData);

    PokemonRecord boxed{};
    boxed.partyRecord = false;
    boxed.currentHP = 44;
    boxed.maxHP = 55;
    boxed.status = 0x08;
    assert(!Native::partyViewData(boxed));
    auto party = boxed;
    party.partyRecord = true;
    const auto partyData = Native::partyViewData(party);
    assert(partyData);
    assert(partyData->currentHP == 44 && partyData->maxHP == 55);
    assert(partyData->statusText == "Poisoned");

    runTrainerSession(GS, SourceGame::Gold);
    runTrainerSession(GS, SourceGame::Silver);
    runTrainerSession(C, SourceGame::Crystal);
    runCreateParity(GS, SourceGame::Gold);
    runCreateParity(GS, SourceGame::Silver);
    runCreateParity(C, SourceGame::Crystal);

    std::cout << "GSC shared editor parity: non-Pikachu Create, cancel, radar, field gating and source safety PASS\n";
}
