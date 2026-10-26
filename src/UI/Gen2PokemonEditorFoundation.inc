#include "UI/Gen2WorkspacePresentation.h"  // Round 6 7A: the shared Gen II Calculated cell
#include "UI/SharedItemGridPicker.h"
#include "Inventory/ClassicInventoryCatalog.h"
#include "UI/SharedEditorChrome.h"
#include "UI/ClassicFieldColor.h"
#include "UI/ClassicGameContext.h"
// Generation II shared DETAILS | VALUES | MOVES workspace.
// This stays layered over the accepted GSC discovery/export/action-sheet implementation.
// Browsing edits only a local session record; StagedEditor is touched solely by Add / Keep.

#include "Integration/Gen2/Gen2BattleStats.h"
#include "Integration/Gen2/Gen2MoveCompatibility.h"
#include "Integration/Gen2/Gen2PersonalData.h"
#include "Integration/Gen2/Gen2ReadOnlyInventory.h"
#include "Integration/Gen2/Gen2HeldItems.h"
#include "UI/Gen2HeldItemPicker.h"
#include "UI/ItemPickerSort.h"
#include "UI/ItemPickerSearch.h"
#include "UI/Gen2NativePresentation.h"
#include "UI/ExactSaveCapabilities.h"
#include "UI/SharedPokemonEditorContract.h"
#include "UI/Gen2WorkspaceLayers.h"
#include "UI/Gen12DVReroll.h"
#include "UI/Gen2UnownLetter.h"
#include "Names/MoveNames.h"
#include "Names/SpeciesNames.h"
#include "Pokemon/Experience.h"
#include "UI/Gen2PokemonEditorRules.h"
#include "UI/Gen2PokemonSession.h"
#include "UI/PokemonViewActions.h"
#include "UI/Modals/Gen2PokemonDetailsModal.h"
#include "Pokemon/Pokemon2ReadOnly.h"
#include "UI/PokemonEditorFoundationContract.h"
#include "UI/StatsRadar.h"
#include "UI/ScreenChrome.h"
#include "UI/TouchScroll.h"
#include "UI/SharedHeldItemInput.h"
#include "UI/Dialogs/DialogFrame.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace UI::Gen2PokemonEditor {
namespace {

namespace Gen2 = PokeVault::Integration::Gen2;
namespace Rules = PokeBank::UIModel::Gen2PokemonEditor;
namespace Foundation = PokeBank::UIModel::PokemonEditorFoundation;
namespace SharedEditor = PokeBank::UIModel::SharedPokemonEditor;
namespace Gen2Native = PokeBank::UIModel::Gen2Native;

using Mode = Rules::SessionMode;

struct State : Rules::Session {
    const TrainerViewScreen* owner = nullptr;
    uint64_t ownerSerial = 0;   // TrainerViewScreen::instanceSerial (never reused, unlike the address)
    bool itemPicker = false;
    int box = 0;
    int slot = 0;
    bool party = false;
    Foundation::Panel panel = Foundation::Panel::Identity;
    int row = 0;
    int column = 0;
    uint32_t rng = 0x2A6F19D3u;
    std::vector<uint8_t> itemChoices;
    std::vector<uint8_t> itemNativeChoices;
    ItemPickerSort::Mode itemSort = ItemPickerSort::Mode::Native;
    std::string itemQuery;   // NEW-06: Held Item search (narrows the presentation copy only)
    SharedHeldItemSearch::State heldItemSearch;
    int itemChoiceIndex = 0;
    TouchScrollState itemScroll;
    TouchPickerViewport itemViewport;
    // UI-only until a real Crystal caught-history record exists. This lets Create/Edit
    // choose OT gender before choosing a met location without fabricating caughtData.
    Gen2Native::CrystalOtGenderChoice crystalOtGender = Gen2Native::CrystalOtGenderChoice::Unknown;
    // Pending editor-only Crystal met time. It must not create caught history by itself.
    uint8_t crystalMetTime = 0; // 0 Unknown, 1 Morning, 2 Day, 3 Night
};

State& stateFor(const TrainerViewScreen& screen) {
    static State state;
    if ((state.owner != &screen || state.ownerSerial != screen.instanceSerial)) {
        state = {};
        state.owner = &screen; state.ownerSerial = screen.instanceSerial;
    }
    return state;
}

bool isGSC(const TrainerViewScreen& screen) noexcept {
    return Rules::exactGameFromSourceId(screen.sourceGameId).has_value();
}

std::array<uint8_t, 4> editableDVs(const Gen2::PokemonRecord& p) noexcept {
    return {p.dvs[1], p.dvs[2], p.dvs[3], p.dvs[4]};
}

void applyEditableDVs(Gen2::PokemonRecord& p, const std::array<uint8_t, 4>& dvs) noexcept {
    p.dvs[1] = dvs[0];
    p.dvs[2] = dvs[1];
    p.dvs[3] = dvs[2];
    p.dvs[4] = dvs[3];
    p.dvs[0] = Gen2::StagedEditor::derivedHPDV(dvs);
    p.shiny = Gen2::StagedEditor::isShinyDVs(dvs);
    p.gender = static_cast<uint8_t>(Gen2::genderFromAttackDV(p.species, dvs[0]));
    // The Unown letter also comes from these DVs (pokecrystal GetUnownLetter); keep the draft's form in step.
    p.form = p.species == 201 ? static_cast<uint8_t>(PokeBank::UIModel::Gen12DVReroll::unownLetter(dvs)) : 0;
}

void refreshDerived(Gen2::PokemonRecord& p) noexcept {
    applyEditableDVs(p, editableDVs(p));
}

uint32_t minimumExpFor(const Gen2::PokemonRecord& p) noexcept {
    const auto* personal = Gen2::personalRecord(p.species);
    return personal ? Pokemon::getExpForLevel(p.level, personal->experienceGrowth) : 0;
}

std::string itemText(uint8_t item) {
    if (item == 0) return "None";
    return PokeBank::UIModel::Gen2HeldItemPicker::itemName(item);
}

std::string genderText(uint8_t gender) {
    switch (static_cast<Gen2::PokemonGender>(gender)) {
        case Gen2::PokemonGender::Male: return "Male";
        case Gen2::PokemonGender::Female: return "Female";
        case Gen2::PokemonGender::Genderless: return "Genderless";
        default: return "Unknown";
    }
}

std::string pokerusText(uint8_t raw) {
    if (raw == 0) return "None";
    return "S" + std::to_string(raw >> 4U) + " / D" + std::to_string(raw & 0x0FU);
}


constexpr int identityRows() noexcept { return 8; }
constexpr int valueRows() noexcept { return 8; }
constexpr int moveRows() noexcept { return 4; }

int rowsForPanel(Foundation::Panel panel) noexcept {
    switch (panel) {
        case Foundation::Panel::Identity: return identityRows();
        case Foundation::Panel::Values: return valueRows();
        case Foundation::Panel::Moves: return moveRows();
    }
    return identityRows();
}

void clampFocus(State& state) noexcept {
    state.row = std::clamp(state.row, 0, rowsForPanel(state.panel) - 1);
    if (state.panel == Foundation::Panel::Values)
        state.column = state.row <= 4 ? std::clamp(state.column, 0, 1) : 0;
    else if (state.panel == Foundation::Panel::Moves)
        state.column = std::clamp(state.column, 0, 2);
    else
        state.column = 0;
}

void nextPanel(State& state, int direction) noexcept {
    int panel = static_cast<int>(state.panel);
    panel = (panel + direction + 3) % 3;
    state.panel = static_cast<Foundation::Panel>(panel);
    clampFocus(state);
}

Gen2::PokemonRecord defaultCreateRecord(Gen2::StagedEditor& editor) {
    Gen2::PokemonRecord p;
    p.species = 25;
    p.level = PokeBank::UIModel::SpeciesChangeLevelPolicy::fallbackLevel;
    p.nickname = PokeBank::UIModel::classicDefaultNickname(p.species);
    p.originalTrainer = editor.trainerName();
    p.trainerId = editor.trainerId();
    p.heldItem = 0;
    p.moves = {};
    p.pp = {};
    p.ppUps = {};
    p.statExperience = {};
    p.friendship = 70;
    p.pokerus = 0;
    p.caughtData = 0;
    applyEditableDVs(p, {0, 0, 0, 0});
    p.experience = minimumExpFor(p);
    return p;
}

bool beginCreate(TrainerViewScreen& screen, int box) {
    auto* editor = stagedEditor(screen);
    if (!editor) return false;
    auto& state = stateFor(screen);
    state.mode = Mode::Create;
    state.confirmExit = false;
    state.itemPicker = false;
    state.box = box;
    state.party = false;
    state.slot = -1;
    state.panel = Foundation::Panel::Identity;
    state.row = 0;
    state.column = 0;
    state.working = defaultCreateRecord(*editor);
    // A new Pokemon starts with its species' native level-up moves, like a species change during
    // Create and Gen I / III-V Create (Pikachu Lv 5: Thunder Shock, Growl).
    Rules::seedNativeMoves(state.working, screen.sourceGameId);
    state.begin(state.working, Mode::Create);
    state.crystalOtGender = Gen2Native::CrystalOtGenderChoice::Unknown;
    state.crystalMetTime = 0;
    state.rng ^= static_cast<uint32_t>((box + 1) * 0x9E3779B9u);
    screen.controllerNavigation.reset();
    return true;
}

bool beginEdit(TrainerViewScreen& screen, int box, int slot, bool party=false) {
    auto* editor = stagedEditor(screen);
    if (!editor) return false;
    std::string error;
    auto pokemon = party?editor->partyPokemon(static_cast<size_t>(slot),error):
        editor->boxedPokemon(static_cast<std::size_t>(box), static_cast<std::size_t>(slot), error);
    if (!pokemon) {
        screen.postStatus(error.empty() ? "Selected Gen II slot is empty" : error, 300);
        return false;
    }
    auto& state = stateFor(screen);
    state.mode = Mode::Edit;
    state.confirmExit = false;
    state.itemPicker = false;
    state.box = box;
    state.slot = slot;
    state.party = party;
    state.panel = Foundation::Panel::Identity;
    state.row = 0;
    state.column = 0;
    state.baseline = *pokemon; // exact staged state at Edit entry, not original source bytes.
    state.working = *pokemon;
    state.begin(*pokemon, Mode::Edit);
    const auto caught = Gen2Native::decodeCrystalCaughtData(state.working.caughtData);
    state.crystalOtGender = Gen2Native::crystalOtGenderChoiceFromCaught(caught);
    state.crystalMetTime = caught.present ? caught.timeOfDay : 0;
    state.rng ^= static_cast<uint32_t>((box + 1) * 0x85EBCA6Bu + (slot + 1));
    screen.controllerNavigation.reset();
    return true;
}

void closeToActions(TrainerViewScreen& screen) {
    auto& state = stateFor(screen);
    state.mode = Mode::None;
    state.confirmExit = false;
    state.itemPicker = false;
    auto& legacy = editorOverlayState(screen);
    legacy.active = true;
    legacy.review = false;
    legacy.pokemonActions = true;
    legacy.pokemonParty = state.party;
    legacy.pokemonSlot = state.slot;
    legacy.pokemonBox = state.box;
    legacy.pokemonFields = false;
    legacy.pokemonRow = 0;
    screen.controllerNavigation.reset();
}

void closeToBoxAfterAdd(TrainerViewScreen& screen, int box, int slot) {
    const auto target = SharedEditor::postAddSelection(box, slot);
    auto& state = stateFor(screen);
    state.mode = Mode::None;
    state.confirmExit = false;
    state.itemPicker = false;

    auto& legacy = editorOverlayState(screen);
    legacy.active = false;
    legacy.review = false;
    legacy.pokemonActions = false;
    legacy.pokemonFields = false;
    legacy.pokemonBox = target.box;
    legacy.pokemonSlot = target.slot;
    legacy.pokemonRow = 0;

    if (screen.selectedMode == TrainerViewScreen::ViewMode::Storage && screen.storageFocusPane == 0) {
        screen.stSaveBox = target.box;
        screen.stSaveSlot = target.slot;
    } else {
        screen.selectedBoxIndex = target.box;
        screen.selectedItemIndex = target.slot;
        screen.detailViewActive = true;
    }
    screen.controllerNavigation.reset();
}


Rules::CompatibilityState moveStatus(const TrainerViewScreen& screen, const State& state, int slot) noexcept {
    const auto game = Rules::exactGameFromSourceId(screen.sourceGameId);
    if (!game)
        return state.mode == Mode::Create ? Rules::CompatibilityState::NeedsCorrection
                                          : Rules::CompatibilityState::UnusualPreserved;
    return state.moveCompatibility(*game, static_cast<std::size_t>(slot));
}

bool movesAllowCommit(const TrainerViewScreen& screen, const State& state) noexcept {
    for (int slot = 0; slot < 4; ++slot)
        if (!Rules::mayCommit(moveStatus(screen, state, slot))) return false;
    return true;
}

bool stageCreate(TrainerViewScreen& screen) {
    auto& state = stateFor(screen);
    auto* editor = stagedEditor(screen);
    if (!editor || state.mode != Mode::Create) return false;
    if (!movesAllowCommit(screen, state)) {
        screen.postStatus("Add blocked: correct red incompatible move rows first", 300);
        return false;
    }

    const std::string displayName = state.working.nickname.empty()
        ? PokeBank::UIModel::classicDefaultNickname(state.working.species) : state.working.nickname;
    const int destinationBox = state.box;
    std::size_t slot = 0;
    std::string error;
    if (!state.add(*editor, static_cast<std::size_t>(destinationBox), slot, error)) {
        screen.postStatus(error.empty() ? "Could not add Pokemon" : error, 300);
        return false;
    }

    auto& legacy = editorOverlayState(screen);
    legacy.pokemonBox = destinationBox;
    legacy.pokemonSlot = static_cast<int>(slot);
    if (!gscTrainer(screen).refreshStagedBoxPresentation(error)) {
        screen.postStatus(error.empty() ? "Pokemon added, but the staged box display could not refresh" : error, 360);
    } else {
        screen.postStatus(displayName + " added to Box " + std::to_string(destinationBox + 1), 260);
    }
    closeToBoxAfterAdd(screen, destinationBox, static_cast<int>(slot));
    return true;
}

bool stageEdit(TrainerViewScreen& screen) {
    auto& state = stateFor(screen);
    auto* editor = stagedEditor(screen);
    if (!editor || state.mode != Mode::Edit) return false;
    if (!movesAllowCommit(screen, state)) {
        screen.postStatus("Keep blocked: correct newly incompatible move choices first", 300);
        return false;
    }

    std::string error;
    if (!state.keep(*editor, static_cast<std::size_t>(state.box),
                                     static_cast<std::size_t>(state.slot), error,state.party)) {
        screen.postStatus(error.empty() ? "Gen II staged edit was rejected" : error, 300);
        return false;
    }
    gscTrainer(screen).refreshStagedBoxPresentation(error);
    screen.postStatus("Pokemon edit kept", 260);
    closeToActions(screen);
    return true;
}

bool chooseGender(TrainerViewScreen& screen, Gen2::PokemonRecord& p) {
    const auto current = Gen2::genderFromAttackDV(p.species, p.dvs[1]);
    if (current == Gen2::PokemonGender::Genderless || current == Gen2::PokemonGender::Unknown) {
        screen.postStatus(current == Gen2::PokemonGender::Genderless
            ? "This species is genderless in Generation II"
            : "Gender is unavailable for this species", 240);
        return false;
    }
    const auto target = current == Gen2::PokemonGender::Male
        ? Gen2::PokemonGender::Female : Gen2::PokemonGender::Male;
    const auto dvs = editableDVs(p);
    const bool wasShiny = Gen2::StagedEditor::isShinyDVs(dvs);
    int best = -1;
    int bestScore = 1000;
    for (int candidate = 0; candidate <= 15; ++candidate) {
        if (Gen2::genderFromAttackDV(p.species, static_cast<uint8_t>(candidate)) != target) continue;
        auto candidateDVs = dvs;
        candidateDVs[0] = static_cast<uint8_t>(candidate);
        const bool preservesShiny = Gen2::StagedEditor::isShinyDVs(candidateDVs) == wasShiny;
        const int score = (preservesShiny ? 0 : 100) + std::abs(candidate - static_cast<int>(dvs[0]));
        if (score < bestScore) { bestScore = score; best = candidate; }
    }
    if (best < 0) {
        screen.postStatus("This species has no opposite-gender DV state", 240);
        return false;
    }
    auto changed = dvs;
    changed[0] = static_cast<uint8_t>(best);
    applyEditableDVs(p, changed);
    if (p.shiny != wasShiny)
        screen.postStatus("Gender changed with authentic Attack DV; shiny state changed accordingly", 300);
    return true;
}

// "Atk 7->10, Def 6->10": the Gen II DVs a shiny toggle rewrote, so the change is never silent
// (gender, stats and Hidden Power all derive from these DVs).
std::string describeDVChanges(const std::array<uint8_t, 4>& before, const std::array<uint8_t, 4>& after) {
    static constexpr std::array<const char*, 4> names{"Atk", "Def", "Spe", "Spc"};
    std::string out;
    for (std::size_t i = 0; i < before.size(); ++i) {
        if (before[i] == after[i]) continue;
        if (!out.empty()) out += ", ";
        out += std::string(names[i]) + " " + std::to_string(before[i]) + "->" + std::to_string(after[i]);
    }
    return out;
}

// Unown's letter is bits 1-2 of the same DVs, so a shiny toggle keeps it whenever the letter
// allows (Gen2UnownLetter.h) and says so when it cannot.
void toggleUnownShiny(TrainerViewScreen& screen, Gen2::PokemonRecord& p) {
    namespace Letter = PokeBank::UIModel::Gen2UnownLetter;
    const auto before = editableDVs(p);
    const int oldLetter = PokeBank::UIModel::Gen12DVReroll::unownLetter(before);
    const auto toggled = Letter::toggleShiny(before);
    applyEditableDVs(p, toggled.dvs);
    const std::string changes = "DVs " + describeDVChanges(before, toggled.dvs);
    if (!p.shiny) {
        screen.postStatus("Shiny off: Unown " + std::string(1, Letter::letterChar(oldLetter)) +
                          " kept; " + changes, 300);
    } else if (toggled.letterChanged) {
        screen.postStatus("Only Unown " + Letter::shinyLettersText() + " can be shiny: letter " +
                          std::string(1, Letter::letterChar(oldLetter)) + " -> " +
                          std::string(1, Letter::letterChar(p.form)) + "; " + changes, 420);
    } else {
        screen.postStatus("Shiny on: Unown " + std::string(1, Letter::letterChar(p.form)) +
                          " kept; " + changes, 300);
    }
}

// Unown letter selector: one letter A-Z typed on the keyboard; the DVs closest to the current
// ones that give that letter are applied (shiny state kept when that letter can be shiny).
void chooseUnownLetter(TrainerViewScreen& screen, Gen2::PokemonRecord& p) {
    namespace Letter = PokeBank::UIModel::Gen2UnownLetter;
    const auto before = editableDVs(p);
    const std::string current(1, Letter::letterChar(PokeBank::UIModel::Gen12DVReroll::unownLetter(before)));
    const auto result = Utils::promptText("Unown letter", "A-Z; the letter follows the DVs", current, 1);
    if (!result.accepted) return;
    const int letter = Letter::parseLetter(result.text);
    const auto selection = Letter::selectLetter(before, letter);
    switch (selection.outcome) {
        case Letter::Outcome::Invalid:
            screen.postStatus("Enter one letter, A to Z", 240);
            return;
        case Letter::Outcome::Unchanged:
            screen.postStatus("Already Unown " + current, 200);
            return;
        case Letter::Outcome::ShinyImpossible:
            screen.postStatus("Unown " + std::string(1, Letter::letterChar(letter)) +
                              " cannot be shiny; only " + Letter::shinyLettersText() +
                              " can. Turn Shiny off first. Nothing changed", 420);
            return;
        case Letter::Outcome::Changed:
            applyEditableDVs(p, selection.dvs);
            screen.postStatus("Unown " + std::string(1, Letter::letterChar(letter)) + ": DVs " +
                              describeDVChanges(before, selection.dvs) + " (stats follow)", 320);
            return;
    }
}

void toggleShiny(TrainerViewScreen& screen, Gen2::PokemonRecord& p) {
    if (p.species == PokeBank::UIModel::Gen2UnownLetter::kUnownSpecies) {
        toggleUnownShiny(screen, p);
        return;
    }
    const auto before = editableDVs(p);
    auto dvs = before;
    if (Gen2::StagedEditor::isShinyDVs(dvs)) {
        // Preserve Attack DV/gender when making normal; changing Defense alone breaks the shiny pattern.
        dvs[1] = dvs[1] == 10 ? 9 : dvs[1];
        if (Gen2::StagedEditor::isShinyDVs(dvs)) dvs[2] = 9;
        applyEditableDVs(p, dvs);
        screen.postStatus("Shiny off: DVs " + describeDVChanges(before, dvs) +
                          " (stats and Hidden Power follow)", 300);
        return;
    }

    const auto originalGender = Gen2::genderFromAttackDV(p.species, dvs[0]);
    constexpr std::array<uint8_t, 8> shinyAttack{2, 3, 6, 7, 10, 11, 14, 15};
    int best = shinyAttack.front();
    int bestScore = 1000;
    for (uint8_t candidate : shinyAttack) {
        const auto gender = Gen2::genderFromAttackDV(p.species, candidate);
        const bool sameGender = gender == originalGender || originalGender == Gen2::PokemonGender::Genderless;
        const int score = (sameGender ? 0 : 100) + std::abs(static_cast<int>(candidate) - static_cast<int>(dvs[0]));
        if (score < bestScore) { bestScore = score; best = candidate; }
    }
    const std::array<uint8_t, 4> shiny{static_cast<uint8_t>(best), 10, 10, 10};
    applyEditableDVs(p, shiny);
    const std::string changes = "DVs " + describeDVChanges(before, shiny);
    if (Gen2::genderFromAttackDV(p.species, p.dvs[1]) != originalGender &&
        originalGender != Gen2::PokemonGender::Genderless)
        screen.postStatus("Authentic shiny DVs changed gender for this species ratio; " + changes, 360);
    else
        screen.postStatus("Shiny on: " + changes + " (stats and Hidden Power follow)", 300);
}

// Y Random DVs: a new roll that keeps shiny state, Gen II gender (Attack DV vs. ratio) and the
// Unown letter. False when no other combination keeps them (the draft is unchanged).
bool randomizeDVs(State& state) {
    namespace Reroll = PokeBank::UIModel::Gen12DVReroll;
    const uint16_t species = state.working.species;
    const auto result = Reroll::reroll(editableDVs(state.working), state.rng,
        [species](uint8_t attack) { return static_cast<int>(Gen2::genderFromAttackDV(species, attack)); },
        species == 201);
    if (!result.changed) return false;
    applyEditableDVs(state.working, result.dvs);
    return true;
}

void openHeldItemPicker(State& state) {
    state.itemChoices = Rules::heldItemChoices();
    state.itemNativeChoices = state.itemChoices;
    state.itemSort = ItemPickerSort::Mode::Native;
    state.itemQuery.clear();
    state.heldItemSearch={};
    state.itemChoiceIndex = PokeBank::UIModel::Gen2HeldItemPicker::initialIndex(state.itemChoices, state.working.heldItem);
    state.itemScroll.reset();
    state.itemViewport.reset();
    state.itemViewport.ensure(
        state.itemChoiceIndex, static_cast<int>(state.itemChoices.size()),
        PokeBank::UIModel::Gen2HeldItemPicker::rows,
        PokeBank::UIModel::Gen2HeldItemPicker::columns);
    state.itemPicker = true;
}

// Hardware rule: every A press launches at most one swkbd applet. Level and EXP are separate rows.
void editIdentityField(TrainerViewScreen& screen, State& state) {
    auto& p = state.working;
    switch (state.row) {
        case 0: {
            const uint16_t oldSpecies = p.species;
            const std::string oldDefault = PokeBank::UIModel::classicDefaultNickname(oldSpecies);
            const auto result = Utils::promptNumber("Gen II Species ID", p.species, 1, 251);
            if (!result.accepted) return;
            state.setSpecies(static_cast<uint16_t>(result.value), screen.sourceGameId);
            if (state.mode == Mode::Create && p.nickname == oldDefault)
                p.nickname = PokeBank::UIModel::classicDefaultNickname(p.species);
            refreshDerived(p);
            break;
        }
        case 1: {
            const auto result = Utils::promptText("Gen II Nickname", "1-10 Gen II characters", p.nickname, 10);
            if (result.accepted) p.nickname = result.text;
            break;
        }
        case 2: {
            const auto result = Utils::promptNumber("Gen II Level", p.level, 1, 100);
            if (!result.accepted) return;
            state.setLevel(static_cast<uint8_t>(result.value));
            break;
        }
        case 3: {
            const auto result = Utils::promptNumber("Gen II Experience", static_cast<int>(p.experience), 0, static_cast<int>(state.maximumExperience()));
            if (result.accepted) state.setExperience(static_cast<uint32_t>(result.value));
            break;
        }
        case 4: {
            const auto result = Utils::promptText("Gen II OT Name", "1-10 Gen II characters", p.originalTrainer, 10);
            if (result.accepted) p.originalTrainer = result.text;
            break;
        }
        case 5: {
            const auto result = Utils::promptNumber("Gen II Trainer ID", p.trainerId, 0, 65535);
            if (result.accepted) p.trainerId = static_cast<uint16_t>(result.value);
            break;
        }
        case 6:
            chooseGender(screen, p);
            break;
        case 7:
            openHeldItemPicker(state);
            break;
    }
}

void editValueField(TrainerViewScreen& screen, State& state) {
    auto& p = state.working;
    if (state.row <= 4) {
        if (state.column == 0) {
            if (state.row == 0) {
                screen.postStatus("HP DV is derived from Attack/Defense/Speed/Special DVs", 240);
                return;
            }
            auto dvs = editableDVs(p);
            const int index = state.row - 1;
            const auto result = Utils::promptNumber("Gen II DV (0-15)", dvs[static_cast<std::size_t>(index)], 0, 15);
            if (!result.accepted) return;
            dvs[static_cast<std::size_t>(index)] = static_cast<uint8_t>(result.value);
            applyEditableDVs(p, dvs);
        } else {
            auto statExp = p.statExperience;
            const auto result = Utils::promptNumber("Gen II Stat Experience (0-65535)",
                                                    statExp[static_cast<std::size_t>(state.row)], 0, 65535);
            if (!result.accepted) return;
            statExp[static_cast<std::size_t>(state.row)] = static_cast<uint16_t>(result.value);
            p.statExperience = statExp;
        }
        return;
    }
    if (state.row == 5) {
        toggleShiny(screen, p);
        return;
    }
    if (state.row == 6) {
        const auto result = Utils::promptNumber("Gen II Friendship", p.friendship, 0, 255);
        if (result.accepted) p.friendship = static_cast<uint8_t>(result.value);
        return;
    }
    const auto result = Utils::promptNumber("Gen II Pokerus byte", p.pokerus, 0, 255);
    if (result.accepted) p.pokerus = static_cast<uint8_t>(result.value);
}

void editMoveField(State& state) {
    auto& p = state.working;
    const int slot = state.row;
    if (state.column == 0) {
        const auto result = Utils::promptNumber("Gen II Move ID (0=Empty)", p.moves[slot], 0, 251);
        if (!result.accepted) return;
        state.setMove(slot, static_cast<uint16_t>(result.value));
        return;
    }
    if (state.column == 1) {
        const auto result = Utils::promptNumber("Gen II Move PP", std::min(p.pp[slot], state.maximumPP(slot)), 0, state.maximumPP(slot));
        if (result.accepted) state.setPP(slot, static_cast<uint8_t>(result.value));
        return;
    }
    const auto result = Utils::promptNumber("Gen II PP Ups", p.ppUps[slot], 0, p.moves[slot] == 0 ? 0 : 3);
    if (result.accepted) state.setPPUps(slot, static_cast<uint8_t>(result.value));
}

void activateField(TrainerViewScreen& screen, State& state) {
    if (state.panel == Foundation::Panel::Identity) editIdentityField(screen, state);
    else if (state.panel == Foundation::Panel::Values) editValueField(screen, state);
    else editMoveField(state);
}

bool handleItemPicker(TrainerViewScreen& screen, State& state, uint64_t down, uint64_t held,
                      int stickX, int stickY, const TouchInput& touch) {
    const uint64_t nav = screen.controllerNavigation.apply(
        down, held, stickX, stickY,
        HidNpadButton_Up, HidNpadButton_Down, HidNpadButton_Left, HidNpadButton_Right);
    const int count = static_cast<int>(state.itemChoices.size());
    if (SharedPopupDismiss::tappedOutside(touch)) {
        // 7D: a release-confirmed tap outside the picker cancels it like B; this frame's
        // return keeps the gesture from reaching the workspace underneath.
        state.itemScroll.stop();
        state.itemPicker = false;
        screen.controllerNavigation.reset();
        return true;
    }
    namespace Grid = PokeBank::UIModel::SharedHeldItemPicker;
    constexpr int pickerW = Grid::modalWidth, pickerH = Grid::modalHeight;
    const int px = (1280 - pickerW) / 2;
    const int py = (720 - kNavBarH - pickerH) / 2;
    const auto heldItemName=[](uint8_t id){return PokeBank::UIModel::Gen2HeldItemPicker::itemName(id);};
    const auto refilter=[&](const std::string& query) {
        const int row=ItemPickerSearch::apply(state.itemChoices,state.itemNativeChoices,state.itemSort,
            query,state.itemChoiceIndex,heldItemName,SharedSearchInput::searchKey);
        state.itemQuery=query;
        if(row>=0)state.itemChoiceIndex=row;
        state.itemViewport.reset();
        return row>=0;
    };
    const SharedSearchInput::Keys searchKeys{bool(nav&HidNpadButton_Up),bool(nav&HidNpadButton_Down),
        bool(nav&HidNpadButton_Left),bool(nav&HidNpadButton_Right),bool(down&HidNpadButton_A),bool(down&HidNpadButton_B)};
    if(SharedHeldItemSearch::handle(state.heldItemSearch,down&HidNpadButton_Minus,searchKeys,
        SharedSearchInput::sample(touch),px,py,refilter,
        down&(HidNpadButton_L|HidNpadButton_R|HidNpadButton_ZL|HidNpadButton_ZR|HidNpadButton_X|HidNpadButton_Y))) {
        state.itemScroll.stop();
        state.itemViewport.reveal(state.itemChoiceIndex,static_cast<int>(state.itemChoices.size()),
            Grid::rows,Grid::columnsForSearch(state.heldItemSearch.open()));
        return true;
    }
    const int columns=Grid::columnsForSearch(state.heldItemSearch.open());
    namespace Model = PokeBank::UIModel::Gen2HeldItemPicker;
    if (down & HidNpadButton_X) {
        state.itemSort = ItemPickerSort::next(state.itemSort);
        state.itemChoiceIndex = ItemPickerSort::reorder(
            state.itemChoices, state.itemNativeChoices, state.itemSort,
            state.itemChoiceIndex, [](uint8_t id) {
                return PokeBank::UIModel::Gen2HeldItemPicker::itemName(id);
            });
        // NEW-06: a re-sort keeps the active search.
        state.itemChoiceIndex = std::max(0, ItemPickerSearch::narrow(state.itemChoices, state.itemQuery,
                                                                     state.itemChoiceIndex, heldItemName,SharedSearchInput::searchKey));
        state.itemScroll.stop();
        state.itemViewport.reset();
        state.itemViewport.reveal(state.itemChoiceIndex, static_cast<int>(state.itemChoices.size()), Model::rows, columns);
        return true;
    }
    if (down & HidNpadButton_B) {
        state.itemScroll.stop();
        state.itemPicker = false;
        screen.controllerNavigation.reset();
        return true;
    }
    // L06: scrolling and semantic selection stay independent (touch-down is never a choice); a
    // release-confirmed tap on a cell and A on the focus share one step and one commit.
    const auto step = SharedHeldItemInput::update(touch, state.itemScroll, state.itemViewport,
        state.itemChoiceIndex, count, state.heldItemSearch.open(), px, py,
        {bool(nav & HidNpadButton_Up), bool(nav & HidNpadButton_Down), bool(nav & HidNpadButton_Left),
         bool(nav & HidNpadButton_Right), bool(down & HidNpadButton_L), bool(down & HidNpadButton_R),
         bool(down & HidNpadButton_A)});
    if (step.revealed)
        screen.postStatus("Focused choice brought into view; press A again or tap a visible row", 220);
    if (step.choose) {
        state.working.heldItem = state.itemChoices[static_cast<std::size_t>(state.itemChoiceIndex)];
        state.itemPicker = false;
        screen.controllerNavigation.reset();
    }
    return true;
}

bool handleConfirm(TrainerViewScreen& screen, State& state, uint64_t down) {
    // Match PKSE's native creator/details confirmation mapping: B Back, Y Discard, A Keep/Save.
    // One button acts per frame (Gen2WorkspaceLayers::exitConfirmStep): a Keep that fails to stage
    // leaves the dialog open and never falls through to Discard or Back in the same frame.
    // The local session still never mutates source bytes; only the PokeBank staged copy can change.
    namespace Layers = PokeBank::UIModel::Gen2WorkspaceLayers;
    const bool create = state.mode == Mode::Create;
    switch (Layers::exitConfirmStep((down & HidNpadButton_A) != 0, (down & HidNpadButton_B) != 0,
                                    (down & HidNpadButton_Y) != 0)) {
        case Layers::ExitChoice::Keep:
            (void)(create ? stageCreate(screen) : stageEdit(screen));
            return true;
        case Layers::ExitChoice::Discard:
            screen.postStatus(create
                ? "Create draft discarded; staged/source bytes unchanged"
                : "This Edit session discarded; earlier staged work preserved", 300);
            state.discard();
            closeToActions(screen);
            return true;
        case Layers::ExitChoice::Continue:
            state.continueEditing();
            screen.controllerNavigation.reset();
            return true;
        case Layers::ExitChoice::None:
            return true;
    }
    return true;
}

} // namespace

