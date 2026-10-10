#include "Legality/Gen2CrystalDratiniGiftEvidence.h"
#include <cassert>
#include <cstdint>

int main() {
    namespace D = Legality::Gen2CrystalDratiniGift;
    using E = D::Evidence;
    constexpr uint32_t row = 0x0807aa93u;
    static_assert(Legality::Gen2Static::game(row) == Legality::Gen2Static::Game::Crystal);
    static_assert(Legality::Gen2Static::species(row) == 147);
    static_assert(Legality::Gen2Static::location(row) == 42);
    static_assert(Legality::Gen2Static::level(row) == 15);
    static_assert(!Legality::Gen2Static::isEgg(row));
    static_assert(Legality::Gen2Static::shinyPolicy(row) == 0);
    constexpr uint16_t caught = (1u << 14) | (15u << 8) | 42u;
    assert(Legality::Gen2Static::matches("crystal_gbc",147,15,caught,false,false));

    // A level-up at 30/55 can preserve the original level-15 gift data.
    assert(D::analyze("crystal_gbc",148,30,caught,false,false) == E::Dragonair);
    assert(D::analyze("crystal_gbc",149,55,caught,false,false) == E::Dragonite);
    assert(D::analyze("crystal_gbc",149,90,caught,false,true) == E::Dragonite);
    assert(D::analyze("crystal_gbc",148,29,caught,false,false) == E::Unresolved);
    assert(D::analyze("crystal_gbc",149,54,caught,false,false) == E::Unresolved);

    // No possible positive claim without the source identity and retained
    // static encounter's original location/level, or for an egg record.
    assert(D::analyze("crystal_gbc",149,55,caught,true,false) == E::Unresolved);
    assert(D::analyze("gold_gbc",149,55,caught,false,false) == E::Unresolved);
    assert(D::analyze("silver_gbc",148,30,caught,false,false) == E::Unresolved);
    assert(D::analyze("crystal_gbc",149,55,0,false,false) == E::Unresolved);
    assert(D::analyze("crystal_gbc",149,55,
                      (1u<<14)|(15u<<8)|41u,false,false) == E::Unresolved);
    assert(D::analyze("crystal_gbc",149,55,
                      (1u<<14)|(16u<<8)|42u,false,false) == E::Unresolved);
    assert(D::analyze("crystal_gbc",147,55,caught,false,false) == E::Unresolved);
    assert(D::analyze("crystal_gbc",130,55,caught,false,false) == E::Unresolved);
}
