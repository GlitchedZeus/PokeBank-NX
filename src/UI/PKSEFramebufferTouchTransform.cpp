#include "UI/PKSEFramebuffer.h"

#include "nanovg.h"

namespace UI {

void PKSEFramebuffer::pushTranslation(float dx, float dy) {
    if (!ensureFrame()) return;
    nvgSave(vg);
    nvgTranslate(vg, dx, dy);
}

void PKSEFramebuffer::popTransform() {
    if (!ensureFrame()) return;
    nvgRestore(vg);
}

} // namespace UI
