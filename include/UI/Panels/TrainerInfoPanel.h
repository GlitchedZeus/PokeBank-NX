#ifndef UI_PANELS_TRAINER_INFO_PANEL_H
#define UI_PANELS_TRAINER_INFO_PANEL_H

// Forward declarations
class PKSEFramebuffer;
class Trainer;

namespace Panels {
    void drawTrainerInfo(PKSEFramebuffer& fb, Trainer& trainer, int x, int y, int width, int height);
}

#endif
