#pragma once
// L4 V13: strict read-only encrypted PK3 -> raw event text evidence.
// The original game-font bytes are not reconstructed from Unicode, and a
// valid native record alone NEVER authenticates a distribution or receipt.
#include "Encryption/Encryption3FRLG.h"
#include "Legality/Gen3EventRawTextEvidence.h"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>

namespace Legality::Gen3EventNativeRawTextEvidence {
using Source = Gen3EventRawTextEvidence::Source;
using Result = Gen3EventRawTextEvidence::Result;

inline Result inspectEncrypted(std::span<const std::byte> encrypted,
                               const Source& source = {}) {
    // Reject truncated/oversized records *before* invoking the legacy
    // allocated decryptor. A bad-egg flag also quarantines native evidence.
    if (encrypted.size() != Encryption::SIZE_STORED3_FRLG &&
        encrypted.size() != Encryption::SIZE_PARTY3_FRLG)
        return {};
    if ((std::to_integer<uint8_t>(encrypted[0x13]) & 1u) != 0u)
        return {};
    std::unique_ptr<std::byte[]> allocated(
        Encryption::decryptArray3FRLG(encrypted));
    if (!allocated)
        return {};
    const std::span<const std::byte> canonical(allocated.get(), encrypted.size());
    const auto stored = static_cast<uint16_t>(
        std::to_integer<uint8_t>(canonical[0x1c]) |
        (static_cast<uint16_t>(std::to_integer<uint8_t>(canonical[0x1d])) << 8));
    if (stored != Encryption::checksum3FRLG(canonical))
        return {};
    // Present decrypted game-font bytes are factual; source provenance,
    // renames, changed OT encodings and egg/trade histories are not.
    return Gen3EventRawTextEvidence::inspectVerifiedDecrypted(canonical, true, source);
}
} // namespace Legality::Gen3EventNativeRawTextEvidence
