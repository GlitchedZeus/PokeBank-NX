#include "fixtures/gsc_pokemon_fixture.h"
#include "UI/SharedPokemonEditorContract.h"
#include "UI/Gen2PokemonSession.h"
#include "UI/Gen2StagedAccess.h"
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
    assert(session.back());
    assert(session.mode == SessionMode::None);
    assert(std::equal(afterAdd.begin(), afterAdd.end(), editor->stagedBytes().begin()));
}

int main() {
    // Same conceptual surfaces for occupied and empty slots across generations.
    const auto empty = Shared::actionsForSlot(false);
    assert(Shared::surfaceForAction(empty[0]) == Shared::Surface::CreateDraft);
    const auto occupied = Shared::actionsForSlot(true, {.canClone = true});
    assert(Shared::surfaceForAction(occupied[0]) == Shared::Surface::View);
    assert(Shared::surfaceForAction(occupied[1]) == Shared::Surface::Edit);

    // Passive View is one no-focus/no-edit surface. Generation II extends the
    // presentation with real party state and Crystal-only caught/met capability.
    static_assert(!Shared::passiveViewHasFieldCursor());
    static_assert(!Shared::passiveViewAllowsEditing());
    static_assert(!Shared::passiveViewAllowsPanelSwitching());

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

    runCreateParity(GS, SourceGame::Gold);
    runCreateParity(GS, SourceGame::Silver);
    runCreateParity(C, SourceGame::Crystal);

    std::cout << "GSC shared editor parity: non-Pikachu Create, cancel, radar, field gating and source safety PASS\n";
}
