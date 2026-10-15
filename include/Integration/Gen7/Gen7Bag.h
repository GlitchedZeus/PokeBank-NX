#ifndef POKEBANK_GEN7_BAG_H
#define POKEBANK_GEN7_BAG_H

#include "Integration/Gen7/Gen7BagCatalog.h"
#include "Integration/Gen7/Gen7Save.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

// Native Sun/Moon and Ultra Sun/Ultra Moon bag (MyItem, save block 0). Sources, pinned
// PKHeX 6501f0ab: Items/Bags/PlayerBag7SM.cs / PlayerBag7USUM.cs (pouch offset, slot count,
// max count), Saves/Substructures/Inventory/Pouch/InventoryPouch7.cs and
// Inventory/Item/InventoryItem7.cs (slot = u32: item bits 0-9, count 10-19, free-space
// sort index 20-29, NEW flag bit 30, reserved bit 31), Items/ItemStorage7SM.cs /
// ItemStorage7USUM.cs (pouch lists, generated into Gen7BagCatalog.h).
namespace PokeVault::Integration::Gen7 {

enum class BagPocket7 : size_t { Items, Medicine, Machines, Berries, KeyItems, ZCrystals, BattleItems };
inline constexpr size_t BagPocket7Count = 7;  // BattleItems (Roto) exists in USUM only
inline constexpr size_t ItemsBlock7 = 0;

struct BagItem7 {
    uint16_t itemId = 0;
    uint16_t count = 0;
    bool operator==(const BagItem7&) const = default;
};
using ValidatedBag7 = std::array<std::vector<BagItem7>, BagPocket7Count>;

struct BagPocket7Layout {
    size_t offset = 0;
    size_t slots = 0;  // 0 => pocket absent in this family
    uint16_t maxCount = 0;
};

[[nodiscard]] inline BagPocket7Layout bagPocketLayout7(Layout::Family f, size_t pocket) noexcept {
    const bool sm = f == Layout::Family::SunMoon;
    switch (static_cast<BagPocket7>(pocket)) {
        case BagPocket7::Items: return {0x000, sm ? 430u : 427u, 999};
        case BagPocket7::Medicine: return {sm ? size_t{0xB48} : size_t{0xB74}, sm ? 64u : 60u, 999};
        case BagPocket7::Machines: return {sm ? size_t{0x998} : size_t{0x9C4}, 108, 1};
        case BagPocket7::Berries: return {sm ? size_t{0xC48} : size_t{0xC64}, sm ? 72u : 67u, 999};
        case BagPocket7::KeyItems: return {sm ? size_t{0x6B8} : size_t{0x6AC}, sm ? 184u : 198u, 1};
        case BagPocket7::ZCrystals: return {sm ? size_t{0xD68} : size_t{0xD70}, sm ? 30u : 35u, 1};
        case BagPocket7::BattleItems: return sm ? BagPocket7Layout{} : BagPocket7Layout{0xDFC, 11, 999};
    }
    return {};
}

[[nodiscard]] inline std::span<const uint16_t> bagPocketItems7(Layout::Family f, size_t pocket) noexcept {
    namespace C = BagCatalog;
    const bool sm = f == Layout::Family::SunMoon;
    switch (static_cast<BagPocket7>(pocket)) {
        case BagPocket7::Items: return C::kGeneral;
        case BagPocket7::Medicine: return C::kMedicine;
        case BagPocket7::Machines: return C::kMachine;
        case BagPocket7::Berries: return C::kBerry;
        case BagPocket7::KeyItems: return sm ? std::span<const uint16_t>(C::kKeySM) : std::span<const uint16_t>(C::kKeyUSUM);
        case BagPocket7::ZCrystals: return sm ? std::span<const uint16_t>(C::kZCrystalSM) : std::span<const uint16_t>(C::kZCrystalUSUM);
        case BagPocket7::BattleItems: return sm ? std::span<const uint16_t>{} : std::span<const uint16_t>(C::kRotoUSUM);
    }
    return {};
}

[[nodiscard]] constexpr const char* bagPocketName7(size_t pocket) noexcept {
    constexpr std::array<const char*, BagPocket7Count> names{{"Items", "Medicine", "TMs", "Berries", "Key Items",
                                                             "Z-Crystals", "Roto Powers"}};
    return pocket < names.size() ? names[pocket] : "Unknown Pocket";
}

[[nodiscard]] inline bool gen7BagItemInPocket(Layout::Family f, size_t pocket, uint16_t id) noexcept {
    for (const auto v : bagPocketItems7(f, pocket)) if (v == id) return true;
    return false;
}

// Never adds story pockets (Key Items, TMs, Z-Crystals) or PKHeX Unreleased items.
[[nodiscard]] inline bool gen7BagItemAddable(Layout::Family f, size_t pocket, uint16_t id) noexcept {
    const auto p = static_cast<BagPocket7>(pocket);
    if (p == BagPocket7::KeyItems || p == BagPocket7::Machines || p == BagPocket7::ZCrystals) return false;
    for (const auto v : BagCatalog::kUnreleased) if (v == id) return false;
    return gen7BagItemInPocket(f, pocket, id);
}

[[nodiscard]] inline size_t itemsBlockLength7(Layout::Family f) noexcept { return Layout::blocks(f)[ItemsBlock7].length; }
[[nodiscard]] inline size_t itemsBlockOffset7(Layout::Family f) noexcept { return Layout::blocks(f)[ItemsBlock7].offset; }

[[nodiscard]] constexpr uint16_t slotItem(uint32_t v) noexcept { return static_cast<uint16_t>(v & 0x3FF); }
[[nodiscard]] constexpr uint16_t slotCount(uint32_t v) noexcept { return static_cast<uint16_t>((v >> 10) & 0x3FF); }

// Strict decode: any occupied slot (item and count nonzero) must hold a native item of
// its pocket with count <= the pocket maximum; anything else fails the whole bag.
[[nodiscard]] inline std::optional<ValidatedBag7> decodeBag7(Layout::Family f, std::span<const uint8_t> block) {
    if (block.size() != itemsBlockLength7(f)) return std::nullopt;
    ValidatedBag7 bag{};
    for (size_t pocket = 0; pocket < BagPocket7Count; ++pocket) {
        const auto spec = bagPocketLayout7(f, pocket);
        if (spec.offset + spec.slots * 4 > block.size()) return std::nullopt;
        for (size_t slot = 0; slot < spec.slots; ++slot) {
            const uint32_t v = PK7::read32(block, spec.offset + slot * 4);
            const uint16_t id = slotItem(v), count = slotCount(v);
            if (id == 0 || count == 0) continue;
            if (!gen7BagItemInPocket(f, pocket, id) || count > spec.maxCount) return std::nullopt;
            bag[pocket].push_back({id, count});
        }
    }
    return bag;
}

[[nodiscard]] inline std::span<const uint8_t> sourceItemsBlock7(const Gen7Save& save) noexcept {
    return save.bytes().subspan(itemsBlockOffset7(save.family()), itemsBlockLength7(save.family()));
}

[[nodiscard]] inline bool bagDiffWithinSlots7(Layout::Family f, std::span<const uint8_t> a, std::span<const uint8_t> b) noexcept {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (a[i] == b[i]) continue;
        bool inside = false;
        for (size_t p = 0; p < BagPocket7Count && !inside; ++p) {
            const auto spec = bagPocketLayout7(f, p);
            inside = i >= spec.offset && i < spec.offset + spec.slots * 4;
        }
        if (!inside) return false;
    }
    return true;
}

