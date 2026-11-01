#include "UI/ClassicSourceGuard.h"
#include "UI/ReviewEntry.h"
#include "UI/SharedViewOnlyNotice.h"
#include "UI/SharedPanelHeading.h"
#include "UI/SharedAbilityPicker.h"
#include "Integration/Gen3/Gen3HeldItems.h"
#include "UI/SharedItemGridPicker.h"
#include "Inventory/ClassicInventoryCatalog.h"
#include "UI/SharedPokemonShell.h"
#include "UI/ClassicTrainerSessionInput.h"
#include "UI/ClassicReviewOrigin.h"
#include "Pokemon/PersonalInfoTable.h"
#include "Encryption/Encryption3FRLG.h"
#include <cstring>
#include "UI/Gen13PickerPaging.h"
#include "Integration/Gen3/Gen3ExactFormatEditorProvider.h"
#include "Integration/Gen3/Gen3ExportTransaction.h"
#include "Integration/Gen3/Gen3LearnsetTable.h"
#include "Integration/Encounter/EncounterGuardrails.h"
#include "Legacy/FRLGReadOnlyTrainer.h"
#include "Enums/Ball.h"
#include "Enums/GameVersion.h"
#include "Enums/LanguageID.h"
#include "Names/ItemNames.h"
#include "Names/MachineDisplay.h"
#include "Names/LocationDisplayNames.h"
#include "Names/LocationNames.h"
#include "Names/MoveInfo.h"
#include "Names/MoveNames.h"
#include "Names/SpeciesNames.h"
#include "Trainer/Trainer.h"
#include "UI/ClassicGameContext.h"
#include "UI/Common.h"
#include "UI/ScreenChrome.h"
#include "UI/Dialogs/DialogFrame.h"
#include "UI/Gen3SharedPokemonSession.h"
#include "UI/Gen3EncounterPickerModel.h"
#include "UI/Gen4BoxTouchActions.h"
#include "UI/MovePickerPresentation.h"
#include "UI/MovePickerRowUI.h"
#include "UI/NaturePickerLayout.h"
#include "UI/NatureStatPresentation.h"
#include "UI/ItemPickerArtwork.h"
#include "UI/ItemPickerSort.h"
#include "UI/ItemPickerSearch.h"
#include "UI/AbilityEffects.h"
#include "UI/SharedPokemonEditorContract.h"
#include "UI/SharedEditorChrome.h"
#include "UI/SharedBoxedRows.h"
#include "UI/SharedEditorPresentation.h"
#include "UI/SharedMovesPanel.h"
#include "UI/SharedStatReroll.h"
#include "UI/EditFeedback.h"
#include "Utils/HelperUtilities.h"
#include "UI/SharedHeldItemPicker.h"
#include "UI/SharedHeldItemInput.h"
#include "UI/SharedHeldItemPickerPresentation.h"
#include "UI/SharedSpeciesPicker.h"
#include "UI/TouchScroll.h"
#include "Utils/FileUtilities.h"
#include "Utils/Keyboard.h"
#include "Utils/PokeBankPaths.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace UI::Gen3SharedEditorSurface {
namespace {

namespace Shared = PokeBank::UIModel::SharedPokemonEditor;
namespace HeldItemGrid = PokeBank::UIModel::SharedHeldItemPicker;
namespace SessionModel = PokeBank::UIModel::Gen3SharedEditor;
namespace Gen3 = PokeVault::Integration::Gen3;
namespace Encounter = PokeVault::Integration::EncounterGuardrails;
namespace EncounterPicker = PokeBank::UIModel::Gen3EncounterPicker;
namespace MetRows = PokeBank::UIModel::ExactGameMetLocations;
namespace Context = PokeBank::UIModel::ClassicGameContext;
namespace MoveUI = PokeBank::UIModel::MovePickerPresentation;
namespace Presentation = PokeBank::UIModel::SharedEditorPresentation;
// Gen III move facts (RSE and FRLG share one move table and the FRLG group key here).
constexpr Enums::GameVersion kGen3MoveGame = Enums::GameVersion::FRLG;

enum class PickerTarget : uint8_t {
    None,
    HeldItem,
    Language,
    Ball,
    MetLocation,
    Nature,
    Gender,
    Ability,
    Move,
    Form,
};

struct State {
    const TrainerViewScreen* owner = nullptr;
    uint64_t ownerSerial = 0;   // TrainerViewScreen::instanceSerial (never reused, unlike the address)
    bool passive = false;
    int detailsFocus = 0;
    TouchScrollState detailsTouchScroll;
    TouchPickerViewport detailsViewport;
    SharedScrollbar::State detailsScrollbar;   // draggable thumb beside the Details rows
    bool previewValid = false;
    Gen3::StagedPokemonRecord previewInput{};
    Gen3::StagedPokemonRecord previewRecord{};
    std::string previewError;
    bool actions = false;
    bool review = false;
    int reviewScroll = 0;
    TouchScrollState reviewTouchScroll;
    SharedScrollbar::State reviewScrollbar;    // draggable thumb on Pending Changes
    bool legality = false;
    bool confirmRelease = false;
    // Discard All in Review is a two-step Y -> Y confirmation; B keeps the staged edits.
    bool confirmDiscardAll = false;
    bool speciesPicker = false;
    SharedSpeciesPicker::Chooser speciesChooser;   // R2-SEARCH: live species filter + key grid
    bool valuePicker = false;
    PickerTarget pickerTarget = PickerTarget::None;
    int pickerRow = 0;
    TouchScrollState pickerScroll;
    TouchPickerViewport pickerViewport;
    SharedScrollbar::State pickerScrollbar;    // draggable thumb on the value / move picker list
    int pickerMoveSlot = 0;
    bool moveEditor = false;
    int moveEditorRow = 0;
    int moveEditorSlot = 0;
    uint16_t moveBaseline = 0;
    uint8_t movePpBaseline = 0;
    uint8_t movePpUpsBaseline = 0;
    std::vector<uint16_t> pickerValues;
    std::vector<uint16_t> pickerNativeValues;
    ItemPickerSort::Mode pickerSort = ItemPickerSort::Mode::Native;
    std::string pickerQuery;   // NEW-06: Held Item search (narrows the presentation copy only)
    SharedHeldItemSearch::State heldItemSearch;
    std::vector<Encounter::EncounterProvenanceChoice> encounterChoices;
    std::optional<Encounter::EncounterProvenanceChoice> selectedEncounter;
    int actionRow = 0;
    int box = 0;
    int slot = 0;
    uint16_t speciesPreview = 25;
    bool speciesPreviewShiny = false;
    Shared::Focus focus{};
    SessionModel::Session session{};
    bool party = false;
};

State& stateFor(const TrainerViewScreen& screen) {
    static State state;
    if ((state.owner != &screen || state.ownerSerial != screen.instanceSerial)) {
        state = {};
        state.owner = &screen; state.ownerSerial = screen.instanceSerial;
    }
    return state;
}

bool isGen3Gba(const TrainerViewScreen& screen) noexcept {
    return screen.sourceGameId == "ruby_gba" ||
           screen.sourceGameId == "sapphire_gba" ||
           screen.sourceGameId == "emerald_gba" ||
           screen.sourceGameId == "firered_gba" ||
           screen.sourceGameId == "leafgreen_gba";
}

PokeVault::Legacy::FRLGReadOnlyTrainer& bridge(TrainerViewScreen& screen) {
    return static_cast<PokeVault::Legacy::FRLGReadOnlyTrainer&>(screen.trainer);
}

const PokeVault::Legacy::FRLGReadOnlyTrainer& bridge(const TrainerViewScreen& screen) {
    return static_cast<const PokeVault::Legacy::FRLGReadOnlyTrainer&>(screen.trainer);
}

Gen3::StagedPokemonEditor* editor(TrainerViewScreen& screen) noexcept {
    return isGen3Gba(screen) ? bridge(screen).stagedPokemon() : nullptr;
}

// One Review row: a staged Pokemon change, or a staged Trainer Name / Money change (R2-BUG-002).
struct ReviewRow {
    std::string label;
    std::string beforeValue;
    std::string afterValue;
    bool createdDetail = false;   // CG74: shown as the value alone
};

std::vector<ReviewRow> reviewRows(TrainerViewScreen& screen) {
    std::vector<ReviewRow> rows;
    if (!isGen3Gba(screen)) return rows;
    if (const auto* trainer = bridge(screen).stagedTrainer())
        for (const auto& change : trainer->pendingChanges())
            rows.push_back({change.label, change.before, change.after});
    if (const auto* staged = editor(screen))
        for (const auto& change : staged->pendingChanges())
            rows.push_back({change.label, change.beforeValue, change.afterValue, change.createdDetail});
    if (const auto* inventory = bridge(screen).stagedInventory())
        for (const auto& change : inventory->pendingChanges())
            rows.push_back({change.label, std::to_string(change.before), std::to_string(change.after)});
    return rows;
}


bool workspaceActive(const State& state) noexcept {
    return state.session.mode != SessionModel::Mode::None;
}

bool ownsFrameImpl(const TrainerViewScreen& screen) noexcept {
    if (!isGen3Gba(screen)) return false;
    const auto& state = stateFor(screen);
    return state.actions || state.review || state.legality || state.confirmRelease ||
           state.speciesPicker || state.valuePicker || workspaceActive(state);
}

bool boxEntrySurface(const TrainerViewScreen& screen) noexcept {
    return isGen3Gba(screen) &&
           screen.selectedMode == TrainerViewScreen::ViewMode::Boxes &&
           screen.detailViewActive &&
           screen.selectedBoxIndex >= 0 &&
           screen.selectedItemIndex >= 0 &&
           !screen.helpOverlayActive &&
           !screen.details.active &&
           !screen.actionSheet.isOpen() &&
           !screen.saveConfirmActive &&
           !screen.pickerActive &&
           !screen.itemEditDialogActive &&
           !screen.carrying() &&
           !screen.swapActive &&
           !screen.currentlySelecting;
}

// This is a GAME-SAVE Storage pane only. Bank pane actions remain owned by
// the bank model and cannot use the staged native PC Pokemon editor.
bool storageEntrySurface(const TrainerViewScreen& screen) noexcept {
    return isGen3Gba(screen) &&
        screen.selectedMode == TrainerViewScreen::ViewMode::Storage &&
        screen.detailViewActive && screen.storageFocusPane == 0 &&
        screen.stSaveBox >= 0 && screen.stSaveBox < static_cast<int>(screen.trainer.getBoxCount()) &&
        screen.stSaveSlot >= 0 && screen.stSaveSlot < static_cast<int>(screen.trainer.getSlotsPerBox()) &&
        !screen.helpOverlayActive && !screen.details.active && !screen.actionSheet.isOpen() &&
        !screen.saveConfirmActive && !screen.pickerActive && !screen.itemEditDialogActive &&
        !screen.carrying() && !screen.currentlySelecting;
}

bool selectedRecord(TrainerViewScreen& screen, Gen3::StagedPokemonRecord& record,
                    std::string& error) {
    auto* staged = editor(screen);
    if (!staged) {
        error = bridge(screen).stagedPokemonUnavailableReason();
        if (error.empty()) error = "Generation III staged Pokemon editor is unavailable";
        return false;
    }
    const auto value = staged->boxedPokemon(
        static_cast<std::size_t>(screen.selectedBoxIndex),
        static_cast<std::size_t>(screen.selectedItemIndex), error);
    if (!value) return false;
    record = *value;
    return true;
}

bool selectedOccupied(TrainerViewScreen& screen) {
    if (!boxEntrySurface(screen) && !storageEntrySurface(screen)) return false;
    auto* staged = editor(screen);
    if (!staged) return false;
    std::string error;
    const bool storage = storageEntrySurface(screen);
    return staged->boxedPokemon(
        static_cast<std::size_t>(storage ? screen.stSaveBox : screen.selectedBoxIndex),
        static_cast<std::size_t>(storage ? screen.stSaveSlot : screen.selectedItemIndex),
        error).has_value();
}

void resetWorkspaceFocus(State& state) noexcept {
    state.focus = {Shared::Panel::Details, 0, 0};
    state.speciesPicker = false;
    state.speciesPreviewShiny = false;
    state.valuePicker = false;
    state.pickerTarget = PickerTarget::None;
    state.pickerValues.clear();
    state.encounterChoices.clear();
    state.selectedEncounter.reset();
    state.moveEditor = false;
    state.moveEditorRow = 0;
    state.detailsFocus = 0;
    state.detailsTouchScroll.reset();
    state.detailsViewport.reset();
    state.previewValid = false;
}

void beginActions(TrainerViewScreen& screen) {
    auto& state = stateFor(screen);
    state.actions = true;
    state.review = false;
    state.reviewScroll = 0;
    state.reviewTouchScroll.reset();
    state.legality = false;
    state.confirmRelease = false;
    state.speciesPicker = false;
    state.valuePicker = false;
    state.pickerTarget = PickerTarget::None;
    state.pickerValues.clear();
    state.actionRow = 0;
    const bool storage = storageEntrySurface(screen);
    state.box = storage ? screen.stSaveBox : screen.selectedBoxIndex;
    state.slot = storage ? screen.stSaveSlot : screen.selectedItemIndex;
    state.party = screen.selectedMode==TrainerViewScreen::ViewMode::Party;
    if(state.party) {state.box=0;state.slot=screen.selectedPartyIndex;}
    screen.controllerNavigation.reset();
}

bool beginViewOrEdit(TrainerViewScreen& screen, SessionModel::Mode mode) {
    auto& state = stateFor(screen);
    auto* staged = editor(screen);
    if (!staged) return false;
    std::string error;
    auto pokemon = state.party?staged->partyPokemon(static_cast<size_t>(state.slot),error):
        staged->boxedPokemon(static_cast<std::size_t>(state.box),
                                        static_cast<std::size_t>(state.slot), error);
    if (!pokemon) {
        screen.postStatus(error.empty() ? "Selected Generation III slot is empty" : error, 300);
        return false;
    }
    state.passive = false;
    state.session.begin(*pokemon, mode);
    state.actions = false;
    state.review = false;
    state.legality = false;
    resetWorkspaceFocus(state);
    return true;
}

bool beginCreate(TrainerViewScreen& screen, int box, int slot) {
    auto* staged = editor(screen);
    if (!staged) {
        const auto& why = bridge(screen).stagedPokemonUnavailableReason();
        screen.postStatus(why.empty() ? "Generation III staged Pokemon editor unavailable" : why, 360);
        return false;
    }
    if (box < 0 || slot < 0 || box >= staged->boxCount() || slot >= staged->boxCapacity()) {
        screen.postStatus("Select a native Generation III box slot first");
        return false;
    }
    std::string error;
    if (staged->boxedPokemon(static_cast<std::size_t>(box), static_cast<std::size_t>(slot), error)) {
        screen.postStatus("Add Pokemon requires an empty Generation III slot");
        return false;
    }
    if (!error.empty()) {
        screen.postStatus(error);
        return false;
    }

    auto& state = stateFor(screen);
    state.box = box;
    state.slot = slot;
    state.party = false;
    state.actions = false;
    state.review = false;
    state.legality = false;
    state.confirmRelease = false;
    state.passive = false;
    state.session.beginCreate(staged->trainer(), staged->originGame());
    resetWorkspaceFocus(state);
    return true;
}

void refreshPresentation(TrainerViewScreen& screen) {
    std::string error;
    if (!bridge(screen).refreshStagedPokemonPresentation(error))
        screen.postStatus(error.empty() ? "Could not refresh staged Generation III boxes" : error, 360);
}

bool keepWorkspace(TrainerViewScreen& screen) {
    auto& state = stateFor(screen);
    auto* staged = editor(screen);
    if (!staged) return false;
    std::string error;
    bool ok = false;
    if (state.session.mode == SessionModel::Mode::Edit) {
        ok = state.session.keep(*staged, static_cast<std::size_t>(state.box),
                                static_cast<std::size_t>(state.slot), error,state.party);
    } else if (state.session.mode == SessionModel::Mode::Create) {
        ok = state.session.add(*staged, static_cast<std::size_t>(state.box),
                               static_cast<std::size_t>(state.slot), error);
    }
    if (!ok) {
        screen.postStatus(error.empty() ? "Generation III staged Pokemon operation failed" : error, 360);
        return false;
    }
    refreshPresentation(screen);
    if(state.party) screen.selectedPartyIndex=state.slot;
    else {screen.selectedBoxIndex = state.box;screen.selectedItemIndex = state.slot;}
    screen.postStatus("Generation III Pokemon change staged; source save unchanged", 300);
    return true;
}

const char* genderText(uint8_t gender) noexcept {
    return gender == 0 ? "Male" : gender == 1 ? "Female" : "Genderless";
}

std::string exactOriginText(const TrainerViewScreen&,
                            const Gen3::StagedPokemonRecord& p) {
    return Enums::getOriginGameName(p.originGame);
}

std::string metLocationText(const Gen3::StagedPokemonRecord& p) {
    // Player-facing name: no raw id, no PKHeX source tags (Names/LocationDisplayNames.h).
    return Names::getMetLocationDisplayName(p.originGame, p.metLocation);
}

// The Met Location picker is wider than the other compact pickers so the place name and its
// method / level fit on one readable row.
constexpr int kMetLocationPickerWidth = 620;
static_assert(PokeBank::UIModel::SharedMenuGeometry::rule(
    PokeBank::UIModel::SharedMenuGeometry::MenuType::LocationPicker).width == kMetLocationPickerWidth);

std::string encounterOriginText(const TrainerViewScreen&,
                                const Gen3::StagedPokemonRecord& p) {
    if (p.metLocation == 0) return "Unknown";
    // Round 11 FRLG: the record's own origin game's catch templates (not the save's). A location
    // with no template is valid trade / gift / hatch history the data cannot confirm.
    const auto choices = EncounterPicker::templatesForOrigin(p.originGame, p.species);
    const Encounter::EncounterProvenanceChoice* selected = nullptr;
    for (const auto& choice : choices) {
        if (choice.encounter.location != p.metLocation) continue;
        if (!selected) selected = &choice;
        if (choice.encounter.containsLevel(p.metLevel)) { selected = &choice; break; }
    }
    if (!selected) return "Not a listed catch; unverified";
    if (!selected->evolved) return std::string(Names::getSpeciesName(p.species));
    return std::string(Names::getSpeciesName(selected->originalEncounterSpecies)) +
           " → " + Names::getSpeciesName(p.species);
}

Color genderValueColor(uint8_t gender) {
    if (gender == 0)
        return g_themeMode == ThemeMode::Light ? Color(24,112,184) : Color(92,205,255);
    if (gender == 1) return Colors::Magenta;
    return Colors::Text;
}

// Gen III stores a form only for Unown (PID-derived letter, Pokemon3FRLG::form). Deoxys' form
// is decided by the game it is in, not by the record (PKHeX PK3: Form is 0 except Unown).
std::string unownLetterName(uint8_t letter) {
    if (letter < 26) return std::string(1, static_cast<char>('A' + letter));
    return letter == 26 ? "!" : "?";
}
std::string gen3FormLabel(uint16_t species, uint8_t form) {
    if (species == 201) return "Unown " + unownLetterName(form);
    if (species == 386) return "Set by game";
    return "None";
}

std::string detailsValue(const TrainerViewScreen& screen,
                         const Gen3::StagedPokemonRecord& p, int row) {
    switch (row) {
        case 0: return std::string(Names::getSpeciesName(p.species)) + " #" + std::to_string(p.species);
        case 1: return p.nickname.empty() ? "(none)" : p.nickname;
        case 2: return std::to_string(p.level);
        case 3: return std::to_string(p.experience);
        case 4: return p.otName.empty() ? "(none)" : p.otName;
        case 5: return std::to_string(p.tid);
        case 6: return std::to_string(p.sid);
        // CG71: a stored Safari Ball / Berry Juice held item is kept and labelled honestly
        // ("unreleased as held item"); the picker never offers it.
        case 7: return std::string(Names::getItemNameG3(p.heldItem)) +
                       (Gen3::storedUnreleasedHeldItem(p.heldItem) ? Gen3::kUnreleasedHeldItemShortSuffix : "");
        case 8: return std::to_string(p.friendship);
        case 9: return Enums::getLanguageName(p.language);
        case 10: return exactOriginText(screen, p);
        case 11: return Enums::getBallName(p.ball);
        case 12: return std::to_string(p.metLevel);
        case 13: return metLocationText(p);
        case 14: return Presentation::pokerusLabel(Presentation::pokerusState(p.pokerus));
        case 15: return gen3FormLabel(p.species, p.form);
        default: return {};
    }
}

constexpr std::array<const char*,16> detailLabels{{
    "Species", "Nickname", "Level", "Experience", "OT", "Trainer ID", "SID",
    "Held Item", "Friendship", "Language", "Origin", "Ball",
    "Met Level", "Met Location", "Pokerus", "Form"
}};
static_assert(detailLabels.size() == Shared::layoutFor(Shared::Generation::Gen3).detailsRows);

// Locked in every mode (gold ReadOnly field state): SID, Origin, and Form except Unown's letter.
bool detailLocked(const Gen3::StagedPokemonRecord& p, int row) noexcept {
    return row == 6 || row == 10 || (row == 15 && p.species != 201);
}

bool detailEditable(const State& state, int row) noexcept {
    if (!state.session.editable()) return false;
    if (row == 15) return state.session.working.species == 201; // Unown letter (PID-derived)
    if (row == 1 || row == 2 || row == 3 || row == 4 || row == 5) return true;
    // L06 (Round 6): Pokérus (14) is editable in Create too, as in Gen IV. The Create backend
    // validates, writes and verifies create.pokerus (Gen3StagedPokemonEditor::stageAddBoxPokemon).
    if (state.session.mode == SessionModel::Mode::Create)
        return row == 0 || row == 1 || row == 2 || row == 7 ||
               row == 8 || row == 9 || row == 11 || row == 12 || row == 13 || row == 14;
    return row == 0 || row == 1 || row == 2 || row == 3 || row == 4 ||
           row == 7 || row == 8 || row == 9 || row == 11 || row == 12 ||
           row == 13 || row == 14;
}

bool variableGenderSpecies(uint16_t species) noexcept {
    const auto ratio = Pokemon::getPersonalInfo(species, 0).genderRatio;
    return ratio > 0 && ratio < 254;
}

bool pidLinkedRowEditable(const State& state, int row) noexcept {
    if (!state.session.editable()) return false;
    if (row == 6 || row == 8) return true; // Nature / Shiny
    if (row == 7) return variableGenderSpecies(state.session.working.species);
    if (row == 9) return true;
    return false; // PID itself is always derived/read-only.
}

// View (passive): A or a completed tap on a field says why nothing changes (owner Oct 25 L06),
// with the Gen IV / V wording (UI/SharedViewOnlyNotice.h). Rows Edit cannot change either (SID,
// Origin, a non-Unown Form, PID, a fixed Gender) never point at Edit.
void viewOnlyActivate(TrainerViewScreen& screen, const State& state) {
    namespace Notice = PokeBank::UIModel::ViewOnlyNotice;
    constexpr std::array<const char*,5> pidLabels{{"Nature", "Gender", "Shiny", "Ability", "PID"}};
    const auto& focus = state.focus;
    const int row = focus.row;
    std::string field;
    bool locked = false;
    if (focus.panel == Shared::Panel::Details) {
        if (row < 0 || row >= static_cast<int>(detailLabels.size())) return;
        field = detailLabels[static_cast<size_t>(row)];
        locked = row == 6 || row == 10 || (row == 15 && state.session.working.species != 201);
    } else if (focus.panel == Shared::Panel::Moves) {
        field = "moves";
    } else if (row < 6) {
        field = focus.column == 0 ? "IVs" : "EVs";
    } else if (row <= 10) {
        field = pidLabels[static_cast<size_t>(row - 6)];
        locked = row == 10 || (row == 7 && !variableGenderSpecies(state.session.working.species));
    } else {
        return;
    }
    screen.postStatus(locked ? Notice::lockedHint(field) : Notice::editHint(field), Notice::kFrames);
}

void normalizeEditableFocus(State& state, int verticalDirection = 0, bool panelChanged = false) {
    state.focus = Shared::normalizeMoveRowFocus(Shared::Generation::Gen3, state.focus);
    if (!state.session.editable()) return;

    if (state.focus.panel == Shared::Panel::Details) {
        if (detailEditable(state, state.focus.row)) return;
        int row = state.focus.row;
        const int direction = verticalDirection < 0 ? -1 : 1;
        for (int step = 0; step < static_cast<int>(detailLabels.size()); ++step) {
            row = (row + direction + static_cast<int>(detailLabels.size())) %
                  static_cast<int>(detailLabels.size());
            if (detailEditable(state, row)) {
                state.focus.row = static_cast<uint8_t>(row);
                state.focus.column = 0;
                return;
            }
        }
        return;
    }

    if (state.focus.panel == Shared::Panel::Values) {
        if (state.focus.row < 6) {
            if (state.focus.column >= 2) state.focus.column = 1;
            return;
        }

        state.focus.column = 0;
        if (pidLinkedRowEditable(state, state.focus.row)) return;

        int row = state.focus.row;
        const int direction = verticalDirection < 0 ? -1 : 1;
        for (int step = 0; step < 5; ++step) {
            row += direction;
            if (row < 6) row = 10;
            if (row > 10) row = 6;
            if (pidLinkedRowEditable(state, row)) {
                state.focus.row = static_cast<uint8_t>(row);
                return;
            }
        }
        // If a panel transition landed on PID/fixed-only rows, choose the first
        // editable stat cell instead of trapping focus.
        if (panelChanged) state.focus = {Shared::Panel::Values, 0, 0};
        return;
    }

    if (state.focus.panel == Shared::Panel::Moves) {
        if (panelChanged && state.focus.row >= 4) state.focus.row = 0;
        state.focus.row = static_cast<uint8_t>(state.focus.row % 4);
        state.focus.column = 0;
    }
}

// One encounter choice as {place, "method · level [· origin species]"} for the picker row.
std::pair<std::string, std::string> metLocationChoiceText(const State& state, uint16_t value) {
    const auto& stored = state.session.working;
    if (value == EncounterPicker::kKeepStoredRow) {
        // R2-BUG-008: the save's own met location, not judged (traded / hatched / event / other).
        return {"Keep stored: " + Names::getMetLocationDisplayName(stored.originGame, stored.metLocation),
                "Met Lv. " + std::to_string(stored.metLevel)};   // the name already says "Keep stored"
    }
    if (MetRows::isOtherNativeRow(value)) {
        // Round 11 FRLG: a location native to the record's origin game where this species is not a
        // listed catch (or a trade / egg / event marker): honest, never presented as a catch.
        const auto location = MetRows::otherNativeRowLocation(value);
        return {Names::getMetLocationDisplayName(stored.originGame, location), MetRows::gen3OtherDetail(location)};
    }
    const auto* selected = EncounterPicker::choiceAt(state.encounterChoices, value);
    if (!selected) return {"Unknown location", {}};
    const auto& encounter = selected->encounter;
    std::string place = Names::getMetLocationDisplayName(state.session.working.originGame, encounter.location);
    // Round 5 NEW-10: short enough that every row fits the 620 px window on one line
    // (tests/test_met_location_fit.cpp).
    std::string detail = std::string(Encounter::methodName(encounter.method)) +
                         " Lv. " + std::to_string(encounter.minLevel);
    if (encounter.maxLevel != encounter.minLevel)
        detail += "–" + std::to_string(encounter.maxLevel);
    if (selected->evolved) {
        detail += ", met as " + std::string(Names::getSpeciesName(selected->originalEncounterSpecies));
        if (selected->minimumEvolutionLevel != 0)
            detail += " (evo. " + std::to_string(selected->minimumEvolutionLevel) + ")";
    }
    // R2-BUG-008: the save's own met data is the "Keep stored" row; candidate rows show only the
    // encounter itself (NEW-10 / Round 3 L03: no "not stored" filler on every other row).
    return {std::move(place), std::move(detail)};
}

std::string pickerValueLabel(const State& state, uint16_t value) {
    switch (state.pickerTarget) {
        case PickerTarget::HeldItem:
            // CG71: a stored unreleased item is listed only so it can be kept (the stored value is
            // appended); it carries the same honest label as the DETAILS row.
            return value == 0 ? "None" :
                Names::machineDisplayLabel(Enums::GameVersion::FRLG, value,
                                           Names::getItemNameG3(value)) +
                (Gen3::storedUnreleasedHeldItem(value) ? Gen3::kUnreleasedHeldItemShortSuffix : "");
        case PickerTarget::Language: return Enums::getLanguageName(static_cast<uint8_t>(value));
        case PickerTarget::Ball: return Enums::getBallName(static_cast<uint8_t>(value));
        case PickerTarget::Nature: return Trainer::getNatureName(static_cast<uint8_t>(value));
        case PickerTarget::Gender: return genderText(static_cast<uint8_t>(value));
        case PickerTarget::Ability: {
            const auto& personal = Pokemon::getPersonalInfoG3(state.session.working.species);
            const uint16_t ability = value == 2 ? personal.ability2 : personal.ability1;
            // NEW-13: the ability's name and short effect, not "Slot 1 / Slot 2".
            return UI::AbilityEffects::row(Trainer::getAbilityName(ability), ability, 3);
        }
        case PickerTarget::MetLocation: {
            const auto row = metLocationChoiceText(state, value);
            return row.second.empty() ? row.first : row.first + " — " + row.second;
        }
        case PickerTarget::Move:
            return MoveUI::rowLabel(value, Enums::GameVersion::FRLG);
        case PickerTarget::Form:
            return "Unown " + unownLetterName(static_cast<uint8_t>(value));
        default: return {};
    }
}

// Phase 2: ONE geometry for every editor value list (UI/SharedMenuGeometry.h). Ball, Met
// Location and the short choice lists (Language, Ability, Form, Gender, Pokerus) share their
// rows; draw and touch handling both read this layout, so they cannot drift apart.
PokeBank::UIModel::SharedMenuGeometry::Layout valueListLayout(PickerTarget target, int count) noexcept {
    namespace G = PokeBank::UIModel::SharedMenuGeometry;
    if(target==PickerTarget::Ability)return SharedAbilityPicker::layout(count);
    // Met Location retains its bounded source-context columns.
    return G::layout(target == PickerTarget::Ball ? G::MenuType::BallPicker
                     : target == PickerTarget::MetLocation
                         ? G::MenuType::LocationPicker
                         : G::MenuType::ChoicePicker, count);
}

const char* pickerTitle(PickerTarget target) noexcept {
    switch (target) {
        case PickerTarget::HeldItem: return "Choose Held Item";
        case PickerTarget::Language: return "Choose Language";
        case PickerTarget::Ball: return "Choose Ball";
        case PickerTarget::MetLocation: return "Choose Met Location";
        case PickerTarget::Nature: return "Choose Nature";
        case PickerTarget::Gender: return "Choose Gender";
        case PickerTarget::Ability: return "Choose Ability";
        case PickerTarget::Move: return "Choose Move";
        case PickerTarget::Form: return "Choose Unown Letter";
        default: return "Choose Value";
    }
}

uint16_t currentPickerValue(const State& state, PickerTarget target, int moveSlot = 0) noexcept {
    const auto& p = state.session.working;
    switch (target) {
        case PickerTarget::HeldItem: return p.heldItem;
        case PickerTarget::Language: return p.language;
        case PickerTarget::Ball: return p.ball;
        case PickerTarget::MetLocation: return p.metLocation;
        case PickerTarget::Nature: return p.nature;
        case PickerTarget::Gender: return p.gender;
        case PickerTarget::Ability: return p.abilityNumber;
        case PickerTarget::Form: return p.form;
        case PickerTarget::Move:
            return moveSlot >= 0 && moveSlot < 4 ? p.moves[static_cast<std::size_t>(moveSlot)] : 0;
        default: return 0;
    }
}

void openValuePicker(TrainerViewScreen& screen, State& state, PickerTarget target,
                     int moveSlot = 0) {
    state.pickerScroll.reset();
    state.pickerViewport.reset();
    state.pickerTarget = target;
    if(target==PickerTarget::HeldItem)state.heldItemSearch={};
    state.pickerMoveSlot = moveSlot;
    state.pickerValues.clear();
    state.encounterChoices.clear();

    if (target == PickerTarget::HeldItem) {
        const auto count = Names::getItemCountG3();
        state.pickerValues.reserve(count);
        for (std::size_t id = 0; id < count; ++id) {
            const char* name = Names::getItemNameG3(static_cast<uint16_t>(id));
            if (Gen3::releasedHeldItem(static_cast<uint16_t>(id)) &&
                (id == 0 || (name && *name && std::string(name) != "???")))
                state.pickerValues.push_back(static_cast<uint16_t>(id));
        }
    } else if (target == PickerTarget::Language) {
        state.pickerValues = {1, 2, 3, 4, 5, 7};
    } else if (target == PickerTarget::Ball) {
        const auto balls = Enums::getBallList(Enums::GameVersion::FRLG);
        state.pickerValues.assign(balls.begin(), balls.end());
    } else if (target == PickerTarget::MetLocation) {
        // Round 11 FRLG: the record's origin game decides the list (FireRed / LeafGreen: Kanto and
        // Sevii; R/S/E: Hoenn), never the save's. A species with no catch template in that game
        // (a traded Rayquaza, an evolved Charizard) still gets every native location.
        const uint8_t origin = state.session.working.originGame;
        state.encounterChoices = EncounterPicker::templatesForOrigin(origin, state.session.working.species);
        // R2-BUG-008: a stored origin the templates do not cover stays choosable (keep stored).
        state.pickerValues = EncounterPicker::rowValuesWithNative(
            state.encounterChoices, state.session.working.metLocation, origin);
        if (state.pickerValues.empty()) {
            state.pickerTarget = PickerTarget::None;
            screen.postStatus("Met Location has no native Generation III list for this origin game", 300);
            return;
        }
        state.pickerRow = EncounterPicker::selectedRowWithNative(
            state.encounterChoices, state.pickerValues, state.session.working.metLocation,
            state.session.working.metLevel);
        state.valuePicker = true;
        screen.controllerNavigation.reset();
        return;
    } else if (target == PickerTarget::Nature) {
        for (uint16_t value = 0; value < 25; ++value) state.pickerValues.push_back(value);
    } else if (target == PickerTarget::Gender) {
        if (!variableGenderSpecies(state.session.working.species)) {
            state.pickerTarget = PickerTarget::None;
            screen.postStatus("Gender is fixed for this Generation III species", 240);
            return;
        }
        state.pickerValues = {0, 1};
    } else if (target == PickerTarget::Ability) {
        state.pickerValues = SessionModel::nativeAbilitySlots(state.session.working.species);
    } else if (target == PickerTarget::Form) {
        for (uint16_t letter = 0; letter < 28; ++letter) state.pickerValues.push_back(letter);
    } else if (target == PickerTarget::Move) {
        const auto game = PokeVault::Integration::Gen3EditorProvider::sourceGameForId(screen.sourceGameId);
        if (game) {
            // Match the accepted Gen I/II picker contract: Empty + moves the species can
            // learn in this exact current game. Cross-version Transfer/Preserved entries
            // remain valid for an untouched existing Pokemon, but are not offered as new edits.
            state.pickerValues.push_back(0);
            for (uint16_t move = 1; move < Gen3::Learnset::kMoveCount; ++move) {
                if (PokeBank::UIModel::SharedMoveFacts::moveExists(move, kGen3MoveGame) &&
                    Gen3::Learnset::directlyLearnable(*game, state.session.working.species, move))
                    state.pickerValues.push_back(move);
            }
        }
    }

    const uint16_t current = currentPickerValue(state, target, moveSlot);
    // A stale/zero current ability or a cross-version/preserved move must never add a
    // non-native choice to its picker. Cancel still leaves the original draft untouched.
    if (target != PickerTarget::Ability && target != PickerTarget::Move &&
        std::find(state.pickerValues.begin(), state.pickerValues.end(), current) ==
        state.pickerValues.end())
        state.pickerValues.push_back(current);

    if (state.pickerValues.empty()) {
        state.pickerTarget = PickerTarget::None;
        screen.postStatus("No valid values are available for this Generation III field", 300);
        return;
    }

    if (target == PickerTarget::HeldItem || target == PickerTarget::Move) {
        state.pickerNativeValues = state.pickerValues;
        state.pickerSort = ItemPickerSort::Mode::Native;
        state.pickerQuery.clear();
    }
    state.pickerRow = 0;
    for (int i = 0; i < static_cast<int>(state.pickerValues.size()); ++i) {
        if (state.pickerValues[static_cast<std::size_t>(i)] == current) {
            state.pickerRow = i;
            break;
        }
    }
    state.valuePicker = true;
    screen.controllerNavigation.reset();
}

void applyValuePicker(State& state) {
    if (!state.valuePicker || state.pickerValues.empty()) return;
    const auto value = state.pickerValues[static_cast<std::size_t>(state.pickerRow)];
    auto& p = state.session.working;
    switch (state.pickerTarget) {
        case PickerTarget::HeldItem: p.heldItem = value; break;
        case PickerTarget::Language: p.language = static_cast<uint8_t>(value); break;
        case PickerTarget::Ball: p.ball = static_cast<uint8_t>(value); break;
        case PickerTarget::Nature:
            state.session.setNature(static_cast<uint8_t>(value));
            break;
        case PickerTarget::Gender:
            state.session.setGender(static_cast<uint8_t>(value));
            break;
        case PickerTarget::Ability:
            state.session.setAbilityNumber(static_cast<uint8_t>(value));
            break;
        case PickerTarget::Form:
            state.session.setUnownLetter(static_cast<uint8_t>(value));
            break;
        case PickerTarget::MetLocation: {
            if (MetRows::isOtherNativeRow(value)) {
                // No template: the met level stays as entered (Met Level row prompts 0-100).
                p.metLocation = MetRows::otherNativeRowLocation(value);
                state.selectedEncounter.reset();
            } else if (const auto* choice = EncounterPicker::choiceAt(state.encounterChoices, value)) {
                p.metLocation = choice->encounter.location;
                state.session.setEncounterMetLevel(choice->encounter.minLevel);
                state.selectedEncounter = *choice;
                state.session.noteMetLocationChosen();   // Round 11: the Create Met Location is chosen
            }
            break;
        }
        case PickerTarget::Move: {
            const auto slot = static_cast<std::size_t>(state.pickerMoveSlot);
            p.moves[slot] = value;
            p.ppUps[slot] = 0;
            p.pp[slot] = value == 0 ? 0 :
                Names::getMoveBasePP(value, Enums::GameVersion::FRLG);
            break;
        }
        default: break;
    }
    state.valuePicker = false;
    state.pickerTarget = PickerTarget::None;
    state.pickerValues.clear();
    state.encounterChoices.clear();
    state.previewValid = false;
}

void editDetail(TrainerViewScreen& screen, State& state, int row) {
    auto& p = state.session.working;
    if (!detailEditable(state, row)) {
        screen.postStatus("This Generation III field is read-only in the current safe adapter", 300);
        return;
    }
    if (row == 0) {
        state.speciesPreview = std::clamp<uint16_t>(p.species, 1, 386);
        state.speciesPreviewShiny = p.shiny;
        state.speciesPicker = true;
        state.speciesChooser.reset();   // R2-SEARCH: every opening starts with the full dex
        screen.controllerNavigation.reset();
        return;
    }
    if (row == 1) {
        const auto r = Utils::promptText("Generation III Nickname", "1-10 Gen III characters",
                                         p.nickname, 10);
        if (r.accepted) p.nickname = r.text;
        return;
    }
    if (row == 2) {
        const auto r = Utils::promptNumber("Generation III Level", p.level, 1, 100);
        if (r.accepted) state.session.setLevel(static_cast<uint8_t>(r.value));
        return;
    }
    if (row == 3) {
        const uint32_t maximum =
            Pokemon::getExpForLevel(100, Pokemon::getGrowthRate(p.species));
        const auto r = Utils::promptNumber("Generation III Experience",
            static_cast<int>(std::min<uint32_t>(p.experience, 2000000000u)),
            0, static_cast<int>(std::min<uint32_t>(maximum, 2000000000u)));
        if (r.accepted) state.session.setExperience(static_cast<uint32_t>(r.value));
        return;
    }
    if (row == 4) {
        const auto r = Utils::promptText("Generation III OT", "1-7 Gen III characters",
                                         p.otName, 7);
        if (r.accepted) p.otName = r.text;
        return;
    }

    if (row == 5) {
        const auto r = Utils::promptNumber("Trainer ID", p.tid, 0, 65535);
        if (r.accepted) {
            p.tid = static_cast<uint16_t>(r.value);
            // Keeps shininess stable across the TID change (explicit request when it would flip).
            state.session.setTid(p.tid);
            state.previewValid = false;
        }
        return;
    }

    if (row == 7) {
        openValuePicker(screen, state, PickerTarget::HeldItem);
        if (Gen3::storedUnreleasedHeldItem(p.heldItem))
            screen.postStatus(std::string(Names::getItemNameG3(p.heldItem)) +
                " is unreleased as a held item in Gen III; it is kept unless you choose another", 360);
        return;
    }
    if (row == 15) { openValuePicker(screen, state, PickerTarget::Form); return; }
    if (row == 9) {
        // Inline toggle: A cycles Japanese / English / French / Italian / German / Spanish in
        // place. PK3 names are 5 / 5 characters in Japanese and 10 / 7 otherwise
        // (Gen3StagedPokemonEditor maxNicknameLength / maxOtLength), so a language the current
        // nickname or OT cannot fit is skipped with a clear message, never staged.
        const auto codepoints = [](const std::string& text) {
            std::size_t n = 0;
            for (unsigned char c : text) n += (c & 0xC0) != 0x80;
            return n;
        };
        const auto fits = [&](uint8_t language) {
            return codepoints(p.nickname) <= (language == 1 ? 5u : 10u) &&
                   codepoints(p.otName) <= (language == 1 ? 5u : 7u);
        };
        uint8_t next = Presentation::nextGen3Language(p.language);
        bool skipped = false;
        for (std::size_t i = 0; i < Presentation::kGen3Languages.size() && !fits(next); ++i) {
            skipped = true;
            next = Presentation::nextGen3Language(next);
        }
        if (next != p.language) {
            p.language = next;
            state.previewValid = false;
        }
        if (skipped)
            screen.postStatus("Japanese needs a nickname and OT of 5 characters or fewer", 300);
        return;
    }
    if (row == 11) { openValuePicker(screen, state, PickerTarget::Ball); return; }
    if (row == 13) { openValuePicker(screen, state, PickerTarget::MetLocation); return; }
    if (row == 14) {
        // Inline toggle: None -> Infected -> Cured -> None, native-valid PK3 bytes only; the
        // untouched source byte comes back exactly when its state is reached again.
        p.pokerus = Presentation::nextPokerusByte(p.pokerus, state.session.baseline.pokerus);
        state.previewValid = false;
        return;
    }
    if (row == 12) {
        auto choices = EncounterPicker::templatesForOrigin(p.originGame, p.species);
        std::optional<Encounter::EncounterProvenanceChoice> chosen = state.selectedEncounter;
        if (!chosen || chosen->encounter.location != p.metLocation) {
            chosen.reset();
            for (const auto& choice : choices) {
                const auto& encounter = choice.encounter;
                if (encounter.location != p.metLocation) continue;
                if (encounter.containsLevel(p.metLevel)) { chosen = choice; break; }
                if (!chosen) chosen = choice;
            }
        }
        if (!chosen) {
            if (p.metLocation == 0) {
                screen.postStatus("Choose a Met Location first", 300);
                return;
            }
            // Round 11 FRLG: a native location with no catch template (trade / gift / hatch
            // history, or a kept stored value): any native met level; 0 is the hatched marker.
            const auto r = Utils::promptNumber("Met Level (not a listed encounter)", p.metLevel, 0, 100);
            if (r.accepted) state.session.setEncounterMetLevel(static_cast<uint8_t>(r.value));
            return;
        }
        const auto& encounter = chosen->encounter;
        if (encounter.fixedLevel()) {
            state.session.setEncounterMetLevel(encounter.minLevel);
            state.selectedEncounter = *chosen;
            screen.postStatus("This encounter has fixed met level " + std::to_string(encounter.minLevel), 240);
            return;
        }
        const auto r = Utils::promptNumber("Met Level for selected encounter", p.metLevel,
                                           encounter.minLevel, encounter.maxLevel);
        if (r.accepted) {
            state.session.setEncounterMetLevel(static_cast<uint8_t>(r.value));
            state.selectedEncounter = *chosen;
        }
        return;
    }

    int current = 0, minimum = 0, maximum = 255;
    const char* title = "Generation III value";
    switch (row) {
        case 8: current = p.friendship; title = "Friendship"; break;
        default: return;
    }
    const auto r = Utils::promptNumber(title, current, minimum, maximum);
    if (!r.accepted) return;
    switch (row) {
        case 8: p.friendship = static_cast<uint8_t>(r.value); break;
        default: break;
    }
}

void editStatCell(TrainerViewScreen& screen, State& state) {
    if (!state.session.editable()) return;

    if (state.focus.row >= 6) {
        state.focus.column = 0;
        switch (state.focus.row) {
            case 6:
                openValuePicker(screen, state, PickerTarget::Nature);
                return;
            case 7:
                if (pidLinkedRowEditable(state, 7)) {
                    if (!state.session.cycleGender())
                        screen.postStatus("Gender is fixed for this Generation III species", 240);
                    state.previewValid = false;
                } else {
                    screen.postStatus("Gender is fixed for this Generation III species", 240);
                }
                return;
            case 8:
                state.session.setShiny(!state.session.working.shiny);
                state.previewValid = false;
                return;
            case 9: {
                const auto slots = SessionModel::nativeAbilitySlots(state.session.working.species);
                if (slots.size() <= 1) {
                    const auto& personal = Pokemon::getPersonalInfoG3(state.session.working.species);
                    screen.postStatus(std::string(Trainer::getAbilityName(personal.ability1)) +
                        " is this species' only native Generation III ability", 300);
                    return;
                }
                openValuePicker(screen, state, PickerTarget::Ability);
                return;
            }
            default:
                screen.postStatus("PID is derived from the selected Generation III traits", 260);
                return;
        }
    }

    if (state.focus.column >= 2) return;
    const std::size_t stat = state.focus.row;
    if (state.focus.column == 0) {
        const auto r = Utils::promptNumber("Generation III IV",
            state.session.working.ivs[stat], 0, 31);
        if (r.accepted) state.session.working.ivs[stat] = static_cast<uint8_t>(r.value);
    } else {
        const auto r = Utils::promptNumber("Generation III EV",
            state.session.working.evs[stat], 0, 255);
        if (!r.accepted) return;
        auto evs = state.session.working.evs;
        evs[stat] = static_cast<uint8_t>(r.value);
        unsigned total = 0;
        for (uint8_t value : evs) total += value;
        if (total > 510) {
            screen.postStatus("Generation III EV total cannot exceed 510", 300);
            return;
        }
        state.session.working.evs = evs;
    }
    state.previewValid = false;
}

int gen3MoveMaxPP(uint16_t move, uint8_t ppUps) {
    if (move == 0) return 0;
    const int base = Names::getMoveBasePP(move, Enums::GameVersion::FRLG);
    return base * (5 + std::min<int>(ppUps, 3)) / 5;
}

void beginMoveEditor(TrainerViewScreen& screen, State& state, int slot) {
    state.moveEditor = true;
    state.moveEditorRow = 0;
    state.moveEditorSlot = std::clamp(slot, 0, 3);
    const auto index = static_cast<std::size_t>(state.moveEditorSlot);
    state.moveBaseline = state.session.working.moves[index];
    state.movePpBaseline = state.session.working.pp[index];
    state.movePpUpsBaseline = state.session.working.ppUps[index];
    screen.controllerNavigation.reset();
}

void cancelMoveEditor(TrainerViewScreen& screen, State& state) {
    if (!state.moveEditor) return;
    const auto index = static_cast<std::size_t>(state.moveEditorSlot);
    state.session.working.moves[index] = state.moveBaseline;
    state.session.working.pp[index] = state.movePpBaseline;
    state.session.working.ppUps[index] = state.movePpUpsBaseline;
    state.moveEditor = false;
    state.moveEditorRow = 0;
    state.previewValid = false;
    screen.controllerNavigation.reset();
}

void editMoveCell(TrainerViewScreen& screen, State& state) {
    if (!state.session.editable()) return;
    beginMoveEditor(screen, state, static_cast<int>(state.focus.row));
}


PokeBank::UIModel::SharedPickerPaging::Keys gen3PickerKeys(u64 down) noexcept {
    return {(down & HidNpadButton_Up) != 0, (down & HidNpadButton_Down) != 0,
            (down & HidNpadButton_Left) != 0, (down & HidNpadButton_Right) != 0,
            (down & HidNpadButton_L) != 0, (down & HidNpadButton_R) != 0,
            (down & HidNpadButton_ZL) != 0, (down & HidNpadButton_ZR) != 0};
}

bool handleValuePicker(TrainerViewScreen& screen, State& state, u64 down, const TouchInput& touch) {
    if (!state.valuePicker) return false;
    // 7D: a release-confirmed tap outside the picker cancels it like B; this frame's return
    // keeps the gesture from reaching the workspace underneath.
    if (SharedPopupDismiss::tappedOutside(touch)) {
        state.pickerScroll.stop();
        screen.controllerNavigation.reset();
        state.valuePicker = false;
        state.pickerTarget = PickerTarget::None;
        state.pickerValues.clear();
        return true;
    }
    const int count = static_cast<int>(state.pickerValues.size());
    const bool gridPicker = state.pickerTarget == PickerTarget::HeldItem;
    const auto heldItemName=[&](uint16_t id){return pickerValueLabel(state, id);};
    if(gridPicker) {
        const auto refilter=[&](const std::string& query) {
            const int row=ItemPickerSearch::apply(state.pickerValues,state.pickerNativeValues,
                state.pickerSort,query,state.pickerRow,heldItemName,SharedSearchInput::searchKey);
            state.pickerQuery=query;
            if(row>=0)state.pickerRow=row;
            state.pickerViewport.reset();
            return row>=0;
        };
        const int px=(1280-HeldItemGrid::modalWidth)/2,py=(720-kNavBarH-HeldItemGrid::modalHeight)/2;
        const SharedSearchInput::Keys searchKeys{bool(down&HidNpadButton_Up),bool(down&HidNpadButton_Down),
            bool(down&HidNpadButton_Left),bool(down&HidNpadButton_Right),bool(down&HidNpadButton_A),bool(down&HidNpadButton_B),
        bool(down&HidNpadButton_L),bool(down&HidNpadButton_R)};
        if(SharedHeldItemSearch::handle(state.heldItemSearch,down&HidNpadButton_Minus,searchKeys,
            SharedSearchInput::sample(touch),px,py,refilter,
            down&(HidNpadButton_L|HidNpadButton_R|HidNpadButton_ZL|HidNpadButton_ZR|HidNpadButton_X|HidNpadButton_Y))) {
            state.pickerScroll.stop();
            state.pickerViewport.reveal(state.pickerRow,static_cast<int>(state.pickerValues.size()),
                HeldItemGrid::rows,HeldItemGrid::columnsForSearch(state.heldItemSearch.open()));
            return true;
        }
    }
    const int columns = gridPicker ? HeldItemGrid::columnsForSearch(state.heldItemSearch.open()) : 1;
    const int compactCount = std::max(0, count);
    const bool nature = state.pickerTarget == PickerTarget::Nature;
    const auto natureLayout = PokeBank::UIModel::NaturePickerLayout::forChoices(compactCount);
    const bool compact = state.pickerTarget == PickerTarget::Language ||
        state.pickerTarget == PickerTarget::Ball ||
        state.pickerTarget == PickerTarget::MetLocation ||
        nature || state.pickerTarget == PickerTarget::Ability ||
        state.pickerTarget == PickerTarget::Gender || state.pickerTarget == PickerTarget::Form;
    const int visibleRows = gridPicker ? HeldItemGrid::rows
        : state.pickerTarget == PickerTarget::Move ? MoveUI::compactPickerLayout().visibleRows
        : nature ? natureLayout.visibleRows
        : compact ? std::min(8, compactCount) : 10;
    state.pickerViewport.ensure(state.pickerRow, count, visibleRows, columns);
    const int maxFirstRow = TouchPickerViewport::maxFirst(count, visibleRows, columns);
    // Shared scrollbar first on the row lists that draw one: a held thumb moves the viewport,
    // never the selection, and owns the frame's touch. The held-item grid publishes no track.
    const bool pickerHasScrollbar = state.pickerTarget != PickerTarget::HeldItem;
    if (!pickerHasScrollbar) state.pickerScrollbar = {};
    const auto pickerBar = pickerHasScrollbar
        ? SharedScrollbar::handleTouch(state.pickerScrollbar, SharedScrollbar::sample(touch),
              SharedScrollbar::rows(maxFirstRow + visibleRows, visibleRows, state.pickerViewport.firstRow))
        : SharedScrollbar::TouchResult{};
    if (pickerBar.changed) {
        state.pickerViewport.firstRow = pickerBar.scroll;
        state.pickerScroll.stop();
    }
    // Fully visible lists are not scroll surfaces; don't rubber-band small pickers.
    if (pickerBar.consumed) {
        state.pickerScroll.stop();
    } else if (count > 0 && maxFirstRow > 0) {
        if (state.pickerTarget == PickerTarget::HeldItem) {
  // L06: SharedHeldItemInput::update below owns the grid's scroll, tap and A.
        } else if (state.pickerTarget == PickerTarget::Move) {
  constexpr auto pickerLayout = MoveUI::compactPickerLayout();
  const int px = (1280 - pickerLayout.width) / 2;
  const int py = (720 - kNavBarH - pickerLayout.height) / 2;
  state.pickerScroll.updateVertical(
      touch, px + 24, py + pickerLayout.rowStart,
      pickerLayout.width - 48,
      pickerLayout.visibleRows * pickerLayout.rowStep,
      pickerLayout.rowStep, state.pickerViewport.firstRow, maxFirstRow + 1);
        } else {
  const auto valueList = valueListLayout(state.pickerTarget, count);
  const int pickerW = nature ? natureLayout.width : compact ? valueList.frame.w : 680;
  const int pickerH = nature ? natureLayout.height : compact ? valueList.frame.h : 560;
  const int rowStep = nature ? natureLayout.rowStep : compact ? valueList.rule.rowHeight : 42;
  const int px = (1280 - pickerW) / 2;
  const int py = (720 - kNavBarH - pickerH) / 2;
  state.pickerScroll.updateVertical(
      touch, px + 22, py + 86, pickerW - 44,
      visibleRows * rowStep, rowStep, state.pickerViewport.firstRow, maxFirstRow + 1);
        }
    }
    if (maxFirstRow == 0)
        state.pickerScroll.stop();
    // Do not retarget on contact: the same finger may scroll the picker.
    // A release-confirmed tap is the only direct-touch selection.
    const int touchTap = screen.touchedButtonId(touch);
    bool tappedChoice = false;
    // The Held Item grid resolves its own taps (SharedHeldItemInput::update below).
    if (!gridPicker && !pickerBar.consumed && touchTap >= 4000 && touchTap < 4000 + count) {
        state.pickerRow = touchTap - 4000;
        down |= HidNpadButton_A;
        tappedChoice = true;
    }
    if (count <= 0) {
        state.valuePicker = false;
        return true;
    }

    if ((gridPicker || state.pickerTarget == PickerTarget::Move) && (down & HidNpadButton_X)) {
        state.pickerSort = ItemPickerSort::next(state.pickerSort);
        state.pickerRow = ItemPickerSort::reorder(
            state.pickerValues, state.pickerNativeValues,
            state.pickerSort, state.pickerRow,
            [&](uint16_t id) {
                if (state.pickerTarget == PickerTarget::Move)
                    return id == 0 ? std::string() : std::string(Names::getMoveName(id));
                return pickerValueLabel(state, id);
            });
        if (gridPicker)   // NEW-06: a re-sort keeps the active search
            state.pickerRow = std::max(0, ItemPickerSearch::narrow(state.pickerValues, state.pickerQuery,
                                                                   state.pickerRow, heldItemName,SharedSearchInput::searchKey));
        state.pickerScroll.stop();
        state.pickerViewport.reset();
        state.pickerViewport.reveal(state.pickerRow, static_cast<int>(state.pickerValues.size()), visibleRows, columns);
        return true;
    }

    if (gridPicker && !(down & HidNpadButton_B)) {
        // L06: tap and A share one step and one commit (UI/SharedHeldItemInput.h).
        const int px = (1280 - HeldItemGrid::modalWidth) / 2;
        const int py = (720 - kNavBarH - HeldItemGrid::modalHeight) / 2;
        const auto held = SharedHeldItemInput::update(touch, state.pickerScroll, state.pickerViewport,
            state.pickerRow, count, state.heldItemSearch.open(), px, py,
            {bool(down & HidNpadButton_Up), bool(down & HidNpadButton_Down), bool(down & HidNpadButton_Left),
             bool(down & HidNpadButton_Right), bool(down & HidNpadButton_L), bool(down & HidNpadButton_R),
             bool(down & HidNpadButton_A)});
        if (held.revealed)
            screen.postStatus("Focused choice brought into view; press A again or tap a visible row", 220);
        if (held.choose) applyValuePicker(state);
        return true;
    }
    if (down & (HidNpadButton_Up | HidNpadButton_Down | HidNpadButton_Left | HidNpadButton_Right | HidNpadButton_L | HidNpadButton_R))
        state.pickerScroll.stop();
    if (state.pickerTarget == PickerTarget::HeldItem) {
        if (down & HidNpadButton_Up)
            state.pickerRow = HeldItemGrid::move(state.pickerRow, count, 0, -1);
        if (down & HidNpadButton_Down)
            state.pickerRow = HeldItemGrid::move(state.pickerRow, count, 0, 1);
        if (down & HidNpadButton_Left)
            state.pickerRow = HeldItemGrid::move(state.pickerRow, count, -1, 0);
        if (down & HidNpadButton_Right)
            state.pickerRow = HeldItemGrid::move(state.pickerRow, count, 1, 0);
        if (down & HidNpadButton_L)
            state.pickerRow = HeldItemGrid::move(state.pickerRow, count, 0, 0, -1);
        if (down & HidNpadButton_R)
            state.pickerRow = HeldItemGrid::move(state.pickerRow, count, 0, 0, 1);
    } else {
        state.pickerRow = PokeBank::UIModel::SharedPickerPaging::pagedRow(
            state.pickerRow, count, gen3PickerKeys(down));
    }

    if (gen3PickerKeys(down).any())
        state.pickerViewport.reveal(state.pickerRow, count, visibleRows, columns);

    if (down & HidNpadButton_B) {
        state.pickerScroll.stop();
        // A held direction must not keep repeating into workspace focus after the picker closes.
        screen.controllerNavigation.reset();
        state.valuePicker = false;
        state.pickerTarget = PickerTarget::None;
        state.pickerValues.clear();
        return true;
    }
    if (down & HidNpadButton_A) {
        // The finger can browse far away from the highlighted controller choice. A must never
        // commit that now-hidden choice: reveal it first, and require a new explicit confirm.
        // L06 (Round 6): a completed tap on a partly visible cell (the clipped row a finger
        // scroll leaves at the grid's edge) is revealed AND chosen, exactly like A on a visible
        // cell. Only a controller A on a choice scrolled out of sight stops to reveal it.
        if (!tappedChoice && !state.pickerViewport.containsSelection(
                state.pickerRow, count, visibleRows, columns)) {
            state.pickerScroll.stop();
            state.pickerViewport.reveal(state.pickerRow, count, visibleRows, columns);
            screen.postStatus("Focused choice brought into view; press A again or tap a visible row", 220);
            return true;
        }
        if (tappedChoice) state.pickerViewport.reveal(state.pickerRow, count, visibleRows, columns);
        state.pickerScroll.stop();
        applyValuePicker(state);
        return true;
    }
    return true;
}

// Gen III offers its whole native dex (1..386); the chooser filters by the bare species name.
constexpr auto gen3SpeciesAvailable = [](uint16_t) { return true; };
constexpr auto gen3SpeciesName = [](uint16_t species) { return std::string(Names::getSpeciesName(species)); };

bool handleSpeciesPicker(TrainerViewScreen& screen, State& state, u64 down, const TouchInput& touch) {
    if (!state.speciesPicker) return false;
    if (SharedPopupDismiss::tappedOutside(touch)) {   // 7D: like B, never a tap-through
        state.speciesPicker = false;
        return true;
    }
    // R2-SEARCH: the search key grid and the list scrollbar take their touches first; X / Minus
    // toggle the grid, and while it is open the D-pad / stick, A and B type into it.
    uint16_t chosen = std::clamp<uint16_t>(state.speciesPreview, 1, 386);
    const auto touched = SharedSpeciesPicker::handleTouch(
        state.speciesChooser, SharedSearchInput::sample(touch), chosen, 386,
        SharedSpeciesPicker::ModalX, SharedSpeciesPicker::ModalY, gen3SpeciesAvailable, gen3SpeciesName);
    const auto keyed = SharedSpeciesPicker::handleKeys(
        state.speciesChooser, gen3PickerKeys(down), (down & HidNpadButton_A) != 0,
        (down & HidNpadButton_B) != 0, (down & (HidNpadButton_X | HidNpadButton_Minus)) != 0,
        chosen, 386, gen3SpeciesAvailable, gen3SpeciesName);
    state.speciesPreview = chosen;
    if (touched.keys || keyed.keys)
        down &= ~(HidNpadButton_Up | HidNpadButton_Down | HidNpadButton_Left | HidNpadButton_Right |
                  HidNpadButton_L | HidNpadButton_R | HidNpadButton_ZL | HidNpadButton_ZR |
                  HidNpadButton_A | HidNpadButton_B);
    // A species-list swipe must advance from the actual preview, not
    // a provisional row focused when the finger first landed.
    const int speciesTap = screen.touchedButtonId(touch);
    if (!touched.touch && speciesTap >= 31500 && speciesTap <= 31500 + 386) {
        state.speciesPreview = static_cast<uint16_t>(speciesTap - 31500);
        down |= HidNpadButton_A;
    }
    const int dy = touch.deltaY();
    if (!touched.touch && touch.justReleased() && touch.dragged() &&
        SharedSpeciesPicker::gestureStartedInList(touch.startX(), touch.startY()) &&
        std::abs(dy) >= 24 && std::abs(dy) > std::abs(touch.deltaX())) {
        const int steps = std::max(1, std::abs(dy) / 43);
        state.speciesPreview = SharedSpeciesPicker::neighbour(
            state.speciesChooser, chosen, dy < 0 ? steps : -steps);
    }
    // Shared paging (Gen13PickerPaging.h): over the whole dex when no filter is typed, over the
    // filtered list otherwise (R2-SEARCH).
    if (gen3PickerKeys(down).any())
        state.speciesPreview = state.speciesChooser.filtered()
            ? SharedSpeciesPicker::paged(state.speciesChooser, state.speciesPreview, gen3PickerKeys(down))
            : static_cast<uint16_t>(PokeBank::UIModel::SharedPickerPaging::pagedRow(
                  static_cast<int>(state.speciesPreview) - 1, 386, gen3PickerKeys(down)) + 1);
    if (down & HidNpadButton_B) {
        state.speciesPicker = false;
        return true;
    }
    if (down & HidNpadButton_Y) {
        state.speciesPreviewShiny = !state.speciesPreviewShiny;
        return true;
    }
    if (down & HidNpadButton_A) {
        // Session::setSpecies applies the shared Round 11 policies: moves (SpeciesChangeMoves.h)
        // and history (SpeciesChangeHistoryPolicy.h, checked against
        // Encounter::forGameSpeciesWithGen3Provenance). A Create draft's invalid encounter and
        // Pokerus reset; an existing Pokemon keeps its recorded Met Location and Pokerus.
        const uint16_t previousSpecies = state.session.working.species;
        if (state.session.setSpecies(state.speciesPreview, screen.sourceGameId)) {
            state.session.setShiny(state.speciesPreviewShiny);
            state.speciesPicker = false;
            if (state.session.working.species != previousSpecies) state.selectedEncounter.reset();
            if (state.session.encounterReset)
                screen.postStatus(PokeBank::UIModel::SpeciesChangeHistoryPolicy::kChooseEncounterStatus, 300);
        }
        else
            screen.postStatus("Species is not representable in Generation III");
        return true;
    }
    return true;
}

bool handleMoveEditor(TrainerViewScreen& screen, State& state, u64 down, const TouchInput& touch) {
    if (!state.moveEditor) return false;
    if (state.valuePicker) return false;
    // Move editor commands also require a clean completed tap.
    const int moveTap = screen.touchedButtonId(touch);
    if (moveTap >= 31400 && moveTap < 31405) { state.moveEditorRow = moveTap - 31400; down |= HidNpadButton_A; }
    if (down & HidNpadButton_Up) state.moveEditorRow = (state.moveEditorRow + 4) % 5;
    if (down & HidNpadButton_Down) state.moveEditorRow = (state.moveEditorRow + 1) % 5;
    if (down & HidNpadButton_B) {
        cancelMoveEditor(screen, state);
        return true;
    }
    if (!(down & HidNpadButton_A)) return true;

    const auto slot = static_cast<std::size_t>(state.moveEditorSlot);
    if (state.moveEditorRow == 0) {
        openValuePicker(screen, state, PickerTarget::Move, state.moveEditorSlot);
        return true;
    }
    if (state.moveEditorRow == 1) {
        if (state.session.working.moves[slot] == 0) {
            screen.postStatus("Choose a move before editing PP", 220);
            return true;
        }
        const int maximum = gen3MoveMaxPP(state.session.working.moves[slot],
                                          state.session.working.ppUps[slot]);
        const auto r = Utils::promptNumber("Current PP",
            std::min<int>(state.session.working.pp[slot], maximum), 0, maximum);
        if (r.accepted) state.session.working.pp[slot] = static_cast<uint8_t>(r.value);
        state.previewValid = false;
        return true;
    }
    if (state.moveEditorRow == 2) {
        if (state.session.working.moves[slot] == 0) {
            screen.postStatus("Empty move slots keep PP Ups at zero", 220);
            return true;
        }
        const auto r = Utils::promptNumber("PP Ups", state.session.working.ppUps[slot], 0, 3);
        if (r.accepted) {
            state.session.working.ppUps[slot] = static_cast<uint8_t>(r.value);
            state.session.working.pp[slot] = static_cast<uint8_t>(std::min<int>(
                state.session.working.pp[slot],
                gen3MoveMaxPP(state.session.working.moves[slot], state.session.working.ppUps[slot])));
        }
        state.previewValid = false;
        return true;
    }
    if (state.moveEditorRow == 3) {
        state.moveEditor = false;
        state.moveEditorRow = 0;
        state.previewValid = false;
        screen.controllerNavigation.reset();
        return true;
    }
    cancelMoveEditor(screen, state);
    return true;
}

bool handleWorkspace(TrainerViewScreen& screen, State& state, u64 down, const TouchInput& touch) {
    if (!workspaceActive(state)) return false;
    if (handleValuePicker(screen, state, down, touch)) return true;
    if (handleSpeciesPicker(screen, state, down, touch)) return true;
    if (handleMoveEditor(screen, state, down, touch)) return true;

    if (state.session.confirmExit) {
        if (down & HidNpadButton_A) {
            if (keepWorkspace(screen)) state.session.confirmExit = false;
            return true;
        }
        if (down & HidNpadButton_Y) {
            const bool create = state.session.mode == SessionModel::Mode::Create;
            state.session.discardDraft();
            screen.postStatus(create
                ? "Generation III Create draft discarded; staged/source bytes unchanged"
                : "Generation III Edit session discarded; staged/source bytes unchanged");
            return true;
        }
        if (down & HidNpadButton_B) {
            state.session.continueEditing();
            return true;
        }
        return true;
    }

    // Round 6 (P1-RETURN): + opens the shared Review from Create / Edit (Y stays re-roll). Review
    // is handled and drawn before this workspace, and its B returns here with the draft and focus
    // untouched. A passive View keeps its read-only controls.
    if (state.session.mode != SessionModel::Mode::View) {
        namespace Entry = PokeBank::UIModel::ReviewEntry;
        if ((down & HidNpadButton_Plus) &&
            Entry::plusOpensReview(state.session.mode == SessionModel::Mode::Create ? Entry::Screen::Create
                                                                                    : Entry::Screen::Edit, 3)) {
            if (!screen.openStagedReview(false))
                screen.postStatus("No staged Generation III changes to review", 220);
            return true;
        }
    }

    const auto panelBeforeNavigation = state.focus.panel;
    const bool passiveView = state.session.mode == SessionModel::Mode::View;
    // Drag scrolls the visible left-side rows without changing the selected
    // Pokémon field. The old release handler advanced focus by 1–N rows and
    // skipped Held Item/other fields, especially after a quick swipe.
    constexpr int visibleDetailRows = 8;
    const SharedPokemonShell::Geometry detailGeometry(1280, 720, kNavBarH);
    state.detailsViewport.ensure(state.detailsFocus,
                                 static_cast<int>(detailLabels.size()), visibleDetailRows);
    // Shared scrollbar first: a held thumb moves the Details viewport (not the focused field)
    // and owns the frame's touch, so no row under the finger is dragged or tapped.
    const auto detailsBar = SharedScrollbar::handleTouch(state.detailsScrollbar, SharedScrollbar::sample(touch),
        SharedScrollbar::rows(static_cast<int>(detailLabels.size()), visibleDetailRows,
                              state.detailsViewport.firstRow));
    if (detailsBar.changed) {
        state.detailsViewport.firstRow = detailsBar.scroll;
        state.detailsTouchScroll.stop();
    }
    if (detailsBar.consumed)
        state.detailsTouchScroll.stop();
    else
        state.detailsTouchScroll.updateVertical(
            touch, detailGeometry.leftX + 8, detailGeometry.y + 216,
            detailGeometry.leftW - 16, visibleDetailRows * 38, 38,
            state.detailsViewport.firstRow,
            TouchPickerViewport::maxFirst(static_cast<int>(detailLabels.size()), visibleDetailRows) + 1);
    if (down & (HidNpadButton_Up | HidNpadButton_Down |
                HidNpadButton_L | HidNpadButton_R))
        state.detailsTouchScroll.stop();
    int verticalDirection = 0;
    if (down & HidNpadButton_L)
        state.focus = passiveView
            ? Shared::passiveViewSwitchPanel(Shared::Generation::Gen3, state.focus, -1)
            : Shared::switchPanel(Shared::Generation::Gen3, state.focus, -1);
    if (down & HidNpadButton_R)
        state.focus = passiveView
            ? Shared::passiveViewSwitchPanel(Shared::Generation::Gen3, state.focus, 1)
            : Shared::switchPanel(Shared::Generation::Gen3, state.focus, 1);
    if (down & HidNpadButton_Up) {
        state.focus = passiveView
            ? Shared::passiveViewMoveVertical(Shared::Generation::Gen3, state.focus, -1)
            : Shared::moveVertical(Shared::Generation::Gen3, state.focus, -1);
        verticalDirection = -1;
    }
    if (down & HidNpadButton_Down) {
        state.focus = passiveView
            ? Shared::passiveViewMoveVertical(Shared::Generation::Gen3, state.focus, 1)
            : Shared::moveVertical(Shared::Generation::Gen3, state.focus, 1);
        verticalDirection = 1;
    }
    if (down & HidNpadButton_Left)
        state.focus = passiveView
            ? Shared::passiveViewMoveColumn(Shared::Generation::Gen3, state.focus, -1)
            : Shared::moveRowColumn(Shared::Generation::Gen3, state.focus, -1);
    if (down & HidNpadButton_Right)
        state.focus = passiveView
            ? Shared::passiveViewMoveColumn(Shared::Generation::Gen3, state.focus, 1)
            : Shared::moveRowColumn(Shared::Generation::Gen3, state.focus, 1);

    const bool panelChanged = state.focus.panel != panelBeforeNavigation;
    if (passiveView)
        state.focus = Shared::normalizePassiveViewFocus(Shared::Generation::Gen3, state.focus);
    else
        normalizeEditableFocus(state, verticalDirection, panelChanged);
    state.detailsFocus = Shared::detailsScrollFocus(state.focus, state.detailsFocus);
    state.detailsFocus = std::min<int>(state.detailsFocus, static_cast<int>(detailLabels.size()) - 1);
    if (down & (HidNpadButton_Up | HidNpadButton_Down |
                HidNpadButton_L | HidNpadButton_R))
        state.detailsViewport.reveal(state.detailsFocus,
                                     static_cast<int>(detailLabels.size()), visibleDetailRows);

    auto applyDirectFocus = [&](int id) {
        if (id >= 31000 && id < 31000 + static_cast<int>(detailLabels.size())) {
            const int row = id - 31000;
            if (!(passiveView || detailEditable(state, row))) return false;
            state.focus = {Shared::Panel::Details, static_cast<uint8_t>(row), 0};
            state.detailsFocus = row;
            // CG69 (Round 6 L06): reveal the tapped row first, as the Gen IV / V
            // Workspace::focusCompletedTap does, so a FIRST tap on the partly visible row
            // (Pokérus, row 14 of 16 with 8 visible) passes the A gate and acts like A.
            state.detailsTouchScroll.stop();
            state.detailsViewport.reveal(row, static_cast<int>(detailLabels.size()), visibleDetailRows);
            return true;
        }
        if (id >= 31100 && id < 31106) {
            state.focus = {Shared::Panel::Values, static_cast<uint8_t>(id - 31100), 0}; return true;
        }
        if (id >= 31120 && id < 31126) {
            state.focus = {Shared::Panel::Values, static_cast<uint8_t>(id - 31120), 1}; return true;
        }
        if (id >= 31206 && id <= 31210) {
            const int row = id - 31200;
            if (!(passiveView || pidLinkedRowEditable(state, row))) return false;
            state.focus = {Shared::Panel::Values, static_cast<uint8_t>(row), 0};
            return true;
        }
        if (id >= 31300 && id < 31304) {
            state.focus = {Shared::Panel::Moves, static_cast<uint8_t>(id - 31300), 0}; return true;
        }
        return false;
    };
    // A touch-down can still become a swipe. Never retarget semantic focus
    // before the release confirms a tap inside the same clipped row.
    const int directTap = screen.touchedButtonId(touch);
    if (applyDirectFocus(directTap)) {
        if (state.session.editable()) down |= HidNpadButton_A;
        else if (passiveView) viewOnlyActivate(screen, state);
    }
    if (touch.justReleased() && touch.dragged()) {
        const int dx = touch.deltaX(), dy = touch.deltaY();
        if (std::abs(dx) >= 96 && std::abs(dx) > std::abs(dy)) {
            state.focus = passiveView
                ? Shared::passiveViewSwitchPanel(Shared::Generation::Gen3, state.focus, dx < 0 ? 1 : -1)
                : Shared::switchPanel(Shared::Generation::Gen3, state.focus, dx < 0 ? 1 : -1);
            state.detailsFocus = Shared::detailsScrollFocus(state.focus, state.detailsFocus);
        }
    }

    if (down & HidNpadButton_B) {
        if (state.session.mode == SessionModel::Mode::View) {
            state.session.mode = SessionModel::Mode::None;
            if (state.passive) { screen.closeDetailsModal(); state.passive = false; }
        } else {
            // Create and Edit both enter the shared explicit exit safeguard.
            state.session.back();
        }
        return true;
    }
    if ((down & HidNpadButton_X) && state.session.editable()) {
        keepWorkspace(screen);
        return true;
    }
    if ((down & HidNpadButton_Y) && state.session.editable()) {
        // Y re-rolls IVs/EVs in the working draft at once: no prompt, a new result every press.
        // PK3 IVs/EVs are not PID-derived, so Shiny/Gender/Nature/Ability cannot change.
        namespace Reroll = PokeBank::UIModel::SharedStatReroll;
        Reroll::Rng rng(Utils::rand32());
        auto& w = state.session.working;
        const auto next = Reroll::reroll(rng, {w.ivs, w.evs});
        w.ivs = next.ivs;
        w.evs = next.evs;
        state.previewValid = false;
        // Round 11 H22: the new IVs / EVs and stats are the feedback; no success toast.
        namespace EditFeedback = PokeBank::UIModel::EditFeedback;
        EditFeedback::applied(screen, EditFeedback::Kind::Reroll);
        return true;
    }
    if ((down & HidNpadButton_A) && passiveView) {
        viewOnlyActivate(screen, state);
        return true;
    }
    if (!(down & HidNpadButton_A) || !state.session.editable()) return true;
    if (state.focus.panel == Shared::Panel::Details &&
        !state.detailsViewport.containsSelection(
            state.focus.row, static_cast<int>(detailLabels.size()), 8)) {
        state.detailsTouchScroll.stop();
        state.detailsViewport.reveal(
            state.focus.row, static_cast<int>(detailLabels.size()), 8);
        screen.postStatus("Focused field brought into view; press A again", 200);
        return true;
    }

    if (state.focus.panel == Shared::Panel::Details)
        editDetail(screen, state, state.focus.row);
    else if (state.focus.panel == Shared::Panel::Values)
        editStatCell(screen, state);
    else
        editMoveCell(screen, state);
    return true;
}

// The screenshot's read-only action sheet is also the fallback for a
// Generation III source that cannot acquire a validated staged editor.
// An unavailable workspace must NEVER make an empty-slot tap disappear,
// and never enable Add/Edit/Release on external source bytes.
const Pokemon::Pokemon* sourceBoxPokemon(TrainerViewScreen& screen, const State& state) {
    if (state.box < 0 || state.box >= static_cast<int>(screen.trainer.boxes.size()))
        return nullptr;
    const auto& box = screen.trainer.boxes[static_cast<std::size_t>(state.box)];
    if (state.slot < 0 || state.slot >= static_cast<int>(box.size()))
        return nullptr;
    const auto* pokemon = box[static_cast<std::size_t>(state.slot)].get();
    return pokemon && pokemon->speciesID() != 0 ? pokemon : nullptr;
}

bool handleActions(TrainerViewScreen& screen, State& state, u64 down, const TouchInput& touch) {
    if (!state.actions) return false;
    auto* staged = editor(screen);
    if (!staged) {
        const bool occupied = sourceBoxPokemon(screen, state) != nullptr;
        const int count = occupied ? 2 : 1; // View/Cancel or Cancel only.
        state.actionRow = std::clamp(state.actionRow, 0, count - 1);
        const int tap = screen.touchedButtonId(touch);
        if (tap >= 31600 && tap < 31600 + count) {
            state.actionRow = tap - 31600;
            down |= HidNpadButton_A;
        }
        if (down & HidNpadButton_Up)
            state.actionRow = (state.actionRow + count - 1) % count;
        if (down & HidNpadButton_Down)
            state.actionRow = (state.actionRow + 1) % count;
        if (down & HidNpadButton_B) {
            state.actions = false;
            screen.controllerNavigation.reset();
            return true;
        }
        if (!(down & HidNpadButton_A)) return true;
        state.actions = false;
        if (occupied && state.actionRow == 0) {
            // Native base View is read-only and does not mutate either save.
            // Target hand-off only (never drawn): open the read-only details that the Gen III
            // passive View adopts, bypassing the generic-sheet router.
            screen.actionSheet.open({
                PokeVault::UIModel::PokemonLocation::SaveBox, state.box, state.slot});
            screen.openActionSheetTargetDetails(true);
            screen.actionSheet.close();
        }
        screen.controllerNavigation.reset();
        return true;
    }
    std::string error;
    const bool occupied = staged->boxedPokemon(static_cast<std::size_t>(state.box),
                                                static_cast<std::size_t>(state.slot), error).has_value();
    const auto actions = Shared::actionsForSlot(
        occupied, {/*clone*/true, /*remove*/true, /*legality*/true, /*import*/true, /*export*/true},
        !reviewRows(screen).empty());
    if (actions.count == 0) return true;
    state.actionRow = std::clamp(
        state.actionRow, 0, std::max(0, static_cast<int>(actions.count) - 1));
    // A contact is not a selection. Preserve focus if the finger swipes away.
    const int actionTap = screen.touchedButtonId(touch);
    if (actionTap >= 31600 && actionTap < 31600 + static_cast<int>(actions.count)) {
        state.actionRow = actionTap - 31600; down |= HidNpadButton_A;
    }
    if (down & HidNpadButton_Up)
        state.actionRow = (state.actionRow - 1 + static_cast<int>(actions.count)) %
                          static_cast<int>(actions.count);
    if (down & HidNpadButton_Down)
        state.actionRow = (state.actionRow + 1) % static_cast<int>(actions.count);
    if (down & HidNpadButton_B) {
        state.actions = false;
        return true;
    }
    if (!(down & HidNpadButton_A)) return true;

    const auto action = actions[static_cast<std::size_t>(state.actionRow)];
    switch (action) {
        case Shared::Action::View: beginViewOrEdit(screen, SessionModel::Mode::View); break;
        case Shared::Action::Edit: beginViewOrEdit(screen, SessionModel::Mode::Edit); break;
        case Shared::Action::Add: beginCreate(screen, state.box, state.slot); break;
        case Shared::Action::ImportFile:
            // Feature A: the shared file dialog stages into this empty slot.
            state.actions = false;
            screen.controllerNavigation.reset();
            PokemonFileMenus::beginImport(screen, 3, state.box, state.slot);
            break;
        case Shared::Action::ExportFile:
            state.actions = false;
            screen.controllerNavigation.reset();
            PokemonFileMenus::exportPokemon(screen, 3, false, state.box, state.slot);
            break;
        case Shared::Action::LegalityProvenance:
            state.actions = false;
            state.legality = true;
            break;
        case Shared::Action::Review:
            state.actions = false;
            state.review = true;
            break;
        case Shared::Action::Remove:
            state.actions = false;
            state.confirmRelease = true;
            break;
        case Shared::Action::Clone: {
            // Round 11 H31 (owner): the shared visual Clone carry, never number keypads. Actions
            // closes; the copy rides the box cursor from the first empty slot of this box; A places
            // it on any empty slot (occupied / unreadable slots are refused), B cancels.
            state.actions = false;
            screen.controllerNavigation.reset();
            if (!ClassicPackedMove::beginCloneFromAction(screen, state.box, state.slot))
                screen.postStatus("Clone could not start from here", 240);
            break;
        }
        case Shared::Action::Close:
            state.actions = false;
            break;
        default:
            screen.postStatus("This Generation III action is not enabled in the current milestone");
            break;
    }
    return true;
}

// Verified export of the staged Pokemon copy (plus any pending Bag and Trainer edits) into the
// app-owned exports/gen3 root. The emulator source is never written.
void exportStagedGen3Copy(TrainerViewScreen& screen) {
    auto* staged = editor(screen);
    const auto* inventory = bridge(screen).stagedInventory();
    const auto* trainer = bridge(screen).stagedTrainer();
    if (!staged || (!staged->hasPendingChanges() &&
                    !(inventory && inventory->hasPendingChanges()) &&
                    !(trainer && trainer->hasPendingChanges()))) {
        screen.postStatus("No staged Generation III changes to export", 240);
        return;
    }
    std::string error;
    // Round 6 (7E G5): the source must be the bytes opened (ClassicSourceGuard.h).
    if (!PokeBank::UIModel::ClassicSourceGuard::unchangedSinceOpen(screen.classicSourceSnapshot, error)) {
        screen.postStatus(error, 480);
        return;
    }
    const std::string root = PokeBank::Paths::gen3ExportsRoot();
    const std::string timestamp = Utils::getTimestamp();
    if (!PokeBank::Paths::ensureDirectoryTree(root, &error) ||
        !PokeBank::Paths::isSafeTimestamp(timestamp)) {
        screen.postStatus(error.empty() ? "Could not prepare the Generation III export folder" : error, 420);
        return;
    }
    Gen3::Gen3ExportRequest request;
    request.rootDirectory = root;
    request.directoryStem = PokeBank::Paths::sanitizeComponent(
        Gen3::sourceGameId(staged->sourceGame())) + "_" + timestamp;
    request.sourcePath = screen.backupDir;
    request.timestamp = timestamp;
#ifdef POKEVAULT_GIT_COMMIT
    request.applicationSha = POKEVAULT_GIT_COMMIT;
#endif
    // Pending Bag edits are merged in by sector and Trainer edits applied last, so one exported
    // save carries every workspace.
    const auto result = Gen3::publishVerifiedGen3Export(*staged, request, inventory, trainer);
    if (!result.success) {
        screen.postStatus(result.error.empty() ? "Verified Generation III export failed" : result.error, 480);
        return;
    }
    screen.postStatus("Verified copy exported: " + result.directory + "/edited.sav (source unchanged)", 600);
}

bool handleReview(TrainerViewScreen& screen, State& state, u64 down, const TouchInput& touch) {
    if (!state.review) {
        state.confirmDiscardAll = false;
        return false;
    }
    auto* staged = editor(screen);
    if (!staged) {
        state.review = false;
        state.confirmDiscardAll = false;
        return true;
    }
    if (state.confirmDiscardAll && !(down & HidNpadButton_Y)) {
        // B keeps every staged edit; A/X/D-pad/touch are inert while the question is open.
        if (down & HidNpadButton_B) {
            state.confirmDiscardAll = false;
            screen.controllerNavigation.reset();
        }
        return true;
    }
    const auto rows = reviewRows(screen);
    (void)touch;
    namespace Review=PokeBank::UIModel::SharedReview;
    if(down&(HidNpadButton_Left|HidNpadButton_Up|HidNpadButton_L))
        state.reviewScroll=static_cast<int>(Review::move(state.reviewScroll,-1,rows.size()));
    if(down&(HidNpadButton_Right|HidNpadButton_Down|HidNpadButton_R))
        state.reviewScroll=static_cast<int>(Review::move(state.reviewScroll,1,rows.size()));
    if (down & HidNpadButton_B) {
        state.reviewTouchScroll.reset();
        state.review = false;
        state.actions = screen.selectedMode != TrainerViewScreen::ViewMode::Trainer;
        if (PokeBank::UIModel::ClassicReviewOrigin::takeStandalone(screen)) state.actions = false; // standalone Review: back to the screen underneath
        screen.controllerNavigation.reset();
        return true;
    }
    if ((down & HidNpadButton_X) && !rows.empty()) {
        exportStagedGen3Copy(screen);
        return true;
    }
    if (down & HidNpadButton_Y) {
        if (!state.confirmDiscardAll) {
            // First Y only asks; an explicit second Y discards every staged edit.
            if (!rows.empty()) state.confirmDiscardAll = true;
            screen.controllerNavigation.reset();
            return true;
        }
        state.confirmDiscardAll = false;
        state.reviewTouchScroll.reset();
        staged->discard();
        if (auto* inventory = bridge(screen).stagedInventory()) {
            inventory->discard();
            (void)ClassicInventory::refreshPresentation(screen);
        }
        if (auto* trainer = bridge(screen).stagedTrainer()) {
            trainer->discard();
            bridge(screen).refreshStagedTrainerPresentation();
        }
        refreshPresentation(screen);
        state.review = false;
        state.actions = screen.selectedMode != TrainerViewScreen::ViewMode::Trainer;
        if (PokeBank::UIModel::ClassicReviewOrigin::takeStandalone(screen)) state.actions = false; // standalone Review: back to the screen underneath
        screen.controllerNavigation.reset();
        screen.postStatus("All staged changes discarded - still editing; the original save is unchanged", 360);
        return true;
    }
    return true;
}

bool handleReleaseConfirm(TrainerViewScreen& screen, State& state, u64 down) {
    if (!state.confirmRelease) return false;
    if (down & HidNpadButton_B) {
        state.confirmRelease = false;
        state.actions = true;
        screen.controllerNavigation.reset();
        return true;
    }
    // A never stages a destructive edit. Release requires an explicit
    // second-step Y confirmation; B always returns to Actions unchanged.
    if (!(down & HidNpadButton_Y)) return true;
    auto* staged = editor(screen);
    if (!staged) return true;
    std::string error;
    if (!staged->stageReleaseBoxPokemon(
            static_cast<std::size_t>(state.box), static_cast<std::size_t>(state.slot), error)) {
        screen.postStatus(error.empty() ? "Generation III Release failed" : error, 360);
    } else {
        refreshPresentation(screen);
        screen.postStatus("Generation III Release staged; source save unchanged", 300);
    }
    state.confirmRelease = false;
    return true;
}

std::string surfaceContext(const TrainerViewScreen& screen, const State& state) {
    Context::SurfaceMode mode = Context::SurfaceMode::ViewReadOnly;
    if (state.session.mode == SessionModel::Mode::Edit) mode = Context::SurfaceMode::EditStaged;
    else if (state.session.mode == SessionModel::Mode::Create) mode = Context::SurfaceMode::CreateDraft;
    if (state.passive) {
        if (screen.details.source == TrainerViewScreen::EditSource::Party)
            return "Party Slot " + std::to_string(screen.details.partyIndex + 1);
        return "Legacy Storage / Slot " + std::to_string(screen.selectedItemIndex + 1);
    }
    return Context::contextLine(
        state.party?"Party "+std::to_string(state.slot+1):
            "Box " + std::to_string(state.box + 1) + " / Slot " + std::to_string(state.slot + 1),
        screen.sourceGameId, mode);
}

void drawPanel(PKSEFramebuffer& fb, int x, int y, int w, int h, const char* title) {
    // Round 7 P2: the shared Gen IV / V panel and heading (UI/SharedPanelHeading.h).
    drawPanelSurface(fb, x, y, w, h, false, 14);
    PokeBank::UIModel::PanelHeading::draw(fb, x, y, title);
}

void drawFocus(PKSEFramebuffer& fb, int x, int y, int w, int h, bool selected) {
    if (selected) fb.drawRoundedRect(x, y, w, h, 6, Colors::FocusBorder, 2);
}

void drawValuePicker(TrainerViewScreen& screen, PKSEFramebuffer& fb, State& state) {
    if (!state.valuePicker || state.pickerValues.empty()) return;
    SharedEditorChrome::beginModal(screen.touchButtons);
    const bool heldItems = state.pickerTarget == PickerTarget::HeldItem;
    const bool movePicker = state.pickerTarget == PickerTarget::Move;
    constexpr auto moveLayout = MoveUI::compactPickerLayout();
    const int pickerCount=static_cast<int>(state.pickerValues.size());
    const bool compactChoices=state.pickerTarget==PickerTarget::Language ||
        state.pickerTarget==PickerTarget::Ball ||
        state.pickerTarget==PickerTarget::MetLocation ||
        state.pickerTarget==PickerTarget::Nature ||
        state.pickerTarget==PickerTarget::Ability ||
        state.pickerTarget==PickerTarget::Gender || state.pickerTarget==PickerTarget::Form;
    const bool naturePicker=state.pickerTarget==PickerTarget::Nature;
    const auto natureLayout=PokeBank::UIModel::NaturePickerLayout::forChoices(pickerCount);
    const int visibleChoices=naturePicker?natureLayout.visibleRows:
        compactChoices?std::min(8,pickerCount):10;
    const auto valueList = valueListLayout(state.pickerTarget, pickerCount);
    const int w = heldItems ? HeldItemGrid::modalWidth : movePicker ? moveLayout.width :
        naturePicker ? natureLayout.width : compactChoices ? valueList.frame.w : 680;
    const int h = heldItems ? HeldItemGrid::modalHeight : movePicker ? moveLayout.height :
        naturePicker ? natureLayout.height : compactChoices ? valueList.frame.h : 560;
    const int x = (fb.getWidth() - w) / 2;
    const int y = (fb.getHeight() - kNavBarH - h) / 2;
    SharedPopupDismiss::publish(x, y, w, h);   // 7D: a tap outside closes it like B

    // Keep every Gen III picker in the same dark modal family as Gen I/II.
    fb.drawFilledRect(0, 0, fb.getWidth(), fb.getHeight(), Color(0, 0, 0, 165));
    fb.drawFilledRoundedRect(x, y, w, h, 18, Colors::Panel);
    fb.drawRoundedRect(x, y, w, h, 18, Colors::Divider, 1);

    if (movePicker) {
        fb.drawText(x + 24, y + 18, "Move — Generation III", Colors::Text, TextStyle::Heading);
        {
            const std::string sort = std::string("Sort: ") + ItemPickerSort::label(state.pickerSort);
            int sw = 0, sh = 0;
            fb.measureText(sort, sw, sh, TextStyle::Caption);
            fb.drawText(x + w - 24 - sw, y + 26, sort, Colors::TextDim, TextStyle::Caption);
        }
        const auto columns = SharedMovesPanel::chooserColumns(x + 40, w - 80);
        SharedMovesPanel::drawChooserCaptions(fb, columns, y + 58, kGen3MoveGame);
        const int count = static_cast<int>(state.pickerValues.size());
        constexpr int visible = moveLayout.visibleRows;
        // A picker opened this frame still has an unset viewport (firstRow -1): settle it the way
        // input does, so the first draw never reads pickerValues[-1] or shifts its hitboxes.
        state.pickerViewport.ensure(state.pickerRow, count, visible);
        const int first = state.pickerViewport.firstRow;
        const int last = std::min(count, first + visible);
        const int viewportTop = y + moveLayout.rowStart;
        fb.setClipRect(x + 24, viewportTop, w - 48, visible * moveLayout.rowStep);
        for (int i = first; i < last; ++i) {
            const int yy = viewportTop + (i - first) * moveLayout.rowStep + state.pickerScroll.offset();
            appendClippedTouchButton(screen.touchButtons, 4000 + i,
                x + 24, yy, w - 48, moveLayout.highlightHeight,
                x + 24, viewportTop, w - 48, visible * moveLayout.rowStep);
            const bool selected = i == state.pickerRow;
            SharedBoxedRows::drawBox(fb, SharedBoxedRows::rect(x + 24, yy, w - 48, moveLayout.highlightHeight), {selected});  // R2-BUG-012 boxed row
            const auto value = state.pickerValues[static_cast<std::size_t>(i)];
            SharedMovesPanel::drawChooserRow(fb, columns, yy + 6, value, kGen3MoveGame, selected);
        }
        fb.clearClip();
        SharedScrollbar::drawAndPublish(fb, state.pickerScrollbar, x + w - 18, viewportTop,
            SharedScrollbar::kTrackWidth, visible * moveLayout.rowStep,
            SharedScrollbar::rows(count, visible, first));
        Presentation::PickerFooterSpec footer;
        footer.jump = true;
        footer.xLabel = "Sort";
        SharedEditorChrome::drawPickerFooter(fb, footer);
        return;
    }

    const int count = static_cast<int>(state.pickerValues.size());
    if (heldItems) {
        const int heldColumns=HeldItemGrid::columnsForSearch(state.heldItemSearch.open());
        state.pickerViewport.ensure(state.pickerRow, count, HeldItemGrid::rows, heldColumns);   // see the list draws below
        SharedHeldItemPickerPresentation::drawHeading(
            fb, x, y, "Held Item — Generation III", state.pickerRow, state.heldItemSearch.noMatches?0:count,state.heldItemSearch.open());
        fb.drawText(x + w - 250, y + 55,
                    std::string("Sort: ") + ItemPickerSort::label(state.pickerSort),
                    Colors::TextDim, TextStyle::Caption);
        if (const std::string tag = ItemPickerSearch::tag(state.pickerQuery); !tag.empty()) {
            // NEW-06: the active search, top right on the title line.
            int tw = 0, th = 0;
            fb.measureText(tag, tw, th, TextStyle::Caption);
            fb.drawText(x + w - 28 - tw, y + 26, tag, Colors::Accent, TextStyle::Caption);
        }
        // R2-BUG-005: a focused Gen III TM/HM shows its move. TM01-50 / HM01-08 teach the same
        // moves in Ruby/Sapphire/Emerald and FireRed/LeafGreen (pret pokeemerald / pokefirered),
        // so the FireRed catalog row is exact for every Gen III game.
        if (state.pickerRow >= 0 && state.pickerRow < count)
            UI::SharedItemGridPicker::drawMachineDetail(fb, x + 150, y + 55, w - 420, kGen3MoveGame,
                PokeVault::Inventory::classicMachineMove(PokeVault::Inventory::ClassicGame::FireRed,
                    state.pickerValues[static_cast<std::size_t>(state.pickerRow)]));
        SharedHeldItemPickerPresentation::drawGrid(
            fb, x, y, state.pickerValues, state.pickerRow,
            [&](uint16_t value) { return pickerValueLabel(state, value); },
            [&](uint16_t value) {
                const auto disc = UI::ClassicMachineArtwork::artKey(
                    screen.trainer.getGameGroup(), value);
                return disc.empty() ? Names::getItemNameG3(value) : disc;
            },
            state.pickerViewport.firstRow, state.pickerScroll.offset(),&state.heldItemSearch);
        const int gridX = x + HeldItemGrid::gridInsetX;
        const int gridY = y + HeldItemGrid::gridInsetY;
        const int cellHeight = HeldItemGrid::rowPitch;
        const int cellW = HeldItemGrid::cellWidth;
        const int firstRow = state.pickerViewport.firstRow;
        const int firstVisibleItem = state.pickerViewport.firstRow * heldColumns + 1;
        const int lastVisibleItem = std::min(count,
            firstVisibleItem + HeldItemGrid::pageSizeForSearch(state.heldItemSearch.open()) - 1);
        (void)lastVisibleItem;
        for (int i = std::max(0, (firstRow - 1) * heldColumns);
             !state.heldItemSearch.noMatches && i < std::min(count, (firstRow + HeldItemGrid::rows + 1) * heldColumns); ++i) {
            const int offset = i - firstRow * heldColumns;
            const int cellX = gridX + offset % heldColumns * cellW;
            const int cellY = gridY + offset / heldColumns * cellHeight + state.pickerScroll.offset();
            appendClippedTouchButton(screen.touchButtons, 4000 + i,
                cellX, cellY - 3, cellW - 10, cellHeight - 4,
                gridX, gridY - 3, HeldItemGrid::gridWidth(state.heldItemSearch.open()), HeldItemGrid::rows * cellHeight);
        }
        Presentation::PickerFooterSpec footer;
        footer.gridPaging = true;
        footer.xLabel = "Sort";
        footer.minusLabel = "Search";   // NEW-06
        if(state.heldItemSearch.open() || state.heldItemSearch.noMatches)
            SharedHeldItemSearch::drawFooter(fb,state.heldItemSearch);
        else
        SharedEditorChrome::drawPickerFooter(fb, footer);
        return;
    }
    fb.drawText(x + 28, y + 20, pickerTitle(state.pickerTarget),
                Colors::Text, TextStyle::Heading);

    fb.drawText(x + 28, y + 55,
                std::to_string(state.pickerRow + 1) + " / " +
                    std::to_string(state.pickerValues.size()),
                Colors::TextDim, TextStyle::Caption);
    const int visible = compactChoices ? visibleChoices : 10;
    // A picker opened this frame still has an unset viewport (firstRow -1): settle it the way
    // input does, so the first draw never reads pickerValues[-1] or shifts its hitboxes.
    state.pickerViewport.ensure(state.pickerRow, count, visible);
    const int first = state.pickerViewport.firstRow;
    const int last = std::min(count, first + visible);
    const int rowStep = naturePicker ? natureLayout.rowStep : compactChoices ? valueList.rule.rowHeight : 42;
    const int choiceHeight=naturePicker? natureLayout.highlightHeight : rowStep-4;
    const int viewportTop = y + 90;
    // NEW-08: a scrolling list keeps a scrollbar gutter, so the bar never crowds the rows
    // (rows, clip, touch boxes and the boxed highlight all use rowW).
    const int scrollbarGap = SharedScrollbar::kTouchSlop + 4;
    const int barGutter = count > visible ? SharedScrollbar::kTrackWidth + scrollbarGap : 0;
    const int rowW = w - 44 - barGutter;
    fb.setClipRect(x + 22, viewportTop - 4, rowW, visible * rowStep);
    for (int i = first; i < last; ++i) {
        const int yy = viewportTop + (i - first) * rowStep + state.pickerScroll.offset();
        appendClippedTouchButton(screen.touchButtons, 4000 + i,
            x + 22, yy - 4, rowW, choiceHeight,
            x + 22, viewportTop - 4, rowW, visible * rowStep);
        const bool selected = i == state.pickerRow;
        SharedBoxedRows::drawBox(fb, SharedBoxedRows::rect(x + 22, yy - 4, rowW, choiceHeight), {selected});  // R2-BUG-012 boxed row
        const auto value = state.pickerValues[static_cast<std::size_t>(i)];
        if (state.pickerTarget == PickerTarget::MetLocation) {
            // Bright place name on the left, method / level on the right; nothing overlaps.
            const auto row = metLocationChoiceText(state, value);
            int nw = 0, nh = 0, dw = 0, dh = 0;
            fb.measureText(row.first, nw, nh, TextStyle::Body);
            // Round 5 NEW-10: the detail gets what the measured name leaves, so at 620 px every
            // real row shows both in full (tests/test_met_location_fit.cpp).
            const std::string detail = SharedMovesPanel::fit(fb, row.second, std::max(0, rowW - 32 - nw - 20));
            fb.measureText(detail, dw, dh, TextStyle::Caption);
            fb.drawText(x + 38, yy + 7,
                        SharedMovesPanel::fit(fb, row.first, rowW - 32 - dw - 20, TextStyle::Body),
                        Colors::Text, TextStyle::Body);
            fb.drawText(x + 22 + rowW - 16 - dw, yy + 10, detail,
                        selected ? Colors::Text : Colors::TextSecondary, TextStyle::Caption);
            continue;
        }
        if (naturePicker) {
            fb.drawText(x + 38, yy + 7, pickerValueLabel(state, value), Colors::Text, TextStyle::Body);
            NatureStats::drawEffect(fb, x + 166, yy + 10, value);
            continue;
        }
        if (state.pickerTarget == PickerTarget::Ability) {
            SharedAbilityPicker::drawRow(fb,x+22,yy-4,rowW,choiceHeight,
                pickerValueLabel(state,value),selected);
            continue;
        }
        if (state.pickerTarget == PickerTarget::Ball) {
            // The ball sprite leads its name at the BallPicker rule's gap (no far-right sprite).
            const auto cell = PokeBank::UIModel::SharedMenuGeometry::Rect{x + 22, yy - 4, rowW, 38};
            const auto sprite = PokeBank::UIModel::SharedMenuGeometry::spriteRect(valueList, cell);
            ItemPickerArtwork::draw(fb, sprite.right(), sprite.y, sprite.w,
                                    Enums::getBallName(static_cast<uint8_t>(value)));
            fb.drawText(PokeBank::UIModel::SharedMenuGeometry::labelRect(valueList, cell).x, yy + 7,
                        pickerValueLabel(state, value), Colors::Text, TextStyle::Body);
            continue;
        }
        fb.drawText(x + 38, yy + 7, pickerValueLabel(state, value),
                    Colors::Text, TextStyle::Body);
    }
    fb.clearClip();
    SharedScrollbar::drawAndPublish(fb, state.pickerScrollbar, x + 22 + rowW + scrollbarGap, viewportTop - 4,
        SharedScrollbar::kTrackWidth, visible * rowStep,
        SharedScrollbar::rows(count, visible, first));
    Presentation::PickerFooterSpec footer;
    footer.page = count > visible;
    footer.jump = count > 50;
    SharedEditorChrome::drawPickerFooter(fb, footer);
}

void drawMoveEditor(TrainerViewScreen& screen, PKSEFramebuffer& fb, State& state) {
    if (!state.moveEditor) return;
    const auto slot = static_cast<std::size_t>(state.moveEditorSlot);
    constexpr int w = 650, h = 390;
    const int x = (fb.getWidth() - w) / 2;
    const int y = (fb.getHeight() - kNavBarH - h) / 2;
    SharedEditorChrome::beginModal(screen.touchButtons);
    fb.drawFilledRect(0, 0, fb.getWidth(), fb.getHeight() - kNavBarH, Color(0, 0, 0, 95));
    fb.drawFilledRoundedRect(x, y, w, h, 18, Colors::Panel);
    fb.drawRoundedRect(x, y, w, h, 18, Colors::Divider, 1);
    fb.drawText(x + 24, y + 18, "Move " + std::to_string(state.moveEditorSlot + 1),
                Colors::Text, TextStyle::Heading);
    {
        const std::string detail = PokeBank::UIModel::SharedMoveFacts::detailLine(
            PokeBank::UIModel::SharedMoveFacts::facts(state.session.working.moves[slot], kGen3MoveGame));
        fb.drawText(x + 24, y + 52, detail.empty() ? std::string("Empty move slot") : detail,
                    Colors::TextDim, TextStyle::Caption);
    }
    const uint16_t move = state.session.working.moves[slot];
    const std::array<std::pair<std::string, std::string>, 5> rows{{
        {"Move", move == 0 ? std::string("Empty") : std::string(Names::getMoveName(move))},
        {"PP", std::to_string(state.session.working.pp[slot]) + "/" +
               std::to_string(gen3MoveMaxPP(move, state.session.working.ppUps[slot]))},
        {"PP Ups", std::to_string(state.session.working.ppUps[slot])},
        {"Apply", "Keep these move details"},
        {"Cancel", "Discard these move details"},
    }};
    int rowY = y + 88;
    for (int i = 0; i < static_cast<int>(rows.size()); ++i) {
        const bool selected = i == state.moveEditorRow;
        screen.touchButtons.push_back({31400 + i, x + 22, rowY, w - 44, 42});
        SharedBoxedRows::drawBox(fb, SharedBoxedRows::rect(x + 22, rowY, w - 44, 42), {selected});  // R2-BUG-012 boxed row
        fb.drawText(x + 38, rowY + 10, rows[static_cast<std::size_t>(i)].first,
                    selected ? Colors::Text : Colors::TextDim);
        int valueW = 0, valueH = 0;
        fb.measureText(rows[static_cast<std::size_t>(i)].second, valueW, valueH, TextStyle::Body);
        fb.drawText(x + w - 38 - valueW, rowY + 10, rows[static_cast<std::size_t>(i)].second,
                    selected ? Colors::Text : Colors::TextDim);
        rowY += 48;
    }
    Presentation::PickerFooterSpec footer;
    footer.page = false;
    SharedEditorChrome::drawPickerFooter(fb, footer);
}

void drawWorkspace(TrainerViewScreen& screen, PKSEFramebuffer& fb, State& state) {
    // Preview uses the exact serializer on a private copy only when the draft changes.
    // Failed drafts never display stale computed stats as if they were valid.
    if (state.session.editable() && (!state.previewValid ||
        !SessionModel::sameEditableRecord(state.previewInput, state.session.working))) {
        state.previewInput = state.session.working;
        state.previewError.clear();
        if (auto* staged = editor(screen)) {
            auto preview = state.session.mode == SessionModel::Mode::Create
                ? staged->previewCreate(state.box, state.slot, state.session.createRequest(), state.previewError)
                : state.party?staged->previewPartyEdit(state.slot,state.session.editRequest(),state.previewError)
                : staged->previewEdit(state.box, state.slot, state.session.editRequest(), state.previewError);
            if (preview) {
                state.previewRecord = *preview;
                // Unrequested PID-linked values follow the exact preview, so toggles start from
                // the displayed Gender/Shiny/Ability. Re-arm the cache key with the adopted draft.
                state.session.adoptPreview(*preview);
                state.previewInput = state.session.working;
            }
        } else state.previewError = "Exact preview unavailable";
        state.previewValid = true;
    }
    const auto& p = state.session.editable() && state.previewError.empty()
        ? state.previewRecord : state.session.working;
    screen.touchButtons.clear();
    auto mode = Context::SurfaceMode::ViewReadOnly;
    if (state.session.mode == SessionModel::Mode::Edit) mode = Context::SurfaceMode::EditStaged;
    if (state.session.mode == SessionModel::Mode::Create) mode = Context::SurfaceMode::CreateDraft;
    SharedPokemonShell::drawChrome(fb, p.species, p.nickname, p.level, p.gender, p.shiny,
        mode, state.passive ? surfaceContext(screen, state) :
        state.party?"Party "+std::to_string(state.slot+1):
            "Box " + std::to_string(state.box + 1) + " / Slot " + std::to_string(state.slot + 1), screen.sourceGameId);
    const SharedPokemonShell::Geometry geometry(fb.getWidth(), fb.getHeight(), kNavBarH);
    const int y = geometry.y, h = geometry.h;
    const int leftX = geometry.leftX, leftW = geometry.leftW;
    const int midX = geometry.midX, midW = geometry.midW;
    const int rightX = geometry.rightX, rightW = geometry.rightW;

    drawPanel(fb, leftX, y, leftW, h, "DETAILS");
    SharedPokemonShell::drawPortrait(fb, leftX, y, p.species, p.shiny);
    const auto& personal = Pokemon::getPersonalInfoG3(p.species);
    const std::array<uint8_t,2> types{personal.type1, personal.type2};
    int typeX = leftX + 66;
    for (std::size_t i = 0; i < types.size(); ++i) {
        if (types[i] == 255 || (i && types[i] == types[0])) continue;
        if (auto* sprite = SpriteManager::getTypeSprite(types[i]); sprite && sprite->data && sprite->height > 0) {
            const int width = sprite->width * 22 / sprite->height;
            fb.drawImageScaled(typeX, y + 180, sprite->width, sprite->height, width, 22, sprite->data, sprite->channels);
            typeX += width + 8;
        }
    }
    constexpr std::size_t detailVisibleRows = 8;
    constexpr int detailRowStep = 38;
    const auto detailViewport = SharedPokemonShell::detailsViewport(geometry);
    const int detailViewportY = detailViewport.top;
    const int detailViewportH = detailViewport.height;
    state.detailsViewport.ensure(state.detailsFocus, static_cast<int>(detailLabels.size()),
                                 static_cast<int>(detailVisibleRows));
    const int firstDetailRow = state.detailsViewport.firstRow;
    const int detailPixelOffset = state.detailsTouchScroll.offset();
    SharedPokemonShell::drawScrollableDetails(fb, leftX, y, leftW, h,
        detailLabels.size(), static_cast<std::size_t>(state.detailsFocus),
        state.focus.panel == Shared::Panel::Details,
        [](std::size_t row) { return detailLabels[row]; },
        [&](std::size_t row) {
            // Round 11 (H07 for all gens): a new Pokemon's Met Location is not chosen until picked.
            if (row == 13 && state.session.mode == SessionModel::Mode::Create && !state.session.metLocationChosen &&
                state.session.metLocationChoosable())
                return std::string("Choose a location");
            return detailsValue(screen, p, static_cast<int>(row));
        },
        [&](std::size_t row) {
            return detailLocked(p, static_cast<int>(row))
                ? PokeBank::UIModel::SharedFieldState::State::ReadOnly
                : PokeBank::UIModel::SharedFieldState::State::Editable;
        },
        detailPixelOffset, firstDetailRow, &state.detailsScrollbar);
    {
        // Hitboxes follow the exact scroll position and are clipped to the
        // visible list. Never publish hidden rows through the footer.
        for (int row = std::max(0, firstDetailRow - 1);
             row < std::min(static_cast<int>(detailLabels.size()),
                            firstDetailRow + static_cast<int>(detailVisibleRows) + 1); ++row) {
            const int yy = detailViewport.firstRowY + (row - firstDetailRow) * detailRowStep + detailPixelOffset;
            appendClippedTouchButton(screen.touchButtons, 31000 + row,
                leftX + 8, yy + 2, leftW - 36, 30,
                leftX + 8, detailViewportY, leftW - 16, detailViewportH);
        }
    }
    drawPanel(fb, midX, y, midW, h, "STATS");
    fb.drawText(midX + 126, y + 50, "IV", Colors::TextDim, TextStyle::Caption);
    fb.drawText(midX + 190, y + 50, "EV", Colors::TextDim, TextStyle::Caption);
    fb.drawText(midX + 274, y + 50, "Stat", Colors::TextDim, TextStyle::Caption);
    constexpr std::array<const char*,6> statLabels{{"HP","Attack","Defense","Speed","Sp. Atk","Sp. Def"}};
    for (int row = 0; row < 6; ++row) {
        const int yy = y + 78 + row * 40;
        const bool selected = state.focus.panel == Shared::Panel::Values &&
                              state.focus.row == row;
        screen.touchButtons.push_back({31100 + row, midX + 112, yy - 5, 62, 30});
        screen.touchButtons.push_back({31120 + row, midX + 176, yy - 5, 70, 30});
        if (selected) {
            const auto cell = Shared::cellFocusFor(Shared::Generation::Gen3, state.focus);
            drawFocus(fb, midX + cell.x, yy - 5, cell.width, 30, true);
        }
        fb.drawText(midX + 18, yy, statLabels[static_cast<std::size_t>(row)],
                    NatureStats::statColor(p.nature, row, selected ? Colors::SelectedText : Colors::TextDim), TextStyle::Caption);
        fb.drawText(midX + 126, yy, std::to_string(p.ivs[static_cast<std::size_t>(row)]), Colors::Text, TextStyle::Caption);
        fb.drawText(midX + 190, yy, std::to_string(p.evs[static_cast<std::size_t>(row)]), Colors::Text, TextStyle::Caption);
        fb.drawText(midX + 274, yy, state.previewError.empty() ? std::to_string(p.calculatedStats[static_cast<std::size_t>(row)]) : "—",
                    SharedFieldStateColors::color(PokeBank::UIModel::SharedFieldState::Token::Gold), TextStyle::Caption);   // Phase 4: derived stat, gold ReadOnly
    }
    fb.drawHDivider(midX + 14, y + 310, midW - 28);
    fb.drawText(midX + 18, y + 318, "PID-LINKED / SAFE EDIT",
                Colors::TextDim, TextStyle::Caption);
    const std::array<std::pair<const char*,std::string>,5> derived{{
        {"Nature", Trainer::getNatureName(p.nature)},
        {"Gender", genderText(p.gender)},
        {"Shiny", p.shiny ? "Yes" : "No"},
        {"Ability", Trainer::getAbilityName(p.ability)},
        {"PID", std::to_string(p.pid)},
    }};
    for (int i = 0; i < 5; ++i) {
        const int row = i + 6;
        const int yy = y + 346 + i * 35;
        const bool editableRow = row < 10 && pidLinkedRowEditable(state, row);
        const bool passiveView = state.session.mode == SessionModel::Mode::View;
        // Read-only data is still information, not a disabled control. Keep the derived
        // block readable in passive View mode, and keep PID readable in Create/Edit
        // even though PID itself remains derived and non-editable.
        const bool readableReadOnly = !editableRow && (passiveView || i == 4);
        const bool selected = state.focus.panel == Shared::Panel::Values &&
                              state.focus.row == row &&
                              (passiveView || editableRow);
        screen.touchButtons.push_back({31200 + row, midX + 8, yy - 5, midW - 16, 29});
        drawFocus(fb, midX + 8, yy - 5, midW - 16, 29, selected);
        const Color labelColor = selected ? Colors::SelectedText
            : editableRow ? Colors::TextDim
            : readableReadOnly ? Colors::TextSecondary
            : Colors::Divider;
        fb.drawText(midX + 18, yy, derived[static_cast<std::size_t>(i)].first,
                    labelColor, TextStyle::Caption);
        Color valueColor = selected ? Colors::SelectedText
            : editableRow ? Colors::Text
            : readableReadOnly ? Colors::Text
            : Colors::TextDim;
        if (i == 1 && editableRow && !selected) valueColor = genderValueColor(p.gender);
        if (i == 4) valueColor = SharedFieldStateColors::color(PokeBank::UIModel::SharedFieldState::Token::Gold);   // PID: derived, gold ReadOnly
        fb.drawText(midX + 130, yy, derived[static_cast<std::size_t>(i)].second,
                    valueColor, TextStyle::Caption);
        // Same as the shared Gen IV/V panel: the PID status sits on its own subordinate line so
        // the label never runs into the 10-digit value column.
        if (i == 4)
            fb.drawText(midX + 18, yy + 15, "read-only", Colors::TextDim, TextStyle::Caption);
    }

    drawPanel(fb, rightX, y, rightW, h, "MOVES");
    {
        // The ONE shared Moves panel (UI/SharedMovesPanel.h), same as Gen IV / V.
        const auto movesLayout = SharedMovesPanel::layout(rightX, y, rightW);
        const auto visibleMoveFocus = Shared::normalizeMoveRowFocus(Shared::Generation::Gen3, state.focus);
        const auto descriptor =
            PokeVault::Integration::Gen3EditorProvider::descriptorForSource(screen.sourceGameId, true);
        using MoveResult = PokeBank::UIModel::ExactFormatEditor::MoveCompatibilityResult;
        std::array<SharedMovesPanel::Slot, 4> slots{};
        for (int row = 0; row < 4; ++row) {
            const auto i = static_cast<std::size_t>(row);
            const uint16_t move = p.moves[i];
            auto& slot = slots[i];
            slot.move = move;
            slot.pp = p.pp[i];
            slot.ppUps = p.ppUps[i];
            slot.maxPP = gen3MoveMaxPP(move, p.ppUps[i]);
            screen.touchButtons.push_back({31300 + row, movesLayout.focusX(), movesLayout.rowTop(row),
                                           movesLayout.focusW(), SharedMovesPanel::Layout::kRowHeight});
            if (move == 0) continue;
            bool existingSource = false;
            for (uint16_t sourceMove : state.session.baseline.moves)
                if (sourceMove != 0 && sourceMove == move) existingSource = true;
            auto result = MoveResult::Invalid;
            if (descriptor)
                result = descriptor->moves.evaluate(
                    {screen.sourceGameId, p.species, 0, move, existingSource});
            slot.status = "Invalid";
            slot.statusColor = Colors::Error;
            if (result == MoveResult::Compatible) {
                slot.status = "OK";
                slot.statusColor = Colors::Success;
            } else if (result == MoveResult::PreserveExisting) {
                slot.status = "Preserved";
                slot.statusColor = Colors::Warning;
            } else if (result == MoveResult::Unsupported) {
                slot.status = "Transfer";
                slot.statusColor = Colors::Warning;
            }
        }
        SharedMovesPanel::draw(fb, movesLayout, slots, kGen3MoveGame,
            visibleMoveFocus.panel == Shared::Panel::Moves ? static_cast<int>(visibleMoveFocus.row) : -1);
    }
    struct DataRow { std::string label, value; };
    // Same data pane as Gen IV/V: the PID is shown once, in gold (ReadOnly field state), in the
    // STATS panel's PID-linked block; no "(read-only)" text row here (QA CG16).
    const std::vector<DataRow> rows{
        {"Origin", Enums::getOriginGameName(p.originGame)},
        {"Language", Enums::getLanguageName(p.language)},
        {"Ball", Enums::getBallName(p.ball)},
        {"Met Location", metLocationText(p)},
        {"Encounter origin", encounterOriginText(screen, p)},
        {"Legality", Presentation::legalityLabel(Presentation::LegalityState::NotChecked)}
    };
    SharedPokemonShell::drawDataAndGraph(fb, rightX, y + 244, rightW, h - 256,
        Presentation::kOriginLegalityHeading, rows, state.previewError.empty() ? p.calculatedStats : std::array<uint16_t,6>{});

    if (state.session.mode == SessionModel::Mode::View) {
        drawNavBar(fb, {{"D-pad/Stick", "Navigate"}, {"L/R", "Panel"}, {"B", "Back"}});
    } else if (state.session.mode == SessionModel::Mode::Create) {
        drawNavBar(fb, {{"D-pad/Stick", "Navigate"}, {"L/R", "Panel"}, {"A", "Edit"},
                        {"Y", PokeBank::UIModel::SharedStatReroll::kRerollHint},
                        {"X", "Add"}, {"+", "Review"}, {"B", "Exit"}});
    } else {
        drawNavBar(fb, {{"D-pad/Stick", "Navigate"}, {"L/R", "Panel"}, {"A", "Edit"},
                        {"Y", PokeBank::UIModel::SharedStatReroll::kRerollHint},
                        {"X", "Keep"}, {"+", "Review"}, {"B", "Exit"}});
    }

    if (state.session.confirmExit) {
        // Reuse PKSE's native creator/details confirmation chrome. The Generation III
        // session still targets only the staged copy; original source bytes remain immutable.
        const bool create = state.session.mode == SessionModel::Mode::Create;
        const int w = create ? 540 : 560;
        constexpr int hh = 226;
        const int xx = (fb.getWidth() - w) / 2;
        const int yy = (fb.getHeight() - hh) / 2;
        const char* title = create ? "New Pokémon" : "Unsaved changes";
        const Color accent = create ? Colors::Primary : Colors::Orange;
        SharedEditorChrome::beginModal(screen.touchButtons);
        const int cy = Dialogs::drawDialogFrame(fb, xx, yy, w, hh, title, accent);

        fb.drawText(xx + 28, cy,
                    create ? "Keep this new Pokémon?" : "This Pokémon has unsaved changes.",
                    Colors::Text);
        fb.drawText(xx + 28, cy + 34,
                    create ? "Discard removes it; Back keeps editing."
                           : "Discard loses them; Back keeps editing.",
                    Colors::TextDim, TextStyle::Caption);

        const int cbw = 160, cbh = TouchTargetMin, cby = yy + hh - cbh - 18;
        drawGlyphButton(fb, xx + 24, cby, cbw, cbh, "B", "Back", Colors::PanelAlt);
        drawGlyphButton(fb, xx + (w - cbw) / 2, cby, cbw, cbh, "Y", "Discard", Colors::PanelAlt);
        drawGlyphButton(fb, xx + w - 24 - cbw, cby, cbw, cbh, "A",
                        create ? "Keep" : "Save", Colors::PanelAlt);
        SharedEditorChrome::drawFooter(fb, Presentation::confirmFooter(
            "A", create ? "Keep" : "Save", "Y", "Discard"));
    }

    if (state.moveEditor) drawMoveEditor(screen, fb, state);
    if (state.speciesPicker) {
        SharedEditorChrome::beginModal(screen.touchButtons);
        const auto modal = SharedSpeciesPicker::drawModalChrome(fb);
        SharedPopupDismiss::publish(modal.x, modal.y, modal.width, modal.height);   // 7D
        SharedSpeciesPicker::refilter(state.speciesChooser, 386, gen3SpeciesAvailable, gen3SpeciesName);
        SharedSpeciesPicker::drawContent(
            fb, modal.x, modal.y, state.speciesPreview, 386, state.speciesPreviewShiny,
            [](uint16_t species) {
                return std::to_string(species) + "  " + Names::getSpeciesName(species);
            },
            [](uint16_t species) {
                return SharedSpeciesPicker::selectedTitle(species, Names::getSpeciesName(species));
            },
            &latestTouchGesture(), true, {0xFF, 0xFF}, &state.speciesChooser);
        // Touch ids stay 31500 + species; the chooser publishes the rows it actually drew
        // (speciesStart is the dex number the drawn window would start at if it were contiguous,
        // so the id arithmetic reads as before: 31500 + speciesStart + i == 31500 + species).
        for (int i = 0; i < state.speciesChooser.rowCount; ++i) {
            const auto& row = state.speciesChooser.rows[static_cast<std::size_t>(i)];
            const int speciesStart = static_cast<int>(row.species) - i;
            screen.touchButtons.push_back({31500 + speciesStart + i, row.x, row.y, row.w, row.h});
        }
        Presentation::PickerFooterSpec footer;
        footer.jump = true;
        footer.xLabel = "Search";
        footer.yLabel = "Normal/Shiny";
        SharedEditorChrome::drawPickerFooter(fb, footer);
    }
    if (state.valuePicker) drawValuePicker(screen, fb, state);
}

void drawLegality(TrainerViewScreen& screen, PKSEFramebuffer& fb, State& state) {
    namespace P=PokeBank::UIModel::SharedEditorPresentation;
    screen.drawGSCOverlay(fb);std::string error;
    auto* staged=editor(screen);
    const auto p=!staged?std::nullopt:state.party?staged->partyPokemon(static_cast<size_t>(state.slot),error):
        staged->boxedPokemon(static_cast<size_t>(state.box),static_cast<size_t>(state.slot),error);
    std::vector<std::pair<std::string,std::string>> rows{
        {"Source game",std::string(Context::exactGameName(screen.sourceGameId))},
        {"Native format","PK3 checksum verified; save sections checked on export"},
        {"Encounter / evolution / transfers","Full history not evaluated"},
        {"Rule coverage","Editor guardrails only; no complete legality check"}};
    if(p) {
        rows.push_back({"Pokemon",std::string(Names::getSpeciesName(p->species))+" #"+std::to_string(p->species)});
        rows.push_back({"Nickname",p->nickname});rows.push_back({"Original Trainer",p->otName});
        rows.push_back({"Trainer / Secret ID",std::to_string(p->tid)+" / "+std::to_string(p->sid)});
        rows.push_back({"Origin game",Enums::getOriginGameName(p->originGame)});
        rows.push_back({"Met location",metLocationText(*p)});
        rows.push_back({"Met level",std::to_string(p->metLevel)});
        rows.push_back({"PID",std::to_string(p->pid)});
        for(size_t i=0;i<4;++i)rows.push_back({"Move "+std::to_string(i+1),p->moves[i]?Names::getMoveName(p->moves[i]):"Empty"});
    }
    SharedEditorChrome::drawOriginLegalityPanel(fb,screen.touchButtons,rows,P::LegalityState::NotChecked);
}

void drawActions(TrainerViewScreen& screen, PKSEFramebuffer& fb, State& state) {
    auto* staged = editor(screen);
    if (!staged) {
        const auto* pokemon = sourceBoxPokemon(screen, state);
        const bool occupied = pokemon != nullptr;
        PokeBank::UIModel::NarrowPokemonMenu::Model model{};
        if (occupied) {
            // No validated staged editor: only the read-only View exists (shared order).
            model.append("View", 0);
            model.append("Cancel", 1);
        } else {
            model.append("Add Pokemon"); // blocked until a validated staged copy exists
            model.append("Cancel", 0);
        }
        screen.drawGSCOverlay(fb);
        const std::string title = pokemon ? std::string(pokemon->species()) : "Empty Slot";
        drawNarrowPokemonActions(fb, screen.touchButtons, model, state.actionRow,
            title, screen.legacyProviderLabel() + " GBA SOURCE / READ ONLY", 31600);
        SharedEditorChrome::Presentation::PickerFooterSpec footer;
        footer.page = false;
        SharedEditorChrome::drawPickerFooter(fb, footer);
        return;
    }
    std::string error;
    const auto pokemon = staged->boxedPokemon(static_cast<std::size_t>(state.box),
                                               static_cast<std::size_t>(state.slot), error);
    const bool occupied = pokemon.has_value();
    const auto actions = Shared::actionsForSlot(
        occupied, {true, true, true, true, true}, !reviewRows(screen).empty());
    screen.drawGSCOverlay(fb);
    // One narrow, greyed-out menu for touch and A. Underlying box hitboxes
    // are replaced by enabled native command IDs only.
    const auto model = PokeBank::UIModel::NarrowPokemonMenu::classic(actions, occupied);
    const std::string title = pokemon
        ? std::string(Names::getSpeciesName(pokemon->species)) : "Empty Slot";
    drawNarrowPokemonActions(fb, screen.touchButtons, model, state.actionRow,
                            title,
                            screen.legacyProviderLabel() + " GBA SOURCE / READ ONLY", 31600);
    SharedEditorChrome::Presentation::PickerFooterSpec footer;
    footer.page = false;
    SharedEditorChrome::drawPickerFooter(fb, footer);
}

// Second step of Review -> Discard All. Drawn over the review list; nothing changes until Y.
// The confirm owns the frame's touch: the review list's rows and footer are not tappable under it.
void drawDiscardAllConfirm(TrainerViewScreen& screen, PKSEFramebuffer& fb, const char*) {
    SharedEditorChrome::drawReviewDiscardConfirm(fb,screen.touchButtons);
}

void drawReview(TrainerViewScreen& screen, PKSEFramebuffer& fb, State& state) {
    auto* staged=editor(screen);
    if(!staged)return;
    PokeBank::UIModel::SharedReview::Rows rows;
    for(const auto& change:reviewRows(screen))
        rows.emplace_back(change.label,change.createdDetail?change.afterValue:
                          change.beforeValue+" -> "+change.afterValue);
    SharedEditorChrome::drawReviewPanel(fb,screen.touchButtons,rows,
        static_cast<size_t>(state.reviewScroll),!rows.empty(),"Generation III");
    if(state.confirmDiscardAll)
        drawDiscardAllConfirm(screen,fb,"Generation III");
}

void drawReleaseConfirm(TrainerViewScreen& screen, PKSEFramebuffer& fb, State& state) {
    SharedEditorChrome::beginModal(screen.touchButtons);
    fb.drawFilledRect(0, 0, fb.getWidth(), fb.getHeight(), Colors::Background);
    constexpr int w = 760, h = 260;
    const int x = (fb.getWidth() - w) / 2;
    const int y = (fb.getHeight() - h) / 2;
    fb.drawFilledRoundedRect(x, y, w, h, 18, Colors::Panel);
    fb.drawRoundedRect(x, y, w, h, 18, Colors::Divider, 1);
    fb.drawText(x + 34, y + 30, "Release this Pokemon?", Colors::Text, TextStyle::Heading);
    fb.drawText(x + 34, y + 88,
        "Box " + std::to_string(state.box + 1) + " / Slot " + std::to_string(state.slot + 1),
        Colors::TextDim, TextStyle::Body);
    fb.drawText(x + 34, y + 130, "This removes it only from the PokeBank staged copy.",
                Colors::Text, TextStyle::Body);
    fb.drawText(x + 34, y + 166, "The original source save is never written.",
                Colors::TextDim, TextStyle::Caption);
    // R2-BUG-012: each choice is its own boxed row; a tap presses that button (Y releases).
    SharedBoxedRows::drawGlyphChoices(fb, x + 34, y + h - SharedBoxedRows::kChoiceHeight - 18, w - 68,
        std::array<SharedBoxedRows::GlyphChoice, 2>{{{"B", "Cancel"}, {"Y", "Confirm Release"}}});
    drawNavBar(fb, {{"Y", "Confirm Release"}, {"B", "Cancel"}});
}

} // namespace

