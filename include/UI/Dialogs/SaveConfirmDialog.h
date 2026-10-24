#ifndef UI_DIALOGS_SAVE_CONFIRM_DIALOG_H
#define UI_DIALOGS_SAVE_CONFIRM_DIALOG_H

// Forward declarations
namespace UI {
    class PKSEFramebuffer;
    class TrainerViewScreen;
}

namespace UI {
namespace Dialogs {
    void drawSaveConfirmDialog(UI::TrainerViewScreen& screen, UI::PKSEFramebuffer& fb);
    // The one Gen I-V staged exit: A Review Changes / B Continue Editing / X Export Copy /
    // Y Discard & Exit (input: UI/ClassicStagedExit.h).
    void drawStagedExitDialog(UI::TrainerViewScreen& screen, UI::PKSEFramebuffer& fb);
}
}

#endif
