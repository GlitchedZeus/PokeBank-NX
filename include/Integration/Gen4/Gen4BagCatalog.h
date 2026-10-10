#ifndef POKEBANK_INTEGRATION_GEN4_BAG_CATALOG_H
#define POKEBANK_INTEGRATION_GEN4_BAG_CATALOG_H

#include "Integration/Gen4/Gen4ReadOnlyInventory.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace PokeVault::Integration::Gen4 {

// Source: pinned PKHeX 66ef5983a5d349efa2dd615f2926a1b1ee97306e
// PKHeX.Core/Items/ItemStorage4*.cs and Items/Bags/PlayerBag4*.cs
// Exact native Gen IV pouch ID ranges and count limits. This is a
// structural editing catalog, NOT an encounter, story, ownership, or
// comprehensive legality decision. Never extend by guessing item IDs.
[[nodiscard]] constexpr bool gen4BagItemAllowed(Layout layout,
                                                 size_t pocket,
                                                 uint16_t id) noexcept {
    if(pocket>=BagPocketCount || id==0 || id>536)return false;
    // PKHeX ItemStorage4.Unreleased (not native player-bag Add choices).
    if(id==5 || id==16 || id==147 || id==499 || id==500)return false;
    const bool dp=layout==Layout::DiamondPearl;
    const bool pt=layout==Layout::Platinum;
    const bool hgss=layout==Layout::HeartGoldSoulSilver;
    if(!dp && !pt && !hgss)return false;
    switch(static_cast<BagPocket>(pocket)) {
        case BagPocket::Items:
            return (id>=68 && id<=111) ||
                   (!dp && id==112) ||
                   (id>=135 && id<=136) ||
                   (id>=213 && id<=327);
        case BagPocket::KeyItems:
            if(dp)return id>=428 && id<=464;
            if(pt)return id>=428 && id<=467;
            if(hgss) {
                constexpr std::array<uint16_t,38> keys{{
                    434,435,437,444,445,446,447,450,456,464,465,466,
                    468,469,470,471,472,473,474,475,476,477,478,479,
                    480,481,482,483,484,501,502,503,504,532,533,534,
                    535,536
                }};
                for(const auto key:keys)if(key==id)return true;
            }
            return false;
        case BagPocket::Machines:return id>=328 && id<=427;
        case BagPocket::Mail:return id>=137 && id<=148;
        case BagPocket::Medicine:return id>=17 && id<=54;
        case BagPocket::Berries:return id>=149 && id<=212;
        case BagPocket::Balls:
            return (id>=1 && id<=4) || (id>=6 && id<=15) ||
                   (hgss && id>=492 && id<=498);
        case BagPocket::BattleItems:return id>=55 && id<=67;
    }
    return false;
}

[[nodiscard]] constexpr uint16_t gen4BagMaxQuantity(size_t pocket,
                                                      uint16_t id) noexcept {
    if(pocket>=BagPocketCount)return 0;
    if(pocket==static_cast<size_t>(BagPocket::KeyItems))return 1;
    if(pocket==static_cast<size_t>(BagPocket::Machines))
        return id>=420 && id<=427?1:99;
    return 999;
}

[[nodiscard]] inline std::vector<uint16_t> gen4BagChoices(
    Layout layout,size_t pocket) {
    std::vector<uint16_t> ids;
    if(pocket>=BagPocketCount)return ids;
    for(uint16_t id=1;id<=536;++id)
        if(gen4BagItemAllowed(layout,pocket,id))ids.push_back(id);
    return ids;
}
} // namespace PokeVault::Integration::Gen4

#endif
