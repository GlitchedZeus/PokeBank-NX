#include "Integration/Gen1/Gen1Shiny.h"
#include "Legacy/RBYReadOnlyTrainer.h"
#include "Legacy/GSCReadOnlyTrainer.h"
#include "UI/SpriteManager.h"

#include <algorithm>
#include <cmath>

namespace UI::ClassicPackedMove {
namespace {

enum class CarryGeneration : uint8_t { None, Gen1, Gen2 };

struct CarryState {
    const TrainerViewScreen* owner = nullptr;
    std::string sourceGameId;
    CarryGeneration generation = CarryGeneration::None;
    bool active = false;
    int sourceBox = 0;
    int sourceSlot = 0;
    uint16_t species = 0;
    uint8_t form = 0;
    bool shiny = false;
};

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

bool gridOwnsInput(const TrainerViewScreen& screen) noexcept {
    return screen.selectedMode == TrainerViewScreen::ViewMode::Boxes &&
           screen.detailViewActive &&
           screen.selectedItemIndex >= 0 &&
           !screen.helpOverlayActive &&
           !screen.details.active &&
           !screen.actionSheet.isOpen() &&
           !screen.saveConfirmActive &&
           !screen.pickerActive &&
           !screen.itemEditDialogActive;
}

void setLegacyHoldingPresentation(TrainerViewScreen& screen, bool holding) noexcept {
    // Reuse the accepted Box panel's amber cursor, "HOLDING" footer, header-focus suppression,
    // and X-release suppression. The raw source coordinates are deliberately invalid because the
    // staged source has already been compacted; the carried sprite is drawn by this overlay.
    screen.swapActive = holding;
    screen.swapSourceBox = -1;
    screen.swapSourceSlot = -1;
}

void clearState(TrainerViewScreen& screen, CarryState& state) noexcept {
    setLegacyHoldingPresentation(screen, false);
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

bool beginGen1(TrainerViewScreen& screen, CarryState& state) {
    auto& trainer = static_cast<PokeVault::Legacy::RBYReadOnlyTrainer&>(screen.trainer);
    auto* editor = trainer.stagedPokemon();
    if (!editor) {
        screen.postStatus(trainer.stagedPokemonUnavailableReason().empty()
            ? "Generation I staged Pokemon editing is unavailable"
            : trainer.stagedPokemonUnavailableReason(), 300);
        return true;
    }

    std::string error;
    const auto pokemon = editor->boxedPokemon(
        static_cast<std::size_t>(screen.selectedBoxIndex),
        static_cast<std::size_t>(screen.selectedItemIndex), error);
    if (!pokemon) {
        screen.postStatus(error.empty() ? "There is no Pokemon in this slot to move" : error, 220);
        return true;
    }

    const int sourceBox = screen.selectedBoxIndex;
    const int sourceSlot = screen.selectedItemIndex;
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
    state.species = pokemon->species;
    state.form = 0;
    state.shiny = PokeVault::Integration::Gen1::ShinyDVs::isShiny(
        {pokemon->dvs[1], pokemon->dvs[2], pokemon->dvs[3], pokemon->dvs[4]});
    setLegacyHoldingPresentation(screen, true);
    screen.postStatus("Pokemon picked up - Y places it, B cancels", 220);
    return true;
}

bool beginGen2(TrainerViewScreen& screen, CarryState& state) {
    auto& trainer = static_cast<PokeVault::Legacy::GSCReadOnlyTrainer&>(screen.trainer);
    auto* editor = trainer.stagedEditor();
    if (!editor) {
        screen.postStatus(trainer.stagedEditingUnavailableReason().empty()
            ? "Generation II staged Pokemon editing is unavailable"
            : trainer.stagedEditingUnavailableReason(), 300);
        return true;
    }

    std::string error;
    const auto pokemon = editor->boxedPokemon(
        static_cast<std::size_t>(screen.selectedBoxIndex),
        static_cast<std::size_t>(screen.selectedItemIndex), error);
    if (!pokemon) {
        screen.postStatus(error.empty() ? "There is no Pokemon in this slot to move" : error, 220);
        return true;
    }

    const int sourceBox = screen.selectedBoxIndex;
    const int sourceSlot = screen.selectedItemIndex;
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
    state.species = pokemon->species;
    state.form = pokemon->form;
    state.shiny = pokemon->shiny;
    setLegacyHoldingPresentation(screen, true);
    screen.postStatus("Pokemon picked up - Y places it, B cancels", 220);
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
            ok = editor->placePackedMove(
                static_cast<std::size_t>(screen.selectedBoxIndex),
                static_cast<std::size_t>(screen.selectedItemIndex),
                placedSlot, error);
            if (ok) refreshGen1(screen, error);
        }
    } else if (state.generation == CarryGeneration::Gen2) {
        auto& trainer = static_cast<PokeVault::Legacy::GSCReadOnlyTrainer&>(screen.trainer);
        if (auto* editor = trainer.stagedEditor()) {
            ok = editor->placePackedMove(
                static_cast<std::size_t>(screen.selectedBoxIndex),
                static_cast<std::size_t>(screen.selectedItemIndex),
                placedSlot, error);
            if (ok) refreshGen2(screen, error);
        }
    }