bool beginPassiveView(TrainerViewScreen& screen) {
    auto& state = stateFor(screen);
    // Never let inherited Details bootstrap replace an active shared Create/Edit/View session.
    if (workspaceActive(state)) return state.passive;
    if (!screen.details.active) return false;
    auto* target = screen.detailsTargetPokemon();
    if (!target || target->getGameGroup() != Enums::GameVersion::FRLG || target->getData().size() < 80)
        return false;
    if (state.passive && workspaceActive(state)) return true;
    auto* encrypted = Encryption::encryptArray3FRLG(target->getData());
    std::array<uint8_t,80> raw{};
    std::memcpy(raw.data(), encrypted, raw.size());
    delete[] encrypted;
    std::string error;
    auto record = Gen3::StagedPokemonEditor::decodeRecord(raw, error);
    if (!record) {
        screen.closeDetailsModal();
        screen.postStatus(error.empty() ? "Invalid native PK3 record" : error, 300);
        return true;
    }
    state.session.begin(*record, SessionModel::Mode::View);
    state.passive = true;
    resetWorkspaceFocus(state);
    return true;
}

bool ownsFrame(const TrainerViewScreen& screen) noexcept {
    return ownsFrameImpl(screen);
}

// R2-BUG-002: the HOME Trainer page stages Name / Money through Gen3StagedTrainerEditor with the
// one classic Trainer session flow (UI/ClassicTrainerSessionInput.h, shared with Gen I / II / IV).
// Review lists the change, Discard restores it, Export applies it to the verified copy.
bool trainerMainPage(const TrainerViewScreen& screen, const State& state) noexcept {
    return screen.detailViewActive && screen.selectedMode == TrainerViewScreen::ViewMode::Trainer &&
        !state.legality && !state.confirmRelease && !state.review && !state.actions &&
        !state.speciesPicker && !state.valuePicker && !workspaceActive(state) &&
        !screen.helpOverlayActive && !screen.details.active && !screen.actionSheet.isOpen() &&
        !screen.saveConfirmActive && !screen.pickerActive && !screen.itemEditDialogActive &&
        !screen.carrying() && !screen.currentlySelecting;
}

