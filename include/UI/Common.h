#ifndef UI_COMMON_H
#define UI_COMMON_H

#include <cstdint>

// RGB Color structure
struct Color {
    uint8_t r, g, b, a;

    constexpr Color(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255)
        : r(red), g(green), b(blue), a(alpha) {}

    uint32_t toRGBA8() const {
        return (r << 0) | (g << 8) | (b << 16) | (a << 24);
    }
};

// Common colors
namespace Colors {
    constexpr Color Black(0, 0, 0);
    constexpr Color White(255, 255, 255);
    constexpr Color Gray(128, 128, 128);
    constexpr Color LightGray(192, 192, 192);
    constexpr Color DarkGray(64, 64, 64);
    constexpr Color Red(255, 0, 0);
    constexpr Color Green(0, 255, 0);
    constexpr Color Blue(0, 0, 255);
    constexpr Color Yellow(255, 255, 0);
    constexpr Color Cyan(0, 255, 255);
    constexpr Color Magenta(255, 0, 255);
    constexpr Color Orange(255, 165, 0);

    // UI-specific colors
    constexpr Color Background(30, 30, 30);
    constexpr Color Panel(45, 45, 45);
    constexpr Color Selected(70, 130, 180);
    constexpr Color Border(100, 100, 100);
    constexpr Color Text(220, 220, 220);
    constexpr Color TextDim(150, 150, 150);
}

#endif
