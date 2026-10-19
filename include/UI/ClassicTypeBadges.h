#ifndef UI_CLASSIC_TYPE_BADGES_H
#define UI_CLASSIC_TYPE_BADGES_H

#include "UI/ClassicGameContext.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/SpriteManager.h"

#include <cstdint>

namespace UI::ClassicTypeBadges {

// The ONE type badge: the same romfs/sprites/types/<id>.png images the Pokémon summary draws
// (SharedPokemonEditorSurface / Gen III summary), scaled to `height`. `canonicalType` is the
// PKHeX type order (Normal = 0 ... Dark = 16; 17 = Fairy, which Gen I-V never has and this
// badge refuses). Returns the drawn width, 0 when nothing was drawn (no sprite / not a type).
inline constexpr uint8_t kLastGen15Type = 16;   // Dark

inline int canonicalWidth(uint8_t canonicalType, int height = 22) {
    if (canonicalType > kLastGen15Type || height <= 0) return 0;
    auto* sprite = SpriteManager::getTypeSprite(canonicalType);
    if (!sprite || !sprite->data || sprite->height <= 0) return 0;
    return (sprite->width * height) / sprite->height;
}

inline int drawCanonical(PKSEFramebuffer& fb, int x, int y, uint8_t canonicalType, int height = 22) {
    const int width = canonicalWidth(canonicalType, height);
    if (width <= 0) return 0;
    auto* sprite = SpriteManager::getTypeSprite(canonicalType);
    fb.drawImageScaled(x, y, sprite->width, sprite->height, width, height,
                       sprite->data, sprite->channels);
    return width;
}

// Classic (Gen I/II save) type ids go through the same badge after normalisation.
inline int spriteWidth(uint8_t classicType, int height = 22) {
    const int normalized = PokeBank::UIModel::ClassicGameContext::normalizedTypeSpriteId(classicType);
    return normalized < 0 ? 0 : canonicalWidth(static_cast<uint8_t>(normalized), height);
}

inline void drawSprite(PKSEFramebuffer& fb, int x, int y, uint8_t classicType, int height = 22) {
    const int normalized = PokeBank::UIModel::ClassicGameContext::normalizedTypeSpriteId(classicType);
    if (normalized >= 0) drawCanonical(fb, x, y, static_cast<uint8_t>(normalized), height);
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
