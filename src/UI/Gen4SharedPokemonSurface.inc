#include "Integration/Gen4/Gen4ExactFormatEditorProvider.h"
#include "Legacy/Gen4ReadOnlyTrainer.h"
#include "Names/MoveInfo.h"
#include "Names/MoveNames.h"
#include "Names/MovePresence.h"
#include "Names/ItemNames.h"
#include "Enums/Ball.h"
#include "Enums/LanguageID.h"
#include "Names/SpeciesNames.h"
#include "Pokemon/Pokemon4ReadOnlyView.h"
#include "Trainer/Trainer.h"
#include "UI/ClassicGameContext.h"
#include "UI/Common.h"
#include "UI/Dialogs/DialogFrame.h"
#include "UI/Gen4SharedPokemonSession.h"
#include "UI/ScreenChrome.h"
#include "UI/SharedPokemonEditorContract.h"
#include "UI/SharedPokemonShell.h"
#include "Utils/Keyboard.h"
#include "Utils/StringHelpers.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace UI::Gen4SharedEditorSurface {
namespace {

namespace Shared = PokeBank::UIModel::SharedPokemonEditor;
namespace SessionModel = PokeBank::UIModel::Gen4SharedEditor;
namespace Context = PokeBank::UIModel::ClassicGameContext;
namespace Gen4 = PokeVault::Integration::Gen4;

enum class PickerTarget : uint8_t {
    None, Species, Nature, Ability, HeldItem, Language, Ball, Pokerus, Move
};
enum class TargetKind : uint8_t { Box, Party };

struct State {
    const TrainerViewScreen* owner = nullptr;
    bool actions = false;
    bool review = false;
    int actionRow = 0;
    TargetKind target = TargetKind::Box;
    int box = 0;
    int slot = 0;
    Shared::Focus focus{};
    int detailsFocus = 0;
    SessionModel::Session session{};

    bool valuePicker = false;
    PickerTarget pickerTarget = PickerTarget::None;
    int pickerRow = 0;
    std::vector<uint16_t> pickerValues;

