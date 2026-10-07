#ifndef UI_SPRITE_LAYOUT_H
#define UI_SPRITE_LAYOUT_H

#include <algorithm>
#include <cstdint>

namespace PokeBank::UIModel {

struct SpriteRect {
    int x;
    int y;
    int width;
    int height;
};

// Fit a sprite inside a destination box without cropping or changing its aspect ratio.
// The result is deterministic and contains no frame-time/idle-animation state.
constexpr SpriteRect containSprite(int x, int y, int boxWidth, int boxHeight,
                                   int sourceWidth, int sourceHeight) {
    if (boxWidth <= 0 || boxHeight <= 0 || sourceWidth <= 0 || sourceHeight <= 0) {
        return {x, y, 0, 0};
    }

    int width = boxWidth;
    int height = static_cast<int>((static_cast<std::int64_t>(sourceHeight) * boxWidth
                                   + sourceWidth / 2) / sourceWidth);
    if (height > boxHeight) {
        height = boxHeight;
        width = static_cast<int>((static_cast<std::int64_t>(sourceWidth) * boxHeight
                                  + sourceHeight / 2) / sourceHeight);
    }

    width = std::clamp(width, 1, boxWidth);
    height = std::clamp(height, 1, boxHeight);
    return {x + (boxWidth - width) / 2, y + (boxHeight - height) / 2, width, height};
}

} // namespace PokeBank::UIModel

#endif
