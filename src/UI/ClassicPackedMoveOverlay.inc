#include "Integration/Gen1/Gen1Shiny.h"
#include "Legacy/RBYReadOnlyTrainer.h"
#include "Legacy/GSCReadOnlyTrainer.h"
#include "UI/ClassicPackedMultiSelect.h"
#include "UI/SpriteManager.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace UI::ClassicPackedMove {
namespace {

namespace Multi = PokeBank::UIModel::ClassicPackedMove;
enum class CarryGeneration : uint8_t { None, Gen1, Gen2 };

struct CarriedVisual {
    uint16_t species = 0;
    uint8_t form = 0;
    bool shiny = false;
};

struct CarryState {
    const TrainerViewScreen* owner = nullptr;
    std::string sourceGameId;
    CarryGeneration generation = CarryGeneration::None;
    bool active = false;
    int sourceBox = 0;
    int sourceSlot = 0;
    std::vector<CarriedVisual> visuals;

    bool yPending = false;
    int yFrames = 0;
    int pendingBox = 0;
    int pendingSlot = 0;

    bool selecting = false;
    int selectionBox = 0;
    int selectionAnchor = 0;
    int selectionCursor = 0;
};

constexpr int kMultiHoldFrames = 8;

CarryState& stateFor(TrainerViewScreen& screen) {
    static CarryState state;
    if (state.owner != &screen || state.sourceGameId != screen.sourceGameId) {
        state = {};
        state.owner = &screen;
        state.sourceGameId = screen.sourceGameId;
    }
    return state;
}

bool isGen1(const TrainerViewScreen& screen) noexcept {
    return screen.sourceGameId == "red_gb" ||
           screen.sourceGameId == "blue_gb" ||
           screen.sourceGameId == "yellow_gb";
}

bool isGen2(const TrainerViewScreen& screen) noexcept {
    return screen.sourceGameId == "gold_gbc" ||
           screen.sourceGameId == "silver_gbc" ||
           screen.sourceGameId == "crystal_gbc";
}

bool classicSource(const TrainerViewScreen& screen) noexcept {
    return isGen1(screen) || isGen2(screen);
}

bool gridOwnsInput(const TrainerViewScreen& screen) noexcept {
    return classicSource(screen) &&
           screen.selectedMode == TrainerViewScreen::ViewMode::Boxes &&
           screen.detailViewActive &&
           screen.selectedItemIndex >= 0 &&
           !screen.helpOverlayActive &&
           !screen.details.active &&
           !screen.actionSheet.isOpen() &&
           !screen.saveConfirmActive &&
           !screen.pickerActive &&
           !screen.itemEditDialogActive;
}

int classicCapacity(TrainerViewScreen& screen) {
    if (isGen1(screen)) {
        auto& trainer = static_cast<PokeVault::Legacy::RBYReadOnlyTrainer&>(screen.trainer);
        return trainer.stagedPokemon() ? 20 : 0;
    }
    if (isGen2(screen)) {
        auto& trainer = static_cast<PokeVault::Legacy::GSCReadOnlyTrainer&>(screen.trainer);
        return trainer.stagedEditor() ? static_cast<int>(trainer.stagedEditor()->metadata().boxCapacity) : 0;
    }
    return 0;
}

void setLegacyHoldingPresentation(TrainerViewScreen& screen, bool holding) noexcept {
    screen.swapActive = holding;
    screen.swapSourceBox = -1;
    screen.swapSourceSlot = -1;
}

void clearTransientSelection(TrainerViewScreen& screen, CarryState& state) noexcept {
    state.yPending = false;
    state.yFrames = 0;
    state.selecting = false;
    screen.currentlySelecting = false;
}

void clearState(TrainerViewScreen& screen, CarryState& state) noexcept {
    setLegacyHoldingPresentation(screen, false);
    screen.currentlySelecting = false;
    const auto owner = state.owner;
    const auto sourceGameId = state.sourceGameId;
    state = {};
    state.owner = owner;
    state.sourceGameId = sourceGameId;
}

bool refreshGen1(TrainerViewScreen& screen, std::string& error) {
    auto& trainer = static_cast<PokeVault::Legacy::RBYReadOnlyTrainer&>(screen.trainer);
    return trainer.refreshBoxesFromStagedPokemon(error);
}

bool refreshGen2(TrainerViewScreen& screen, std::string& error) {
    auto& trainer = static_cast<PokeVault::Legacy::GSCReadOnlyTrainer&>(screen.trainer);
    return trainer.refreshStagedBoxPresentation(error);
}

std::optional<CarriedVisual> visualGen1(TrainerViewScreen& screen, int box, int slot) {
    auto& trainer = static_cast<PokeVault::Legacy::RBYReadOnlyTrainer&>(screen.trainer);
    auto* editor = trainer.stagedPokemon();
    if (!editor) return {};
    std::string error;
    auto pokemon = editor->boxedPokemon(static_cast<std::size_t>(box), static_cast<std::size_t>(slot), error);
    if (!pokemon) return {};
    CarriedVisual out;
    out.species = pokemon->species;
    out.shiny = PokeVault::Integration::Gen1::ShinyDVs::isShiny(
        {pokemon->dvs[1], pokemon->dvs[2], pokemon->dvs[3], pokemon->dvs[4]});
    return out;
}

std::optional<CarriedVisual> visualGen2(TrainerViewScreen& screen, int box, int slot) {
    auto& trainer = static_cast<PokeVault::Legacy::GSCReadOnlyTrainer&>(screen.trainer);
    auto* editor = trainer.stagedEditor();
    if (!editor) return {};
    std::string error;
    auto pokemon = editor->boxedPokemon(static_cast<std::size_t>(box), static_cast<std::size_t>(slot), error);
    if (!pokemon) return {};
    return CarriedVisual{pokemon->species, pokemon->form, pokemon->shiny};
}

bool beginGen1(TrainerViewScreen& screen, CarryState& state, int sourceBox, int sourceSlot) {
    auto& trainer = static_cast<PokeVault::Legacy::RBYReadOnlyTrainer&>(screen.trainer);
    auto* editor = trainer.stagedPokemon();
    if (!editor) {
        screen.postStatus(trainer.stagedPokemonUnavailableReason().empty()
            ? "Generation I staged Pokemon editing is unavailable"
            : trainer.stagedPokemonUnavailableReason(), 300);
        return true;
    }
    const auto visual = visualGen1(screen, sourceBox, sourceSlot);
    if (!visual) {
        screen.postStatus("There is no Pokemon in this slot to move", 220);
        return true;
    }
    std::string error;
    if (!editor->beginPackedMove(static_cast<std::size_t>(sourceBox),
                                 static_cast<std::size_t>(sourceSlot), error)) {
        screen.postStatus(error.empty() ? "Could not pick up this Pokemon" : error, 300);
        return true;
    }
    if (!refreshGen1(screen, error)) {
        std::string cancelError;
        editor->cancelPackedMove(cancelError);
        refreshGen1(screen, cancelError);
        screen.postStatus(error.empty() ? "Could not refresh the staged Generation I box" : error, 320);
        return true;
    }
    state.active = true;
    state.generation = CarryGeneration::Gen1;
    state.sourceBox = sourceBox;
    state.sourceSlot = sourceSlot;
    state.visuals = {*visual};
    clearTransientSelection(screen, state);
    setLegacyHoldingPresentation(screen, true);
    screen.postStatus("Pokemon picked up - Y places it, B cancels", 220);
    return true;
}

bool beginGen2(TrainerViewScreen& screen, CarryState& state, int sourceBox, int sourceSlot) {
    auto& trainer = static_cast<PokeVault::Legacy::GSCReadOnlyTrainer&>(screen.trainer);
    auto* editor = trainer.stagedEditor();
    if (!editor) {
        screen.postStatus(trainer.stagedEditingUnavailableReason().empty()
            ? "Generation II staged Pokemon editing is unavailable"
            : trainer.stagedEditingUnavailableReason(), 300);
        return true;
    }
    const auto visual = visualGen2(screen, sourceBox, sourceSlot);
    if (!visual) {
        screen.postStatus("There is no Pokemon in this slot to move", 220);
        return true;
    }
    std::string error;
    if (!editor->beginPackedMove(static_cast<std::size_t>(sourceBox),
                                 static_cast<std::size_t>(sourceSlot), error)) {
        screen.postStatus(error.empty() ? "Could not pick up this Pokemon" : error, 300);
        return true;
    }
    if (!refreshGen2(screen, error)) {
        std::string cancelError;
        editor->cancelPackedMove(cancelError);
        refreshGen2(screen, cancelError);
        screen.postStatus(error.empty() ? "Could not refresh the staged Generation II box" : error, 320);
        return true;
    }
    state.active = true;
    state.generation = CarryGeneration::Gen2;
    state.sourceBox = sourceBox;
    state.sourceSlot = sourceSlot;
    state.visuals = {*visual};
    clearTransientSelection(screen, state);
    setLegacyHoldingPresentation(screen, true);
    screen.postStatus("Pokemon picked up - Y places it, B cancels", 220);
    return true;
}

std::vector<std::size_t> occupiedSelectionGen1(TrainerViewScreen& screen, int box,
                                                const Multi::SelectionSlots& rectangle,
                                                std::vector<CarriedVisual>& visuals) {
    std::vector<std::size_t> slots;
    auto& trainer = static_cast<PokeVault::Legacy::RBYReadOnlyTrainer&>(screen.trainer);
    auto* editor = trainer.stagedPokemon();
    if (!editor) return slots;
    for (std::size_t i = 0; i < rectangle.count; ++i) {
        std::string error;
        const int slot = rectangle[i];
        auto pokemon = editor->boxedPokemon(static_cast<std::size_t>(box), static_cast<std::size_t>(slot), error);
        if (!pokemon) continue;
        slots.push_back(static_cast<std::size_t>(slot));
        CarriedVisual visual;
        visual.species = pokemon->species;
        visual.shiny = PokeVault::Integration::Gen1::ShinyDVs::isShiny(
            {pokemon->dvs[1], pokemon->dvs[2], pokemon->dvs[3], pokemon->dvs[4]});
        visuals.push_back(visual);
    }
    return slots;
}

std::vector<std::size_t> occupiedSelectionGen2(TrainerViewScreen& screen, int box,
                                                const Multi::SelectionSlots& rectangle,
                                                std::vector<CarriedVisual>& visuals) {
    std::vector<std::size_t> slots;
    auto& trainer = static_cast<PokeVault::Legacy::GSCReadOnlyTrainer&>(screen.trainer);
    auto* editor = trainer.stagedEditor();
    if (!editor) return slots;
    for (std::size_t i = 0; i < rectangle.count; ++i) {
        std::string error;
        const int slot = rectangle[i];
        auto pokemon = editor->boxedPokemon(static_cast<std::size_t>(box), static_cast<std::size_t>(slot), error);
        if (!pokemon) continue;
        slots.push_back(static_cast<std::size_t>(slot));
        visuals.push_back({pokemon->species, pokemon->form, pokemon->shiny});
    }
    return slots;
}

bool beginSelectedGroup(TrainerViewScreen& screen, CarryState& state) {
    const int capacity = classicCapacity(screen);
    const auto rectangle = Multi::rectangleSlots(state.selectionAnchor, state.selectionCursor, capacity);
    std::vector<CarriedVisual> visuals;
    std::vector<std::size_t> slots;
    if (isGen1(screen)) slots = occupiedSelectionGen1(screen, state.selectionBox, rectangle, visuals);
    else slots = occupiedSelectionGen2(screen, state.selectionBox, rectangle, visuals);

    if (slots.empty()) {
        screen.postStatus("The selected rectangle contains no Pokemon", 220);
        clearTransientSelection(screen, state);
        return true;
    }

    std::string error;
    bool ok = false;
    if (isGen1(screen)) {
        auto& trainer = static_cast<PokeVault::Legacy::RBYReadOnlyTrainer&>(screen.trainer);
        if (auto* editor = trainer.stagedPokemon()) {
            ok = editor->beginPackedGroupMove(static_cast<std::size_t>(state.selectionBox), slots, error);
            if (ok) ok = refreshGen1(screen, error);
        }
        if (ok) state.generation = CarryGeneration::Gen1;
    } else {
        auto& trainer = static_cast<PokeVault::Legacy::GSCReadOnlyTrainer&>(screen.trainer);
        if (auto* editor = trainer.stagedEditor()) {
            ok = editor->beginPackedGroupMove(static_cast<std::size_t>(state.selectionBox), slots, error);
            if (ok) ok = refreshGen2(screen, error);
        }
        if (ok) state.generation = CarryGeneration::Gen2;
    }

    if (!ok) {
        screen.postStatus(error.empty() ? "Could not pick up the selected Pokemon" : error, 320);
        clearTransientSelection(screen, state);
        return true;
    }

    state.active = true;
    state.sourceBox = state.selectionBox;
    state.sourceSlot = state.selectionAnchor;
    state.visuals = std::move(visuals);
    clearTransientSelection(screen, state);
    setLegacyHoldingPresentation(screen, true);
    screen.postStatus(std::to_string(state.visuals.size()) + " Pokemon picked up - Y places group, B cancels", 260);
    return true;
}

bool cancel(TrainerViewScreen& screen, CarryState& state) {
    std::string error;
    bool ok = false;
    if (state.generation == CarryGeneration::Gen1) {
        auto& trainer = static_cast<PokeVault::Legacy::RBYReadOnlyTrainer&>(screen.trainer);
        if (auto* editor = trainer.stagedPokemon())
            ok = editor->cancelPackedMove(error) && refreshGen1(screen, error);
    } else if (state.generation == CarryGeneration::Gen2) {
        auto& trainer = static_cast<PokeVault::Legacy::GSCReadOnlyTrainer&>(screen.trainer);
        if (auto* editor = trainer.stagedEditor())
            ok = editor->cancelPackedMove(error) && refreshGen2(screen, error);
    }

    if (ok) {
        screen.selectedBoxIndex = state.sourceBox;
        screen.selectedItemIndex = state.sourceSlot;
        screen.postStatus("Move cancelled - staged ordering restored", 220);
        clearState(screen, state);
    } else {
        screen.postStatus(error.empty() ? "Could not restore the carried Pokemon" : error, 360);
    }
    return true;
}

bool place(TrainerViewScreen& screen, CarryState& state) {
    if (screen.selectedItemIndex < 0) {
        screen.postStatus("Choose a box slot before placing the carried Pokemon", 220);
        return true;
    }

    std::string error;
    std::size_t placedSlot = 0;
    bool ok = false;
    if (state.generation == CarryGeneration::Gen1) {
        auto& trainer = static_cast<PokeVault::Legacy::RBYReadOnlyTrainer&>(screen.trainer);
        if (auto* editor = trainer.stagedPokemon()) {
            ok = editor->placePackedGroupMove(
                static_cast<std::size_t>(screen.selectedBoxIndex),
                static_cast<std::size_t>(screen.selectedItemIndex), placedSlot, error);
            if (ok) ok = refreshGen1(screen, error);
        }
    } else if (state.generation == CarryGeneration::Gen2) {
        auto& trainer = static_cast<PokeVault::Legacy::GSCReadOnlyTrainer&>(screen.trainer);
        if (auto* editor = trainer.stagedEditor()) {
            ok = editor->placePackedGroupMove(
                static_cast<std::size_t>(screen.selectedBoxIndex),
                static_cast<std::size_t>(screen.selectedItemIndex), placedSlot, error);
            if (ok) ok = refreshGen2(screen, error);
        }
    }

    if (!ok) {
        screen.postStatus(error.empty() ? "That destination cannot accept the carried Pokemon" : error, 320);
        setLegacyHoldingPresentation(screen, true);
        return true;
    }

    screen.selectedItemIndex = static_cast<int>(placedSlot);
    screen.postStatus(state.visuals.size() == 1
        ? "Pokemon moved with packed box ordering"
        : std::to_string(state.visuals.size()) + " Pokemon moved in deterministic packed order", 260);
    clearState(screen, state);
    return true;
}

void resizeSelection(TrainerViewScreen& screen, CarryState& state, uint64_t down) {
    const int capacity = classicCapacity(screen);
    int next = state.selectionCursor;
    if (down & HidNpadButton_Up) next = Multi::moveCursor(next, Multi::Direction::Up, capacity);
    if (down & HidNpadButton_Down) next = Multi::moveCursor(next, Multi::Direction::Down, capacity);
    if (down & HidNpadButton_Left) next = Multi::moveCursor(next, Multi::Direction::Left, capacity);
    if (down & HidNpadButton_Right) next = Multi::moveCursor(next, Multi::Direction::Right, capacity);
    state.selectionCursor = next;
    screen.selectedItemIndex = next;
}

bool slotOccupiedForFooter(TrainerViewScreen& screen) {
    if (screen.selectedBoxIndex < 0 || screen.selectedItemIndex < 0 ||
        screen.selectedBoxIndex >= static_cast<int>(screen.trainer.boxes.size())) return false;
    const auto& box = screen.trainer.boxes[static_cast<std::size_t>(screen.selectedBoxIndex)];
    if (screen.selectedItemIndex >= static_cast<int>(box.size())) return false;
    const auto& pokemon = box[static_cast<std::size_t>(screen.selectedItemIndex)];
    return pokemon && pokemon->speciesID() != 0;
}

const TrainerViewScreen::TouchButton* selectedCell(TrainerViewScreen& screen) {
    for (const auto& button : screen.touchButtons)
        if (button.id == screen.selectedItemIndex) return &button;
    return nullptr;
}

void drawVisual(PKSEFramebuffer& fb, const CarriedVisual& visual, int cx, int cy, int size) {
    if (auto* sprite = SpriteManager::getIconSprite(visual.species, visual.form, visual.shiny);
        sprite && sprite->data) {
        fb.drawImageScaled(cx - size / 2, cy - size / 2,
                           sprite->width, sprite->height, size, size,
                           sprite->data, sprite->channels);
    }
}

} // namespace

