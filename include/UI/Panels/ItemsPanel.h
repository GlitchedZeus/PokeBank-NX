#ifndef UI_PANELS_ITEMS_PANEL_H
#define UI_PANELS_ITEMS_PANEL_H

// Forward declarations
class PKSEFramebuffer;
class TrainerViewScreen;

namespace Panels {
    void drawItems(TrainerViewScreen& screen, PKSEFramebuffer& fb, int x, int y, int width, int height);
}

#endif
