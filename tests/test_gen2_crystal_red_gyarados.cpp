#include "Legality/Gen2StaticEncounter.h"
#include <cassert>
#include <cstdint>

int main() {
    // PKHeX pinned Crystal static row: level 30 at Lake of Rage,
    // special forced shiny DVs, not an egg. A match is evidence of
    // compatibility only, not proof of where this specimen originated.
    namespace S = Legality::Gen2Static;
    constexpr uint32_t red = 0x088f2682u;
    static_assert(S::game(red) == S::Game::Crystal);
    static_assert(S::species(red) == 130);
    static_assert(S::location(red) == 38);
    static_assert(S::level(red) == 30);
    static_assert(S::shinyPolicy(red) == 1);
    static_assert(!S::isEgg(red));
    constexpr uint16_t caught = (1u<<14)|(30u<<8)|38u;
    assert(S::matches("crystal_gbc",130,30,caught,false,true));
    assert(S::matches("crystal_gbc",130,100,caught,false,true));
    assert(!S::matches("crystal_gbc",130,30,caught,false,false));
    assert(!S::matches("crystal_gbc",130,29,caught,false,true));
    assert(!S::matches("crystal_gbc",130,30,caught,true,true));
    assert(!S::matches("crystal_gbc",130,30,(1u<<14)|(29u<<8)|38u,false,true));
    assert(!S::matches("crystal_gbc",130,30,(1u<<14)|(30u<<8)|39u,false,true));
    assert(!S::matches("crystal_gbc",129,30,caught,false,true));
    assert(!S::matches("crystal_gbc",130,30,caught,false,false));
}
