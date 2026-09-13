#include "UI/ClassicInventoryOverlay.h"

#include "Integration/Gen1/Gen1StagedInventoryEditor.h"
#include "Integration/Gen2/Gen2StagedEditor.h"
#include "Integration/Gen3/Gen3StagedInventoryEditor.h"
#include "Inventory/ClassicInventoryCatalog.h"
#include "Legacy/FRLGReadOnlyTrainer.h"
#include "Legacy/GSCReadOnlyTrainer.h"
#include "Legacy/RBYReadOnlyTrainer.h"
#include "Trainer/Trainer.h"
#include "UI/ClassicInventoryUIModel.h"
#include "UI/Common.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/TrainerViewScreen.h"
#include "Utils/FileUtilities.h"
#include "Utils/Keyboard.h"
#include "Utils/PokeBankPaths.h"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <sys/stat.h>
#include <utility>
#include <vector>

namespace UI::ClassicInventory {
namespace {

using PokeVault::Inventory::ClassicGame;
using PokeVault::Inventory::ClassicPocket;
using Gen2Pocket = PokeVault::Integration::Gen2::InventoryPocket;
using Gen3Pouch = PokeVault::Integration::Gen3::InventoryPouch;

struct Row {
    uint16_t itemId = 0;
    uint16_t quantity = 0;
};

enum class SourceKind : uint8_t { None, Gen1, Gen2, Gen3 };

struct OverlayState {
    const TrainerViewScreen* owner = nullptr;
    bool pickerActive = false;
    bool reviewActive = false;
    bool warningActive = false;
    int pickerRow = 0;
    int reviewRow = 0;
    ClassicPocket pendingPocket = ClassicPocket::Items;
    uint16_t pendingItem = 0;
    uint16_t pendingQuantity = 0;
    std::vector<uint16_t> pickerItems;
    std::string lastExportDirectory;
};

OverlayState& stateFor(const TrainerViewScreen& screen) {
    static OverlayState state;
    if (state.owner != &screen) {
        state = {};
        state.owner = &screen;
    }
    return state;
}

SourceKind sourceKind(std::string_view id) noexcept {
    if (id == "red_gb" || id == "blue_gb" || id == "yellow_gb") return SourceKind::Gen1;
    if (id == "gold_gbc" || id == "silver_gbc" || id == "crystal_gbc") return SourceKind::Gen2;
    if (id == "ruby_gba" || id == "sapphire_gba" || id == "emerald_gba" ||
        id == "firered_gba" || id == "leafgreen_gba") return SourceKind::Gen3;
    return SourceKind::None;
}

std::optional<ClassicGame> exactGame(const TrainerViewScreen& screen) noexcept {
    return PokeBank::UIModel::classicInventoryGame(screen.sourceGameId);
}

PokeVault::Legacy::RBYReadOnlyTrainer* rbyTrainer(TrainerViewScreen& screen) noexcept {
    return sourceKind(screen.sourceGameId) == SourceKind::Gen1
        ? static_cast<PokeVault::Legacy::RBYReadOnlyTrainer*>(&screen.trainer) : nullptr;
}

PokeVault::Legacy::GSCReadOnlyTrainer* gscTrainer(TrainerViewScreen& screen) noexcept {
    return sourceKind(screen.sourceGameId) == SourceKind::Gen2
        ? static_cast<PokeVault::Legacy::GSCReadOnlyTrainer*>(&screen.trainer) : nullptr;
}

PokeVault::Legacy::FRLGReadOnlyTrainer* gen3Trainer(TrainerViewScreen& screen) noexcept {
    return sourceKind(screen.sourceGameId) == SourceKind::Gen3
        ? static_cast<PokeVault::Legacy::FRLGReadOnlyTrainer*>(&screen.trainer) : nullptr;
}

std::optional<Gen2Pocket> gen2Pocket(ClassicPocket pocket) noexcept {
    switch (pocket) {
        case ClassicPocket::Items: return Gen2Pocket::Items;
        case ClassicPocket::Balls: return Gen2Pocket::Balls;
        case ClassicPocket::KeyItems: return Gen2Pocket::KeyItems;
        case ClassicPocket::TMHM: return Gen2Pocket::TMHM;
        case ClassicPocket::PCItems: return Gen2Pocket::PCItems;
        case ClassicPocket::Medicines:
        case ClassicPocket::Berries:
            return std::nullopt;
    }
    return std::nullopt;
}

std::optional<Gen3Pouch> gen3Pouch(ClassicPocket pocket) noexcept {
    switch (pocket) {
        case ClassicPocket::Items: return Gen3Pouch::Items;
        case ClassicPocket::KeyItems: return Gen3Pouch::KeyItems;
        case ClassicPocket::Balls: return Gen3Pouch::PokeBalls;
        case ClassicPocket::TMHM: return Gen3Pouch::TMCase;
        case ClassicPocket::Berries: return Gen3Pouch::BerryPouch;
        case ClassicPocket::PCItems: return Gen3Pouch::PCItems;
        case ClassicPocket::Medicines:
            return std::nullopt;
    }
    return std::nullopt;
}

bool backendAvailable(TrainerViewScreen& screen, std::string& error) {
    error.clear();
    switch (sourceKind(screen.sourceGameId)) {
        case SourceKind::Gen1: {
            auto* trainer = rbyTrainer(screen);
            if (trainer && trainer->stagedInventoryAvailable()) return true;
            if (trainer) error = trainer->stagedInventoryUnavailableReason();
            break;
        }
        case SourceKind::Gen2: {
            auto* trainer = gscTrainer(screen);
            if (trainer && trainer->stagedEditingAvailable()) return true;
            if (trainer) error = trainer->stagedEditingUnavailableReason();
            break;
        }
        case SourceKind::Gen3: {
            auto* trainer = gen3Trainer(screen);
            if (trainer && trainer->stagedInventoryAvailable()) return true;
            if (trainer) error = trainer->stagedInventoryUnavailableReason();
            break;
        }
        case SourceKind::None:
            break;
    }
    if (error.empty()) error = "Staged classic inventory editing is unavailable; source remains read-only";
    return false;
}

std::vector<Row> entriesFor(TrainerViewScreen& screen, ClassicPocket pocket, std::string& error) {
    error.clear();
    std::vector<Row> out;
    switch (sourceKind(screen.sourceGameId)) {
        case SourceKind::Gen1: {
            auto* editor = rbyTrainer(screen)->stagedInventory();
            if (!editor) { error = "Generation I staged inventory is unavailable"; return {}; }
            const auto entries = editor->entries(pocket, error);
            if (!error.empty()) return {};
            out.reserve(entries.size());
            for (const auto& entry : entries) out.push_back({entry.itemId, entry.quantity});
            return out;
        }
        case SourceKind::Gen2: {
            const auto mapped = gen2Pocket(pocket);
            if (!mapped) { error = "Generation II inventory category is unsupported"; return {}; }
            auto* editor = gscTrainer(screen)->stagedEditor();
            if (!editor) { error = "Generation II staged inventory is unavailable"; return {}; }
            const auto entries = editor->inventoryEntries(*mapped, error);
            if (!error.empty()) return {};
            out.reserve(entries.size());
            for (const auto& entry : entries) out.push_back({entry.itemId, entry.quantity});
            return out;
        }
        case SourceKind::Gen3: {
            const auto mapped = gen3Pouch(pocket);
            if (!mapped) { error = "Generation III inventory category is unsupported"; return {}; }
            auto* editor = gen3Trainer(screen)->stagedInventory();
            if (!editor) { error = "Generation III staged inventory is unavailable"; return {}; }
            const auto entries = editor->inventoryEntries(*mapped, error);
            if (!error.empty()) return {};
            out.reserve(entries.size());
            for (const auto& entry : entries) out.push_back({entry.itemId, entry.count});
            return out;
        }
        case SourceKind::None:
            break;
    }
    error = "Not a supported classic inventory source";
    return {};
}

uint16_t stagedQuantity(TrainerViewScreen& screen, ClassicPocket pocket, uint16_t itemId) {
    std::string error;
    const auto rows = entriesFor(screen, pocket, error);
    if (!error.empty()) return 0;
    const auto it = std::find_if(rows.begin(), rows.end(), [itemId](const Row& row) {
        return row.itemId == itemId;
    });
    return it == rows.end() ? 0 : it->quantity;
}

bool stageQuantity(TrainerViewScreen& screen, ClassicPocket pocket, uint16_t itemId,
                   uint16_t quantity, std::string& error) {
    error.clear();
    switch (sourceKind(screen.sourceGameId)) {
        case SourceKind::Gen1:
            return rbyTrainer(screen)->stagedInventory()->stageSetQuantity(pocket, itemId, quantity, error);
        case SourceKind::Gen2: {
            const auto mapped = gen2Pocket(pocket);
            if (!mapped || itemId > 0xFF || quantity > 0xFF) {
                error = "Generation II item or quantity is outside the stored range";
                return false;
            }
            return gscTrainer(screen)->stagedEditor()->stageInventoryQuantity(
                *mapped, static_cast<uint8_t>(itemId), static_cast<uint8_t>(quantity), error);
        }
        case SourceKind::Gen3: {
            const auto mapped = gen3Pouch(pocket);
            if (!mapped) { error = "Generation III inventory category is unsupported"; return false; }
            return gen3Trainer(screen)->stagedInventory()->stageInventoryQuantity(
                *mapped, itemId, quantity, error);
        }
        case SourceKind::None:
            break;
    }
    error = "Not a supported classic inventory source";
    return false;
}

bool hasPending(TrainerViewScreen& screen) noexcept {
    switch (sourceKind(screen.sourceGameId)) {
        case SourceKind::Gen1: {
            auto* t = rbyTrainer(screen); return t && t->stagedInventory() && t->stagedInventory()->hasPendingChanges();
        }
        case SourceKind::Gen2: {
            auto* t = gscTrainer(screen); return t && t->stagedEditor() && t->stagedEditor()->hasPendingChanges();
        }
        case SourceKind::Gen3: {
            auto* t = gen3Trainer(screen); return t && t->stagedInventory() && t->stagedInventory()->hasPendingChanges();
        }
        case SourceKind::None: return false;
    }
    return false;
}

std::vector<std::string> pendingLines(TrainerViewScreen& screen) {
    std::vector<std::string> lines;
    const auto game = exactGame(screen);
    if (!game) return lines;
    switch (sourceKind(screen.sourceGameId)) {
        case SourceKind::Gen1: {
            auto* editor = rbyTrainer(screen)->stagedInventory();
            if (!editor) return lines;
            for (const auto& change : editor->pendingChanges()) {
                std::string line = std::string(PokeVault::Inventory::pocketName(change.pocket)) + " — " +
                    PokeVault::Inventory::displayItemName(*game, change.pocket, change.itemId) + ": " +
                    std::to_string(change.before) + " -> " + std::to_string(change.after);
                lines.push_back(std::move(line));
            }
            break;
        }
        case SourceKind::Gen2: {
            auto* editor = gscTrainer(screen)->stagedEditor();
            if (!editor) return lines;
            for (const auto& change : editor->pendingChanges())
                lines.push_back(change.label + ": " + change.beforeValue + " -> " + change.afterValue);
            break;
        }
        case SourceKind::Gen3: {
            auto* editor = gen3Trainer(screen)->stagedInventory();
            if (!editor) return lines;
            for (const auto& change : editor->pendingChanges())
                lines.push_back(change.label + ": " + std::to_string(change.before) + " -> " +
                                std::to_string(change.after));
            break;
        }
        case SourceKind::None:
            break;
    }
    return lines;
}

void discardAll(TrainerViewScreen& screen) noexcept {
    switch (sourceKind(screen.sourceGameId)) {
        case SourceKind::Gen1:
            if (auto* e = rbyTrainer(screen)->stagedInventory()) e->discard();
            break;
        case SourceKind::Gen2:
            if (auto* e = gscTrainer(screen)->stagedEditor()) e->discard();
            break;
        case SourceKind::Gen3:
            if (auto* e = gen3Trainer(screen)->stagedInventory()) e->discard();
            break;
        case SourceKind::None:
            break;
    }
}

std::span<const uint8_t> originalBytes(TrainerViewScreen& screen) noexcept {
    switch (sourceKind(screen.sourceGameId)) {
        case SourceKind::Gen1:
            if (auto* e = rbyTrainer(screen)->stagedInventory()) return e->originalBytes();
            break;
        case SourceKind::Gen2:
            if (auto* e = gscTrainer(screen)->stagedEditor()) return e->originalBytes();
            break;
        case SourceKind::Gen3:
            if (auto* e = gen3Trainer(screen)->stagedInventory()) return e->originalBytes();
            break;
        case SourceKind::None:
            break;
    }
    return {};
}

std::vector<uint8_t> finalizedBytes(TrainerViewScreen& screen, std::string& error) {
    switch (sourceKind(screen.sourceGameId)) {
        case SourceKind::Gen1:
            return rbyTrainer(screen)->stagedInventory()->finalize(error);
        case SourceKind::Gen2:
            return gscTrainer(screen)->stagedEditor()->finalizedBytes(error);
        case SourceKind::Gen3:
            return gen3Trainer(screen)->stagedInventory()->finalizedBytes(error);
        case SourceKind::None:
            break;
    }
    error = "Not a supported classic staged source";
    return {};
}

int generation(TrainerViewScreen& screen) noexcept {
    switch (sourceKind(screen.sourceGameId)) {
        case SourceKind::Gen1: return 1;
        case SourceKind::Gen2: return 2;
        case SourceKind::Gen3: return 3;
        case SourceKind::None: return 0;
    }
    return 0;
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
    if (!hasPending(screen)) {
        error = "No pending changes to export";
        return {};
    }
    auto edited = finalizedBytes(screen, error);
    if (edited.empty()) {
        if (error.empty()) error = "Staged save finalization failed";
        return {};
    }
    const auto original = originalBytes(screen);
    if (original.empty() || original.size() != edited.size()) {
        error = "Original/staged classic save size mismatch";
        return {};
    }

    const int gen = generation(screen);
    const std::string root = PokeBank::Paths::exportsRoot() + "/gen" + std::to_string(gen);
    if (!PokeBank::Paths::ensureDirectoryTree(root, &error)) return {};
    const std::string timestamp = Utils::getTimestamp();
    if (!PokeBank::Paths::isSafeTimestamp(timestamp)) {
        error = "Could not construct a safe classic export timestamp";
        return {};
    }
    const std::string stem = PokeBank::Paths::sanitizeComponent(screen.sourceGameId) + "_" + timestamp;
    std::string exportDir = root + "/" + stem;
    for (int suffix = 2; pathExists(exportDir) && suffix < 1000; ++suffix)
        exportDir = root + "/" + stem + "-" + std::to_string(suffix);
    if (pathExists(exportDir) || !PokeBank::Paths::ensureDirectoryTree(exportDir, &error)) {
        if (error.empty()) error = "Could not create unique classic staged export directory";
        return {};
    }

    if (!writeBytes(exportDir + "/original_backup.srm", original)) {
        error = "Could not write automatic original backup";
        return {};
    }
    if (!writeBytes(exportDir + "/edited.srm", edited)) {
        error = "Could not write staged edited save";
        return {};
    }

    std::string manifest;
    manifest += "PokeBank NX classic staged export\n";
    manifest += "GAME_ID=" + screen.sourceGameId + "\n";
    manifest += "GENERATION=" + std::to_string(gen) + "\n";
    manifest += "SOURCE_PATH=" + screen.backupDir + "\n";
    manifest += "SOURCE_KIND=RetroArchLegacy_READ_ONLY\n";
    manifest += "ORIGINAL_BACKUP=original_backup.srm\n";
    manifest += "EDITED_SAVE=edited.srm\n";
    manifest += "ORIGINAL_BYTES=" + std::to_string(original.size()) + "\n";
    manifest += "EDITED_BYTES=" + std::to_string(edited.size()) + "\n";
    manifest += "LIVE_RETROARCH_WRITE=DISABLED\n";
    manifest += "LIVE_EMULATOR_WRITE=DISABLED\n";
    manifest += "LIVE_INSTALLED_GAME_WRITE=DISABLED\n";
    manifest += "PENDING_CHANGES:\n";
    for (const auto& line : pendingLines(screen)) manifest += "- " + line + "\n";
    if (!writeText(exportDir + "/EDIT_MANIFEST.txt", manifest)) {
        error = "Edited save was written, but provenance manifest creation failed";
        return {};
    }
    return exportDir;
}

bool currentPocket(TrainerViewScreen& screen, ClassicGame& game, ClassicPocket& pocket) {
    const auto g = exactGame(screen);
    if (!g) return false;
    const auto p = PokeBank::UIModel::classicInventoryPocketAt(*g, screen.selectedCategory);
    if (!p) return false;
    game = *g;
    pocket = *p;
    return true;
}

bool currentItem(TrainerViewScreen& screen, uint16_t& itemId, uint16_t& quantity) {
    if (screen.selectedCategory < 0 ||
        screen.selectedCategory >= static_cast<int>(screen.trainer.items.size())) return false;
    const auto visible = screen.visibleItemIndices();
    if (screen.selectedItemIndex < 0 ||
        screen.selectedItemIndex >= static_cast<int>(visible.size())) return false;
    const auto& item = screen.trainer.items[screen.selectedCategory][visible[screen.selectedItemIndex]];
    itemId = item.itemId;
    quantity = item.count;
    return quantity != 0;
}

void focusItem(TrainerViewScreen& screen, uint16_t itemId) {
    if (screen.selectedCategory < 0 ||
        screen.selectedCategory >= static_cast<int>(screen.trainer.items.size())) return;
    const auto visible = screen.visibleItemIndices();
    for (int i = 0; i < static_cast<int>(visible.size()); ++i) {
        const auto& item = screen.trainer.items[screen.selectedCategory][visible[i]];
        if (item.itemId != itemId) continue;
        screen.selectedItemIndex = i;
        const int perPage = std::max(1, (CONTENT_PANEL_HEIGHT - 106) / 52);
        screen.currentPage = i / perPage;
        return;
    }
}

bool applyMutation(TrainerViewScreen& screen, ClassicPocket pocket, uint16_t itemId,
                   uint16_t quantity) {
    std::string error;
    if (!stageQuantity(screen, pocket, itemId, quantity, error)) {
        screen.postStatus(error.empty() ? "Classic inventory edit was rejected" : error, 360);
        return false;
    }
    if (!refreshPresentation(screen)) {
        screen.postStatus("Edit staged, but the inventory presentation could not refresh", 360);
        return false;
    }
    if (quantity != 0) focusItem(screen, itemId);
    else {
        const auto visible = screen.visibleItemIndices();
        if (screen.selectedItemIndex >= static_cast<int>(visible.size()))
            screen.selectedItemIndex = std::max(0, static_cast<int>(visible.size()) - 1);
        const int perPage = std::max(1, (CONTENT_PANEL_HEIGHT - 106) / 52);
        screen.currentPage = visible.empty() ? 0 : screen.selectedItemIndex / perPage;
    }
    screen.postStatus(quantity == 0
        ? "Item removal staged only; source save unchanged"
        : "Inventory change staged only; source save unchanged", 240);
    return true;
}

void queueMutationWithWarning(TrainerViewScreen& screen, ClassicGame game, ClassicPocket pocket,
                              uint16_t itemId, uint16_t quantity) {
    auto& state = stateFor(screen);
    if (!PokeBank::UIModel::classicInventoryNeedsStoryWarning(game, pocket, itemId)) {
        applyMutation(screen, pocket, itemId, quantity);
        return;
    }
    state.pendingPocket = pocket;
    state.pendingItem = itemId;
    state.pendingQuantity = quantity;
    state.warningActive = true;
}

void openAddPicker(TrainerViewScreen& screen, ClassicGame game, ClassicPocket pocket) {
    auto& state = stateFor(screen);
    state.pickerItems = PokeBank::UIModel::classicInventoryAddableItems(game, pocket);
    state.pickerRow = 0;
    if (state.pickerItems.empty()) {
        screen.postStatus("No valid addable items exist in this category", 300);
        return;
    }
    state.pickerActive = true;
}

bool selectPickerItem(TrainerViewScreen& screen) {
    auto& state = stateFor(screen);
    ClassicGame game{};
    ClassicPocket pocket{};
    if (!currentPocket(screen, game, pocket) || state.pickerItems.empty()) return false;
    const uint16_t itemId = state.pickerItems[static_cast<std::size_t>(state.pickerRow)];
    const uint16_t existing = stagedQuantity(screen, pocket, itemId);
    if (existing != 0) {
        state.pickerActive = false;
        refreshPresentation(screen);
        focusItem(screen, itemId);
        screen.postStatus("Already in this category; focused the existing item", 240);
        return true;
    }

    const auto rule = PokeVault::Inventory::quantityRule(game, pocket, itemId);
    uint16_t quantity = rule.minimum;
    if (rule.editable) {
        const auto result = Utils::promptNumber(
            "Add " + PokeVault::Inventory::displayItemName(game, pocket, itemId),
            static_cast<int>(rule.minimum), static_cast<int>(rule.minimum),
            static_cast<int>(rule.maximum));
        if (!result.accepted) return true;
        quantity = static_cast<uint16_t>(result.value);
    }
    state.pickerActive = false;
    queueMutationWithWarning(screen, game, pocket, itemId, quantity);
    return true;
}

void drawRow(PKSEFramebuffer& fb, int x, int y, int width, const std::string& text, bool selected) {
    constexpr int h = 48;
    fb.drawFilledRoundedRect(x, y, width, h, 10, selected ? Colors::AccentDim : Colors::PanelAlt);
    if (selected) fb.drawRoundedRect(x, y, width, h, 10, Colors::Accent, 2);
    fb.drawText(x + 18, y + 13, text, selected ? Colors::Text : Colors::TextDim, TextStyle::Body);
}

} // namespace

bool isClassicSource(const TrainerViewScreen& screen) noexcept {
    return sourceKind(screen.sourceGameId) != SourceKind::None &&
           PokeBank::UIModel::classicInventoryGame(screen.sourceGameId).has_value();
}

bool refreshPresentation(TrainerViewScreen& screen) {
    const auto game = exactGame(screen);
    if (!game) return false;
    std::string error;
    if (!backendAvailable(screen, error)) return false;
    const auto categories = PokeBank::UIModel::classicInventoryCategories(*game);
    std::vector<std::vector<Trainer::InventoryItem>> next(categories.size());
    for (std::size_t category = 0; category < categories.size(); ++category) {
        const auto rows = entriesFor(screen, categories[category], error);
        if (!error.empty()) return false;
        next[category].reserve(rows.size());
        for (const auto& row : rows)
            if (row.quantity != 0)
                next[category].push_back(Trainer::InventoryItem{row.itemId, row.quantity, false, false});
    }
    screen.trainer.items = std::move(next);
    return true;
}

bool handleInput(TrainerViewScreen& screen, uint64_t down) {
    if (!isClassicSource(screen)) return false;
    auto& state = stateFor(screen);

    if (state.warningActive) {
        if (down & HidNpadButton_B) {
            state.warningActive = false;
            return true;
        }
        if (down & HidNpadButton_A) {
            const auto pocket = state.pendingPocket;
            const uint16_t item = state.pendingItem;
            const uint16_t quantity = state.pendingQuantity;
            state.warningActive = false;
            applyMutation(screen, pocket, item, quantity);
            return true;
        }
        return true;
    }

    if (state.pickerActive) {
        const int count = static_cast<int>(state.pickerItems.size());
        if (count == 0) { state.pickerActive = false; return true; }
        if (down & HidNpadButton_Up) state.pickerRow = (state.pickerRow - 1 + count) % count;
        if (down & HidNpadButton_Down) state.pickerRow = (state.pickerRow + 1) % count;
        if (down & HidNpadButton_B) { state.pickerActive = false; return true; }
        if (down & HidNpadButton_A) { selectPickerItem(screen); return true; }
        return true;
    }

    if (state.reviewActive) {
        const auto lines = pendingLines(screen);
        const int count = static_cast<int>(lines.size());
        if (count > 0 && (down & HidNpadButton_Up)) state.reviewRow = (state.reviewRow - 1 + count) % count;
        if (count > 0 && (down & HidNpadButton_Down)) state.reviewRow = (state.reviewRow + 1) % count;
        if (down & HidNpadButton_B) { state.reviewActive = false; return true; }
        if (down & HidNpadButton_Y) {
            discardAll(screen);
            refreshPresentation(screen);
            state.reviewActive = false;
            screen.postStatus("Staged changes discarded; original save was never modified", 300);
            return true;
        }
        if (down & HidNpadButton_A) {
            std::string error;
            const std::string dir = exportStagedCopy(screen, error);
            if (dir.empty()) screen.postStatus(error.empty() ? "Classic staged export failed" : error, 360);
            else {
                state.lastExportDirectory = dir;
                screen.postStatus("Exported edited copy + original backup under PokeBank NX exports", 360);
            }
            return true;
        }
        return true;
    }

    if (!screen.detailViewActive || screen.selectedMode != TrainerViewScreen::ViewMode::Items ||
        screen.helpOverlayActive || screen.details.active || screen.actionSheet.isOpen() ||
        screen.saveConfirmActive || screen.pickerActive || screen.itemEditDialogActive ||
        screen.itemRemoveConfirmActive) return false;

    std::string availabilityError;
    const uint64_t classicActions = HidNpadButton_A | HidNpadButton_X | HidNpadButton_Y |
                                    HidNpadButton_L | HidNpadButton_R | HidNpadButton_Plus;
    if ((down & classicActions) && !backendAvailable(screen, availabilityError)) {
        screen.postStatus(availabilityError, 360);
        return true;
    }

    ClassicGame game{};
    ClassicPocket pocket{};
    if (!currentPocket(screen, game, pocket)) {
        if (down & classicActions) {
            screen.postStatus("This inventory category is not supported by the selected classic game", 300);
            return true;
        }
        return false;
    }

    if (down & HidNpadButton_L) {
        const int count = static_cast<int>(PokeBank::UIModel::classicInventoryCategories(game).size());
        screen.selectedCategory = (screen.selectedCategory - 1 + count) % count;
        screen.selectedItemIndex = 0;
        screen.currentPage = 0;
        return true;
    }
    if (down & HidNpadButton_R) {
        const int count = static_cast<int>(PokeBank::UIModel::classicInventoryCategories(game).size());
        screen.selectedCategory = (screen.selectedCategory + 1) % count;
        screen.selectedItemIndex = 0;
        screen.currentPage = 0;
        return true;
    }
    if (down & HidNpadButton_Plus) {
        state.reviewActive = true;
        state.reviewRow = 0;
        return true;
    }
    if (down & HidNpadButton_Y) {
        openAddPicker(screen, game, pocket);
        return true;
    }

    uint16_t itemId = 0;
    uint16_t quantity = 0;
    if ((down & (HidNpadButton_A | HidNpadButton_X)) && !currentItem(screen, itemId, quantity)) {
        screen.postStatus("No item is selected. Use Y to add an item to this category.", 240);
        return true;
    }

    if (down & HidNpadButton_A) {
        const auto rule = PokeVault::Inventory::quantityRule(game, pocket, itemId);
        if (!rule.editable) {
            screen.postStatus("This item has a fixed quantity; amount editing is not applicable", 300);
            return true;
        }
        const auto result = Utils::promptNumber(
            "Edit " + PokeVault::Inventory::displayItemName(game, pocket, itemId),
            static_cast<int>(quantity), static_cast<int>(rule.minimum), static_cast<int>(rule.maximum));
        if (result.accepted)
            applyMutation(screen, pocket, itemId, static_cast<uint16_t>(result.value));
        return true;
    }

    if (down & HidNpadButton_X) {
        queueMutationWithWarning(screen, game, pocket, itemId, 0);
        return true;
    }
    return false;
}

void drawOverlay(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    if (!isClassicSource(screen)) return;
    auto& state = stateFor(screen);
    if (!state.pickerActive && !state.reviewActive && !state.warningActive) return;

    screen.touchButtons.clear();
    constexpr int width = 900;
    constexpr int height = 610;
    const int x = (fb.getWidth() - width) / 2;
    const int y = 66;
    fb.drawSoftShadow(x, y, width, height, 18);
    fb.drawFilledRoundedRect(x, y, width, height, 18, Colors::Panel);
    fb.drawRoundedRect(x, y, width, height, 18, Colors::Accent, 2);

    const auto game = exactGame(screen);
    const auto pocket = game ? PokeBank::UIModel::classicInventoryPocketAt(*game, screen.selectedCategory)
                             : std::optional<ClassicPocket>{};

    if (state.warningActive) {
        fb.drawText(x + 30, y + 24, "Key Item Warning", Colors::Text, TextStyle::Heading);
        fb.drawText(x + 30, y + 100,
                    "This Key Item may depend on story/event state.", Colors::Text, TextStyle::Body);
        fb.drawText(x + 30, y + 142,
                    "Changing inventory does not reproduce or reverse the associated story event.",
                    Colors::TextDim, TextStyle::Body);
        fb.drawText(x + 30, y + 196,
                    "No unrelated event flags will be modified.", Colors::TextDim, TextStyle::Body);
        fb.drawText(x + 30, y + height - 54, "A Continue    B Cancel",
                    Colors::Text, TextStyle::Body);
        return;
    }

    if (state.pickerActive && game && pocket) {
        fb.drawText(x + 30, y + 20,
                    "Add Item to " + std::string(PokeVault::Inventory::pocketName(*pocket)),
                    Colors::Text, TextStyle::Heading);
        fb.drawText(x + 30, y + 56,
                    "Exact-game catalog only — stored item IDs remain native to this game.",
                    Colors::TextDim, TextStyle::Caption);
        constexpr int visibleRows = 8;
        const int count = static_cast<int>(state.pickerItems.size());
        const int start = std::clamp(state.pickerRow - visibleRows / 2, 0,
                                     std::max(0, count - visibleRows));
        int rowY = y + 94;
        for (int i = start; i < std::min(count, start + visibleRows); ++i) {
            const uint16_t itemId = state.pickerItems[static_cast<std::size_t>(i)];
            std::string label = PokeVault::Inventory::displayItemName(*game, *pocket, itemId);
            if (stagedQuantity(screen, *pocket, itemId) != 0) label += "  (Already in pouch)";
            drawRow(fb, x + 30, rowY, width - 60, label, i == state.pickerRow);
            rowY += 57;
        }
        fb.drawText(x + 30, y + height - 42, "Up/Down Select    A Add / Focus Existing    B Cancel",
                    Colors::TextDim, TextStyle::Caption);
        return;
    }

    if (state.reviewActive) {
        const auto lines = pendingLines(screen);
        fb.drawText(x + 30, y + 20, "Pending Classic Save Changes", Colors::Text, TextStyle::Heading);
        fb.drawText(x + 30, y + 56,
                    "Export creates a separate edited .srm plus an automatic original backup.",
                    Colors::TextDim, TextStyle::Caption);
        if (lines.empty()) {
            fb.drawText(x + 34, y + 116, "No pending changes.", Colors::TextDim, TextStyle::Body);
        } else {
            constexpr int visibleRows = 7;
            const int count = static_cast<int>(lines.size());
            const int start = std::clamp(state.reviewRow - visibleRows / 2, 0,
                                         std::max(0, count - visibleRows));
            int rowY = y + 94;
            for (int i = start; i < std::min(count, start + visibleRows); ++i) {
                drawRow(fb, x + 30, rowY, width - 60, lines[static_cast<std::size_t>(i)],
                        i == state.reviewRow);
                rowY += 57;
            }
        }
        fb.drawText(x + 30, y + height - 72,
                    "A Export Edited Copy    Y Discard Staged Changes    B Back",
                    Colors::Text, TextStyle::Body);
        if (!state.lastExportDirectory.empty())
            fb.drawText(x + 30, y + height - 40, "Last export: " + state.lastExportDirectory,
                        Colors::TextDim, TextStyle::Caption);
    }
}

} // namespace UI::ClassicInventory
