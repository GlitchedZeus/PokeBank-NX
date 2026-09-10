#include "Integration/Gen3/PKSMGen3Adapter.h"
#include "Legacy/FRLGReadOnlyTrainer.h"
#include "Utils/Gen3Text.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <span>
#include <string>
#include <vector>

namespace {
    using PokeVault::Integration::Gen3::SourceGame;

    constexpr size_t kSaveSize = 0x20000;
    constexpr size_t kSectorSize = 0x1000;
    constexpr size_t kSectorCount = 14;
    constexpr size_t kSlotBase[2] = {0, 0xE000};
    constexpr std::array<size_t, kSectorCount> kChunkLengths = {
        0xF2C, 0xF80, 0xF80, 0xF80, 0xF08, 0xF80, 0xF80,
        0xF80, 0xF80, 0xF80, 0xF80, 0xF80, 0xF80, 0x7D0,
    };

    using LogicalSectors = std::array<std::array<uint8_t, kSectorSize>, kSectorCount>;

    enum class InventoryCase {
        Valid,
        InvalidNonEmpty,
        StaleZeroCount,
    };

    void write16(std::span<uint8_t> bytes, size_t offset, uint16_t value) {
        bytes[offset] = static_cast<uint8_t>(value);
        bytes[offset + 1] = static_cast<uint8_t>(value >> 8);
    }

    void write32(std::span<uint8_t> bytes, size_t offset, uint32_t value) {
        write16(bytes, offset, static_cast<uint16_t>(value));
        write16(bytes, offset + 2, static_cast<uint16_t>(value >> 16));
    }

    uint16_t sectorChecksum(std::span<const uint8_t> data) {
        uint32_t sum = 0;
        for (size_t offset = 0; offset < data.size(); offset += 4) {
            uint32_t word = data[offset];
            if (offset + 1 < data.size()) word |= static_cast<uint32_t>(data[offset + 1]) << 8;
            if (offset + 2 < data.size()) word |= static_cast<uint32_t>(data[offset + 2]) << 16;
            if (offset + 3 < data.size()) word |= static_cast<uint32_t>(data[offset + 3]) << 24;
            sum += word;
        }
        return static_cast<uint16_t>((sum & 0xFFFFu) + (sum >> 16));
    }

    void writeName(std::span<uint8_t> bytes, size_t offset, size_t length,
                   std::u16string_view name) {
        std::fill_n(bytes.begin() + static_cast<std::ptrdiff_t>(offset), length,
                    Utils::GEN3_TERMINATOR);
        for (size_t index = 0; index < std::min(length, name.size()); ++index)
            bytes[offset + index] = Utils::charToGen3(name[index]);
    }

    bool emerald(SourceGame game) {
        return game == SourceGame::EmeraldGBA;
    }

    LogicalSectors makeLogical(SourceGame game, InventoryCase inventoryCase) {
        LogicalSectors logical{};
        writeName(logical[0], 0x00, 8, u"WILL");
        logical[0][0x08] = 1;
        write16(logical[0], 0x0A, 54321);
        write16(logical[0], 0x0C, 12345);

        const uint32_t securityKey = emerald(game) ? 0xA1B2C3D4u : 0u;
        write32(logical[0], 0xAC, securityKey);
        if (emerald(game)) logical[0][0xEE0] = 0x42;
        write32(logical[1], 0x490, 2100u ^ securityKey);

        // Empty party and boxes are intentional here. The test is about whether those models remain
        // accessible when inventory is unavailable, not about duplicating the existing PK3 fixture.
        logical[1][0x234] = 0;

        const uint16_t key16 = static_cast<uint16_t>(securityKey);
        const auto stored = [&](uint16_t decoded) {
            return emerald(game) ? static_cast<uint16_t>(decoded ^ key16) : decoded;
        };
        write16(logical[1], 0x0560, 13);       // Potion
        write16(logical[1], 0x0562, stored(3));

        if (inventoryCase == InventoryCase::InvalidNonEmpty) {
            // A genuinely non-empty out-of-range entry makes only the optional inventory model
            // unavailable. It must not invalidate sector/save/trainer/party/box access.
            write16(logical[1], 0x0564, 377);
            write16(logical[1], 0x0566, stored(1));
        } else if (inventoryCase == InventoryCase::StaleZeroCount) {
            // Independent Gen III implementations load fixed-width slots and treat count-zero
            // entries as unowned/clearable state. Preserve a deliberately stale item id here.
            write16(logical[1], 0x0564, 600);
            write16(logical[1], 0x0566, stored(0));
        }
        return logical;
    }

