#ifndef POKEBANK_UI_STATS_RADAR_H
#define POKEBANK_UI_STATS_RADAR_H

#include "UI/PKSEFramebuffer.h"
#include "UI/BattleStatRadarModel.h"

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
        // At most eight intersections. A bounded insertion sort also avoids
        // libstdc++'s 16-element sort fast path on this small fixed array.
        for (std::size_t i = 1; i < count; ++i)
            for (std::size_t j = i; j > 0 && xs[j] < xs[j - 1]; --j)
                std::swap(xs[j], xs[j - 1]);
        for (std::size_t i = 0; i + 1 < count; i += 2) {
            const int x0 = static_cast<int>(std::ceil(xs[i]));
            const int x1 = static_cast<int>(std::floor(xs[i + 1]));
            if (x1 >= x0) fb.drawFilledRect(x0, y, x1 - x0 + 1, 1, color);
        }
    }
}

// PokeBank-owned variable-axis radar renderer. The inherited framebuffer hexagon renderer is
// optimized around six modern stats; this helper intentionally also supports true five-axis Gen I
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

// Shared bounded five-axis renderer. Axis labels show raw battle stats; the scale
// caption explicitly names the common outer-ring value. Text is measured before
// placement, and labels occupy the clear space above/below their vertices.
inline void drawGen1Labeled(PKSEFramebuffer& fb, int x, int y, int width, int height,
                           const std::array<uint16_t,5>& stats) {
    const auto model = PokeBank::UIModel::gen1RadarModel(stats);
    std::array<std::string,5> labels{};
    std::array<int,5> widths{};
    int lineH = 0;
    for (size_t i = 0; i < stats.size(); ++i) {
        labels[i] = std::string(PokeBank::UIModel::gen1RadarLabels[i]) + " " + std::to_string(stats[i]);
        int h = 0;
        fb.measureText(labels[i], widths[i], h, TextStyle::Caption);
        lineH = std::max(lineH, h);
    }
    const int radius = std::min((width - 20) / 2,
        static_cast<int>((height - 2 * lineH - 12) / 1.81f));
    if (radius <= 0) return;
    const int cx = x + width / 2, cy = y + lineH + 4 + radius;
    draw(fb, cx, cy, radius, model.normalized, 1.0f,
         Color(232, 60, 70, 58), Colors::Divider, Colors::Accent);
    for (size_t i = 0; i < labels.size(); ++i) {
        const auto v = vertex(cx, cy, static_cast<float>(radius), static_cast<int>(i), 5);
        const bool right = i == 1 || i == 2;
        int lx = i == 0 ? cx - widths[i] / 2 : right ? x + width - widths[i] : x;
        int ly = i == 0 ? y : static_cast<int>(std::lround(v.y)) + (i == 2 || i == 3 ? 4 : -lineH - 4);
        fb.drawText(lx, ly, labels[i], Colors::Text, TextStyle::Caption);
    }
    const std::string scale = "Scale " + std::to_string(static_cast<unsigned>(model.scale));
    int sw = 0, sh = 0;
    fb.measureText(scale, sw, sh, TextStyle::Caption);
    fb.drawText(cx - sw / 2, y + height - sh, scale, Colors::TextDim, TextStyle::Caption);
}

// Truthful Generation II renderer: six calculated battle stats, while the editor still
// exposes only the four stored DVs (one Special DV feeds both SpA and SpD).
inline void drawGen2Labeled(PKSEFramebuffer& fb, int x, int y, int width, int height,
                           const std::array<uint16_t,6>& stats) {
    const auto model = PokeBank::UIModel::gen2RadarModel(stats);
    std::array<std::string,6> labels{};
    std::array<int,6> widths{};
    int lineH = 0;
    for (size_t i = 0; i < stats.size(); ++i) {
        labels[i] = std::string(PokeBank::UIModel::gen2RadarLabels[i]) + " " + std::to_string(stats[i]);
        int h = 0;
        fb.measureText(labels[i], widths[i], h, TextStyle::Caption);
        lineH = std::max(lineH, h);
    }

    const int radius = std::min((width - 24) / 2,
                                std::max(0, (height - 2 * lineH - 22) / 2));
    if (radius <= 0) return;
    const int cx = x + width / 2;
    const int cy = y + lineH + 6 + radius;
    draw(fb, cx, cy, radius, model.normalized, 1.0f,
         Color(232, 60, 70, 58), Colors::Divider, Colors::Accent);

    for (size_t i = 0; i < labels.size(); ++i) {
        const auto v = vertex(cx, cy, static_cast<float>(radius), static_cast<int>(i), 6);
        int lx = x;
        int ly = static_cast<int>(std::lround(v.y)) - lineH / 2;
        if (i == 0 || i == 3) {
            lx = cx - widths[i] / 2;
            ly = i == 0 ? y : y + height - 2 * lineH - 2;
        } else if (i == 1 || i == 2) {
            lx = x + width - widths[i];
        }
        fb.drawText(lx, ly, labels[i], Colors::Text, TextStyle::Caption);
    }

    const std::string scale = "Scale " + std::to_string(static_cast<unsigned>(model.scale));
    int sw = 0, sh = 0;
    fb.measureText(scale, sw, sh, TextStyle::Caption);
    fb.drawText(cx - sw / 2, y + height - sh, scale, Colors::TextDim, TextStyle::Caption);
}

} // namespace UI::StatsRadar

#endif
