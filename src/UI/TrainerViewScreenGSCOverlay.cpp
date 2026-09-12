#include "UI/TrainerViewScreen.h"

#include "Enums/GameVersion.h"
#include "Integration/Gen2/Gen2StagedEditor.h"
#include "Legacy/GSCReadOnlyTrainer.h"
#include "Names/MoveNames.h"
#include "Names/SpeciesNames.h"
#include "UI/Common.h"
#include "UI/LegacyPresentationRules.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/ScreenChrome.h"
#include "Trainer/Trainer.h"
#include "Utils/FileUtilities.h"
#include "Utils/Keyboard.h"
#include "Utils/PokeBankPaths.h"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <array>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <utility>
#include <vector>

namespace UI {
namespace {

using Gen2Editor = PokeVault::Integration::Gen2::StagedEditor;
using PokeVault::Integration::Gen2::InventoryPocket;
using PokeVault::Integration::Gen2::kMasterBallItemId;
using PokeVault::Integration::Gen2::kPokeBallItemId;
using PokeVault::Integration::Gen2::kPotionItemId;

struct Gen2EditorOverlayState {
    const TrainerViewScreen* owner = nullptr;
    bool active = false;
    bool review = false;
    bool pokemonActions = false;
    bool pokemonFields = false;
    int selectedRow = 0;
    int reviewRow = 0;
    int pokemonBox = 0;
    int pokemonSlot = 0;
    int pokemonRow = 0;
    std::string lastExportDirectory;
};

Gen2EditorOverlayState& editorOverlayState(const TrainerViewScreen& screen) {
    static Gen2EditorOverlayState state;
    if (state.owner != &screen) {
        state = {};
        state.owner = &screen;
    }
    return state;
}

bool isGSCSource(const TrainerViewScreen& screen) noexcept {
    return screen.sourceGameId == "gold_gbc" ||
           screen.sourceGameId == "silver_gbc" ||
           screen.sourceGameId == "crystal_gbc";
}

PokeVault::Legacy::GSCReadOnlyTrainer& gscTrainer(TrainerViewScreen& screen) {
    return static_cast<PokeVault::Legacy::GSCReadOnlyTrainer&>(screen.trainer);
}

const PokeVault::Legacy::GSCReadOnlyTrainer& gscTrainer(const TrainerViewScreen& screen) {
    return static_cast<const PokeVault::Legacy::GSCReadOnlyTrainer&>(screen.trainer);
}

Gen2Editor* stagedEditor(TrainerViewScreen& screen) noexcept {
    return gscTrainer(screen).stagedEditor();
}

bool itemCategoryNavigationAvailable(const TrainerViewScreen& screen) noexcept {
    return screen.selectedMode == TrainerViewScreen::ViewMode::Items &&
           !screen.pickerActive && !screen.itemEditDialogActive &&
           !screen.itemRemoveConfirmActive && !screen.statEdit.dialogActive &&
           !screen.saveConfirmActive && !screen.releaseConfirmActive &&
           !screen.storageExitConfirmActive && !screen.groupMenuActive &&
           !screen.creator.keepConfirmActive && !screen.details.active &&
           !screen.details.discardConfirmActive && !screen.gen3ConvertConfirmActive &&
           !screen.lgpeTransferConfirmActive && !screen.actionSheet.isOpen();
}

bool mkdirIfNeeded(const std::string& path) {
    if (::mkdir(path.c_str(), 0777) == 0) return true;
    return errno == EEXIST;
}

bool pathExists(const std::string& path) {
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

std::string exportStagedCopy(TrainerViewScreen& screen, Gen2Editor& editor, std::string& error) {
    error.clear();
    if (!editor.hasPendingChanges()) {
        error = "No pending changes to export";
        return {};
    }

    auto edited = editor.finalizedBytes(error);
    if (edited.empty()) {
        if (error.empty()) error = "Staged save finalization failed";
        return {};
    }

    const std::string gen2Root = PokeBank::Paths::gen2ExportsRoot();
    if (!PokeBank::Paths::ensureGen2ExportsRoot(&error)) {
        if (error.empty()) error = "Could not create PokeBank NX export directory";
        return {};
    }

    const std::string timestamp = Utils::getTimestamp();
    std::string exportDir = PokeBank::Paths::gen2ExportDirectory(screen.sourceGameId, timestamp);
    if (exportDir.empty()) {
        error = "Could not construct safe PokeBank NX export path";
        return {};
    }
    const std::string baseName = exportDir.substr(gen2Root.size() + 1);
    for (int suffix = 2; pathExists(exportDir) && suffix < 1000; ++suffix)
        exportDir = gen2Root + "/" + baseName + "-" + std::to_string(suffix);
    if (pathExists(exportDir) || !mkdirIfNeeded(exportDir)) {
        error = "Could not create unique staged export directory";
        return {};
    }

    const std::string backupPath = exportDir + "/original_backup.srm";
    const std::string editedPath = exportDir + "/edited.srm";
    if (!writeBytes(backupPath, editor.originalBytes())) {
        error = "Could not write automatic original backup";
        return {};
    }
    if (!writeBytes(editedPath, edited)) {
        error = "Could not write staged edited save";
        return {};
    }

    std::string manifest;
    manifest += "PokeBank NX Generation II staged export\n";
    manifest += "GAME_ID=" + screen.sourceGameId + "\n";
    manifest += "SOURCE_PATH=" + screen.backupDir + "\n";
    manifest += "SOURCE_KIND=RetroArchLegacy_READ_ONLY\n";
    manifest += "ORIGINAL_BACKUP=original_backup.srm\n";
    manifest += "EDITED_SAVE=edited.srm\n";
    manifest += "ORIGINAL_BYTES=" + std::to_string(editor.originalBytes().size()) + "\n";
    manifest += "EDITED_BYTES=" + std::to_string(edited.size()) + "\n";
    manifest += "LIVE_RETROARCH_WRITE=DISABLED\n";
    manifest += "LIVE_INSTALLED_GAME_WRITE=DISABLED\n";
    manifest += "PENDING_CHANGES:\n";
    for (const auto& change : editor.pendingChanges())
        manifest += "- " + change.label + ": " + change.beforeValue + " -> " + change.afterValue + "\n";
    if (!writeText(exportDir + "/EDIT_MANIFEST.txt", manifest)) {
        error = "Edited save was written, but provenance manifest creation failed";
        return {};
    }
    return exportDir;
}

void openStagedEditor(TrainerViewScreen& screen) {
    auto& state = editorOverlayState(screen);
    const auto& bridge = gscTrainer(screen);
    if (!bridge.stagedEditingAvailable()) {
        const auto& why = bridge.stagedEditingUnavailableReason();
        screen.postStatus(why.empty() ? "Staged editing is unavailable for this Gen II save" : why);
        return;
    }
    state.active = true;
    state.review = false;
    state.pokemonActions = false;
    state.pokemonFields = false;
    state.reviewRow = 0;
    if (screen.selectedMode == TrainerViewScreen::ViewMode::Items) {
        // Jump near the actual pocket the user was viewing: Items -> Potion, Balls -> Poke Ball.
        state.selectedRow = screen.selectedCategory == 3 ? 3 : 2;
    } else {
        state.selectedRow = 0;
    }
}


std::string gen2ItemDisplay(uint8_t item) {
    if (item == 0) return "None (0)";
    const auto name = PokeVault::Integration::Gen2::gen2ItemName(item);
    return (name.empty() ? std::string("Item") : std::string(name)) + " (" + std::to_string(item) + ")";
}

std::string gen2MoveDisplay(uint8_t move) {
    if (move == 0) return "Empty (0)";
    return std::string(Names::getMoveName(move)) + " (" + std::to_string(move) + ")";
}

std::array<uint8_t, 4> editableDVs(const PokeVault::Integration::Gen2::PokemonRecord& pokemon) {
    return {pokemon.dvs[1], pokemon.dvs[2], pokemon.dvs[3], pokemon.dvs[4]};
}

bool openPokemonActions(TrainerViewScreen& screen) {
    auto* editor = stagedEditor(screen);
    if (!editor) {
        screen.postStatus("Staged Pokemon editor is unavailable; source remains read-only");
        return false;
    }
    if (screen.selectedBoxIndex < 0 || screen.selectedItemIndex < 0 ||
        screen.selectedBoxIndex >= static_cast<int>(editor->metadata().boxCount) ||
        screen.selectedItemIndex >= static_cast<int>(editor->metadata().boxCapacity)) {
        screen.postStatus("Select a Generation II box slot first");
        return false;
    }
    auto& state = editorOverlayState(screen);
    state.active = true;
    state.review = false;
    state.pokemonActions = true;
    state.pokemonFields = false;
    state.pokemonBox = screen.selectedBoxIndex;
    state.pokemonSlot = screen.selectedItemIndex;
    state.pokemonRow = 0;
    state.reviewRow = 0;
    return true;
}

bool stagePokemonFieldEdit(TrainerViewScreen& screen, int row) {
    auto* editor = stagedEditor(screen);
    auto& state = editorOverlayState(screen);
    if (!editor) return false;
    std::string error;
    auto pokemon = editor->boxedPokemon(static_cast<std::size_t>(state.pokemonBox),
                                        static_cast<std::size_t>(state.pokemonSlot), error);
    if (!pokemon) {
        screen.postStatus(error.empty() ? "Selected Gen II box slot is empty" : error);
        return false;
    }

    PokeVault::Integration::Gen2::BoxPokemonEdit edit;
    switch (row) {
        case 0: {
            const auto r = Utils::promptNumber("Gen II Species ID", pokemon->species, 1, 251);
            if (!r.accepted) return false;
            edit.species = static_cast<uint16_t>(r.value);
            break;
        }
        case 1: {
            const auto r = Utils::promptText("Gen II Nickname", "1-10 Gen II characters",
                                             pokemon->nickname, 10);
            if (!r.accepted) return false;
            edit.nickname = r.text;
            break;
        }
        case 2: {
            const auto r = Utils::promptNumber("Gen II Level", pokemon->level, 1, 100);
            if (!r.accepted) return false;
            edit.level = static_cast<uint8_t>(r.value);
            break;
        }
        case 3: {
            const auto r = Utils::promptNumber("Gen II Experience", static_cast<int>(pokemon->experience),
                                               0, 2000000);
            if (!r.accepted) return false;
            edit.experience = static_cast<uint32_t>(r.value);
            break;
        }
        case 4: {
            const auto r = Utils::promptNumber("Gen II Held Item ID (0=None)", pokemon->heldItem, 0, 255);
            if (!r.accepted) return false;
            edit.heldItem = static_cast<uint8_t>(r.value);
            break;
        }
        case 5: case 7: case 9: case 11: {
            const int slot = (row - 5) / 2;
            auto moves = pokemon->moves;
            auto pp = pokemon->pp;
            auto ppUps = pokemon->ppUps;
            const auto r = Utils::promptNumber("Gen II Move ID (0=Empty)", moves[slot], 0, 251);
            if (!r.accepted) return false;
            moves[slot] = static_cast<uint8_t>(r.value);
            ppUps[slot] = 0;
            pp[slot] = moves[slot] == 0 ? 0 : Gen2Editor::gen2MoveBasePP(moves[slot]);
            edit.moves = moves;
            edit.pp = pp;
            edit.ppUps = ppUps;
            break;
        }
        case 6: case 8: case 10: case 12: {
            const int slot = (row - 6) / 2;
            auto pp = pokemon->pp;
            auto ppUps = pokemon->ppUps;
            const auto r = Utils::promptNumber("Gen II Move PP", pp[slot], 0, 63);
            if (!r.accepted) return false;
            const auto ups = Utils::promptNumber("Gen II PP Ups (0-3)", ppUps[slot], 0, 3);
            if (!ups.accepted) return false;
            pp[slot] = static_cast<uint8_t>(r.value);
            ppUps[slot] = static_cast<uint8_t>(ups.value);
            edit.pp = pp;
            edit.ppUps = ppUps;
            break;
        }
        case 13: case 14: case 15: case 16: {
            const int index = row - 13;
            auto dvs = editableDVs(*pokemon);
            const auto r = Utils::promptNumber("Gen II DV (0-15)", dvs[index], 0, 15);
            if (!r.accepted) return false;
            dvs[index] = static_cast<uint8_t>(r.value);
            edit.dvs = dvs;
            break;
        }
        case 17: case 18: case 19: case 20: case 21: {
            const int index = row - 17;
            auto statExperience = pokemon->statExperience;
            const auto r = Utils::promptNumber("Gen II Stat Experience (0-65535)",
                                               statExperience[index], 0, 65535);
            if (!r.accepted) return false;
            statExperience[index] = static_cast<uint16_t>(r.value);
            edit.statExperience = statExperience;
            break;
        }
        case 22: {
            const auto r = Utils::promptText("Gen II OT Name", "1-10 Gen II characters",
                                             pokemon->originalTrainer, 10);
            if (!r.accepted) return false;
            edit.otName = r.text;
            break;
        }
        case 23: {
            const auto r = Utils::promptNumber("Gen II Trainer ID", pokemon->trainerId, 0, 65535);
            if (!r.accepted) return false;
            edit.trainerId = static_cast<uint16_t>(r.value);
            break;
        }
        case 24: {
            const auto r = Utils::promptNumber("Gen II Friendship", pokemon->friendship, 0, 255);
            if (!r.accepted) return false;
            edit.friendship = static_cast<uint8_t>(r.value);
            break;
        }
        case 25: {
            const auto r = Utils::promptNumber("Gen II Pokerus byte", pokemon->pokerus, 0, 255);
            if (!r.accepted) return false;
            edit.pokerus = static_cast<uint8_t>(r.value);
            break;
        }
        case 26: {
            const auto r = Utils::promptNumber("Gen II Caught/Met data", pokemon->caughtData, 0, 65535);
            if (!r.accepted) return false;
            edit.caughtData = static_cast<uint16_t>(r.value);
            break;
        }
        default:
            return false;
    }
    if (!editor->stageBoxPokemonEdit(static_cast<std::size_t>(state.pokemonBox),
                                     static_cast<std::size_t>(state.pokemonSlot), edit, error)) {
        screen.postStatus(error.empty() ? "Gen II Pokemon edit was rejected" : error, 300);
        return false;
    }
    screen.postStatus("Pokemon change staged only; RetroArch source .srm unchanged", 240);
    return true;
}

bool stageAddPokemon(TrainerViewScreen& screen, int box) {
    auto* editor = stagedEditor(screen);
    if (!editor) return false;
    const auto species = Utils::promptNumber("Add Gen II Pokemon - Species ID", 25, 1, 251);
    if (!species.accepted) return false;
    const auto level = Utils::promptNumber("Add Gen II Pokemon - Level", 5, 1, 100);
    if (!level.accepted) return false;
    const auto nickname = Utils::promptText("Add Gen II Pokemon - Nickname",
                                            "Blank uses the species name; max 10 Gen II characters", "", 10);
    if (!nickname.accepted) return false;
    const auto held = Utils::promptNumber("Add Gen II Pokemon - Held Item ID", 0, 0, 255);
    if (!held.accepted) return false;

    PokeVault::Integration::Gen2::BoxPokemonCreate create;
    create.species = static_cast<uint16_t>(species.value);
    create.level = static_cast<uint8_t>(level.value);
    create.nickname = nickname.text;
    create.heldItem = static_cast<uint8_t>(held.value);
    for (int i = 0; i < 4; ++i) {
        const auto move = Utils::promptNumber("Add Gen II Pokemon - Move " + std::to_string(i + 1) + " ID",
                                              0, 0, 251);
        if (!move.accepted) return false;
        create.moves[i] = static_cast<uint8_t>(move.value);
    }
    for (int i = 0; i < 4; ++i) {
        const auto dv = Utils::promptNumber("Add Gen II Pokemon - " +
            std::string(i == 0 ? "Attack" : i == 1 ? "Defense" : i == 2 ? "Speed" : "Special") + " DV",
            8, 0, 15);
        if (!dv.accepted) return false;
        create.dvs[i] = static_cast<uint8_t>(dv.value);
    }
    const auto friendship = Utils::promptNumber("Add Gen II Pokemon - Friendship", 70, 0, 255);
    if (!friendship.accepted) return false;
    create.friendship = static_cast<uint8_t>(friendship.value);
    const auto caught = Utils::promptNumber("Add Gen II Pokemon - Caught/Met data", 0, 0, 65535);
    if (!caught.accepted) return false;
    create.caughtData = static_cast<uint16_t>(caught.value);

    std::size_t slot = 0;
    std::string error;
    if (!editor->stageAddBoxPokemon(static_cast<std::size_t>(box), create, slot, error)) {
        screen.postStatus(error.empty() ? "Could not add Gen II Pokemon" : error, 300);
        return false;
    }
    auto& state = editorOverlayState(screen);
    state.pokemonBox = box;
    state.pokemonSlot = static_cast<int>(slot);
    screen.postStatus("Added Pokemon to staged Box " + std::to_string(box + 1) +
                      " Slot " + std::to_string(slot + 1) + "; source unchanged", 300);
    return true;
}

void handlePokemonFieldInput(TrainerViewScreen& screen, u64 down) {
    auto& state = editorOverlayState(screen);
    constexpr int rowCount = 28;
    if (down & HidNpadButton_Up) state.pokemonRow = (state.pokemonRow - 1 + rowCount) % rowCount;
    if (down & HidNpadButton_Down) state.pokemonRow = (state.pokemonRow + 1) % rowCount;
    if (down & HidNpadButton_B) {
        state.pokemonFields = false;
        return;
    }
    if (!(down & HidNpadButton_A)) return;
    if (state.pokemonRow == rowCount - 1) {
        state.pokemonFields = false;
        return;
    }
    stagePokemonFieldEdit(screen, state.pokemonRow);
}

void handlePokemonActionInput(TrainerViewScreen& screen, u64 down) {
    auto& state = editorOverlayState(screen);
    auto* editor = stagedEditor(screen);
    if (!editor) { state = {}; state.owner = &screen; return; }
    std::string error;
    const auto pokemon = editor->boxedPokemon(static_cast<std::size_t>(state.pokemonBox),
                                              static_cast<std::size_t>(state.pokemonSlot), error);
    const bool occupied = pokemon.has_value();
    const int rowCount = occupied ? 6 : 3;
    if (down & HidNpadButton_Up) state.pokemonRow = (state.pokemonRow - 1 + rowCount) % rowCount;
    if (down & HidNpadButton_Down) state.pokemonRow = (state.pokemonRow + 1) % rowCount;
    if (down & HidNpadButton_X) { state.review = true; state.reviewRow = 0; return; }
    if (down & HidNpadButton_B) {
        if (editor->hasPendingChanges()) { state.review = true; state.reviewRow = 0; }
        else { state.active = false; state.pokemonActions = false; }
        return;
    }
    if (!(down & HidNpadButton_A)) return;

    if (!occupied) {
        if (state.pokemonRow == 0) {
            if (stageAddPokemon(screen, state.pokemonBox)) state.pokemonRow = 0;
        } else if (state.pokemonRow == 1) {
            state.review = true; state.reviewRow = 0;
        } else {
            state.active = false; state.pokemonActions = false;
        }
        return;
    }

    switch (state.pokemonRow) {
        case 0:
            state.pokemonFields = true;
            state.pokemonRow = 0;
            break;
        case 1: {
            const auto destination = Utils::promptNumber("Clone to Gen II Box", state.pokemonBox + 1, 1,
                                                         editor->metadata().boxCount);
            if (!destination.accepted) break;
            std::size_t destinationSlot = 0;
            if (!editor->stageCloneBoxPokemon(static_cast<std::size_t>(state.pokemonBox),
                                              static_cast<std::size_t>(state.pokemonSlot),
                                              static_cast<std::size_t>(destination.value - 1),
                                              destinationSlot, error))
                screen.postStatus(error.empty() ? "Clone failed" : error, 300);
            else
                screen.postStatus("Clone staged in Box " + std::to_string(destination.value) +
                                  " Slot " + std::to_string(destinationSlot + 1) + "; source unchanged", 300);
            break;
        }
        case 2: {
            if (!editor->stageBoxPokemonShiny(static_cast<std::size_t>(state.pokemonBox),
                                              static_cast<std::size_t>(state.pokemonSlot),
                                              !pokemon->shiny, error))
                screen.postStatus(error.empty() ? "Shiny toggle failed" : error, 360);
            else
                screen.postStatus(pokemon->shiny ? "Non-shiny DV state staged; source unchanged"
                                                 : "Legal Gen II shiny DV state staged; source unchanged", 300);
            break;
        }
        case 3:
            stageAddPokemon(screen, state.pokemonBox);
            break;
        case 4:
            state.review = true; state.reviewRow = 0;
            break;
        case 5:
            state.active = false; state.pokemonActions = false;
            break;
        default:
            break;
    }
}

void handleStagedEditorInput(TrainerViewScreen& screen, u64 down) {
    auto& state = editorOverlayState(screen);
    auto* editor = stagedEditor(screen);
    if (!editor) {
        state.active = false;
        state.review = false;
        screen.postStatus("Staged editor became unavailable; source remains read-only");
        return;
    }

    if (state.review) {
        const int changeCount = static_cast<int>(editor->pendingChanges().size());
        if (changeCount > 0 && (down & HidNpadButton_Up))
            state.reviewRow = (state.reviewRow - 1 + changeCount) % changeCount;
        if (changeCount > 0 && (down & HidNpadButton_Down))
            state.reviewRow = (state.reviewRow + 1) % changeCount;
        if (down & HidNpadButton_B) {
            state.review = false;
            return;
        }
        if (down & HidNpadButton_Y) {
            editor->discard();
            state.review = false;
            screen.postStatus("Staged changes discarded; original save was never modified");
            return;
        }
        if (down & HidNpadButton_A) {
            std::string error;
            const std::string exportDir = exportStagedCopy(screen, *editor, error);
            if (exportDir.empty()) {
                screen.postStatus(error.empty() ? "Staged export failed" : error);
            } else {
                state.lastExportDirectory = exportDir;
                screen.postStatus("Exported edited copy + original backup to PokeBank NX exports/gen2");
            }
        }
        return;
    }

    if (state.pokemonFields) {
        handlePokemonFieldInput(screen, down);
        return;
    }
    if (state.pokemonActions) {
        handlePokemonActionInput(screen, down);
        return;
    }

    constexpr int rowCount = 7;
    if (down & HidNpadButton_Up)
        state.selectedRow = (state.selectedRow - 1 + rowCount) % rowCount;
    if (down & HidNpadButton_Down)
        state.selectedRow = (state.selectedRow + 1) % rowCount;
    if (down & HidNpadButton_X) {
        state.review = true;
        state.reviewRow = 0;
        return;
    }
    if (down & HidNpadButton_B) {
        if (editor->hasPendingChanges()) {
            state.review = true;
            state.reviewRow = 0;
            screen.postStatus("Review, export, or discard staged changes before closing");
        } else {
            state.active = false;
        }
        return;
    }
    if (!(down & HidNpadButton_A)) return;

    std::string error;
    switch (state.selectedRow) {
        case 0: {
            const auto result = Utils::promptText(
                "Generation II Trainer Name", "1-7 supported Gen II characters",
                editor->trainerName(), static_cast<int>(PokeVault::Integration::Gen2::kInternationalTrainerNameMax));
            if (result.accepted && !editor->stageTrainerName(result.text, error))
                screen.postStatus(error);
            else if (result.accepted)
                screen.postStatus("Trainer name staged; source .srm unchanged");
            break;
        }
        case 1: {
            const auto result = Utils::promptNumber(
                "Generation II Money", static_cast<int>(editor->money()), 0,
                static_cast<int>(PokeVault::Integration::Gen2::kMaxMoney));
            if (result.accepted && !editor->stageMoney(static_cast<uint32_t>(result.value), error))
                screen.postStatus(error);
            else if (result.accepted)
                screen.postStatus("Money staged; source .srm unchanged");
            break;
        }
        case 2: {
            const int current = editor->itemQuantity(InventoryPocket::Items, kPotionItemId);
            const auto result = Utils::promptNumber("Potion quantity (0 removes)", current, 0, 99);
            if (result.accepted && !editor->stageItemQuantity(
                    InventoryPocket::Items, kPotionItemId, static_cast<uint8_t>(result.value), error))
                screen.postStatus(error);
            else if (result.accepted)
                screen.postStatus("Potion quantity staged; source .srm unchanged");
            break;
        }
        case 3: {
            const int current = editor->itemQuantity(InventoryPocket::Balls, kPokeBallItemId);
            const auto result = Utils::promptNumber("Poke Ball quantity (0 removes)", current, 0, 99);
            if (result.accepted && !editor->stageItemQuantity(
                    InventoryPocket::Balls, kPokeBallItemId, static_cast<uint8_t>(result.value), error))
                screen.postStatus(error);
            else if (result.accepted)
                screen.postStatus("Poke Ball quantity staged; source .srm unchanged");
            break;
        }
        case 4: {
            const int current = editor->itemQuantity(InventoryPocket::Balls, kMasterBallItemId);
            const auto result = Utils::promptNumber("Master Ball quantity (0 removes)", current, 0, 99);
            if (result.accepted && !editor->stageItemQuantity(
                    InventoryPocket::Balls, kMasterBallItemId, static_cast<uint8_t>(result.value), error))
                screen.postStatus(error);
            else if (result.accepted)
                screen.postStatus("Master Ball quantity staged; source .srm unchanged");
            break;
        }
        case 5:
            state.review = true;
            state.reviewRow = 0;
            break;
        case 6:
            editor->discard();
            screen.postStatus("Staged changes discarded; original save was never modified");
            break;
        default:
            break;
    }
}

void drawGSCTrainerCard(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    constexpr int x = 12;
    constexpr int y = 80;
    constexpr int h = 560;
    constexpr int headerH = 46;
    const int w = fb.getWidth() - x;

    // The accepted base screen drew the later-generation Trainer card first. Replace only the content
    // panel for GSC so no SID, full modern IDs, account/profile data, or unsupported metadata can remain
    // visible. The title bar and existing navigation footer stay untouched.
    fb.drawFilledRoundedRect(x, y, w, h, 16, Colors::Panel);
    fb.drawRoundedRect(x, y, w, h, 16, Colors::Border, 1);
    fb.drawFilledRoundedRect(x, y, w, headerH, 16, Colors::AccentDim);
    fb.drawFilledRect(x, y + headerH - 16, w, 16, Colors::AccentDim);
    fb.drawText(x + 22, y + (headerH - fb.lineHeight(TextStyle::Heading)) / 2,
                "Trainer — Generation II / Source Read Only", Colors::Text, TextStyle::Heading);

    screen.touchButtons.clear();

    const int cardW = 680;
    const int cardX = x + (w - cardW) / 2;
    int rowY = y + headerH + 36;
    constexpr int rowH = 56;
    constexpr int rowGap = 14;

    auto row = [&](const char* label, const std::string& value) {
        fb.drawSoftShadow(cardX, rowY, cardW, rowH, 14);
        fb.drawFilledRoundedRect(cardX, rowY, cardW, rowH, 14, Colors::PanelAlt);
        fb.drawText(cardX + 24, rowY + (rowH - fb.lineHeight(TextStyle::Body)) / 2,
                    label, Colors::TextDim, TextStyle::Body);
        int vw = 0, vh = 0;
        fb.measureText(value, vw, vh, TextStyle::Body);
        fb.drawText(cardX + cardW - 24 - vw, rowY + (rowH - vh) / 2,
                    value, Colors::Text, TextStyle::Body);
        rowY += rowH + rowGap;
    };

    const Trainer::Trainer& trainer = screen.trainer;
    row("Name", trainer.trainerName.empty() ? std::string("(none)") : trainer.trainerName);
    row("Money", "$" + std::to_string(trainer.money));
    row("Trainer ID", std::to_string(trainer.TID16));

    // The Gen II trainer bridge guarantees a semantic gender value for validated GSC sources:
    // Gold/Silver resolve to their fixed male player character, while Crystal is save-derived.
    if (trainer.trainerGender <= 1)
        row("Gender", trainer.trainerGender == 0 ? "Male" : "Female");

    rowY += 8;
    const char* note = screen.sourceGameId == "crystal_gbc"
        ? "Crystal gender is shown only from the validated Crystal save field."
        : "Gold/Silver use the games' fixed male player character.";
    fb.drawText(cardX + 12, rowY, note, Colors::TextDim, TextStyle::Caption);
    fb.drawText(cardX + 12, rowY + 28,
                "No SID is stored. X opens staged editing; the source .srm remains read-only.",
                Colors::TextDim, TextStyle::Caption);

    // The accepted base draws transient status before this overlay; redraw it so a read-only-action
    // refusal is not hidden by the corrected card.
    if (screen.storageStatusFrames > 0 && !screen.storageStatus.empty()) {
        int tw = 0, th = 0;
        fb.measureText(screen.storageStatus, tw, th);
        const int padX = 18, bw = tw + padX * 2, bh = th + 14;
        const int footerY = fb.getHeight() - kNavBarH;
        const int bx = (fb.getWidth() - bw) / 2, by = footerY - bh - 12;
        fb.drawFilledRoundedRect(bx, by, bw, bh, 8, Colors::Panel);
        fb.drawRoundedRect(bx, by, bw, bh, 8, Colors::Accent, 2);
        fb.drawText(bx + padX, by + 7, screen.storageStatus, Colors::Text);
    }
}

void drawEditorRow(PKSEFramebuffer& fb, int x, int y, int w, const std::string& label,
                   const std::string& value, bool selected) {
    constexpr int h = 48;
    fb.drawFilledRoundedRect(x, y, w, h, 10, selected ? Colors::AccentDim : Colors::PanelAlt);
    if (selected) fb.drawRoundedRect(x, y, w, h, 10, Colors::Accent, 2);
    fb.drawText(x + 18, y + 13, label, Colors::Text, TextStyle::Body);
    int tw = 0, th = 0;
    fb.measureText(value, tw, th, TextStyle::Body);
    fb.drawText(x + w - 18 - tw, y + (h - th) / 2, value,
                selected ? Colors::Text : Colors::TextDim, TextStyle::Body);
}

void drawStagedEditor(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    auto& state = editorOverlayState(screen);
    auto* editor = stagedEditor(screen);
    if (!state.active || !editor) return;

    constexpr int w = 860;
    constexpr int h = 610;
    const int x = (fb.getWidth() - w) / 2;
    const int y = 66;
    fb.drawSoftShadow(x, y, w, h, 18);
    fb.drawFilledRoundedRect(x, y, w, h, 18, Colors::Panel);
    fb.drawRoundedRect(x, y, w, h, 18, Colors::Accent, 2);

    fb.drawText(x + 28, y + 20, state.review ? "Pending Changes" : "Generation II Staged Editor",
                Colors::Text, TextStyle::Heading);
    fb.drawText(x + 28, y + 54,
                "RetroArch source is read-only. Export creates a separate edited copy + original backup.",
                Colors::TextDim, TextStyle::Caption);

    if (state.pokemonFields && !state.review) {
        std::string error;
        auto pokemon = editor->boxedPokemon(static_cast<std::size_t>(state.pokemonBox),
                                            static_cast<std::size_t>(state.pokemonSlot), error);
        fb.drawText(x + 28, y + 20, "Generation II Pokemon Editor", Colors::Text, TextStyle::Heading);
        fb.drawText(x + 28, y + 54,
                    "Staged copy only. DVs / Stat Experience are real Gen II fields. FULL legality is not claimed.",
                    Colors::TextDim, TextStyle::Caption);
        if (!pokemon) {
            fb.drawText(x + 32, y + 110, error.empty() ? "Pokemon is unavailable." : error,
                        Colors::Text, TextStyle::Body);
            return;
        }
        const std::array<std::pair<std::string, std::string>, 28> rows{{
            {"Species", std::string(Names::getSpeciesName(pokemon->species)) + " (" + std::to_string(pokemon->species) + ")"},
            {"Nickname", pokemon->nickname},
            {"Level", std::to_string(pokemon->level)},
            {"Experience", std::to_string(pokemon->experience)},
            {"Held Item", gen2ItemDisplay(pokemon->heldItem)},
            {"Move 1", gen2MoveDisplay(pokemon->moves[0])}, {"Move 1 PP", std::to_string(pokemon->pp[0]) + " / PP Ups " + std::to_string(pokemon->ppUps[0])},
            {"Move 2", gen2MoveDisplay(pokemon->moves[1])}, {"Move 2 PP", std::to_string(pokemon->pp[1]) + " / PP Ups " + std::to_string(pokemon->ppUps[1])},
            {"Move 3", gen2MoveDisplay(pokemon->moves[2])}, {"Move 3 PP", std::to_string(pokemon->pp[2]) + " / PP Ups " + std::to_string(pokemon->ppUps[2])},
            {"Move 4", gen2MoveDisplay(pokemon->moves[3])}, {"Move 4 PP", std::to_string(pokemon->pp[3]) + " / PP Ups " + std::to_string(pokemon->ppUps[3])},
            {"Attack DV", std::to_string(pokemon->dvs[1])},
            {"Defense DV", std::to_string(pokemon->dvs[2])},
            {"Speed DV", std::to_string(pokemon->dvs[3])},
            {"Special DV", std::to_string(pokemon->dvs[4])},
            {"HP Stat Experience", std::to_string(pokemon->statExperience[0])},
            {"Attack Stat Experience", std::to_string(pokemon->statExperience[1])},
            {"Defense Stat Experience", std::to_string(pokemon->statExperience[2])},
            {"Speed Stat Experience", std::to_string(pokemon->statExperience[3])},
            {"Special Stat Experience", std::to_string(pokemon->statExperience[4])},
            {"OT Name", pokemon->originalTrainer},
            {"Trainer ID", std::to_string(pokemon->trainerId)},
            {"Friendship", std::to_string(pokemon->friendship)},
            {"Pokerus", std::to_string(pokemon->pokerus)},
            {"Caught/Met Data", std::to_string(pokemon->caughtData)},
            {"Back", "HP DV " + std::to_string(pokemon->dvs[0]) +
                     " (derived) / Shiny " + std::string(pokemon->shiny ? "Yes" : "No")},
        }};
        constexpr int visible = 8;
        const int start = std::clamp(state.pokemonRow - visible / 2, 0,
                                     static_cast<int>(rows.size()) - visible);
        int rowY = y + 92;
        for (int i = start; i < start + visible; ++i) {
            drawEditorRow(fb, x + 30, rowY, w - 60, rows[i].first, rows[i].second,
                          i == state.pokemonRow);
            rowY += 57;
        }
        fb.drawText(x + 32, y + h - 40,
                    "A Edit    B Pokemon Actions    HP DV is derived; shiny/gender follow Gen II DVs",
                    Colors::TextDim, TextStyle::Caption);
        return;
    }

    if (state.pokemonActions && !state.review) {
        std::string error;
        const auto pokemon = editor->boxedPokemon(static_cast<std::size_t>(state.pokemonBox),
                                                  static_cast<std::size_t>(state.pokemonSlot), error);
        fb.drawText(x + 28, y + 20, "Generation II Pokemon Actions", Colors::Text, TextStyle::Heading);
        fb.drawText(x + 28, y + 54,
                    "Box " + std::to_string(state.pokemonBox + 1) + " Slot " +
                    std::to_string(state.pokemonSlot + 1) + " — source .srm stays READ ONLY",
                    Colors::TextDim, TextStyle::Caption);
        std::vector<std::pair<std::string, std::string>> rows;
        if (pokemon) {
            rows = {
                {"Edit", pokemon->nickname + " Lv. " + std::to_string(pokemon->level)},
                {"Clone", "Append to an empty box slot"},
                {pokemon->shiny ? "Make Non-Shiny" : "Make Shiny", "Real Gen II DV rules"},
                {"Add Pokemon", "Append to this box"},
                {"Review Pending Changes", std::to_string(editor->pendingChanges().size()) + " change(s)"},
                {"Close", "No source write"},
            };
        } else {
            rows = {
                {"Add Pokemon", "Append to this box"},
                {"Review Pending Changes", std::to_string(editor->pendingChanges().size()) + " change(s)"},
                {"Close", "No source write"},
            };
        }
        int rowY = y + 112;
        for (int i = 0; i < static_cast<int>(rows.size()); ++i) {
            drawEditorRow(fb, x + 30, rowY, w - 60, rows[i].first, rows[i].second,
                          i == state.pokemonRow);
            rowY += 57;
        }
        fb.drawText(x + 32, y + h - 40,
                    "A Select    X Review    B Close/Review    FORMAT VALID != full encounter legality",
                    Colors::TextDim, TextStyle::Caption);
        return;
    }

    if (state.review) {
        int ry = y + 100;
        if (editor->pendingChanges().empty()) {
            fb.drawText(x + 32, ry, "No pending changes.", Colors::TextDim, TextStyle::Body);
        } else {
            constexpr int visible = 7;
            const int count = static_cast<int>(editor->pendingChanges().size());
            const int start = std::clamp(state.reviewRow - visible / 2, 0, std::max(0, count - visible));
            const int end = std::min(count, start + visible);
            for (int i = start; i < end; ++i) {
                const auto& change = editor->pendingChanges()[static_cast<std::size_t>(i)];
                fb.drawText(x + 32, ry, change.label + ":", i == state.reviewRow ? Colors::Accent : Colors::TextDim, TextStyle::Body);
                fb.drawText(x + 320, ry, change.beforeValue + "  ->  " + change.afterValue,
                            Colors::Text, TextStyle::Body);
                ry += 54;
            }
        }
        fb.drawText(x + 32, y + h - 74,
                    "Up/Down Browse    A Export Edited Save    Y Discard Changes    B Back",
                    Colors::Text, TextStyle::Body);
        if (!state.lastExportDirectory.empty())
            fb.drawText(x + 32, y + h - 42, "Last export: " + state.lastExportDirectory,
                        Colors::TextDim, TextStyle::Caption);
        return;
    }

    const std::vector<std::pair<std::string, std::string>> rows{
        {"Trainer Name", editor->trainerName()},
        {"Money", "$" + std::to_string(editor->money())},
        {"Potion", std::to_string(editor->itemQuantity(InventoryPocket::Items, kPotionItemId))},
        {"Poke Ball", std::to_string(editor->itemQuantity(InventoryPocket::Balls, kPokeBallItemId))},
        {"Master Ball", std::to_string(editor->itemQuantity(InventoryPocket::Balls, kMasterBallItemId))},
        {"Review Pending Changes", std::to_string(editor->pendingChanges().size()) + " change(s)"},
        {"Discard Staged Changes", editor->hasPendingChanges() ? "Available" : "Nothing staged"},
    };
    int rowY = y + 92;
    for (int i = 0; i < static_cast<int>(rows.size()); ++i) {
        drawEditorRow(fb, x + 30, rowY, w - 60, rows[i].first, rows[i].second,
                      i == state.selectedRow);
        rowY += 57;
    }
    fb.drawText(x + 32, y + h - 40,
                "A Edit / Select    X Review    B Close (review required if changes are pending)",
                Colors::TextDim, TextStyle::Caption);
}

} // namespace

void TrainerViewScreen::update(const PadState& pad, const TouchInput& touch) {
    const bool validatedGSC = trainer.getGameGroup() == Enums::GameVersion::GSC && isGSCSource(*this);
    if (validatedGSC) {
        auto& overlay = editorOverlayState(*this);
        const u64 down = padGetButtonsDown(&pad);
        if (overlay.active) {
            handleStagedEditorInput(*this, down);
            return;
        }

        const bool pokemonEntrySurface = detailViewActive && selectedMode == ViewMode::Boxes &&
            selectedItemIndex >= 0 && !helpOverlayActive && !details.active && !actionSheet.isOpen() &&
            !saveConfirmActive && !pickerActive && !itemEditDialogActive;
        if (pokemonEntrySurface && (down & HidNpadButton_X)) {
            openPokemonActions(*this);
            return;
        }

        const bool editorEntrySurface = detailViewActive &&
            (selectedMode == ViewMode::Trainer || selectedMode == ViewMode::Items) &&
            !helpOverlayActive && !details.active && !actionSheet.isOpen() &&
            !saveConfirmActive && !pickerActive && !itemEditDialogActive;
        if (editorEntrySurface && (down & HidNpadButton_X)) {
            openStagedEditor(*this);
            return; // never let the inherited save/edit handlers interpret this X press
        }
    }

    // The accepted large implementation has no GSC switch arm, so its L/R item-tab handling is a
    // deliberate no-op for group 70. Handle the five real Gen II pockets here, then delegate every
    // other input path unchanged. This preserves RBY=2 and every existing Gen III+ pouch count.
    if (validatedGSC && itemCategoryNavigationAvailable(*this)) {
        const u64 down = padGetButtonsDown(&pad);
        const int count = legacyInventoryCategoryCount(Enums::GameVersion::GSC);
        if (count > 0 && (down & HidNpadButton_L)) {
            selectedCategory = (selectedCategory - 1 + count) % count;
            currentPage = 0;
            selectedItemIndex = 0;
        }
        if (count > 0 && (down & HidNpadButton_R)) {
            selectedCategory = (selectedCategory + 1) % count;
            currentPage = 0;
            selectedItemIndex = 0;
        }
    }

    updateLegacyBase(pad, touch);

    // The inherited read-only summary input knows about the modern Ribbons/Legality overlays and can
    // set those flags from Y/R before it reaches the read-only edit guard. The dedicated Gen II modal
    // intentionally has neither surface, so retaining either flag would create an invisible blocking
    // overlay on the next frame. Clear them only for validated GSC sources after base input handling;
    // accepted RBY and Gen III+ behavior is untouched.
    if (validatedGSC) {
        details.ribbonOverlay = false;
        details.legalityOverlay = false;
    }
}

void TrainerViewScreen::draw(PKSEFramebuffer& fb) {
    drawLegacyBase(fb);

    if (trainer.getGameGroup() == Enums::GameVersion::GSC && isGSCSource(*this)) {
        if (detailViewActive && selectedMode == ViewMode::Trainer && !helpOverlayActive)
            drawGSCTrainerCard(*this, fb);
        drawStagedEditor(*this, fb);
    }
}

} // namespace UI