bool handleInput(TrainerViewScreen& screen, uint64_t down, uint64_t held, const TouchInput& touch) {
    auto& state = stateFor(screen);

    if (state.active) {
        setLegacyHoldingPresentation(screen, true);
        if (down & HidNpadButton_B) return cancel(screen, state);
        if (down & HidNpadButton_Y) return place(screen, state);
        if (down & HidNpadButton_Plus) {
            cancel(screen, state);
            return false;
        }
        if (down & (HidNpadButton_A | HidNpadButton_X)) {
            screen.postStatus("Place with Y or cancel with B first", 180);
            return true;
        }
        const int tapped = screen.touchedButtonId(touch);
        if (tapped >= 0 && tapped < classicCapacity(screen)) {
            screen.selectedItemIndex = tapped;
            return true;
        }
        if (tapped == 1002 || tapped == 2000) return true;
        return false; // D-pad and L/R keep using the accepted box navigation path.
    }

    if (state.selecting) {
        screen.currentlySelecting = true;
        if (down & HidNpadButton_B) {
            screen.selectedBoxIndex = state.selectionBox;
            screen.selectedItemIndex = state.selectionAnchor;
            clearTransientSelection(screen, state);
            screen.postStatus("Multi-select cancelled; staged boxes unchanged", 200);
            return true;
        }
        if (down & HidNpadButton_Plus) {
            clearTransientSelection(screen, state);
            return false;
        }
        if (!(held & HidNpadButton_Y)) return beginSelectedGroup(screen, state);
        if (down & (HidNpadButton_L | HidNpadButton_R | HidNpadButton_A | HidNpadButton_X)) {
            screen.postStatus("Hold Y and use the D-pad to resize; release Y to pick up", 180);
            return true;
        }
        resizeSelection(screen, state, down);
        return true;
    }

    if (state.yPending) {
        if (down & HidNpadButton_B) {
            clearTransientSelection(screen, state);
            return true;
        }
        if (down & HidNpadButton_Plus) {
            clearTransientSelection(screen, state);
            return false;
        }
        if (held & HidNpadButton_Y) {
            ++state.yFrames;
            if (state.yFrames >= kMultiHoldFrames) {
                state.selecting = true;
                state.yPending = false;
                state.selectionBox = state.pendingBox;
                state.selectionAnchor = state.pendingSlot;
                state.selectionCursor = state.pendingSlot;
                screen.selectedBoxIndex = state.selectionBox;
                screen.selectedItemIndex = state.selectionCursor;
                screen.currentlySelecting = true;
                screen.postStatus("Multi-select: hold Y, resize with D-pad, release Y to pick up", 260);
            }
            return true;
        }

        const int box = state.pendingBox;
        const int slot = state.pendingSlot;
        state.yPending = false;
        state.yFrames = 0;
        if (isGen1(screen)) return beginGen1(screen, state, box, slot);
        if (isGen2(screen)) return beginGen2(screen, state, box, slot);
        return false;
    }

    if (!(down & HidNpadButton_Y) || !gridOwnsInput(screen)) return false;
    const int capacity = classicCapacity(screen);
    if (screen.selectedItemIndex < 0 || screen.selectedItemIndex >= capacity) return true;
    state.yPending = true;
    state.yFrames = 0;
    state.pendingBox = screen.selectedBoxIndex;
    state.pendingSlot = screen.selectedItemIndex;
    return true;
}

