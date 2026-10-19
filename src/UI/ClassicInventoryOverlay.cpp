#include "UI/ClassicInventoryOverlay.h"
#include "UI/SharedBoxedRows.h"   // R2-BUG-012: Add Item picker rows use the shared boxed style

#include "Integration/Common/ClassicStrictValidators.h"
#include "Integration/Common/VerifiedStagedExport.h"
#include "Integration/Gen1/Gen1StagedInventoryEditor.h"
#include "Integration/Gen2/Gen2StagedEditor.h"
#include "Integration/Gen3/Gen3StagedInventoryEditor.h"
#include "Inventory/ClassicInventoryCatalog.h"
#include "Legacy/FRLGReadOnlyTrainer.h"
#include "Legacy/GSCReadOnlyTrainer.h"
#include "Legacy/RBYReadOnlyTrainer.h"
#include "Trainer/Trainer.h"
#include "UI/ClassicInventoryUIModel.h"
#include "UI/SharedBagAddFlow.h"
#include "UI/SharedBagAddPicker.h"
#include "UI/InventoryUIContract.h"
#include "UI/Common.h"
#include "UI/ScreenChrome.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/TrainerViewScreen.h"
#include "UI/TouchInput.h"
#include "UI/TouchScroll.h"
#include "UI/ItemPickerArtwork.h"
#include "UI/ItemPickerSort.h"
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
namespace Common = PokeVault::Integration::Common;

using PokeVault::Inventory::ClassicGame;
using PokeVault::Inventory::ClassicPocket;
using Gen2Pocket = PokeVault::Integration::Gen2::InventoryPocket;
using Gen3Pouch = PokeVault::Integration::Gen3::InventoryPouch;

// ItemsPanel renders eight 52px rows in the fixed inventory content region.
constexpr int kClassicInventoryRowsPerPage = 8;

struct Row {
    uint16_t itemId = 0;
    uint16_t quantity = 0;
};

enum class SourceKind : uint8_t { None, Gen1, Gen2, Gen3 };

struct OverlayState {
    const TrainerViewScreen* owner = nullptr;
    std::string sourceGameId;
    bool presentationInitialized = false;
    bool pickerActive = false;
    bool optionsActive = false;
    bool reviewActive = false;
    bool helpActive = false;
    bool warningActive = false;
    bool discardConfirmActive = false;
    bool removeConfirmActive = false;
    int pickerRow = 0;
    int optionsRow = 0;
    int reviewRow = 0;
    TouchScrollState pickerScroll;
    TouchPickerViewport pickerViewport;
    SharedScrollbar::State pickerScrollbar;   // R2-SCROLLBAR: draggable thumb on the Add Item list
    TouchScrollState reviewScroll;
    TouchPickerViewport reviewViewport;
    SharedScrollbar::State reviewScrollbar;   // R2-SCROLLBAR: draggable thumb on Pending Changes
    ClassicPocket pendingPocket = ClassicPocket::Items;
    uint16_t pendingItem = 0;
    uint16_t pendingQuantity = 0;
    std::vector<uint16_t> pickerItems;
    std::vector<uint16_t> pickerNativeItems;
    ItemPickerSort::Mode pickerSort = ItemPickerSort::Mode::Native;
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

// Strict parser for the exact classic game behind the current staged Bag workspace.
Common::StrictSaveValidator strictValidator(TrainerViewScreen& screen) {
    switch (sourceKind(screen.sourceGameId)) {
        case SourceKind::Gen1:
            return Common::gen1StrictValidator(rbyTrainer(screen)->stagedInventory()->sourceGame());
        case SourceKind::Gen2:
            return Common::gen2StrictValidator(gscTrainer(screen)->stagedEditor()->metadata().sourceGame);
        case SourceKind::Gen3:
            return Common::gen3StrictValidator(gen3Trainer(screen)->stagedInventory()->sourceGame());
        case SourceKind::None:
            break;
    }
    return {};
}

// Verified Bag export: temp directory, fsync, read-back + SHA-256, strict reparse before and after
// writing, atomic publish. Only the app-owned exports root is written; the source never is.
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

