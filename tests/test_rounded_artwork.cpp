// Round 11 (HOME card): rounded artwork mask geometry. The region scenery at the top of the HOME
// selected-game card must never paint outside the card's rounded corners (radius 18) or under its
// focus ring. Pure geometry (UI/RoundedArtwork.h); no framebuffer.
#include "UI/RoundedArtwork.h"

#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <utility>

using namespace UI::RoundedArtwork;

static int failures = 0;
#define CHECK(cond)                                                                    \
    do {                                                                               \
        if (!(cond)) {                                                                 \
            std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);       \
            ++failures;                                                                \
        }                                                                              \
    } while (0)

// HOME card geometry is irrelevant to the maths; use a card with several sizes and offsets so the
// checks hold for any region artwork position (handheld and docked draw the same logical 1280x720).
static void checkBandInsideCard(int cardX, int cardY, int cardW, int cardH, int bandH, float radius, int inset) {
    const Band band = topBand(cardX, cardY, cardW, bandH, radius, inset);
    const Rect card{cardX, cardY, cardW, cardH};
    const CornerRadii cardCorners{radius, radius, radius, radius};
    // The inset of the card's border: a pixel of the band must lie inside the card shrunk by `inset`
    // with concentric corners, i.e. never on or past the border/focus ring.
    const Rect inner{cardX + inset, cardY + inset, cardW - 2 * inset, cardH - 2 * inset};
    const float ir = innerRadius(radius, static_cast<float>(inset));
    const CornerRadii innerCorners{ir, ir, ir, ir};
    int bandPixels = 0;
    for (int y = cardY - 2; y < cardY + bandH + 2; ++y)
        for (int x = cardX - 2; x < cardX + cardW + 2; ++x) {
            if (!contains(band.rect, band.radii, x, y)) continue;
            ++bandPixels;
            CHECK(contains(card, cardCorners, x, y));
            CHECK(contains(inner, innerCorners, x, y));
        }
    CHECK(bandPixels > 0);
}

int main() {
    // Concentric inner radius.
    CHECK(innerRadius(18.0f, 2.0f) == 16.0f);
    CHECK(innerRadius(2.0f, 4.0f) == 0.0f);

    // Top band of the HOME card: inset 2, top corners 16, bottom corners square.
    const Band b = topBand(100, 50, 600, 270, 18.0f, 2);
    CHECK(b.rect.x == 102 && b.rect.y == 52 && b.rect.w == 596 && b.rect.h == 268);
    CHECK(b.radii.topLeft == 16.0f && b.radii.topRight == 16.0f);
    CHECK(b.radii.bottomLeft == 0.0f && b.radii.bottomRight == 0.0f);

    // The old rectangular clip painted the card's corner pixel; the rounded band must not.
    const Rect oldClip{102, 52, 596, 268};
    CHECK(contains(oldClip, CornerRadii{}, 102, 52));   // the regression: square corner
    CHECK(!contains(b.rect, b.radii, 102, 52));         // fixed: outside the rounded corner
    CHECK(!contains(b.rect, b.radii, 697, 52));         // top-right likewise
    CHECK(!contains(b.rect, b.radii, 104, 54));
    CHECK(contains(b.rect, b.radii, 400, 52));          // top edge centre is painted
    CHECK(contains(b.rect, b.radii, 102, 319));         // bottom edge square (party strip below)
    CHECK(contains(b.rect, b.radii, 697, 319));
    CHECK(!contains(b.rect, b.radii, 400, 320));        // nothing below the band

    // Every painted pixel stays inside the card and inside its border, at several card geometries.
    checkBandInsideCard(100, 50, 600, 560, 270, 18.0f, 2);
    checkBandInsideCard(0, 0, 37, 80, 30, 18.0f, 2);   // narrower than two radii
    checkBandInsideCard(640, 87, 616, 610, 270, 18.0f, 2);
    checkBandInsideCard(10, 10, 300, 300, 120, 24.0f, 3);

    // Scale-to-cover: aspect preserved, the destination fully covered and centred.
    const Rect dst{102, 52, 596, 268};
    for (const auto& [sw, sh] : {std::pair{1280, 720}, std::pair{400, 400}, std::pair{300, 900}, std::pair{2000, 300}}) {
        const Rect c = coverRect(dst, sw, sh);
        CHECK(c.x <= dst.x && c.y <= dst.y);
        CHECK(c.x + c.w >= dst.x + dst.w - 1 && c.y + c.h >= dst.y + dst.h - 1);
        CHECK(c.w == dst.w || c.h == dst.h);
        CHECK(std::abs((c.x + c.w / 2) - (dst.x + dst.w / 2)) <= 1);
        CHECK(std::abs((c.y + c.h / 2) - (dst.y + dst.h / 2)) <= 1);
        // aspect ratio within one pixel of rounding
        CHECK(std::abs(c.w * sh - c.h * sw) <= std::max(sw, sh));
    }
    // Degenerate input never divides by zero.
    const Rect same = coverRect(dst, 0, 0);
    CHECK(same.x == dst.x && same.w == dst.w);

    if (failures) return 1;
    std::puts("PASS test_rounded_artwork");
    return 0;
}