// App-owned copy of the MyItem block; every operation re-decodes and audits that only the
// targeted u32 slot changed. Quantity edits keep the free-space index / NEW / reserved
// bits; Add writes item|count<<10 (sort index 0, NEW clear) like PKHeX's GetValue(false).
class Gen7StagedBag {
public:
    static std::optional<Gen7StagedBag> create(const Gen7Save& save, std::string* error = nullptr) {
        const auto block = sourceItemsBlock7(save);
        if (!decodeBag7(save.family(), block)) {
            if (error) *error = "Gen VII bag is not a fully recognized native bag";
            return std::nullopt;
        }
        return Gen7StagedBag(save.family(), std::vector<uint8_t>(block.begin(), block.end()));
    }

    [[nodiscard]] std::span<const uint8_t> original() const noexcept { return original_; }
    [[nodiscard]] std::span<const uint8_t> staged() const noexcept { return staged_; }
    [[nodiscard]] bool dirty() const noexcept { return staged_ != original_; }
    [[nodiscard]] ValidatedBag7 current() const { return *decodeBag7(family_, staged_); }
    void discard() noexcept { staged_ = original_; }

    [[nodiscard]] bool setQuantity(size_t pocket, size_t visibleIndex, uint16_t quantity, std::string* error = nullptr) {
        if (pocket >= BagPocket7Count || quantity == 0 || quantity > bagPocketLayout7(family_, pocket).maxCount)
            return fail(error, "Gen VII item quantity must be 1..the pocket maximum");
        size_t at = 0;
        if (!locate(pocket, visibleIndex, at, error)) return false;
        const uint32_t v = PK7::read32(staged_, at);
        return commit(at, (v & ~(0x3FFu << 10)) | (uint32_t{quantity} << 10), pocket, error);
    }