bool active(const TrainerViewScreen& screen) noexcept {
    return stateFor(screen).mode != Mode::None;
}

bool handleInput(TrainerViewScreen& screen, uint64_t down, uint64_t held,
                 int stickX, int stickY, const TouchInput& touch) {
    if (!isGSC(screen)) return false;
    auto& state = stateFor(screen);

    if (state.mode == Mode::None) {
        auto& legacy = editorOverlayState(screen);
        if (!legacy.active || legacy.review || !legacy.pokemonActions || !(down & HidNpadButton_A))
            return false;

        auto* editor = stagedEditor(screen);
        if (!editor) return false;
        std::string error;
        auto pokemon = editor->boxedPokemon(static_cast<std::size_t>(legacy.pokemonBox),
                                             static_cast<std::size_t>(legacy.pokemonSlot), error);
        const bool occupied = pokemon.has_value();
        const auto action = Rules::slotActionAt(occupied, legacy.pokemonRow);
        if (action == Rules::SlotAction::View) {
            state.begin(*pokemon, Mode::View);
            screen.controllerNavigation.reset();
            return true;
        }
        if (action == Rules::SlotAction::Edit)
            return beginEdit(screen, legacy.pokemonBox, legacy.pokemonSlot);
        if (action == Rules::SlotAction::Add)
            return beginCreate(screen, legacy.pokemonBox);
        return false;
    }

    if (state.mode == Mode::View) {
        if (PokeBank::UIModel::passiveViewAction(down & HidNpadButton_B) == PokeBank::UIModel::PassiveViewAction::Back)
            closeToActions(screen);
        return true;
    }
    if (state.itemPicker) return handleItemPicker(screen, state, down, held, stickX, stickY, touch);
    if (state.confirmExit) return handleConfirm(screen, state, down);

    const uint64_t nav = screen.controllerNavigation.apply(
        down, held, stickX, stickY,
        HidNpadButton_Up, HidNpadButton_Down, HidNpadButton_Left, HidNpadButton_Right);

    if (down & HidNpadButton_L) nextPanel(state, -1);
    if (down & HidNpadButton_R) nextPanel(state, 1);
    if (nav & HidNpadButton_Up) {
        state.row = (state.row - 1 + rowsForPanel(state.panel)) % rowsForPanel(state.panel);
        clampFocus(state);
    }
    if (nav & HidNpadButton_Down) {
        state.row = (state.row + 1) % rowsForPanel(state.panel);
        clampFocus(state);
    }
    if (nav & HidNpadButton_Left) {
        if (state.panel == Foundation::Panel::Values && state.row <= 4) state.column = std::max(0, state.column - 1);
        if (state.panel == Foundation::Panel::Moves) state.column = std::max(0, state.column - 1);
    }
    if (nav & HidNpadButton_Right) {
        if (state.panel == Foundation::Panel::Values && state.row <= 4) state.column = std::min(1, state.column + 1);
        if (state.panel == Foundation::Panel::Moves) state.column = std::min(2, state.column + 1);
    }

    if (down & HidNpadButton_Y) {
        randomizeDVs(state);
        screen.postStatus("Authentic Gen II DVs randomized (0-15); HP/gender/shiny re-derived", 240);
    }
    if (down & HidNpadButton_A) activateField(screen, state);
    if ((down & HidNpadButton_X) && state.mode == Mode::Create) stageCreate(screen);

    if (down & HidNpadButton_B) {
        if (state.back()) closeToActions(screen);
        else screen.controllerNavigation.reset();
    }

    return true;
}

