#include "UI/Gen1PokemonEditorOverlay.h"

#include "Integration/Gen1/Gen1StagedInventoryEditor.h"
#include "Integration/Gen1/Gen1StagedPokemonEditor.h"
#include "Legacy/RBYReadOnlyTrainer.h"
#include "Names/MoveNames.h"
#include "Names/SpeciesNames.h"
#include "Pokemon/Experience.h"
#include "UI/ClassicInventoryOverlay.h"
#include "UI/Common.h"
#include "UI/Gen1PokemonEditorUIContract.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/SpriteManager.h"
#include "UI/TrainerViewScreen.h"
#include "Utils/FileUtilities.h"
#include "Utils/Keyboard.h"
#include "Utils/PokeBankPaths.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <sys/stat.h>
#include <unordered_map>
#include <utility>
#include <vector>

namespace UI::Gen1PokemonEditor {
namespace {

using Editor = PokeVault::Integration::Gen1::StagedPokemonEditor;
using BoxPokemonCreate = PokeVault::Integration::Gen1::BoxPokemonCreate;
using BoxPokemonEdit = PokeVault::Integration::Gen1::BoxPokemonEdit;
using PokemonRecord = PokeVault::Integration::Gen1::PokemonRecord;
namespace UX = PokeBank::UIModel::Gen1Editor;

enum class Mode : uint8_t {
    Closed,
    Actions,
    View,
    Edit,
    AddDraft,
    CloneConfirm,
    RemoveConfirm,
    Review,
    Provenance,
};

enum class PickerKind : uint8_t { None, Species, Move };

struct OverlayState {
    const TrainerViewScreen* owner = nullptr;
    std::string sourceGameId;
    Mode mode = Mode::Closed;
    int box = 0;
    int slot = 0;
    int row = 0;
    int cloneBox = 0;
    int reviewRow = 0;
    PickerKind picker = PickerKind::None;
    int pickerValue = 0;
    int pickerMoveSlot = 0;
    bool pickerForDraft = false;
    BoxPokemonCreate draft{};
    std::string lastExportDirectory;
};

OverlayState& stateFor(const TrainerViewScreen& screen) {
    static OverlayState state;
    if (state.owner != &screen || state.sourceGameId != screen.sourceGameId) {
        state = {};
        state.owner = &screen;
        state.sourceGameId = screen.sourceGameId;
    }
    return state;
}

PokeVault::Legacy::RBYReadOnlyTrainer* rbyTrainer(TrainerViewScreen& screen) noexcept {
    if (!isGen1Source(screen)) return nullptr;
    return static_cast<PokeVault::Legacy::RBYReadOnlyTrainer*>(&screen.trainer);
}

const PokeVault::Legacy::RBYReadOnlyTrainer* rbyTrainer(const TrainerViewScreen& screen) noexcept {
    if (!isGen1Source(screen)) return nullptr;
    return static_cast<const PokeVault::Legacy::RBYReadOnlyTrainer*>(&screen.trainer);
}

Editor* editor(TrainerViewScreen& screen) noexcept {
    auto* trainer = rbyTrainer(screen);
    return trainer ? trainer->stagedPokemon() : nullptr;
}

const Editor* editor(const TrainerViewScreen& screen) noexcept {
    auto* trainer = rbyTrainer(screen);
    return trainer ? trainer->stagedPokemon() : nullptr;
}

bool editorAvailable(const TrainerViewScreen& screen) noexcept {
    const auto* trainer = rbyTrainer(screen);
    return trainer && trainer->stagedPokemonAvailable();
}

bool entrySurface(const TrainerViewScreen& screen) noexcept {
    return screen.detailViewActive && screen.selectedMode == TrainerViewScreen::ViewMode::Boxes &&
        screen.selectedItemIndex >= 0 && !screen.helpOverlayActive && !screen.details.active &&
        !screen.actionSheet.isOpen() && !screen.saveConfirmActive && !screen.pickerActive &&
        !screen.itemEditDialogActive;
}

bool refreshBoxes(TrainerViewScreen& screen) {
    auto* trainer = rbyTrainer(screen);
    if (!trainer) return false;
    std::string error;
    if (trainer->refreshBoxesFromStagedPokemon(error)) return true;
    screen.postStatus(error.empty() ? "Could not refresh staged Generation I boxes" : error, 300);
    return false;
}

std::optional<PokemonRecord> pokemonAt(TrainerViewScreen& screen, int box, int slot, std::string& error) {
    auto* e = editor(screen);
    if (!e) {
        auto* trainer = rbyTrainer(screen);
        error = trainer && !trainer->stagedPokemonUnavailableReason().empty()
            ? trainer->stagedPokemonUnavailableReason()
            : "Generation I staged boxed Pokemon editing is unavailable";
        return {};
    }
    return e->boxedPokemon(static_cast<size_t>(box), static_cast<size_t>(slot), error);
}

std::optional<PokemonRecord> selectedPokemon(TrainerViewScreen& screen, std::string& error) {
    const auto& state = stateFor(screen);
    return pokemonAt(screen, state.box, state.slot, error);
}

int firstEmpty(Editor& e, int box) {
    std::string error;
    for (int slot = 0; slot < 20; ++slot) {
        const auto pokemon = e.boxedPokemon(static_cast<size_t>(box), static_cast<size_t>(slot), error);
        if (!error.empty()) return -1;
        if (!pokemon) return slot;
    }
    return -1;
}

std::string speciesCompact(uint16_t species) {
    if (species < 1 || species > 151) return "Invalid";
    return UX::speciesPickerRow(species, Names::getSpeciesName(species));
}

std::string moveDisplay(uint8_t move) {
    if (move == 0) return "Empty";
    if (move > 165) return "Invalid";
    return std::string(Names::getMoveName(move)) + " (#" + std::to_string(move) + ")";
}

std::array<uint8_t, 4> storedDVs(const PokemonRecord& pokemon) {
    return {pokemon.dvs[1], pokemon.dvs[2], pokemon.dvs[3], pokemon.dvs[4]};
}

uint32_t draftExperience(const BoxPokemonCreate& draft) noexcept {
    return Pokemon::getExpForLevel(draft.level, Editor::growthRate(draft.species));
}

const char* sectionForRow(int row) noexcept {
    if (row < 4) return "Summary";
    if (row < 16) return "Moves";
    if (row < 21) return "DVs";
    if (row < 26) return "Stat Exp";
    if (row < 28) return "Trainer";
    return "Actions";
}

int jumpSectionRow(int row, int direction) noexcept {
    constexpr auto starts = UX::addSectionStarts();
    int current = 0;
    for (int i = 0; i < static_cast<int>(starts.size()); ++i) {
        if (row >= static_cast<int>(starts[i])) current = i;
    }
    current = (current + direction + static_cast<int>(starts.size())) % static_cast<int>(starts.size());
    return static_cast<int>(starts[current]);
}

const char* fieldLabel(int row) noexcept {
    static constexpr const char* labels[] = {
        "Species", "Nickname", "Level", "Experience",
        "Move 1", "Move 1 PP", "Move 1 PP Ups",
        "Move 2", "Move 2 PP", "Move 2 PP Ups",
        "Move 3", "Move 3 PP", "Move 3 PP Ups",
        "Move 4", "Move 4 PP", "Move 4 PP Ups",
        "Attack DV", "Defense DV", "Speed DV", "Special DV", "HP DV",
        "HP Stat Exp", "Attack Stat Exp", "Defense Stat Exp", "Speed Stat Exp", "Special Stat Exp",
        "OT", "Trainer ID", "Review / Stage Add", "Back / Cancel",
    };
    return (row >= 0 && row < 30) ? labels[row] : "Field";
}

std::string draftValue(const BoxPokemonCreate& d, const Editor& e, int row) {
    if (row == 0) return speciesCompact(d.species);
    if (row == 1) return d.nickname.empty() ? std::string(Names::getSpeciesName(d.species)) + " (default)" : d.nickname;
    if (row == 2) return std::to_string(d.level);
    if (row == 3) return std::to_string(draftExperience(d)) + " (derived)";
    if (row >= 4 && row < 16) {
        const int slot = (row - 4) / 3;
        const int part = (row - 4) % 3;
        if (part == 0) return moveDisplay(d.moves[slot]);
        if (part == 1) return std::to_string(d.pp[slot]);
        return std::to_string(d.ppUps[slot]);
    }
    if (row >= 16 && row < 20) return std::to_string(d.dvs[row - 16]);
    if (row == 20) return std::to_string(Editor::derivedHPDV(d.dvs)) + " (derived / read-only)";
    if (row >= 21 && row < 26) return std::to_string(d.statExperience[row - 21]);
    if (row == 26) return d.otName.empty() ? e.view().trainer().name + " (default)" : d.otName;
    if (row == 27) return std::to_string(d.trainerId.value_or(e.view().trainer().trainerId));
    if (row == 28) return "Stage this complete draft";
    if (row == 29) return "Discard draft; no staged mutation";
    return {};
}

std::string existingValue(const PokemonRecord& p, int row) {
    if (row == 0) return speciesCompact(p.species);
    if (row == 1) return p.nickname;
    if (row == 2) return std::to_string(p.level);
    if (row == 3) return std::to_string(p.experience);
    if (row >= 4 && row < 16) {
        const int slot = (row - 4) / 3;
        const int part = (row - 4) % 3;
        if (part == 0) return moveDisplay(p.moves[slot]);
        if (part == 1) return std::to_string(p.pp[slot]);
        return std::to_string(p.ppUps[slot]);
    }
    if (row >= 16 && row < 20) return std::to_string(p.dvs[(row - 16) + 1]);
    if (row == 20) return std::to_string(p.dvs[0]) + " (derived / read-only)";
    if (row >= 21 && row < 26) return std::to_string(p.statExperience[row - 21]);
    if (row == 26) return p.originalTrainer;
    if (row == 27) return std::to_string(p.trainerId);
    if (row == 28) return "Open semantic Pending Changes";
    if (row == 29) return "Back to Actions";
    return {};
}

void closeOverlay(OverlayState& state) {
    state.mode = Mode::Closed;
    state.row = 0;
    state.picker = PickerKind::None;
}

void openActions(TrainerViewScreen& screen) {
    auto& state = stateFor(screen);
    auto* trainer = rbyTrainer(screen);
    if (!trainer || !trainer->stagedPokemonAvailable()) {
        const std::string why = trainer ? trainer->stagedPokemonUnavailableReason() : std::string{};
        screen.postStatus(why.empty() ? "Generation I boxed editing is unavailable; source remains read-only" : why, 300);
        return;
    }
    if (screen.selectedBoxIndex < 0 || screen.selectedBoxIndex >= static_cast<int>(screen.trainer.getBoxCount()) ||
        screen.selectedItemIndex < 0 || screen.selectedItemIndex >= static_cast<int>(screen.trainer.getSlotsPerBox())) {
        screen.postStatus("Select a Generation I box slot first");
        return;
    }
    state.mode = Mode::Actions;
    state.box = screen.selectedBoxIndex;
    state.slot = screen.selectedItemIndex;
    state.row = 0;
    state.picker = PickerKind::None;
}

void beginAddDraft(TrainerViewScreen& screen) {
    auto& state = stateFor(screen);
    auto* e = editor(screen);
    if (!e) return;
    const int expected = firstEmpty(*e, state.box);
    if (expected < 0) {
        screen.postStatus("This Generation I box is full", 300);
        return;
    }
    if (state.slot != expected) {
        screen.postStatus("Gen I boxes are contiguous; Add is available at the first empty slot only", 300);
        return;
    }
    state.draft = BoxPokemonCreate{};
    state.mode = Mode::AddDraft;
    state.row = 0;
    state.picker = PickerKind::None;
}

void openSpeciesPicker(OverlayState& state, uint16_t current, bool draft) {
    state.picker = PickerKind::Species;
    state.pickerValue = std::clamp<int>(current, 1, 151);
    state.pickerForDraft = draft;
}

void openMovePicker(OverlayState& state, uint8_t current, int slot, bool draft) {
    state.picker = PickerKind::Move;
    state.pickerValue = std::clamp<int>(current, 0, 165);
    state.pickerMoveSlot = slot;
    state.pickerForDraft = draft;
}

bool applyPicker(TrainerViewScreen& screen) {
    auto& state = stateFor(screen);
    auto* e = editor(screen);
    if (!e) return false;
    if (state.pickerForDraft) {
        if (state.picker == PickerKind::Species) {
            // Preview-only identity change. The draft lives solely in overlay memory until Stage Add.
            state.draft.species = static_cast<uint16_t>(state.pickerValue);
        } else if (state.picker == PickerKind::Move) {
            const int slot = state.pickerMoveSlot;
            state.draft.moves[slot] = static_cast<uint8_t>(state.pickerValue);
            state.draft.ppUps[slot] = 0;
            state.draft.pp[slot] = Editor::moveBasePP(state.draft.moves[slot]);
        }
        state.picker = PickerKind::None;
        return true;
    }

    std::string error;
    auto pokemon = selectedPokemon(screen, error);
    if (!pokemon) {
        screen.postStatus(error.empty() ? "Selected boxed Pokemon is unavailable" : error, 300);
        state.picker = PickerKind::None;
        return false;
    }
    BoxPokemonEdit edit;
    if (state.picker == PickerKind::Species) {
        edit.species = static_cast<uint16_t>(state.pickerValue);
    } else if (state.picker == PickerKind::Move) {
        auto moves = pokemon->moves;
        moves[state.pickerMoveSlot] = static_cast<uint8_t>(state.pickerValue);
        edit.moves = moves;
    }
    if (!e->stageEdit(static_cast<size_t>(state.box), static_cast<size_t>(state.slot), edit, error)) {
        screen.postStatus(error.empty() ? "Generation I edit was rejected" : error, 300);
        state.picker = PickerKind::None;
        return false;
    }
    state.picker = PickerKind::None;
    refreshBoxes(screen);
    screen.postStatus("Pokemon change staged only; original RetroArch .srm unchanged", 240);
    return true;
}

bool stageAdd(TrainerViewScreen& screen) {
    auto& state = stateFor(screen);
    auto* e = editor(screen);
    if (!e) return false;
    std::string error;
    if (!e->stageAdd(static_cast<size_t>(state.box), static_cast<size_t>(state.slot), state.draft, error)) {
        screen.postStatus(error.empty() ? "Generation I Add was rejected" : error, 360);
        return false;
    }
    refreshBoxes(screen);
    screen.postStatus("Pokemon staged in box; original RetroArch .srm unchanged", 300);
    closeOverlay(state);
    return true;
}

bool editDraftField(TrainerViewScreen& screen) {
    auto& state = stateFor(screen);
    auto* e = editor(screen);
    if (!e) return false;
    const int row = state.row;
    if (row == 0) {
        openSpeciesPicker(state, state.draft.species, true);
        return true;
    }
    if (row == 1) {
        const std::string initial = state.draft.nickname.empty()
            ? std::string(Names::getSpeciesName(state.draft.species)) : state.draft.nickname;
        const auto result = Utils::promptText("Generation I Nickname", "1-10 supported Gen I characters", initial, 10);
        if (!result.accepted) return false;
        state.draft.nickname = result.text;
        return true;
    }
    if (row == 2) {
        const auto result = Utils::promptNumber("Generation I Level", state.draft.level, 1, 100);
        if (!result.accepted) return false;
        state.draft.level = static_cast<uint8_t>(result.value);
        return true;
    }
    if (row == 3) {
        screen.postStatus("Draft EXP is derived from Species + Level and updates automatically", 240);
        return false;
    }
    if (row >= 4 && row < 16) {
        const int slot = (row - 4) / 3;
        const int part = (row - 4) % 3;
        if (part == 0) {
            openMovePicker(state, state.draft.moves[slot], slot, true);
            return true;
        }
        if (part == 1) {
            const int maxPP = Editor::moveMaxPP(state.draft.moves[slot], state.draft.ppUps[slot]);
            const auto result = Utils::promptNumber("Generation I Move PP", state.draft.pp[slot], 0, maxPP);
            if (!result.accepted) return false;
            state.draft.pp[slot] = static_cast<uint8_t>(result.value);
            return true;
        }
        const auto result = Utils::promptNumber("Generation I PP Ups", state.draft.ppUps[slot], 0, 3);
        if (!result.accepted) return false;
        state.draft.ppUps[slot] = static_cast<uint8_t>(result.value);
        state.draft.pp[slot] = std::min<uint8_t>(state.draft.pp[slot],
            Editor::moveMaxPP(state.draft.moves[slot], state.draft.ppUps[slot]));
        return true;
    }
    if (row >= 16 && row < 20) {
        const int i = row - 16;
        const auto result = Utils::promptNumber("Generation I DV (0-15)", state.draft.dvs[i], 0, 15);
        if (!result.accepted) return false;
        state.draft.dvs[i] = static_cast<uint8_t>(result.value);
        return true;
    }
    if (row == 20) {
        screen.postStatus("HP DV is derived from Attack/Defense/Speed/Special low bits", 240);
        return false;
    }
    if (row >= 21 && row < 26) {
        const int i = row - 21;
        const auto result = Utils::promptNumber("Generation I Stat Experience (0-65535)",
            state.draft.statExperience[i], 0, 65535);
        if (!result.accepted) return false;
        state.draft.statExperience[i] = static_cast<uint16_t>(result.value);
        return true;
    }
    if (row == 26) {
        const std::string initial = state.draft.otName.empty() ? e->view().trainer().name : state.draft.otName;
        const auto result = Utils::promptText("Generation I OT", "1-7 supported Gen I characters", initial, 7);
        if (!result.accepted) return false;
        state.draft.otName = result.text;
        return true;
    }
    if (row == 27) {
        const int current = state.draft.trainerId.value_or(e->view().trainer().trainerId);
        const auto result = Utils::promptNumber("Generation I Trainer ID", current, 0, 65535);
        if (!result.accepted) return false;
        state.draft.trainerId = static_cast<uint16_t>(result.value);
        return true;
    }
    if (row == 28) return stageAdd(screen);
    if (row == 29) {
        state.draft = BoxPokemonCreate{};
        state.mode = Mode::Actions;
        state.row = 0;
        return true;
    }
    return false;
}

bool editExistingField(TrainerViewScreen& screen) {
    auto& state = stateFor(screen);
    auto* e = editor(screen);
    if (!e) return false;
    std::string error;
    auto pokemon = selectedPokemon(screen, error);
    if (!pokemon) {
        screen.postStatus(error.empty() ? "Selected boxed Pokemon is unavailable" : error, 300);
        return false;
    }
    const int row = state.row;
    if (row == 28) {
        state.mode = Mode::Review;
        state.reviewRow = 0;
        return true;
    }
    if (row == 29) {
        state.mode = Mode::Actions;
        state.row = 0;
        return true;
    }

    BoxPokemonEdit edit;
    if (row == 0) {
        openSpeciesPicker(state, pokemon->species, false);
        return true;
    }
    if (row == 1) {
        const auto result = Utils::promptText("Generation I Nickname", "1-10 supported Gen I characters", pokemon->nickname, 10);
        if (!result.accepted) return false;
        edit.nickname = result.text;
    } else if (row == 2) {
        const auto result = Utils::promptNumber("Generation I Level", pokemon->level, 1, 100);
        if (!result.accepted) return false;
        edit.level = static_cast<uint8_t>(result.value);
    } else if (row == 3) {
        const auto result = Utils::promptNumber("Generation I Experience", static_cast<int>(pokemon->experience), 0, 2000000);
        if (!result.accepted) return false;
        edit.experience = static_cast<uint32_t>(result.value);
    } else if (row >= 4 && row < 16) {
        const int slot = (row - 4) / 3;
        const int part = (row - 4) % 3;
        if (part == 0) {
            openMovePicker(state, pokemon->moves[slot], slot, false);
            return true;
        }
        if (part == 1) {
            auto pp = pokemon->pp;
            const int maxPP = Editor::moveMaxPP(pokemon->moves[slot], pokemon->ppUps[slot]);
            const auto result = Utils::promptNumber("Generation I Move PP", pp[slot], 0, maxPP);
            if (!result.accepted) return false;
            pp[slot] = static_cast<uint8_t>(result.value);
            edit.pp = pp;
        } else {
            auto ups = pokemon->ppUps;
            const auto result = Utils::promptNumber("Generation I PP Ups", ups[slot], 0, 3);
            if (!result.accepted) return false;
            ups[slot] = static_cast<uint8_t>(result.value);
            edit.ppUps = ups;
            auto pp = pokemon->pp;
            pp[slot] = std::min<uint8_t>(pp[slot], Editor::moveMaxPP(pokemon->moves[slot], ups[slot]));
            edit.pp = pp;
        }
    } else if (row >= 16 && row < 20) {
        auto dvs = storedDVs(*pokemon);
        const int i = row - 16;
        const auto result = Utils::promptNumber("Generation I DV (0-15)", dvs[i], 0, 15);
        if (!result.accepted) return false;
        dvs[i] = static_cast<uint8_t>(result.value);
        edit.dvs = dvs;
    } else if (row == 20) {
        screen.postStatus("HP DV is derived from Attack/Defense/Speed/Special low bits", 240);
        return false;
    } else if (row >= 21 && row < 26) {
        auto stats = pokemon->statExperience;
        const int i = row - 21;
        const auto result = Utils::promptNumber("Generation I Stat Experience (0-65535)", stats[i], 0, 65535);
        if (!result.accepted) return false;
        stats[i] = static_cast<uint16_t>(result.value);
        edit.statExperience = stats;
    } else if (row == 26) {
        const auto result = Utils::promptText("Generation I OT", "1-7 supported Gen I characters", pokemon->originalTrainer, 7);
        if (!result.accepted) return false;
        edit.otName = result.text;
    } else if (row == 27) {
        const auto result = Utils::promptNumber("Generation I Trainer ID", pokemon->trainerId, 0, 65535);
        if (!result.accepted) return false;
        edit.trainerId = static_cast<uint16_t>(result.value);
    }

    if (!e->stageEdit(static_cast<size_t>(state.box), static_cast<size_t>(state.slot), edit, error)) {
        screen.postStatus(error.empty() ? "Generation I edit was rejected" : error, 300);
        return false;
    }
    refreshBoxes(screen);
    screen.postStatus("Pokemon change staged only; original RetroArch .srm unchanged", 240);
    return true;
}

bool stageClone(TrainerViewScreen& screen) {
    auto& state = stateFor(screen);
    auto* e = editor(screen);
    if (!e) return false;
    const int dest = firstEmpty(*e, state.cloneBox);
    if (dest < 0) {
        screen.postStatus("Destination Generation I box is full", 240);
        return false;
    }
    std::string error;
    if (!e->stageClone(static_cast<size_t>(state.box), static_cast<size_t>(state.slot),
                       static_cast<size_t>(state.cloneBox), static_cast<size_t>(dest), error)) {
        screen.postStatus(error.empty() ? "Generation I clone was rejected" : error, 300);
        return false;
    }
    refreshBoxes(screen);
    screen.postStatus("Clone staged into first empty Gen I slot; original .srm unchanged", 300);
    closeOverlay(state);
    return true;
}

bool stageRemove(TrainerViewScreen& screen) {
    auto& state = stateFor(screen);
    auto* e = editor(screen);
    if (!e) return false;
    std::string error;
    if (!e->stageRemove(static_cast<size_t>(state.box), static_cast<size_t>(state.slot), error)) {
        screen.postStatus(error.empty() ? "Generation I removal was rejected" : error, 300);
        return false;
    }
    refreshBoxes(screen);
    screen.postStatus("Removal staged with Gen I box compaction; original .srm unchanged", 300);
    closeOverlay(state);
    return true;
}

bool hasPending(TrainerViewScreen& screen) {
    auto* trainer = rbyTrainer(screen);
    if (!trainer) return false;
    const bool pokemon = trainer->stagedPokemon() && trainer->stagedPokemon()->hasPendingChanges();
    const bool inventory = trainer->stagedInventory() && trainer->stagedInventory()->hasPendingChanges();
    return pokemon || inventory;
}

std::vector<std::string> pendingLines(TrainerViewScreen& screen) {
    std::vector<std::string> out;
    auto* trainer = rbyTrainer(screen);
    if (!trainer) return out;
    if (auto* e = trainer->stagedPokemon()) {
        for (const auto& change : e->pendingChanges())
            out.push_back(change.label + ": " + change.beforeValue + " -> " + change.afterValue);
    }
    if (auto* inv = trainer->stagedInventory()) {
        for (const auto& change : inv->pendingChanges()) {
            out.push_back("Inventory item " + std::to_string(change.itemId) + ": " +
                std::to_string(change.before) + " -> " + std::to_string(change.after));
        }
    }
    return out;
}

bool pathExists(const std::string& path) noexcept {
    struct stat st{};
    return ::stat(path.c_str(), &st) == 0;
}

bool writeBytes(const std::string& path, std::span<const uint8_t> bytes) {
    FILE* out = std::fopen(path.c_str(), "wb");
    if (!out) return false;
    const bool ok = std::fwrite(bytes.data(), 1, bytes.size(), out) == bytes.size();
    const bool closeOk = std::fclose(out) == 0;
    return ok && closeOk;
}

bool writeText(const std::string& path, const std::string& text) {
    return writeBytes(path, std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(text.data()), text.size()));
}

std::string exportStagedCopy(TrainerViewScreen& screen, std::string& error) {
    error.clear();
    auto* trainer = rbyTrainer(screen);
    auto* pokemon = trainer ? trainer->stagedPokemon() : nullptr;
    if (!pokemon) {
        error = "Generation I staged Pokemon editor is unavailable";
        return {};
    }
    if (!hasPending(screen)) {
        error = "No pending Generation I changes to export";
        return {};
    }

    std::vector<uint8_t> inventoryCandidate;
    if (auto* inventory = trainer->stagedInventory(); inventory && inventory->hasPendingChanges()) {
        inventoryCandidate = inventory->finalize(error);
        if (inventoryCandidate.empty()) return {};
    }
    const auto edited = pokemon->finalizedBytes(error, inventoryCandidate);
    if (edited.empty()) {
        if (error.empty()) error = "Generation I staged export finalization failed";
        return {};
    }

    const std::string root = PokeBank::Paths::gen1ExportsRoot();
    if (!PokeBank::Paths::ensureGen1ExportsRoot(&error)) return {};
    const std::string timestamp = Utils::getTimestamp();
    std::string exportDir = PokeBank::Paths::gen1ExportDirectory(screen.sourceGameId, timestamp);
    if (exportDir.empty()) {
        error = "Could not construct safe Generation I export path";
        return {};
    }
    const std::string baseName = exportDir.substr(root.size() + 1);
    for (int suffix = 2; pathExists(exportDir) && suffix < 1000; ++suffix)
        exportDir = root + "/" + baseName + "-" + std::to_string(suffix);
    if (pathExists(exportDir) || ::mkdir(exportDir.c_str(), 0777) != 0) {
        error = "Could not create unique Generation I export directory";
        return {};
    }

    if (!writeBytes(exportDir + "/original_backup.srm", pokemon->originalBytes())) {
        error = "Could not write automatic original Generation I backup";
        return {};
    }
    if (!writeBytes(exportDir + "/edited.srm", edited)) {
        error = "Could not write staged Generation I edited save";
        return {};
    }

    std::string manifest;
    manifest += "PokeBank NX Generation I staged boxed-Pokemon export\n";
    manifest += "GAME_ID=" + screen.sourceGameId + "\n";
    manifest += "SOURCE_PATH=" + screen.backupDir + "\n";
    manifest += "SOURCE_KIND=RetroArchLegacy_READ_ONLY\n";
    manifest += "ORIGINAL_BACKUP=original_backup.srm\n";
    manifest += "EDITED_SAVE=edited.srm\n";
    manifest += "LIVE_RETROARCH_WRITE=DISABLED\n";
    manifest += "LIVE_INSTALLED_GAME_WRITE=DISABLED\n";
    manifest += "PARTY_EDIT=DEFERRED\n";
    manifest += "LAYOUT=International_RBY_only\n";
    manifest += "PENDING_CHANGES:\n";
    for (const auto& line : pendingLines(screen)) manifest += "- " + line + "\n";
    if (!writeText(exportDir + "/EDIT_MANIFEST.txt", manifest)) {
        error = "Edited save was written, but provenance manifest creation failed";
        return {};
    }
    return exportDir;
}

void discardAll(TrainerViewScreen& screen) {
    auto* trainer = rbyTrainer(screen);
    if (!trainer) return;
    if (auto* e = trainer->stagedPokemon()) e->discard();
    if (auto* e = trainer->stagedInventory()) e->discard();
    refreshBoxes(screen);
    ClassicInventory::refreshPresentation(screen);
}

void handlePickerInput(TrainerViewScreen& screen, uint64_t down) {
    auto& state = stateFor(screen);
    const int minValue = state.picker == PickerKind::Species ? 1 : 0;
    const int maxValue = state.picker == PickerKind::Species ? 151 : 165;
    if (down & HidNpadButton_Up) state.pickerValue = std::max(minValue, state.pickerValue - 1);
    if (down & HidNpadButton_Down) state.pickerValue = std::min(maxValue, state.pickerValue + 1);
    if (down & HidNpadButton_L) state.pickerValue = std::max(minValue, state.pickerValue - 10);
    if (down & HidNpadButton_R) state.pickerValue = std::min(maxValue, state.pickerValue + 10);
    if (down & HidNpadButton_B) state.picker = PickerKind::None;
    else if (down & HidNpadButton_A) applyPicker(screen);
}

void handleActiveInput(TrainerViewScreen& screen, uint64_t down) {
    auto& state = stateFor(screen);
    auto* e = editor(screen);
    if (!e) {
        closeOverlay(state);
        return;
    }
    if (state.picker != PickerKind::None) {
        handlePickerInput(screen, down);
        return;
    }

    if (state.mode == Mode::Actions) {
        std::string error;
        const auto pokemon = selectedPokemon(screen, error);
        const auto actions = UX::actionsForSlot(static_cast<bool>(pokemon));
        const int rows = static_cast<int>(actions.count);
        if (down & HidNpadButton_Up) state.row = (state.row + rows - 1) % rows;
        if (down & HidNpadButton_Down) state.row = (state.row + 1) % rows;
        if (down & HidNpadButton_B) { closeOverlay(state); return; }
        if (!(down & HidNpadButton_A)) return;
        switch (actions[static_cast<size_t>(state.row)]) {
            case UX::Action::View: state.mode = Mode::View; state.row = 0; break;
            case UX::Action::Edit: state.mode = Mode::Edit; state.row = 0; break;
            case UX::Action::Clone: state.cloneBox = state.box; state.mode = Mode::CloneConfirm; break;
            case UX::Action::Remove: state.mode = Mode::RemoveConfirm; break;
            case UX::Action::LegalityProvenance: state.mode = Mode::Provenance; break;
            case UX::Action::AddPokemon: beginAddDraft(screen); break;
            case UX::Action::ReviewPendingChanges: state.mode = Mode::Review; state.reviewRow = 0; break;
            case UX::Action::Cancel: closeOverlay(state); break;
        }
        return;
    }

    if (state.mode == Mode::View || state.mode == Mode::Provenance) {
        if (down & (HidNpadButton_A | HidNpadButton_B)) { state.mode = Mode::Actions; state.row = 0; }
        return;
    }

    if (state.mode == Mode::Edit || state.mode == Mode::AddDraft) {
        constexpr int rows = 30;
        if (down & HidNpadButton_Up) state.row = (state.row + rows - 1) % rows;
        if (down & HidNpadButton_Down) state.row = (state.row + 1) % rows;
        if (down & HidNpadButton_L) state.row = jumpSectionRow(state.row, -1);
        if (down & HidNpadButton_R) state.row = jumpSectionRow(state.row, 1);
        if (state.mode == Mode::AddDraft && (down & HidNpadButton_X)) { stageAdd(screen); return; }
        if (down & HidNpadButton_B) {
            if (state.mode == Mode::AddDraft) state.draft = BoxPokemonCreate{};
            state.mode = Mode::Actions;
            state.row = 0;
            return;
        }
        if (down & HidNpadButton_A) {
            if (state.mode == Mode::AddDraft) editDraftField(screen);
            else editExistingField(screen);
        }
        return;
    }

    if (state.mode == Mode::CloneConfirm) {
        if (down & HidNpadButton_Left) state.cloneBox = (state.cloneBox + 11) % 12;
        if (down & HidNpadButton_Right) state.cloneBox = (state.cloneBox + 1) % 12;
        if (down & HidNpadButton_B) { state.mode = Mode::Actions; state.row = 0; return; }
        if (down & HidNpadButton_A) stageClone(screen);
        return;
    }

    if (state.mode == Mode::RemoveConfirm) {
        if (down & HidNpadButton_B) { state.mode = Mode::Actions; state.row = 0; return; }
        if (down & HidNpadButton_A) stageRemove(screen);
        return;
    }

    if (state.mode == Mode::Review) {
        const auto lines = pendingLines(screen);
        const int count = static_cast<int>(lines.size());
        if (count > 0 && (down & HidNpadButton_Up)) state.reviewRow = (state.reviewRow + count - 1) % count;
        if (count > 0 && (down & HidNpadButton_Down)) state.reviewRow = (state.reviewRow + 1) % count;
        if (down & HidNpadButton_B) { state.mode = Mode::Actions; state.row = 0; return; }
        if (down & HidNpadButton_Y) {
            discardAll(screen);
            screen.postStatus("All Generation I staged changes discarded; original .srm was never modified", 300);
            state.reviewRow = 0;
            return;
        }
        if (down & HidNpadButton_A) {
            std::string error;
            const auto path = exportStagedCopy(screen, error);
            if (path.empty()) screen.postStatus(error.empty() ? "Generation I export failed" : error, 360);
            else {
                state.lastExportDirectory = path;
                screen.postStatus("Edited copy exported under PokeBank-NX; original .srm unchanged", 360);
            }
        }
    }
}

void drawFooter(PKSEFramebuffer& fb, const std::string& text) {
    const int h = 48;
    const int y = fb.getHeight() - h;
    fb.drawFilledRect(0, y, fb.getWidth(), h, Colors::Panel);
    fb.drawFilledRect(0, y, fb.getWidth(), 2, Colors::Accent);
    int tw = 0, th = 0;
    fb.measureText(text, tw, th, TextStyle::Caption);
    fb.drawText(std::max(16, (fb.getWidth() - tw) / 2), y + (h - th) / 2, text, Colors::Text, TextStyle::Caption);
}

void drawRow(PKSEFramebuffer& fb, int x, int y, int w, const std::string& label,
             const std::string& value, bool selected) {
    if (selected) fb.drawSelectionHighlight(x, y, w, 38);
    fb.drawText(x + 12, y + 9, label, selected ? Colors::SelectedText : Colors::TextDim, TextStyle::Caption);
    fb.drawText(x + 205, y + 8, value, selected ? Colors::SelectedText : Colors::Text);
}

void drawPreview(PKSEFramebuffer& fb, int x, int y, int w, const PokemonRecord* pokemon,
                 const BoxPokemonCreate* draft, const Editor& e) {
    const uint16_t species = draft ? draft->species : (pokemon ? pokemon->species : 0);
    const uint8_t level = draft ? draft->level : (pokemon ? pokemon->level : 0);
    const std::string nickname = draft
        ? (draft->nickname.empty() ? std::string(Names::getSpeciesName(species)) : draft->nickname)
        : (pokemon ? pokemon->nickname : std::string{});
    const uint32_t exp = draft ? draftExperience(*draft) : (pokemon ? pokemon->experience : 0);

    fb.drawText(x, y, draft ? "LIVE DRAFT PREVIEW" : "STAGED POKEMON", Colors::Accent, TextStyle::Caption);
    if (species >= 1 && species <= 151) {
        if (auto* sprite = SpriteManager::getSprite(species, false); sprite && sprite->data) {
            fb.drawSpriteStaticContained(x, y + 30, w, 210, sprite->width, sprite->height, sprite->data, sprite->channels);
        } else {
            fb.drawText(x + 26, y + 120, "Sprite unavailable", Colors::TextDim);
        }
    }
    fb.drawText(x, y + 254, speciesCompact(species), Colors::Text, TextStyle::Heading);
    fb.drawText(x, y + 292, nickname, Colors::Text);
    fb.drawText(x, y + 326, "Level " + std::to_string(level), Colors::Text);
    fb.drawText(x, y + 358, "EXP " + std::to_string(exp), Colors::TextDim);

    if (pokemon) {
        fb.drawText(x, y + 404, "READ-ONLY BATTLE STATS", Colors::Accent, TextStyle::Caption);
        fb.drawText(x, y + 432, "HP " + std::to_string(pokemon->maxHP) +
            "   Atk " + std::to_string(pokemon->attack), Colors::Text);
        fb.drawText(x, y + 462, "Def " + std::to_string(pokemon->defense) +
            "   Spe " + std::to_string(pokemon->speed), Colors::Text);
        fb.drawText(x, y + 492, "Special " + std::to_string(pokemon->special), Colors::Text);
    } else if (draft) {
        fb.drawText(x, y + 404, "DRAFT DERIVED VALUES", Colors::Accent, TextStyle::Caption);
        fb.drawText(x, y + 432, "HP DV " + std::to_string(Editor::derivedHPDV(draft->dvs)), Colors::Text);
        fb.drawText(x, y + 462, "TID " + std::to_string(draft->trainerId.value_or(e.view().trainer().trainerId)), Colors::Text);
        fb.drawText(x, y + 492, "Battle stats validate after Stage Add", Colors::TextDim, TextStyle::Caption);
    }
}

void drawEditor(TrainerViewScreen& screen, PKSEFramebuffer& fb, int x, int y, int w, int h, bool add) {
    auto& state = stateFor(screen);
    auto* e = editor(screen);
    if (!e) return;
    std::string error;
    auto pokemon = add ? std::optional<PokemonRecord>{} : selectedPokemon(screen, error);
    if (!add && !pokemon) {
        fb.drawText(x + 30, y + 80, error.empty() ? "Pokemon unavailable" : error, Colors::Accent);
        return;
    }

    const std::string title = add ? "Add Pokemon — DRAFT" : "Edit Pokemon — STAGED";
    fb.drawText(x + 26, y + 18, title, Colors::Text, TextStyle::Heading);
    fb.drawText(x + 26, y + 51,
        "Box " + std::to_string(state.box + 1) + " / Slot " + std::to_string(state.slot + 1) +
        "   |   One scrollable workspace   |   " + sectionForRow(state.row),
        Colors::TextDim, TextStyle::Caption);

    const int leftX = x + 28;
    const int leftW = 270;
    drawPreview(fb, leftX, y + 82, leftW, pokemon ? &*pokemon : nullptr, add ? &state.draft : nullptr, *e);

    const int listX = x + 330;
    const int listW = w - 360;
    const int listY = y + 82;
    const int visible = 12;
    const int start = std::clamp(state.row - visible / 2, 0, 30 - visible);
    for (int i = 0; i < visible; ++i) {
        const int field = start + i;
        const std::string value = add ? draftValue(state.draft, *e, field) : existingValue(*pokemon, field);
        drawRow(fb, listX, listY + i * 41, listW, fieldLabel(field), value, field == state.row);
    }
    fb.drawText(listX, y + h - 70,
        "Row " + std::to_string(state.row + 1) + "/30   •   " + sectionForRow(state.row),
        Colors::TextDim, TextStyle::Caption);
    fb.drawText(leftX, y + h - 70,
        add ? "Nothing is staged until Stage Add." : "Edits are staged; source .srm stays immutable.",
        Colors::Accent, TextStyle::Caption);
}

void drawPicker(TrainerViewScreen& screen, PKSEFramebuffer& fb, int outerX, int outerY, int outerW, int outerH) {
    auto& state = stateFor(screen);
    const int w = 650;
    const int h = 520;
    const int x = outerX + (outerW - w) / 2;
    const int y = outerY + (outerH - h) / 2;
    fb.drawFilledRect(outerX, outerY, outerW, outerH, Color(0, 0, 0, 105));
    fb.drawSoftShadow(x, y, w, h, 16);
    fb.drawFilledRoundedRect(x, y, w, h, 16, Colors::Panel);
    fb.drawRoundedRect(x, y, w, h, 16, Colors::Accent, 2);
    fb.drawText(x + 24, y + 18,
        state.picker == PickerKind::Species ? "Choose Species" : "Choose Move",
        Colors::Text, TextStyle::Heading);

    const int minValue = state.picker == PickerKind::Species ? 1 : 0;
    const int maxValue = state.picker == PickerKind::Species ? 151 : 165;
    constexpr int visible = 9;
    const int start = std::clamp(state.pickerValue - visible / 2, minValue,
        std::max(minValue, maxValue - visible + 1));
    for (int i = 0; i < visible; ++i) {
        const int value = start + i;
        if (value > maxValue) break;
        const int rowY = y + 72 + i * 42;
        const bool selected = value == state.pickerValue;
        if (selected) fb.drawSelectionHighlight(x + 24, rowY, w - 48, 38);
        std::string text;
        if (state.picker == PickerKind::Species) {
            text = UX::speciesPickerRow(static_cast<uint16_t>(value), Names::getSpeciesName(value));
        } else if (value == 0) {
            text = "000 - Empty move slot";
        } else {
            char prefix[8]{};
            std::snprintf(prefix, sizeof(prefix), "%03d - ", value);
            text = std::string(prefix) + Names::getMoveName(value);
        }
        fb.drawText(x + 40, rowY + 8, text, selected ? Colors::SelectedText : Colors::Text);
    }
    fb.drawText(x + 28, y + h - 38,
        "Compact ID + name rows — no split-screen gap", Colors::TextDim, TextStyle::Caption);
}

void drawActions(TrainerViewScreen& screen, PKSEFramebuffer& fb, int x, int y, int w, int h) {
    auto& state = stateFor(screen);
    std::string error;
    const auto pokemon = selectedPokemon(screen, error);
    const auto actions = UX::actionsForSlot(static_cast<bool>(pokemon));
    fb.drawText(x + 26, y + 18, pokemon ? "Generation I Pokemon Actions" : "Generation I Empty Slot",
        Colors::Text, TextStyle::Heading);
    fb.drawText(x + 26, y + 54,
        "Box " + std::to_string(state.box + 1) + " / Slot " + std::to_string(state.slot + 1),
        Colors::TextDim);
    static constexpr const char* names[] = {
        "View", "Edit", "Clone", "Remove", "Legality & Provenance",
        "Add Pokemon", "Review Pending Changes", "Cancel",
    };
    int rowY = y + 104;
    for (std::size_t i = 0; i < actions.count; ++i) {
        const auto action = actions[i];
        const int idx = static_cast<int>(action);
        if (static_cast<int>(i) == state.row) fb.drawSelectionHighlight(x + 30, rowY, w - 60, 44);
        fb.drawText(x + 48, rowY + 11, names[idx], static_cast<int>(i) == state.row ? Colors::SelectedText : Colors::Text);
        rowY += 50;
    }
    fb.drawText(x + 30, y + h - 62,
        "Original source is immutable. All mutations are staged/export-only.", Colors::Accent, TextStyle::Caption);
}

void drawView(TrainerViewScreen& screen, PKSEFramebuffer& fb, int x, int y, int w, int h) {
    auto& state = stateFor(screen);
    auto* e = editor(screen);
    if (!e) return;
    std::string error;
    auto p = selectedPokemon(screen, error);
    if (!p) {
        fb.drawText(x + 30, y + 90, error.empty() ? "Pokemon unavailable" : error, Colors::Accent);
        return;
    }
    fb.drawText(x + 26, y + 18, "Generation I Pokemon Summary", Colors::Text, TextStyle::Heading);
    drawPreview(fb, x + 30, y + 80, 300, &*p, nullptr, *e);
    const int rx = x + 380;
    fb.drawText(rx, y + 92, "Moves", Colors::Accent, TextStyle::Caption);
    for (int i = 0; i < 4; ++i)
        fb.drawText(rx, y + 126 + i * 34,
            std::to_string(i + 1) + ". " + moveDisplay(p->moves[i]) + "  PP " + std::to_string(p->pp[i]) +
            "  Ups " + std::to_string(p->ppUps[i]), Colors::Text);
    fb.drawText(rx, y + 282, "DVs", Colors::Accent, TextStyle::Caption);
    fb.drawText(rx, y + 316,
        "HP " + std::to_string(p->dvs[0]) + " / Atk " + std::to_string(p->dvs[1]) +
        " / Def " + std::to_string(p->dvs[2]) + " / Spe " + std::to_string(p->dvs[3]) +
        " / Special " + std::to_string(p->dvs[4]), Colors::Text);
    fb.drawText(rx, y + 370, "Trainer", Colors::Accent, TextStyle::Caption);
    fb.drawText(rx, y + 404, "OT " + p->originalTrainer + "   TID " + std::to_string(p->trainerId), Colors::Text);
}

void drawReview(TrainerViewScreen& screen, PKSEFramebuffer& fb, int x, int y, int w, int h) {
    auto& state = stateFor(screen);
    const auto lines = pendingLines(screen);
    fb.drawText(x + 26, y + 18, "Generation I Pending Changes", Colors::Text, TextStyle::Heading);
    if (lines.empty()) {
        fb.drawText(x + 32, y + 106, "No staged changes.", Colors::TextDim);
    } else {
        constexpr int visible = 9;
        const int start = std::clamp(state.reviewRow - visible / 2, 0,
            std::max(0, static_cast<int>(lines.size()) - visible));
        for (int i = start; i < std::min(static_cast<int>(lines.size()), start + visible); ++i) {
            drawRow(fb, x + 28, y + 82 + (i - start) * 48, w - 56,
                std::to_string(i + 1), lines[i], i == state.reviewRow);
        }
    }
    if (!state.lastExportDirectory.empty())
        fb.drawText(x + 30, y + h - 72, "Last export: " + state.lastExportDirectory, Colors::TextDim, TextStyle::Caption);
    fb.drawText(x + 30, y + h - 46,
        "A Export edited copy + backup   Y Discard staged changes   B Back", Colors::Accent, TextStyle::Caption);
}

void drawProvenance(TrainerViewScreen& screen, PKSEFramebuffer& fb, int x, int y, int w, int h) {
    auto* trainer = rbyTrainer(screen);
    auto* e = editor(screen);
    fb.drawText(x + 26, y + 18, "Generation I Legality & Provenance", Colors::Text, TextStyle::Heading);
    const int lx = x + 40, vx = x + 330;
    const std::array<std::pair<const char*, std::string>, 7> rows{{
        {"Source game", screen.sourceGameId},
        {"Region/layout", trainer && trainer->japaneseLayout() ? "Japanese (EDIT DISABLED)" : "International (validated editor)"},
        {"Source mutation", "HARD DISABLED"},
        {"Party mutation", "DEFERRED"},
        {"Format validation", "Strict reparse + checksum on export"},
        {"Encounter legality", "NOT CLAIMED by this editor"},
        {"Original bytes", e ? std::to_string(e->originalBytes().size()) + " immutable bytes" : "Unavailable"},
    }};
    for (int i = 0; i < static_cast<int>(rows.size()); ++i) {
        fb.drawText(lx, y + 100 + i * 52, rows[i].first, Colors::TextDim);
        fb.drawText(vx, y + 100 + i * 52, rows[i].second,
            i == 2 || i == 5 ? Colors::Accent : Colors::Text);
    }
    fb.drawText(lx, y + h - 62,
        "Pending semantic changes: " + std::to_string(pendingLines(screen).size()), Colors::Text);
}

void drawConfirmation(TrainerViewScreen& screen, PKSEFramebuffer& fb, int x, int y, int w, int h) {
    auto& state = stateFor(screen);
    if (state.mode == Mode::CloneConfirm) {
        auto* e = editor(screen);
        const int dest = e ? firstEmpty(*e, state.cloneBox) : -1;
        fb.drawText(x + 26, y + 18, "Stage Clone", Colors::Text, TextStyle::Heading);
        fb.drawText(x + 40, y + 120,
            "Destination: Box " + std::to_string(state.cloneBox + 1) +
            (dest >= 0 ? " / Slot " + std::to_string(dest + 1) : " / FULL"), Colors::Text, TextStyle::Heading);
        fb.drawText(x + 40, y + 186,
            "The source Pokemon stays untouched. Clone uses the first empty contiguous Gen I slot.", Colors::TextDim);
    } else {
        fb.drawText(x + 26, y + 18, "Stage Removal?", Colors::Text, TextStyle::Heading);
        fb.drawText(x + 40, y + 120,
            "Removal compacts the Gen I box arrays in staged memory only.", Colors::Text);
        fb.drawText(x + 40, y + 174,
            "The original RetroArch .srm is never written.", Colors::Accent);
    }
}

std::string footerText(const TrainerViewScreen& screen, const OverlayState& state) {
    if (state.picker == PickerKind::Species || state.picker == PickerKind::Move)
        return "D-pad Navigate   L/R Page ±10   A Select   B Cancel";
    switch (state.mode) {
        case Mode::Actions: return "D-pad Navigate   A Select   B Back";
        case Mode::View: return "A/B Back";
        case Mode::Edit: return "D-pad/Stick Scroll   A Edit   L/R Jump Section   B Back";
        case Mode::AddDraft: return "D-pad/Stick Scroll   A Edit/Select   L/R Jump Section   X Stage Add   B Cancel Draft";
        case Mode::CloneConfirm: return "Left/Right Destination Box   A Stage Clone   B Cancel";
        case Mode::RemoveConfirm: return "A Confirm Remove   B Cancel";
        case Mode::Review: return "D-pad Navigate   A Export Copy   Y Discard Staged   B Back";
        case Mode::Provenance: return "A/B Back";
        case Mode::Closed: break;
    }
    if (entrySurface(screen)) {
        if (!editorAvailable(screen)) return "A View   L/R Box   B Back";
        std::string error;
        auto* mutableScreen = const_cast<TrainerViewScreen*>(&screen);
        const auto p = pokemonAt(*mutableScreen, screen.selectedBoxIndex, screen.selectedItemIndex, error);
        if (p) return "A Actions   Y Remove   L/R Box   B Back";
        return "A Actions   X Add   L/R Box   B Back";
    }
    return {};
}

} // namespace

