// NEW-19 / R2-TRAINER / R2-BUG-002: the Gen I-IV staged-session exit menu (Review / Continue /
// Discard & Exit / Export Copy), the classic counterpart of Gen V's ConfirmExit.
//
// The shell still decides WHEN to ask (B on the hub, + anywhere, B out of the Items shortcut, all
// gated on trainer.hasStagedChanges(), which counts Trainer-only edits). This layer takes the
// question over for a Gen I-IV source (ClassicStagedExit::adopt) so the old two-button
// Discard / Cancel dialog is never shown, and routes each choice to what already exists:
// Review -> that generation's own Review page, Export Copy -> its verified export, Discard & Exit
// -> the shell's leave path. No new export path; the source save is never written.
#include "UI/ClassicStagedExitMenu.h"

namespace UI::ClassicStagedExitOverlay {
namespace {

namespace Exit = PokeBank::UIModel::ClassicStagedExit;

enum class Lane { None, Gen1, Gen2, Gen3, Gen4 };

Lane laneFor(const TrainerViewScreen& screen) noexcept {
    if (Gen1PokemonEditor::isGen1SourceUX(screen)) return Lane::Gen1;
    if (Gen2PokemonEditor::isGSC(screen)) return Lane::Gen2;
    if (Gen3SharedEditorSurface::isGen3Gba(screen)) return Lane::Gen3;
    if (Gen4SharedEditorSurface::isGen4Nds(screen)) return Lane::Gen4;
    return Lane::None;
}

bool openReview(TrainerViewScreen& screen, Lane lane) {
    switch (lane) {
        case Lane::Gen1: return Gen1PokemonEditor::openReviewFromExit(screen);
        case Lane::Gen2: return Gen2PokemonEditor::openReviewFromExit(screen);
        case Lane::Gen3: return Gen3SharedEditorSurface::openReviewFromExit(screen);
        case Lane::Gen4: return Gen4SharedEditorSurface::openReviewFromExit(screen);
        case Lane::None: break;
    }
    return false;
}

bool exportCopy(TrainerViewScreen& screen, Lane lane) {
    switch (lane) {
        case Lane::Gen1: return Gen1PokemonEditor::exportFromExit(screen);
        case Lane::Gen2: return Gen2PokemonEditor::exportFromExit(screen);
        case Lane::Gen3: return Gen3SharedEditorSurface::exportFromExit(screen);
        case Lane::Gen4: return Gen4SharedEditorSurface::exportFromExit(screen);
        case Lane::None: break;
    }
    return false;
}

// Adopts the shell's pending dirty-exit prompt. True while the menu owns the frame.
bool menuOpen(TrainerViewScreen& screen) {
    if (laneFor(screen) == Lane::None) return false;
    return Exit::adopt(Exit::sessionFor(screen), screen.saveConfirmActive,
                       screen.exitingWithUnsavedChanges, screen.exitingViaPlus,
                       screen.itemsShortcutActive, screen.trainer.hasStagedChanges());
}

} // namespace

bool handleInput(TrainerViewScreen& screen, u64 down) {
    if (!menuOpen(screen)) return false;
    auto& session = Exit::sessionFor(screen);
    const Lane lane = laneFor(screen);
    const auto choice = Exit::choiceFor((down & HidNpadButton_A) != 0, (down & HidNpadButton_B) != 0,
                                        (down & HidNpadButton_X) != 0, (down & HidNpadButton_Y) != 0);
    const bool fromItemsShortcut = session.itemsShortcut;
    const bool viaPlus = session.viaPlus;
    switch (Exit::choose(session, choice)) {
        case Exit::Effect::None:
            break;
        case Exit::Effect::Close:
            screen.controllerNavigation.reset();
            break;
        case Exit::Effect::OpenReview:
            if (!openReview(screen, lane)) {
                Exit::reviewUnavailable(session);
                screen.postStatus("Review is unavailable for this source; nothing was changed", 300);
            }
            break;
        case Exit::Effect::Export:
            // The generation's own verified export posts its result; the menu stays so the user
            // can now leave safely or continue editing.
            (void)exportCopy(screen, lane);
            break;
        case Exit::Effect::Exit:
            // Exactly the shell's old Discard & Exit: in-memory staged edits are dropped with the
            // screen; nothing is written anywhere.
            if (fromItemsShortcut) screen.itemsShortcutActive = false;
            screen.exitAfterSave = false;
            if (viaPlus) screen.exitRequested = true;
            screen.goBack = true;
            break;
    }
    return true; // the menu is modal: nothing underneath sees this frame's input
}

bool draw(TrainerViewScreen& screen, PKSEFramebuffer& fb) {
    if (!menuOpen(screen)) return false;
    screen.drawGSCOverlay(fb); // the hub (or Items view) the user is leaving, dimmed below
    fb.drawFilledRect(0, 0, fb.getWidth(), fb.getHeight() - kNavBarH, Color(0, 0, 0, 140));
    // The menu owns the frame's touch; the covered hub's footer is never tappable.
    SharedEditorChrome::beginModal(screen.touchButtons);
    constexpr int w = 960, h = 250;
    const int x = (fb.getWidth() - w) / 2;
    const int y = (fb.getHeight() - kNavBarH - h) / 2;
    fb.drawFilledRoundedRect(x, y, w, h, 18, Colors::Panel);
    fb.drawRoundedRect(x, y, w, h, 18, Colors::Border, 1);
    fb.drawText(x + 26, y + 24, Exit::kTitle, Colors::Text, TextStyle::Heading);
    fb.drawText(x + 26, y + 93, Exit::kMessage, Colors::TextSecondary, TextStyle::Body);
    fb.drawText(x + 26, y + 142, Exit::kSafety, Colors::Accent, TextStyle::Caption);
    // Each choice is its own boxed row; a tap presses exactly that button (R2-BUG-012).
    std::vector<SharedBoxedRows::GlyphChoice> choices;
    std::vector<ControllerHint> keys;
    for (const auto& button : Exit::kButtons) {
        choices.push_back({button.glyph, button.label});
        keys.push_back({button.glyph, button.label});
    }
    SharedBoxedRows::drawGlyphChoices(fb, x + 26, y + h - SharedBoxedRows::kChoiceHeight - 18, w - 52, choices);
    drawNavBar(fb, keys);
    return true;
}

} // namespace UI::ClassicStagedExitOverlay
