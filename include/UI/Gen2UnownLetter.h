#pragma once

#include "UI/Gen12DVReroll.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <string>

// Gen II Unown letter selector (R2-REROLL / Round 6 P6). The letter is NEVER stored on its own:
// pokecrystal engine/gfx/load_pics.asm GetUnownLetter derives it from bits 1-2 of the four stored
// DVs (Gen12DVReroll::unownLetter), and engine/gfx/color.asm CheckShininess makes a Pokemon shiny
// only when Defense, Speed and Special are 10 and the Attack DV has bit 1 set. Choosing a letter
// therefore means choosing DVs, and only a few letters have a shiny DV set at all; which ones is
// computed from those two rules below, not written by hand.
namespace PokeBank::UIModel::Gen2UnownLetter {

namespace DV = Gen12DVReroll;
using DVs = DV::DVs;

inline constexpr uint16_t kUnownSpecies = 201;
inline constexpr int kLetterCount = 26;

constexpr char letterChar(int letter) noexcept {
    return letter >= 0 && letter < kLetterCount ? static_cast<char>('A' + letter) : '?';
}

// Bit n set = Unown letter n has at least one shiny DV set.
constexpr uint32_t shinyLetterMask() noexcept {
    uint32_t mask = 0;
    for (unsigned v = 0; v < 0x10000u; ++v) {
        const DVs dvs{static_cast<uint8_t>(v >> 12), static_cast<uint8_t>((v >> 8) & 15u),
                      static_cast<uint8_t>((v >> 4) & 15u), static_cast<uint8_t>(v & 15u)};
        if (DV::isShiny(dvs)) mask |= 1u << DV::unownLetter(dvs);
    }
    return mask;
}

// Computed once (the full scan above), so per-frame note text stays cheap.
inline uint32_t cachedShinyLetterMask() noexcept {
    static const uint32_t mask = shinyLetterMask();
    return mask;
}

inline bool canBeShiny(int letter) noexcept {
    return letter >= 0 && letter < kLetterCount && (cachedShinyLetterMask() >> letter & 1u) != 0;
}

// "I and V": the shiny letters, for status and note text.
inline std::string shinyLettersText() {
    std::string out;
    int remaining = 0;
    for (int letter = 0; letter < kLetterCount; ++letter) remaining += canBeShiny(letter);
    for (int letter = 0; letter < kLetterCount; ++letter) {
        if (!canBeShiny(letter)) continue;
        if (!out.empty()) out += remaining == 1 ? " and " : ", ";
        out += letterChar(letter);
        --remaining;
    }
    return out;
}

constexpr int distance(const DVs& a, const DVs& b) noexcept {
    int total = 0;
    for (std::size_t i = 0; i < a.size(); ++i) {
        const int d = static_cast<int>(a[i]) - static_cast<int>(b[i]);
        total += d < 0 ? -d : d;
    }
    return total;
}

struct Candidate {
    DVs dvs{};
    bool found = false;
};

// The DV set closest to `current` (smallest total change) whose letter is `letter` (any letter
// when letter < 0) and whose shiny state is `shiny`. Exact without scanning all 65536 sets:
//   - a shiny set is {Attack with bit 1 set, 10, 10, 10} (CheckShininess), so only kShinyAttack;
//   - the letter depends only on bits 1-2 of each DV, so for each 8-bit GetUnownLetter value in
//     the letter's range every DV is chosen independently among its four values (bits 0 and 3
//     free). That per-DV optimum can only be shiny through Defense = Speed = Special = 10 (bit 1
//     of Attack is fixed by the value), so the best non-shiny set then moves exactly one of those
//     three to its next-closest value.
// tests/test_gen2_unown_letter_selector.cpp checks both shortcuts against a full scan.
constexpr Candidate nearest(const DVs& current, int letter, bool shiny) noexcept {
    Candidate best;
    int bestScore = 1 << 30;
    const auto consider = [&](const DVs& dvs, int score) {
        if (DV::isShiny(dvs) != shiny) return;
        if (letter >= 0 && DV::unownLetter(dvs) != letter) return;
        if (score < bestScore) { bestScore = score; best = {dvs, true}; }
    };
    if (shiny) {
        for (const uint8_t attack : DV::kShinyAttack) {
            const DVs dvs{attack, 10, 10, 10};
            consider(dvs, distance(current, dvs));
        }
        return best;
    }
    const auto gap = [](uint8_t a, uint8_t b) { return a > b ? a - b : b - a; };
    const int first = letter < 0 ? 0 : letter;
    const int last = letter < 0 ? kLetterCount - 1 : letter;
    for (int l = first; l <= last; ++l) {
        for (int value = l * 10; value <= l * 10 + 9 && value <= 255; ++value) {
            DVs closest{}, second{};
            for (std::size_t i = 0; i < 4; ++i) {
                const unsigned bits12 = static_cast<unsigned>(value) >> (6 - 2 * i) & 3u;
                int bestGap = 99, secondGap = 99;
                for (unsigned free = 0; free < 4u; ++free) {
                    const auto option = static_cast<uint8_t>((free >> 1) << 3 | bits12 << 1 | (free & 1u));
                    const int g = gap(option, current[i]);
                    if (g < bestGap) {
                        second[i] = closest[i]; secondGap = bestGap;
                        closest[i] = option; bestGap = g;
                    } else if (g < secondGap) {
                        second[i] = option; secondGap = g;
                    }
                }
            }
            if (!DV::isShiny(closest)) {
                consider(closest, distance(current, closest));
                continue;
            }
            for (std::size_t i = 1; i < 4; ++i) {
                DVs alternative = closest;
                alternative[i] = second[i];
                consider(alternative, distance(current, alternative));
            }
        }
    }
    return best;
}

enum class Outcome : uint8_t { Invalid, Unchanged, Changed, ShinyImpossible };

struct Selection {
    Outcome outcome = Outcome::Invalid;
    DVs dvs{};
};

// The letter selector. Keeps the shiny state whenever the requested letter has a DV set in that
// state; a shiny Unown asked for a letter with no shiny DV set is ShinyImpossible and the DVs are
// left unchanged, so the caller can explain instead of silently dropping shininess.
inline Selection selectLetter(const DVs& current, int letter) noexcept {
    if (letter < 0 || letter >= kLetterCount) return {Outcome::Invalid, current};
    if (DV::unownLetter(current) == letter) return {Outcome::Unchanged, current};
    const bool shiny = DV::isShiny(current);
    if (shiny && !canBeShiny(letter)) return {Outcome::ShinyImpossible, current};
    const auto found = nearest(current, letter, shiny);
    return found.found ? Selection{Outcome::Changed, found.dvs} : Selection{Outcome::Invalid, current};
}

// The Shiny toggle for Unown. Off always keeps the letter (every letter has non-shiny DV sets).
// On keeps the letter when it can be shiny; otherwise the closest shiny DV set is used and
// `letterChanged` says so.
struct ShinyToggle {
    DVs dvs{};
    bool letterChanged = false;
};
inline ShinyToggle toggleShiny(const DVs& current) noexcept {
    const int letter = DV::unownLetter(current);
    if (DV::isShiny(current)) return {nearest(current, letter, false).dvs, false};
    if (canBeShiny(letter)) return {nearest(current, letter, true).dvs, false};
    const auto any = nearest(current, -1, true);
    return {any.dvs, DV::unownLetter(any.dvs) != letter};
}

// Parses the keyboard answer: one letter, either case. -1 when it is not exactly one letter.
inline int parseLetter(const std::string& text) noexcept {
    if (text.size() != 1) return -1;
    const char c = text[0];
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a';
    return -1;
}

} // namespace PokeBank::UIModel::Gen2UnownLetter
