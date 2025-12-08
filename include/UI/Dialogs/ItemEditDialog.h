#ifndef UI_DIALOGS_ITEM_EDIT_DIALOG_H
#define UI_DIALOGS_ITEM_EDIT_DIALOG_H

// Forward declarations
namespace UI {
    class PKSEFramebuffer;
    class TrainerViewScreen;
}

namespace UI {
namespace Dialogs {
    void drawItemEditDialog(UI::TrainerViewScreen& screen, UI::PKSEFramebuffer& fb);
}
}

#endif
