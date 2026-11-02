// Round 11 (HOME card): the region scenery helper paints the artwork and its scrim as rounded
// shapes concentric with the card (radius 18 -> 16 inside the 2 px border), never as a rectangular
// image through setClipRect. Runs against the host framebuffer recorder (tests/host-ui).
#include "HostUi.h"

#include "UI/CardScenery.h"

#include <cstdio>

static int failures = 0;
#define CHECK(cond)                                                                    \
    do {                                                                               \
        if (!(cond)) {                                                                 \
            std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);       \
            ++failures;                                                                \
        }                                                                              \
    } while (0)

int main() {
    UI::PKSEFramebuffer fb;
    static const unsigned char pixels[4 * 4 * 4] = {};

    HostUi::resetLayout();
    HostUi::clipOn = false;
    const auto band = UI::CardScenery::drawTopBand(fb, 640, 87, 616, 270, 4, 4, pixels);
    CHECK(HostUi::roundedShapes.size() == 2);
    CHECK(HostUi::rectImageDraws == 0);        // no rectangular, unmasked image
    CHECK(HostUi::clipSets == 0);              // no rectangular clip at all
    if (HostUi::roundedShapes.size() == 2) {
        const auto& img = HostUi::roundedShapes[0];
        const auto& scrim = HostUi::roundedShapes[1];
        CHECK(img.image && !scrim.image);
        for (const auto* s : {&img, &scrim}) {
            CHECK(s->x == 642 && s->y == 89 && s->w == 612 && s->h == 268);
            CHECK(s->topLeft == 16.0f && s->topRight == 16.0f);
            CHECK(s->bottomLeft == 0.0f && s->bottomRight == 0.0f);
        }
        CHECK(scrim.rgba == UI::CardScenery::kHomeSceneryScrim.toRGBA8());
    }
    CHECK(band.rect.x == 642 && band.radii.topLeft == 16.0f);

    // Missing artwork draws nothing (the card keeps its plain surface).
    HostUi::resetLayout();
    UI::CardScenery::drawTopBand(fb, 640, 87, 616, 270, 0, 0, nullptr);
    CHECK(HostUi::roundedShapes.empty() && HostUi::rectImageDraws == 0);

    if (failures) return 1;
    std::puts("PASS test_ui_home_card_scenery_host");
    return 0;
}