    Common::VerifiedExportRequest request;
    request.rootDirectory = root;
    request.directoryStem = PokeBank::Paths::sanitizeComponent(screen.sourceGameId) + "_" + timestamp;
    request.generationLabel = "Generation " + std::string(gen == 1 ? "I" : gen == 2 ? "II" : "III") + " Bag";
    request.fileExtension = ".srm";
    request.sourcePath = screen.backupDir;
    request.gameId = screen.sourceGameId;
    request.saveFormat = gen == 3 ? "Gen III rotating-sector Flash" : "Classic battery SRAM";
    request.timestamp = timestamp;
#ifdef POKEVAULT_GIT_COMMIT
    request.applicationSha = POKEVAULT_GIT_COMMIT;
#endif
    request.originalBytes = original;
    request.editedBytes = edited;
    request.changeLines = pendingLines(screen);
    request.validator = strictValidator(screen);
    const auto result = Common::publishVerifiedStagedExport(request);
    if (!result.success) {
        error = result.error.empty() ? "Verified classic export failed" : result.error;
        return {};
    }
    return result.directory;
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

// R2-BUG-006: the classic Add picker is the shared compact grid (UI/SharedItemGridPicker.h).
// Input and draw build it from the SAME native labels, so the cell a tap / D-pad / scrollbar
// lands on is the cell drawn.
UI::SharedItemGridPicker::Grid classicPickerGrid(const TrainerViewScreen& screen, const OverlayState& state) {
    const auto game = exactGame(screen);
    const auto pocket = game ? PokeBank::UIModel::classicInventoryPocketAt(*game, screen.selectedCategory)
                             : std::optional<ClassicPocket>{};
    return UI::SharedBagAddPicker::pickerGrid(static_cast<int>(state.pickerItems.size()), [&](int index) {
        return game && pocket
            ? PokeVault::Inventory::displayItemName(*game, *pocket, state.pickerItems[static_cast<std::size_t>(index)])
            : std::string();
    });
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
        const int perPage = kClassicInventoryRowsPerPage;
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
        const int perPage = kClassicInventoryRowsPerPage;
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
    state.pickerNativeItems = state.pickerItems;
    state.pickerSort = ItemPickerSort::Mode::Native;
    state.pickerRow = 0;
    state.pickerScroll.reset();
    state.pickerViewport.reset(); // Initialized from the actual visible rows on first draw/update.
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
    // The shared Gen I-V Add decision (UI/SharedBagAddFlow.h): an existing stack is focused,
    // a new item is staged at quantity 1 with NO numpad; A on its row edits the quantity later.
    // Capacity is left to the staged writer, whose refusal names the exact full pocket.
    PokeBank::UIModel::BagAdd::PocketState pocketState{};
    pocketState.heldInPocket = stagedQuantity(screen, pocket, itemId);
    const PokeVault::Inventory::QuantityPolicy::ItemQuantityRule rule =
        PokeBank::UIModel::classicItemQuantityRule(game, pocket, itemId);
    const auto decision = PokeBank::UIModel::BagAdd::decidePick(rule, pocketState);
    using PokeBank::UIModel::BagAdd::PickOutcome;
    switch (decision.outcome) {
        case PickOutcome::NotAllowed:
            screen.postStatus(decision.message, 240);
            return true;
        case PickOutcome::FocusExisting:
            state.pickerActive = false;
            refreshPresentation(screen);
            focusItem(screen, itemId);
            screen.postStatus(decision.message, 240);
            return true;
        case PickOutcome::PocketFull:
            state.pickerActive = false;
            screen.postStatus(decision.message, 300);
            return true;
        case PickOutcome::StageNew:
            break;
    }
    state.pickerActive = false;
    queueMutationWithWarning(screen, game, pocket, itemId, decision.quantity);
    return true;
}

void drawRow(PKSEFramebuffer& fb, int x, int y, int width, const std::string& text,
             bool selected, int height = 48, bool brightUnselected = false) {
    fb.drawFilledRoundedRect(x, y, width, height, 10, Colors::PanelAlt);
    if (selected) fb.drawRoundedRect(x, y, width, height, 10, Colors::FocusBorder, 2);
    const int ty = y + (height - fb.lineHeight(TextStyle::Body)) / 2;
    const auto textColor = (selected || brightUnselected) ? Colors::Text : Colors::TextDim;
    fb.drawText(x + 18, ty, text, textColor, TextStyle::Body);
}

} // namespace

bool isClassicSource(const TrainerViewScreen& screen) noexcept {
    return sourceKind(screen.sourceGameId) != SourceKind::None &&
           PokeBank::UIModel::classicInventoryGame(screen.sourceGameId).has_value();
}

