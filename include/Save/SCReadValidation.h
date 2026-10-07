#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <cstdio>
#include <unordered_set>
#include <vector>

#include "Encryption/Encryption8SWSH.h"
#include "Encryption/Encryption9LZA.h"
#include "Encryption/Encryption9SV.h"
#include "Enums/GameVersion.h"
#include "Save/Block.h"
#include "Utils/HelperUtilities.h"

namespace Save {
namespace SCReadValidation {

enum class PokemonFamily : uint8_t { SWSH, SV, ZA };

// Exact SC block identities consumed by the supported trainer models. Keeping these tiny layout
// facts here avoids coupling the preflight validator to the full Trainer class headers.
inline constexpr uint32_t SWSH_MY_STATUS = 0xF25C070E;
inline constexpr uint32_t SWSH_PARTY = 0x2985FE5D;
inline constexpr uint32_t SWSH_MONEY = 0x1B882B09;
inline constexpr uint32_t SWSH_ITEMS = 0x1177C2C4;
inline constexpr uint32_t SWSH_BOX = 0x0D66012C;
inline constexpr uint32_t SWSH_BOX_LAYOUT = 0x19722C89;
inline constexpr uint32_t SWSH_CURRENT_BOX = 0x017C3CBB;

inline constexpr uint32_t GEN9_MY_STATUS = 0xE3E89BD1;
inline constexpr uint32_t GEN9_PARTY = 0x3AA1A9AD;
inline constexpr uint32_t GEN9_MONEY = 0x4F35D0DD;
inline constexpr uint32_t GEN9_ITEMS = 0x21C9BD44;
inline constexpr uint32_t GEN9_BOX = 0x0D66012C;
inline constexpr uint32_t GEN9_BOX_LAYOUT = 0x19722C89;
inline constexpr uint32_t GEN9_CURRENT_BOX = 0x017C3CBB;
inline constexpr uint32_t ZA_SAVE_REVISION = 0x0926555A;

inline constexpr std::size_t BOX_COUNT = 32;
inline constexpr std::size_t BOX_SLOTS = 30;
inline constexpr std::size_t BOX_NAME_BYTES = 0x22;
inline constexpr std::size_t GEN9_ITEM_BLOCK_BYTES = 0xBB80;
inline constexpr std::size_t SWSH_ITEM_BLOCK_BYTES = 4600 + 64 * 4;

inline const Block* findRequired(const std::vector<Block>& blocks,
                                 uint32_t key,
                                 Enums::SCTypeCode type,
                                 std::size_t minimumSize,
                                 std::string_view& error,
                                 std::string* diagnostic = nullptr) noexcept {
    for (const auto& block : blocks) {
        if (block.key != key) continue;
        if (block.type != type) {
            error = "A required game block uses an unsupported SC type.";
            if (diagnostic) {
                char buffer[192];
                std::snprintf(buffer, sizeof(buffer),
                    "required block key=0x%08X wrong type: expected=%u actual=%u bytes=%zu",
                    static_cast<unsigned>(key), static_cast<unsigned>(type),
                    static_cast<unsigned>(block.type), block.data.size());
                *diagnostic = buffer;
            }
            return nullptr;
        }
        if (block.data.size() < minimumSize) {
            error = "A required game block is truncated for the supported layout.";
            if (diagnostic) {
                char buffer[192];
                std::snprintf(buffer, sizeof(buffer),
                    "required block key=0x%08X truncated: minimum=%zu actual=%zu type=%u",
                    static_cast<unsigned>(key), minimumSize, block.data.size(),
                    static_cast<unsigned>(block.type));
                *diagnostic = buffer;
            }
            return nullptr;
        }
        return &block;
    }
    error = "A required game block is missing.";
    if (diagnostic) {
        char buffer[128];
        std::snprintf(buffer, sizeof(buffer), "required block key=0x%08X missing",
                      static_cast<unsigned>(key));
        *diagnostic = buffer;
    }
    return nullptr;
}

inline const Block* findRequiredPayload(const std::vector<Block>& blocks,
                                        uint32_t key,
                                        std::size_t minimumSize,
                                        std::string_view& error,
                                        std::string* diagnostic = nullptr) noexcept {
    for (const auto& block : blocks) {
        if (block.key != key) continue;
        // The supported Trainer readers consume these blocks by key and raw payload bytes.
        // Their SC wrapper tag is not part of the parsed field layout, so preflight checks the
        // source-backed payload geometry here. True scalar/value blocks still use findRequired().
        if (block.data.size() < minimumSize) {
            error = "A required game block is truncated for the supported layout.";
            if (diagnostic) {
                char buffer[192];
                std::snprintf(buffer, sizeof(buffer),
                    "required payload key=0x%08X truncated: minimum=%zu actual=%zu wrapperType=%u",
                    static_cast<unsigned>(key), minimumSize, block.data.size(),
                    static_cast<unsigned>(block.type));
                *diagnostic = buffer;
            }
            return nullptr;
        }
        return &block;
    }
    error = "A required game block is missing.";
    if (diagnostic) {
        char buffer[128];
        std::snprintf(buffer, sizeof(buffer), "required payload key=0x%08X missing",
                      static_cast<unsigned>(key));
        *diagnostic = buffer;
    }
    return nullptr;
}

inline std::string_view validatePokemonRecords(const Block& block,
                                               PokemonFamily family,
                                               std::size_t entitySize,
                                               std::size_t stride,
                                               std::size_t slotCount,
                                               std::size_t storedSize,
                                               std::string* diagnostic = nullptr) {
    if (slotCount == 0) return {};
    const std::size_t required = (slotCount - 1) * stride + entitySize;
    if (block.data.size() < required) {
        if (diagnostic) {
            char buffer[192];
            std::snprintf(buffer, sizeof(buffer),
                "Pokemon block key=0x%08X truncated: minimum=%zu actual=%zu",
                static_cast<unsigned>(block.key), required, block.data.size());
            *diagnostic = buffer;
        }
        return "A required Pokemon storage block is truncated.";
    }

    for (std::size_t slot = 0; slot < slotCount; ++slot) {
        const std::size_t offset = slot * stride;
        const uint8_t* raw = block.data.data() + offset;
        if (std::all_of(raw, raw + entitySize, [](uint8_t b) { return b == 0; }))
            continue;

        std::unique_ptr<std::byte[]> decoded;
        const auto encrypted = std::span<const std::byte>(
            reinterpret_cast<const std::byte*>(raw), entitySize);
        switch (family) {
            case PokemonFamily::SWSH:
                decoded.reset(Encryption::decryptArray8SWSH(encrypted));
                break;
            case PokemonFamily::SV:
                decoded.reset(Encryption::decryptArray9SV(encrypted));
                break;
            case PokemonFamily::ZA:
                decoded.reset(Encryption::decryptArray9LZA(encrypted));
                break;
        }
        if (!decoded) {
            if (diagnostic) {
                char buffer[160];
                std::snprintf(buffer, sizeof(buffer),
                    "Pokemon block key=0x%08X slot=%zu decode failed",
                    static_cast<unsigned>(block.key), slot);
                *diagnostic = buffer;
            }
            return "A Pokemon record could not be decoded.";
        }

        const auto* pk = reinterpret_cast<const uint8_t*>(decoded.get());
        const uint16_t species = Utils::readUInt16LittleEndian(pk + 8);

        // Native modern saves normally represent an empty slot as a non-zero encrypted blank.
        // Occupancy is therefore a property of the DECRYPTED entity (species 0), not of the raw
        // ciphertext. Empty slots are not Pokemon records and must not be rejected for stale or
        // undefined checksum/nature bytes. Occupied records remain fully fail-closed below.
        if (species == 0)
            continue;

        uint16_t checksum = 0;
        for (std::size_t i = 8; i < storedSize; i += 2)
            checksum = static_cast<uint16_t>(
                checksum + Utils::readUInt16LittleEndian(pk + i));
        if (checksum != Utils::readUInt16LittleEndian(pk + 6)) {
            if (diagnostic) {
                char buffer[192];
                std::snprintf(buffer, sizeof(buffer),
                    "Pokemon block key=0x%08X slot=%zu invalid checksum speciesRaw=%u",
                    static_cast<unsigned>(block.key), slot, static_cast<unsigned>(species));
                *diagnostic = buffer;
            }
            return "A Pokemon record has an invalid checksum. No repair attempted.";
        }

        if (species > 1025 || pk[0x20] >= 25 || pk[0x21] >= 25) {
            if (diagnostic) {
                char buffer[224];
                std::snprintf(buffer, sizeof(buffer),
                    "Pokemon block key=0x%08X slot=%zu unsupported domain: speciesRaw=%u nature=%u statNature=%u",
                    static_cast<unsigned>(block.key), slot, static_cast<unsigned>(species),
                    static_cast<unsigned>(pk[0x20]), static_cast<unsigned>(pk[0x21]));
                *diagnostic = buffer;
            }
            return "A Pokemon record has unsupported species or nature data.";
        }
    }
    return {};
}

inline std::string_view validateUniqueKeys(const std::vector<Block>& blocks,
                                           std::string* diagnostic = nullptr) {
    std::unordered_set<uint32_t> seen;
    seen.reserve(blocks.size());
    for (const auto& block : blocks) {
        if (!seen.insert(block.key).second) {
            if (diagnostic) {
                char buffer[128];
                std::snprintf(buffer, sizeof(buffer), "duplicate SC block key=0x%08X",
                              static_cast<unsigned>(block.key));
                *diagnostic = buffer;
            }
            return "Duplicate save block keys.";
        }
    }
    return {};
}

inline std::string_view validateSWSH(const std::vector<Block>& blocks,
                                     std::string* diagnostic = nullptr) {
    if (const auto duplicate = validateUniqueKeys(blocks, diagnostic); !duplicate.empty())
        return duplicate;

    std::string_view error;
    constexpr std::size_t partySize =
        6 * Encryption::SIZE_PARTY8_SWSH;
    constexpr std::size_t boxSize =
        BOX_COUNT * BOX_SLOTS * Encryption::SIZE_PARTY8_SWSH;
    constexpr std::size_t boxLayoutSize =
        BOX_COUNT * BOX_NAME_BYTES;
    // Final SWSH pouch is Key Items: offset 4600, 64 entries, four bytes each.
    constexpr std::size_t itemSize = 4600 + 64 * 4;

    if (!findRequiredPayload(blocks, SWSH_MY_STATUS, 0xB0 + 0x1A, error, diagnostic))
        return error;
    const Block* party = findRequiredPayload(blocks, SWSH_PARTY, partySize, error, diagnostic);
    if (!party) return error;
    if (!findRequiredPayload(blocks, SWSH_MONEY, 8, error, diagnostic))
        return error;
    if (!findRequiredPayload(blocks, SWSH_ITEMS, itemSize, error, diagnostic))
        return error;
    const Block* box = findRequiredPayload(blocks, SWSH_BOX, boxSize, error, diagnostic);
    if (!box) return error;
    if (!findRequiredPayload(blocks, SWSH_BOX_LAYOUT, boxLayoutSize, error, diagnostic))
        return error;
    if (!findRequired(blocks, SWSH_CURRENT_BOX, Enums::SCTypeCode::Byte, 1, error, diagnostic))
        return error;

    if (const auto e = validatePokemonRecords(
            *party, PokemonFamily::SWSH,
            Encryption::SIZE_PARTY8_SWSH, Encryption::SIZE_PARTY8_SWSH, 6,
            Encryption::SIZE_STORED8_SWSH, diagnostic); !e.empty())
        return e;
    if (const auto e = validatePokemonRecords(
            *box, PokemonFamily::SWSH,
            Encryption::SIZE_PARTY8_SWSH, Encryption::SIZE_PARTY8_SWSH,
            BOX_COUNT * 30, Encryption::SIZE_STORED8_SWSH, diagnostic); !e.empty())
        return e;
    return {};
}

inline std::string_view validateSV(const std::vector<Block>& blocks,
                                   std::string* diagnostic = nullptr) {
    if (const auto duplicate = validateUniqueKeys(blocks, diagnostic); !duplicate.empty())
        return duplicate;

    std::string_view error;
    constexpr std::size_t partySize = 6 * Encryption::SIZE_PARTY9_SV;
    constexpr std::size_t boxSize =
        BOX_COUNT * BOX_SLOTS * Encryption::SIZE_PARTY9_SV;
    constexpr std::size_t boxLayoutSize =
        BOX_COUNT * BOX_NAME_BYTES;

    if (!findRequiredPayload(blocks, GEN9_MY_STATUS, 0x10 + 0x1A, error, diagnostic))
        return error;
    const Block* party = findRequiredPayload(blocks, GEN9_PARTY, partySize, error, diagnostic);
    if (!party) return error;
    if (!findRequired(blocks, GEN9_MONEY, Enums::SCTypeCode::UInt32, 4, error, diagnostic))
        return error;
    if (!findRequiredPayload(blocks, GEN9_ITEMS, GEN9_ITEM_BLOCK_BYTES, error, diagnostic))
        return error;
    const Block* box = findRequiredPayload(blocks, GEN9_BOX, boxSize, error, diagnostic);
    if (!box) return error;
    if (!findRequiredPayload(blocks, GEN9_BOX_LAYOUT, boxLayoutSize, error, diagnostic))
        return error;
    if (!findRequired(blocks, GEN9_CURRENT_BOX, Enums::SCTypeCode::Byte, 1, error, diagnostic))
        return error;

    if (const auto e = validatePokemonRecords(
            *party, PokemonFamily::SV,
            Encryption::SIZE_PARTY9_SV, Encryption::SIZE_PARTY9_SV, 6,
            Encryption::SIZE_STORED9_SV, diagnostic); !e.empty())
        return e;
    if (const auto e = validatePokemonRecords(
            *box, PokemonFamily::SV,
            Encryption::SIZE_PARTY9_SV, Encryption::SIZE_PARTY9_SV,
            BOX_COUNT * 30, Encryption::SIZE_STORED9_SV, diagnostic); !e.empty())
        return e;
    return {};
}

inline std::string_view validateZA(const std::vector<Block>& blocks,
                                   std::string* diagnostic = nullptr) {
    if (const auto duplicate = validateUniqueKeys(blocks, diagnostic); !duplicate.empty())
        return duplicate;

    std::string_view error;
    constexpr std::size_t partySize = 6 * Encryption::PARTY_SLOT_SIZE9_LZA;
    constexpr std::size_t boxSize =
        BOX_COUNT * BOX_SLOTS * Encryption::BOX_SLOT_SIZE9_LZA;
    constexpr std::size_t boxLayoutSize =
        BOX_COUNT * BOX_NAME_BYTES;

    if (!findRequiredPayload(blocks, GEN9_MY_STATUS, 0x10 + 0x1A, error, diagnostic))
        return error;
    const Block* party = findRequiredPayload(blocks, GEN9_PARTY, partySize, error, diagnostic);
    if (!party) return error;
    if (!findRequired(blocks, GEN9_MONEY, Enums::SCTypeCode::UInt32, 4, error, diagnostic))
        return error;
    if (!findRequiredPayload(blocks, GEN9_ITEMS, GEN9_ITEM_BLOCK_BYTES, error, diagnostic))
        return error;
    const Block* box = findRequiredPayload(blocks, GEN9_BOX, boxSize, error, diagnostic);
    if (!box) return error;
    if (!findRequiredPayload(blocks, GEN9_BOX_LAYOUT, boxLayoutSize, error, diagnostic))
        return error;
    if (!findRequired(blocks, GEN9_CURRENT_BOX, Enums::SCTypeCode::Byte, 1, error, diagnostic))
        return error;
    if (!findRequired(blocks, ZA_SAVE_REVISION, Enums::SCTypeCode::UInt64, 8, error, diagnostic))
        return error;

    if (const auto e = validatePokemonRecords(
            *party, PokemonFamily::ZA,
            Encryption::SIZE_PARTY9_LZA, Encryption::PARTY_SLOT_SIZE9_LZA, 6,
            Encryption::SIZE_STORED9_LZA, diagnostic); !e.empty())
        return e;
    if (const auto e = validatePokemonRecords(
            *box, PokemonFamily::ZA,
            Encryption::SIZE_PARTY9_LZA, Encryption::BOX_SLOT_SIZE9_LZA,
            BOX_COUNT * 30, Encryption::SIZE_STORED9_LZA, diagnostic); !e.empty())
        return e;
    return {};
}

} // namespace SCReadValidation

inline std::string_view validateSCReadLayout(const std::vector<Block>& blocks,
                                             Enums::GameVersion group,
                                             std::string* diagnostic = nullptr) {
    if (diagnostic) diagnostic->clear();
    switch (group) {
        case Enums::GameVersion::SWSH:
            return SCReadValidation::validateSWSH(blocks, diagnostic);
        case Enums::GameVersion::SV:
            return SCReadValidation::validateSV(blocks, diagnostic);
        case Enums::GameVersion::ZA:
            return SCReadValidation::validateZA(blocks, diagnostic);
        default:
            if (diagnostic) *diagnostic = "unsupported SC game routing";
            return "Unsupported SC game layout.";
    }
}
} // namespace Save