bool handleTrainerSession(TrainerViewScreen& screen, State& state, u64 down, u64 held,
                          int stickX, int stickY, const TouchInput& touch) {
    if (!trainerMainPage(screen, state)) return false;
    auto& trainerBridge = bridge(screen);
    if (!trainerBridge.stagedTrainerAvailable()) return false;
    auto* trainer = trainerBridge.stagedTrainer();
    UI::ClassicTrainerSession::Fields fields;
    fields.name = trainer->trainerName();
    fields.money = trainer->money();
    fields.nameMax = Gen3::Gen3StagedTrainerEditor::kNameMax;
    fields.moneyMax = Gen3::Gen3StagedTrainerEditor::kMaxMoney;
    fields.nameGuide = "1-7 characters the game can store";
    fields.nameEditable = trainer->nameEditable();
    fields.nameReadOnlyReason = "Japanese Gen III trainer names are read-only here";
    // P3: native Play Time and Trainer Gender (SaveBlock2), applied to the verified export copy.
    if (trainer->playTimeEditable()) {
        const auto time = trainer->playTime();
        fields.playTime = PokeVault::Integration::Common::NativePlayTime{time.hours, time.minutes, time.seconds, false};
        fields.playTimeMaxHours = Gen3::Gen3StagedTrainerEditor::kMaxPlayedHours;
        fields.stagePlayTime = [&](const PokeVault::Integration::Common::NativePlayTime& next, std::string& error) {
            if (!trainer->stagePlayTime({next.hours, next.minutes, next.seconds}, error)) return false;
            trainerBridge.refreshStagedTrainerPresentation();
            return true;
        };
    }
    fields.gender = trainer->gender();
    if (trainer->genderEditable())
        fields.stageGender = [&](uint8_t gender, std::string& error) {
            if (!trainer->stageGender(gender, error)) return false;
            trainerBridge.refreshStagedTrainerPresentation();
            return true;
        };
    return UI::ClassicTrainerSession::handle(screen, down, held, stickX, stickY, touch, fields,
        [&](const std::string& name, uint32_t money, std::string& error) {
            if (!trainer->stage(name, money, error)) return false;
            trainerBridge.refreshStagedTrainerPresentation();
            return true;
        }, [&] {
            state.review = true;
            state.reviewScroll = 0;
            state.reviewTouchScroll.reset();
        });
}

