// Q05 (independent QA 2026-10-27; compare upstream PKSE 0c993bc, which stopped requiring the
// sector signature): PokeBank NX keeps the 0x08012025 signature requirement, because the games
// themselves require it. pret pokefirered ce6406d8 src/save.c:
//   - GetSaveValidStatus (466-560): a sector counts only if signature == SECTOR_SIGNATURE and its
//     checksum matches; a slot is OK only when all 14 sectors count; a slot with no signature at
//     all is SAVE_STATUS_EMPTY, and the game loads the other slot.
//   - WriteSectorSignatureByte (399-415): the signature's low byte is written LAST, as the commit
//     marker of a save. A torn write therefore shows as a bad signature in the newer slot.
// Dropping the check would let such a torn newer slot (higher counter) win over the good older
// one. This test pins the game's semantics alongside every compensating structural check.
#include "Integration/Gen3/Gen3SaveValidation.h"

#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <span>
#include <vector>

using namespace PokeVault::Integration::Gen3;

namespace {
void put16(std::vector<uint8_t>& b, std::size_t o, uint16_t v) { b[o] = uint8_t(v); b[o + 1] = uint8_t(v >> 8); }
void put32(std::vector<uint8_t>& b, std::size_t o, uint32_t v) { put16(b, o, uint16_t(v)); put16(b, o + 2, uint16_t(v >> 16)); }

std::size_t sectorAt(uint8_t slot, std::size_t physical) {
    return Detail::kSlotBases[slot] + physical * Detail::kSectorSize;
}
void reseal(std::vector<uint8_t>& save, uint8_t slot, std::size_t physical) {
    const std::size_t o = sectorAt(slot, physical);
    const uint16_t id = uint16_t(save[o + 0xFF4] | (save[o + 0xFF5] << 8));
    put16(save, o + 0xFF6, Detail::sectorChecksum(std::span<const uint8_t>(save).subspan(o, Detail::kChunkLengths[id % 14])));
}
void writeSlot(std::vector<uint8_t>& save, uint8_t slot, uint32_t counter, uint8_t rotation, uint8_t fill) {
    for (std::size_t physical = 0; physical < Detail::kSectorCount; ++physical) {
        const uint16_t id = uint16_t((physical + rotation) % Detail::kSectorCount);
        const std::size_t o = sectorAt(slot, physical);
        for (std::size_t i = 0; i < Detail::kSectorDataSize; ++i) save[o + i] = uint8_t(fill + id + i);
        if (id == 0) put32(save, o + 0xAC, 1);  // FRLG marker
        put16(save, o + 0xFF4, id);
        put32(save, o + 0xFF8, Detail::kSectorSignature);
        put32(save, o + 0xFFC, counter);
        reseal(save, slot, physical);
    }
}
std::vector<uint8_t> twoSlots() {
    std::vector<uint8_t> save(Detail::kSaveSize, 0);
    writeSlot(save, 0, 7, 0, 0x10);   // older, good
    writeSlot(save, 1, 8, 3, 0x40);   // newer, good
    return save;
}
uint8_t active(const std::vector<uint8_t>& save, Detail::SlotValidation (&slots)[2]) {
    slots[0] = Detail::validateSlot(save, 0);
    slots[1] = Detail::validateSlot(save, 1);
    return Detail::selectActiveSlot(slots);
}
}  // namespace

int main() {
    Detail::SlotValidation slots[2];

    // Both slots good: the newer counter wins.
    {
        auto save = twoSlots();
        assert(active(save, slots) == 1 && slots[0].valid && slots[1].valid);
    }
    // Torn newer save: its last sector's commit byte (signature low byte) never landed. The data
    // and checksum are complete and the counter is higher; the game loads the OLDER slot.
    {
        auto save = twoSlots();
        save[sectorAt(1, 13) + 0xFF8] = 0xFF;  // erased flash byte, signature 0x080120FF
        assert(active(save, slots) == 0);
        assert(!slots[1].valid && slots[1].error == SaveError::BadSectorSignature);
    }
    // A never-written slot (no signature anywhere) is EMPTY to the game: the other slot is used.
    {
        auto save = twoSlots();
        for (std::size_t p = 0; p < Detail::kSectorCount; ++p) { put32(save, sectorAt(1, p) + 0xFF8, 0); }
        assert(active(save, slots) == 0 && slots[0].valid && !slots[1].valid);
    }
    // Signature-free in BOTH slots, with every id, counter and checksum consistent: the game sees an
    // empty save, so PokeBank NX opens nothing (upstream PKSE 0c993bc would accept this file).
    {
        auto save = twoSlots();
        for (uint8_t s = 0; s < 2; ++s)
            for (std::size_t p = 0; p < Detail::kSectorCount; ++p) put32(save, sectorAt(s, p) + 0xFF8, 0);
        active(save, slots);
        assert(!slots[0].valid && !slots[1].valid);
    }
    // Compensating structural checks, each of which alone invalidates the newer slot.
    {
        auto save = twoSlots();                                  // bad checksum
        save[sectorAt(1, 2) + 0x10] ^= 0xFF;
        assert(active(save, slots) == 0 && slots[1].error == SaveError::BadSectorChecksum);
    }
    {
        auto save = twoSlots();                                  // duplicate sector id
        put16(save, sectorAt(1, 5) + 0xFF4, uint16_t(save[sectorAt(1, 4) + 0xFF4]));
        reseal(save, 1, 5);
        assert(active(save, slots) == 0 && slots[1].error == SaveError::DuplicateSector);
    }
    {
        auto save = twoSlots();                                  // out-of-range sector id
        put16(save, sectorAt(1, 6) + 0xFF4, 14);
        assert(active(save, slots) == 0 && slots[1].error == SaveError::InvalidSectorId);
    }
    {
        auto save = twoSlots();                                  // counters disagree inside a slot
        put32(save, sectorAt(1, 9) + 0xFFC, 99);
        assert(active(save, slots) == 0 && slots[1].error == SaveError::MismatchedSectorCounters);
    }
    // Family classification still follows the active slot's block 0 (FRLG marker = 1).
    {
        auto save = twoSlots();
        assert(active(save, slots) == 1);
        assert(Detail::detectFamily(save, slots[1]) == Detail::SaveFamily::FireRedLeafGreen);
    }
    std::cout << "Gen III sector signature policy (game semantics, torn-save fallback): PASS\n";
}
