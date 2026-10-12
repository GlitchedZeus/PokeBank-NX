#ifndef UI_PRODUCT_CHROME_H
#define UI_PRODUCT_CHROME_H

#include <string>

#include "UI/ScreenChrome.h"

namespace UI {

    // Product-facing title bar used by the approved PokeBank NX home and its secondary
    // destinations. Safety/build diagnostics remain in Settings instead of occupying normal UI.
    inline void drawProductTitleBar(PKSEFramebuffer& fb, const std::string& subtitle = {}) {
        fb.drawSoftShadow(0, -40, fb.getWidth(), kHeaderH + 40, kChromeRadius);
        fb.drawFilledRoundedRect(0, -kChromeRadius, fb.getWidth(), kHeaderH + kChromeRadius,
                                 kChromeRadius, Colors::SurfaceRaised);

        constexpr int cx = 34, cy = 30, r = 19;
        constexpr Color ballWhite(248, 248, 248);
        constexpr Color ballBand(24, 25, 29);

        fb.drawFilledCircle(cx, cy, r, ballWhite);
        fb.setClipRect(cx - r, cy - r, r * 2, r);
        fb.drawFilledCircle(cx, cy, r, Colors::BrandAccent);
        fb.clearClip();
        fb.drawFilledRect(cx - r, cy - 3, r * 2, 6, ballBand);
        fb.drawFilledCircle(cx, cy, 8, ballBand);
        fb.drawFilledCircle(cx, cy, 5, ballWhite);
        fb.drawCircle(cx, cy, r, ballBand, 2);
        fb.drawCircle(cx, cy, 5, ballBand, 1);

        constexpr int brandX = 62;
        fb.drawText(brandX, 8, "PokeBank", Colors::TextPrimary, TextStyle::Title);
        int brandW = 0, brandH = 0;
        fb.measureText("PokeBank", brandW, brandH, TextStyle::Title);
        const int nxX = brandX + brandW + 8;
        fb.drawFilledRoundedRect(nxX, 15, 40, 28, 9, Colors::BrandAccent);
        int nxW = 0, nxH = 0;
        fb.measureText("NX", nxW, nxH, TextStyle::Caption);
        fb.drawText(nxX + (40 - nxW) / 2, 15 + (28 - nxH) / 2,
                    "NX", Colors::Surface, TextStyle::Caption);

        if (!subtitle.empty())
            fb.drawText(nxX + 56, 23, subtitle, Colors::TextSecondary, TextStyle::Caption);
    }

} // namespace UI

#endif