    if (!ok) {
        screen.postStatus(error.empty() ? "That destination cannot accept the carried Pokemon" : error, 320);
        // Full destinations and other rejected drops deliberately keep the carry live.
        setLegacyHoldingPresentation(screen, true);
        return true;
    }

    screen.selectedItemIndex = static_cast<int>(placedSlot);
    screen.postStatus("Pokemon moved with packed box ordering", 220);
    clearState(screen, state);
    return true;
}

} // namespace

bool handleInput(TrainerViewScreen& screen, uint64_t down, const TouchInput& touch) {
    auto& state = stateFor(screen);

    if (state.active) {
        setLegacyHoldingPresentation(screen, true);

        if (down & HidNpadButton_B) return cancel(screen, state);
        if (down & HidNpadButton_Y) return place(screen, state);

        // Exit is allowed, but never with a half-complete carry: restore first, then let the
        // accepted exit path process the + press in the same frame.
        if (down & HidNpadButton_Plus) {
            cancel(screen, state);
            return false;
        }

        // A / X must not open Actions/Create/Release while a Pokemon is physically in hand.
        if (down & (HidNpadButton_A | HidNpadButton_X)) {
            screen.postStatus("Place with Y or cancel with B first", 180);
            return true;
        }

        // During a carry, tapping a box cell is cursor movement only. This preserves touch
        // navigation without converting a second tap into A/Actions in the legacy handler.
        const int tapped = screen.touchedButtonId(touch);
        if (tapped >= 0 && tapped < static_cast<int>(screen.trainer.getSlotsPerBox())) {
            screen.selectedItemIndex = tapped;
            return true;
        }
        if (tapped == 1002 || tapped == 2000) return true;
        return false;
    }

    if (!(down & HidNpadButton_Y) || !gridOwnsInput(screen)) return false;
    if (isGen1(screen)) return beginGen1(screen, state);
    if (isGen2(screen)) return beginGen2(screen, state);
    return false;
}

void draw(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    auto& state = stateFor(screen);
    if (!state.active ||
        screen.selectedMode != TrainerViewScreen::ViewMode::Boxes ||
        !screen.detailViewActive ||
        screen.selectedItemIndex < 0) return;

    const int selected = screen.selectedItemIndex;
    const TrainerViewScreen::TouchButton* cell = nullptr;
    for (const auto& button : screen.touchButtons) {
        if (button.id == selected) {
            cell = &button;
            break;
        }
    }
    if (!cell) return;

    const int cx = cell->x + cell->w / 2;
    const int cy = cell->y + cell->h / 2;
    const int radius = std::max(18, std::min(cell->w, cell->h) / 2 - 6);
    const int bob = static_cast<int>(std::sin(fb.getTimeSeconds() * 3.4) * 3.0);

    fb.drawFilledEllipse(cx + radius / 4, cy + radius + 11,
                         std::max(12, radius - 5), std::max(4, radius / 5),
                         Color(0, 0, 0, 90));

    if (auto* sprite = SpriteManager::getIconSprite(state.species, state.form, state.shiny);
        sprite && sprite->data) {
        const int size = std::max(34, radius * 2);
        fb.drawImageScaled(cx - size / 2, cy - size / 2 + bob,
                           sprite->width, sprite->height, size, size,
                           sprite->data, sprite->channels);
    }

    // Redraw the accepted pointer over the carried sprite so the hand/cursor remains obvious.
    fb.drawPointerCursor(cx, cy - radius - 3 + bob, kGridCursorH, Colors::Primary);
    drawNavBar(fb, {{"Y", "Drop"}, {"L/R", "Box"}, {"B", "Cancel"}});
}

} // namespace UI::ClassicPackedMove
