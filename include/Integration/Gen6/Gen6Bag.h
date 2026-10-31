#ifndef POKEBANK_GEN6_BAG_H
#define POKEBANK_GEN6_BAG_H

#include "Integration/Gen6/Gen6BagCatalog.h"
#include "Integration/Gen6/Gen6Save.h"
#include "Integration/Gen6/Gen6SaveLayout.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

// Native X/Y and OR/AS player bag (MyItem, save block 1). Sources, pinned PKHeX
// 6501f0ab: Items/Bags/PlayerBag6XY.cs, PlayerBag6AO.cs (pouch offsets within the
// block and max counts), Saves/Substructures/Inventory/Pouch/InventoryPouch4.cs
// (slot = u16 item, u16 count; slot count = the pouch's item-ID list length) and
// Items/ItemStorage6XY.cs / 6AO.cs (lists, generated into Gen6BagCatalog.h).
// PKSM-Core aa22d7a4 SavXY/SavORAS uses the same absolute pouch offsets.
namespace PokeVault::Integration::Gen6 {

enum class BagPocket6 : size_t { Items, KeyItems, Machines, Medicine, Berries };
inline constexpr size_t BagPocket6Count = 5;
inline constexpr size_t ItemsBlock = 1;

struct BagItem6 {
    uint16_t itemId = 0;
    uint16_t count = 0;
    bool operator==(const BagItem6&) const = default;
};
using ValidatedBag6 = std::array<std::vector<BagItem6>, BagPocket6Count>;

struct BagPocket6Layout {
    size_t offset = 0;  // relative to the MyItem block
    size_t slots = 0;
    uint16_t maxCount = 0;
};

[[nodiscard]] inline std::span<const uint16_t> bagPocketItems6(Layout::Family f, size_t pocket) noexcept {
    namespace C = BagCatalog;
    const bool xy = f == Layout::Family::XY;
    switch (static_cast<BagPocket6>(pocket)) {
        case BagPocket6::Items: return xy ? std::span<const uint16_t>(C::kGeneralXY) : std::span<const uint16_t>(C::kGeneralAO);
        case BagPocket6::KeyItems: return xy ? std::span<const uint16_t>(C::kKeyXY) : std::span<const uint16_t>(C::kKeyAO);
        case BagPocket6::Machines: return xy ? std::span<const uint16_t>(C::kMachineXY) : std::span<const uint16_t>(C::kMachineAO);
        case BagPocket6::Medicine: return xy ? std::span<const uint16_t>(C::kMedicineXY) : std::span<const uint16_t>(C::kMedicineAO);
        case BagPocket6::Berries: return C::kBerry;
    }
    return {};
}

[[nodiscard]] inline BagPocket6Layout bagPocketLayout6(Layout::Family f, size_t pocket) noexcept {
    const bool xy = f == Layout::Family::XY;
    const size_t slots = bagPocketItems6(f, pocket).size();
    switch (static_cast<BagPocket6>(pocket)) {
        case BagPocket6::Items: return {0x000, slots, 999};
        case BagPocket6::KeyItems: return {0x640, slots, 1};
        case BagPocket6::Machines: return {0x7C0, slots, 1};
        case BagPocket6::Medicine: return {xy ? size_t{0x968} : size_t{0x970}, slots, 999};
        case BagPocket6::Berries: return {xy ? size_t{0xA68} : size_t{0xA70}, slots, 999};
    }
    return {};
}

[[nodiscard]] constexpr const char* bagPocketName6(size_t pocket) noexcept {
    constexpr std::array<const char*, BagPocket6Count> names{{"Items", "Key Items", "TMs & HMs", "Medicine", "Berries"}};
    return pocket < names.size() ? names[pocket] : "Unknown Pocket";
}

[[nodiscard]] inline bool gen6BagItemInPocket(Layout::Family f, size_t pocket, uint16_t id) noexcept {
    for (const auto v : bagPocketItems6(f, pocket)) if (v == id) return true;
    return false;
}

// Add choices: a native pocket member, not in PKHeX's Unreleased list, and never a
// Key Item or TM/HM (story progress items; adding them can desynchronise flags).
[[nodiscard]] inline bool gen6BagItemAddable(Layout::Family f, size_t pocket, uint16_t id) noexcept {
    if (pocket == static_cast<size_t>(BagPocket6::KeyItems) || pocket == static_cast<size_t>(BagPocket6::Machines))
        return false;
    for (const auto v : BagCatalog::kUnreleased) if (v == id) return false;
    return gen6BagItemInPocket(f, pocket, id);
}

[[nodiscard]] inline size_t itemsBlockLength6(Layout::Family f) noexcept { return Layout::blocks(f)[ItemsBlock].length; }
[[nodiscard]] inline size_t itemsBlockOffset6(Layout::Family f) noexcept { return Layout::blocks(f)[ItemsBlock].offset; }

// Strict decode of one MyItem block. Any slot holding an item that is not native
// to its pocket, or a count above the pocket maximum, fails the whole decode: an
// unrecognized bag is shown as unavailable, never guessed. Zero id/count = empty.
[[nodiscard]] inline std::optional<ValidatedBag6> decodeBag6(Layout::Family f, std::span<const uint8_t> block) {
    if (block.size() != itemsBlockLength6(f)) return std::nullopt;
    ValidatedBag6 bag{};
    for (size_t pocket = 0; pocket < BagPocket6Count; ++pocket) {
        const auto spec = bagPocketLayout6(f, pocket);
        if (spec.offset + spec.slots * 4 > block.size()) return std::nullopt;
        for (size_t slot = 0; slot < spec.slots; ++slot) {
            const size_t at = spec.offset + slot * 4;
            const uint16_t id = PK6::read16(block, at);
            const uint16_t count = PK6::read16(block, at + 2);
            if (id == 0 || count == 0) continue;
            if (!gen6BagItemInPocket(f, pocket, id) || count > spec.maxCount) return std::nullopt;
            bag[pocket].push_back({id, count});
        }
    }
    return bag;
}

[[nodiscard]] inline std::span<const uint8_t> sourceItemsBlock6(const Gen6Save& save) noexcept {
    return save.bytes().subspan(itemsBlockOffset6(save.family()), itemsBlockLength6(save.family()));
}

// True when every differing byte of two MyItem blocks lies inside a pocket's slot array.
[[nodiscard]] inline bool bagDiffWithinSlots(Layout::Family f, std::span<const uint8_t> a,
                                             std::span<const uint8_t> b) noexcept {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (a[i] == b[i]) continue;
        bool inside = false;
        for (size_t p = 0; p < BagPocket6Count && !inside; ++p) {
            const auto spec = bagPocketLayout6(f, p);
            inside = i >= spec.offset && i < spec.offset + spec.slots * 4;
        }
        if (!inside) return false;
    }
    return true;
}