bool handleInput(TrainerViewScreen& screen, uint64_t down, uint64_t held,
                 int stickX, int stickY) {
    static const TouchInput noTouch{};
    return handleInput(screen, down, held, stickX, stickY, noTouch);
}

namespace {

void selectedRowBackground(PKSEFramebuffer& fb, int x, int y, int w, bool selected) {
    if (!selected) return;
    // Keep the normal dark row surface; selection is outline-only.
    fb.drawRoundedRect(x, y - 5, w, 28, 8, Colors::FocusBorder, 2);
}

Color statusColor(Rules::CompatibilityState status) {
    if (status == Rules::CompatibilityState::Ok) return Colors::Success;
    if (status == Rules::CompatibilityState::UnusualPreserved) return Colors::Warning;
    return Colors::Error;
}

void drawIdentity(PKSEFramebuffer& fb, const State& state, int x, int y, int w, int h) {
    drawPanelSurface(fb, x, y, w, h, false, 14);
    PokeBank::UIModel::PanelHeading::draw(fb, x, y, "DETAILS");
    const auto& p = state.working;
    const std::array<std::pair<std::string, std::string>, 8> rows{{
        {"Species", std::string(Names::getSpeciesName(p.species)) + " #" + std::to_string(p.species)},
        {"Nickname", p.nickname},
        {"Level", std::to_string(p.level)},
        {"EXP", std::to_string(p.experience)},
        {"OT", p.originalTrainer},
        {"Trainer ID", std::to_string(p.trainerId)},
        {"Gender", genderText(p.gender)},
        {"Held Item", itemText(p.heldItem)},
    }};
    for (int i = 0; i < static_cast<int>(rows.size()); ++i) {
        const int yy = y + 54 + i * 43;
        selectedRowBackground(fb, x + 10, yy, w - 20,
                              state.panel == Foundation::Panel::Identity && state.row == i);
        fb.drawText(x + 18, yy, rows[static_cast<std::size_t>(i)].first,
                    state.panel == Foundation::Panel::Identity && state.row == i
                        ? Colors::SelectedText : Colors::TextDim, TextStyle::Caption);
        fb.drawText(x + 110, yy, rows[static_cast<std::size_t>(i)].second, Colors::Text, TextStyle::Caption);
    }
    fb.drawText(x + 18, y + h - 48, "Gen II format fields only", Colors::TextDim, TextStyle::Caption);
    fb.drawText(x + 18, y + h - 25, "Nature / Ability / Ribbons / Marks hidden", Colors::TextDim, TextStyle::Caption);
}

void drawValues(PKSEFramebuffer& fb, const State& state, int x, int y, int w, int h) {
    drawPanelSurface(fb, x, y, w, h, false, 14);
    fb.drawText(x + 14, y + 10, "VALUES",
                state.panel == Foundation::Panel::Values ? Colors::Accent : Colors::Text,
                TextStyle::Heading);
    fb.drawText(x + 120, y + 44, "DV", Colors::TextDim, TextStyle::Caption);
    fb.drawText(x + 190, y + 44, "Stat Exp", Colors::TextDim, TextStyle::Caption);
    fb.drawText(x + 305, y + 44, "Stat", Colors::TextDim, TextStyle::Caption);

    const auto& p = state.working;
    const auto battle = Gen2::calculateBattleStats(p.species, p.level, editableDVs(p), p.statExperience).asArray();
    const std::array<const char*, 5> labels{"HP", "Attack", "Defense", "Speed", "Special"};
    const std::array<uint8_t, 5> dvs{p.dvs[0], p.dvs[1], p.dvs[2], p.dvs[3], p.dvs[4]};
    for (int i = 0; i < 5; ++i) {
        const int yy = y + 78 + i * 45;
        selectedRowBackground(fb, x + 10, yy, w - 20,
                              state.panel == Foundation::Panel::Values && state.row == i);
        fb.drawText(x + 18, yy, labels[static_cast<std::size_t>(i)],
                    state.panel == Foundation::Panel::Values && state.row == i
                        ? Colors::SelectedText : Colors::TextDim, TextStyle::Caption);
        const Color dvColor = Colors::Text;
        const Color statExpColor = Colors::Text;
        fb.drawText(x + 116, yy, std::to_string(dvs[static_cast<std::size_t>(i)]) + (i == 0 ? " *" : ""),
                    dvColor, TextStyle::Caption);
        fb.drawText(x + 190, yy, std::to_string(p.statExperience[static_cast<std::size_t>(i)]),
                    statExpColor, TextStyle::Caption);
        // Round 6 7A: the same Calculated cell model as every Gen II STATS painter (no slash).
        const auto cell = PokeBank::UIModel::Gen2Workspace::calculatedCell(battle, i);
        fb.drawText(x + 286, yy, cell.tag.empty() ? cell.value : cell.tag + " " + cell.value,
                    Colors::Text, TextStyle::Caption);
    }

    const std::array<std::pair<std::string, std::string>, 3> extras{{
        {"Shiny", p.shiny ? "Yes" : "No"},
        {"Friendship", std::to_string(p.friendship)},
        {"Pokerus", pokerusText(p.pokerus)},
    }};
    for (int i = 0; i < 3; ++i) {
        const int row = 5 + i;
        const int yy = y + 314 + i * 48;
        selectedRowBackground(fb, x + 10, yy, w - 20,
                              state.panel == Foundation::Panel::Values && state.row == row);
        fb.drawText(x + 18, yy, extras[static_cast<std::size_t>(i)].first,
                    state.panel == Foundation::Panel::Values && state.row == row
                        ? Colors::SelectedText : Colors::TextDim, TextStyle::Caption);
        fb.drawText(x + 130, yy, extras[static_cast<std::size_t>(i)].second,
                    ::UI::ClassicFieldColor::shinyValueColor(state.panel == Foundation::Panel::Values && state.row == row),
                    TextStyle::Caption);
    }
    fb.drawText(x + 18, y + h - 27, "* HP DV derived; one Special DV feeds SpA + SpD",
                Colors::TextDim, TextStyle::Caption);
}

void drawMoves(PKSEFramebuffer& fb, const TrainerViewScreen& screen, const State& state,
               int x, int y, int w, int h) {
    drawPanelSurface(fb, x, y, w, h, false, 14);
    PokeBank::UIModel::PanelHeading::draw(fb, x, y, "MOVES");
    fb.drawText(x + 198, y + 43, "PP", Colors::TextDim, TextStyle::Caption);
    fb.drawText(x + 260, y + 43, "Ups", Colors::TextDim, TextStyle::Caption);
    for (int slot = 0; slot < 4; ++slot) {
        const int yy = y + 76 + slot * 50;
        selectedRowBackground(fb, x + 10, yy, w - 20,
                              state.panel == Foundation::Panel::Moves && state.row == slot);
        const uint8_t move = state.working.moves[static_cast<std::size_t>(slot)];
        const std::string name = move == 0 ? "Empty" : std::string(Names::getMoveName(move));
        const auto status = moveStatus(screen, state, slot);
        const bool selected = state.panel == Foundation::Panel::Moves && state.row == slot;
        fb.drawText(x + 18, yy, name, Colors::Text,
                    TextStyle::Caption);
        fb.drawText(x + 198, yy, std::to_string(state.working.pp[static_cast<std::size_t>(slot)]),
                    Colors::Text, TextStyle::Caption);
        fb.drawText(x + 260, yy, std::to_string(state.working.ppUps[static_cast<std::size_t>(slot)]),
                    Colors::Text, TextStyle::Caption);
        fb.drawText(x + 310, yy, Rules::compatibilityText(status), statusColor(status), TextStyle::Caption);
    }

    const auto* personal = Gen2::personalRecord(state.working.species);
    const auto battle = Gen2::calculateBattleStats(state.working.species, state.working.level,
                                                    editableDVs(state.working), state.working.statExperience).asArray();
    fb.drawFilledRect(x + 14, y + 286, w - 28, 1, Colors::Divider);
    fb.drawText(x + 18, y + 304, "GEN II DATA", Colors::Accent, TextStyle::Caption);
    fb.drawText(x + 18, y + 332, "Game", Colors::TextDim, TextStyle::Caption);
    fb.drawText(x + 105, y + 332,
                screen.sourceGameId == "gold_gbc" ? "Gold" : screen.sourceGameId == "silver_gbc" ? "Silver" : "Crystal",
                Colors::Text, TextStyle::Caption);
    fb.drawText(x + 18, y + 356, "Record", Colors::TextDim, TextStyle::Caption);
    fb.drawText(x + 105, y + 356, state.mode == Mode::Create ? "Create draft" : "Staged box",
                Colors::Text, TextStyle::Caption);
    fb.drawText(x + 18, y + 380, "Growth", Colors::TextDim, TextStyle::Caption);
    fb.drawText(x + 105, y + 380, personal ? Rules::growthGroupName(personal->experienceGrowth) : "Unknown",
                Colors::Text, TextStyle::Caption);
    const uint32_t nextExp = personal && state.working.level < 100
        ? Pokemon::getExpForLevel(static_cast<uint8_t>(state.working.level + 1), personal->experienceGrowth)
        : state.working.experience;
    const uint32_t remaining = nextExp > state.working.experience ? nextExp - state.working.experience : 0;
    fb.drawText(x + 18, y + 404, "EXP next", Colors::TextDim, TextStyle::Caption);
    fb.drawText(x + 105, y + 404, std::to_string(remaining), Colors::Text, TextStyle::Caption);
    if (const auto caps = PokeBank::UIModel::PokemonEditorFoundation::capabilitiesForSourceId(screen.sourceGameId);
        caps && caps->supportsCrystalCaughtData) {
        fb.drawText(x + 18, y + 428, "Caught/Met", Colors::TextDim, TextStyle::Caption);
        fb.drawText(x + 105, y + 428, std::to_string(state.working.caughtData), Colors::Text, TextStyle::Caption);
    }
    StatsRadar::drawGen2Labeled(fb, x + w - 175, y + 305, 160, 150, battle);
}

void drawHeldItemPicker(TrainerViewScreen& screen, PKSEFramebuffer& fb, const State& state) {
    namespace Grid = PokeBank::UIModel::SharedHeldItemPicker;
    const int heldColumns=Grid::columnsForSearch(state.heldItemSearch.open());
    const int W = fb.getWidth(), H = fb.getHeight();
    constexpr int w = Grid::modalWidth, h = Grid::modalHeight;
    const int x = (W - w) / 2;
    const int y = (H - kNavBarH - h) / 2;
    // L06: the modal owns the frame's touch; covered workspace rows must not shadow its cells.
    UI::SharedEditorChrome::beginModal(screen.touchButtons);
    SharedPopupDismiss::publish(x, y, w, h);   // 7D: a tap outside closes it like B
    fb.drawFilledRect(0, 0, W, H, Color(0, 0, 0, 165));
    fb.drawFilledRoundedRect(x, y, w, h, 18, Colors::Panel);
    fb.drawRoundedRect(x, y, w, h, 18, Colors::Divider, 1);
    SharedHeldItemPickerPresentation::drawHeading(
        fb, x, y, "Held Item — Generation II",
        state.itemChoiceIndex, state.heldItemSearch.noMatches?0:static_cast<int>(state.itemChoices.size()),state.heldItemSearch.open());
    namespace Model = PokeBank::UIModel::Gen2HeldItemPicker;
    fb.drawText(x + w - 250, y + 55,
                std::string("Sort: ") + ItemPickerSort::label(state.itemSort),
                Colors::TextDim, TextStyle::Caption);
    if (const std::string tag = ItemPickerSearch::tag(state.itemQuery); !tag.empty()) {
        // NEW-06: the active search, top right on the title line.
        int tw = 0, th = 0;
        fb.measureText(tag, tw, th, TextStyle::Caption);
        fb.drawText(x + w - 28 - tw, y + 26, tag, Colors::Accent, TextStyle::Caption);
    }
    // R2-BUG-005: a focused Gen II TM/HM shows its move (the GSC machine table is shared by
    // Gold, Silver and Crystal; ClassicInventoryCatalog, pret/pokecrystal).
    if (state.itemChoiceIndex >= 0 && state.itemChoiceIndex < static_cast<int>(state.itemChoices.size()))
        UI::SharedItemGridPicker::drawMachineDetail(fb, x + 150, y + 55, w - 420, Enums::GameVersion::GSC,
            PokeVault::Inventory::classicMachineMove(PokeVault::Inventory::ClassicGame::Gold,
                state.itemChoices[static_cast<std::size_t>(state.itemChoiceIndex)]));
    const int count = static_cast<int>(state.itemChoices.size());
    const int firstRow = state.itemViewport.firstRow;
    const int liveOffset = state.itemScroll.offset();
    Gen2HeldItemPickerPresentation::drawList(fb, x, y, state.itemChoices, state.itemChoiceIndex,
                                             firstRow, liveOffset,&state.heldItemSearch);
    const int gridX = x + Grid::gridInsetX, gridY = y + Grid::gridInsetY;
    const int cellW = Grid::cellWidth;
    for (int i = std::max(0, (firstRow - 1) * heldColumns);
         !state.heldItemSearch.noMatches && i < std::min(count, (firstRow + Model::rows + 1) * heldColumns); ++i) {
        const int off = i - firstRow * heldColumns;
        const int cellX = gridX + (off % heldColumns) * cellW;
        const int cellY = gridY + (off / heldColumns) * Grid::rowPitch + liveOffset;
        appendClippedTouchButton(screen.touchButtons, 3000 + i,
            cellX, cellY - 3, cellW - 10, Grid::rowPitch - 4,
            gridX, gridY - 3, Grid::gridWidth(state.heldItemSearch.open()),
            Grid::rows * Grid::rowPitch);
    }
    PokeBank::UIModel::SharedEditorPresentation::PickerFooterSpec footer;
    footer.gridPaging = true;
    footer.xLabel = "Sort";
    footer.minusLabel = "Search";   // NEW-06
    if(state.heldItemSearch.open() || state.heldItemSearch.noMatches)
        SharedHeldItemSearch::drawFooter(fb,state.heldItemSearch);
    else
        UI::SharedEditorChrome::drawPickerFooter(fb,footer);
}

void drawConfirm(TrainerViewScreen& screen, PKSEFramebuffer& fb, const State& state) {
    // Reuse PKSE's native creator/details confirmation chrome. Generation II keeps its
    // existing PokeBank staging semantics underneath; this changes presentation/input only.
    // The dialog owns the frame's touch and draws its own footer (the shared confirm footer
    // from Gen2WorkspaceLayers), so nothing underneath it stays tappable.
    UI::SharedEditorChrome::beginModal(screen.touchButtons);
    const bool create = state.mode == Mode::Create;
    const int w = create ? 540 : 560;
    constexpr int h = 226;
    const int x = (fb.getWidth() - w) / 2;
    const int y = (fb.getHeight() - h) / 2;
    const char* title = create ? "New Pokémon" : "Unsaved changes";
    const Color accent = create ? Colors::Primary : Colors::Orange;
    const int cy = Dialogs::drawDialogFrame(fb, x, y, w, h, title, accent);

    fb.drawText(x + 28, cy,
                create ? "Keep this new Pokémon?" : "This Pokémon has unsaved changes.",
                Colors::Text);
    fb.drawText(x + 28, cy + 34,
                create ? "Discard removes it; Back keeps editing."
                       : "Discard loses them; Back keeps editing.",
                Colors::TextDim, TextStyle::Caption);

    const int cbw = 160, cbh = TouchTargetMin, cby = y + h - cbh - 18;
    drawGlyphButton(fb, x + 24, cby, cbw, cbh, "B", "Back", Colors::PanelAlt);
    drawGlyphButton(fb, x + (w - cbw) / 2, cby, cbw, cbh, "Y", "Discard", Colors::PanelAlt);
    drawGlyphButton(fb, x + w - 24 - cbw, cby, cbw, cbh, "A",
                    create ? "Keep" : "Save", Colors::PanelAlt);
    namespace Layers = PokeBank::UIModel::Gen2WorkspaceLayers;
    const auto footer = Layers::footer(Layers::Layer::ExitConfirm,
                                       create ? Layers::Session::Create : Layers::Session::Edit);
    std::vector<ControllerHint> hints;
    for (std::size_t i = 0; i < footer.count; ++i)
        hints.push_back({footer.hints[i].glyph, footer.hints[i].label});
    drawNavBar(fb, hints);
}

} // namespace