bool isGen1Source(const TrainerViewScreen& screen) noexcept {
    return screen.sourceGameId == "red_gb" || screen.sourceGameId == "blue_gb" || screen.sourceGameId == "yellow_gb";
}

bool handleInput(TrainerViewScreen& screen, uint64_t down) {
    if (!isGen1Source(screen)) return false;
    auto& state = stateFor(screen);
    if (state.mode != Mode::Closed) {
        handleActiveInput(screen, down);
        return true;
    }

    if (!entrySurface(screen) || !editorAvailable(screen)) return false;
    if (!(down & (HidNpadButton_A | HidNpadButton_X | HidNpadButton_Y))) return false;

    state.box = screen.selectedBoxIndex;
    state.slot = screen.selectedItemIndex;
    state.row = 0;
    state.picker = PickerKind::None;
    std::string error;
    const auto pokemon = selectedPokemon(screen, error);

    if (down & HidNpadButton_X) {
        if (pokemon) {
            screen.postStatus("X Add is available on an empty first-contiguous Gen I slot", 240);
            return true;
        }
        beginAddDraft(screen);
        return true;
    }
    if (down & HidNpadButton_Y) {
        if (!pokemon) {
            screen.postStatus("Y Remove is available only on an occupied Gen I slot", 240);
            return true;
        }
        state.mode = Mode::RemoveConfirm;
        return true;
    }
    openActions(screen);
    return true;
}

