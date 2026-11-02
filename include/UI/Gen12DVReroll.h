#pragma once

#include <array>
#include <cstdint>

// Y "Random DVs" in the Gen I and Gen II editors. Each press re-rolls the draft's four stored DVs
// (Attack, Defense, Speed, Special; HP is derived) immediately, with no prompt, and keeps what
// the player asked for:
//   - the shiny state. Gen II shininess is a DV pattern (pokecrystal CheckShininess: Defense,
//     Speed and Special 10, Attack with bit 1 set); a Gen I Pokemon is shown shiny under the same
//     rule (Gen1Shiny.h), Gen I itself stores no flag;
//   - the Gen II gender, which is the Attack DV against the species ratio (Gen2PersonalData
//     genderFromAttackDV, pret GetGender). Gen I has no gender, so no constraint there;
//   - the Gen II Unown letter, which is bits 1-2 of each stored DV (pokecrystal GetUnownLetter;
//     Gen2ReadOnlySave decodes it the same way).
// Every press returns different DVs whenever another combination satisfies those constraints;
// when none does, `changed` is false and the caller says so instead of pretending.
namespace PokeBank::UIModel::Gen12DVReroll {

using DVs = std::array<uint8_t, 4>; // Attack, Defense, Speed, Special

constexpr std::array<uint8_t, 8> kShinyAttack{2, 3, 6, 7, 10, 11, 14, 15};

constexpr bool isShiny(const DVs& dvs) noexcept {
    return (dvs[0] & 0x02u) != 0 && dvs[1] == 10 && dvs[2] == 10 && dvs[3] == 10;
}

// Unown A..Z (0..25) from the stored DVs.
constexpr int unownLetter(const DVs& dvs) noexcept {
    const unsigned value = (((dvs[0] >> 1) & 3u) << 6) | (((dvs[1] >> 1) & 3u) << 4) |
                           (((dvs[2] >> 1) & 3u) << 2) | ((dvs[3] >> 1) & 3u);
    const unsigned letter = value / 10u;
    return static_cast<int>(letter > 25u ? 25u : letter);
}

struct Result {
    DVs dvs{};
    bool changed = false;
};

// xorshift32: deterministic per seed, no global state; the editor owns the seed.
constexpr uint32_t next(uint32_t& state) noexcept {
    if (state == 0) state = 0x9E3779B9u;
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
}

struct NoGender {
    constexpr int operator()(uint8_t) const noexcept { return 0; }
};

// genderOf(attackDV) -> any comparable gender code (Gen I: NoGender). `unown` keeps the letter.
template <typename GenderOf = NoGender>
constexpr Result reroll(const DVs& current, uint32_t& rng, GenderOf genderOf = {}, bool unown = false) noexcept {
    const bool shiny = isShiny(current);
    const auto gender = genderOf(current[0]);
    const int letter = unown ? unownLetter(current) : -1;
    const auto acceptable = [&](const DVs& candidate) {
        return isShiny(candidate) == shiny && genderOf(candidate[0]) == gender &&
               (!unown || unownLetter(candidate) == letter) && candidate != current;
    };

    if (shiny) {
        // Only the Attack DV can vary; pick uniformly among the shiny Attack values that keep
        // gender (and letter), excluding the current one.
        std::array<uint8_t, 8> options{};
        int count = 0;
        for (const uint8_t attack : kShinyAttack) {
            const DVs candidate{attack, 10, 10, 10};
            if (acceptable(candidate)) options[static_cast<std::size_t>(count++)] = attack;
        }
        if (count == 0) return {current, false};
        const uint8_t attack = options[static_cast<std::size_t>(next(rng) % static_cast<uint32_t>(count))];
        return {{attack, 10, 10, 10}, true};
    }

    // Non-shiny: uniform random draws, which succeed almost always within a few tries...
    for (int attempt = 0; attempt < 256; ++attempt) {
        const uint32_t r = next(rng);
        const DVs candidate{static_cast<uint8_t>(r & 15u), static_cast<uint8_t>((r >> 4) & 15u),
                            static_cast<uint8_t>((r >> 8) & 15u), static_cast<uint8_t>((r >> 12) & 15u)};
        if (acceptable(candidate)) return {candidate, true};
    }
    // ...and an exhaustive scan from a random start guarantees an answer whenever one exists
    // (e.g. a narrow gender ratio combined with an Unown letter).
    const uint32_t start = next(rng) & 0xFFFFu;
    for (uint32_t i = 0; i < 0x10000u; ++i) {
        const uint32_t v = (start + i) & 0xFFFFu;
        const DVs candidate{static_cast<uint8_t>(v & 15u), static_cast<uint8_t>((v >> 4) & 15u),
                            static_cast<uint8_t>((v >> 8) & 15u), static_cast<uint8_t>((v >> 12) & 15u)};
        if (acceptable(candidate)) return {candidate, true};
    }
    return {current, false};
}

} // namespace PokeBank::UIModel::Gen12DVReroll
