#include "Integration/Gen1/Gen1ReadOnlyInventory.h"
#include "Names/ItemNames.h"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <span>
#include <string>
#include <utility>
#include <vector>

using namespace PokeVault::Integration::Gen1;

namespace {
constexpr size_t INT_BAG = 0x25C9;
constexpr size_t INT_PC = 0x27E6;
constexpr size_t JPN_BAG = 0x25C4;
constexpr size_t JPN_PC = 0x27DC;

void writeBlock(std::vector<uint8_t>& raw, size_t offset,
                std::initializer_list<std::pair<uint8_t, uint8_t>> entries) {
    raw[offset] = static_cast<uint8_t>(entries.size());
    size_t i = 0;
    for (const auto& [item, qty] : entries) {
        raw[offset + 1 + i * 2] = item;
        raw[offset + 2 + i * 2] = qty;
        ++i;
    }
    raw[offset + 1 + i * 2] = 0xFF;
}

std::vector<uint8_t> validInternational() {
    std::vector<uint8_t> raw(0x8000, 0);
    writeBlock(raw, INT_BAG, {{0x14, 3}, {0x2D, 1}, {0xC9, 2}, {0xC4, 1}});
    writeBlock(raw, INT_PC, {{0x01, 7}, {0x53, 4}});
    return raw;
}

std::vector<uint8_t> validJapanese() {
    std::vector<uint8_t> raw(0x8000, 0);
    writeBlock(raw, JPN_BAG, {{0x14, 5}, {0xFA, 1}});
    writeBlock(raw, JPN_PC, {{0x2D, 1}, {0xC8, 1}});
    return raw;
}
}

int main() {
    {
        auto raw = validInternational();
        const auto before = raw;
        const auto inv = decodeInventory(raw, RegionLayout::International);
        assert(inv.available && inv.error.empty());
        assert(raw == before); // decoder is strictly read-only.
        assert(inv.bag.size() == 4);
        assert(inv.bag[0].itemId == 0x14 && inv.bag[0].quantity == 3);
        assert(inv.bag[1].itemId == 0x2D && inv.bag[1].quantity == 1); // key item stays in Bag.
        assert(inv.bag[2].itemId == 0xC9 && inv.bag[2].quantity == 2); // TM01 is a normal item entry.
        assert(inv.bag[3].itemId == 0xC4 && inv.bag[3].quantity == 1); // HM01 likewise.
        assert(inv.pcItems.size() == 2);
        assert(inv.pcItems[0].itemId == 0x01 && inv.pcItems[0].quantity == 7);
    }
    {
        auto raw = validJapanese();
        const auto inv = decodeInventory(raw, RegionLayout::Japanese);
        assert(inv.available);
        assert(inv.bag.size() == 2 && inv.bag[0].quantity == 5);
        assert(inv.pcItems.size() == 2 && inv.pcItems[0].itemId == 0x2D);
    }
    {
        std::vector<uint8_t> raw(0x8000, 0);
        writeBlock(raw, INT_BAG, {});
        writeBlock(raw, INT_PC, {});
        const auto inv = decodeInventory(raw, RegionLayout::International);
        assert(inv.available && inv.bag.empty() && inv.pcItems.empty());
    }
    {
        auto raw = validInternational();
        raw[INT_BAG] = 21;
        assert(!decodeInventory(raw, RegionLayout::International).available);
    }
    {
        auto raw = validInternational();
        raw[INT_PC] = 51;
        assert(!decodeInventory(raw, RegionLayout::International).available);
    }
    {
        auto raw = validInternational();
        raw[INT_BAG + 1 + raw[INT_BAG] * 2] = 0;
        assert(!decodeInventory(raw, RegionLayout::International).available);
    }
    {
        auto raw = validInternational();
        raw[INT_BAG + 1] = 0x07; // not accepted by pinned PKSM-Core Sav1::validItems1.
        assert(!decodeInventory(raw, RegionLayout::International).available);
    }
    {
        auto raw = validInternational();
        raw[INT_BAG + 2] = 0;
        assert(!decodeInventory(raw, RegionLayout::International).available);
        raw = validInternational();
        raw[INT_BAG + 2] = 100;
        assert(!decodeInventory(raw, RegionLayout::International).available);
    }
    {
        auto raw = validInternational();
        raw.resize(INT_BAG + 10);
        assert(!decodeInventory(raw, RegionLayout::International).available);
    }
    {
        auto raw = validInternational();
        raw.resize(INT_PC + 20);
        assert(!decodeInventory(raw, RegionLayout::International).available);
    }

    static_assert(kInventoryCategoryCount == 2);
    assert(std::string(inventoryCategoryName(0)) == "Bag");
    assert(std::string(inventoryCategoryName(1)) == "PC Items");
    assert(std::string(inventoryCategoryName(2)) == "?");

    assert(std::string(Names::getItemNameG1(0x14)) == "Potion");
    assert(std::string(Names::getItemNameG1(0x2D)) == "Bike Voucher");
    assert(std::string(Names::getItemNameG1(0xC4)) == "HM01");
    assert(std::string(Names::getItemNameG1(0xC8)) == "HM05");
    assert(std::string(Names::getItemNameG1(0xC9)) == "TM01");
    assert(std::string(Names::getItemNameG1(0xFA)) == "TM50");
    assert(std::string(Names::getItemNameG1(0x07)) == "???");

    std::cout << "RBY strict read-only inventory tests passed\n";
    return 0;
}
