#pragma once
// Rounded artwork geometry (Round 11, HOME card scenery). Pure and host-testable: no framebuffer.
//
// Artwork that fills the top band of a rounded card must follow the card's own corners. A
// rectangular clip (setClipRect) lets the image's square corners protrude past the card's rounded
// corners and its focus ring. Instead the renderer fills ONE vector path (a rectangle whose top
// corners are rounded, nvgRoundedRectVarying) with an image pattern placed at the scale-to-cover
// rectangle: the path does the masking, so there are no image strips and no per-row clipping, and it
// stays exact at any output scale (handheld or docked) because it is vector geometry.

#include <algorithm>

namespace UI::RoundedArtwork {

struct CornerRadii {
    float topLeft = 0, topRight = 0, bottomRight = 0, bottomLeft = 0;
};

struct Rect {
    int x = 0, y = 0, w = 0, h = 0;
};

// Only the top corners rounded (a band at the top of a card); the bottom edge stays square.
[[nodiscard]] constexpr CornerRadii topCorners(float radius) noexcept {
    return {radius, radius, 0.0f, 0.0f};
}

// The radius of a shape inset by `inset` pixels inside a rounded shape of `outerRadius`: concentric
// corners keep an even border, so the inner radius shrinks by the inset (never below zero).
[[nodiscard]] constexpr float innerRadius(float outerRadius, float inset) noexcept {
    return outerRadius > inset ? outerRadius - inset : 0.0f;
}

// Scale-to-cover: the smallest aspect-preserving rectangle covering `dst`, centred on it. The part
// outside `dst` is cropped by the rounded path, never stretched.
[[nodiscard]] inline Rect coverRect(const Rect& dst, int srcW, int srcH) noexcept {
    if (dst.w <= 0 || dst.h <= 0 || srcW <= 0 || srcH <= 0) return dst;
    int w = dst.w;
    int h = std::max(1, srcH * w / srcW);
    if (h < dst.h) {
        h = dst.h;
        w = std::max(1, srcW * h / srcH);
    }
    return {dst.x + (dst.w - w) / 2, dst.y + (dst.h - h) / 2, w, h};
}

// The scenery band at the top of a card: inset by `inset` from the card's left, top and right edges
// (inside its border), `bandHeight` tall, with top corners concentric to the card's corners.
struct Band {
    Rect rect;
    CornerRadii radii;
};
[[nodiscard]] constexpr Band topBand(int cardX, int cardY, int cardW, int bandHeight, float cardRadius,
                                     int inset) noexcept {
    return {{cardX + inset, cardY + inset, cardW - 2 * inset, bandHeight - inset},
            topCorners(innerRadius(cardRadius, static_cast<float>(inset)))};
}

// Whether the pixel centre (px + 0.5, py + 0.5) lies inside `r` with corners `c`. Used by tests to
// prove that a band never paints outside the card's rounded corner.
[[nodiscard]] inline bool contains(const Rect& r, const CornerRadii& c, int px, int py) noexcept {
    const float x = static_cast<float>(px) + 0.5f, y = static_cast<float>(py) + 0.5f;
    const float l = static_cast<float>(r.x), t = static_cast<float>(r.y);
    const float rt = l + static_cast<float>(r.w), b = t + static_cast<float>(r.h);
    if (x < l || x >= rt || y < t || y >= b) return false;
    const auto outsideCorner = [&](float cx, float cy, float rad) {
        const float dx = x - cx, dy = y - cy;
        return dx * dx + dy * dy > rad * rad;
    };
    if (c.topLeft > 0 && x < l + c.topLeft && y < t + c.topLeft &&
        outsideCorner(l + c.topLeft, t + c.topLeft, c.topLeft)) return false;
    if (c.topRight > 0 && x > rt - c.topRight && y < t + c.topRight &&
        outsideCorner(rt - c.topRight, t + c.topRight, c.topRight)) return false;
    if (c.bottomRight > 0 && x > rt - c.bottomRight && y > b - c.bottomRight &&
        outsideCorner(rt - c.bottomRight, b - c.bottomRight, c.bottomRight)) return false;
    if (c.bottomLeft > 0 && x < l + c.bottomLeft && y > b - c.bottomLeft &&
        outsideCorner(l + c.bottomLeft, b - c.bottomLeft, c.bottomLeft)) return false;
    return true;
}

} // namespace UI::RoundedArtwork
