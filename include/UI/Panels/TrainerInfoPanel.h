#ifndef TRAINER_INFO_PANEL_H
#define TRAINER_INFO_PANEL_H

// Forward declarations
class PKSEFramebuffer;
class Trainer;

namespace Panels {
    void drawTrainerInfo(PKSEFramebuffer& fb, Trainer& trainer, int x, int y, int width, int height);
}

#endif // TRAINER_INFO_PANEL_H
