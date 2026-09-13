from pathlib import Path
import re


def read(path):
    return Path(path).read_text()


def write(path, text):
    Path(path).write_text(text)


def replace_once(path, old, new):
    text = read(path)
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{path}: expected exactly one literal anchor, found {count}: {old[:100]!r}")
    write(path, text.replace(old, new, 1))


def regex_once(path, pattern, repl, flags=0):
    text = read(path)
    out, count = re.subn(pattern, repl, text, count=1, flags=flags)
    if count != 1:
        raise SystemExit(f"{path}: expected exactly one regex anchor, found {count}: {pattern[:120]!r}")
    write(path, out)


# Shared category model. All UI surfaces and backend mapping consume this order.
replace_once(
    'src/UI/ClassicInventoryUIModel.cpp',
    '''constexpr std::array<ClassicPocket, 5> kGen2Categories{{
    ClassicPocket::TMHM, ClassicPocket::Items, ClassicPocket::KeyItems,
    ClassicPocket::Balls, ClassicPocket::PCItems,
}};''',
    '''constexpr std::array<ClassicPocket, 5> kGen2Categories{{
    ClassicPocket::Items, ClassicPocket::Balls, ClassicPocket::KeyItems,
    ClassicPocket::TMHM, ClassicPocket::PCItems,
}};''')

replace_once(
    'tests/test_classic_inventory_ui_model.cpp',
    '''    assert(classicInventoryCategories(*gold).size() == 5);
    assert(classicInventoryPocketAt(*gold, 0) == ClassicPocket::TMHM);
    assert(classicInventoryPocketAt(*gold, 1) == ClassicPocket::Items);
    assert(classicInventoryPocketAt(*gold, 2) == ClassicPocket::KeyItems);
    assert(classicInventoryPocketAt(*gold, 3) == ClassicPocket::Balls);
    assert(classicInventoryPocketAt(*gold, 4) == ClassicPocket::PCItems);''',
    '''    assert(classicInventoryCategories(*gold).size() == 5);
    assert(classicInventoryPocketAt(*gold, 0) == ClassicPocket::Items);
    assert(classicInventoryPocketAt(*gold, 1) == ClassicPocket::Balls);
    assert(classicInventoryPocketAt(*gold, 2) == ClassicPocket::KeyItems);
    assert(classicInventoryPocketAt(*gold, 3) == ClassicPocket::TMHM);
    assert(classicInventoryPocketAt(*gold, 4) == ClassicPocket::PCItems);''')

# Existing controller: initialize the staged presentation once per opened source and never act under a modal.
replace_once(
    'src/UI/ClassicInventoryOverlay.cpp',
    '''struct OverlayState {
    const TrainerViewScreen* owner = nullptr;
    bool pickerActive = false;''',
    '''struct OverlayState {
    const TrainerViewScreen* owner = nullptr;
    std::string sourceGameId;
    bool presentationInitialized = false;
    bool pickerActive = false;''')

replace_once(
    'src/UI/ClassicInventoryOverlay.cpp',
    '''OverlayState& stateFor(const TrainerViewScreen& screen) {
    static OverlayState state;
    if (state.owner != &screen) {
        state = {};
        state.owner = &screen;
    }
    return state;
}''',
    '''OverlayState& stateFor(const TrainerViewScreen& screen) {
    static OverlayState state;
    if (state.owner != &screen || state.sourceGameId != screen.sourceGameId) {
        state = {};
        state.owner = &screen;
        state.sourceGameId = screen.sourceGameId;
    }
    return state;
}''')

replace_once(
    'src/UI/ClassicInventoryOverlay.cpp',
    '''bool handleInput(TrainerViewScreen& screen, uint64_t down) {
    if (!isClassicSource(screen)) return false;
    auto& state = stateFor(screen);

    if (state.warningActive) {''',
    '''bool handleInput(TrainerViewScreen& screen, uint64_t down) {
    if (!isClassicSource(screen)) return false;
    auto& state = stateFor(screen);

    if (!state.presentationInitialized && refreshPresentation(screen)) {
        state.presentationInitialized = true;
        const auto game = exactGame(screen);
        const int categoryCount = game
            ? static_cast<int>(PokeBank::UIModel::classicInventoryCategories(*game).size()) : 0;
        if (categoryCount > 0)
            screen.selectedCategory = std::clamp(screen.selectedCategory, 0, categoryCount - 1);
        screen.selectedItemIndex = 0;
        screen.currentPage = 0;
    }

    if (state.warningActive) {''')

