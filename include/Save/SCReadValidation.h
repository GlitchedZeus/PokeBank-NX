#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "Encryption/Encryption8SWSH.h"
#include "Encryption/Encryption9LZA.h"
#include "Encryption/Encryption9SV.h"
#include "Enums/GameVersion.h"
#include "Save/Block.h"
#include "Trainer/Inventory9LZA.h"
#include "Trainer/Inventory9SV.h"
#include "Trainer/Trainer8SWSH.h"
#include "Trainer/Trainer9LZA.h"
#include "Trainer/Trainer9SV.h"
#include "Utils/HelperUtilities.h"

namespace Save {
namespace SCReadValidation {

enum class PokemonFamily : uint8_t { SWSH, SV, ZA };

inline const Block* findRequired(const std::vector<Block>& blocks,
                                 uint32_t key,
                                 Enums::SCTypeCode type,
                                 std::size_t minimumSize,
                                 std::string_view& error) noexcept {
    for (const auto& block : blocks) {
        if (block.key != key) continue;
        if (block.type != type) {
            error = "A required game block uses an unsupported SC type.";
            return nullptr;
        }
        if (block.data.size() < minimumSize) {
            error = "A required game block is truncated for the supported layout.";
            return nullptr;
        }
        return &block;
    }
    error = "A required game block is missing.";
    return nullptr;
}

inline std::string_view validatePokemonRecords(const Block& block,
                                               PokemonFamily family,
                                               std::size_t entitySize,
                                               std::size_t stride,
                                               std::size_t slotCount,
                                               std::size_t storedSize) {
    if (slotCount == 0) return {};
    const std::size_t required = (slotCount - 1) * stride + entitySize;
    if (block.data.size() < required)
        return "A required Pokemon storage block is truncated.";

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
        if (!decoded)
            return "A Pokemon record could not be decoded.";

        const auto* pk = reinterpret_cast<const uint8_t*>(decoded.get());
        uint16_t checksum = 0;
        for (std::size_t i = 8; i < storedSize; i += 2)
            checksum = static_cast<uint16_t>(
                checksum + Utils::readUInt16LittleEndian(pk + i));
        if (checksum != Utils::readUInt16LittleEndian(pk + 6))
            return "A Pokemon record has an invalid checksum. No repair attempted.";

        const uint16_t species = Utils::readUInt16LittleEndian(pk + 8);
        if (species > 1025 || pk[0x20] >= 25 || pk[0x21] >= 25)
            return "A Pokemon record has unsupported species or nature data.";
    }
    return {};
}

inline std::string_view validateUniqueKeys(const std::vector<Block>& blocks) {
    std::unordered_set<uint32_t> seen;
    seen.reserve(blocks.size());
    for (const auto& block : blocks) {
        if (!seen.insert(block.key).second)
            return "Duplicate save block keys.";
    }
    return {};
}

inline std::string_view validateSWSH(const std::vector<Block>& blocks) {
    if (const auto duplicate = validateUniqueKeys(blocks); !duplicate.empty())
        return duplicate;

    std::string_view error;
    constexpr std::size_t partySize =
        6 * Encryption::SIZE_PARTY8_SWSH;
    constexpr std::size_t boxSize =
        Trainer::BOX_COUNT8_SWSH * 30 * Encryption::SIZE_PARTY8_SWSH;
    constexpr std::size_t boxLayoutSize =
        Trainer::BOX_COUNT8_SWSH * Trainer::BOX_NAME_LENGTH8_SWSH;
    // Final SWSH pouch is Key Items: offset 4600, 64 entries, four bytes each.
    constexpr std::size_t itemSize = 4600 + 64 * 4;

    const Block* myStatus = findRequired(
        blocks, Trainer::MY_STATUS8_SWSH, Enums::SCTypeCode::Object, 0xB0 + 0x1A, error);
    if (!myStatus) return error;
    const Block* party = findRequired(
        blocks, Trainer::PARTY8_SWSH, Enums::SCTypeCode::Object, partySize, error);
    if (!party) return error;
    if (!findRequired(blocks, Trainer::MONEY8_SWSH, Enums::SCTypeCode::Object, 8, error))
        return error;
    if (!findRequired(blocks, Trainer::ITEM8_SWSH, Enums::SCTypeCode::Object, itemSize, error))
        return error;
    const Block* box = findRequired(
        blocks, Trainer::BOX8_SWSH, Enums::SCTypeCode::Object, boxSize, error);
    if (!box) return error;
    if (!findRequired(blocks, Trainer::BOX_LAYOUT8_SWSH, Enums::SCTypeCode::Object,
                      boxLayoutSize, error))
        return error;
    if (!findRequired(blocks, Trainer::CURRENT_BOX8_SWSH, Enums::SCTypeCode::UInt32, 4, error))
        return error;

    if (const auto e = validatePokemonRecords(
            *party, PokemonFamily::SWSH,
            Encryption::SIZE_PARTY8_SWSH, Encryption::SIZE_PARTY8_SWSH, 6,
            Encryption::SIZE_STORED8_SWSH); !e.empty())
        return e;
    if (const auto e = validatePokemonRecords(
            *box, PokemonFamily::SWSH,
            Encryption::SIZE_PARTY8_SWSH, Encryption::SIZE_PARTY8_SWSH,
            Trainer::BOX_COUNT8_SWSH * 30, Encryption::SIZE_STORED8_SWSH); !e.empty())
        return e;
    return {};
}

inline std::string_view validateSV(const std::vector<Block>& blocks) {
    if (const auto duplicate = validateUniqueKeys(blocks); !duplicate.empty())
        return duplicate;

    std::string_view error;
    constexpr std::size_t partySize = 6 * Encryption::SIZE_PARTY9_SV;
    constexpr std::size_t boxSize =
        Trainer::BOX_COUNT9_SV * 30 * Encryption::SIZE_PARTY9_SV;
    constexpr std::size_t boxLayoutSize =
        Trainer::BOX_COUNT9_SV * Trainer::BOX_NAME_LENGTH9_SV;

    const Block* myStatus = findRequired(
        blocks, Trainer::MY_STATUS9_SV, Enums::SCTypeCode::Object, 0x10 + 0x1A, error);
    if (!myStatus) return error;
    const Block* party = findRequired(
        blocks, Trainer::PARTY9_SV, Enums::SCTypeCode::Object, partySize, error);
    if (!party) return error;
    if (!findRequired(blocks, Trainer::MONEY9_SV, Enums::SCTypeCode::UInt32, 4, error))
        return error;
    if (!findRequired(blocks, Trainer::ITEM9_SV, Enums::SCTypeCode::Object,
                      Trainer::ITEM_BLOCK_SIZE9_SV, error))
        return error;
    const Block* box = findRequired(
        blocks, Trainer::BOX9_SV, Enums::SCTypeCode::Object, boxSize, error);
    if (!box) return error;
    if (!findRequired(blocks, Trainer::BOX_LAYOUT9_SV, Enums::SCTypeCode::Object,
                      boxLayoutSize, error))
        return error;
    if (!findRequired(blocks, Trainer::CURRENT_BOX9_SV, Enums::SCTypeCode::UInt32, 4, error))
        return error;

    if (const auto e = validatePokemonRecords(
            *party, PokemonFamily::SV,
            Encryption::SIZE_PARTY9_SV, Encryption::SIZE_PARTY9_SV, 6,
            Encryption::SIZE_STORED9_SV); !e.empty())
        return e;
    if (const auto e = validatePokemonRecords(
            *box, PokemonFamily::SV,
            Encryption::SIZE_PARTY9_SV, Encryption::SIZE_PARTY9_SV,
            Trainer::BOX_COUNT9_SV * 30, Encryption::SIZE_STORED9_SV); !e.empty())
        return e;
    return {};
}

inline std::string_view validateZA(const std::vector<Block>& blocks) {
    if (const auto duplicate = validateUniqueKeys(blocks); !duplicate.empty())
        return duplicate;

    std::string_view error;
    constexpr std::size_t partySize = 6 * Encryption::PARTY_SLOT_SIZE9_LZA;
    constexpr std::size_t boxSize =
        Trainer::BOX_COUNT9_LZA * 30 * Encryption::BOX_SLOT_SIZE9_LZA;
    constexpr std::size_t boxLayoutSize =
        Trainer::BOX_COUNT9_LZA * Trainer::BOX_NAME_LENGTH9_LZA;

    const Block* myStatus = findRequired(
        blocks, Trainer::MY_STATUS9_LZA, Enums::SCTypeCode::Object, 0x10 + 0x1A, error);
    if (!myStatus) return error;
    const Block* party = findRequired(
        blocks, Trainer::PARTY9_LZA, Enums::SCTypeCode::Object, partySize, error);
    if (!party) return error;
    if (!findRequired(blocks, Trainer::MONEY9_LZA, Enums::SCTypeCode::UInt32, 4, error))
        return error;
    if (!findRequired(blocks, Trainer::ITEM9_LZA, Enums::SCTypeCode::Object,
                      Trainer::ITEM_BLOCK_SIZE9_LZA, error))
        return error;
    const Block* box = findRequired(
        blocks, Trainer::BOX9_LZA, Enums::SCTypeCode::Object, boxSize, error);
    if (!box) return error;
    if (!findRequired(blocks, Trainer::BOX_LAYOUT9_LZA, Enums::SCTypeCode::Object,
                      boxLayoutSize, error))
        return error;
    if (!findRequired(blocks, Trainer::CURRENT_BOX9_LZA, Enums::SCTypeCode::UInt32, 4, error))
        return error;
    if (!findRequired(blocks, Trainer::SAVE_REVISION9_LZA, Enums::SCTypeCode::UInt64, 8, error))
        return error;

    if (const auto e = validatePokemonRecords(
            *party, PokemonFamily::ZA,
            Encryption::SIZE_PARTY9_LZA, Encryption::PARTY_SLOT_SIZE9_LZA, 6,
            Encryption::SIZE_STORED9_LZA); !e.empty())
        return e;
    if (const auto e = validatePokemonRecords(
            *box, PokemonFamily::ZA,
            Encryption::SIZE_PARTY9_LZA, Encryption::BOX_SLOT_SIZE9_LZA,
            Trainer::BOX_COUNT9_LZA * 30, Encryption::SIZE_STORED9_LZA); !e.empty())
        return e;
    return {};
}

} // namespace SCReadValidation

inline std::string_view validateSCReadLayout(const std::vector<Block>& blocks,
                                             Enums::GameVersion group) {
    switch (group) {
        case Enums::GameVersion::SWSH:
            return SCReadValidation::validateSWSH(blocks);
        case Enums::GameVersion::SV:
            return SCReadValidation::validateSV(blocks);
        case Enums::GameVersion::ZA:
            return SCReadValidation::validateZA(blocks);
        default:
            return "Unsupported SC game layout.";
    }
}
} // namespace Save
