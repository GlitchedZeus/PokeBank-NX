#include "UI/TrainerViewScreen.h"

#include "Enums/GameVersion.h"
#include "Integration/Gen2/Gen2StagedEditor.h"
#include "Legacy/GSCReadOnlyTrainer.h"
#include "UI/Common.h"
#include "UI/LegacyPresentationRules.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/ScreenChrome.h"
#include "Trainer/Trainer.h"
#include "Utils/FileUtilities.h"
#include "Utils/Keyboard.h"

#include <algorithm>
#include <cerrno>
#include <cstdio>
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
    int selectedRow = 0;
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

    const std::string root = BASE_SAVE_DIRECTORY + "/Exports";
    const std::string gen2Root = root + "/Gen2";
    if (!mkdirIfNeeded(BASE_SAVE_DIRECTORY) || !mkdirIfNeeded(root) || !mkdirIfNeeded(gen2Root)) {
        error = "Could not create PokeVault export directory";
        return {};
    }

    const std::string baseName = screen.sourceGameId + "_" + Utils::getTimestamp();
    std::string exportDir = gen2Root + "/" + baseName;
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
    manifest += "PokeVault NX Generation II staged export\n";
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
    if (screen.selectedMode == TrainerViewScreen::ViewMode::Items) {
        // Jump near the actual pocket the user was viewing: Items -> Potion, Balls -> Poke Ball.
        state.selectedRow = screen.selectedCategory == 3 ? 3 : 2;
    } else {
        state.selectedRow = 0;
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
                screen.postStatus("Exported edited copy + original backup to PokeVault/Exports/Gen2");
            }
        }
        return;
    }

    constexpr int rowCount = 7;
    if (down & HidNpadButton_Up)
        state.selectedRow = (state.selectedRow - 1 + rowCount) % rowCount;
    if (down & HidNpadButton_Down)
        state.selectedRow = (state.selectedRow + 1) % rowCount;
    if (down & HidNpadButton_X) {
        state.review = true;
        return;
    }
    if (down & HidNpadButton_B) {
        if (editor->hasPendingChanges()) {
            state.review = true;
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

    if (state.review) {
        int ry = y + 100;
        if (editor->pendingChanges().empty()) {
            fb.drawText(x + 32, ry, "No pending changes.", Colors::TextDim, TextStyle::Body);
        } else {
            for (const auto& change : editor->pendingChanges()) {
                fb.drawText(x + 32, ry, change.label + ":", Colors::TextDim, TextStyle::Body);
                fb.drawText(x + 250, ry, change.beforeValue + "  ->  " + change.afterValue,
                            Colors::Text, TextStyle::Body);
                ry += 54;
            }
        }
        fb.drawText(x + 32, y + h - 74,
                    "A Export Edited Save    Y Discard Changes    B Back",
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
