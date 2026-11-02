#include "UI/PokemonViewActions.h"

namespace UI::Gen1PokemonEditor {

bool foundationPassiveViewActive(const TrainerViewScreen& screen) noexcept {
    return ux2StateFor(screen).mode == UX2Mode::View;
}

// Read-only View is navigable like Gen II-V View: D-pad/stick move focus across DETAILS, STATS
// and MOVES, L/R switch panels, a completed tap focuses a field, and nothing ever activates or
// changes a value. B or the Back target leaves.
bool handleFoundationPassiveViewInput(TrainerViewScreen& screen, uint64_t down, int touchTarget,
                                      uint64_t held, int stickX, int stickY) {
    if (!foundationPassiveViewActive(screen)) return false;
    foundationSyncMainMode(screen);
    auto& foundation = foundationStateFor(screen);
    const uint64_t nav = foundation.navigation.apply(
        down, held, stickX, stickY,
        HidNpadButton_Up, HidNpadButton_Down, HidNpadButton_Left, HidNpadButton_Right);
    if (nav & HidNpadButton_Up) foundation.focus = Foundation::moveFocus(foundation.focus, Foundation::Direction::Up);
    if (nav & HidNpadButton_Down) foundation.focus = Foundation::moveFocus(foundation.focus, Foundation::Direction::Down);
    if (nav & HidNpadButton_Left) foundation.focus = Foundation::moveFocus(foundation.focus, Foundation::Direction::Left);
    if (nav & HidNpadButton_Right) foundation.focus = Foundation::moveFocus(foundation.focus, Foundation::Direction::Right);
    if (down & HidNpadButton_L) foundation.focus = Foundation::nextPanel(Foundation::nextPanel(foundation.focus));
    if (down & HidNpadButton_R) foundation.focus = Foundation::nextPanel(foundation.focus);
    (void)foundationFocusTouchTarget(foundation, touchTarget);

    if (PokeBank::UIModel::passiveViewAction(down & HidNpadButton_B, touchTarget) ==
        PokeBank::UIModel::PassiveViewAction::Back)
        foundationExitMainMode(screen);
    return true;
}

bool handleFoundationPassiveViewInput(TrainerViewScreen& screen, uint64_t down,
                                      int touchTarget = -1) {
    return handleFoundationPassiveViewInput(screen, down, touchTarget, 0, 0, 0);
}

void drawFoundationPassiveView(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    if (!foundationPassiveViewActive(screen)) return;

    // View is a distinct read-only mode, but it uses the same page-owned fullscreen
    // presentation contract as Create/Edit. No save/browser framebuffer is left visible.
    drawFullscreenGen1Workspace(screen, fb);
    screen.touchButtons.push_back({PokeBank::UIModel::passiveBackTarget,
                                   16, fb.getHeight() - 68, 190, 58});
}

} // namespace UI::Gen1PokemonEditor