bool stagedEditingAvailable(TrainerViewScreen& screen) {
    if (!isClassicSource(screen)) return false;
    std::string error;
    return backendAvailable(screen, error);
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

bool handleInput(TrainerViewScreen& screen, uint64_t down, const TouchInput& touch) {
    if (!isClassicSource(screen)) return false;
    auto& state = stateFor(screen);
    const int touchId = screen.touchedButtonId(touch);
    // Inventory actions, review, and Add Item keep their semantic focus until
    // a release-confirmed same-target tap. A finger landing on Options may
    // still become a swipe without silently changing the highlighted action.
    if (!state.presentationInitialized && refreshPresentation(screen)) {
        screen.captureInventorySourceBaseline();
        state.presentationInitialized = true;
        const auto game = exactGame(screen);
        const int categoryCount = game
            ? static_cast<int>(PokeBank::UIModel::classicInventoryCategories(*game).size()) : 0;
        if (categoryCount > 0)
            screen.selectedCategory = std::clamp(screen.selectedCategory, 0, categoryCount - 1);
        screen.selectedItemIndex = 0;
        screen.currentPage = 0;
    }

    if (state.removeConfirmActive) {
        // The Remove question owns every input: Y removes, B cancels, nothing else acts.
        switch (PokeBank::UIModel::classicRemoveConfirmStep((down & HidNpadButton_Y) != 0,
                                                            (down & HidNpadButton_B) != 0)) {
            case PokeBank::UIModel::ClassicRemoveConfirm::Cancel:
                state.removeConfirmActive = false;
                break;
            case PokeBank::UIModel::ClassicRemoveConfirm::Remove: {
                state.removeConfirmActive = false;
                ClassicGame game{};
                ClassicPocket pocket{};
                if (currentPocket(screen, game, pocket))
                    queueMutationWithWarning(screen, game, state.pendingPocket, state.pendingItem, 0);
                break;
            }
            case PokeBank::UIModel::ClassicRemoveConfirm::None:
                break;
        }
        return true;
    }

    if (state.warningActive) {
        if (down & HidNpadButton_B) { state.warningActive = false; return true; }
        if (down & HidNpadButton_A) {
            const auto pocket = state.pendingPocket;
            const uint16_t item = state.pendingItem;
            const uint16_t quantity = state.pendingQuantity;
            state.warningActive = false;
            applyMutation(screen, pocket, item, quantity);
        }
        return true;
    }

    if (state.discardConfirmActive) {
        if (down & HidNpadButton_B) { state.discardConfirmActive = false; return true; }
        if (down & HidNpadButton_A) {
            discardAll(screen);
            refreshPresentation(screen);
            state.discardConfirmActive = false;
            screen.postStatus("Staged changes discarded; original source save was never modified", 300);
        }
        return true;
    }

    if (state.pickerActive) {
        const int count = static_cast<int>(state.pickerItems.size());
        // The shared Gen I-V compact grid (R2-BUG-006, UI/SharedItemGridPicker.h): the same cells
        // the picker draws and publishes as touch targets; the viewport scrolls by grid rows.
        namespace AddGrid = UI::SharedItemGridPicker;
        const auto geometry = classicPickerGrid(screen, state);
        const int rowH = AddGrid::kRule.rowHeight;
        const int visibleRows = std::max(1, geometry.visibleRows);
        const int columns = geometry.columns;
        state.pickerViewport.ensure(state.pickerRow, count, visibleRows, columns);
        const int maxFirstRow = TouchPickerViewport::maxFirst(count, visibleRows, columns);
        // Shared scrollbar first: a held thumb moves the window, not the row, and owns the touch.
        const auto pickerBar = SharedScrollbar::handleTouch(state.pickerScrollbar, SharedScrollbar::sample(touch),
            AddGrid::scrollModel(geometry, state.pickerViewport.firstRow));
        if (pickerBar.changed) {
            state.pickerViewport.firstRow = pickerBar.scroll;
            state.pickerScroll.stop();
        }
        if (maxFirstRow > 0 && !pickerBar.consumed) {
            state.pickerScroll.updateVertical(
                touch, geometry.list.x, geometry.list.y, geometry.list.w, geometry.list.h,
                rowH, state.pickerViewport.firstRow, maxFirstRow + 1);
        } else {
            state.pickerScroll.stop();
        }
        if (!pickerBar.consumed && touchId >= 0 && touchId < count) {
            state.pickerScroll.stop();
            state.pickerRow = touchId;
            down |= HidNpadButton_A;
        }
        if (count == 0) { state.pickerScroll.stop(); state.pickerActive = false; return true; }
        if (down & HidNpadButton_X) {
            ClassicGame game{};
            ClassicPocket pocket{};
            if (currentPocket(screen, game, pocket)) {
                state.pickerSort = ItemPickerSort::next(state.pickerSort);
                state.pickerRow = ItemPickerSort::reorder(
                    state.pickerItems, state.pickerNativeItems,
                    state.pickerSort, state.pickerRow,
                    [&](uint16_t id) {
                        return PokeVault::Inventory::displayItemName(game, pocket, id);
                    });
                state.pickerScroll.stop();
                state.pickerViewport.reset();
                state.pickerViewport.reveal(state.pickerRow, count, visibleRows, columns);
            }
            return true;
        }
        const AddGrid::Keys gridKeys{
            (down & HidNpadButton_Up) != 0, (down & HidNpadButton_Down) != 0,
            (down & HidNpadButton_Left) != 0, (down & HidNpadButton_Right) != 0,
            (down & HidNpadButton_L) != 0, (down & HidNpadButton_R) != 0,
            (down & HidNpadButton_ZL) != 0, (down & HidNpadButton_ZR) != 0};
        if (gridKeys.any()) {
            // Grid navigation: D-pad cell to cell, L/R one screen of rows, ZL/ZR five.
            // Use the visible touchscreen viewport for controller page steps.
            const int page = visibleRows;
            state.pickerScroll.stop();
            state.pickerRow = AddGrid::navigate(geometry, state.pickerRow, gridKeys, page);
            state.pickerViewport.reveal(state.pickerRow, count, visibleRows, columns);
        }
        if (down & HidNpadButton_B) { state.pickerScroll.stop(); state.pickerActive = false; return true; }
        if (down & HidNpadButton_A) {
            if (!state.pickerViewport.containsSelection(state.pickerRow, count, visibleRows, columns)) {
                state.pickerScroll.stop();
                state.pickerViewport.reveal(state.pickerRow, count, visibleRows, columns);
                screen.postStatus("Focused item brought into view; press A again or tap a visible row", 220);
                return true;
            }
            state.pickerScroll.stop();
            selectPickerItem(screen);
            return true;
        }
        return true;
    }

    if (state.helpActive) {
        if (down & (HidNpadButton_B | HidNpadButton_Minus)) state.helpActive = false;
        return true;
    }

    if (state.reviewActive) {
        const auto lines = pendingLines(screen);
        const int count = static_cast<int>(lines.size());
        constexpr int width = 900, y = 66, rowStep = 57, visibleRows = 7;
        const int x = (1280 - width) / 2;
        state.reviewViewport.ensure(state.reviewRow, count, visibleRows);
        const int maxFirstRow = TouchPickerViewport::maxFirst(count, visibleRows);
        const auto reviewBar = SharedScrollbar::handleTouch(state.reviewScrollbar, SharedScrollbar::sample(touch),
            SharedScrollbar::rows(count, visibleRows, state.reviewViewport.firstRow));
        if (reviewBar.changed) {
            state.reviewViewport.firstRow = reviewBar.scroll;
            state.reviewScroll.stop();
        }
        if (maxFirstRow > 0 && !reviewBar.consumed) {
            state.reviewScroll.updateVertical(
                touch, x + 30, y + 94, width - 60, visibleRows * rowStep,
                rowStep, state.reviewViewport.firstRow, maxFirstRow + 1);
        } else {
            state.reviewScroll.stop();
        }
        if (!reviewBar.consumed && touchId >= 0 && touchId < count) {
            state.reviewScroll.stop();
            state.reviewRow = touchId;
        }
        if (down & (HidNpadButton_Up | HidNpadButton_Down))
            state.reviewScroll.stop();
        if (count > 0 && (down & HidNpadButton_Up)) state.reviewRow = (state.reviewRow - 1 + count) % count;
        if (count > 0 && (down & HidNpadButton_Down)) state.reviewRow = (state.reviewRow + 1) % count;
        if (down & (HidNpadButton_Up | HidNpadButton_Down))
            state.reviewViewport.reveal(state.reviewRow, count, visibleRows);
        if (down & HidNpadButton_B) {
            state.reviewScroll.stop();
            state.reviewActive = false;
            state.optionsActive = true;
        }
        return true;
    }

    if (state.optionsActive) {
        constexpr int optionCount = 4;
        if (touchId >= 0 && touchId < optionCount) { state.optionsRow = touchId; down |= HidNpadButton_A; }
        if (down & HidNpadButton_Up) state.optionsRow = (state.optionsRow - 1 + optionCount) % optionCount;
        if (down & HidNpadButton_Down) state.optionsRow = (state.optionsRow + 1) % optionCount;
        if (down & HidNpadButton_B) { state.optionsActive = false; return true; }
        if (!(down & HidNpadButton_A)) return true;
        switch (state.optionsRow) {
            case 0:
                state.optionsActive = false;
                state.reviewActive = true;
                state.reviewRow = 0;
                state.reviewScroll.reset();
                state.reviewViewport.reset();
                state.reviewViewport.ensure(
                    state.reviewRow, static_cast<int>(pendingLines(screen).size()), 7);
                break;
            case 1: {
                std::string error;
                const std::string dir = exportStagedCopy(screen, error);
                if (dir.empty()) screen.postStatus(error.empty() ? "Classic staged export failed" : error, 360);
                else {
                    state.lastExportDirectory = dir;
                    screen.postStatus("Exported edited copy + original backup under PokeBank NX exports", 360);
                }
                break;
            }
            case 2:
                if (!hasPending(screen)) screen.postStatus("No staged changes to discard", 240);
                else state.discardConfirmActive = true;
                break;
            case 3:
                state.optionsActive = false;
                break;
        }
        return true;
    }

    if (!screen.detailViewActive || screen.selectedMode != TrainerViewScreen::ViewMode::Items ||
        screen.helpOverlayActive || screen.details.active || screen.actionSheet.isOpen() ||
        screen.saveConfirmActive || screen.pickerActive || screen.itemEditDialogActive ||
        screen.itemRemoveConfirmActive || screen.statEdit.dialogActive ||
        screen.releaseConfirmActive || screen.storageExitConfirmActive || screen.groupMenuActive ||
        screen.creator.keepConfirmActive || screen.details.discardConfirmActive ||
        screen.gen3ConvertConfirmActive || screen.lgpeTransferConfirmActive) return false;

    // The header arrows publish negative touch IDs. Route their presses through
    // this exact-game category model instead of the inherited two-category RBY
    // screen, which cannot navigate Medicines/Balls/Key Items/TMs/PC correctly.
    using CategoryTouch = PokeBank::UIModel::InventoryCategoryTouch;
    if (touchId == CategoryTouch::PreviousId) down |= HidNpadButton_L;
    if (touchId == CategoryTouch::NextId) down |= HidNpadButton_R;

    std::optional<PokeBank::UIModel::ClassicInventoryInput> input;
    if (down & HidNpadButton_A) input = PokeBank::UIModel::ClassicInventoryInput::A;
    else if (down & HidNpadButton_X) input = PokeBank::UIModel::ClassicInventoryInput::X;
    else if (down & HidNpadButton_Y) input = PokeBank::UIModel::ClassicInventoryInput::Y;
    else if (down & HidNpadButton_L) input = PokeBank::UIModel::ClassicInventoryInput::L;
    else if (down & HidNpadButton_R) input = PokeBank::UIModel::ClassicInventoryInput::R;
    else if (down & HidNpadButton_Plus) input = PokeBank::UIModel::ClassicInventoryInput::Plus;
    else if (down & HidNpadButton_Minus) input = PokeBank::UIModel::ClassicInventoryInput::Minus;
    else if (down & HidNpadButton_B) input = PokeBank::UIModel::ClassicInventoryInput::B;
    else return false;

    const auto action = PokeBank::UIModel::classicInventoryAction(*input);
    if (action == PokeBank::UIModel::ClassicInventoryAction::Back) return false;

    ClassicGame game{};
    ClassicPocket pocket{};
    if (!currentPocket(screen, game, pocket)) {
        screen.postStatus("This inventory category is not supported by the selected classic game", 300);
        return true;
    }

    if (action == PokeBank::UIModel::ClassicInventoryAction::PreviousCategory ||
        action == PokeBank::UIModel::ClassicInventoryAction::NextCategory) {
        const int delta = action == PokeBank::UIModel::ClassicInventoryAction::PreviousCategory ? -1 : 1;
        screen.selectedCategory = PokeBank::UIModel::classicInventoryShiftCategory(
            game, screen.selectedCategory, delta);
        screen.selectedItemIndex = 0;
        screen.currentPage = 0;
        return true;
    }

    if (action == PokeBank::UIModel::ClassicInventoryAction::Help) {
        state.helpActive = true;
        return true;
    }

    std::string availabilityError;
    if (!backendAvailable(screen, availabilityError)) {
        screen.postStatus(availabilityError, 360);
        return true;
    }

    if (action == PokeBank::UIModel::ClassicInventoryAction::Options) {
        state.optionsActive = true;
        state.optionsRow = 0;
        return true;
    }

    uint16_t itemId = 0;
    uint16_t quantity = 0;
    const bool hasSelection = currentItem(screen, itemId, quantity);
    const bool quantityEditable = hasSelection &&
        PokeBank::UIModel::classicInventoryQuantityEditable(game, pocket, itemId);
    const auto availability = PokeBank::UIModel::classicInventoryActionAvailability(
        true, hasSelection, quantityEditable);

    if (action == PokeBank::UIModel::ClassicInventoryAction::AddItem) {
        if (availability.addItem) openAddPicker(screen, game, pocket);
        return true;
    }
    if (action == PokeBank::UIModel::ClassicInventoryAction::RemoveItem) {
        if (availability.removeItem) {
            state.pendingPocket = pocket;
            state.pendingItem = itemId;
            state.pendingQuantity = 0;
            state.removeConfirmActive = true;
        }
        return true;
    }
    if (action == PokeBank::UIModel::ClassicInventoryAction::EditAmount) {
        if (!availability.editAmount) {
            if (!hasSelection) screen.postStatus("No item selected. Press X to add an item.", 240);
            else screen.postStatus("This item has a fixed/presence-only quantity", 240);
            return true;
        }
        const PokeVault::Inventory::QuantityPolicy::ItemQuantityRule rule =
            PokeBank::UIModel::classicItemQuantityRule(game, pocket, itemId);
        const auto row = PokeBank::UIModel::BagAdd::decideRow(rule, quantity);
        if (row.outcome != PokeBank::UIModel::BagAdd::RowOutcome::EditQuantity) {
            screen.postStatus(row.message, 240);
            return true;
        }
        const auto result = Utils::promptNumber(
            "Edit " + PokeVault::Inventory::displayItemName(game, pocket, itemId),
            static_cast<int>(row.current), static_cast<int>(row.minimum), static_cast<int>(row.maximum));
        if (result.accepted)
            applyMutation(screen, pocket, itemId, static_cast<uint16_t>(result.value));
        return true;
    }
    return true;
}

void drawOverlay(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    if (!isClassicSource(screen)) return;
    auto& state = stateFor(screen);
    if (!state.pickerActive && !state.optionsActive && !state.reviewActive && !state.helpActive &&
        !state.warningActive && !state.discardConfirmActive && !state.removeConfirmActive) return;

    screen.touchButtons.clear();
    // Bag confirmations draw through the shared Gen I-V presentation (UI/SharedBagAddPicker.h:
    // SharedMenuGeometry RemoveConfirm / KeyItemWarning), the same as Gen IV/V.
    if (state.removeConfirmActive || state.warningActive) {
        SharedEditorChrome::beginModal(screen.touchButtons);
        const auto confirmGame = exactGame(screen);
        const std::string name = confirmGame
            ? PokeVault::Inventory::displayItemName(*confirmGame, state.pendingPocket, state.pendingItem)
            : std::string("this item");
        const auto group = !confirmGame ? Enums::GameVersion::FRLG
            : *confirmGame <= ClassicGame::Yellow ? Enums::GameVersion::RBY
            : *confirmGame <= ClassicGame::Crystal ? Enums::GameVersion::GSC : Enums::GameVersion::FRLG;
        if (state.removeConfirmActive) {
            UI::SharedBagAddPicker::drawRemoveConfirm(fb, screen.touchButtons, group, "Remove " + name + "?",
                state.pendingItem, name, stagedQuantity(screen, state.pendingPocket, state.pendingItem),
                "Staged copy only; the original source save stays unchanged.");
            drawNavBar(fb, {{"Y", "Remove"}, {"B", "Cancel"}});
        } else {
            UI::SharedBagAddPicker::drawKeyItemWarning(fb, screen.touchButtons, group, state.pendingItem,
                                                       name, PokeVault::Inventory::pocketName(state.pendingPocket),
                                                       state.pendingQuantity == 0);
            drawNavBar(fb, {{"A", "Confirm"}, {"B", "Cancel"}});
        }
        return;
    }

    const bool compactPicker = state.pickerActive;
    const auto pickerGeometry = classicPickerGrid(screen, state);
    const int width = compactPicker ? pickerGeometry.frame.w : 900;
    const int y = compactPicker ? pickerGeometry.frame.y : 66;
    const int height = compactPicker ? pickerGeometry.frame.h : 610;
    const int x = (fb.getWidth() - width) / 2;
    const int radius = compactPicker ? 16 : 18;
    if (compactPicker) {
        // Match Gen IV's modal ownership: dim underlying source Items and
        // route all new touch controls only to this foreground picker.
        fb.drawFilledRect(0, 0, fb.getWidth(), fb.getHeight() - 46,
                          Color(0, 0, 0, 160));
    } else {
        fb.drawSoftShadow(x, y, width, height, radius);
    }
    fb.drawFilledRoundedRect(x, y, width, height, radius, Colors::Panel);
    fb.drawRoundedRect(x, y, width, height, radius, Colors::Divider, 1);

    const auto game = exactGame(screen);
    const auto pocket = game ? PokeBank::UIModel::classicInventoryPocketAt(*game, screen.selectedCategory)
                             : std::optional<ClassicPocket>{};

    if (state.discardConfirmActive) {
        fb.drawText(x + 30, y + 24, "Discard Staged Changes?", Colors::Text, TextStyle::Heading);
        fb.drawText(x + 30, y + 104, "This discards the shared staged classic-save transaction.", Colors::Text, TextStyle::Body);
        fb.drawText(x + 30, y + 148, "The original source save remains unchanged.", Colors::TextDim, TextStyle::Body);
        drawNavBar(fb, {{"A", "Discard Changes"}, {"B", "Cancel"}});
        return;
    }

    if (state.pickerActive && game && pocket) {
        // Same shared compact grid as Gen IV/V (R2-BUG-006, UI/SharedItemGridPicker.h): sprite,
        // name and held quantity per cell; one rectangle is the highlight and the touch target.
        namespace AddGrid = UI::SharedItemGridPicker;
        const auto& g = pickerGeometry;
        const int pad = AddGrid::kRule.padX;
        const int count = static_cast<int>(state.pickerItems.size());
        fb.drawText(x + pad, y + 8, "Add Item to " + std::string(PokeVault::Inventory::pocketName(*pocket)),
                    Colors::Text, TextStyle::Heading);
        fb.drawText(x + pad, y + 40,
                    std::string("Supported native items | Sort: ") + ItemPickerSort::label(state.pickerSort) +
                        "  |  " + std::to_string(state.pickerRow + 1) + " / " + std::to_string(count),
                    Colors::TextSecondary, TextStyle::Caption);
        // The exact classic game group, so a TM/HM's detail and art use this generation's table.
        const auto group = *game <= ClassicGame::Yellow
            ? Enums::GameVersion::RBY
            : *game <= ClassicGame::Crystal ? Enums::GameVersion::GSC : Enums::GameVersion::FRLG;
        if (state.pickerRow >= 0 && state.pickerRow < count)
            AddGrid::drawMachineDetail(fb, x + pad, y + AddGrid::kDetailTop, g.frame.w - 2 * pad, group,
                PokeVault::Inventory::classicMachineMove(*game, state.pickerItems[static_cast<std::size_t>(state.pickerRow)]));

        const int visibleRows = std::max(1, g.visibleRows);
        state.pickerViewport.ensure(state.pickerRow, count, visibleRows, g.columns);
        const int start = state.pickerViewport.firstRow;   // ensure() above clamps it to the grid
        const auto label = [&](int i) {
            const uint16_t itemId = state.pickerItems[static_cast<std::size_t>(i)];
            const std::string artName = PokeVault::Inventory::displayItemName(*game, *pocket, itemId);
            std::string label = artName;
            return label;
        };
        AddGrid::drawCells(fb, g, state.pickerRow, start, state.pickerScroll.offset(),
            [&](const PokeBank::UIModel::SharedMenuGeometry::Rect& cell, int i, bool focused) {
                // R2-BUG-012 boxed cell (the box IS the published touch rect), as drawMenuCell does.
                SharedBoxedRows::drawBox(fb, SharedBoxedRows::rect(cell.x, cell.y, cell.w, cell.h), {focused});
                SharedBoxedRows::publish(screen.touchButtons, i, cell, {focused});
            },
            label,
            [&](PKSEFramebuffer& fb, const PokeBank::UIModel::SharedMenuGeometry::Rect& sprite, int i) {
                const uint16_t itemId = state.pickerItems[static_cast<std::size_t>(i)];
                const std::string artName = label(i);
                // Native IDs are never interchangeable across generations.
                // A label like TM43 cannot select art without its actual GBA/GBC
                // taught move. PC Items uses the same discs as the TM/HM pocket.
                std::string spriteKey = artName;
                if (PokeVault::Inventory::classicMachineMove(*game, itemId) != 0) {
                    spriteKey = ClassicMachineArtwork::artKey(group, itemId);
                } else if (*game > ClassicGame::Yellow &&
                           *game <= ClassicGame::Crystal) {
                    const std::string slug = ItemSpriteKey::slug(artName);
                    if (ItemSpriteKey::isGen2Mail(slug))
                        spriteKey = "gsc-mail-icon-" + slug;
                }
                ItemPickerArtwork::draw(fb, sprite.right(), sprite.y, sprite.w, spriteKey);
            },
            [&](int i) { return stagedQuantity(screen, *pocket, state.pickerItems[static_cast<std::size_t>(i)]); });
        {
            const auto track = AddGrid::scrollbarTrack(g, SharedScrollbar::kTrackWidth);
            SharedScrollbar::drawAndPublish(fb, state.pickerScrollbar, track.x, track.y,
                SharedScrollbar::kTrackWidth, track.h, AddGrid::scrollModel(g, start));
        }
        drawNavBar(fb, {{"Up/Down", "Navigate"}, {"A", "Add / Select"},
                        {"B", "Cancel"}, {"L/R", "Page"}, {"X", "Sort"}});
        return;
    }

    if (state.helpActive) {
        fb.drawText(x + 30, y + 20, "Inventory Help", Colors::Text, TextStyle::Heading);
        // R2-BUG-013: key rows draw their buttons as glyph badges (ScreenChrome::drawGlyphHelpLine);
        // the notes stay plain text. Rows: ClassicInventoryUIModel classicInventoryHelpLines().
        int rowY = y + 78;
        for (const auto line : PokeBank::UIModel::classicInventoryHelpLines()) {
            drawGlyphHelpLine(fb, x + 34, rowY, std::string(line), Colors::TextDim, TextStyle::Body);
            rowY += 48;
        }
        drawNavBar(fb, {{"B", "Close Help"}});
        return;
    }

    if (state.optionsActive) {
        fb.drawText(x + 30, y + 20, "Inventory Options", Colors::Text, TextStyle::Heading);
        const std::array<std::string, 4> rows{{
            "Review Pending Changes (" + std::to_string(pendingLines(screen).size()) + ")",
            "Export Staged Save", "Discard Staged Changes", "Cancel",
        }};
        int rowY = y + 94;
        for (int i = 0; i < static_cast<int>(rows.size()); ++i) {
            drawRow(fb, x + 30, rowY, width - 60, rows[static_cast<std::size_t>(i)], i == state.optionsRow);
            screen.touchButtons.push_back({i, x + 30, rowY, width - 60, 48});
            rowY += 62;
        }
        drawNavBar(fb, {{"Up/Down", "Choose"}, {"A", "Open"}, {"B", "Close"}});
        return;
    }

    if (state.reviewActive) {
        const auto lines = pendingLines(screen);
        fb.drawText(x + 30, y + 20, "Pending Classic Save Changes", Colors::Text, TextStyle::Heading);
        fb.drawText(x + 30, y + 56, "Review only — use Inventory Options for Export or Discard.",
                    Colors::TextDim, TextStyle::Caption);
        if (lines.empty()) {
            fb.drawText(x + 34, y + 116, "No pending changes.", Colors::TextDim, TextStyle::Body);
        } else {
            constexpr int visibleRows = 7;
            const int count = static_cast<int>(lines.size());
            state.reviewViewport.ensure(state.reviewRow, count, visibleRows);
            const int start = state.reviewViewport.firstRow;
            const int drawFirst = std::max(0, start - 1);
            const int drawLast = std::min(count, start + visibleRows + 1);
            const int liveOffset = state.reviewScroll.offset();
            const int listTop = y + 94;
            constexpr int rowStep = 57;
            fb.setClipRect(x + 30, listTop, width - 60, visibleRows * rowStep);
            for (int i = drawFirst; i < drawLast; ++i) {
                const int rowY = listTop + (i - start) * rowStep + liveOffset;
                drawRow(fb, x + 30, rowY, width - 60,
                        lines[static_cast<std::size_t>(i)], i == state.reviewRow);
                if (rowY + 48 > listTop && rowY < listTop + visibleRows * rowStep)
                    appendClippedTouchButton(screen.touchButtons, i,
                        x + 30, rowY, width - 60, 48,
                        x + 30, listTop, width - 60, visibleRows * rowStep);
            }
            fb.clearClip();
            SharedScrollbar::drawAndPublish(fb, state.reviewScrollbar, x + width - 16, listTop,
                SharedScrollbar::kTrackWidth, visibleRows * rowStep,
                SharedScrollbar::rows(count, visibleRows, start));
        }
        drawNavBar(fb, {{"Up/Down", "Browse"}, {"B", "Back to Options"}});
        if (!state.lastExportDirectory.empty())
            fb.drawText(x + 30, y + height - 40, "Last export: " + state.lastExportDirectory,
                        Colors::TextDim, TextStyle::Caption);
    }
}

} // namespace UI::ClassicInventory
