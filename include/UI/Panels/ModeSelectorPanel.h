#ifndef UI_PANELS_MODE_SELECTOR_PANEL_H
#define UI_PANELS_MODE_SELECTOR_PANEL_H

// Forward declarations
class PKSEFramebuffer;
class TrainerViewScreen;

namespace Panels {
    void drawModeSelector(PKSEFramebuffer& fb, int selectedMode, int x, int y, int width, int height);
}

#endif
