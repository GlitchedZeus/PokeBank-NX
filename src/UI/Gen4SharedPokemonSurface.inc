#include "Inventory/ItemQuantityPolicy.h"
#include "UI/Gen4BagQuantity.h"
#include "UI/SharedBagAddFlow.h"
#include "UI/SharedBagAddPicker.h"
#include "Names/LocationDisplayNames.h"
#include "Integration/Gen4/Gen4ExactFormatEditorProvider.h"
#include "UI/Gen4BoxTouchActions.h"
#include "Integration/Gen4/Gen4MoveCompatibility.h"
#include "Integration/Gen4/Gen4BagCatalog.h"
#include "Legacy/Gen4ReadOnlyTrainer.h"
#include "Enums/Ball.h"
#include "Enums/LanguageID.h"
#include "Names/ItemNames.h"
#include "Names/MachineDisplay.h"
#include "Names/Gen4HeldItemCatalog.h"
#include "Names/LocationNames.h"
#include "Names/MoveInfo.h"
#include "Names/MoveNames.h"
#include "Names/MovePresence.h"
#include "Names/SpeciesNames.h"
#include "Pokemon/Pokemon4ReadOnlyView.h"
#include "Pokemon/PersonalInfo4HGSS.h"
#include "Trainer/Trainer.h"
#include "UI/ClassicGameContext.h"
#include "Integration/Gen4/Gen4ExportTransaction.h"
#include "UI/Common.h"
#include "UI/Dialogs/DialogFrame.h"
#include "UI/Gen4SharedPokemonSession.h"
#include "UI/MovePickerPresentation.h"
#include "UI/MovePickerRowUI.h"
#include "UI/NaturePickerLayout.h"
#include "UI/ItemPickerArtwork.h"
#include "UI/ItemPickerSort.h"
#include "UI/Gen4BoxMoveNavigation.h"
#include "UI/Gen4DataPresentation.h"
#include "UI/ScreenChrome.h"
#include "UI/SharedPokemonEditorContract.h"
#include "UI/SharedHeldItemPicker.h"
#include "UI/SharedHeldItemPickerPresentation.h"
#include "UI/SharedSpeciesPicker.h"
#include "UI/SpeciesChangeMoves.h"
#include "UI/TouchScroll.h"
#include "UI/SharedPokemonShell.h"
#include "SharedPokemonEditorSurface.inc"
#include "Utils/Keyboard.h"
#include "Utils/PokeBankPaths.h"
#include "Utils/StringHelpers.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace UI::Gen4SharedEditorSurface {
namespace {

namespace Shared = PokeBank::UIModel::SharedPokemonEditor;
namespace HeldItemGrid = PokeBank::UIModel::SharedHeldItemPicker;
namespace SessionModel = PokeBank::UIModel::Gen4SharedEditor;
namespace Context = PokeBank::UIModel::ClassicGameContext;
namespace Gen4 = PokeVault::Integration::Gen4;
namespace MoveUI = PokeBank::UIModel::MovePickerPresentation;
namespace Surface = UI::SharedPokemonEditorSurface;

using PickerTarget = Surface::PickerTarget;
using Surface::genderText;
enum class TargetKind : uint8_t { Box, Party };

enum class MenuAction : uint8_t {
    View, AddMasterVault, AddBank, TransferGame, Edit, MovePokemon, Clone,
    Legality, Release, Add, Review, Close
};
struct MenuActionEntry {
    MenuAction action = MenuAction::Close;
    const char* label = "Cancel";
    bool enabled = true;
};
struct MenuActionSet {
    std::array<MenuActionEntry, 12> values{};
    size_t count = 0;
    const MenuActionEntry& operator[](size_t index) const noexcept { return values[index]; }
};

// Gen IV session state around the ONE shared editor workspace state.
struct State : Surface::EditorState {
    const TrainerViewScreen* owner = nullptr;
    bool actions = false;
    bool review = false;
    bool legality = false;
    bool confirmRelease = false;
    // Discard All in Review is a two-step Y -> Y confirmation; B keeps the staged edits.
    bool confirmDiscardAll = false;
    int actionRow = 0;
    TargetKind target = TargetKind::Box;
    int box = 0;
    int slot = 0;
    SessionModel::Session session{};

