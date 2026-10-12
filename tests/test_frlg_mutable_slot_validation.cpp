#include "Integration/Gen3/Gen3SaveValidation.h"

#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <span>
#include <vector>

using namespace PokeVault::Integration::Gen3;

namespace {
void write16(std::span<uint8_t> bytes, std::size_t offset, uint16_t value) {
    bytes[offset] = static_cast<uint8_t>(value);
    bytes[offset + 1] = static_cast<uint8_t>(value >> 8);
}

void write32(std::span<uint8_t> bytes, std::size_t offset, uint32_t value) {
    write16(bytes, offset, static_cast<uint16_t>(value));
    write16(bytes, offset + 2, static_cast<uint16_t>(value >> 16));
}

void writeSlot(std::vector<uint8_t>& save, uint8_t slot, uint32_t counter, uint8_t rotation) {
    auto bytes = std::span<uint8_t>(save);
    for (std::size_t physical = 0; physical < Detail::kSectorCount; ++physical) {
        const uint16_t id = static_cast<uint16_t>(
            (physical + rotation) % Detail::kSectorCount);
        const std::size_t offset =
            Detail::kSlotBases[slot] + physical * Detail::kSectorSize;

        // FRLG family discriminator lives in logical sector 0.
        if (id == 0) write32(bytes, offset + 0xAC, 1);

        write16(bytes, offset + 0xFF4, id);
        write32(bytes, offset + 0xFF8, Detail::kSectorSignature);
        write32(bytes, offset + 0xFFC, counter);
        write16(bytes, offset + 0xFF6,
                Detail::sectorChecksum(
                    std::span<const uint8_t>(save).subspan(
                        offset, Detail::kChunkLengths[id])));
    }
}

std::vector<uint8_t> fixture() {
    std::vector<uint8_t> save(Detail::kSaveSize, 0);
    writeSlot(save, 0, 7, 0);
    writeSlot(save, 1, 9, 5);
    return save;
}
}

int main() {
    auto save = fixture();
    auto bytes = std::span<const uint8_t>(save);

    Detail::SlotValidation slots[2] = {
        Detail::validateSlot(bytes, 0),
        Detail::validateSlot(bytes, 1),
    };
    assert(slots[0].valid && slots[1].valid);
    assert(Detail::selectActiveSlot(slots) == 1);
    assert(Detail::detectFamily(bytes, slots[1]) ==
           Detail::SaveFamily::FireRedLeafGreen);

    // AUDIT-017: the higher-counter slot is not authoritative if its checksum is bad.
    save[Detail::kSlotBases[1] + 0xFF6] ^= 0xFF;
    bytes = std::span<const uint8_t>(save);
    slots[0] = Detail::validateSlot(bytes, 0);
    slots[1] = Detail::validateSlot(bytes, 1);
    assert(slots[0].valid && !slots[1].valid);
    assert(Detail::selectActiveSlot(slots) == 0);
    assert(Detail::detectFamily(bytes, slots[0]) ==
           Detail::SaveFamily::FireRedLeafGreen);

    // If both rotating slots are damaged, mutable open has no recovery candidate.
    save[Detail::kSlotBases[0] + 0xFF6] ^= 0xFF;
    bytes = std::span<const uint8_t>(save);
    slots[0] = Detail::validateSlot(bytes, 0);
    slots[1] = Detail::validateSlot(bytes, 1);
    assert(!slots[0].valid && !slots[1].valid);

    std::cout << "FRLG checksum-valid rotating-slot selection: PASS\n";
    return 0;
}
