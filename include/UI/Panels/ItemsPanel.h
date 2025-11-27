#ifndef ITEMS_PANEL_H
#define ITEMS_PANEL_H

// Forward declarations
class PKSEFramebuffer;
class TrainerViewScreen;

namespace Panels {
    void drawItems(TrainerViewScreen& screen, PKSEFramebuffer& fb, int x, int y, int width, int height);
}

#endif // ITEMS_PANEL_H