// App-owned copy of the MyItem block. Every operation re-decodes the result and
// audits that only the targeted 4-byte slot changed; failures restore the prior bytes.
class Gen6StagedBag {
public:
    static std::optional<Gen6StagedBag> create(const Gen6Save& save, std::string* error = nullptr) {
        const auto block = sourceItemsBlock6(save);
        if (!decodeBag6(save.family(), block)) {
            if (error) *error = "Gen VI bag is not a fully recognized native bag";
            return std::nullopt;
        }
        return Gen6StagedBag(save.family(), std::vector<uint8_t>(block.begin(), block.end()));
    }

    [[nodiscard]] Layout::Family family() const noexcept { return family_; }
    [[nodiscard]] std::span<const uint8_t> original() const noexcept { return original_; }
    [[nodiscard]] std::span<const uint8_t> staged() const noexcept { return staged_; }
    [[nodiscard]] bool dirty() const noexcept { return staged_ != original_; }
    [[nodiscard]] ValidatedBag6 current() const { return *decodeBag6(family_, staged_); }
    void discard() noexcept { staged_ = original_; }

    [[nodiscard]] bool setQuantity(size_t pocket, size_t visibleIndex, uint16_t quantity, std::string* error = nullptr) {
        if (pocket >= BagPocket6Count || quantity == 0 || quantity > bagPocketLayout6(family_, pocket).maxCount)
            return fail(error, "Gen VI item quantity must be 1..the pocket maximum");
        size_t at = 0;
        BagItem6 expected{};
        if (!locate(pocket, visibleIndex, at, expected, error)) return false;
        if (quantity == expected.count) { if (error) error->clear(); return true; }
        return commit(at, expected.itemId, quantity, pocket, error);
    }

