#ifndef UI_PANELS_MODE_SELECTOR_PANEL_H
#define UI_PANELS_MODE_SELECTOR_PANEL_H

// Forward declarations
namespace UI {
    class PKSEFramebuffer;
    class TrainerViewScreen;
}

namespace UI {
namespace Panels {
    void drawModeSelector(UI::PKSEFramebuffer& fb, int selectedMode, int x, int y, int width, int height);
}
}

#endif
