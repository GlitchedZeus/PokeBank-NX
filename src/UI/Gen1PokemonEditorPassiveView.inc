#include "UI/PokemonViewActions.h"

namespace UI::Gen1PokemonEditor {

bool foundationPassiveViewActive(const TrainerViewScreen& screen) noexcept {
    return ux2StateFor(screen).mode == UX2Mode::View;
}

bool handleFoundationPassiveViewInput(TrainerViewScreen& screen, uint64_t down,
                                      int touchTarget = -1) {
    if (!foundationPassiveViewActive(screen)) return false;

    if (PokeBank::UIModel::passiveViewAction(down & HidNpadButton_B, touchTarget) ==
        PokeBank::UIModel::PassiveViewAction::Back)
        foundationExitMainMode(screen);
    return true;
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