    [[nodiscard]] bool remove(size_t pocket, size_t visibleIndex, std::string* error = nullptr) {
        const auto p = static_cast<BagPocket7>(pocket);
        if (p == BagPocket7::KeyItems || p == BagPocket7::Machines || p == BagPocket7::ZCrystals)
            return fail(error, "Gen VII story pockets (Key Items, TMs, Z-Crystals) cannot be removed");
        size_t at = 0;
        if (!locate(pocket, visibleIndex, at, error)) return false;
        return commit(at, 0, pocket, error);
    }

    [[nodiscard]] bool add(size_t pocket, uint16_t itemId, uint16_t quantity, std::string* error = nullptr) {
        if (pocket >= BagPocket7Count || !gen7BagItemAddable(family_, pocket, itemId) || quantity == 0 ||
            quantity > bagPocketLayout7(family_, pocket).maxCount)
            return fail(error, "Gen VII Add rejected: item is not addable to this native pocket");
        const auto bag = current();
        for (const auto& stack : bag[pocket])
            if (stack.itemId == itemId) return fail(error, "Gen VII item already exists in this pocket; edit quantity instead");
        const auto spec = bagPocketLayout7(family_, pocket);
        size_t afterLast = 0;
        for (size_t slot = 0; slot < spec.slots; ++slot) {
            const uint32_t v = PK7::read32(staged_, spec.offset + slot * 4);
            if (slotItem(v) != 0 && slotCount(v) != 0) afterLast = slot + 1;
        }
        for (size_t slot = afterLast; slot < spec.slots; ++slot) {
            const size_t at = spec.offset + slot * 4;
            if (PK7::read32(staged_, at) == 0) return commit(at, itemId | (uint32_t{quantity} << 10), pocket, error);
        }
        return fail(error, "Gen VII Add refused: this pocket has no empty native slot");
    }

private:
    Gen7StagedBag(Layout::Family f, std::vector<uint8_t> block) : family_(f), original_(block), staged_(std::move(block)) {}
    static bool fail(std::string* error, const char* why) {
        if (error) *error = why;
        return false;
    }
    bool locate(size_t pocket, size_t visibleIndex, size_t& at, std::string* error) const {
        if (pocket >= BagPocket7Count) return fail(error, "Gen VII bag pocket is out of range");
        const auto spec = bagPocketLayout7(family_, pocket);
        size_t populated = 0;
        for (size_t slot = 0; slot < spec.slots; ++slot) {
            const size_t pos = spec.offset + slot * 4;
            const uint32_t v = PK7::read32(staged_, pos);
            if (slotItem(v) == 0 || slotCount(v) == 0) continue;
            if (populated++ == visibleIndex) {
                at = pos;
                return true;
            }
        }
        return fail(error, "Gen VII item row is out of range");
    }
    bool commit(size_t at, uint32_t value, size_t pocket, std::string* error) {
        auto backup = staged_;
        PK7::write32(staged_, at, value);
        const auto before = decodeBag7(family_, backup);
        const auto after = decodeBag7(family_, staged_);
        bool ok = before && after;
        for (size_t i = 0; ok && i < staged_.size(); ++i)
            if ((i < at || i >= at + 4) && staged_[i] != backup[i]) ok = false;
        for (size_t p = 0; ok && p < BagPocket7Count; ++p)
            if (p != pocket && (*after)[p] != (*before)[p]) ok = false;
        if (!ok) {
            staged_ = std::move(backup);
            return fail(error, "Gen VII bag change failed exact native pouch reparse");
        }
        if (error) error->clear();
        return true;
    }

    Layout::Family family_;
    std::vector<uint8_t> original_;
    std::vector<uint8_t> staged_;
};

} // namespace PokeVault::Integration::Gen7

#endif
