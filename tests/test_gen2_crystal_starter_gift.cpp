#include "Legality/Gen2CrystalStarterGiftEvidence.h"
#include <cassert>
#include <initializer_list>

int main() {
    namespace G = Legality::Gen2CrystalStarterGift;
    using A = G::Ancestor;
    // Exact pinned PKHeX Crystal starter gift rows: level 5, location 1.
    constexpr uint32_t chikorita = 0x08028198u;
    constexpr uint32_t cyndaquil = 0x0802819bu;
    constexpr uint32_t totodile = 0x0802819eu;
    static_assert(Legality::Gen2Static::game(chikorita) == Legality::Gen2Static::Game::Crystal);
    static_assert(Legality::Gen2Static::game(cyndaquil) == Legality::Gen2Static::Game::Crystal);
    static_assert(Legality::Gen2Static::game(totodile) == Legality::Gen2Static::Game::Crystal);
    static_assert(Legality::Gen2Static::species(chikorita) == 152);
    static_assert(Legality::Gen2Static::species(cyndaquil) == 155);
    static_assert(Legality::Gen2Static::species(totodile) == 158);
    static_assert(Legality::Gen2Static::location(chikorita) == 1);
    static_assert(Legality::Gen2Static::location(cyndaquil) == 1);
    static_assert(Legality::Gen2Static::location(totodile) == 1);
    static_assert(Legality::Gen2Static::level(chikorita) == 5);
    static_assert(Legality::Gen2Static::level(cyndaquil) == 5);
    static_assert(Legality::Gen2Static::level(totodile) == 5);
    static_assert(!Legality::Gen2Static::isEgg(chikorita));
    static_assert(!Legality::Gen2Static::isEgg(cyndaquil));
    static_assert(!Legality::Gen2Static::isEgg(totodile));

    constexpr uint16_t caught = (1u << 14) | (5u << 8) | 1u;
    assert(G::analyze("crystal_gbc", 153, 16, caught, false, false) == A::Chikorita);
    assert(G::analyze("crystal_gbc", 154, 32, caught, false, true) == A::Chikorita);
    assert(G::analyze("crystal_gbc", 156, 14, caught, false, false) == A::Cyndaquil);
    assert(G::analyze("crystal_gbc", 157, 36, caught, false, true) == A::Cyndaquil);
    assert(G::analyze("crystal_gbc", 159, 18, caught, false, false) == A::Totodile);
    assert(G::analyze("crystal_gbc", 160, 30, caught, false, true) == A::Totodile);

    assert(G::analyze("crystal_gbc", 153, 15, caught, false, false) == A::None);
    assert(G::analyze("crystal_gbc", 154, 31, caught, false, false) == A::None);
    assert(G::analyze("crystal_gbc", 156, 13, caught, false, false) == A::None);
    assert(G::analyze("crystal_gbc", 157, 35, caught, false, false) == A::None);
    assert(G::analyze("crystal_gbc", 159, 17, caught, false, false) == A::None);
    assert(G::analyze("crystal_gbc", 160, 29, caught, false, false) == A::None);
    assert(G::analyze("crystal_gbc", 154, 32, caught, true, false) == A::None);
    assert(G::analyze("gold_gbc", 154, 32, caught, false, false) == A::None);
    assert(G::analyze("silver_gbc", 157, 36, caught, false, false) == A::None);
    assert(G::analyze("crystal_gbc", 154, 32, 0, false, false) == A::None);
    assert(G::analyze("crystal_gbc", 154, 32,
                      (1u << 14) | (5u << 8) | 2u, false, false) == A::None);
    assert(G::analyze("crystal_gbc", 154, 32,
                      (1u << 14) | (6u << 8) | 1u, false, false) == A::None);
    assert(G::analyze("crystal_gbc", 152, 32, caught, false, false) == A::None);
    assert(G::analyze("crystal_gbc", 155, 36, caught, false, false) == A::None);
    assert(G::analyze("crystal_gbc", 160, 30,
                      (1u << 14) | (20u << 8) | 16u, false, false) == A::None);
}