    // Use the same native Switch keypad as Gen I-III, never an in-app stepper.
    bool itemAddPickerActive = false;
    int itemAddPocket = 0;
    int itemAddIndex = 0;
    std::vector<uint16_t> itemAddChoices;
    std::vector<uint16_t> itemAddNativeChoices;
    ItemPickerSort::Mode itemAddSort = ItemPickerSort::Mode::Native;
    SharedScrollbar::State itemAddScrollbar;   // R2-SCROLLBAR: draggable thumb on the Add Item list
    // Key Item warning before a Key Item is staged (UI/SharedBagAddFlow.h needsKeyItemWarning).
    bool itemKeyWarningActive = false;
    int itemKeyWarningPocket = 0;
    uint16_t itemKeyWarningId = 0;
    bool itemRemoveConfirmActive = false;
    int itemRemovePocket = 0;
    int itemRemoveVisibleRow = 0;
    uint16_t itemRemoveId = 0;
    uint16_t itemRemoveCount = 0;
};

State& stateFor(const TrainerViewScreen& screen) {
    static State state;
    if (state.owner != &screen) {
        state = {};
        state.owner = &screen;
    }
    return state;
}

bool isGen4Nds(const TrainerViewScreen& screen) noexcept {
    return PokeVault::Integration::Gen4EditorProvider::isGen4NdsId(screen.sourceGameId);
}

PokeVault::Legacy::Gen4ReadOnlyTrainer& bridge(TrainerViewScreen& screen) {
    return static_cast<PokeVault::Legacy::Gen4ReadOnlyTrainer&>(screen.trainer);
}
const PokeVault::Legacy::Gen4ReadOnlyTrainer& bridge(const TrainerViewScreen& screen) {
    return static_cast<const PokeVault::Legacy::Gen4ReadOnlyTrainer&>(screen.trainer);
}

Gen4::Gen4StagedPokemonEditor* editor(TrainerViewScreen& screen) noexcept {
    return isGen4Nds(screen) ? bridge(screen).stagedPokemon() : nullptr;
}

bool workspaceActive(const State& state) noexcept {
    return state.session.mode != SessionModel::Mode::None;
}

bool ownsFrameImpl(const TrainerViewScreen& screen) noexcept {
    if (!isGen4Nds(screen)) return false;
    const auto& state = stateFor(screen);
    return state.actions || state.review || state.legality || state.confirmRelease ||
           state.itemRemoveConfirmActive || state.itemAddPickerActive ||
           state.itemKeyWarningActive ||
           state.valuePicker || state.moveEditor || workspaceActive(state);
}

bool boxEntrySurface(const TrainerViewScreen& screen) noexcept {
    return isGen4Nds(screen) &&
           screen.selectedMode == TrainerViewScreen::ViewMode::Boxes &&
           screen.detailViewActive &&
           screen.selectedBoxIndex >= 0 && screen.selectedBoxIndex < 18 &&
           screen.selectedItemIndex >= 0 && screen.selectedItemIndex < 30 &&
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

// Staged game-save PC only; never intercept the app-owned Bank side.
bool storageEntrySurface(const TrainerViewScreen& screen) noexcept {
    return isGen4Nds(screen) &&
        screen.selectedMode == TrainerViewScreen::ViewMode::Storage &&
        screen.detailViewActive && screen.storageFocusPane == 0 &&
        screen.stSaveBox >= 0 && screen.stSaveBox < 18 &&
        screen.stSaveSlot >= 0 && screen.stSaveSlot < 30 &&
        !screen.helpOverlayActive && !screen.details.active && !screen.actionSheet.isOpen() &&
        !screen.saveConfirmActive && !screen.pickerActive && !screen.itemEditDialogActive &&
        !screen.carrying() && !screen.currentlySelecting;
}

bool partyEntrySurface(const TrainerViewScreen& screen) noexcept {
    return isGen4Nds(screen) &&
           screen.selectedMode == TrainerViewScreen::ViewMode::Party &&
           screen.detailViewActive &&
           screen.selectedPartyIndex >= 0 &&
           screen.selectedPartyIndex < static_cast<int>(screen.trainer.getPartySize()) &&
           !screen.helpOverlayActive &&
           !screen.details.active &&
           !screen.actionSheet.isOpen() &&
           !screen.saveConfirmActive &&
           !screen.pickerActive &&
           !screen.itemEditDialogActive;
}

std::optional<Pokemon::Pokemon4ReadOnly> boxRecord(
    TrainerViewScreen& screen, int box, int slot, std::string& error) {
    if (box < 0 || box >= 18 || slot < 0 || slot >= 30) {
        error = "Generation IV box selection is outside the native 18 x 30 storage";
        return std::nullopt;
    }
    if (auto* staged = editor(screen))
        return staged->boxedPokemon(static_cast<size_t>(box), static_cast<size_t>(slot), &error);

    const auto& source = bridge(screen).sourceSave().box(
        static_cast<size_t>(box), static_cast<size_t>(slot));
    if (!source.valid() || source.empty()) {
        error = source.empty() ? "Selected Generation IV slot is empty"
                               : "Selected Generation IV PK4 failed strict validation";
        return std::nullopt;
    }
    error.clear();
    return source;
}

std::optional<Pokemon::Pokemon4ReadOnly> partyRecord(
    TrainerViewScreen& screen, int slot, std::string& error) {
    if (slot < 0 || slot >= static_cast<int>(bridge(screen).sourceSave().partyCount())) {
        error = "Generation IV party selection is outside the occupied party";
        return std::nullopt;
    }
    if (auto* staged = editor(screen))
        return staged->partyPokemon(static_cast<size_t>(slot), &error);

    const auto party = bridge(screen).sourceSave().nativePartySlots();
    if (static_cast<size_t>(slot) >= party.size()) {
        error = "Generation IV party selection is outside the native party records";
        return std::nullopt;
    }
    const auto& source = party[static_cast<size_t>(slot)];
    if (!source.valid() || source.empty() || !source.isParty()) {
        error = source.empty() ? "Selected Generation IV party slot is empty"
                               : "Selected Generation IV party PK4 failed strict validation";
        return std::nullopt;
    }
    error.clear();
    return source;
}

std::optional<Pokemon::Pokemon4ReadOnly> selectedRecord(
    TrainerViewScreen& screen, const State& state, std::string& error) {
    return state.target == TargetKind::Party
        ? partyRecord(screen, state.slot, error)
        : boxRecord(screen, state.box, state.slot, error);
}

bool selectedOccupied(TrainerViewScreen& screen) {
    std::string error;
    if (partyEntrySurface(screen))
        return partyRecord(screen, screen.selectedPartyIndex, error).has_value();
    if (boxEntrySurface(screen) || storageEntrySurface(screen)) {
        const bool storage = storageEntrySurface(screen);
        return boxRecord(screen, storage ? screen.stSaveBox : screen.selectedBoxIndex,
                         storage ? screen.stSaveSlot : screen.selectedItemIndex, error).has_value();
    }
    return false;
}

void beginActions(TrainerViewScreen& screen) {
    auto& state = stateFor(screen);
    state.actions = true;
    state.review = false;
    state.actionRow = 0;
    if (partyEntrySurface(screen)) {
        state.target = TargetKind::Party;
        state.box = 0;
        state.slot = screen.selectedPartyIndex;
    } else {
        state.target = TargetKind::Box;
        const bool storage = storageEntrySurface(screen);
        state.box = storage ? screen.stSaveBox : screen.selectedBoxIndex;
        state.slot = storage ? screen.stSaveSlot : screen.selectedItemIndex;
    }
    screen.controllerNavigation.reset();
}

bool beginPassiveView(TrainerViewScreen& screen) {
    if (!isGen4Nds(screen) || !screen.details.active) return false;
    if (screen.details.source == TrainerViewScreen::EditSource::Bank) return false;
    auto* target = screen.detailsTargetPokemon();
    if (!target) return false;
    const auto group = target->getGameGroup();
    if (group != Enums::GameVersion::DP && group != Enums::GameVersion::PT &&
        group != Enums::GameVersion::HGSS) return false;

    auto* view = static_cast<Pokemon::Pokemon4ReadOnlyView*>(target);
    auto& state = stateFor(screen);
    std::string error;
    if (!state.session.begin(view->source(), bridge(screen).sourceSave().rawFamily(),
                             SessionModel::Mode::View, error)) {
        screen.closeDetailsModal();
        screen.postStatus(error.empty() ? "Could not open Generation IV shared View" : error, 360);
        return true;
    }
    state.target = screen.details.source == TrainerViewScreen::EditSource::Party
        ? TargetKind::Party : TargetKind::Box;
    state.box = screen.selectedBoxIndex;
    state.slot = state.target == TargetKind::Party
        ? screen.details.partyIndex : screen.selectedItemIndex;
    state.actions = false;
    state.review = false;
    Surface::resetWorkspaceFocus(state);
    screen.closeDetailsModal();
    screen.controllerNavigation.reset();
    return true;
}

bool beginViewOrEdit(TrainerViewScreen& screen, SessionModel::Mode mode) {
    auto& state = stateFor(screen);
    if (mode == SessionModel::Mode::Edit && !editor(screen)) {
        const auto& why = bridge(screen).stagedPokemonUnavailableReason();
        screen.postStatus(why.empty()
            ? "Generation IV staged editing is unavailable for this source"
            : why, 360);
        return false;
    }

    std::string error;
    auto record = selectedRecord(screen, state, error);
    if (!record) {
        screen.postStatus(error.empty() ? "Selected Generation IV slot is empty" : error, 300);
        return false;
    }
    if (!state.session.begin(*record, bridge(screen).sourceSave().rawFamily(), mode, error)) {
        screen.postStatus(error.empty() ? "Could not start Generation IV editor session" : error, 360);
        return false;
    }
    state.actions = false;
    state.review = false;
    Surface::resetWorkspaceFocus(state);
    return true;
}

bool refreshPresentation(TrainerViewScreen& screen) {
    std::string error;
    if (!bridge(screen).refreshStagedPokemonPresentation(error)) {
        screen.postStatus(error.empty()
            ? "Could not refresh staged Generation IV presentation" : error, 360);
        return false;
    }
    return true;
}

bool keepWorkspace(TrainerViewScreen& screen) {
    auto& state = stateFor(screen);
    auto* staged = editor(screen);
    if (!staged) {
        screen.postStatus("Generation IV staged editor is unavailable", 300);
        return false;
    }
    std::string error;
    const bool kept = state.session.creating()
        ? state.session.keepCreate(*staged, static_cast<size_t>(state.box),
                                   static_cast<size_t>(state.slot), error)
        : state.target == TargetKind::Party
            ? state.session.keepParty(*staged, static_cast<size_t>(state.slot), error)
            : state.session.keep(*staged, static_cast<size_t>(state.box),
                                 static_cast<size_t>(state.slot), error);
    if (!kept) {
        screen.postStatus(error.empty() ? "Generation IV staged edit failed" : error, 360);
        return false;
    }
    if (!refreshPresentation(screen)) {
        // The staged bytes remain in custody, but do not claim UI success while presentation is stale.
        screen.postStatus("Generation IV change is staged, but presentation refresh failed", 420);
        return false;
    }
    if (state.target == TargetKind::Party)
        screen.selectedPartyIndex = state.slot;
    else {
        screen.selectedBoxIndex = state.box;
        screen.selectedItemIndex = state.slot;
    }
    screen.postStatus("Generation IV Pokemon change staged; external source unchanged", 300);
    return true;
}

std::optional<Pokemon::Pokemon4ReadOnly> workingRecord(const State& state,
                                                       Enums::GameVersion group) {
    if (!state.session.working) return std::nullopt;
    const auto bytes = state.session.working->encryptedBytes();
    Pokemon::Pokemon4ReadOnly record(bytes, group);
    if (!record.valid() || record.empty()) return std::nullopt;
    return record;
}

const char* gameName(std::string_view id) noexcept {
    if (id == "diamond_nds") return "Diamond";
    if (id == "pearl_nds") return "Pearl";
    if (id == "platinum_nds") return "Platinum";
    if (id == "heartgold_nds") return "HeartGold";
    if (id == "soulsilver_nds") return "SoulSilver";
    return "Generation IV";
}

std::string gen4FormLabel(uint16_t species, uint8_t form) {
    if (species == 201) {
        if (form < 26) return std::string(1, static_cast<char>('A' + form));
        if (form == 26) return "!";
        if (form == 27) return "?";
    }
    if (species == 172)
        return form == 0 ? "Normal" : form == 1 ? "Spiky-eared" : "Form " + std::to_string(form);
    if (species == 412 || species == 413 || species == 414) {
        constexpr std::array<const char*,3> names{{"Plant", "Sandy", "Trash"}};
        if (form < names.size()) return names[form];
    }
    if (species == 421) {
        constexpr std::array<const char*,2> names{{"Overcast", "Sunshine"}};
        if (form < names.size()) return names[form];
    }
    if (species == 422 || species == 423) {
        constexpr std::array<const char*,2> names{{"West", "East"}};
        if (form < names.size()) return names[form];
    }
    if (species == 479) {
        constexpr std::array<const char*,6> names{{"Normal", "Heat", "Wash", "Frost", "Fan", "Mow"}};
        if (form < names.size()) return names[form];
    }
    if (species == 487) {
        constexpr std::array<const char*,2> names{{"Altered", "Origin"}};
        if (form < names.size()) return names[form];
    }
    if (species == 492) {
        constexpr std::array<const char*,2> names{{"Land", "Sky"}};
        if (form < names.size()) return names[form];
    }
    if (species == 493) {
        constexpr std::array<const char*,18> names{{
            "Normal", "Fighting", "Flying", "Poison", "Ground", "Rock", "Bug", "Ghost",
            "Steel", "??? (unused)", "Fire", "Water", "Grass", "Electric", "Psychic",
            "Ice", "Dragon", "Dark"
        }};
        if (form < names.size()) return names[form];
    }
    return form == 0 ? "Default" : "Form " + std::to_string(form);
}

constexpr std::array<const char*, 16> detailLabels{{
    "Species", "Nickname", "Level", "Experience", "OT", "Trainer ID",
    "SID", "Held Item", "Friendship", "Language", "Origin", "Ball",
    "Met Level", "Met Location", "Pokerus", "Form"
}};

bool variableGenderSpecies(const Pokemon::Pokemon4ReadOnly& p) noexcept {
    const uint8_t ratio = p.personal().genderRatio;
    return ratio > 0 && ratio < 254;
}

bool dualAbilitySpecies(const Pokemon::Pokemon4ReadOnly& p) noexcept {
    return p.personal().ability2 != 0 && p.personal().ability2 != p.personal().ability1;
}

bool nativeGen4Origin(const Pokemon::Pokemon4ReadOnly& p) noexcept {
    const auto originGroup = Enums::getGameGroup(
        static_cast<Enums::GameVersion>(p.originVersion()));
    return originGroup == Enums::GameVersion::DP ||
           originGroup == Enums::GameVersion::PT ||
           originGroup == Enums::GameVersion::HGSS;
}

bool hasAlternateValidForm(const State& state) {
    if (!state.session.working) return false;
    const uint8_t current = state.session.working->form();
    const uint8_t count = state.session.working->formCount();
    for (uint8_t form = 0; form < count; ++form) {
        if (form == current) continue;
        auto probe = *state.session.working;
        if (probe.setForm(form)) return true;
    }
    return false;
}

bool detailEditable(const State& state, int row,
                    const Pokemon::Pokemon4ReadOnly& p) {
    if (!state.session.editable()) return false;
    switch (row) {
        case 0:  // Species
        case 1:  // Nickname
        case 2:  // Level
        case 3:  // Experience
        case 4:  // Original Trainer
        case 5:  // Trainer ID
        case 7:  // Held Item
        case 8:  // Friendship
        case 9:  // Language
        case 12: // Met Level
        case 14: // Pokerus
            return true;
        case 11: // Ball
        case 13: // Met Location
            return nativeGen4Origin(p);
        case 15: // Form
            return hasAlternateValidForm(state);
        default:
            return false; // SID/Origin stay visible but are not controls.
    }
}

bool pidLinkedRowEditable(const State& state, int row,
                          const Pokemon::Pokemon4ReadOnly& p) noexcept {
    if (!state.session.editable()) return false;
    if (row == 6 || row == 8) return true; // Nature / Shiny
    if (row == 7) return variableGenderSpecies(p);
    if (row == 9) return dualAbilitySpecies(p);
    return false;
}

std::string detailValue(const Pokemon::Pokemon4ReadOnly& p,
                        const Pokemon::Pokemon4ReadOnlyView& view, int row) {
    switch (row) {
        case 0: return std::to_string(p.species()) + " - " + Names::getSpeciesName(p.species());
        case 1: {
            const auto value = Utils::utf16ToUtf8(p.nickname());
            return value.empty() ? "(none)" : value;
        }
        case 2: return std::to_string(view.level());
        case 3: return std::to_string(p.experience());
        case 4: {
            const auto value = Utils::utf16ToUtf8(p.originalTrainerName());
            return value.empty() ? "(none)" : value;
        }
        case 5: return std::to_string(p.tid());
        case 6: return std::to_string(p.sid());
        case 7: return p.heldItem() == 0 ? "(none)" : Names::getItemName(p.heldItem());
        case 8: return std::to_string(p.friendship());
        case 9: return Enums::getLanguageName(p.language());
        case 10: return Enums::getOriginGameName(p.originVersion());
        case 11: return Enums::getBallName(view.ball());
        case 12: return std::to_string(p.metLevel());
        case 13:
            return Names::getMetLocationDisplayName(p.originVersion(), view.metLocation());
        case 14: {
            const uint8_t pk = p.pokerusState();
            return pk == 0 ? "None" : (pk & 0x0Fu) == 0 ? "Cured" : "Infected";
        }
        case 15:
            return gen4FormLabel(p.species(), p.form());
        default: return {};
    }
}

int moveMaxPP(uint16_t move, uint8_t ppUps, Enums::GameVersion group) {
    if (move == 0) return 0;
    const int base = Names::getMoveBasePP(move, group);
    return base <= 0 ? 0 : base * (5 + std::min<int>(ppUps, 3)) / 5;
}

void openSpeciesPicker(TrainerViewScreen& screen, State& state) {
    state.valuePicker = true;
    state.pickerScroll.reset();
    state.pickerViewport.reset();
    state.pickerTarget = PickerTarget::Species;
    state.pickerValues.clear();
    for (uint16_t species = 1; species <= 493; ++species)
        state.pickerValues.push_back(species);
    const uint16_t current = state.session.working ? state.session.working->species() : 1;
    const auto it = std::find(state.pickerValues.begin(), state.pickerValues.end(), current);
    state.pickerRow = it == state.pickerValues.end()
        ? 0 : static_cast<int>(std::distance(state.pickerValues.begin(), it));
    state.speciesPreviewShiny =
        state.session.working ? state.session.working->shiny() : false;
    screen.controllerNavigation.reset();
}

bool beginCreate(TrainerViewScreen& screen) {
    auto& state = stateFor(screen);
    auto* staged = editor(screen);
    if (state.target != TargetKind::Box) {
        screen.postStatus("Generation IV Create is supported only in empty PC box slots", 300);
        return false;
    }
    if (!staged) {
        const auto& why = bridge(screen).stagedPokemonUnavailableReason();
        screen.postStatus(why.empty()
            ? "Generation IV Create requires a validated app-owned staged workspace" : why, 420);
        return false;
    }
    std::string error;
    // Same Create default as Gen II / III / V: Pikachu (25), present in every Gen IV game.
    auto draft = staged->createBoxDraft(
        static_cast<size_t>(state.box), static_cast<size_t>(state.slot), 25, &error);
    if (!draft) {
        screen.postStatus(error.empty() ? "Could not start Generation IV Create draft" : error, 360);
        return false;
    }
    if (!state.session.beginCreate(std::move(*draft), error)) {
        screen.postStatus(error.empty() ? "Could not start Generation IV Create session" : error, 360);
        return false;
    }
    state.actions = false;
    state.review = false;
    Surface::resetWorkspaceFocus(state);
    state.focus = {Shared::Panel::Details, 0, 0};
    // Match Gen I-III: Create opens the shared editor first. Species is edited
    // explicitly from the focused Species row rather than forcing a picker modal.
    return true;
}

void openHeldItemPicker(TrainerViewScreen& screen, State& state) {
    state.valuePicker = true;
    state.pickerScroll.reset();
    state.pickerViewport.reset();
    state.pickerTarget = PickerTarget::HeldItem;
    state.pickerValues.clear();
    state.pickerValues.push_back(0);
    const auto group = bridge(screen).sourceSave().rawFamily();
    const size_t count = Names::getItemCount();
    const uint16_t maximum = Names::gen4MaximumHeldItemId(group);
    for (uint16_t item = 1; item < count && item <= maximum; ++item)
        if (Names::isGen4HeldItemPresent(item, group))
            state.pickerValues.push_back(item);
    state.pickerNativeValues = state.pickerValues;
    state.pickerSort = ItemPickerSort::Mode::Native;
    const uint16_t current = state.session.working ? state.session.working->heldItem() : 0;
    const auto it = std::find(state.pickerValues.begin(), state.pickerValues.end(), current);
    state.pickerRow = it == state.pickerValues.end() ? 0 :
        static_cast<int>(std::distance(state.pickerValues.begin(), it));
    screen.controllerNavigation.reset();
}

void openBallPicker(TrainerViewScreen& screen, State& state,
                    const Pokemon::Pokemon4ReadOnly& record) {
    const auto originGroup = Enums::getGameGroup(
        static_cast<Enums::GameVersion>(record.originVersion()));
    if (originGroup != Enums::GameVersion::DP &&
        originGroup != Enums::GameVersion::PT &&
        originGroup != Enums::GameVersion::HGSS) {
        screen.postStatus(
            "Ball editing is read-only for transferred non-Gen-IV origins", 300);
        return;
    }

    state.valuePicker = true;
    state.pickerScroll.reset();
    state.pickerViewport.reset();
    state.pickerTarget = PickerTarget::Ball;
    state.pickerValues.clear();
    for (const auto ball : Enums::getBallList(originGroup))
        state.pickerValues.push_back(ball);
    const uint16_t current = state.session.working ? state.session.working->ball() : 4;
    const auto it = std::find(state.pickerValues.begin(), state.pickerValues.end(), current);
    state.pickerRow = it == state.pickerValues.end() ? 0 :
        static_cast<int>(std::distance(state.pickerValues.begin(), it));
    screen.controllerNavigation.reset();
}

void openMetLocationPicker(TrainerViewScreen& screen, State& state,
                           const Pokemon::Pokemon4ReadOnly& record) {
    const auto originGroup = Enums::getGameGroup(
        static_cast<Enums::GameVersion>(record.originVersion()));
    if (originGroup != Enums::GameVersion::DP &&
        originGroup != Enums::GameVersion::PT &&
        originGroup != Enums::GameVersion::HGSS) {
        screen.postStatus(
            "Met Location editing is read-only for transferred non-Gen-IV origins", 300);
        return;
    }
    state.valuePicker = true;
    state.pickerScroll.reset();
    state.pickerViewport.reset();
    state.pickerTarget = PickerTarget::MetLocation;
    state.pickerValues.clear();
    const auto table = Names::getLocationTable(record.originVersion());
    for (size_t id = 0; id < table.count && id <= 0xFFFFu; ++id)
        if (table.names && table.names[id] && table.names[id][0] != '\0' &&
            Names::isGen4NativeMetLocation(
                record.originVersion(), static_cast<uint16_t>(id)))
            state.pickerValues.push_back(static_cast<uint16_t>(id));
    Pokemon::Pokemon4ReadOnlyView view(record);
    const uint16_t current = view.metLocation();
    const auto it = std::find(state.pickerValues.begin(), state.pickerValues.end(), current);
    state.pickerRow = it == state.pickerValues.end() ? 0 :
        static_cast<int>(std::distance(state.pickerValues.begin(), it));
    screen.controllerNavigation.reset();
}

void openFormPicker(TrainerViewScreen& screen, State& state) {
    if (!state.session.working) return;
    const uint8_t count = state.session.working->formCount();
    state.pickerValues.clear();
    for (uint16_t form = 0; form < count; ++form) {
        auto probe = *state.session.working;
        if (probe.setForm(static_cast<uint8_t>(form)))
            state.pickerValues.push_back(form);
    }
    if (state.pickerValues.size() <= 1) {
        screen.postStatus("This Pokémon has no alternate Form valid for this Generation IV slot", 300);
        return;
    }
    state.valuePicker = true;
    state.pickerScroll.reset();
    state.pickerViewport.reset();
    state.pickerTarget = PickerTarget::Form;
    const uint8_t current = state.session.working->form();
    const auto it = std::find(state.pickerValues.begin(), state.pickerValues.end(), current);
    state.pickerRow = it == state.pickerValues.end()
        ? 0 : static_cast<int>(std::distance(state.pickerValues.begin(), it));
    screen.controllerNavigation.reset();
}


std::vector<uint16_t> gen4MoveChoices(TrainerViewScreen& screen, State& state) {
    if (!state.session.working) return {};
    return PokeVault::Integration::Gen4MoveCompatibility::selectableMoves(
        screen.sourceGameId, state.session.working->species(),
        state.session.working->form(), state.session.working->moves());
}

void openNaturePicker(TrainerViewScreen& screen, State& state) {
    state.valuePicker = true;
    state.pickerScroll.reset();
    state.pickerViewport.reset();
    state.pickerTarget = PickerTarget::Nature;
    state.pickerValues.clear();
    for (uint16_t i = 0; i < 25; ++i) state.pickerValues.push_back(i);
    state.pickerRow = state.session.working ? state.session.working->nature() : 0;
    screen.controllerNavigation.reset();
}

void openAbilityPicker(TrainerViewScreen& screen, State& state,
                       const Pokemon::Pokemon4ReadOnly& p) {
    if (!dualAbilitySpecies(p)) {
        screen.postStatus("This Generation IV species has one ability slot", 240);
        return;
    }
    state.valuePicker = true;
    state.pickerScroll.reset();
    state.pickerViewport.reset();
    state.pickerTarget = PickerTarget::Ability;
    state.pickerValues = {0, 1};
    state.pickerRow = state.session.working ? state.session.working->abilitySlot() : 0;
    screen.controllerNavigation.reset();
}

// Applies the chosen native value to the Gen IV draft (shared picker A).
bool applyGen4Picker(TrainerViewScreen& screen, State& state, PickerTarget target,
                     uint16_t value) {
    if (!state.session.working) return false;
    bool ok = false;
    if (target == PickerTarget::Species) {
        const auto before = state.session.working->encryptedBytes();
        const uint16_t previousSpecies = state.session.working->species();
        ok = state.session.working->setSpecies(value);
        if (ok && state.session.working->shiny() != state.speciesPreviewShiny)
            ok = state.session.working->setShiny(state.speciesPreviewShiny);
        if (ok && value != previousSpecies) {
            // R2-BUG-004: the new species' native default moveset at its level for this game's
            // DP / Pt / HGSS table (PKHeX lvlmove_*), base PP, no PP Ups (setMove writes both).
            namespace MoveReset = PokeBank::UIModel::SpeciesChangeMoves;
            const auto group = bridge(screen).sourceSave().rawFamily();
            ok = MoveReset::setMoves(*state.session.working, MoveReset::forSpecies(
                MoveReset::gen4TableForGroup(group), value, state.session.working->form(),
                state.session.working->level(),
                [group](uint16_t move) { return Names::getMoveBasePP(move, group); }));
        }
        if (!ok) {
            auto restored = Pokemon::Pokemon4Mutable::fromEncrypted(
                before, bridge(screen).sourceSave().rawFamily());
            if (restored) state.session.working = std::move(*restored);
            screen.postStatus(
                "Could not reconcile Species/Shiny while preserving PID-linked traits",
                360);
            return false;
        }
    } else if (target == PickerTarget::HeldItem) {
        ok = state.session.working->setHeldItem(value);
    } else if (target == PickerTarget::Language) {
        ok = state.session.working->setLanguage(static_cast<uint8_t>(value));
    } else if (target == PickerTarget::Ball) {
        ok = state.session.working->setBall(static_cast<uint8_t>(value));
    } else if (target == PickerTarget::MetLocation) {
        ok = state.session.working->setMetLocation(value);
    } else if (target == PickerTarget::Form) {
        ok = state.session.working->setForm(static_cast<uint8_t>(value));
    } else if (target == PickerTarget::Pokerus) {
        const auto mode = value == 0x10
            ? Pokemon::PokerusMode::Cured
            : value == 0x11 ? Pokemon::PokerusMode::Infected
                            : Pokemon::PokerusMode::None;
        ok = state.session.working->setPokerusMode(mode);
    } else if (target == PickerTarget::Move) {
        const size_t slot = static_cast<size_t>(state.moveEditorSlot);
        ok = state.session.working->setMove(slot, value);
    } else if (target == PickerTarget::Nature) {
        ok = state.session.working->setNature(static_cast<uint8_t>(value));
    } else if (target == PickerTarget::Ability) {
        ok = state.session.working->setAbilitySlot(static_cast<uint8_t>(value));
    }

    if (!ok) {
        screen.postStatus("Could not apply that Generation IV draft choice safely", 300);
        return false;
    }
    return true;
}

bool editDetail(TrainerViewScreen& screen, State& state, int row,
                const Pokemon::Pokemon4ReadOnly& p) {
    if (!state.session.working) return false;
    auto& mon = *state.session.working;
    if (row == 0) {
        openSpeciesPicker(screen, state);
        return true;
    }
    if (row == 1) {
        const auto current = Utils::utf16ToUtf8(mon.nickname());
        const auto result = Utils::promptText(
            "Nickname", "Pokemon nickname", current, 10);
        if (result.accepted && !mon.setNickname(Utils::utf8ToUtf16(result.text)))
            screen.postStatus("That nickname cannot be represented by this Generation IV save", 300);
        return true;
    }
    if (row == 2) {
        const auto r = Utils::promptNumber("Level", mon.level(), 1, 100);
        if (r.accepted && !mon.setLevel(static_cast<uint8_t>(r.value)))
            screen.postStatus("Generation IV level edit was refused", 240);
        return true;
    }
    if (row == 3) {
        const uint32_t maximum = Pokemon::getExpForLevel(100, p.personal().growthRate);
        const auto r = Utils::promptNumber(
            "Experience", static_cast<int>(std::min<uint32_t>(mon.experience(), maximum)),
            0, static_cast<int>(maximum));
        if (r.accepted && !mon.setExperience(static_cast<uint32_t>(r.value)))
            screen.postStatus("Generation IV EXP edit was refused", 240);
        return true;
    }
    if (row == 4) {
        const auto current = Utils::utf16ToUtf8(p.originalTrainerName());
        const auto r = Utils::promptText(
            "Original Trainer", "1-7 Generation IV characters", current, 7);
        if (r.accepted &&
            !mon.setOriginalTrainerName(Utils::utf8ToUtf16(r.text)))
            screen.postStatus("That OT name cannot be represented by this Generation IV save", 300);
        return true;
    }
    if (row == 5) {
        const auto r = Utils::promptNumber("Trainer ID", mon.tid(), 0, 65535);
        if (r.accepted && !mon.setTID(static_cast<uint16_t>(r.value)))
            screen.postStatus(
                "Could not change Trainer ID while preserving PID-linked traits", 360);
        return true;
    }
    if (row == 7) {
        openHeldItemPicker(screen, state);
        return true;
    }
    if (row == 8) {
        const auto r = Utils::promptNumber("Friendship", mon.friendship(), 0, 255);
        if (r.accepted) mon.setFriendship(static_cast<uint8_t>(r.value));
        return true;
    }
    if (row == 9) {
        // Inline toggle: A cycles this game group's languages in place (no popup).
        // setLanguage keeps the default (un-nicknamed) name in that language.
        std::vector<uint8_t> languages;
        const auto group = bridge(screen).sourceSave().rawFamily();
        for (uint8_t language = 1; language <= 8; ++language)
            if (Enums::groupHasLanguage(group, language)) languages.push_back(language);
        if (languages.size() <= 1) {
            screen.postStatus("This game group has a single language", 240);
            return true;
        }
        auto it = std::find(languages.begin(), languages.end(), mon.language());
        const uint8_t next = it == languages.end() || ++it == languages.end() ? languages.front() : *it;
        if (!mon.setLanguage(next))
            screen.postStatus("This Pokemon's names cannot be stored in that language", 300);
        return true;
    }
    if (row == 11) {
        openBallPicker(screen, state, p);
        return true;
    }
    if (row == 12) {
        const auto r = Utils::promptNumber("Met Level", mon.metLevel(), 0, 100);
        if (r.accepted) mon.setMetLevel(static_cast<uint8_t>(r.value));
        return true;
    }
    if (row == 13) {
        openMetLocationPicker(screen, state, p);
        return true;
    }
    if (row == 14) {
        // Inline toggle: None -> Infected -> Cured -> None. Returning to the untouched
        // state restores the source byte exactly (strain/days are never invented).
        uint8_t baseline = 0;
        if (!state.session.baselineEncrypted.empty()) {
            Pokemon::Pokemon4ReadOnly source(state.session.baselineEncrypted,
                                             bridge(screen).sourceSave().rawFamily());
            if (source.valid()) baseline = source.pokerusState();
        }
        const uint8_t next = PokeBank::UIModel::SharedEditorPresentation::nextPokerusByte(
            mon.pokerus(), baseline);
        if (!mon.setPokerus(next))
            screen.postStatus("Pokerus could not be changed for this Pokemon", 240);
        return true;
    }
    if (row == 15) {
        openFormPicker(screen, state);
        return true;
    }
    return false;
}

// STATS PID-linked rows 6..9 (Nature, Gender, Shiny, Ability); PID stays read-only.
bool editPidRow(TrainerViewScreen& screen, State& state, int row,
                const Pokemon::Pokemon4ReadOnly& p) {
    if (row == 6) {
        openNaturePicker(screen, state);
        return true;
    }
    if (row == 7) {
        if (!variableGenderSpecies(p)) {
            screen.postStatus("Gender is fixed for this Generation IV species", 240);
            return true;
        }
        if (!state.session.cycleGender())
            screen.postStatus("Could not toggle Gender while preserving PID-linked traits", 300);
        return true;
    }
    if (row == 8) {
        if (!state.session.cycleShiny())
            screen.postStatus("Could not toggle Shiny while preserving PID-linked traits", 300);
        return true;
    }
    if (row == 9) {
        openAbilityPicker(screen, state, p);
        return true;
    }
    return false;
}

MenuActionSet gen4Actions(const State& state, bool occupied, bool stagedAvailable,
                          bool hasPendingChanges) {
    MenuActionSet result{};
    const auto add = [&](MenuAction action, const char* label, bool enabled = true) {
        result.values[result.count++] = {action, label, enabled};
    };

    if (!occupied) {
        if (state.target == TargetKind::Box)
            add(MenuAction::Add, "Add Pokemon", stagedAvailable);
        if (hasPendingChanges) add(MenuAction::Review, "Review Pending Changes", true);
        add(MenuAction::Close, "Cancel", true);
        return result;
    }

    // The shared action order (NarrowPokemonActionMenu.h): View, Edit, Clone, Move, Release,
    // Origin & Legality, Review, Cancel. Only commands Gen IV really performs are listed:
    // Party has no Clone/Move/Release in the staged Gen IV editor, and Make Shiny / Master
    // Vault / Bank / Transfer placeholders are not shown (Shiny is edited in the editor).
    add(MenuAction::View, "View", true);
    add(MenuAction::Edit, "Edit", stagedAvailable);
    if (state.target != TargetKind::Party) {
        add(MenuAction::Clone, "Clone", stagedAvailable);
        add(MenuAction::MovePokemon, "Move", stagedAvailable);
        add(MenuAction::Release, "Release", stagedAvailable);
    }
    add(MenuAction::Legality, PokeBank::UIModel::NarrowPokemonMenu::kOriginLegalityLabel, true);
    if (hasPendingChanges) add(MenuAction::Review, "Review Pending Changes", true);
    add(MenuAction::Close, "Cancel", true);
    return result;
}

bool handleActions(TrainerViewScreen& screen, State& state, u64 down, const TouchInput& touch) {
    if (!state.actions) return false;
    std::string error;
    const bool occupied = selectedRecord(screen, state, error).has_value();
    auto* staged = editor(screen);
    const auto actions = gen4Actions(
        state, occupied, staged != nullptr, staged && staged->hasChanges());
    state.actionRow = std::clamp(
        state.actionRow, 0, std::max(0, static_cast<int>(actions.count) - 1));
    if (actions.count == 0) return true;
    // Touch-down can become a swipe; do not steal the selected action row.
    const int actionTap = screen.touchedButtonId(touch);
    if (actionTap >= 41600 && actionTap < 41600 + static_cast<int>(actions.count)) { state.actionRow = actionTap - 41600; down |= HidNpadButton_A; }
    // Real disabled options are greyed and not focusable with A or touch.
    // Controller navigation advances through *enabled* staged commands only.
    if (!actions[static_cast<std::size_t>(state.actionRow)].enabled)
        for (int i = 0; i < static_cast<int>(actions.count); ++i)
            if (actions[static_cast<std::size_t>(i)].enabled) {
                state.actionRow = i;
                break;
            }
    const int direction = (down & HidNpadButton_Up) ? -1 :
                          (down & HidNpadButton_Down) ? 1 : 0;
    if (direction != 0) {
        for (int step = 0; step < static_cast<int>(actions.count); ++step) {
            const int next = (state.actionRow + direction + static_cast<int>(actions.count)) %
                             static_cast<int>(actions.count);
            state.actionRow = next;
            if (actions[static_cast<std::size_t>(next)].enabled) break;
        }
    }
    if (down & HidNpadButton_B) {
        state.actions = false;
        return true;
    }
    if (!(down & HidNpadButton_A)) return true;

    const auto entry = actions[static_cast<size_t>(state.actionRow)];
    if (!entry.enabled) {
        if (entry.action == MenuAction::Add) {
            const auto& why = bridge(screen).stagedPokemonUnavailableReason();
            screen.postStatus(why.empty()
                ? "Gen IV Add needs a strictly validated staged save" : why, 420);
            return true;
        }
        switch (entry.action) {
            case MenuAction::AddMasterVault:
                screen.postStatus("Master Vault is intentionally not started", 300);
                break;
            case MenuAction::AddBank:
                screen.postStatus("Party-to-Bank transfer remains locked in this Gen IV slice", 300);
                break;
            case MenuAction::TransferGame:
                screen.postStatus("Live source writes remain hard disabled", 300);
                break;
            case MenuAction::Clone:
                screen.postStatus("Party Clone is not enabled yet; Box Clone is staged and available", 300);
                break;
            default:
                screen.postStatus("This Generation IV action is not enabled", 240);
                break;
        }
        return true;
    }

    switch (entry.action) {
        case MenuAction::View:
            beginViewOrEdit(screen, SessionModel::Mode::View);
            break;
        case MenuAction::Edit:
            beginViewOrEdit(screen, SessionModel::Mode::Edit);
            break;
        case MenuAction::MovePokemon:
            // This menu shortcut enters the SAME hold-Y picker and carried
            // visual as Gens I-III, rather than a second native move handler.
            state.actions = false;
            if (!ClassicPackedMove::beginGen4FromAction(screen, state.box, state.slot))
                screen.postStatus("Gen IV move could not start", 260);
            break;
        case MenuAction::Add:
            beginCreate(screen);
            break;
        case MenuAction::Clone: {
            if (!staged || state.target != TargetKind::Box) break;
            const auto box = Utils::promptNumber("Clone to Gen IV Box", state.box + 1, 1, 18);
            if (!box.accepted) break;
            const auto slot = Utils::promptNumber("Clone to Slot", state.slot + 1, 1, 30);
            if (!slot.accepted) break;
            if (!staged->stageCloneBoxPokemon(
                    static_cast<size_t>(state.box), static_cast<size_t>(state.slot),
                    static_cast<size_t>(box.value - 1), static_cast<size_t>(slot.value - 1),
                    &error)) {
                screen.postStatus(error.empty() ? "Generation IV Clone failed" : error, 360);
            } else if (refreshPresentation(screen)) {
                screen.postStatus("Generation IV Clone staged; external source unchanged", 300);
            }
            break;
        }
        case MenuAction::Release:
            state.actions = false;
            state.confirmRelease = true;
            break;
        case MenuAction::Legality:
            state.actions = false;
            state.legality = true;
            break;
        case MenuAction::Review:
            state.actions = false;
            state.review = true;
            break;
        case MenuAction::Close:
            state.actions = false;
            break;
        default:
            break;
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
    // A never stages a destructive edit. Release requires Y confirmation;
    // a stray A in this dialog is intentionally inert.
    if (!(down & HidNpadButton_Y)) return true;
    auto* staged = editor(screen);
    std::string error;
    if (!staged || state.target != TargetKind::Box ||
        !staged->stageReleaseBoxPokemon(
            static_cast<size_t>(state.box), static_cast<size_t>(state.slot), &error)) {
        screen.postStatus(error.empty() ? "Generation IV Release failed" : error, 360);
    } else if (refreshPresentation(screen)) {
        screen.postStatus("Generation IV Release staged; external source unchanged", 300);
    }
    state.confirmRelease = false;
    state.actions = false;
    return true;
}

// Verified export of the staged copy into the app-owned exports/gen4 root. Never writes the
// emulator source; the staged workspace is kept so the user can continue editing.
void exportStagedGen4Copy(TrainerViewScreen& screen) {
    auto* staged = editor(screen);
    if (!staged || !staged->hasChanges()) {
        screen.postStatus("No staged Generation IV changes to export", 240);
        return;
    }
    std::string error;
    const std::string root = PokeBank::Paths::gen4ExportsRoot();
    const std::string timestamp = Utils::getTimestamp();
    if (!PokeBank::Paths::ensureDirectoryTree(root, &error) ||
        !PokeBank::Paths::isSafeTimestamp(timestamp)) {
        screen.postStatus(error.empty() ? "Could not prepare the Generation IV export folder" : error, 420);
        return;
    }
    PokeVault::Integration::Gen4::Gen4ExportRequest request;
    request.rootDirectory = root;
    request.directoryStem = PokeBank::Paths::sanitizeComponent(staged->exactGameId()) + "_" + timestamp;
    request.sourcePath = screen.backupDir;
    // The Open snapshot: export re-reads the source and fails closed if it changed since.
    request.sourceSnapshot = screen.gen4SourceSnapshot;
    request.timestamp = timestamp;
#ifdef POKEVAULT_GIT_COMMIT
    request.applicationSha = POKEVAULT_GIT_COMMIT;
#endif
    const auto result = PokeVault::Integration::Gen4::publishVerifiedGen4Export(*staged, request);
    if (!result.success) {
        screen.postStatus(result.error.empty() ? "Verified Generation IV export failed" : result.error, 480);
        return;
    }
    screen.postStatus("Verified copy exported: " + result.directory + "/edited.sav (source unchanged)", 600);
}

bool handleReview(TrainerViewScreen& screen, State& state, u64 down) {
    if (!state.review) {
        state.confirmDiscardAll = false;
        return false;
    }
    if (state.confirmDiscardAll) {
        if (down & HidNpadButton_B) {
            state.confirmDiscardAll = false;
            screen.controllerNavigation.reset();
            return true;
        }
        // A/X are inert here: only an explicit second Y discards every staged edit.
        if (!(down & HidNpadButton_Y)) return true;
        state.confirmDiscardAll = false;
        if (auto* staged = editor(screen)) {
            staged->discard();
            if (refreshPresentation(screen))
                screen.postStatus("All staged Generation IV edits discarded; source unchanged", 300);
        }
        state.review = false;
        state.actions = true;
        screen.controllerNavigation.reset();
        return true;
    }
    if (down & HidNpadButton_X) {
        exportStagedGen4Copy(screen);
        return true;
    }
    if (down & HidNpadButton_B) {
        state.review = false;
        state.actions = true;
        screen.controllerNavigation.reset();
        return true;
    }
    if (down & HidNpadButton_Y) {
        const auto* staged = editor(screen);
        if (staged && staged->hasChanges()) state.confirmDiscardAll = true;
        screen.controllerNavigation.reset();
        return true;
    }
    return true;
}

std::string gen4DexLabel(uint16_t value) {
    std::string number = std::to_string(value);
    while (number.size() < 3) number = "0" + number;
    return "#" + number + "  " + Names::getSpeciesName(value);
}

std::string pickerValueLabel(TrainerViewScreen& screen, const State& state,
                             PickerTarget target, uint16_t value) {
    if (target == PickerTarget::Species)
        return gen4DexLabel(value);
    if (target == PickerTarget::HeldItem)
        return value == 0 ? "(none)" :
            Names::machineDisplayLabel(bridge(screen).sourceSave().rawFamily(), value,
                                       Names::getItemName(value));
    if (target == PickerTarget::Language)
        return Enums::getLanguageName(static_cast<uint8_t>(value));
    if (target == PickerTarget::Ball)
        return Enums::getBallName(static_cast<uint8_t>(value));
    if (target == PickerTarget::MetLocation) {
        const auto preview = workingRecord(state, bridge(screen).sourceSave().rawFamily());
        std::string label = preview
            ? Names::getMetLocationDisplayName(preview->originVersion(), value)
            : std::string("Unknown location");
        if (preview)
            label += "  |  Met Lv. " + std::to_string(preview->metLevel());
        return label;
    }
    if (target == PickerTarget::Pokerus)
        return value == 0 ? "None" : value == 0x10 ? "Cured" : "Infected";
    if (target == PickerTarget::Form) {
        const uint16_t species = state.session.working ? state.session.working->species() : 0;
        return gen4FormLabel(species, static_cast<uint8_t>(value));
    }
    if (target == PickerTarget::Move)
        return MoveUI::rowLabel(value, bridge(screen).sourceSave().rawFamily());
    if (target == PickerTarget::Nature)
        return Trainer::getNatureName(static_cast<uint8_t>(value));
    if (state.session.working) {
        const uint16_t abilityId =
            state.session.working->abilityForSlot(static_cast<uint8_t>(value));
        return abilityId == 0
            ? "Ability Slot " + std::to_string(value + 1)
            : std::string(Trainer::getAbilityName(abilityId)) +
                " (Slot " + std::to_string(value + 1) + ")";
    }
    return {};
}

// Generation IV adapter of the ONE shared editor workspace
// (SharedPokemonEditorSurface.inc). Every Gen IV string, setter and catalog
// below is the one the Gen IV surface has always used.
struct Gen4Editor {
    using Record = Pokemon::Pokemon4ReadOnly;
    static constexpr auto generation = Shared::Generation::Gen4;
    static constexpr int kDetailRows = static_cast<int>(detailLabels.size());
    static constexpr int kMaxSpecies = 493;
    static constexpr bool kCompactForm = false;
    static constexpr const char* kDetailReadOnly =
        "This Generation IV field is visible but read-only in the current G4-04 slice";
    static constexpr const char* kValueReadOnly = "This Generation IV value is read-only";
    static constexpr const char* kHeldItemTitle = "Held Item — Generation IV";
    static constexpr const char* kMovePickerTitle = "Move — Generation IV";
    static constexpr const char* kMovePickerSubtitle =
        "Empty + compatible moves only • exact Gen IV Acc / Pwr / PP";
    static constexpr const char* kMoveEditorSubtitle =
        "Gen IV move compatibility checked • Origin & Legality: incomplete";
    static constexpr const char* kDataTitle = "Origin & Legality";

    static State& gen4(Surface::EditorState& state) { return static_cast<State&>(state); }
    static const State& gen4(const Surface::EditorState& state) {
        return static_cast<const State&>(state);
    }
    static Enums::GameVersion family(const TrainerViewScreen& screen) {
        return bridge(screen).sourceSave().rawFamily();
    }

    static bool hasWorking(const TrainerViewScreen&, const Surface::EditorState& state) {
        return gen4(state).session.working.has_value();
    }
    static bool editable(const TrainerViewScreen&, const Surface::EditorState& state) {
        return gen4(state).session.editable();
    }
    static bool passive(const TrainerViewScreen&, const Surface::EditorState& state) {
        return gen4(state).session.mode == SessionModel::Mode::View;
    }
    static bool creating(const TrainerViewScreen&, const Surface::EditorState& state) {
        return gen4(state).session.creating();
    }
    static bool confirmExit(const TrainerViewScreen&, const Surface::EditorState& state) {
        return gen4(state).session.confirmExit;
    }
    static std::optional<Record> preview(const TrainerViewScreen& screen,
                                         const Surface::EditorState& state) {
        return workingRecord(gen4(state), family(screen));
    }
    static void previewFailed(TrainerViewScreen& screen, Surface::EditorState& state) {
        auto& s = gen4(state);
        if (s.session.working)
            screen.postStatus("Generation IV editor draft failed strict PK4 preview", 360);
        s.session.close();
    }
    static void sessionBack(TrainerViewScreen&, Surface::EditorState& state) {
        gen4(state).session.back();
    }
    static void continueEditing(TrainerViewScreen&, Surface::EditorState& state) {
        gen4(state).session.continueEditing();
    }
    static void discard(TrainerViewScreen& screen, Surface::EditorState& state) {
        gen4(state).session.discardDraft();
        screen.postStatus("Gen IV draft cancelled; original source remains unchanged", 120);
    }
    static void keep(TrainerViewScreen& screen, Surface::EditorState& state) {
        if (keepWorkspace(screen)) gen4(state).session.confirmExit = false;
    }

    static const char* detailLabel(int row) { return detailLabels[static_cast<size_t>(row)]; }
    static bool detailEditable(const TrainerViewScreen&, const Surface::EditorState& state,
                               int row, const Record& p) {
        return UI::Gen4SharedEditorSurface::detailEditable(gen4(state), row, p);
    }
    // Locked in every mode: SID and Origin, a transferred Pokemon's encounter data, a fixed form.
    static bool detailLocked(const TrainerViewScreen&, const Surface::EditorState& state,
                             int row, const Record& p) {
        const auto& s = gen4(state);
        switch (row) {
            case 6: case 10: return true;
            case 11: case 13: return !UI::Gen4SharedEditorSurface::nativeGen4Origin(p);
            case 15: return s.session.working && s.session.working->formCount() <= 1;
            default: return false;
        }
    }
    static std::string detailValue(const TrainerViewScreen&, const Surface::EditorState& state,
                                   const Record& p, int row) {
        const auto& s = gen4(state);
        if (row == 15 && s.session.working && s.session.working->formCount() <= 1)
            return std::string("Default (fixed)");
        Pokemon::Pokemon4ReadOnlyView view(p);
        return UI::Gen4SharedEditorSurface::detailValue(p, view, row);
    }
    static bool editDetail(TrainerViewScreen& screen, Surface::EditorState& state, int row,
                           const Record& p) {
        return UI::Gen4SharedEditorSurface::editDetail(screen, gen4(state), row, p);
    }
    static void passiveActivate(TrainerViewScreen& screen, Surface::EditorState& state, int row) {
        const auto& s = gen4(state);
        if (row == 15 && s.session.working && s.session.working->formCount() <= 1)
            screen.postStatus("This Gen IV Pokémon has only its default form", 135);
    }

    static std::array<uint8_t,6> ivs(const Record& p) { return p.ivs(); }
    static std::array<uint8_t,6> evs(const Record& p) { return p.evs(); }
    static std::array<uint16_t,6> stats(const TrainerViewScreen&, const Surface::EditorState&,
                                        const Record& p) {
        Pokemon::Pokemon4ReadOnlyView view(p);
        return {{view.statHPMax(),view.statATK(),view.statDEF(),
                 view.statSPE(),view.statSPA(),view.statSPD()}};
    }
    static void setIv(TrainerViewScreen&, Surface::EditorState& state, size_t stat, uint8_t value) {
        gen4(state).session.working->setIV(stat, value);
    }
    static void setEv(TrainerViewScreen&, Surface::EditorState& state, size_t stat, uint8_t value) {
        gen4(state).session.working->setEV(stat, value);
    }
    static bool pidRowEditable(const TrainerViewScreen&, const Surface::EditorState& state,
                               int row, const Record& p) {
        return pidLinkedRowEditable(gen4(state), row, p);
    }
    static uint8_t gender(const Record& p) { return p.gender(); }
    static std::string pidRowValue(const TrainerViewScreen&, const Record& raw, int row) {
        Pokemon::Pokemon4ReadOnlyView view(raw);
        switch (row) {
            case 6: return Trainer::getNatureName(view.nature());
            case 7: return genderText(raw.gender());
            case 8: return view.isShiny(raw.id32(), {}) ? "Yes" : "No";
            case 9: return Trainer::getAbilityName(raw.ability());
            default: return std::to_string(raw.pid());
        }
    }
    static bool editPidRow(TrainerViewScreen& screen, Surface::EditorState& state, int row,
                           const Record& p) {
        return UI::Gen4SharedEditorSurface::editPidRow(screen, gen4(state), row, p);
    }

    static std::array<uint16_t,4> moves(const Record& p) { return p.moves(); }
    static std::array<uint8_t,4> pp(const Record& p) { return p.pp(); }
    static std::array<uint8_t,4> ppUps(const Record& p) { return p.ppUps(); }
    static uint16_t workingMove(const TrainerViewScreen&, const Surface::EditorState& state, int slot) {
        return gen4(state).session.working->moves()[static_cast<size_t>(slot)];
    }
    static uint8_t workingPP(const TrainerViewScreen&, const Surface::EditorState& state, int slot) {
        return gen4(state).session.working->pp()[static_cast<size_t>(slot)];
    }
    static uint8_t workingPPUps(const TrainerViewScreen&, const Surface::EditorState& state, int slot) {
        return gen4(state).session.working->ppUps()[static_cast<size_t>(slot)];
    }
    static void setMove(TrainerViewScreen&, Surface::EditorState& state, int slot, uint16_t move) {
        gen4(state).session.working->setMove(static_cast<size_t>(slot), move);
    }
    static void setPP(TrainerViewScreen&, Surface::EditorState& state, int slot, uint8_t value) {
        gen4(state).session.working->setPP(static_cast<size_t>(slot), value);
    }
    static void setPPUps(TrainerViewScreen&, Surface::EditorState& state, int slot, uint8_t value) {
        gen4(state).session.working->setPPUps(static_cast<size_t>(slot), value);
    }
    static int maxPP(const TrainerViewScreen& screen, uint16_t move, uint8_t ppUps) {
        return moveMaxPP(move, ppUps, family(screen));
    }
    static std::vector<uint16_t> moveChoices(TrainerViewScreen& screen, Surface::EditorState& state) {
        return gen4MoveChoices(screen, gen4(state));
    }
    static void moveStatus(TrainerViewScreen& screen, const Surface::EditorState& state,
                           const Record& raw, uint16_t move, const char*& statusOut,
                           Color& colorOut) {
        const auto& s = gen4(state);
        bool existingSource = false;
        if (!s.session.baselineEncrypted.empty()) {
            Pokemon::Pokemon4ReadOnly baseline(s.session.baselineEncrypted, family(screen));
            if (baseline.valid()) {
                for (const uint16_t sourceMove : baseline.moves())
                    if (sourceMove != 0 && sourceMove == move) existingSource = true;
            }
        }
        const auto descriptor =
            PokeVault::Integration::Gen4EditorProvider::descriptorForSource(
                screen.sourceGameId, editor(screen) != nullptr);
        auto result = PokeBank::UIModel::ExactFormatEditor::MoveCompatibilityResult::Invalid;
        if (move != 0 && descriptor)
            result = descriptor->moves.evaluate(
                {screen.sourceGameId, raw.species(), raw.form(), move, existingSource});
        const char* status = move == 0 ? "-" : "Invalid";
        Color statusColor = move == 0 ? Colors::TextDim : Colors::Error;
        using MoveResult = PokeBank::UIModel::ExactFormatEditor::MoveCompatibilityResult;
        if (move != 0 && result == MoveResult::Compatible) {
            status = "OK";
            statusColor = Colors::Success;
        } else if (result == MoveResult::PreserveExisting) {
            status = "Preserved";
            statusColor = Colors::Warning;
        } else if (result == MoveResult::Unsupported) {
            status = "Transfer";
            statusColor = Colors::Warning;
        }
        statusOut = status;
        colorOut = statusColor;
    }
    // Exact DP / Pt / HGSS game group for the shared Moves panel and chooser facts.
    static Enums::GameVersion moveGame(const TrainerViewScreen& screen) { return family(screen); }

    static std::string pickerLabel(TrainerViewScreen& screen, const Surface::EditorState& state,
                                   PickerTarget target, uint16_t value) {
        return pickerValueLabel(screen, gen4(state), target, value);
    }
    static bool applyPicker(TrainerViewScreen& screen, Surface::EditorState& state,
                            PickerTarget target, uint16_t value) {
        return applyGen4Picker(screen, gen4(state), target, value);
    }
    static std::array<uint8_t,2> speciesPreviewTypes(const TrainerViewScreen&, uint16_t species) {
        const auto& speciesPersonal = Pokemon::getPersonalInfo4HGSS(species, 0);
        return {SharedSpeciesPicker::canonicalToClassicType(speciesPersonal.type1),
                SharedSpeciesPicker::canonicalToClassicType(speciesPersonal.type2)};
    }
    static std::string speciesLabel(uint16_t value) { return gen4DexLabel(value); }
    static std::string heldItemArtKey(TrainerViewScreen& screen, uint16_t value) {
        const auto disc = UI::ClassicMachineArtwork::artKey(screen.trainer.getGameGroup(), value);
        return disc.empty() ? Names::getItemName(value) : disc;
    }

    static Surface::ChromeInfo chrome(const TrainerViewScreen&, const Surface::EditorState& state,
                                      const Record& raw) {
        const auto& s = gen4(state);
        Pokemon::Pokemon4ReadOnlyView view(raw);
        Surface::ChromeInfo info;
        info.species = raw.species();
        info.nickname = Utils::utf16ToUtf8(raw.nickname());
        info.level = view.level();
        info.gender = raw.gender();
        info.shiny = view.isShiny(raw.id32(), {});
        info.mode = s.session.mode == SessionModel::Mode::Create
            ? Context::SurfaceMode::CreateDraft
            : s.session.mode == SessionModel::Mode::Edit
                ? Context::SurfaceMode::EditStaged
                : Context::SurfaceMode::ViewReadOnly;
        info.context = s.target == TargetKind::Party
            ? "Party / Slot " + std::to_string(s.slot + 1)
            : "Box " + std::to_string(s.box + 1) + " / Slot " + std::to_string(s.slot + 1);
        return info;
    }
    static std::array<uint8_t,2> portraitTypes(const TrainerViewScreen&, const Record& raw) {
        return {{raw.personal().type1, raw.personal().type2}};
    }
    static std::vector<Surface::DataRow> dataRows(TrainerViewScreen& screen,
                                                  const Surface::EditorState&, const Record& r) {
        const auto* raw = &r;
        Pokemon::Pokemon4ReadOnlyView view(r);
        return {
            {"Game", gameName(screen.sourceGameId)},
            {"Origin", Enums::getOriginGameName(raw->originVersion())},
            {"Language", Enums::getLanguageName(raw->language())},
            {"Ball", Enums::getBallName(view.ball())},
            {"Met", [&] {
                const uint16_t location = view.metLocation();
                const std::string label = location == 0 ? "(not recorded)"
                    : Names::getMetLocationDisplayName(raw->originVersion(), location);
                return label + " / Lv. " + std::to_string(raw->metLevel());
            }()},
            {"Friendship", std::to_string(raw->friendship())},
            {"Pokérus", PokeBank::UIModel::Gen4DataPresentation::pokerus(raw->pokerusState())},
            {"Fateful", raw->fatefulEncounter() ? "Yes" : "No"},
            {"Egg status", raw->isEgg() ? "Unhatched" : "Not egg"},
            {"OT gender", raw->originalTrainerGender() == 0 ? "Male" : "Female"},
            {"PC markings", PokeBank::UIModel::Gen4DataPresentation::markings(raw->markings())},
            // Structure + move guardrails run; no full encounter/event legality checker exists.
            {"Legality", PokeBank::UIModel::SharedEditorPresentation::legalityLabel(
                PokeBank::UIModel::SharedEditorPresentation::LegalityState::Incomplete)}
        };
    }
};

bool handleWorkspace(TrainerViewScreen& screen, State& state, u64 down, const TouchInput& touch) {
    if (!workspaceActive(state)) return false;
    return Surface::handleWorkspace<Gen4Editor>(screen, state, down, touch);
}

void drawWorkspace(TrainerViewScreen& screen, PKSEFramebuffer& fb, State& state) {
    Surface::drawWorkspace<Gen4Editor>(screen, fb, state);
}

PokeBank::UIModel::NarrowPokemonMenu::Model narrowGen4Actions(
    const MenuActionSet& actions, bool /*occupied*/) {
    // gen4Actions already lists the shared order; each row keeps its native index, and a
    // disabled command (no validated staged save) stays visible but grey and inert.
    PokeBank::UIModel::NarrowPokemonMenu::Model model{};
    for (int i = 0; i < static_cast<int>(actions.count); ++i) {
        const auto& entry = actions[static_cast<std::size_t>(i)];
        model.append(entry.label, entry.enabled ? i : -1);
    }
    return model;
}

void drawActions(TrainerViewScreen& screen, PKSEFramebuffer& fb, State& state) {
    std::string error;
    const auto pokemon = selectedRecord(screen, state, error);
    const bool occupied = pokemon.has_value();
    auto* staged = editor(screen);
    const auto actions = gen4Actions(
        state, occupied, staged != nullptr, staged && staged->hasChanges());
    screen.drawGSCOverlay(fb);
    // The old narrow popup is a presentation-only projection: touch hitboxes
    // carry exactly the enabled staged Gen IV native index used by A.
    const auto model = narrowGen4Actions(actions, occupied);
    const std::string title = pokemon
        ? std::string(Names::getSpeciesName(pokemon->species())) : "Empty Slot";
    drawNarrowPokemonActions(fb, screen.touchButtons, model, state.actionRow,
                            title,
                            screen.legacyProviderLabel() + " NDS SOURCE / READ ONLY", 41600);
    SharedEditorChrome::Presentation::PickerFooterSpec footer;
    footer.page = false;
    SharedEditorChrome::drawPickerFooter(fb, footer);
}

// Second step of Review -> Discard All. Drawn over the review list; nothing changes until Y.
// The confirm owns the frame's touch: the review list's rows and footer are not tappable under it.
void drawDiscardAllConfirm(TrainerViewScreen& screen, PKSEFramebuffer& fb, const char* generation) {
    SharedEditorChrome::beginModal(screen.touchButtons);
    fb.drawFilledRect(0, 0, fb.getWidth(), fb.getHeight() - kNavBarH, Color(0, 0, 0, 150));
    constexpr int w = 760, h = 240;
    const int x = (fb.getWidth() - w) / 2;
    const int y = (fb.getHeight() - kNavBarH - h) / 2;
    fb.drawFilledRoundedRect(x, y, w, h, 18, Colors::Panel);
    fb.drawRoundedRect(x, y, w, h, 18, Colors::Divider, 1);
    fb.drawText(x + 34, y + 30, "Discard all staged changes?", Colors::Text, TextStyle::Heading);
    fb.drawText(x + 34, y + 96,
                std::string("Every pending ") + generation + " edit in this workspace is lost.",
                Colors::Text, TextStyle::Body);
    fb.drawText(x + 34, y + 140, "Export Copy first to keep them. The original save is never written.",
                Colors::TextDim, TextStyle::Caption);
    drawNavBar(fb, {{"Y", "Discard All"}, {"B", "Keep Changes"}});
}

void drawReview(TrainerViewScreen& screen, PKSEFramebuffer& fb, const State& state) {
    auto* staged = editor(screen);
    fb.drawFilledRect(0, 0, fb.getWidth(), fb.getHeight(), Colors::Background);
    fb.drawText(32, 24, "Review Pending Changes", Colors::Text, TextStyle::Heading);
    fb.drawText(32, 62,
                "Generation IV app-owned staged workspace • external source bytes unchanged",
                Colors::TextDim, TextStyle::Caption);
    const bool changed = staged && staged->hasChanges();
    fb.drawFilledRoundedRect(44, 118, fb.getWidth() - 88, 110, 12, Colors::Panel);
    fb.drawText(64, 138, changed ? "Staged PK4 edits are pending" : "No staged Gen IV changes",
                changed ? Colors::Text : Colors::TextDim, TextStyle::Body);
    fb.drawText(64, 178,
                changed ? "Strict PK4 serialize + Storage CRC + full save reparse: PASS"
                        : "Original source remains the current baseline",
                Colors::TextDim, TextStyle::Caption);
    fb.drawText(44, 270, "Original emulator source: HARD READ ONLY", Colors::Accent);
    fb.drawText(44, 308, "Create/Edit/Box Clone/Release use staged bytes only; source remains immutable",
                Colors::TextDim, TextStyle::Caption);
    if (changed) {
        // Original-versus-staged comparison, from strict parses of both buffers.
        const auto lines = PokeVault::Integration::Gen4::describeStagedSlotChanges(*staged);
        constexpr size_t kShown = 8;
        int y = 352;
        fb.drawText(44, y, "Changed slots (original -> staged)", Colors::Text, TextStyle::Body);
        y += 36;
        for (size_t i = 0; i < lines.size() && i < kShown; ++i, y += 30)
            fb.drawText(64, y, lines[i], Colors::TextDim, TextStyle::Caption);
        if (lines.size() > kShown)
            fb.drawText(64, y, "+" + std::to_string(lines.size() - kShown) + " more",
                        Colors::TextDim, TextStyle::Caption);
        if (state.confirmDiscardAll) {
            drawDiscardAllConfirm(screen, fb, "Generation IV");
        } else {
            drawNavBar(fb, {{"X", "Export Copy"}, {"Y", "Discard All Staged"}, {"B", "Back"}});
        }
    } else {
        drawNavBar(fb, {{"B", "Back"}});
    }
}

void drawLegality(TrainerViewScreen& screen, PKSEFramebuffer& fb, State& state) {
    screen.drawGSCOverlay(fb);
    fb.drawFilledRect(0, 0, fb.getWidth(), fb.getHeight() - kNavBarH, Color(0, 0, 0, 120));
    constexpr int x = 210, y = 82, w = 860, h = 500;
    fb.drawFilledRoundedRect(x, y, w, h, 18, Colors::Panel);
    fb.drawRoundedRect(x, y, w, h, 18, Colors::Divider, 1);
    fb.drawText(x + 24, y + 18, PokeBank::UIModel::SharedEditorPresentation::kOriginLegalityHeading,
                Colors::Text, TextStyle::Heading);
    fb.drawText(x + 24, y + 52,
                "PK4 structure and move guardrails checked • no full legality checker",
                Colors::TextDim, TextStyle::Caption);

    std::string error;
    auto record = selectedRecord(screen, state, error);
    if (!record) {
        fb.drawText(x + 34, y + 110, error.empty() ? "No valid Pokemon selected" : error,
                    Colors::Warning, TextStyle::Body);
        drawNavBar(fb, {{"B", "Back"}});
        return;
    }

    fb.drawText(x + 34, y + 104, "Exact source", Colors::TextDim);
    fb.drawText(x + 250, y + 104, gameName(screen.sourceGameId), Colors::Text);
    fb.drawText(x + 34, y + 142, "PK4 structure", Colors::TextDim);
    fb.drawText(x + 250, y + 142, record->valid() ? "PASS" : "INVALID",
                record->valid() ? Colors::Success : Colors::Error);
    fb.drawText(x + 34, y + 180, "Source mutation", Colors::TextDim);
    fb.drawText(x + 250, y + 180, "HARD DISABLED", Colors::Accent);
    fb.drawText(x + 34, y + 218, "Legality", Colors::TextDim);
    fb.drawText(x + 250, y + 218, PokeBank::UIModel::SharedEditorPresentation::legalityLabel(
                    PokeBank::UIModel::SharedEditorPresentation::LegalityState::Incomplete),
                Colors::Warning);

    int rowY = y + 270;
    const auto moves = record->moves();
    for (size_t i = 0; i < moves.size(); ++i) {
        const uint16_t move = moves[i];
        const auto availability = PokeVault::Integration::Gen4MoveCompatibility::classify(
            screen.sourceGameId, record->species(), record->form(), move, true);
        const char* status = "Invalid";
        Color color = Colors::Error;
        using A = PokeVault::Integration::Gen4MoveCompatibility::Availability;
        if (move == 0) {
            status = "Empty";
            color = Colors::TextDim;
        } else if (availability == A::Direct) {
            status = "OK";
            color = Colors::Success;
        } else if (availability == A::Transfer) {
            status = "Transfer";
            color = Colors::Warning;
        } else if (availability == A::Preserved) {
            status = "Preserved";
            color = Colors::Warning;
        }
        const std::string moveName = move == 0 ? "Empty" : Names::getMoveName(move);
        fb.drawText(x + 34, rowY, "Move " + std::to_string(i + 1), Colors::TextDim,
                    TextStyle::Caption);
        fb.drawText(x + 150, rowY, moveName, Colors::Text, TextStyle::Caption);
        fb.drawText(x + 650, rowY, status, color, TextStyle::Caption);
        rowY += 36;
    }
    drawNavBar(fb, {{"B", "Back"}});
}

void drawReleaseConfirm(TrainerViewScreen& screen, PKSEFramebuffer& fb, State& state) {
    screen.drawGSCOverlay(fb);
    SharedEditorChrome::beginModal(screen.touchButtons);
    fb.drawFilledRect(0, 0, fb.getWidth(), fb.getHeight() - kNavBarH, Color(0, 0, 0, 120));
    constexpr int w = 760, h = 260;
    const int x = (fb.getWidth() - w) / 2;
    const int y = (fb.getHeight() - kNavBarH - h) / 2;
    fb.drawFilledRoundedRect(x, y, w, h, 18, Colors::Panel);
    fb.drawRoundedRect(x, y, w, h, 18, Colors::Divider, 1);
    fb.drawText(x + 34, y + 30, "Release this Pokemon?", Colors::Text, TextStyle::Heading);
    fb.drawText(x + 34, y + 88,
                "Box " + std::to_string(state.box + 1) +
                    " / Slot " + std::to_string(state.slot + 1),
                Colors::TextDim, TextStyle::Body);
    fb.drawText(x + 34, y + 130, "This removes it only from the app-owned staged copy.",
                Colors::Text, TextStyle::Body);
    fb.drawText(x + 34, y + 166, "The original DraStic/emulator save is never written.",
                Colors::TextDim, TextStyle::Caption);
    drawNavBar(fb, {{"Y", "Confirm Release"}, {"B", "Cancel"}});
}

// The ONE per-item quantity query of the Gen IV bag (UI/Gen4BagQuantity.h), shared with the
// Items footer in TrainerViewScreenBase.inc so the footer only offers A where it edits.
using Gen4QuantityRule = PokeBank::UIModel::Gen4BagQuantity::Rule;
Gen4QuantityRule gen4QuantityRule(const TrainerViewScreen& screen, size_t pocket, uint16_t id) noexcept {
    // The exact game resolves through Thread I's QuantityPolicy; unknown ids get no editor.
    PokeVault::Inventory::QuantityPolicy::Game game{};
    if (!PokeVault::Inventory::QuantityPolicy::gameFromId(screen.sourceGameId, game)) return {};
    return PokeBank::UIModel::Gen4BagQuantity::rule(game, pocket, id);
}

// Touch ids the shared Add picker publishes (input hit-tests the same rectangles directly).
constexpr int kGen4BagAddTouchBase=91000;

PokeVault::Inventory::QuantityPolicy::ItemQuantityRule gen4PolicyRule(
    const TrainerViewScreen& screen, size_t pocket, uint16_t id) noexcept {
    PokeVault::Inventory::QuantityPolicy::Game game{};
    if (!PokeVault::Inventory::QuantityPolicy::gameFromId(screen.sourceGameId, game)) return {};
    return PokeBank::UIModel::Gen4BagQuantity::policyRule(game, pocket, id);
}

// Highlight the row holding `id` in the visible pocket (after an Add or a focus-existing pick).
void focusGen4Item(TrainerViewScreen& screen,size_t pocket,uint16_t id) {
    if(pocket>=screen.trainer.items.size())return;
    const auto visible=screen.visibleItemIndices();
    const auto& rows=screen.trainer.items[pocket];
    for(size_t i=0;i<visible.size();++i)
        if(rows[static_cast<size_t>(visible[i])].itemId==id) {
            screen.selectedItemIndex=static_cast<int>(i);
            screen.currentPage=screen.selectedItemIndex/8;
            break;
        }
}

// Stage one shared-flow Add through the real Gen IV writer; a refused write or refresh restores
// every staged byte. Used by the picker's A and by the Key Item warning's A-confirm.
bool stageGen4BagAdd(TrainerViewScreen& screen,State& state,size_t addPocket,uint16_t id,
                     uint16_t quantity) {
    auto* current=editor(screen);
    if(!current) {
        screen.postStatus("Gen IV staged workspace unavailable",280);
        return false;
    }
    auto backup=*current;
    std::string error;
    if(!current->stageBagAdd(addPocket,id,quantity,&error)) {
        screen.postStatus(error,300);
        return false;
    }
    if(!bridge(screen).refreshStagedPokemonPresentation(error)) {
        *current=std::move(backup);
        std::string restored;
        (void)bridge(screen).refreshStagedPokemonPresentation(restored);
        screen.postStatus("Gen IV Add refresh failed; previous stage retained: "+error,360);
        return false;
    }
    state.itemAddPickerActive=false;
    state.itemAddChoices.clear();
    focusGen4Item(screen,addPocket,id);
    screen.controllerNavigation.reset();
    screen.postStatus("Gen IV item added to staged workspace ONLY (x"+
                      std::to_string(quantity)+"); A on it sets the quantity",280);
    return true;
}

bool handleItemQuantity(TrainerViewScreen& screen,State& state,u64 down,
                        u64 held,int stickX,int stickY,const TouchInput& touch) {
    const bool itemsScreen=screen.detailViewActive &&
        screen.selectedMode==TrainerViewScreen::ViewMode::Items;
    if(state.itemAddPickerActive) {
        u64 keys=screen.controllerNavigation.apply(
            down,held,stickX,stickY,
            HidNpadButton_Up,HidNpadButton_Down,
            HidNpadButton_Left,HidNpadButton_Right);
        if(keys&HidNpadButton_B) {
            state.itemAddPickerActive=false;
            state.itemAddChoices.clear();
            screen.controllerNavigation.reset();
            return true;
        }
        const int size=static_cast<int>(state.itemAddChoices.size());
        if(size<=0) {
            state.itemAddPickerActive=false;
            return true;
        }
        // Touch uses the SAME SharedMenuGeometry rectangles the picker draws: a clean tap on a
        // row chooses it (same as A); a vertical swipe that starts on the list only scrolls.
        namespace AddPicker=UI::SharedBagAddPicker;
        const int pageRows=AddPicker::visibleRows(size);
        // Shared scrollbar first: the window is centred on the selection, so the thumb moves
        // the selection to the row that puts the window where the finger put it.
        const auto addBar=SharedScrollbar::handleTouch(state.itemAddScrollbar,SharedScrollbar::sample(touch),
            SharedScrollbar::rows(size,pageRows,AddPicker::firstVisible(state.itemAddIndex,size)));
        if(addBar.changed) state.itemAddIndex=std::clamp(addBar.scroll+pageRows/2,0,size-1);
        if(!addBar.consumed && touch.justReleased() && touch.dragged()) {
            const int dy=touch.deltaY();
            if(AddPicker::onList(touch.startX(),touch.startY(),size) &&
               std::abs(dy)>=24 && std::abs(dy)>std::abs(touch.deltaX())) {
                const int rows=std::max(1,std::abs(dy)/AddPicker::pickerLayout(size).rule.rowHeight);
                state.itemAddIndex=std::clamp(
                    state.itemAddIndex+(dy<0?rows:-rows),0,size-1);
            }
            return true;
        }
        if(!addBar.consumed && touch.justTapped()) {
            const int startHit=AddPicker::hitChoice(touch.startX(),touch.startY(),state.itemAddIndex,size);
            const int endHit=AddPicker::hitChoice(touch.x(),touch.y(),state.itemAddIndex,size);
            if(startHit>=0 && startHit==endHit) {
                state.itemAddIndex=endHit;
                keys|=HidNpadButton_A;
            }
        }
        if(down&HidNpadButton_X) {
            state.itemAddSort = ItemPickerSort::next(state.itemAddSort);
            state.itemAddIndex = ItemPickerSort::reorder(
                state.itemAddChoices, state.itemAddNativeChoices,
                state.itemAddSort, state.itemAddIndex,
                [&](uint16_t id) {
                    return Names::machineDisplayLabel(
                        screen.trainer.getGameGroup(), id, Names::getItemName(id));
                });
            return true;
        }
        if(keys&HidNpadButton_Up)
            state.itemAddIndex=(state.itemAddIndex+size-1)%size;
        if(keys&HidNpadButton_Down)
            state.itemAddIndex=(state.itemAddIndex+1)%size;
        if(keys&(HidNpadButton_L|HidNpadButton_Left))
            state.itemAddIndex=std::max(0,state.itemAddIndex-pageRows);
        if(keys&(HidNpadButton_R|HidNpadButton_Right))
            state.itemAddIndex=std::min(size-1,state.itemAddIndex+pageRows);
        if(keys&HidNpadButton_ZL)
            state.itemAddIndex=std::max(0,state.itemAddIndex-5*pageRows);
        if(keys&HidNpadButton_ZR)
            state.itemAddIndex=std::min(size-1,state.itemAddIndex+5*pageRows);
        if(!(keys&HidNpadButton_A))return true;
        if(!itemsScreen || screen.selectedCategory!=state.itemAddPocket) {
            screen.postStatus("Gen IV Add pocket changed; cancelled",280);
            state.itemAddPickerActive=false;
            return true;
        }
        auto* staged=editor(screen);
        if(!staged) {
            screen.postStatus("Gen IV Add requires a verified staged workspace",280);
            state.itemAddPickerActive=false;
            return true;
        }
        const uint16_t id=state.itemAddChoices[static_cast<size_t>(state.itemAddIndex)];
        const size_t addPocket=static_cast<size_t>(state.itemAddPocket);
        if(!Gen4::gen4BagItemAllowed(staged->layout(),addPocket,id)) {
            screen.postStatus("Gen IV item invalid in selected native pocket",300);
            return true;
        }
        // Shared Gen I-V Add (UI/SharedBagAddFlow.h): a new item is staged at x1 with NO
        // keypad; a held item focuses its existing stack; a full pouch is refused. A on the
        // row afterwards edits the quantity only when the item is quantity-editable.
        namespace BagAdd=PokeBank::UIModel::BagAdd;
        const auto& pouch=screen.trainer.items[addPocket];
        BagAdd::PocketState pocketState{};
        pocketState.usedSlots=pouch.size();
        pocketState.capacity=Gen4::bagLayout(staged->layout())[addPocket].slots;
        for(const auto& stack:pouch)
            if(stack.itemId==id)pocketState.heldInPocket=stack.count;
        const auto addRule=gen4PolicyRule(screen,addPocket,id);
        const auto decision=BagAdd::decidePick(addRule,pocketState,Gen4::bagPocketName(addPocket));
        screen.controllerNavigation.reset();
        switch(decision.outcome) {
            case BagAdd::PickOutcome::NotAllowed:
                screen.postStatus(decision.message,300);
                return true;
            case BagAdd::PickOutcome::FocusExisting:
                state.itemAddPickerActive=false;
                state.itemAddChoices.clear();
                focusGen4Item(screen,addPocket,id);
                screen.postStatus(decision.message,260);
                return true;
            case BagAdd::PickOutcome::PocketFull:
                state.itemAddPickerActive=false;
                state.itemAddChoices.clear();
                screen.postStatus(decision.message,320);
                return true;
            case BagAdd::PickOutcome::StageNew:
                break;
        }
        if(BagAdd::needsKeyItemWarning(addRule)) {
            // Same as Gen I-III: a Key Item is staged only after an A-confirmed warning.
            state.itemAddPickerActive=false;
            state.itemAddChoices.clear();
            state.itemKeyWarningActive=true;
            state.itemKeyWarningPocket=static_cast<int>(addPocket);
            state.itemKeyWarningId=id;
            return true;
        }
        (void)stageGen4BagAdd(screen,state,addPocket,id,decision.quantity);
        return true;
    }
    if(state.itemKeyWarningActive) {
        // A confirms, B cancels with nothing staged; nothing else acts.
        namespace BagAdd=PokeBank::UIModel::BagAdd;
        if(down&HidNpadButton_B) {
            state.itemKeyWarningActive=false;
            screen.controllerNavigation.reset();
            return true;
        }
        if(!(down&HidNpadButton_A))return true;
        state.itemKeyWarningActive=false;
        const size_t pocket=static_cast<size_t>(state.itemKeyWarningPocket);
        if(!itemsScreen || screen.selectedCategory!=state.itemKeyWarningPocket) {
            screen.postStatus("Gen IV pocket changed; add cancelled",280);
            return true;
        }
        BagAdd::PocketState pocketState{};
        if(pocket<screen.trainer.items.size())
            for(const auto& stack:screen.trainer.items[pocket])
                if(stack.itemId==state.itemKeyWarningId)pocketState.heldInPocket=stack.count;
        const auto decision=BagAdd::decidePick(gen4PolicyRule(screen,pocket,state.itemKeyWarningId),
                                               pocketState);
        if(decision.outcome!=BagAdd::PickOutcome::StageNew) {
            screen.postStatus(decision.message,280);
            return true;
        }
        (void)stageGen4BagAdd(screen,state,pocket,state.itemKeyWarningId,decision.quantity);
        return true;
    }
    if(state.itemRemoveConfirmActive) {
        // Deliberate Y->Y two-press removal with a separate confirmation frame.
        // A never removes; B always cancels without touching a staged byte.
        if(down&HidNpadButton_B) {
            state.itemRemoveConfirmActive=false;
            screen.controllerNavigation.reset();
            return true;
        }
        if(!(down&HidNpadButton_Y))return true;
        const int pocket=state.itemRemovePocket;
        const int index=state.itemRemoveVisibleRow;
        if(!itemsScreen || screen.selectedCategory!=pocket ||
           screen.selectedItemIndex!=index ||
           pocket<0 || pocket>=static_cast<int>(screen.trainer.items.size())) {
            state.itemRemoveConfirmActive=false;
            screen.postStatus("Gen IV item selection changed; remove cancelled",300);
            return true;
        }
        const auto visible=screen.visibleItemIndices();
        if(index<0 || index>=static_cast<int>(visible.size())) {
            state.itemRemoveConfirmActive=false;
            screen.postStatus("Gen IV item no longer exists; remove cancelled",300);
            return true;
        }
        const auto& item=screen.trainer.items[static_cast<size_t>(pocket)]
                                     [static_cast<size_t>(visible[static_cast<size_t>(index)])];
        if(item.itemId!=state.itemRemoveId || item.count!=state.itemRemoveCount) {
            state.itemRemoveConfirmActive=false;
            screen.postStatus("Gen IV item identity/count changed; remove cancelled",300);
            return true;
        }
        auto* staged=editor(screen);
        if(!staged) {
            state.itemRemoveConfirmActive=false;
            screen.postStatus("Gen IV staged item removal is unavailable",300);
            return true;
        }
        auto backup=*staged;
        std::string error;
        if(!staged->stageBagRemove(static_cast<size_t>(pocket),
                                   static_cast<size_t>(index),&error)) {
            screen.postStatus(error,300);
            return true;
        }
        if(!bridge(screen).refreshStagedPokemonPresentation(error)) {
            *staged=std::move(backup);
            std::string restored;
            (void)bridge(screen).refreshStagedPokemonPresentation(restored);
            screen.postStatus("Gen IV item remove refresh failed; staged item retained: "+error,360);
            return true;
        }
        state.itemRemoveConfirmActive=false;
        const auto remaining=screen.visibleItemIndices().size();
        screen.selectedItemIndex=remaining?
            std::min(screen.selectedItemIndex,static_cast<int>(remaining)-1):0;
        screen.controllerNavigation.reset();
        screen.postStatus("Gen IV item removed from staged workspace ONLY",300);
        return true;
    }
    {
        // Forward controller movement and B/Exit to the shared Items panel.
        // Only A on an occupied, validated native stack opens an app-memory
        // draft. A must NEVER perform a destructive removal or source write.
        if(!itemsScreen || !(down&(HidNpadButton_A|HidNpadButton_Y|HidNpadButton_X)) ||
           screen.helpOverlayActive || screen.itemEditDialogActive ||
           screen.saveConfirmActive || screen.pickerActive ||
           screen.actionSheet.isOpen())return false;
        const int pocket=screen.selectedCategory;
        if(down&HidNpadButton_X) {
            auto* staged=editor(screen);
            if(!staged || pocket<0 || pocket>=static_cast<int>(Gen4::BagPocketCount) ||
               pocket>=static_cast<int>(screen.trainer.items.size()) ||
               !Gen4::decodeReadOnlyBag(bridge(screen).sourceSave())) {
                screen.postStatus("Gen IV Add requires a validated native source bag",300);
                return true;
            }
            const auto candidates=Gen4::gen4BagChoices(staged->layout(),
                                                       static_cast<size_t>(pocket));
            // Every native item of the pouch, held or not: choosing a held one focuses its
            // stack (never a duplicate entry), exactly like Gen I-III and Gen V.
            state.itemAddChoices=candidates;
            if(state.itemAddChoices.empty()) {
                screen.postStatus("No supported Gen IV item IDs in this pocket",280);
                return true;
            }
            state.itemAddPickerActive=true;
            state.itemAddPocket=pocket;
            state.itemAddNativeChoices=state.itemAddChoices;
            state.itemAddSort=ItemPickerSort::Mode::Native;
            state.itemAddIndex=0;
            screen.controllerNavigation.reset();
            return true;
        }
        const int visible=screen.selectedItemIndex;
        if(pocket<0 || pocket>=static_cast<int>(screen.trainer.items.size())) {
            screen.postStatus("No validated Gen IV item pocket is selected",220);
            return true;
        }
        const auto indices=screen.visibleItemIndices();
        if(visible<0 || visible>=static_cast<int>(indices.size())) {
            screen.postStatus("Choose an existing Gen IV item; X adds a new staged stack",220);
            return true;
        }
        const int raw=indices[static_cast<size_t>(visible)];
        if(raw<0 || raw>=static_cast<int>(screen.trainer.items[pocket].size())) {
            screen.postStatus("Gen IV selected item is no longer in the visible pocket",220);
            return true;
        }
        const auto& item=screen.trainer.items[pocket][static_cast<size_t>(raw)];
        if(item.itemId==0 || item.count==0 || item.count>999) {
            screen.postStatus("Gen IV item stack is empty or invalid; X adds a staged item",240);
            return true;
        }
        auto* staged=editor(screen);
        if(!staged) {
            screen.postStatus("Gen IV inventory source remains read-only; staged workspace unavailable",240);
            return true;
        }
        if(!Gen4::gen4BagItemAllowed(staged->layout(),static_cast<size_t>(pocket),item.itemId)) {
            screen.postStatus("Gen IV item is not in this title's verified native pocket catalog",300);
            return true;
        }
        if(down&HidNpadButton_Y) {
            state.itemRemoveConfirmActive=true;
            state.itemRemovePocket=pocket;
            state.itemRemoveVisibleRow=visible;
            state.itemRemoveId=item.itemId;
            state.itemRemoveCount=item.count;
            screen.controllerNavigation.reset();
            return true;
        }
        const auto rule=gen4QuantityRule(screen,static_cast<size_t>(pocket),item.itemId);
        const auto row=PokeBank::UIModel::BagAdd::decideRow(
            gen4PolicyRule(screen,static_cast<size_t>(pocket),item.itemId),item.count,
            Gen4::gen4BagMaxQuantity(static_cast<size_t>(pocket),item.itemId));
        const uint16_t limit=row.maximum;
        if(!rule.valid || row.outcome==PokeBank::UIModel::BagAdd::RowOutcome::NotAllowed) {
            screen.postStatus("Gen IV item quantity outside native limits",300);
            return true;
        }
        if(row.outcome==PokeBank::UIModel::BagAdd::RowOutcome::FixedQuantity) {
            // Key Items and HMs are unique: no quantity editor, nothing staged.
            screen.postStatus("This item is unique; its quantity is fixed at 1",260);
            return true;
        }
        const uint16_t originalId=item.itemId;
        const uint16_t originalCount=item.count;
        const auto r=Utils::promptNumber(
            "Edit "+Names::machineDisplayLabel(
                screen.trainer.getGameGroup(),originalId,Names::getItemName(originalId)),
            static_cast<int>(originalCount),1,static_cast<int>(limit));
        screen.controllerNavigation.reset();
        if(!r.accepted)return true; // Cancel cannot mutate the stage.
        if(r.value<1 || r.value>static_cast<int>(limit)) {
            screen.postStatus("Gen IV item quantity outside native limits",300);
            return true;
        }
        if(r.value==originalCount)return true;
        if(!screen.detailViewActive ||
           screen.selectedMode!=TrainerViewScreen::ViewMode::Items ||
           screen.selectedCategory!=pocket || screen.selectedItemIndex!=visible) {
            screen.postStatus("Gen IV selection changed; quantity edit cancelled",280);
            return true;
        }
        const auto freshIndices=screen.visibleItemIndices();
        if(visible<0 || visible>=static_cast<int>(freshIndices.size())) {
            screen.postStatus("Gen IV selected item no longer available",280);
            return true;
        }
        const int freshRaw=freshIndices[static_cast<size_t>(visible)];
        const auto& currentPocket=screen.trainer.items[static_cast<size_t>(pocket)];
        if(freshRaw<0 || freshRaw>=static_cast<int>(currentPocket.size()) ||
           currentPocket[static_cast<size_t>(freshRaw)].itemId!=originalId ||
           currentPocket[static_cast<size_t>(freshRaw)].count!=originalCount) {
            screen.postStatus("Gen IV item identity changed; edit cancelled",280);
            return true;
        }
        auto* current=editor(screen);
        if(!current) {
            screen.postStatus("Gen IV staged workspace unavailable",280);
            return true;
        }
        auto backup=*current;
        std::string error;
        if(!current->stageBagQuantity(
            static_cast<size_t>(pocket),static_cast<size_t>(visible),
            static_cast<uint16_t>(r.value),&error)) {
            screen.postStatus(error,300);
            return true;
        }
        if(!bridge(screen).refreshStagedPokemonPresentation(error)) {
            *current=std::move(backup);
            std::string restored;
            (void)bridge(screen).refreshStagedPokemonPresentation(restored);
            screen.postStatus("Gen IV item refresh failed; previous stage retained: "+error,360);
            return true;
        }
        screen.postStatus("Gen IV item quantity staged; emulator source unchanged",300);
        return true;
    }
    // D-pad/stick movement and B remain owned by the underlying Items panel.
    return false;
}

void drawItemAddPicker(TrainerViewScreen& screen,PKSEFramebuffer& fb,const State& state) {
    screen.drawGSCOverlay(fb);
    const auto game=screen.trainer.getGameGroup();
    const auto label=[&](int index) {
        const auto id=state.itemAddChoices[static_cast<size_t>(index)];
        return Names::machineDisplayLabel(game,id,Names::getItemName(id));
    };
    const int count=static_cast<int>(state.itemAddChoices.size());
    UI::SharedBagAddPicker::drawPicker(fb,screen.touchButtons,kGen4BagAddTouchBase,game,
        "Add Item",
        std::string(Gen4::bagPocketName(static_cast<size_t>(state.itemAddPocket)))+
            "  |  Sort: "+ItemPickerSort::label(state.itemAddSort),
        count,state.itemAddIndex,label,
        [&](int index){return state.itemAddChoices[static_cast<size_t>(index)];},
        [&](int index) {
            const auto id=state.itemAddChoices[static_cast<size_t>(index)];
            const size_t pocket=static_cast<size_t>(state.itemAddPocket);
            if(pocket>=screen.trainer.items.size())return uint16_t{0};
            for(const auto& stack:screen.trainer.items[pocket])if(stack.itemId==id)return stack.count;
            return uint16_t{0};
        });
    // Shared draggable scrollbar (publishes into the mutable overlay state).
    const auto track=UI::SharedBagAddPicker::scrollbarTrack(count,SharedScrollbar::kTrackWidth);
    SharedScrollbar::drawAndPublish(fb, stateFor(screen).itemAddScrollbar, track.x, track.y,
        SharedScrollbar::kTrackWidth, track.h,
        SharedScrollbar::rows(count, UI::SharedBagAddPicker::visibleRows(count),
                              UI::SharedBagAddPicker::firstVisible(state.itemAddIndex,count)));
    drawNavBar(fb,{{"D-pad/Stick","Choose"},{"L/R","Page"},
                   {"A","Add"},{"B","Cancel"},{"X","Sort"}});
}

void drawItemKeyWarning(TrainerViewScreen& screen,PKSEFramebuffer& fb,const State& state) {
    screen.drawGSCOverlay(fb);
    const auto game=screen.trainer.getGameGroup();
    const uint16_t id=state.itemKeyWarningId;
    SharedEditorChrome::beginModal(screen.touchButtons);
    UI::SharedBagAddPicker::drawKeyItemWarning(fb,screen.touchButtons,game,id,
        Names::machineDisplayLabel(game,id,Names::getItemName(id)),false);
    drawNavBar(fb,{{"A","Confirm"},{"B","Cancel"}});
}

void drawItemRemoveConfirm(TrainerViewScreen& screen,PKSEFramebuffer& fb,const State& state) {
    screen.drawGSCOverlay(fb);
    SharedEditorChrome::beginModal(screen.touchButtons);
    UI::SharedBagAddPicker::drawRemoveConfirm(fb,screen.touchButtons,screen.trainer.getGameGroup(),
        "Remove Gen IV Item?",state.itemRemoveId,Names::getItemName(state.itemRemoveId),
        state.itemRemoveCount,"Staged copy only; Discard Changes restores it.");
    drawNavBar(fb,{{"Y","Confirm Remove"},{"B","Cancel"}});
}

} // namespace

bool ownsFrame(const TrainerViewScreen& screen) noexcept {
    return ownsFrameImpl(screen);
}

bool handleInput(TrainerViewScreen& screen, u64 down, u64 held,
                 int stickX, int stickY, const TouchInput& touch) {
    if (!isGen4Nds(screen)) return false;

    beginPassiveView(screen);
    auto& state = stateFor(screen);
    if(handleItemQuantity(screen,state,down,held,stickX,stickY,touch))return true;
    const bool ownsNavigation =
        state.actions || state.review || state.legality || state.confirmRelease ||
        state.valuePicker || state.moveEditor || workspaceActive(state);
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
    if (handleReview(screen, state, navigated)) return true;
    if (handleWorkspace(screen, state, navigated, touch)) return true;
    if (handleActions(screen, state, navigated, touch)) return true;

    const bool partyEntry = partyEntrySurface(screen);
    const bool boxEntry = boxEntrySurface(screen);
    const bool storageEntry = storageEntrySurface(screen);
    // First-tap Boxes entry must also work from an old header focus (-1).
    // Resolve tapped native cell, apply the full modal guard, then restore
    // the cursor if the tap was blocked. Add always requires its menu action.
    if (screen.selectedMode == TrainerViewScreen::ViewMode::Boxes &&
        screen.detailViewActive && touch.justTapped()) {
        const int tapped = screen.touchedButtonId(touch);
        const int slots = static_cast<int>(screen.trainer.getSlotsPerBox());
        if (PokeBank::UIModel::Gen4BoxTouchActions::selectsTappedBoxSlot(tapped, slots)) {
            const int previous = screen.selectedItemIndex;
            screen.selectedItemIndex = tapped;
            if (boxEntrySurface(screen)) {
                beginActions(screen);
                return true;
            }
            screen.selectedItemIndex = previous;
        }
        // The summary panel's "A / tap: Actions" target opens the SAME native Gen IV sheet,
        // never the inherited generic action sheet (Make Shiny / Legality placeholders).
        if (tapped == PokeBank::UIModel::Gen4BoxTouchActions::SummaryPanelTouchId &&
            PokeBank::UIModel::Gen4BoxTouchActions::opensSelectedSlotActions(
                tapped, screen.selectedItemIndex, slots) && boxEntrySurface(screen)) {
            beginActions(screen);
            return true;
        }
    }
    // A party tap must reach the Gen IV sheet even when the D-pad cursor sits on an empty
    // party slot (partyEntrySurface is false there): select the tapped Pokemon first, then
    // apply the full modal guard, exactly like the first-tap Boxes entry above.
    if (screen.selectedMode == TrainerViewScreen::ViewMode::Party &&
        screen.detailViewActive && touch.justTapped()) {
        const int tapped = screen.touchedButtonId(touch);
        if (tapped >= 6200 && tapped < 6206 &&
            tapped - 6200 < static_cast<int>(screen.trainer.getPartySize())) {
            const int previous = screen.selectedPartyIndex;
            screen.selectedPartyIndex = tapped - 6200;
            if (partyEntrySurface(screen)) {
                beginActions(screen);
                return true;
            }
            screen.selectedPartyIndex = previous;
        }
    }
    if (!partyEntry && !boxEntry && !storageEntry) return false;


    if (navigated & HidNpadButton_A) {
        if (partyEntry || boxEntry || storageEntry) beginActions(screen);
        return true;
    }
    if ((navigated & HidNpadButton_X) && boxEntry && !selectedOccupied(screen)) {
        beginActions(screen);
        auto& createState = stateFor(screen);
        createState.actionRow = 0; // Add
        beginCreate(screen);
        return true;
    }
    return false;
}

bool draw(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    // Like Gen III: adopt a pending passive View in the draw too, so the inherited generic
    // details modal can never flash for one frame before handleInput runs.
    if (isGen4Nds(screen)) beginPassiveView(screen);
    if (!ownsFrameImpl(screen)) return false;
    auto& state = stateFor(screen);
    if (state.itemAddPickerActive)drawItemAddPicker(screen,fb,state);
    else if (state.itemKeyWarningActive)drawItemKeyWarning(screen,fb,state);
    else if (state.itemRemoveConfirmActive)drawItemRemoveConfirm(screen,fb,state);
    else if (state.confirmRelease) drawReleaseConfirm(screen, fb, state);
    else if (state.legality) drawLegality(screen, fb, state);
    else if (state.review) drawReview(screen, fb, state);
    else if (workspaceActive(state)) drawWorkspace(screen, fb, state);
    else if (state.actions) drawActions(screen, fb, state);
    return true;
}

} // namespace UI::Gen4SharedEditorSurface
