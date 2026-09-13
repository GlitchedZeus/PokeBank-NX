#ifndef POKEBANK_UI_STATS_RADAR_H
#define POKEBANK_UI_STATS_RADAR_H

#include "UI/PKSEFramebuffer.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace UI::StatsRadar {

struct PointF { float x = 0.0f, y = 0.0f; };

inline PointF vertex(int cx, int cy, float radius, int index, int count) noexcept {
    constexpr float kPi = 3.14159265358979323846f;
    const float angle = (-90.0f + 360.0f * static_cast<float>(index) / static_cast<float>(count)) * kPi / 180.0f;
    return {cx + std::cos(angle) * radius, cy + std::sin(angle) * radius};
}

inline void dottedLine(PKSEFramebuffer& fb, PointF a, PointF b, Color color, int step = 3) {
    const float dx = b.x - a.x, dy = b.y - a.y;
    const float length = std::sqrt(dx * dx + dy * dy);
    const int points = std::max(1, static_cast<int>(length) / std::max(1, step));
    for (int i = 0; i <= points; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(points);
        fb.drawFilledCircle(static_cast<int>(std::lround(a.x + dx * t)),
                            static_cast<int>(std::lround(a.y + dy * t)), 1, color);
    }
}

template <std::size_t N>
inline void fillPolygon(PKSEFramebuffer& fb, const std::array<PointF,N>& points, Color color) {
    float minY = points[0].y, maxY = points[0].y;
    for (const auto& p : points) { minY = std::min(minY, p.y); maxY = std::max(maxY, p.y); }
    for (int y = static_cast<int>(std::floor(minY)); y <= static_cast<int>(std::ceil(maxY)); ++y) {
        std::array<float,N> xs{};
        std::size_t count = 0;
        const float scanY = static_cast<float>(y) + 0.5f;
        for (std::size_t i = 0; i < N; ++i) {
            const auto& a = points[i];
            const auto& b = points[(i + 1) % N];
            if ((a.y <= scanY && b.y > scanY) || (b.y <= scanY && a.y > scanY)) {
                const float t = (scanY - a.y) / (b.y - a.y);
                xs[count++] = a.x + (b.x - a.x) * t;
            }
        }
        std::sort(xs.begin(), xs.begin() + static_cast<std::ptrdiff_t>(count));
        for (std::size_t i = 0; i + 1 < count; i += 2) {
            const int x0 = static_cast<int>(std::ceil(xs[i]));
            const int x1 = static_cast<int>(std::floor(xs[i + 1]));
            if (x1 >= x0) fb.drawFilledRect(x0, y, x1 - x0 + 1, 1, color);
        }
    }
}

// PokeBank-owned variable-axis radar renderer. The inherited framebuffer hexagon renderer is
// optimized around six modern stats; this helper intentionally supports true five-axis Gen I
// HP/Attack/Defense/Speed/Special without inventing Sp.Atk or Sp.Def.
template <std::size_t N>
inline void draw(PKSEFramebuffer& fb, int cx, int cy, int radius,
                 const std::array<float,N>& values, float maxValue,
                 Color fill, Color web, Color outline) {
    static_assert(N >= 3 && N <= 8);
    if (radius <= 0 || maxValue <= 0.0f) return;

    for (int ring = 1; ring <= 4; ++ring) {
        const float r = static_cast<float>(radius) * static_cast<float>(ring) / 4.0f;
        std::array<PointF,N> webPoints{};
        for (std::size_t i = 0; i < N; ++i) webPoints[i] = vertex(cx, cy, r, static_cast<int>(i), static_cast<int>(N));
        for (std::size_t i = 0; i < N; ++i) dottedLine(fb, webPoints[i], webPoints[(i + 1) % N], web, 4);
    }
    for (std::size_t i = 0; i < N; ++i)
        dottedLine(fb, {static_cast<float>(cx), static_cast<float>(cy)},
                   vertex(cx, cy, static_cast<float>(radius), static_cast<int>(i), static_cast<int>(N)), web, 4);

    std::array<PointF,N> data{};
    for (std::size_t i = 0; i < N; ++i) {
        const float normalized = std::clamp(values[i] / maxValue, 0.0f, 1.0f);
        data[i] = vertex(cx, cy, static_cast<float>(radius) * normalized,
                         static_cast<int>(i), static_cast<int>(N));
    }
    fillPolygon(fb, data, fill);
    for (std::size_t i = 0; i < N; ++i)
        dottedLine(fb, data[i], data[(i + 1) % N], outline, 2);
}

} // namespace UI::StatsRadar

#endif
