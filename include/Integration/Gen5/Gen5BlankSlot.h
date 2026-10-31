#ifndef POKEBANK_GEN5_BLANK_SLOT_H
#define POKEBANK_GEN5_BLANK_SLOT_H

#include "Encryption/Encryption5.h"

#include <algorithm>
#include <cstdint>
#include <span>
#include <vector>

namespace PokeVault::Integration::Gen5 {

// "No Pokemon" in a PK5 box slot. Retail BW/B2W2 boxes store the AT-REST
// (encrypted) form of a blank PK5: PID 0, checksum 0, zero payload, which the
// PK5 cipher turns into nonzero keystream bytes. PKHeX writes exactly that
// when it clears a slot (pinned 6501f0ab SaveFile.ClearBoxes:
// WriteSlotBox(BlankPKM, blank)). Raw zero bytes are also read as empty.
[[nodiscard]] inline const std::vector<uint8_t>& encryptedBlankBoxSlot5() {
    static const std::vector<uint8_t> blank =
        Crypto::encryptCandidate(std::vector<uint8_t>(Crypto::StoredSize, 0));
    return blank;
}

// True only for the two native empty encodings: raw zeros, or bytes that
// decrypt to an all-zero 136-byte PK5 (the encrypted blank).
[[nodiscard]] inline bool nativeBlankSlot5(std::span<const uint8_t> encrypted) {
    const auto zero = [](std::span<const uint8_t> b) {
        return std::all_of(b.begin(), b.end(), [](uint8_t v) { return v == 0; });
    };
    if (encrypted.size() != Crypto::StoredSize && encrypted.size() != Crypto::PartySize) return false;
    if (zero(encrypted)) return true;
    if (encrypted.size() != Crypto::StoredSize) return false;
    const auto plain = Crypto::decrypt(encrypted);
    return plain.size() == Crypto::StoredSize && zero(plain);
}

} // namespace PokeVault::Integration::Gen5
#endif
