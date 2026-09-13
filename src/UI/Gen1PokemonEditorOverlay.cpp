#include "UI/Gen1PokemonEditorOverlay.h"

#include "Integration/Gen1/Gen1StagedInventoryEditor.h"
#include "Integration/Gen1/Gen1StagedPokemonEditor.h"
#include "Legacy/RBYReadOnlyTrainer.h"
#include "Names/MoveNames.h"
#include "Names/SpeciesNames.h"
#include "UI/ClassicInventoryOverlay.h"
#include "UI/Common.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/ScreenChrome.h"
#include "UI/TrainerViewScreen.h"
#include "Utils/FileUtilities.h"
#include "Utils/Keyboard.h"
#include "Utils/PokeBankPaths.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdio>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <sys/stat.h>
#include <utility>
#include <vector>

namespace UI::Gen1PokemonEditor {
namespace {

using Editor = PokeVault::Integration::Gen1::StagedPokemonEditor;
using BoxPokemonCreate = PokeVault::Integration::Gen1::BoxPokemonCreate;
using BoxPokemonEdit = PokeVault::Integration::Gen1::BoxPokemonEdit;
using PokemonRecord = PokeVault::Integration::Gen1::PokemonRecord;

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

enum class EditSection : uint8_t { Summary, Moves, DVs, StatExp, Trainer };

struct OverlayState {
    const TrainerViewScreen* owner = nullptr;
    std::string sourceGameId;
    Mode mode = Mode::Closed;
    int box = 0;
    int slot = 0;
    int row = 0;
    EditSection section = EditSection::Summary;
    int addPage = 0;
    BoxPokemonCreate draft{};
    PickerKind picker = PickerKind::None;
    int pickerValue = 0;
    int pickerMoveSlot = 0;
    bool pickerForDraft = false;
    int cloneBox = 0;
    int reviewRow = 0;
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

bool refreshBoxes(TrainerViewScreen& screen) {
    auto* trainer = rbyTrainer(screen);
    if (!trainer) return false;
    std::string error;
    if (trainer->refreshBoxesFromStagedPokemon(error)) return true;
    screen.postStatus(error.empty() ? "Could not refresh staged Generation I boxes" : error, 300);
    return false;
}

std::optional<PokemonRecord> selectedPokemon(TrainerViewScreen& screen, std::string& error) {
    auto* e = editor(screen);
    if (!e) {
        auto* trainer = rbyTrainer(screen);
        error = trainer && !trainer->stagedPokemonUnavailableReason().empty()
            ? trainer->stagedPokemonUnavailableReason()
            : "Generation I staged boxed Pokemon editing is unavailable";
        return {};
    }
    const auto& state = stateFor(screen);
    return e->boxedPokemon(static_cast<size_t>(state.box), static_cast<size_t>(state.slot), error);
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

std::string speciesDisplay(uint16_t species) {
    if (species < 1 || species > 151) return "Invalid";
    return std::string(Names::getSpeciesName(species)) + " (" + std::to_string(species) + ")";
}

std::string moveDisplay(uint8_t move) {
    if (move == 0) return "Empty (0)";
    if (move > 165) return "Invalid (" + std::to_string(move) + ")";
    return std::string(Names::getMoveName(move)) + " (" + std::to_string(move) + ")";
}

std::array<uint8_t, 4> storedDVs(const PokemonRecord& pokemon) {
    return {pokemon.dvs[1], pokemon.dvs[2], pokemon.dvs[3], pokemon.dvs[4]};
}

int editRowCount(EditSection section) noexcept {
    switch (section) {
        case EditSection::Summary: return 4;
        case EditSection::Moves: return 12;
        case EditSection::DVs: return 5;
        case EditSection::StatExp: return 5;
        case EditSection::Trainer: return 2;
    }
    return 0;
}

const char* sectionName(EditSection section) noexcept {
    switch (section) {
        case EditSection::Summary: return "Summary";
        case EditSection::Moves: return "Moves";
        case EditSection::DVs: return "DVs";
        case EditSection::StatExp: return "Stat Exp";
        case EditSection::Trainer: return "Trainer";
    }
    return "Editor";
}

int addPageRows(int page) noexcept {
    switch (page) {
        case 0: return 1; // Species
        case 1: return 1; // Level
        case 2: return 1; // Nickname
        case 3: return 4; // Moves
        case 4: return 4; // DVs
        case 5: return 5; // Stat Exp
        case 6: return 2; // OT / TID
        case 7: return 1; // Stage Add
        default: return 1;
    }
}

const char* addPageName(int page) noexcept {
    static constexpr const char* names[] = {
        "Species", "Level", "Nickname", "Moves", "DVs", "Stat Exp", "Trainer", "Review",
    };
    return (page >= 0 && page < 8) ? names[page] : "Add Pokemon";
}

void openActions(TrainerViewScreen& screen) {
    auto& state = stateFor(screen);
    auto* trainer = rbyTrainer(screen);
    if (!trainer || !trainer->stagedPokemonAvailable()) {
        const std::string& why = trainer ? trainer->stagedPokemonUnavailableReason() : std::string{};
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
    state.section = EditSection::Summary;
    state.picker = PickerKind::None;
}

void closeOverlay(OverlayState& state) {
    state.mode = Mode::Closed;
    state.row = 0;
    state.picker = PickerKind::None;
}

void beginAddDraft(TrainerViewScreen& screen) {
    auto& state = stateFor(screen);
    auto* e = editor(screen);
    if (!e) return;
    std::string error;
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
    state.addPage = 0;
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
        if (state.picker == PickerKind::Species) state.draft.species = static_cast<uint16_t>(state.pickerValue);
        else if (state.picker == PickerKind::Move) {
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

bool editField(TrainerViewScreen& screen) {
    auto& state = stateFor(screen);
    auto* e = editor(screen);
    if (!e) return false;
    std::string error;
    auto pokemon = selectedPokemon(screen, error);
    if (!pokemon) {
        screen.postStatus(error.empty() ? "Selected boxed Pokemon is unavailable" : error, 300);
        return false;
    }

    BoxPokemonEdit edit;
    switch (state.section) {
        case EditSection::Summary:
            switch (state.row) {
                case 0:
                    openSpeciesPicker(state, pokemon->species, false);
                    return true;
                case 1: {
                    const auto result = Utils::promptText("Generation I Nickname",
                        "1-10 supported Gen I characters", pokemon->nickname, 10);
                    if (!result.accepted) return false;
                    edit.nickname = result.text;
                    break;
                }
                case 2: {
                    const auto result = Utils::promptNumber("Generation I Level", pokemon->level, 1, 100);
                    if (!result.accepted) return false;
                    edit.level = static_cast<uint8_t>(result.value);
                    break;
                }
                case 3: {
                    const auto result = Utils::promptNumber("Generation I Experience",
                        static_cast<int>(pokemon->experience), 0, 2000000);
                    if (!result.accepted) return false;
                    edit.experience = static_cast<uint32_t>(result.value);
                    break;
                }
            }
            break;
        case EditSection::Moves: {
            const int moveSlot = state.row / 3;
            const int part = state.row % 3;
            if (part == 0) {
                openMovePicker(state, pokemon->moves[moveSlot], moveSlot, false);
                return true;
            }
            if (part == 1) {
                auto pp = pokemon->pp;
                const int maxPP = Editor::moveMaxPP(pokemon->moves[moveSlot], pokemon->ppUps[moveSlot]);
                const auto result = Utils::promptNumber("Generation I Move PP", pp[moveSlot], 0, maxPP);
                if (!result.accepted) return false;
                pp[moveSlot] = static_cast<uint8_t>(result.value);
                edit.pp = pp;
            } else {
                auto ups = pokemon->ppUps;
                const auto result = Utils::promptNumber("Generation I PP Ups", ups[moveSlot], 0, 3);
                if (!result.accepted) return false;
                ups[moveSlot] = static_cast<uint8_t>(result.value);
                edit.ppUps = ups;
                // Clamp current PP if reducing PP Ups lowers the representable maximum.
                auto pp = pokemon->pp;
                pp[moveSlot] = std::min<uint8_t>(pp[moveSlot], Editor::moveMaxPP(pokemon->moves[moveSlot], ups[moveSlot]));
                edit.pp = pp;
            }
            break;
        }
        case EditSection::DVs:
            if (state.row == 4) {
                screen.postStatus("HP DV is derived from Attack/Defense/Speed/Special low bits", 240);
                return false;
            } else {
                auto dvs = storedDVs(*pokemon);
                const auto result = Utils::promptNumber("Generation I DV (0-15)", dvs[state.row], 0, 15);
                if (!result.accepted) return false;
                dvs[state.row] = static_cast<uint8_t>(result.value);
                edit.dvs = dvs;
            }
            break;
        case EditSection::StatExp: {
            auto statExp = pokemon->statExperience;
            const auto result = Utils::promptNumber("Generation I Stat Experience (0-65535)",
                statExp[state.row], 0, 65535);
            if (!result.accepted) return false;
            statExp[state.row] = static_cast<uint16_t>(result.value);
            edit.statExperience = statExp;
            break;
        }
        case EditSection::Trainer:
            if (state.row == 0) {
                const auto result = Utils::promptText("Generation I OT",
                    "1-7 supported Gen I characters", pokemon->originalTrainer, 7);
                if (!result.accepted) return false;
                edit.otName = result.text;
            } else {
                const auto result = Utils::promptNumber("Generation I Trainer ID", pokemon->trainerId, 0, 65535);
                if (!result.accepted) return false;
                edit.trainerId = static_cast<uint16_t>(result.value);
            }
            break;
    }

    if (!e->stageEdit(static_cast<size_t>(state.box), static_cast<size_t>(state.slot), edit, error)) {
        screen.postStatus(error.empty() ? "Generation I edit was rejected" : error, 300);
        return false;
    }
    refreshBoxes(screen);
    screen.postStatus("Pokemon change staged only; original RetroArch .srm unchanged", 240);
    return true;
}

bool editDraftField(TrainerViewScreen& screen) {
    auto& state = stateFor(screen);
    auto* e = editor(screen);
    if (!e) return false;
    switch (state.addPage) {
        case 0:
            openSpeciesPicker(state, state.draft.species, true);
            return true;
        case 1: {
            const auto result = Utils::promptNumber("Add Gen I Pokemon - Level", state.draft.level, 1, 100);
            if (!result.accepted) return false;
            state.draft.level = static_cast<uint8_t>(result.value);
            return true;
        }
        case 2: {
            const auto result = Utils::promptText("Add Gen I Pokemon - Nickname",
                "Blank uses the species name; max 10", state.draft.nickname, 10);
            if (!result.accepted) return false;
            state.draft.nickname = result.text;
            return true;
        }
        case 3:
            openMovePicker(state, state.draft.moves[state.row], state.row, true);
            return true;
        case 4: {
            const auto result = Utils::promptNumber("Add Gen I Pokemon - DV (0-15)", state.draft.dvs[state.row], 0, 15);
            if (!result.accepted) return false;
            state.draft.dvs[state.row] = static_cast<uint8_t>(result.value);
            return true;
        }
        case 5: {
            const auto result = Utils::promptNumber("Add Gen I Pokemon - Stat Experience",
                state.draft.statExperience[state.row], 0, 65535);
            if (!result.accepted) return false;
            state.draft.statExperience[state.row] = static_cast<uint16_t>(result.value);
            return true;
        }
        case 6:
            if (state.row == 0) {
                const std::string initial = state.draft.otName.empty() ? screen.trainer.trainerName : state.draft.otName;
                const auto result = Utils::promptText("Add Gen I Pokemon - OT",
                    "Blank uses source trainer; max 7", initial, 7);
                if (!result.accepted) return false;
                state.draft.otName = result.text;
            } else {
                const int initial = state.draft.trainerId.value_or(screen.trainer.TID16);
                const auto result = Utils::promptNumber("Add Gen I Pokemon - Trainer ID", initial, 0, 65535);
                if (!result.accepted) return false;
                state.draft.trainerId = static_cast<uint16_t>(result.value);
            }
            return true;
        case 7: {
            std::string error;
            if (!e->stageAdd(static_cast<size_t>(state.box), static_cast<size_t>(state.slot), state.draft, error)) {
                screen.postStatus(error.empty() ? "Generation I Add was rejected" : error, 300);
                return false;
            }
            refreshBoxes(screen);
            screen.postStatus("Pokemon staged in empty slot; original RetroArch .srm unchanged", 300);
            state.mode = Mode::Actions;
            state.row = 0;
            return true;
        }
        default:
            return false;
    }
}

bool stageClone(TrainerViewScreen& screen) {
    auto& state = stateFor(screen);
    auto* e = editor(screen);
    if (!e) return false;
    const int dest = firstEmpty(*e, state.cloneBox);
    if (dest < 0) {
        screen.postStatus("Destination Generation I box is full", 300);
        return false;
    }
    std::string error;
    if (!e->stageClone(static_cast<size_t>(state.box), static_cast<size_t>(state.slot),
                       static_cast<size_t>(state.cloneBox), static_cast<size_t>(dest), error)) {
        screen.postStatus(error.empty() ? "Generation I clone was rejected" : error, 300);
        return false;
    }
    refreshBoxes(screen);
    screen.postStatus("Clone staged in an empty slot; source Pokemon and original .srm unchanged", 300);
    state.mode = Mode::Actions;
    state.row = 0;
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
    state.mode = Mode::Closed;
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
    return writeBytes(path, std::span<const uint8_t>(
        reinterpret_cast<const uint8_t*>(text.data()), text.size()));
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
    if (down & HidNpadButton_Left) state.pickerValue = std::max(minValue, state.pickerValue - 10);
    if (down & HidNpadButton_Right) state.pickerValue = std::min(maxValue, state.pickerValue + 10);
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
        const int rows = pokemon ? 7 : 4;
        if (down & HidNpadButton_Up) state.row = (state.row + rows - 1) % rows;
        if (down & HidNpadButton_Down) state.row = (state.row + 1) % rows;
        if (down & HidNpadButton_B) { closeOverlay(state); return; }
        if (!(down & HidNpadButton_A)) return;
        if (pokemon) {
            switch (state.row) {
                case 0: state.mode = Mode::View; state.row = 0; break;
                case 1: state.mode = Mode::Edit; state.section = EditSection::Summary; state.row = 0; break;
                case 2: state.cloneBox = state.box; state.mode = Mode::CloneConfirm; break;
                case 3: state.mode = Mode::RemoveConfirm; break;
                case 4: state.mode = Mode::Provenance; break;
                case 5: state.mode = Mode::Review; state.reviewRow = 0; break;
                default: closeOverlay(state); break;
            }
        } else {
            switch (state.row) {
                case 0: beginAddDraft(screen); break;
                case 1: state.mode = Mode::Review; state.reviewRow = 0; break;
                case 2: state.mode = Mode::Provenance; break;
                default: closeOverlay(state); break;
            }
        }
        return;
    }

    if (state.mode == Mode::View || state.mode == Mode::Provenance) {
        if (down & (HidNpadButton_A | HidNpadButton_B)) { state.mode = Mode::Actions; state.row = 0; }
        return;
    }

    if (state.mode == Mode::Edit) {
        const int rows = editRowCount(state.section);
        if (down & HidNpadButton_Up) state.row = (state.row + rows - 1) % rows;
        if (down & HidNpadButton_Down) state.row = (state.row + 1) % rows;
        if (down & HidNpadButton_Left) {
            int s = static_cast<int>(state.section) - 1;
            if (s < 0) s = 4;
            state.section = static_cast<EditSection>(s); state.row = 0;
        }
        if (down & HidNpadButton_Right) {
            state.section = static_cast<EditSection>((static_cast<int>(state.section) + 1) % 5); state.row = 0;
        }
        if (down & HidNpadButton_B) { state.mode = Mode::Actions; state.row = 0; return; }
        if (down & HidNpadButton_A) editField(screen);
        return;
    }

    if (state.mode == Mode::AddDraft) {
        const int rows = addPageRows(state.addPage);
        if (down & HidNpadButton_Up) state.row = (state.row + rows - 1) % rows;
        if (down & HidNpadButton_Down) state.row = (state.row + 1) % rows;
        if (down & HidNpadButton_Left) { state.addPage = std::max(0, state.addPage - 1); state.row = 0; }
        if (down & HidNpadButton_Right) { state.addPage = std::min(7, state.addPage + 1); state.row = 0; }
        if (down & HidNpadButton_B) {
            // The draft lives only in overlay state. Cancelling here has never called stageAdd().
            state.mode = Mode::Actions; state.row = 0; state.draft = BoxPokemonCreate{}; return;
        }
        if (down & HidNpadButton_A) editDraftField(screen);
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

void drawRow(PKSEFramebuffer& fb, int x, int y, int w,
             const std::string& label, const std::string& value, bool selected) {
    constexpr int h = 46;
    fb.drawFilledRoundedRect(x, y, w, h, 10, selected ? Colors::AccentDim : Colors::PanelAlt);
    if (selected) fb.drawRoundedRect(x, y, w, h, 10, Colors::Accent, 2);
    fb.drawText(x + 16, y + 12, label, Colors::Text, TextStyle::Body);
    int vw = 0, vh = 0;
    fb.measureText(value, vw, vh, TextStyle::Body);
    fb.drawText(x + w - 16 - vw, y + (h - vh) / 2, value,
                selected ? Colors::Text : Colors::TextDim, TextStyle::Body);
}

void drawPicker(TrainerViewScreen& screen, PKSEFramebuffer& fb, int x, int y, int w, int h) {
    auto& state = stateFor(screen);
    const bool species = state.picker == PickerKind::Species;
    fb.drawFilledRoundedRect(x + 80, y + 44, w - 160, h - 88, 16, Colors::Panel);
    fb.drawRoundedRect(x + 80, y + 44, w - 160, h - 88, 16, Colors::Accent, 2);
    fb.drawText(x + 108, y + 64, species ? "Generation I Species" : "Generation I Move",
                Colors::Text, TextStyle::Heading);
    fb.drawText(x + 108, y + 96,
                species ? "Only #001-151 are offered." : "Only move IDs 0-165 are offered; 0 clears a slot.",
                Colors::TextDim, TextStyle::Caption);
    const int minValue = species ? 1 : 0;
    const int maxValue = species ? 151 : 165;
    const int start = std::clamp(state.pickerValue - 4, minValue, std::max(minValue, maxValue - 8));
    int rowY = y + 132;
    for (int value = start; value <= std::min(maxValue, start + 8); ++value) {
        const std::string text = species ? speciesDisplay(static_cast<uint16_t>(value))
                                         : moveDisplay(static_cast<uint8_t>(value));
        drawRow(fb, x + 108, rowY, w - 216, std::to_string(value), text, value == state.pickerValue);
        rowY += 50;
    }
    fb.drawText(x + 108, y + h - 70, "Up/Down 1    Left/Right 10    A Select    B Cancel",
                Colors::TextDim, TextStyle::Caption);
}

void drawActions(TrainerViewScreen& screen, PKSEFramebuffer& fb, int x, int y, int w, int h) {
    auto& state = stateFor(screen);
    std::string error;
    const auto pokemon = selectedPokemon(screen, error);
    fb.drawText(x + 28, y + 20, pokemon ? "Generation I Pokemon Actions" : "Generation I Empty Slot",
                Colors::Text, TextStyle::Heading);
    fb.drawText(x + 28, y + 54,
        "Box " + std::to_string(state.box + 1) + " Slot " + std::to_string(state.slot + 1) +
        " — STAGED COPY ONLY / source .srm READ ONLY", Colors::TextDim, TextStyle::Caption);
    std::vector<std::pair<std::string, std::string>> rows;
    if (pokemon) {
        rows = {
            {"View", pokemon->nickname + " Lv. " + std::to_string(pokemon->level)},
            {"Edit", "Summary / Moves / DVs / Stat Exp / Trainer"},
            {"Clone", "Choose a destination box; uses its first empty slot"},
            {"Remove", "Stages synchronized Gen I box compaction"},
            {"Legality & Provenance", "Format validation; full encounter legality not claimed"},
            {"Review Pending Changes", std::to_string(pendingLines(screen).size()) + " change(s)"},
            {"Cancel", "No mutation"},
        };
    } else {
        rows = {
            {"Add Pokemon", "Draft first; no mutation until explicit Stage Add"},
            {"Review Pending Changes", std::to_string(pendingLines(screen).size()) + " change(s)"},
            {"Legality & Provenance", "International R/B/Y editor capability"},
            {"Cancel", "No mutation"},
        };
    }
    int rowY = y + 92;
    for (int i = 0; i < static_cast<int>(rows.size()); ++i) {
        drawRow(fb, x + 30, rowY, w - 60, rows[i].first, rows[i].second, i == state.row);
        rowY += 53;
    }
    fb.drawText(x + 32, y + h - 38, "A Select    B Cancel    No live RetroArch or installed-game writes",
                Colors::TextDim, TextStyle::Caption);
}

void drawView(TrainerViewScreen& screen, PKSEFramebuffer& fb, int x, int y, int w, int h) {
    std::string error;
    const auto pokemon = selectedPokemon(screen, error);
    fb.drawText(x + 28, y + 20, "Generation I Boxed Pokemon", Colors::Text, TextStyle::Heading);
    if (!pokemon) {
        fb.drawText(x + 32, y + 100, error.empty() ? "Pokemon unavailable" : error, Colors::Text);
        return;
    }
    fb.drawText(x + 28, y + 54, "Native Gen I fields only — no SID, nature, ability, held item, ribbons or modern met data.",
                Colors::TextDim, TextStyle::Caption);
    const std::vector<std::pair<std::string, std::string>> rows{
        {"Species", speciesDisplay(pokemon->species)}, {"Nickname", pokemon->nickname},
        {"Level", std::to_string(pokemon->level)}, {"Experience", std::to_string(pokemon->experience)},
        {"OT", pokemon->originalTrainer}, {"Trainer ID", std::to_string(pokemon->trainerId)},
        {"Attack DV", std::to_string(pokemon->dvs[1])}, {"Defense DV", std::to_string(pokemon->dvs[2])},
        {"Speed DV", std::to_string(pokemon->dvs[3])}, {"Special DV", std::to_string(pokemon->dvs[4])},
        {"HP DV (derived)", std::to_string(pokemon->dvs[0])},
    };
    int rowY = y + 90;
    for (const auto& row : rows) {
        drawRow(fb, x + 30, rowY, w - 60, row.first, row.second, false);
        rowY += 47;
        if (rowY > y + h - 70) break;
    }
    fb.drawText(x + 32, y + h - 38, "A/B Back to Actions", Colors::TextDim, TextStyle::Caption);
}

std::vector<std::pair<std::string, std::string>> editRows(const PokemonRecord& p, EditSection section) {
    switch (section) {
        case EditSection::Summary:
            return {{"Species", speciesDisplay(p.species)}, {"Nickname", p.nickname},
                    {"Level", std::to_string(p.level)}, {"Experience", std::to_string(p.experience)}};
        case EditSection::Moves: {
            std::vector<std::pair<std::string, std::string>> rows;
            for (int i = 0; i < 4; ++i) {
                rows.push_back({"Move " + std::to_string(i + 1), moveDisplay(p.moves[i])});
                rows.push_back({"Move " + std::to_string(i + 1) + " PP", std::to_string(p.pp[i]) + " / " +
                    std::to_string(Editor::moveMaxPP(p.moves[i], p.ppUps[i]))});
                rows.push_back({"Move " + std::to_string(i + 1) + " PP Ups", std::to_string(p.ppUps[i])});
            }
            return rows;
        }
        case EditSection::DVs:
            return {{"Attack DV", std::to_string(p.dvs[1])}, {"Defense DV", std::to_string(p.dvs[2])},
                    {"Speed DV", std::to_string(p.dvs[3])}, {"Special DV", std::to_string(p.dvs[4])},
                    {"HP DV (derived / read-only)", std::to_string(p.dvs[0])}};
        case EditSection::StatExp:
            return {{"HP Stat Exp", std::to_string(p.statExperience[0])},
                    {"Attack Stat Exp", std::to_string(p.statExperience[1])},
                    {"Defense Stat Exp", std::to_string(p.statExperience[2])},
                    {"Speed Stat Exp", std::to_string(p.statExperience[3])},
                    {"Special Stat Exp", std::to_string(p.statExperience[4])}};
        case EditSection::Trainer:
            return {{"OT", p.originalTrainer}, {"Trainer ID", std::to_string(p.trainerId)}};
    }
    return {};
}

void drawEdit(TrainerViewScreen& screen, PKSEFramebuffer& fb, int x, int y, int w, int h) {
    auto& state = stateFor(screen);
    std::string error;
    const auto pokemon = selectedPokemon(screen, error);
    fb.drawText(x + 28, y + 20, "Generation I Staged Pokemon Editor", Colors::Text, TextStyle::Heading);
    fb.drawText(x + 28, y + 54,
        std::string("< ") + sectionName(state.section) + " >    Exact Gen I fields; format-valid does not mean encounter-legal.",
        Colors::TextDim, TextStyle::Caption);
    if (!pokemon) {
        fb.drawText(x + 32, y + 110, error.empty() ? "Pokemon unavailable" : error, Colors::Text);
        return;
    }
    const auto rows = editRows(*pokemon, state.section);
    constexpr int visible = 9;
    const int start = std::clamp(state.row - visible / 2, 0, std::max(0, static_cast<int>(rows.size()) - visible));
    int rowY = y + 92;
    for (int i = start; i < std::min(static_cast<int>(rows.size()), start + visible); ++i) {
        drawRow(fb, x + 30, rowY, w - 60, rows[i].first, rows[i].second, i == state.row);
        rowY += 50;
    }
    fb.drawText(x + 32, y + h - 38,
        "Left/Right Section    A Edit    B Actions    One keyboard prompt maximum per frame",
        Colors::TextDim, TextStyle::Caption);
}

std::vector<std::pair<std::string, std::string>> draftRows(const TrainerViewScreen& screen, const OverlayState& state) {
    const auto& d = state.draft;
    switch (state.addPage) {
        case 0: return {{"Species", speciesDisplay(d.species)}};
        case 1: return {{"Level", std::to_string(d.level)}};
        case 2: return {{"Nickname", d.nickname.empty() ? "(species default)" : d.nickname}};
        case 3: {
            std::vector<std::pair<std::string, std::string>> rows;
            for (int i = 0; i < 4; ++i) rows.push_back({"Move " + std::to_string(i + 1), moveDisplay(d.moves[i])});
            return rows;
        }
        case 4:
            return {{"Attack DV", std::to_string(d.dvs[0])}, {"Defense DV", std::to_string(d.dvs[1])},
                    {"Speed DV", std::to_string(d.dvs[2])}, {"Special DV", std::to_string(d.dvs[3])}};
        case 5:
            return {{"HP Stat Exp", std::to_string(d.statExperience[0])},
                    {"Attack Stat Exp", std::to_string(d.statExperience[1])},
                    {"Defense Stat Exp", std::to_string(d.statExperience[2])},
                    {"Speed Stat Exp", std::to_string(d.statExperience[3])},
                    {"Special Stat Exp", std::to_string(d.statExperience[4])}};
        case 6:
            return {{"OT", d.otName.empty() ? screen.trainer.trainerName + " (source default)" : d.otName},
                    {"Trainer ID", std::to_string(d.trainerId.value_or(screen.trainer.TID16))}};
        case 7:
            return {{"Stage Add", "Box " + std::to_string(state.box + 1) + " Slot " + std::to_string(state.slot + 1)}};
        default: return {};
    }
}

void drawAddDraft(TrainerViewScreen& screen, PKSEFramebuffer& fb, int x, int y, int w, int h) {
    auto& state = stateFor(screen);
    fb.drawText(x + 28, y + 20, "Add Generation I Pokemon — DRAFT", Colors::Text, TextStyle::Heading);
    fb.drawText(x + 28, y + 54,
        std::string("Step ") + std::to_string(state.addPage + 1) + "/8 — " + addPageName(state.addPage) +
        ". Nothing is staged until Review -> Stage Add.", Colors::TextDim, TextStyle::Caption);
    const auto rows = draftRows(screen, state);
    int rowY = y + 112;
    for (int i = 0; i < static_cast<int>(rows.size()); ++i) {
        drawRow(fb, x + 30, rowY, w - 60, rows[i].first, rows[i].second, i == state.row);
        rowY += 58;
    }
    if (state.addPage == 4) {
        const auto hp = Editor::derivedHPDV(state.draft.dvs);
        fb.drawText(x + 32, rowY + 8, "Derived HP DV: " + std::to_string(hp) + " (read-only)", Colors::TextDim);
    }
    if (state.addPage == 7) {
        fb.drawText(x + 32, rowY + 18,
            "A commits one semantic staged addition. B cancels the draft with zero staged mutation.",
            Colors::Accent, TextStyle::Body);
    }
    fb.drawText(x + 32, y + h - 38,
        "Left/Right Step    Up/Down Row    A Edit/Stage    B Cancel entire draft",
        Colors::TextDim, TextStyle::Caption);
}

void drawReview(TrainerViewScreen& screen, PKSEFramebuffer& fb, int x, int y, int w, int h) {
    auto& state = stateFor(screen);
    const auto lines = pendingLines(screen);
    fb.drawText(x + 28, y + 20, "Generation I Pending Changes", Colors::Text, TextStyle::Heading);
    fb.drawText(x + 28, y + 54,
        "A Export edited copy + original backup    Y Discard ALL staged RBY changes    B Back",
        Colors::TextDim, TextStyle::Caption);
    if (lines.empty()) {
        fb.drawText(x + 32, y + 118, "No pending changes.", Colors::TextDim);
    } else {
        constexpr int visible = 8;
        const int start = std::clamp(state.reviewRow - visible / 2, 0,
            std::max(0, static_cast<int>(lines.size()) - visible));
        int rowY = y + 96;
        for (int i = start; i < std::min(static_cast<int>(lines.size()), start + visible); ++i) {
            drawRow(fb, x + 30, rowY, w - 60, std::to_string(i + 1), lines[i], i == state.reviewRow);
            rowY += 50;
        }
    }
    if (!state.lastExportDirectory.empty())
        fb.drawText(x + 32, y + h - 68, "Last export: " + state.lastExportDirectory, Colors::TextDim, TextStyle::Caption);
    fb.drawText(x + 32, y + h - 38,
        "Original RetroArch save is immutable. Export path is PokeBank-owned only.", Colors::Accent, TextStyle::Caption);
}

void drawProvenance(TrainerViewScreen& screen, PKSEFramebuffer& fb, int x, int y, int w, int h) {
    auto* trainer = rbyTrainer(screen);
    auto* e = editor(screen);
    fb.drawText(x + 28, y + 20, "Generation I Legality & Provenance", Colors::Text, TextStyle::Heading);
    fb.drawText(x + 32, y + 92, "Source game", Colors::TextDim);
    fb.drawText(x + 300, y + 92, screen.sourceGameId, Colors::Text);
    fb.drawText(x + 32, y + 132, "Region/layout", Colors::TextDim);
    fb.drawText(x + 300, y + 132, trainer && trainer->japaneseLayout() ? "Japanese (EDIT DISABLED)" : "International (validated editor)", Colors::Text);
    fb.drawText(x + 32, y + 172, "Source mutation", Colors::TextDim);
    fb.drawText(x + 300, y + 172, "HARD DISABLED", Colors::Accent);
    fb.drawText(x + 32, y + 212, "Party mutation", Colors::TextDim);
    fb.drawText(x + 300, y + 212, "DEFERRED", Colors::Text);
    fb.drawText(x + 32, y + 252, "Format validation", Colors::TextDim);
    fb.drawText(x + 300, y + 252, "Strict reparse + R/B/Y identity + checksum on export", Colors::Text);
    fb.drawText(x + 32, y + 292, "Encounter legality", Colors::TextDim);
    fb.drawText(x + 300, y + 292, "NOT CLAIMED by this editor", Colors::Accent);
    fb.drawText(x + 32, y + 332, "Original bytes", Colors::TextDim);
    fb.drawText(x + 300, y + 332, e ? std::to_string(e->originalBytes().size()) + " immutable bytes" : "Unavailable", Colors::Text);
    fb.drawText(x + 32, y + 372, "Pending semantic changes", Colors::TextDim);
    fb.drawText(x + 300, y + 372, std::to_string(pendingLines(screen).size()), Colors::Text);
    fb.drawText(x + 32, y + h - 38, "A/B Back to Actions", Colors::TextDim, TextStyle::Caption);
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
        return true; // block inherited input/touch for every frame while the overlay owns focus
    }

    const bool entrySurface = screen.detailViewActive && screen.selectedMode == TrainerViewScreen::ViewMode::Boxes &&
        screen.selectedItemIndex >= 0 && !screen.helpOverlayActive && !screen.details.active &&
        !screen.actionSheet.isOpen() && !screen.saveConfirmActive && !screen.pickerActive && !screen.itemEditDialogActive;
    if (entrySurface && (down & HidNpadButton_A)) {
        openActions(screen);
        return true;
    }
    return false;
}

void drawOverlay(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    if (!isGen1Source(screen)) return;
    auto& state = stateFor(screen);
    if (state.mode == Mode::Closed) return;
    screen.touchButtons.clear();

    const int w = 900;
    const int h = 620;
    const int x = (fb.getWidth() - w) / 2;
    const int y = 62;
    fb.drawFilledRect(0, 0, fb.getWidth(), fb.getHeight(), Color(0, 0, 0, 110));
    fb.drawSoftShadow(x, y, w, h, 18);
    fb.drawFilledRoundedRect(x, y, w, h, 18, Colors::Panel);
    fb.drawRoundedRect(x, y, w, h, 18, Colors::Accent, 2);

    switch (state.mode) {
        case Mode::Actions: drawActions(screen, fb, x, y, w, h); break;
        case Mode::View: drawView(screen, fb, x, y, w, h); break;
        case Mode::Edit: drawEdit(screen, fb, x, y, w, h); break;
        case Mode::AddDraft: drawAddDraft(screen, fb, x, y, w, h); break;
        case Mode::CloneConfirm: {
            auto* e = editor(screen);
            const int dest = e ? firstEmpty(*e, state.cloneBox) : -1;
            fb.drawText(x + 28, y + 20, "Stage Clone", Colors::Text, TextStyle::Heading);
            fb.drawText(x + 32, y + 110,
                "Destination Box " + std::to_string(state.cloneBox + 1) +
                (dest >= 0 ? " / Slot " + std::to_string(dest + 1) : " / FULL"), Colors::Text, TextStyle::Heading);
            fb.drawText(x + 32, y + 164,
                "The source Pokemon stays untouched. Gen I boxes are contiguous, so clone uses the first empty slot.",
                Colors::TextDim);
            fb.drawText(x + 32, y + 214, "Left/Right Destination Box    A Stage Clone    B Cancel", Colors::Accent);
            break;
        }
        case Mode::RemoveConfirm:
            fb.drawText(x + 28, y + 20, "Stage Removal?", Colors::Text, TextStyle::Heading);
            fb.drawText(x + 32, y + 110,
                "A stages removal and synchronizes count/species/record/OT/nickname parallel arrays.", Colors::Text);
            fb.drawText(x + 32, y + 158,
                "The original RetroArch .srm is not written. B cancels with no mutation.", Colors::TextDim);
            fb.drawText(x + 32, y + 214, "A Stage Remove    B Cancel", Colors::Accent);
            break;
        case Mode::Review: drawReview(screen, fb, x, y, w, h); break;
        case Mode::Provenance: drawProvenance(screen, fb, x, y, w, h); break;
        case Mode::Closed: break;
    }

    if (state.picker != PickerKind::None) drawPicker(screen, fb, x, y, w, h);
}

} // namespace UI::Gen1PokemonEditor
