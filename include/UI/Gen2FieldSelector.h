#pragma once

#include "UI/Gen2NativePresentation.h"

#include <array>
#include <cstdint>

// Gen II Pokerus, Crystal Met Time and Crystal OT Gender use the shared editor's selector
// interaction (as Gen IV/V do for their enumerated fields): A opens a small list on the current
// value, Up/Down move, A or a completed tap applies, B cancels with nothing changed, and focus
// returns to the field. The choices are exactly the native Gen II value domains the previous
// in-place cycling produced; nothing new can be written.
namespace PokeBank::UIModel::Gen2FieldSelector {

enum class Field : uint8_t { None, Pokerus, MetTime, OtGender };

struct Choice {
    uint8_t value = 0;
    const char* label = "";
    bool enabled = true;
};

struct Selector {
    Field field = Field::None;
    std::array<Choice, 4> choices{};
    uint8_t count = 0;
    int row = 0;
    constexpr bool open() const noexcept { return field != Field::None; }
};

enum class Outcome : uint8_t { NotOpen, Moved, Swallowed, Cancelled, Apply };

struct Result {
    Outcome outcome = Outcome::NotOpen;
    uint8_t value = 0;
};

namespace detail {
constexpr void add(Selector& s, uint8_t value, const char* label, bool enabled = true) noexcept {
    if (s.count < s.choices.size()) s.choices[s.count++] = {value, label, enabled};
}
constexpr void focusValue(Selector& s, uint8_t current) noexcept {
    s.row = 0;
    for (int i = 0; i < s.count; ++i)
        if (s.choices[static_cast<std::size_t>(i)].value == current) { s.row = i; return; }
    for (int i = 0; i < s.count; ++i)
        if (s.choices[static_cast<std::size_t>(i)].enabled) { s.row = i; return; }
}
} // namespace detail

// Pokerus byte: None (0), Infected and Cured. The current strain is kept; a new infection uses
// the canonical strain 1 / day 1 (same values as the previous cycle, Gen2Native::cyclePokerusState).
constexpr Selector pokerus(uint8_t raw) noexcept {
    const auto state = Gen2Native::decodePokerus(raw);
    const uint8_t strain = state.strain == 0 ? 1 : state.strain;
    Selector s;
    s.field = Field::Pokerus;
    detail::add(s, 0, "None");
    detail::add(s, state.active ? raw : Gen2Native::encodePokerus(strain, 1), "Infected");
    detail::add(s, Gen2Native::encodePokerus(strain, 0), "Cured");
    detail::focusValue(s, state.present ? (state.active ? s.choices[1].value : s.choices[2].value) : 0);
    return s;
}

// Crystal met time (1 Morning, 2 Day, 3 Night). With recorded caught history only the times the
// selected encounter allows are enabled; an unrecorded pending value may also be Unknown (0).
constexpr Selector metTime(uint8_t current, bool recorded, uint8_t allowedMask) noexcept {
    Selector s;
    s.field = Field::MetTime;
    if (!recorded) detail::add(s, 0, "Unknown");
    for (uint8_t code = 1; code <= 3; ++code)
        detail::add(s, code, Gen2Native::crystalMetTimeName(code),
                    !recorded || (allowedMask & static_cast<uint8_t>(1u << code)) != 0);
    detail::focusValue(s, current);
    return s;
}

// Inline Crystal met time: A advances to the next choice metTime() offers (wrapping), skipping
// times the recorded encounter does not allow, with no popup. Returns `current` when no other
// choice is selectable. The value is the 2-bit caught-time code (Gen2Native::CrystalCaughtData).
constexpr uint8_t nextMetTime(uint8_t current, bool recorded, uint8_t allowedMask) noexcept {
    const Selector s = metTime(current, recorded, allowedMask);
    int start = -1;
    for (int i = 0; i < s.count; ++i)
        if (s.choices[static_cast<std::size_t>(i)].value == current) { start = i; break; }
    for (int step = 1; step <= s.count; ++step) {
        const int i = start < 0 ? step - 1 : (start + step) % s.count;
        const auto& choice = s.choices[static_cast<std::size_t>(i)];
        if (choice.enabled && (choice.value != current || start < 0)) return choice.value;
    }
    return current;
}

// Crystal OT gender. Recorded caught history is Male/Female; an unrecorded pending value may also
// be Unknown.
constexpr Selector otGender(Gen2Native::CrystalOtGenderChoice current, bool recorded) noexcept {
    using C = Gen2Native::CrystalOtGenderChoice;
    Selector s;
    s.field = Field::OtGender;
    if (!recorded) detail::add(s, static_cast<uint8_t>(C::Unknown), "Unknown");
    detail::add(s, static_cast<uint8_t>(C::Male), "Male");
    detail::add(s, static_cast<uint8_t>(C::Female), "Female");
    detail::focusValue(s, static_cast<uint8_t>(current));
    return s;
}

// One input frame. tappedRow >= 0 is a completed tap on that row (select + apply when enabled).
constexpr Result step(Selector& s, bool up, bool down, bool a, bool b, int tappedRow = -1) noexcept {
    if (!s.open()) return {Outcome::NotOpen, 0};
    if (b) {
        s.field = Field::None;
        return {Outcome::Cancelled, 0};
    }
    if (tappedRow >= 0 && tappedRow < s.count) {
        s.row = tappedRow;
        a = true;
    }
    if (a) {
        const auto& choice = s.choices[static_cast<std::size_t>(s.row)];
        if (!choice.enabled) return {Outcome::Swallowed, 0};
        s.field = Field::None;
        return {Outcome::Apply, choice.value};
    }
    if (up || down) {
        const int n = s.count;
        for (int step = 1; step <= n; ++step) {
            const int next = ((s.row + (down ? step : -step)) % n + n) % n;
            if (s.choices[static_cast<std::size_t>(next)].enabled) { s.row = next; break; }
        }
        return {Outcome::Moved, 0};
    }
    return {Outcome::Swallowed, 0};
}

} // namespace PokeBank::UIModel::Gen2FieldSelector