void drawOverlay(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    auto& state = stateFor(screen);
    if (state.mode == Mode::None) return;
    if (state.mode == Mode::View) {
        const Pokemon::Pokemon2ReadOnly pokemon(state.working);
        Modals::drawGen2PokemonDetailsModal(screen, fb, pokemon);
        return;
    }
    const int W = fb.getWidth(), H = fb.getHeight();
    fb.drawFilledRect(0, 0, W, H, Color(0, 0, 0, 135));
    fb.drawVerticalGradient(0, 0, W, H,
        Color(Colors::Background.r, Colors::Background.g, Colors::Background.b, 250),
        Color(Colors::Background.r, Colors::Background.g, Colors::Background.b, 255));

    constexpr int workspaceX = 34, workspaceY = 25, workspaceW = 1212, workspaceH = 620;
    drawPanelSurface(fb, workspaceX, workspaceY, workspaceW, workspaceH, false, 18);
    const std::string title = state.mode == Mode::Create ? "Create Pokemon — GEN II DRAFT"
                                                         : "Edit Pokemon — GEN II STAGED";
    fb.drawText(workspaceX + 24, workspaceY + 14, title, Colors::Text, TextStyle::Heading);
    // Shared source-game context line (Gen III-V shape): Box / Slot • <Game> • Source save immutable • mode.
    // A Create draft has no slot until it is added (the staged box assigns the next one).
    namespace GameContext = PokeBank::UIModel::ClassicGameContext;
    std::string record = state.party?"Party "+std::to_string(state.slot+1):"Box " + std::to_string(state.box + 1);
    if (!state.party && state.slot >= 0) record += " / Slot " + std::to_string(state.slot + 1);
    fb.drawText(workspaceX + 24, workspaceY + 50,
                GameContext::contextLine(record, screen.sourceGameId,
                                         state.mode == Mode::Create ? GameContext::SurfaceMode::CreateDraft
                                                                    : GameContext::SurfaceMode::EditStaged),
                Colors::TextDim, TextStyle::Caption);

    constexpr int contentY = workspaceY + 78, contentH = 520;
    constexpr int leftX = workspaceX + 16, leftW = 340;
    constexpr int midX = leftX + leftW + 12, midW = 410;
    constexpr int rightX = midX + midW + 12;
    constexpr int rightW = workspaceX + workspaceW - 16 - rightX;
    drawIdentity(fb, state, leftX, contentY, leftW, contentH);
    drawValues(fb, state, midX, contentY, midW, contentH);
    drawMoves(fb, screen, state, rightX, contentY, rightW, contentH);

    if (state.mode == Mode::Create)
        drawNavBar(fb, {{"D-pad/Stick", "Navigate"}, {"L/R", "Panel"}, {"A", "Edit"},
                        {"Y", "Random DVs"}, {"X", "Add"}, {"B", "Cancel"}});
    else
        drawNavBar(fb, {{"D-pad/Stick", "Navigate"}, {"L/R", "Panel"}, {"A", "Edit"},
                        {"Y", "Random DVs"}, {"B", "Back"}});

    if (state.itemPicker) drawHeldItemPicker(screen, fb, state);
    if (state.confirmExit) drawConfirm(screen, fb, state);
}

} // namespace UI::Gen2PokemonEditor