void drawOverlay(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    if (!isGen1Source(screen)) return;
    auto& state = stateFor(screen);

    if (state.mode == Mode::Closed) {
        const auto text = footerText(screen, state);
        if (!text.empty()) drawFooter(fb, text);
        return;
    }

    screen.touchButtons.clear();
    const int w = 1120;
    const int h = 610;
    const int x = (fb.getWidth() - w) / 2;
    const int y = 42;
    fb.drawFilledRect(0, 0, fb.getWidth(), fb.getHeight() - 48, Color(0, 0, 0, 105));
    fb.drawSoftShadow(x, y, w, h, 18);
    fb.drawFilledRoundedRect(x, y, w, h, 18, Colors::Panel);
    fb.drawRoundedRect(x, y, w, h, 18, Colors::Accent, 2);

    switch (state.mode) {
        case Mode::Actions: drawActions(screen, fb, x, y, w, h); break;
        case Mode::View: drawView(screen, fb, x, y, w, h); break;
        case Mode::Edit: drawEditor(screen, fb, x, y, w, h, false); break;
        case Mode::AddDraft: drawEditor(screen, fb, x, y, w, h, true); break;
        case Mode::CloneConfirm:
        case Mode::RemoveConfirm: drawConfirmation(screen, fb, x, y, w, h); break;
        case Mode::Review: drawReview(screen, fb, x, y, w, h); break;
        case Mode::Provenance: drawProvenance(screen, fb, x, y, w, h); break;
        case Mode::Closed: break;
    }

    if (state.picker != PickerKind::None) drawPicker(screen, fb, x, y, w, h);
    drawFooter(fb, footerText(screen, state));
}

} // namespace UI::Gen1PokemonEditor
