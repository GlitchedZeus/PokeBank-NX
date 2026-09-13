#include "Integration/Gen1/Gen1Shiny.h"

#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>

using PokeVault::Integration::Gen1::ShinyDVs;

int main() {
    constexpr std::array<uint8_t,8> valid{2,3,6,7,10,11,14,15};
    for (uint8_t attack = 0; attack <= 15; ++attack) {
        bool expected = false;
        for (const auto value : valid) if (attack == value) expected = true;
        assert(ShinyDVs::attackIsShinyCompatible(attack) == expected);
        assert(ShinyDVs::isShiny({attack,10,10,10}) == expected);
    }

    for (const auto attack : valid) {
        assert(ShinyDVs::isShiny({attack,10,10,10}));
        assert(!ShinyDVs::isShiny({attack,9,10,10}));
        assert(!ShinyDVs::isShiny({attack,10,9,10}));
        assert(!ShinyDVs::isShiny({attack,10,10,9}));
        assert(ShinyDVs::makeShiny({attack,1,2,3}) == (std::array<uint8_t,4>{attack,10,10,10}));
    }

    // Deterministic nearest compatible Attack DV; ties resolve downward.
    assert(ShinyDVs::nearestShinyAttack(0) == 2);
    assert(ShinyDVs::nearestShinyAttack(1) == 2);
    assert(ShinyDVs::nearestShinyAttack(4) == 3);
    assert(ShinyDVs::nearestShinyAttack(5) == 6);
    assert(ShinyDVs::nearestShinyAttack(8) == 7);
    assert(ShinyDVs::nearestShinyAttack(9) == 10);
    assert(ShinyDVs::nearestShinyAttack(12) == 11);
    assert(ShinyDVs::nearestShinyAttack(13) == 14);

    constexpr std::array<uint8_t,4> normal{8,8,8,8};
    constexpr auto shiny = ShinyDVs::makeShiny(normal);
    static_assert(shiny == std::array<uint8_t,4>{7,10,10,10});
    static_assert(ShinyDVs::isShiny(shiny));

    // UI uses its transient snapshot for Normal -> Shiny -> Normal. This fallback
    // covers a source that started shiny before the session had a normal snapshot.
    constexpr auto fallback = ShinyDVs::makeNormalFallback(shiny);
    static_assert(fallback == std::array<uint8_t,4>{7,9,10,10});
    static_assert(!ShinyDVs::isShiny(fallback));
    static_assert(ShinyDVs::makeNormalFallback(normal) == normal);

    std::cout << "Gen I shiny DV model: PASS (exact rule, deterministic toggle, no stored flag)\n";
    return 0;
}