bool handleInput(TrainerViewScreen& screen, u64 down, u64 held,
                 int stickX, int stickY, const TouchInput& touch) {
    if (!isGen3Gba(screen)) return false;
    if (handleTrainerSession(screen, stateFor(screen), down, held, stickX, stickY, touch)) return true;

    beginPassiveView(screen);
    auto& state = stateFor(screen);
    const bool ownsNavigation = state.legality || state.confirmRelease || state.review ||
                                workspaceActive(state) || state.actions;
    const u64 navigated = ownsNavigation
        ? screen.controllerNavigation.apply(
              down, held, stickX, stickY,
              HidNpadButton_Up, HidNpadButton_Down,
              HidNpadButton_Left, HidNpadButton_Right)
        : down;
    if (state.legality) {
        if (navigated & HidNpadButton_B) {
            state.legality = false;
            state.actions = true;
            screen.controllerNavigation.reset();
        }
        return true;
    }
    if (handleReleaseConfirm(screen, state, navigated)) return true;
    if (handleReview(screen, state, navigated, touch)) return true;
    if (handleWorkspace(screen, state, navigated, touch)) return true;
    if (handleActions(screen, state, navigated, touch)) return true;

    // A completed cell tap must work even when the previous focus is the
    // box-name header (-1). Validate locks after assigning the touched slot.
    if (screen.selectedMode == TrainerViewScreen::ViewMode::Boxes &&
        screen.detailViewActive && touch.justTapped()) {
        const int tapped = screen.touchedButtonId(touch);
        const int slots = static_cast<int>(screen.trainer.getSlotsPerBox());
        if (tapped >= 0 && tapped < slots) {
            const int previous = screen.selectedItemIndex;
            screen.selectedItemIndex = tapped;
            if (boxEntrySurface(screen)) {
                beginActions(screen);
                return true;
            }
            screen.selectedItemIndex = previous;
        }
        // The summary panel's "A / tap: Edit" target (BoxPokemonPanel id 2000) opens the SAME
        // Gen III sheet as A, never the inherited generic action sheet.
        if (tapped == PokeBank::UIModel::Gen4BoxTouchActions::SummaryPanelTouchId &&
            PokeBank::UIModel::Gen4BoxTouchActions::opensSelectedSlotActions(
                tapped, screen.selectedItemIndex, slots) && boxEntrySurface(screen)) {
            beginActions(screen);
            return true;
        }
    }
    const bool inBoxes = boxEntrySurface(screen);
    if (!inBoxes && !storageEntrySurface(screen)) return false;
    const bool occupied = selectedOccupied(screen);
    const auto activation = Shared::boxActivation(
        occupied, (down & HidNpadButton_A) != 0, (down & HidNpadButton_X) != 0);
    if (activation == Shared::BoxActivation::Actions) {
        beginActions(screen);
        return true;
    }
    if (activation == Shared::BoxActivation::Add) {
        // Consume X even if validation rejects the staged Create. Otherwise
        // the old source-writable base creator sees the same X input.
        (void)beginCreate(screen,
            inBoxes ? screen.selectedBoxIndex : screen.stSaveBox,
            inBoxes ? screen.selectedItemIndex : screen.stSaveSlot);
        return true;
    }
    return false;
}

bool draw(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    if (isGen3Gba(screen)) beginPassiveView(screen);
    if (!ownsFrameImpl(screen)) return false;
    auto& state = stateFor(screen);
    if (state.confirmRelease) drawReleaseConfirm(screen, fb, state);
    else if (state.legality) drawLegality(screen, fb, state);
    else if (state.review) drawReview(screen, fb, state);
    else if (workspaceActive(state)) drawWorkspace(screen, fb, state);
    else if (state.actions) drawActions(screen, fb, state);
    return true;
}

} // namespace UI::Gen3SharedEditorSurface