    void writeSlot(std::vector<uint8_t>& save, uint8_t slot, uint32_t counter,
                   uint8_t rotation, SourceGame game, InventoryCase inventoryCase) {
        auto logical = makeLogical(game, inventoryCase);
        for (size_t physical = 0; physical < kSectorCount; ++physical) {
            const uint16_t id = static_cast<uint16_t>((physical + rotation) % kSectorCount);
            const size_t offset = kSlotBase[slot] + physical * kSectorSize;
            std::copy(logical[id].begin(), logical[id].end(), save.begin() + offset);
            write16(save, offset + 0xFF4, id);
            write16(save, offset + 0xFF6, sectorChecksum(
                std::span<const uint8_t>(save.data() + offset, kChunkLengths[id])));
            write32(save, offset + 0xFF8, 0x08012025);
            write32(save, offset + 0xFFC, counter);
        }
    }

    std::vector<uint8_t> makeSave(SourceGame game, InventoryCase inventoryCase) {
        std::vector<uint8_t> save(kSaveSize, 0);
        writeSlot(save, 0, 7, 0, game, inventoryCase);
        writeSlot(save, 1, 9, 5, game, inventoryCase);
        return save;
    }

    void assertValidInventoryAndOpen(SourceGame game) {
        using namespace PokeVault::Integration::Gen3;
        auto source = makeSave(game, InventoryCase::Valid);
        const auto original = source;
        auto parsed = parse(source, game);
        assert(parsed && parsed.error == SaveError::None);
        assert(source == original);
        assert(parsed.save->trainer().name == "WILL");
        assert(parsed.save->trainer().money == 2100);
        assert(parsed.save->inventory().size() == 6);
        assert(parsed.save->inventory()[0].items.size() == 1);
        assert(parsed.save->inventory()[0].items[0].itemId == 13);
        assert(parsed.save->inventory()[0].items[0].count == 3);
        assert(parsed.save->party().empty());
        assert(parsed.save->lastEnumerationError() == SaveError::None);
        assert(parsed.save->boxes().empty());
        assert(parsed.save->lastEnumerationError() == SaveError::None);

        std::string error;
        auto trainer = PokeVault::Legacy::FRLGReadOnlyTrainer::create(*parsed.save, error);
        assert(trainer && error.empty());
        assert(trainer->trainerName == "WILL");
        assert(trainer->money == 2100);
        assert(trainer->items.size() == 6);
        assert(trainer->items[0].size() == 1);
        assert(trainer->items[0][0].itemId == 13 && trainer->items[0][0].count == 3);
        assert(trainer->party.empty());
        assert(trainer->boxes.size() == 14);
        assert(source == original);
    }

    void assertBadInventoryStillOpens(SourceGame game) {
        using namespace PokeVault::Integration::Gen3;
        auto source = makeSave(game, InventoryCase::InvalidNonEmpty);
        const auto original = source;
        auto parsed = parse(source, game);

        // This is the hardware regression contract: inventory failure is secondary, so parse/open
        // still succeeds and all non-inventory read models remain available.
        assert(parsed && parsed.error == SaveError::None);
        assert(source == original);
        assert(parsed.save->trainer().name == "WILL");
        assert(parsed.save->trainer().money == 2100);
        assert(parsed.save->inventory().empty());
        assert(parsed.save->party().empty());
        assert(parsed.save->lastEnumerationError() == SaveError::None);
        assert(parsed.save->boxes().empty());
        assert(parsed.save->lastEnumerationError() == SaveError::None);

        std::string error;
        auto trainer = PokeVault::Legacy::FRLGReadOnlyTrainer::create(*parsed.save, error);
        assert(trainer && error.empty());
        assert(trainer->trainerName == "WILL");
        assert(trainer->money == 2100);
        assert(trainer->items.empty()); // ItemsPanel maps this precise RSE state to unavailable.
        assert(trainer->party.empty());
        assert(trainer->boxes.size() == 14);
        assert(source == original);
    }

    void assertStaleZeroCountAccepted(SourceGame game) {
        using namespace PokeVault::Integration::Gen3;
        auto source = makeSave(game, InventoryCase::StaleZeroCount);
        const auto original = source;
        auto parsed = parse(source, game);
        assert(parsed && parsed.error == SaveError::None);
        assert(parsed.save->inventory().size() == 6);
        assert(parsed.save->inventory()[0].items.size() == 1);
        assert(parsed.save->inventory()[0].items[0].itemId == 13);
        assert(parsed.save->inventory()[0].items[0].count == 3);
        assert(source == original);
    }
}

int main() {
    using PokeVault::Integration::Gen3::SourceGame;

    for (const auto game : {SourceGame::RubyGBA, SourceGame::SapphireGBA,
                            SourceGame::EmeraldGBA}) {
        assertValidInventoryAndOpen(game);
        assertBadInventoryStillOpens(game);
        assertStaleZeroCountAccepted(game);
    }

    std::cout << "RSE save-open/inventory regression tests passed\n";
    return 0;
}