replace_once(
    'src/UI/ClassicInventoryOverlay.cpp',
    '''    if (!screen.detailViewActive || screen.selectedMode != TrainerViewScreen::ViewMode::Items ||
        screen.helpOverlayActive || screen.details.active || screen.actionSheet.isOpen() ||
        screen.saveConfirmActive || screen.pickerActive || screen.itemEditDialogActive ||
        screen.itemRemoveConfirmActive) return false;''',
    '''    if (!screen.detailViewActive || screen.selectedMode != TrainerViewScreen::ViewMode::Items ||
        screen.helpOverlayActive || screen.details.active || screen.actionSheet.isOpen() ||
        screen.saveConfirmActive || screen.pickerActive || screen.itemEditDialogActive ||
        screen.itemRemoveConfirmActive || screen.statEdit.dialogActive ||
        screen.releaseConfirmActive || screen.storageExitConfirmActive || screen.groupMenuActive ||
        screen.creator.keepConfirmActive || screen.details.discardConfirmActive ||
        screen.gen3ConvertConfirmActive || screen.lgpeTransferConfirmActive) return false;''')

# ItemsPanel uses exact classic game + pocket as its source of truth.
replace_once(
    'src/UI/Panels/ItemsPanel.cpp',
    '#include <array>\n#include <string>',
    '#include <array>\n#include <optional>\n#include <string>')
replace_once(
    'src/UI/Panels/ItemsPanel.cpp',
    '#include "Integration/Gen2/Gen2ReadOnlyInventory.h"\n#include "Enums/GameVersion.h"',
    '#include "Integration/Gen2/Gen2ReadOnlyInventory.h"\n#include "Inventory/ClassicInventoryCatalog.h"\n#include "UI/ClassicInventoryUIModel.h"\n#include "Enums/GameVersion.h"')

replace_once(
    'src/UI/Panels/ItemsPanel.cpp',
    '''        const bool rbySource = screen.legacyReadOnlySource() && isRBYSource(screen.sourceGameId);
        const bool gscSource = screen.legacyReadOnlySource() && isGSCSource(screen.sourceGameId);
        const bool rseSource = screen.legacyReadOnlySource() && isRSESource(screen.sourceGameId);
        const char* pouchName = rbySource
            ? PokeVault::Integration::Gen1::inventoryCategoryName(static_cast<size_t>(screen.selectedCategory))
            : gscSource ? gscPouchDisplayName(screen.selectedCategory)
            : rseSource ? rsePouchDisplayName(screen.selectedCategory)
                        : pouchDisplayName(gameGroup, screen.selectedCategory);''',
    '''        const bool rbySource = screen.legacyReadOnlySource() && isRBYSource(screen.sourceGameId);
        const bool gscSource = screen.legacyReadOnlySource() && isGSCSource(screen.sourceGameId);
        const bool rseSource = screen.legacyReadOnlySource() && isRSESource(screen.sourceGameId);
        const auto classicGame = PokeBank::UIModel::classicInventoryGame(screen.sourceGameId);
        const auto classicPocket = classicGame
            ? PokeBank::UIModel::classicInventoryPocketAt(*classicGame, screen.selectedCategory)
            : std::optional<PokeVault::Inventory::ClassicPocket>{};
        const std::string classicPouchName = classicPocket
            ? std::string(PokeVault::Inventory::pocketName(*classicPocket)) : std::string{};
        const char* pouchName = classicPocket
            ? classicPouchName.c_str()
            : rbySource ? PokeVault::Integration::Gen1::inventoryCategoryName(static_cast<size_t>(screen.selectedCategory))
            : gscSource ? gscPouchDisplayName(screen.selectedCategory)
            : rseSource ? rsePouchDisplayName(screen.selectedCategory)
                        : pouchDisplayName(gameGroup, screen.selectedCategory);''')

replace_once(
    'src/UI/Panels/ItemsPanel.cpp',
    '            fb.drawText(x + 24, y + hH + 30, "Invalid category", Colors::TextDim);',
    '            fb.drawText(x + 24, y + hH + 30, classicGame ? "(None)" : "Invalid category", Colors::TextDim);')
replace_once(
    'src/UI/Panels/ItemsPanel.cpp',
    '            fb.drawText(x + 24, y + hH + 30, "No items in this category", Colors::TextDim);',
    '            fb.drawText(x + 24, y + hH + 30, classicGame ? "(None)" : "No items in this category", Colors::TextDim);')