    bool moveEditor = false;
    int moveEditorSlot = 0;
    int moveEditorRow = 0;
    uint16_t moveBaseline = 0;
    uint8_t movePpBaseline = 0;
    uint8_t movePpUpsBaseline = 0;
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
    return state.actions || state.review || state.valuePicker || state.moveEditor ||
           workspaceActive(state);
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
    if (boxEntrySurface(screen))
        return boxRecord(screen, screen.selectedBoxIndex, screen.selectedItemIndex, error).has_value();
    return false;
}

void resetWorkspaceFocus(State& state) noexcept {
    state.focus = {Shared::Panel::Details, 0, 0};
    state.detailsFocus = 0;
    state.valuePicker = false;
    state.pickerTarget = PickerTarget::None;
    state.pickerValues.clear();
    state.moveEditor = false;
    state.moveEditorRow = 0;
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
        state.box = screen.selectedBoxIndex;
        state.slot = screen.selectedItemIndex;
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
    resetWorkspaceFocus(state);
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
    resetWorkspaceFocus(state);
    return true;
}

bool beginCreate(TrainerViewScreen& screen, int box, int slot) {
    auto* staged = editor(screen);
    if (!staged) {
        const auto& why = bridge(screen).stagedPokemonUnavailableReason();
        screen.postStatus(why.empty()
            ? "Generation IV staged Create is unavailable for this source"
            : why, 360);
        return false;
    }
    if (box < 0 || box >= 18 || slot < 0 || slot >= 30) {
        screen.postStatus("Select a native Generation IV box slot first", 300);
        return false;
    }
    std::string error;
    if (staged->boxedPokemon(static_cast<size_t>(box), static_cast<size_t>(slot), &error)) {
        screen.postStatus("Add Pokemon requires an empty Generation IV box slot", 300);
        return false;
    }
    if (error.find("empty") == std::string::npos) {
        screen.postStatus(error.empty() ? "Generation IV Create target is unavailable" : error, 360);
        return false;
    }

    auto& state = stateFor(screen);
    state.target = TargetKind::Box;
    state.box = box;
    state.slot = slot;
    state.actions = false;
    state.review = false;
    const auto& trainer = bridge(screen).sourceSave().trainer();
    const uint32_t pidSeed = 0x9E3779B9u ^
        (static_cast<uint32_t>(trainer.tid) << 16) ^ trainer.sid ^
        (static_cast<uint32_t>(box + 1) * 0x45D9F3Bu) ^
        (static_cast<uint32_t>(slot + 1) * 0x119DE1F3u);
    if (!state.session.beginCreate(bridge(screen).sourceSave(), 25, pidSeed, error)) {
        screen.postStatus(error.empty() ? "Could not start Generation IV Create" : error, 360);
        return false;
    }
    resetWorkspaceFocus(state);
    // Create starts by choosing species, matching the mature Gen I-III flow.
    state.valuePicker = true;
    state.pickerTarget = PickerTarget::Species;
    state.pickerValues.reserve(493);
    for (uint16_t species = 1; species <= 493; ++species)
        state.pickerValues.push_back(species);
    state.pickerRow = 24; // Pikachu default.
    screen.controllerNavigation.reset();
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
    bool kept = false;
    if (state.session.mode == SessionModel::Mode::Create) {
        kept = state.session.add(*staged, static_cast<size_t>(state.box),
                                 static_cast<size_t>(state.slot), error);
    } else if (state.target == TargetKind::Party) {
        kept = state.session.keepParty(*staged, static_cast<size_t>(state.slot), error);
    } else {
        kept = state.session.keep(*staged, static_cast<size_t>(state.box),
                                  static_cast<size_t>(state.slot), error);
    }
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

const char* genderText(uint8_t gender) noexcept {
    return gender == 0 ? "Male" : gender == 1 ? "Female" : "Genderless";
}

Color genderValueColor(uint8_t gender) {
    if (gender == 0)
        return g_themeMode == ThemeMode::Light ? Color(24,112,184) : Color(92,205,255);
    if (gender == 1) return Colors::Magenta;
    return Colors::Text;
}

const char* gameName(std::string_view id) noexcept {
    if (id == "diamond_nds") return "Diamond";
    if (id == "pearl_nds") return "Pearl";
    if (id == "platinum_nds") return "Platinum";
    if (id == "heartgold_nds") return "HeartGold";
    if (id == "soulsilver_nds") return "SoulSilver";
    return "Generation IV";
}

constexpr std::array<const char*, 15> detailLabels{{
    "Species", "Nickname", "Level", "Experience", "OT", "Trainer ID", "SID",
    "Held Item", "Friendship", "Language", "Origin", "Ball",
    "Met Level", "Met Location", "Pokerus"
}};

bool detailEditable(int row) noexcept {
    return (row >= 0 && row <= 9) || row == 11 || row == 12 || row == 14;
}

bool variableGenderSpecies(const Pokemon::Pokemon4ReadOnly& p) noexcept {
    const uint8_t ratio = p.personal().genderRatio;
    return ratio > 0 && ratio < 254;
}

bool dualAbilitySpecies(const Pokemon::Pokemon4ReadOnly& p) noexcept {
    return p.personal().ability2 != 0 && p.personal().ability2 != p.personal().ability1;
}

bool pidLinkedRowEditable(const State& state, int row,
                          const Pokemon::Pokemon4ReadOnly& p) noexcept {
    if (!state.session.editable()) return false;
    if (row == 6 || row == 8) return true; // Nature / Shiny
    if (row == 7) return variableGenderSpecies(p);
    if (row == 9) return dualAbilitySpecies(p);
    return false;
}

void normalizeEditableFocus(State& state, const Pokemon::Pokemon4ReadOnly& p,
                            int verticalDirection = 0, bool panelChanged = false) {
    state.focus = Shared::normalizeMoveRowFocus(Shared::Generation::Gen4, state.focus);
    if (!state.session.editable()) return;

    if (state.focus.panel == Shared::Panel::Details) {
        // G4-04 keeps read-only/native-inspection rows focusable instead of silently skipping them.
        state.focus.row = static_cast<uint8_t>(state.focus.row % detailLabels.size());
        state.focus.column = 0;
        return;
    }

    if (state.focus.panel == Shared::Panel::Values) {
        if (state.focus.row < 6) {
            if (state.focus.column > 1) state.focus.column = 1;
            return;
        }
        state.focus.column = 0;
        if (pidLinkedRowEditable(state, state.focus.row, p)) return;

        int row = state.focus.row;
        const int direction = verticalDirection < 0 ? -1 : 1;
        for (int step = 0; step < 5; ++step) {
            row += direction;
            if (row < 6) row = 10;
            if (row > 10) row = 6;
            if (pidLinkedRowEditable(state, row, p)) {
                state.focus.row = static_cast<uint8_t>(row);
                return;
            }
        }
        if (panelChanged) state.focus = {Shared::Panel::Values, 0, 0};
        return;
    }

    state.focus.row = static_cast<uint8_t>(state.focus.row % 4);
    state.focus.column = 0;
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
        case 7: return p.heldItem() == 0 ? "(None)" : Names::getItemName(p.heldItem());
        case 8: return std::to_string(p.friendship());
        case 9: return Enums::getLanguageName(p.language());
        case 10: return Enums::getOriginGameName(p.originVersion());
        case 11: return Enums::getBallName(view.ball());
        case 12: return std::to_string(p.metLevel());
        case 13: return "ID " + std::to_string(view.metLocation()) + " (inspect only)";
        case 14: {
            const uint8_t pkrs = p.pokerusState();
            if (pkrs == 0) return "None";
            if ((pkrs & 0x0Fu) != 0) return "Infected";
            return "Cured";
        }
        default: return {};
    }
}

void drawPanel(PKSEFramebuffer& fb, int x, int y, int w, int h, const char* title) {
    drawPanelSurface(fb, x, y, w, h, false, 14);
    fb.drawText(x + 14, y + 10, title, Colors::Accent, TextStyle::Caption);
}

void drawFocus(PKSEFramebuffer& fb, int x, int y, int w, int h, bool selected) {
    if (selected)
        fb.drawRoundedRect(x, y, w, h, 6, Colors::FocusBorder, 2);
}

int moveMaxPP(uint16_t move, uint8_t ppUps, Enums::GameVersion group) {
    if (move == 0) return 0;
    const int base = Names::getMoveBasePP(move, group);
    return base <= 0 ? 0 : base * (5 + std::min<int>(ppUps, 3)) / 5;
}

void openSpeciesPicker(TrainerViewScreen& screen, State& state) {
    state.valuePicker = true;
    state.pickerTarget = PickerTarget::Species;
    state.pickerValues.clear();
    state.pickerValues.reserve(493);
    for (uint16_t species = 1; species <= 493; ++species)
        state.pickerValues.push_back(species);
    const uint16_t current = state.session.working ? state.session.working->species() : 1;
    state.pickerRow = std::clamp<int>(static_cast<int>(current) - 1, 0, 492);
    screen.controllerNavigation.reset();
}

void openNaturePicker(TrainerViewScreen& screen, State& state) {
    state.valuePicker = true;
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
    state.pickerTarget = PickerTarget::Ability;
    state.pickerValues = {0, 1};
    state.pickerRow = state.session.working ? state.session.working->abilitySlot() : 0;
    screen.controllerNavigation.reset();
}

bool handleValuePicker(TrainerViewScreen& screen, State& state, u64 down) {
    if (!state.valuePicker) return false;
    if (state.pickerValues.empty()) {
        state.valuePicker = false;
        return true;
    }
    if (down & HidNpadButton_Up)
        state.pickerRow = (state.pickerRow - 1 + static_cast<int>(state.pickerValues.size())) %
                          static_cast<int>(state.pickerValues.size());
    if (down & HidNpadButton_Down)
        state.pickerRow = (state.pickerRow + 1) %
                          static_cast<int>(state.pickerValues.size());
    if (down & HidNpadButton_B) {
        state.valuePicker = false;
        state.pickerTarget = PickerTarget::None;
        screen.controllerNavigation.reset();
        return true;
    }
    if (!(down & HidNpadButton_A) || !state.session.working) return true;

    const uint16_t value = state.pickerValues[static_cast<size_t>(state.pickerRow)];
    bool ok = false;
    if (state.pickerTarget == PickerTarget::Species)
        ok = state.session.working->setSpecies(value);
    else if (state.pickerTarget == PickerTarget::Nature)
        ok = state.session.working->setNature(static_cast<uint8_t>(value));
    else if (state.pickerTarget == PickerTarget::Ability)
        ok = state.session.working->setAbilitySlot(static_cast<uint8_t>(value));

    if (!ok) {
        screen.postStatus(state.pickerTarget == PickerTarget::Species
            ? "That species could not be represented safely in this Generation IV draft"
            : "Could not preserve the other PID-linked Generation IV traits", 300);
        return true;
    }
    state.valuePicker = false;
    state.pickerTarget = PickerTarget::None;
    screen.controllerNavigation.reset();
    return true;
}

void openMoveEditor(TrainerViewScreen& screen, State& state, int slot) {
    if (!state.session.working || slot < 0 || slot >= 4) return;
    state.moveEditor = true;
    state.moveEditorSlot = slot;
    state.moveEditorRow = 1; // Move itself stays read-only in G4-03; start on PP.
    state.movePpBaseline = state.session.working->pp()[static_cast<size_t>(slot)];
    state.movePpUpsBaseline = state.session.working->ppUps()[static_cast<size_t>(slot)];
    screen.controllerNavigation.reset();
}

void cancelMoveEditor(State& state) {
    if (state.session.working && state.moveEditorSlot >= 0 && state.moveEditorSlot < 4) {
        const auto slot = static_cast<size_t>(state.moveEditorSlot);
        state.session.working->setPP(slot, state.movePpBaseline);
        state.session.working->setPPUps(slot, state.movePpUpsBaseline);
    }
    state.moveEditor = false;
    state.moveEditorRow = 0;
}

bool handleMoveEditor(TrainerViewScreen& screen, State& state, u64 down) {
    if (!state.moveEditor || !state.session.working) return false;
    if (down & HidNpadButton_Up) state.moveEditorRow = (state.moveEditorRow + 4) % 5;
    if (down & HidNpadButton_Down) state.moveEditorRow = (state.moveEditorRow + 1) % 5;
    if (down & HidNpadButton_B) {
        cancelMoveEditor(state);
        screen.controllerNavigation.reset();
        return true;
    }
    if (!(down & HidNpadButton_A)) return true;

    const auto slot = static_cast<size_t>(state.moveEditorSlot);
    const auto move = state.session.working->moves()[slot];
    if (state.moveEditorRow == 0) {
        screen.postStatus("Move selection stays read-only until exact Gen IV learnsets are pinned", 300);
    } else if (state.moveEditorRow == 1) {
        if (move == 0) {
            screen.postStatus("Choose a move in a later milestone before editing PP", 240);
        } else {
            const int maximum = moveMaxPP(
                move, state.session.working->ppUps()[slot],
                bridge(screen).sourceSave().rawFamily());
            const auto r = Utils::promptNumber("Current PP",
                std::min<int>(state.session.working->pp()[slot], maximum), 0, maximum);
            if (r.accepted) state.session.working->setPP(slot, static_cast<uint8_t>(r.value));
        }
    } else if (state.moveEditorRow == 2) {
        if (move == 0) {
            screen.postStatus("Empty move slots keep PP Ups at zero", 240);
        } else {
            const auto r = Utils::promptNumber(
                "PP Ups", state.session.working->ppUps()[slot], 0, 3);
            if (r.accepted) {
                state.session.working->setPPUps(slot, static_cast<uint8_t>(r.value));
                const int maximum = moveMaxPP(
                    move, static_cast<uint8_t>(r.value),
                    bridge(screen).sourceSave().rawFamily());
                if (state.session.working->pp()[slot] > maximum)
                    state.session.working->setPP(slot, static_cast<uint8_t>(maximum));
            }
        }
    } else if (state.moveEditorRow == 3) {
        state.moveEditor = false;
        state.moveEditorRow = 0;
        screen.controllerNavigation.reset();
    } else {
        cancelMoveEditor(state);
        screen.controllerNavigation.reset();
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
    if (row == 8) {
        const auto r = Utils::promptNumber("Friendship", mon.friendship(), 0, 255);
        if (r.accepted) mon.setFriendship(static_cast<uint8_t>(r.value));
        return true;
    }
    if (row == 12) {
        const auto r = Utils::promptNumber("Met Level", mon.metLevel(), 0, 100);
        if (r.accepted) mon.setMetLevel(static_cast<uint8_t>(r.value));
        return true;
    }
    return false;
}

bool editStatCell(TrainerViewScreen& screen, State& state,
                  const Pokemon::Pokemon4ReadOnly& p) {
    if (!state.session.working) return false;
    const int row = state.focus.row;
    if (row < 6) {
        const auto stat = static_cast<size_t>(row);
        if (state.focus.column == 0) {
            const auto r = Utils::promptNumber(
                "IV", state.session.working->ivs()[stat], 0, 31);
            if (r.accepted) state.session.working->setIV(stat, static_cast<uint8_t>(r.value));
        } else {
            const auto r = Utils::promptNumber(
                "EV", state.session.working->evs()[stat], 0, 255);
            if (r.accepted) state.session.working->setEV(stat, static_cast<uint8_t>(r.value));
        }
        return true;
    }
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

bool handleWorkspace(TrainerViewScreen& screen, State& state, u64 down) {
    if (!workspaceActive(state)) return false;
    if (handleValuePicker(screen, state, down)) return true;
    if (handleMoveEditor(screen, state, down)) return true;
    if (!state.session.working) {
        state.session.close();
        return true;
    }

    const auto record = workingRecord(state, bridge(screen).sourceSave().rawFamily());
    if (!record) {
        screen.postStatus("Generation IV editor draft failed strict PK4 preview", 360);
        state.session.close();
        return true;
    }

    if (state.session.confirmExit) {
        if (down & HidNpadButton_A) {
            if (keepWorkspace(screen)) state.session.confirmExit = false;
            return true;
        }
        if (down & HidNpadButton_Y) {
            state.session.discardDraft();
            screen.postStatus("Generation IV Edit session discarded; staged/source bytes unchanged", 300);
            return true;
        }
        if (down & HidNpadButton_B) {
            state.session.continueEditing();
            return true;
        }
        return true;
    }

    const bool passive = state.session.mode == SessionModel::Mode::View;
    const auto panelBefore = state.focus.panel;
    int verticalDirection = 0;
    if (down & HidNpadButton_L)
        state.focus = passive ? Shared::passiveViewSwitchPanel(Shared::Generation::Gen4, state.focus, -1)
                              : Shared::switchPanel(Shared::Generation::Gen4, state.focus, -1);
    if (down & HidNpadButton_R)
        state.focus = passive ? Shared::passiveViewSwitchPanel(Shared::Generation::Gen4, state.focus, 1)
                              : Shared::switchPanel(Shared::Generation::Gen4, state.focus, 1);
    if (down & HidNpadButton_Up) {
        state.focus = passive ? Shared::passiveViewMoveVertical(Shared::Generation::Gen4, state.focus, -1)
                              : Shared::moveVertical(Shared::Generation::Gen4, state.focus, -1);
        verticalDirection = -1;
    }
    if (down & HidNpadButton_Down) {
        state.focus = passive ? Shared::passiveViewMoveVertical(Shared::Generation::Gen4, state.focus, 1)
                              : Shared::moveVertical(Shared::Generation::Gen4, state.focus, 1);
        verticalDirection = 1;
    }
    if (down & HidNpadButton_Left)
        state.focus = passive ? Shared::passiveViewMoveColumn(Shared::Generation::Gen4, state.focus, -1)
                              : Shared::moveRowColumn(Shared::Generation::Gen4, state.focus, -1);
    if (down & HidNpadButton_Right)
        state.focus = passive ? Shared::passiveViewMoveColumn(Shared::Generation::Gen4, state.focus, 1)
                              : Shared::moveRowColumn(Shared::Generation::Gen4, state.focus, 1);

    if (passive)
        state.focus = Shared::normalizePassiveViewFocus(Shared::Generation::Gen4, state.focus);
    else
        normalizeEditableFocus(state, *record, verticalDirection,
                               state.focus.panel != panelBefore);
    state.detailsFocus = Shared::detailsScrollFocus(state.focus, state.detailsFocus);
    state.detailsFocus = std::clamp(state.detailsFocus, 0, 14);

    if (down & HidNpadButton_B) {
        state.session.back();
        return true;
    }
    if ((down & HidNpadButton_X) && state.session.editable()) {
        keepWorkspace(screen);
        return true;
    }
    if (!(down & HidNpadButton_A) || !state.session.editable()) return true;

    if (state.focus.panel == Shared::Panel::Details)
        editDetail(screen, state, state.focus.row, *record);
    else if (state.focus.panel == Shared::Panel::Values)
        editStatCell(screen, state, *record);
    else
        openMoveEditor(screen, state, state.focus.row);
    return true;
}

Shared::ActionSet gen4Actions(bool occupied) {
    Shared::ActionSet result{};
    if (!occupied) {
        result.values[result.count++] = Shared::Action::Add;
        result.values[result.count++] = Shared::Action::Review;
        result.values[result.count++] = Shared::Action::Close;
        return result;
    }
    result.values[result.count++] = Shared::Action::View;
    result.values[result.count++] = Shared::Action::Edit;
    result.values[result.count++] = Shared::Action::Review;
    result.values[result.count++] = Shared::Action::Close;
    return result;
}

bool handleActions(TrainerViewScreen& screen, State& state, u64 down) {
    if (!state.actions) return false;
    std::string error;
    const bool occupied = selectedRecord(screen, state, error).has_value();
    const auto actions = gen4Actions(occupied);
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

    switch (actions[static_cast<size_t>(state.actionRow)]) {
        case Shared::Action::View:
            beginViewOrEdit(screen, SessionModel::Mode::View);
            break;
        case Shared::Action::Edit:
            beginViewOrEdit(screen, SessionModel::Mode::Edit);
            break;
        case Shared::Action::Add:
            beginCreate(screen, state.box, state.slot);
            break;
        case Shared::Action::Review:
            state.actions = false;
            state.review = true;
            break;
        case Shared::Action::Close:
            state.actions = false;
            break;
        default:
            break;
    }
    return true;
}

bool handleReview(TrainerViewScreen& screen, State& state, u64 down) {
    if (!state.review) return false;
    if (down & HidNpadButton_B) {
        state.review = false;
        state.actions = true;
        screen.controllerNavigation.reset();
        return true;
    }
    if (down & HidNpadButton_Y) {
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
    return true;
}

void drawValuePicker(TrainerViewScreen& screen, PKSEFramebuffer& fb, State& state) {
    constexpr int w = 650, h = 520;
    const int x = (fb.getWidth() - w) / 2;
    const int y = (fb.getHeight() - kNavBarH - h) / 2;
    fb.drawFilledRect(0, 0, fb.getWidth(), fb.getHeight() - kNavBarH, Color(0,0,0,130));
    drawPanelSurface(fb, x, y, w, h, false, 18);
    fb.drawText(x + 24, y + 18,
        state.pickerTarget == PickerTarget::Species ? "Choose Species" :
        state.pickerTarget == PickerTarget::Nature ? "Choose Nature" : "Choose Ability",
        Colors::Text, TextStyle::Heading);
    const int first = std::max(0, state.pickerRow - 5);
    const int count = std::min<int>(10, static_cast<int>(state.pickerValues.size()) - first);
    for (int i = 0; i < count; ++i) {
        const int index = first + i;
        const int yy = y + 72 + i * 40;
        const bool selected = index == state.pickerRow;
        if (selected) fb.drawSelectionHighlight(x + 22, yy, w - 44, 34);
        const auto value = state.pickerValues[static_cast<size_t>(index)];
        std::string label;
        if (state.pickerTarget == PickerTarget::Species)
            label = std::to_string(value) + " - " + Names::getSpeciesName(value);
        else if (state.pickerTarget == PickerTarget::Nature)
            label = Trainer::getNatureName(static_cast<uint8_t>(value));
        else if (state.session.working) {
            const uint16_t abilityId =
                state.session.working->abilityForSlot(static_cast<uint8_t>(value));
            label = abilityId == 0
                ? "Ability Slot " + std::to_string(value + 1)
                : std::string(Trainer::getAbilityName(abilityId)) +
                    " (Slot " + std::to_string(value + 1) + ")";
        }
        fb.drawText(x + 38, yy + 7, label,
                    selected ? Colors::Text : Colors::TextDim, TextStyle::Body);
    }
    drawNavBar(fb, {{"D-pad/Stick", "Browse"}, {"A", "Choose"}, {"B", "Cancel"}});
}

void drawMoveEditor(TrainerViewScreen& screen, PKSEFramebuffer& fb, State& state) {
    if (!state.moveEditor || !state.session.working) return;
    const auto slot = static_cast<size_t>(state.moveEditorSlot);
    const uint16_t move = state.session.working->moves()[slot];
    constexpr int w = 680, h = 390;
    const int x = (fb.getWidth() - w) / 2;
    const int y = (fb.getHeight() - kNavBarH - h) / 2;
    fb.drawFilledRect(0, 0, fb.getWidth(), fb.getHeight() - kNavBarH, Color(0,0,0,100));
    drawPanelSurface(fb, x, y, w, h, false, 18);
    fb.drawText(x + 24, y + 18, "Move " + std::to_string(state.moveEditorSlot + 1),
                Colors::Text, TextStyle::Heading);
    fb.drawText(x + 24, y + 52,
                "Same shared contextual editor • move choice read-only in G4-03",
                Colors::TextDim, TextStyle::Caption);
    const std::array<std::pair<std::string,std::string>,5> rows{{
        {"Move", move == 0 ? "Empty" : std::string(Names::getMoveName(move)) + " (read-only)"},
        {"PP", std::to_string(state.session.working->pp()[slot]) + "/" +
               std::to_string(moveMaxPP(
                   move, state.session.working->ppUps()[slot],
                   bridge(screen).sourceSave().rawFamily()))},
        {"PP Ups", std::to_string(state.session.working->ppUps()[slot])},
        {"Apply", "Keep these move details"},
        {"Cancel", "Discard these move details"},
    }};
    for (int i = 0; i < 5; ++i) {
        const int yy = y + 88 + i * 48;
        const bool selected = i == state.moveEditorRow;
        if (selected) fb.drawSelectionHighlight(x + 22, yy, w - 44, 42);
        fb.drawText(x + 38, yy + 10, rows[static_cast<size_t>(i)].first,
                    selected ? Colors::Text : Colors::TextDim);
        int vw=0,vh=0;
        fb.measureText(rows[static_cast<size_t>(i)].second, vw, vh);
        fb.drawText(x + w - 38 - vw, yy + 10, rows[static_cast<size_t>(i)].second,
                    selected ? Colors::Text : Colors::TextDim);
    }
    drawNavBar(fb, {{"D-pad/Stick", "Navigate"}, {"A", "Edit / Apply"}, {"B", "Cancel"}});
}

void drawWorkspace(TrainerViewScreen& screen, PKSEFramebuffer& fb, State& state) {
    if (!state.session.working) return;
    const auto raw = workingRecord(state, bridge(screen).sourceSave().rawFamily());
    if (!raw) return;
    Pokemon::Pokemon4ReadOnlyView view(*raw);

    auto mode = Context::SurfaceMode::ViewReadOnly;
    if (state.session.mode == SessionModel::Mode::Edit) mode = Context::SurfaceMode::EditStaged;
    else if (state.session.mode == SessionModel::Mode::Create) mode = Context::SurfaceMode::CreateDraft;
    SharedPokemonShell::drawChrome(
        fb, raw->species(), Utils::utf16ToUtf8(raw->nickname()), view.level(),
        raw->gender(), view.isShiny(raw->id32(), {}), mode,
        state.target == TargetKind::Party
            ? "Party / Slot " + std::to_string(state.slot + 1)
            : "Box " + std::to_string(state.box + 1) + " / Slot " + std::to_string(state.slot + 1),
        screen.sourceGameId);

    const SharedPokemonShell::Geometry g(fb.getWidth(), fb.getHeight(), kNavBarH);
    drawPanel(fb, g.leftX, g.y, g.leftW, g.h, "DETAILS");
    SharedPokemonShell::drawPortrait(
        fb, g.leftX, g.y, raw->species(), view.isShiny(raw->id32(), {}));
    const std::array<uint8_t,2> types{{raw->personal().type1, raw->personal().type2}};
    int typeX = g.leftX + 66;
    for (size_t i = 0; i < types.size(); ++i) {
        if (types[i] == 255 || (i && types[i] == types[0])) continue;
        if (auto* sprite = SpriteManager::getTypeSprite(types[i]);
            sprite && sprite->data && sprite->height > 0) {
            const int width = sprite->width * 22 / sprite->height;
            fb.drawImageScaled(typeX, g.y + 180, sprite->width, sprite->height,
                               width, 22, sprite->data, sprite->channels);
            typeX += width + 8;
        }
    }
    SharedPokemonShell::drawScrollableDetails(
        fb, g.leftX, g.y, g.leftW, g.h, detailLabels.size(), state.detailsFocus,
        state.focus.panel == Shared::Panel::Details,
        [](size_t row) { return detailLabels[row]; },
        [&](size_t row) { return detailValue(*raw, view, static_cast<int>(row)); });

    drawPanel(fb, g.midX, g.y, g.midW, g.h, "STATS");
    fb.drawText(g.midX + 126, g.y + 50, "IV", Colors::TextDim, TextStyle::Caption);
    fb.drawText(g.midX + 190, g.y + 50, "EV", Colors::TextDim, TextStyle::Caption);
    fb.drawText(g.midX + 274, g.y + 50, "Stat", Colors::TextDim, TextStyle::Caption);
    const auto iv = raw->ivs();
    const auto ev = raw->evs();
    const std::array<uint16_t,6> stats{{view.statHPMax(),view.statATK(),view.statDEF(),
                                        view.statSPE(),view.statSPA(),view.statSPD()}};
    constexpr std::array<const char*,6> statLabels{{"HP","Attack","Defense","Speed","Sp. Atk","Sp. Def"}};
    for (int row = 0; row < 6; ++row) {
        const int yy = g.y + 78 + row * 40;
        const bool selected = state.focus.panel == Shared::Panel::Values && state.focus.row == row;
        if (selected) {
            const auto cell = Shared::cellFocusFor(Shared::Generation::Gen4, state.focus);
            drawFocus(fb, g.midX + cell.x, yy - 5, cell.width, 30, true);
        }
        fb.drawText(g.midX + 18, yy, statLabels[static_cast<size_t>(row)],
                    selected ? Colors::SelectedText : Colors::TextDim, TextStyle::Caption);
        fb.drawText(g.midX + 126, yy, std::to_string(iv[static_cast<size_t>(row)]), Colors::Text, TextStyle::Caption);
        fb.drawText(g.midX + 190, yy, std::to_string(ev[static_cast<size_t>(row)]), Colors::Text, TextStyle::Caption);
        fb.drawText(g.midX + 274, yy, std::to_string(stats[static_cast<size_t>(row)]), Colors::TextSecondary, TextStyle::Caption);
    }
    fb.drawHDivider(g.midX + 14, g.y + 310, g.midW - 28);
    fb.drawText(g.midX + 18, g.y + 318, "PID-LINKED / SAFE EDIT", Colors::TextDim, TextStyle::Caption);
    const std::array<std::pair<const char*,std::string>,5> derived{{
        {"Nature", Trainer::getNatureName(view.nature())},
        {"Gender", genderText(raw->gender())},
        {"Shiny", view.isShiny(raw->id32(), {}) ? "Yes" : "No"},
        {"Ability", Trainer::getAbilityName(raw->ability())},
        {"PID (read-only)", std::to_string(raw->pid())},
    }};
    for (int i = 0; i < 5; ++i) {
        const int row = i + 6;
        const int yy = g.y + 346 + i * 35;
        const bool editable = pidLinkedRowEditable(state, row, *raw);
        const bool selected = state.focus.panel == Shared::Panel::Values &&
                              state.focus.row == row &&
                              (state.session.mode == SessionModel::Mode::View || editable);
        drawFocus(fb, g.midX + 8, yy - 5, g.midW - 16, 29, selected);
        fb.drawText(g.midX + 18, yy, derived[static_cast<size_t>(i)].first,
                    selected ? Colors::SelectedText : editable ? Colors::TextDim : Colors::TextSecondary,
                    TextStyle::Caption);
        Color valueColor = selected ? Colors::SelectedText : Colors::Text;
        if (i == 1 && editable && !selected) valueColor = genderValueColor(raw->gender());
        fb.drawText(g.midX + 130, yy, derived[static_cast<size_t>(i)].second,
                    valueColor, TextStyle::Caption);
    }

    drawPanel(fb, g.rightX, g.y, g.rightW, g.h, "MOVES");
    fb.drawText(g.rightX + 186, g.y + 42, "PP", Colors::TextDim, TextStyle::Caption);
    fb.drawText(g.rightX + 242, g.y + 42, "Ups", Colors::TextDim, TextStyle::Caption);
    for (int row = 0; row < 4; ++row) {
        const int yy = g.y + 62 + row * 32;
        const bool selected = state.focus.panel == Shared::Panel::Moves && state.focus.row == row;
        if (selected) {
            const auto cell = Shared::moveRowFocus(g.rightW);
            drawFocus(fb, g.rightX + cell.x, yy - 5, cell.width, 32, true);
        }
        const uint16_t move = raw->moves()[static_cast<size_t>(row)];
        fb.drawText(g.rightX + 18, yy, move == 0 ? "Empty" : Names::getMoveName(move),
                    Colors::Text, TextStyle::Caption);
        fb.drawText(g.rightX + 186, yy, std::to_string(raw->pp()[static_cast<size_t>(row)]),
                    Colors::Text, TextStyle::Caption);
        fb.drawText(g.rightX + 242, yy, std::to_string(raw->ppUps()[static_cast<size_t>(row)]),
                    Colors::Text, TextStyle::Caption);
    }
    fb.drawText(g.rightX + 18, g.y + 205,
                "Move selection: READ ONLY in G4-03", Colors::TextDim, TextStyle::Caption);
    fb.drawText(g.rightX + 18, g.y + 226,
                "A edits PP / PP Ups contextually", Colors::TextDim, TextStyle::Caption);

    struct DataRow { std::string label, value; };
    const std::vector<DataRow> rows{
        {"Game", gameName(screen.sourceGameId)},
        {"Origin", Enums::getOriginGameName(raw->originVersion())},
        {"Language", Enums::getLanguageName(raw->language())},
        {"Ball", Enums::getBallName(view.ball())},
        {"Met", "ID " + std::to_string(view.metLocation()) + " / Lv. " + std::to_string(raw->metLevel())},
        {"Source", "IMMUTABLE"},
        {"Create", "Available in empty box slots"}
    };
    SharedPokemonShell::drawDataAndGraph(
        fb, g.rightX, g.y + 244, g.rightW, g.h - 256, "GEN IV DATA", rows, stats);

    if (state.session.mode == SessionModel::Mode::View)
        drawNavBar(fb, {{"D-pad/Stick", "Navigate"}, {"L/R", "Panel"}, {"B", "Back"}});
    else
        drawNavBar(fb, {{"D-pad/Stick", "Navigate"}, {"L/R", "Panel"}, {"A", "Edit"},
                        {"X", "Keep"}, {"B", "Exit"}});

    if (state.session.confirmExit) {
        constexpr int w = 560, h = 226;
        const int x = (fb.getWidth() - w) / 2;
        const int y = (fb.getHeight() - h) / 2;
        const int cy = Dialogs::drawDialogFrame(fb, x, y, w, h, "Unsaved changes", Colors::Orange);
        fb.drawText(x + 28, cy, "This Pokémon has unsaved changes.", Colors::Text);
        fb.drawText(x + 28, cy + 34, "Discard loses them; Back keeps editing.",
                    Colors::TextDim, TextStyle::Caption);
        const int cbw = 160, cbh = TouchTargetMin, cby = y + h - cbh - 18;
        drawGlyphButton(fb, x + 24, cby, cbw, cbh, "B", "Back", Colors::PanelAlt);
        drawGlyphButton(fb, x + (w - cbw) / 2, cby, cbw, cbh, "Y", "Discard", Colors::PanelAlt);
        drawGlyphButton(fb, x + w - 24 - cbw, cby, cbw, cbh, "A", "Save", Colors::PanelAlt);
    }

    if (state.valuePicker) drawValuePicker(screen, fb, state);
    if (state.moveEditor) drawMoveEditor(screen, fb, state);
}

void drawActions(TrainerViewScreen& screen, PKSEFramebuffer& fb, State& state) {
    std::string error;
    const bool occupied = selectedRecord(screen, state, error).has_value();
    const auto actions = gen4Actions(occupied);

    screen.drawGSCOverlay(fb);
    fb.drawFilledRect(0, 0, fb.getWidth(), fb.getHeight() - kNavBarH, Color(0,0,0,105));
    const int w = 650, h = occupied ? 360 : 300;
    const int x = (fb.getWidth() - w) / 2;
    const int y = (fb.getHeight() - kNavBarH - h) / 2;
    drawPanelSurface(fb, x, y, w, h, false, 18);
    fb.drawText(x + 24, y + 18, occupied ? "Pokemon Actions" : "Empty Slot",
                Colors::Text, TextStyle::Heading);
    fb.drawText(x + 24, y + 52,
                std::string(gameName(screen.sourceGameId)) + " • " +
                (state.target == TargetKind::Party
                    ? "Party / Slot " + std::to_string(state.slot + 1)
                    : "Box " + std::to_string(state.box + 1) + " / Slot " + std::to_string(state.slot + 1)),
                Colors::TextDim, TextStyle::Caption);
    const auto geometry = Shared::actionMenuGeometry();
    int yy = y + geometry.rowStart;
    for (size_t i = 0; i < actions.count; ++i) {
        const bool selected = static_cast<int>(i) == state.actionRow;
        if (selected) fb.drawSelectionHighlight(x + 22, yy, w - 44, geometry.rowHeight);
        fb.drawText(x + 38, yy + 10, Shared::actionLabel(actions[i]),
                    selected ? Colors::Text : Colors::TextDim);
        yy += geometry.rowStep;
    }
    fb.drawText(x + 24, y + h - 34,
                editor(screen) ? "Edits target app-owned staged bytes; source immutable"
                               : "View only — staged editor unavailable for this source",
                Colors::Accent, TextStyle::Caption);
    drawNavBar(fb, {{"D-pad/Stick", "Navigate"}, {"A", "Select"}, {"B", "Cancel"}});
}

void drawReview(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
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
    fb.drawText(44, 308, "Create / Release / Clone remain deferred; Party + Box Edit use staged bytes",
                Colors::TextDim, TextStyle::Caption);
    if (changed)
        drawNavBar(fb, {{"Y", "Discard All Staged"}, {"B", "Back"}});
    else
        drawNavBar(fb, {{"B", "Back"}});
}

} // namespace

bool ownsFrame(const TrainerViewScreen& screen) noexcept {
    return ownsFrameImpl(screen);
}

bool handleInput(TrainerViewScreen& screen, u64 down, u64 held,
                 int stickX, int stickY, const TouchInput& touch) {
    (void)touch;
    if (!isGen4Nds(screen)) return false;

    beginPassiveView(screen);
    auto& state = stateFor(screen);
    const bool ownsNavigation =
        state.actions || state.review || state.valuePicker || state.moveEditor || workspaceActive(state);
    const u64 navigated = ownsNavigation
        ? screen.controllerNavigation.apply(
              down, held, stickX, stickY,
              HidNpadButton_Up, HidNpadButton_Down,
              HidNpadButton_Left, HidNpadButton_Right)
        : down;

    if (handleReview(screen, state, navigated)) return true;
    if (handleWorkspace(screen, state, navigated)) return true;
    if (handleActions(screen, state, navigated)) return true;

    const bool partyEntry = partyEntrySurface(screen);
    const bool boxEntry = boxEntrySurface(screen);
    if (!partyEntry && !boxEntry) return false;
    if (navigated & HidNpadButton_A) {
        if (partyEntry || boxEntry) beginActions(screen);
        return true;
    }
    if ((navigated & HidNpadButton_X) && boxEntry) {
        if (selectedOccupied(screen))
            screen.postStatus("Choose an empty Generation IV box slot to Create", 240);
        else
            beginCreate(screen, screen.selectedBoxIndex, screen.selectedItemIndex);
        return true;
    }
    return false;
}

bool draw(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    if (!ownsFrameImpl(screen)) return false;
    auto& state = stateFor(screen);
    if (state.review) drawReview(screen, fb);
    else if (workspaceActive(state)) drawWorkspace(screen, fb, state);
    else if (state.actions) drawActions(screen, fb, state);
    return true;
}

} // namespace UI::Gen4SharedEditorSurface