void draw(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    auto& state = stateFor(screen);

    if (state.selecting && gridOwnsInput(screen)) {
        const int capacity = classicCapacity(screen);
        const auto rectangle = Multi::rectangleSlots(state.selectionAnchor, state.selectionCursor, capacity);
        for (std::size_t i = 0; i < rectangle.count; ++i) {
            for (const auto& button : screen.touchButtons) {
                if (button.id != rectangle[i]) continue;
                fb.drawRoundedRect(button.x + 2, button.y + 2, button.w - 4, button.h - 4,
                                   10, Colors::Primary, 3);
                break;
            }
        }
        drawNavBar(fb, {{"Y", "Hold / Release Pick Up"}, {"D-pad", "Resize Selection"}, {"B", "Cancel"}});
        return;
    }

    if (state.active && screen.selectedMode == TrainerViewScreen::ViewMode::Boxes &&
        screen.detailViewActive && screen.selectedItemIndex >= 0) {
        const auto* cell = selectedCell(screen);
        if (!cell) return;
        const int cx = cell->x + cell->w / 2;
        const int cy = cell->y + cell->h / 2;
        const int radius = std::max(18, std::min(cell->w, cell->h) / 2 - 6);
        const int bob = static_cast<int>(std::sin(fb.getTimeSeconds() * 3.4) * 3.0);
        fb.drawFilledEllipse(cx + radius / 4, cy + radius + 11,
                             std::max(12, radius - 5), std::max(4, radius / 5), Color(0, 0, 0, 90));

        if (state.visuals.size() <= 1) {
            if (!state.visuals.empty()) drawVisual(fb, state.visuals.front(), cx, cy + bob, std::max(34, radius * 2));
        } else {
            const std::size_t shown = std::min<std::size_t>(3, state.visuals.size());
            const int size = std::max(30, radius * 3 / 2);
            for (std::size_t i = 0; i < shown; ++i)
                drawVisual(fb, state.visuals[i], cx - 14 + static_cast<int>(i) * 14,
                           cy - 7 - static_cast<int>(i) * 5 + bob, size);
            fb.drawPill(cx + radius / 2, cy + radius / 2, 66, 26, Colors::Primary);
            fb.drawText(cx + radius / 2 + 10, cy + radius / 2 + 5,
                        std::to_string(state.visuals.size()) + " Pokemon", Colors::PrimaryText, TextStyle::Caption);
        }
        fb.drawPointerCursor(cx, cy - radius - 3 + bob, kGridCursorH, Colors::Primary);
        drawNavBar(fb, {{"Y", state.visuals.size() == 1 ? "Drop" : "Drop Group"},
                        {"L/R", "Box"}, {"B", "Cancel"}});
        return;
    }

    // Override the legacy box footer last so Y is never advertised as a destructive shortcut.
    if (gridOwnsInput(screen)) {
        const bool occupied = slotOccupiedForFooter(screen);
        if (occupied)
            drawNavBar(fb, {{"A", "Actions"}, {"Y", "Move / Hold Multi"}, {"L/R", "Box"}, {"B", "Back"}});
        else
            drawNavBar(fb, {{"A", "Actions"}, {"X", "Add"}, {"Y", "Hold Multi"}, {"L/R", "Box"}, {"B", "Back"}});
    }
}

} // namespace UI::ClassicPackedMove
