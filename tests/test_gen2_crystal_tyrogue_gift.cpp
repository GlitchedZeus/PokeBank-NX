#include "Legality/Gen2CrystalTyrogueGiftEvidence.h"
#include <cassert>
#include <cstdint>
#include <initializer_list>

int main() {
    namespace S = Legality::Gen2Static;
    namespace T = Legality::Gen2CrystalTyrogueGift;
    using E = T::Evolution;
    // Pinned PKHeX-derived Crystal static/gift record.
    constexpr uint32_t row = 0x080523ecu;
    static_assert(S::game(row) == S::Game::Crystal);
    static_assert(S::species(row) == 236);
    static_assert(S::location(row) == 35);
    static_assert(S::level(row) == 10);
    static_assert(!S::isEgg(row));
    static_assert(S::shinyPolicy(row) == 0);

    constexpr uint16_t caught = (1u<<14)|(10u<<8)|35u;
    assert(S::matches("crystal_gbc",236,10,caught,false,false));
    assert(T::analyze("crystal_gbc",106,20,caught,false,false) == E::Hitmonlee);
    assert(T::analyze("crystal_gbc",107,20,caught,false,true) == E::Hitmonchan);
    assert(T::analyze("crystal_gbc",237,20,caught,false,false) == E::Hitmontop);
    assert(T::analyze("crystal_gbc",106,100,caught,false,true) == E::Hitmonlee);

    // Without a matching source or sufficient level, ancestry is unresolved.
    for(const uint16_t species : {106u,107u,237u}) {
        assert(T::analyze("crystal_gbc",species,19,caught,false,false) == E::Unresolved);
        assert(T::analyze("crystal_gbc",species,20,caught,true,false) == E::Unresolved);
        assert(T::analyze("crystal_gbc",species,20,0,false,false) == E::Unresolved);
        assert(T::analyze("gold_gbc",species,20,caught,false,false) == E::Unresolved);
        assert(T::analyze("silver_gbc",species,20,caught,false,false) == E::Unresolved);
        assert(T::analyze("crystal_gbc",species,20,(1u<<14)|(9u<<8)|35u,false,false) == E::Unresolved);
        assert(T::analyze("crystal_gbc",species,20,(1u<<14)|(10u<<8)|34u,false,false) == E::Unresolved);
    }
    // Hitmon* are independent species; Tyrogue is not evolved already.
    assert(T::analyze("crystal_gbc",236,20,caught,false,false) == E::Unresolved);
    assert(T::analyze("crystal_gbc",95,20,caught,false,false) == E::Unresolved);
}