regex_once(
    'src/UI/Panels/ItemsPanel.cpp',
    r'''            std::string baseName;\n            if \(gameGroup == GameVersion::RBY\) \{.*?            const std::string displayName = Names::machineDisplayLabel\(gameGroup, item\.itemId, baseName\);''',
    '''            std::string displayName;
            if (classicGame && classicPocket) {
                displayName = PokeVault::Inventory::displayItemName(*classicGame, *classicPocket, item.itemId);
            } else {
                std::string baseName;
                if (gameGroup == GameVersion::RBY) {
                    baseName = Names::getItemNameG1(item.itemId);
                } else if (gameGroup == GameVersion::GSC) {
                    baseName = std::string(PokeVault::Integration::Gen2::gen2ItemName(
                        static_cast<uint8_t>(item.itemId)));
                } else if (gameGroup == GameVersion::FRLG) {
                    baseName = Names::getItemNameG3(item.itemId);
                } else {
                    baseName = getItemName(item.itemId);
                }
                displayName = Names::machineDisplayLabel(gameGroup, item.itemId, baseName);
            }''',
    flags=re.S)

# Help bar exposes the actual classic contract, including + Pending Changes.
replace_once(
    'src/UI/TrainerViewScreenBase.inc',
    '#include "UI/Panels/ItemsPanel.h"\n#include "UI/Panels/StoragePanel.h"',
    '#include "UI/Panels/ItemsPanel.h"\n#include "UI/ClassicInventoryOverlay.h"\n#include "UI/Panels/StoragePanel.h"')
replace_once(
    'src/UI/TrainerViewScreenBase.inc',
    '''            if (selectedMode == ViewMode::Items) {
                instructions = "Up/Down: Select  |  A: Edit Amount  |  Y: Add Item  |  X: Remove Item  |  Left/Right: Page  |  L/R: Category  |  B: Back";
            } else if (selectedMode == ViewMode::Boxes) {''',
    '''            if (selectedMode == ViewMode::Items) {
                instructions = ClassicInventory::isClassicSource(*this)
                    ? "Up/Down: Select  |  A: Edit Amount  |  Y: Add Item  |  X: Remove Item  |  L/R: Category  |  +: Pending Changes  |  B: Back"
                    : "Up/Down: Select  |  A: Edit Amount  |  Y: Add Item  |  X: Remove Item  |  Left/Right: Page  |  L/R: Category  |  B: Back";
            } else if (selectedMode == ViewMode::Boxes) {''')

# Live TrainerViewScreen wrapper: classic controller first, then legacy behavior. Inventory X is no longer GSC quick-editor entry.
replace_once(
    'src/UI/TrainerViewScreenGSCOverlay.cpp',
    '#include "UI/Common.h"\n#include "UI/LegacyPresentationRules.h"',
    '#include "UI/Common.h"\n#include "UI/ClassicInventoryOverlay.h"\n#include "UI/LegacyPresentationRules.h"')

regex_once(
    'src/UI/TrainerViewScreenGSCOverlay.cpp',
    r'''bool itemCategoryNavigationAvailable\(const TrainerViewScreen& screen\) noexcept \{.*?\}\n\n''',
    '', flags=re.S)

replace_once(
    'src/UI/TrainerViewScreenGSCOverlay.cpp',
    '''    if (screen.selectedMode == TrainerViewScreen::ViewMode::Items) {
        // Jump near the actual pocket the user was viewing: Items -> Potion, Balls -> Poke Ball.
        state.selectedRow = screen.selectedCategory == 3 ? 3 : 2;
    } else {
        state.selectedRow = 0;
    }''',
    '    state.selectedRow = 0;')

# Scope edits to the trainer/save modal only.
gsc_path = Path('src/UI/TrainerViewScreenGSCOverlay.cpp')
gsc = gsc_path.read_text()
start = gsc.index('void handleStagedEditorInput(')
end = gsc.index('\nvoid drawGSCTrainerCard(', start)
segment = gsc[start:end]
segment, n = re.subn(r'    constexpr int rowCount = 7;', '    constexpr int rowCount = 4;', segment, count=1)
if n != 1:
    raise SystemExit('Gen II staged-editor rowCount anchor missing')
segment, n = re.subn(
    r'''        case 2: \{\n            const int current = editor->itemQuantity\(InventoryPocket::Items, kPotionItemId\);.*?        case 6:\n            editor->discard\(\);\n            screen\.postStatus\("Staged changes discarded; original save was never modified"\);\n            break;''',
    '''        case 2:
            state.review = true;
            state.reviewRow = 0;
            break;
        case 3:
            editor->discard();
            screen.postStatus("Staged changes discarded; original save was never modified");
            break;''',
    segment, count=1, flags=re.S)
if n != 1:
    raise SystemExit('Gen II hardcoded inventory switch cases anchor missing')
gsc = gsc[:start] + segment + gsc[end:]
gsc_path.write_text(gsc)