    [[nodiscard]] bool remove(size_t pocket, size_t visibleIndex, std::string* error = nullptr) {
        if (pocket == static_cast<size_t>(BagPocket6::KeyItems) || pocket == static_cast<size_t>(BagPocket6::Machines))
            return fail(error, "Gen VI Key Items and TMs/HMs are story items; removal is disabled");
        size_t at = 0;
        BagItem6 expected{};
        if (!locate(pocket, visibleIndex, at, expected, error)) return false;
        return commit(at, 0, 0, pocket, error);
    }

    // Appends after the last occupied slot of the pocket (stable order).
    [[nodiscard]] bool add(size_t pocket, uint16_t itemId, uint16_t quantity, std::string* error = nullptr) {
        if (pocket >= BagPocket6Count || !gen6BagItemAddable(family_, pocket, itemId) || quantity == 0 ||
            quantity > bagPocketLayout6(family_, pocket).maxCount)
            return fail(error, "Gen VI Add rejected: item is not addable to this native pocket");
        const auto bag = current();  // keep the decoded bag alive for the loop
        for (const auto& stack : bag[pocket])
            if (stack.itemId == itemId) return fail(error, "Gen VI item already exists in this pocket; edit quantity instead");
        const auto spec = bagPocketLayout6(family_, pocket);
        size_t afterLast = 0;
        for (size_t slot = 0; slot < spec.slots; ++slot) {
            const size_t at = spec.offset + slot * 4;
            if (PK6::read16(staged_, at) != 0 && PK6::read16(staged_, at + 2) != 0) afterLast = slot + 1;
        }
        for (size_t slot = afterLast; slot < spec.slots; ++slot) {
            const size_t at = spec.offset + slot * 4;
            if (PK6::read16(staged_, at) == 0 && PK6::read16(staged_, at + 2) == 0)
                return commit(at, itemId, quantity, pocket, error);
        }
        return fail(error, "Gen VI Add refused: this pocket has no empty native slot");
    }

private:
    Gen6StagedBag(Layout::Family f, std::vector<uint8_t> block) : family_(f), original_(block), staged_(std::move(block)) {}

    static bool fail(std::string* error, const char* why) {
        if (error) *error = why;
        return false;
    }
    bool locate(size_t pocket, size_t visibleIndex, size_t& at, BagItem6& expected, std::string* error) const {
        if (pocket >= BagPocket6Count) return fail(error, "Gen VI bag pocket is out of range");
        const auto bag = current();
        if (visibleIndex >= bag[pocket].size()) return fail(error, "Gen VI item row is out of range");
        expected = bag[pocket][visibleIndex];
        const auto spec = bagPocketLayout6(family_, pocket);
        size_t populated = 0;
        for (size_t slot = 0; slot < spec.slots; ++slot) {
            const size_t pos = spec.offset + slot * 4;
            if (PK6::read16(staged_, pos) == 0 || PK6::read16(staged_, pos + 2) == 0) continue;
            if (populated++ == visibleIndex) {
                at = pos;
                return true;
            }
        }
        return fail(error, "Gen VI item stack no longer exists");
    }
    bool commit(size_t at, uint16_t id, uint16_t count, size_t pocket, std::string* error) {
        auto backup = staged_;
        PK6::write16(staged_, at, id);
        PK6::write16(staged_, at + 2, count);
        const auto before = decodeBag6(family_, backup);
        const auto after = decodeBag6(family_, staged_);
        bool ok = before && after;
        for (size_t i = 0; ok && i < staged_.size(); ++i)
            if ((i < at || i >= at + 4) && staged_[i] != backup[i]) ok = false;
        for (size_t p = 0; ok && p < BagPocket6Count; ++p)
            if (p != pocket && (*after)[p] != (*before)[p]) ok = false;
        if (!ok) {
            staged_ = std::move(backup);
            return fail(error, "Gen VI bag change failed exact native pouch reparse");
        }
        if (error) error->clear();
        return true;
    }

    Layout::Family family_;
    std::vector<uint8_t> original_;
    std::vector<uint8_t> staged_;
};

} // namespace PokeVault::Integration::Gen6

#endif
