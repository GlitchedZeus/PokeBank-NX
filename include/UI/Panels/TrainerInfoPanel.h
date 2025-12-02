#ifndef UI_PANELS_TRAINER_INFO_PANEL_H
#define UI_PANELS_TRAINER_INFO_PANEL_H

// Forward declarations
namespace UI {
    class PKSEFramebuffer;
}
namespace Trainer {
    class Trainer;
}

namespace UI {
namespace Panels {
    void drawTrainerInfo(UI::PKSEFramebuffer& fb, Trainer::Trainer& trainer, int x, int y, int width, int height);
}
}

#endif