replace_once(
    'src/UI/TrainerViewScreenGSCOverlay.cpp',
    '''    const std::vector<std::pair<std::string, std::string>> rows{
        {"Trainer Name", editor->trainerName()},
        {"Money", "$" + std::to_string(editor->money())},
        {"Potion", std::to_string(editor->itemQuantity(InventoryPocket::Items, kPotionItemId))},
        {"Poke Ball", std::to_string(editor->itemQuantity(InventoryPocket::Balls, kPokeBallItemId))},
        {"Master Ball", std::to_string(editor->itemQuantity(InventoryPocket::Balls, kMasterBallItemId))},
        {"Review Pending Changes", std::to_string(editor->pendingChanges().size()) + " change(s)"},
        {"Discard Staged Changes", editor->hasPendingChanges() ? "Available" : "Nothing staged"},
    };''',
    '''    const std::vector<std::pair<std::string, std::string>> rows{
        {"Trainer Name", editor->trainerName()},
        {"Money", "$" + std::to_string(editor->money())},
        {"Review Pending Changes", std::to_string(editor->pendingChanges().size()) + " change(s)"},
        {"Discard Staged Changes", editor->hasPendingChanges() ? "Available" : "Nothing staged"},
    };''')

replace_once(
    'src/UI/TrainerViewScreenGSCOverlay.cpp',
    '''void TrainerViewScreen::update(const PadState& pad, const TouchInput& touch) {
    const bool validatedGSC = trainer.getGameGroup() == Enums::GameVersion::GSC && isGSCSource(*this);
    if (validatedGSC) {
        auto& overlay = editorOverlayState(*this);
        const u64 down = padGetButtonsDown(&pad);''',
    '''void TrainerViewScreen::update(const PadState& pad, const TouchInput& touch) {
    const u64 down = padGetButtonsDown(&pad);
    if (ClassicInventory::handleInput(*this, down)) return;

    const bool validatedGSC = trainer.getGameGroup() == Enums::GameVersion::GSC && isGSCSource(*this);
    if (validatedGSC) {
        auto& overlay = editorOverlayState(*this);''')

replace_once(
    'src/UI/TrainerViewScreenGSCOverlay.cpp',
    '''        const bool editorEntrySurface = detailViewActive &&
            (selectedMode == ViewMode::Trainer || selectedMode == ViewMode::Items) &&''',
    '''        const bool editorEntrySurface = detailViewActive &&
            selectedMode == ViewMode::Trainer &&''')

regex_once(
    'src/UI/TrainerViewScreenGSCOverlay.cpp',
    r'''\n    // The accepted large implementation has no GSC switch arm, so its L/R item-tab handling is a.*?\n    \}\n\n    updateLegacyBase\(pad, touch\);''',
    '\n\n    updateLegacyBase(pad, touch);',
    flags=re.S)

replace_once(
    'src/UI/TrainerViewScreenGSCOverlay.cpp',
    '''    if (trainer.getGameGroup() == Enums::GameVersion::GSC && isGSCSource(*this)) {
        if (detailViewActive && selectedMode == ViewMode::Trainer && !helpOverlayActive)
            drawGSCTrainerCard(*this, fb);
        drawStagedEditor(*this, fb);
    }
}''',
    '''    if (trainer.getGameGroup() == Enums::GameVersion::GSC && isGSCSource(*this)) {
        if (detailViewActive && selectedMode == ViewMode::Trainer && !helpOverlayActive)
            drawGSCTrainerCard(*this, fb);
        drawStagedEditor(*this, fb);
    }
    ClassicInventory::drawOverlay(*this, fb);
}''')

# Static truth checks before compilation.
gsc = read('src/UI/TrainerViewScreenGSCOverlay.cpp')
assert 'ClassicInventory::handleInput(*this, down)' in gsc
assert 'ClassicInventory::drawOverlay(*this, fb)' in gsc
assert '(selectedMode == ViewMode::Trainer || selectedMode == ViewMode::Items)' not in gsc
assert 'Potion quantity (0 removes)' not in gsc
assert '{"Potion", std::to_string(editor->itemQuantity' not in gsc
assert '{"Poke Ball", std::to_string(editor->itemQuantity' not in gsc
assert '{"Master Ball", std::to_string(editor->itemQuantity' not in gsc

items = read('src/UI/Panels/ItemsPanel.cpp')
assert 'displayItemName(*classicGame, *classicPocket, item.itemId)' in items
assert 'classicGame ? "(None)" : "Invalid category"' in items
assert 'classicGame ? "(None)" : "No items in this category"' in items

base = read('src/UI/TrainerViewScreenBase.inc')
assert '+: Pending Changes' in base

model = read('src/UI/ClassicInventoryUIModel.cpp')
assert 'ClassicPocket::Items, ClassicPocket::Balls, ClassicPocket::KeyItems' in model
