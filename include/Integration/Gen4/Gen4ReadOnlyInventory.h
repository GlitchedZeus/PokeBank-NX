#ifndef POKEBANK_INTEGRATION_GEN4_READ_ONLY_INVENTORY_H
#define POKEBANK_INTEGRATION_GEN4_READ_ONLY_INVENTORY_H

#include "Integration/Gen4/Gen4ReadOnlySave.h"
#include "Trainer/Inventory.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace PokeVault::Integration::Gen4 {

// The eight native D/P/Pt/HGSS bag pockets. All offsets are relative to the
// CRC-validated selected General block, NOT a fixed .sav/.dsv file half.
// No item serializer, mutation, source write, or guess for unverified bytes.
enum class BagPocket : size_t {
    Items, KeyItems, Machines, Mail, Medicine, Berries, Balls, BattleItems,
};
inline constexpr size_t BagPocketCount=8;

struct BagPocketLayout {
    size_t offset=0;
    size_t slots=0;
};
using BagLayout=std::array<BagPocketLayout,BagPocketCount>;

[[nodiscard]] constexpr BagLayout bagLayout(Layout layout) noexcept {
    // Documented Gen IV save structures; a slot is two little-endian u16s
    // (native item ID and quantity) and takes exactly four bytes.
    switch(layout) {
        case Layout::DiamondPearl:
            return {{{0x624,165},{0x8B8,50},{0x980,100},{0xB10,12},
                     {0xB40,40},{0xBE0,64},{0xCE0,15},{0xD1C,56}}};
        case Layout::Platinum:
            return {{{0x630,165},{0x8C4,50},{0x98C,100},{0xB1C,12},
                     {0xB4C,40},{0xBEC,64},{0xCEC,15},{0xD28,30}}};
        case Layout::HeartGoldSoulSilver:
            return {{{0x644,165},{0x8D8,43},{0x9A4,100},{0xB34,12},
                     {0xB64,40},{0xC04,64},{0xD04,24},{0xD64,31}}};
    }
    return {};
}

[[nodiscard]] constexpr const char* bagPocketName(size_t index) noexcept {
    constexpr std::array<const char*,BagPocketCount> labels{{
        "Items","Key Items","TM/HM","Mail","Medicines","Berries",
        "Poké Balls","Battle Items",
    }};
    return index<labels.size()?labels[index]:"Unknown Pocket";
}

using ValidatedBag=std::array<std::vector<Trainer::InventoryItem>,BagPocketCount>;

// A corrupt bag does not invalidate otherwise verified Trainer/Party/Boxes.
// Unknown item IDs and invalid quantities fail closed instead of rendering
// fabricated values. Empty 0 / 0xFFFF terminator entries are not inventory.
[[nodiscard]] inline std::optional<ValidatedBag> decodeReadOnlyBag(
    const Gen4ReadOnlySave& source) {
    constexpr uint16_t kMaxGen4ItemId=536;
    constexpr uint16_t kMaxNativeBagCount=999;
    const auto bytes=source.sourceBytes();
    const size_t base=source.generalSelection().offset;
    const auto layout=bagLayout(source.layout());
    const size_t generalSize=source.layout()==Layout::DiamondPearl?0xC100:
        source.layout()==Layout::Platinum?0xCF2C:0xF628;
    ValidatedBag bag{};
    for(size_t pocket=0;pocket<layout.size();++pocket) {
        const auto spec=layout[pocket];
        if(spec.slots>generalSize/4 || spec.offset>generalSize-spec.slots*4)
            return std::nullopt;
        if(base>bytes.size() || generalSize>bytes.size()-base)
            return std::nullopt;
        bag[pocket].reserve(spec.slots);
        for(size_t slot=0;slot<spec.slots;++slot) {
            const size_t at=base+spec.offset+slot*4;
            const uint16_t id=uint16_t(bytes[at]) | (uint16_t(bytes[at+1])<<8);
            const uint16_t count=uint16_t(bytes[at+2]) | (uint16_t(bytes[at+3])<<8);
            if(id==0 || id==0xFFFF)continue;
            if(id>kMaxGen4ItemId || count>kMaxNativeBagCount)
                return std::nullopt;
            if(count==0)continue;
            bag[pocket].push_back(Trainer::InventoryItem{id,count,false,false});
        }
    }
    return bag;
}

} // namespace PokeVault::Integration::Gen4
#endif
