#ifndef UI_CLASSIC_TYPE_BADGES_H
#define UI_CLASSIC_TYPE_BADGES_H

#include "UI/ClassicGameContext.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/SpriteManager.h"

#include <cstdint>

namespace UI::ClassicTypeBadges {

inline int spriteWidth(uint8_t classicType, int height = 22) {
    const int normalized = PokeBank::UIModel::ClassicGameContext::normalizedTypeSpriteId(classicType);
    if (normalized < 0) return 0;
    auto* sprite = SpriteManager::getTypeSprite(static_cast<uint8_t>(normalized));
    if (!sprite || !sprite->data || sprite->height <= 0) return 0;
    return (sprite->width * height) / sprite->height;
}

inline void drawSprite(PKSEFramebuffer& fb, int x, int y, uint8_t classicType, int height = 22) {
    const int normalized = PokeBank::UIModel::ClassicGameContext::normalizedTypeSpriteId(classicType);
    if (normalized < 0) return;
    auto* sprite = SpriteManager::getTypeSprite(static_cast<uint8_t>(normalized));
    if (!sprite || !sprite->data || sprite->height <= 0) return;
    const int width = (sprite->width * height) / sprite->height;
    fb.drawImageScaled(x, y, sprite->width, sprite->height, width, height,
                       sprite->data, sprite->channels);
}

inline void drawPairCentered(PKSEFramebuffer& fb, int x, int width, int y,
                             uint8_t type1, uint8_t type2, int height = 22) {
    constexpr int gap = 8;
    const int w1 = spriteWidth(type1, height);
    const int w2 = type2 != type1 ? spriteWidth(type2, height) : 0;
    const int total = w1 + (w2 > 0 ? gap + w2 : 0);
    int xx = x + (width - total) / 2;
    if (w1 > 0) {
        drawSprite(fb, xx, y, type1, height);
        xx += w1 + gap;
    }
    if (w2 > 0) drawSprite(fb, xx, y, type2, height);
}

} // namespace UI::ClassicTypeBadges

#endif
