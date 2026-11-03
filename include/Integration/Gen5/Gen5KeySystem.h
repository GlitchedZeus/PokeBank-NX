#pragma once
// Round 14 owner decision (B2/W2 Regis): "Both versions initially allow Regirock at Underground
// Ruins. After capturing it, Black 2 unlocks the Iron Key for Registeel; White 2 unlocks the
// Iceberg Key for Regice. The opposite key can be received through Unova Link. Verify the game's
// stored key/access flags where supported; otherwise mark access Incomplete."
//
// The Black 2 / White 2 save stores the Key System in its own checksummed block (PKHeX
// SaveBlockAccessor5B2W2 block 69, "Key-related data", 0x25800 / 0x60 in each save copy; PKHeX
// Saves/Substructures/Gen5/KeySystem5.cs @ 6042efa2). A key's "unlocked" word holds the key's magic
// number XOR the block's crypto word (offset 0x5C): Iron 0x48 = 0xAB031, Iceberg 0x4C = 0xB3818.
// Read-only: nothing here writes the save.
//
// The evidence is used ONLY for a Pokémon caught in this save: the save is Black 2 / White 2, the
// Pokémon's origin game is the save's own game, and its OT TID / SID are the save trainer's.
// Anything else (traded in, another save, unknown) stays Incomplete -- never inferred from the
// Pokémon record alone.
#include "Integration/Encounter/GiftStaticEncounters.h"
#include "Integration/Gen5/Gen5ReadOnlySave.h"

#include <cstddef>
#include <cstdint>
#include <span>

namespace PokeVault::Integration::Gen5::KeySystem {

inline constexpr std::size_t kBlockOffset = 0x25800;   // B2W2 block 69, relative to the save copy
inline constexpr std::size_t kBlockLength = 0x60;
inline constexpr std::size_t kUnlockedIron = 0x48;
inline constexpr std::size_t kUnlockedIceberg = 0x4C;
inline constexpr std::size_t kCrypto = 0x5C;
inline constexpr uint32_t kMagicUnlockedIron = 0xAB031;
inline constexpr uint32_t kMagicUnlockedIceberg = 0xB3818;

struct Chambers {
    bool ironUnlocked = false;
    bool icebergUnlocked = false;
};

constexpr uint32_t read32(std::span<const uint8_t> block, std::size_t offset) noexcept {
    return static_cast<uint32_t>(block[offset]) | (static_cast<uint32_t>(block[offset + 1]) << 8) |
           (static_cast<uint32_t>(block[offset + 2]) << 16) | (static_cast<uint32_t>(block[offset + 3]) << 24);
}

// Decode one Key System block (kBlockLength bytes). A zero word is "not unlocked".
constexpr Chambers decodeBlock(std::span<const uint8_t> block) noexcept {
    Chambers out;
    if (block.size() < kBlockLength) return out;
    const uint32_t crypto = read32(block, kCrypto);
    const uint32_t iron = read32(block, kUnlockedIron), iceberg = read32(block, kUnlockedIceberg);
    out.ironUnlocked = iron != 0 && iron == (kMagicUnlockedIron ^ crypto);
    out.icebergUnlocked = iceberg != 0 && iceberg == (kMagicUnlockedIceberg ^ crypto);
    return out;
}

// The access evidence for a Pokémon in `save` (origin game, OT TID / SID from its record).
inline GiftStaticEncounters::AccessEvidence evidenceFor(const Gen5ReadOnlySave& save, uint8_t origin,
                                                        uint16_t tid, uint16_t sid) {
    GiftStaticEncounters::AccessEvidence evidence;
    if (save.family() != SaveFamily::Black2White2) return evidence;
    const auto trainer = save.trainer();
    if (origin != trainer.gameVersion || (origin != 22 && origin != 23) || tid != trainer.tid || sid != trainer.sid)
        return evidence;
    const auto bytes = save.sourceBytes();
    const std::size_t at = save.selectedCopyOffset() + kBlockOffset;
    if (at + kBlockLength > bytes.size()) return evidence;
    const auto chambers = decodeBlock(bytes.subspan(at, kBlockLength));
    evidence.b2w2IronChamberUnlocked = chambers.ironUnlocked;
    evidence.b2w2IcebergChamberUnlocked = chambers.icebergUnlocked;
    return evidence;
}

} // namespace PokeVault::Integration::Gen5::KeySystem
