#pragma once

#include <algorithm>
#include <array>
#include <memory>
#include <span>
#include <string_view>
#include <unordered_set>
#include "Save/Block.h"
#include "Encryption/Encryption8LA.h"
#include "Utils/HelperUtilities.h"

namespace Save {
// Acceptance gate for the layout actually consumed by Trainer8LA. This is structural
// validation, not legality or a claim to support every historical revision.
inline std::string_view validatePLAReadLayout(const std::vector<Block>& blocks) {
    constexpr std::array<uint32_t, 3> keys{0xf25c070e, 0x2985fe5d, 0x47e1ceab};
    constexpr std::array<size_t, 3> sizes{0x3a, 6 * 0x178, 32 * 30 * 0x168};
    std::array<bool, 3> found{};
    std::unordered_set<uint32_t> seen;
    for (const auto& block : blocks) {
        if (!seen.insert(block.key).second) return "Duplicate save block keys.";
        for (size_t k = 0; k < keys.size(); ++k) {
            if (block.key != keys[k]) continue;
            found[k] = true;
            if (block.type != Enums::SCTypeCode::Object || block.data.size() < sizes[k])
                return "Required PLA block is truncated or uses an unsupported layout.";
            if (k == 0) continue;
            const size_t stride = k == 1 ? 0x178 : 0x168;
            for (size_t offset = 0; offset < sizes[k]; offset += stride) {
                const auto* raw = block.data.data() + offset;
                if (std::all_of(raw, raw + stride, [](uint8_t b) { return b == 0; })) continue;
                std::unique_ptr<std::byte[]> decoded(Encryption::decryptArray8LA(
                    std::span<const std::byte>(reinterpret_cast<const std::byte*>(raw), stride)));
                const auto* pk = reinterpret_cast<const uint8_t*>(decoded.get());
                uint16_t sum = 0;
                for (size_t i = 8; i < 0x168; i += 2) sum += Utils::readUInt16LittleEndian(pk + i);
                if (sum != Utils::readUInt16LittleEndian(pk + 6))
                    return "A PLA Pokemon record has an invalid checksum. No repair attempted.";
                if (Utils::readUInt16LittleEndian(pk + 8) > 905 || pk[0x20] >= 25 || pk[0x21] >= 25)
                    return "A PLA Pokemon record has unsupported species or nature data.";
            }
        }
    }
    if (!std::all_of(found.begin(), found.end(), [](bool b) { return b; }))
        return "Required PLA trainer, party or pasture block is missing.";
    return {};
}
}
