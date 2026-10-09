#include "Legality/Gen2CrystalEeveeGiftEvidence.h"
#include <cassert>
#include <initializer_list>

int main() {
    namespace G=Legality::Gen2CrystalEeveeGift;
    using E=G::Evidence;
    constexpr uint32_t row=0x080a1085u;
    static_assert(Legality::Gen2Static::game(row)==Legality::Gen2Static::Game::Crystal);
    static_assert(Legality::Gen2Static::species(row)==133);
    static_assert(Legality::Gen2Static::location(row)==16);
    static_assert(Legality::Gen2Static::level(row)==20);
    static_assert(!Legality::Gen2Static::isEgg(row));
    static_assert(Legality::Gen2Static::shinyPolicy(row)==0);
    constexpr uint16_t caught=(1u<<14)|(20u<<8)|16u;
    assert(Legality::Gen2Static::matches("crystal_gbc",133,20,caught,false,false));
    for(uint16_t species:{134u,135u,136u}){
        assert(G::analyze("crystal_gbc",species,20,caught,false,false)==E::StoneEvolution);
        assert(G::analyze("crystal_gbc",species,19,caught,false,false)==E::Unresolved);
        assert(G::analyze("crystal_gbc",species,20,caught,true,false)==E::Unresolved);
    }
    for(uint16_t species:{196u,197u}){
        assert(G::analyze("crystal_gbc",species,21,caught,false,false)==E::FriendshipEvolution);
        assert(G::analyze("crystal_gbc",species,20,caught,false,false)==E::Unresolved);
        assert(G::analyze("crystal_gbc",species,21,caught,false,true)==E::FriendshipEvolution);
    }
    assert(G::analyze("crystal_gbc",25,21,caught,false,false)==E::Unresolved);
    assert(G::analyze("silver_gbc",196,21,caught,false,false)==E::Unresolved);
    assert(G::analyze("crystal_gbc",196,21,0,false,false)==E::Unresolved);
    assert(G::analyze("crystal_gbc",196,21,(1u<<14)|(20u<<8)|17u,false,false)==E::Unresolved);
    assert(G::analyze("crystal_gbc",196,21,(1u<<14)|(19u<<8)|16u,false,false)==E::Unresolved);
}
