#ifndef POKEBANK_GEN7B_SAVE_H
#define POKEBANK_GEN7B_SAVE_H

// Strict, read-only Let's Go Pikachu / Let's Go Eevee `main` model (SAV7b).
//
// Every offset cites the pinned references:
//   vendor/PKSM-Core/source/sav/SavLGPE.cpp (aa22d7a4)  MyStatus @0x1000 (TID 0x1000, SID 0x1002,
//       version 0x1004, gender 0x1005, language 0x1035, OT name 0x1038, 13 UTF-16 units);
//       PokeListHeader @0x5A00 (six u16 party pointers, starter/partner pointer @+0x0C, stored count
//       @+0x0E, 1001 = empty); storage @0x5C00, PB7 party-length records; checksums @0xB861A.
//   vendor/PKSM-Core/source/pkx/PB7.cpp                 PB7 = PK7 encryption (232-byte stored part,
//       260-byte party part), field offsets identical to PK7 for every field this lane writes.
//   include/Save/LGPEReadValidation.h                   the CRC-16/ARC block table already used by
//       the installed-title reader (blocks 0, 2, 4, 5, 8, 9, 10).
//
// Storage is a GAPLESS list of 1000 records: indices < count are Pokemon, indices >= count are the
// game's blank records (encryption constant 0). Party members and the partner are pointers into
// that list. parse() refuses anything else; nothing here can write a file.

#include "Integration/Gen6/Gen6PK6.h"
#include "Save/LGPEReadValidation.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace PokeVault::Integration::Gen7b {

namespace PK6 = Gen6::PK6;

namespace Layout {
inline constexpr size_t ActiveSize = Save::LGPEReadValidation::ACTIVE_SIZE;   // 0xB8800
inline constexpr size_t FullSize = Save::LGPEReadValidation::FULL_FILE_SIZE;  // 0x100000
inline constexpr size_t MyStatus = 0x1000;
inline constexpr size_t Tid = 0x1000, Sid = 0x1002, Version = 0x1004, Gender = 0x1005, Language = 0x1035;
inline constexpr size_t OtName = 0x1038, OtNameUnits = 13;
inline constexpr size_t Header = 0x5A00;        // block 8, 0x12 bytes
inline constexpr size_t HeaderBlockSize = 0x12;
inline constexpr size_t PartyPointers = 6;
inline constexpr size_t StarterPointer = 0x0C, CountField = 0x0E;
inline constexpr size_t Storage = 0x5C00;       // block 9, 1000 x 0x104
inline constexpr size_t Slots = 1000;
inline constexpr size_t RecordSize = 0x104;     // PB7::PARTY_LENGTH
inline constexpr size_t StoredSize = 0xE8;      // PB7::BOX_LENGTH
inline constexpr uint16_t EmptyPointer = 1001;
inline constexpr size_t BoxCount = 40, BoxSlots = 25;
inline constexpr size_t ChecksumBase = Save::LGPEReadValidation::CHECKSUM_BASE;  // 0xB861A
inline constexpr uint32_t HeaderKey = 8, StorageKey = 9;
inline constexpr uint8_t VersionPikachu = 42, VersionEevee = 43;   // GameVersion GP / GE
inline constexpr size_t CpOffset = 0xFE;        // PB7::partyCP (cached, derived from friendship/AVs)
inline constexpr size_t AwakeningOffset = 0x24; // PB7::awakened(stat), six bytes

[[nodiscard]] constexpr std::string_view gameIdForVersion(uint8_t v) noexcept {
    return v == VersionPikachu ? "letsgo_pikachu_switch" : v == VersionEevee ? "letsgo_eevee_switch" : std::string_view{};
}
[[nodiscard]] constexpr size_t checksumOffset(uint32_t key) noexcept { return ChecksumBase + key * 8; }
[[nodiscard]] constexpr size_t recordOffset(size_t index) noexcept { return Storage + index * RecordSize; }
} // namespace Layout

struct PokeListHeader {
    std::array<uint16_t, Layout::PartyPointers> party{};
    uint16_t starter = Layout::EmptyPointer;
    uint16_t count = 0;
    bool operator==(const PokeListHeader&) const = default;

    [[nodiscard]] size_t partyCount() const noexcept {
        size_t n = 0;
        for (const auto p : party) if (p != Layout::EmptyPointer) ++n;
        return n;
    }
    [[nodiscard]] bool inParty(size_t index) const noexcept {
        for (const auto p : party) if (p == index) return true;
        return false;
    }
    [[nodiscard]] bool isPartner(size_t index) const noexcept { return starter == index; }
};

// One storage record as raw (encrypted) bytes, exactly as stored.
using RawRecord = std::array<uint8_t, Layout::RecordSize>;

struct StorageModel {
    std::vector<RawRecord> records;   // Layout::Slots entries
    PokeListHeader header;
    bool operator==(const StorageModel&) const = default;
};

struct TrainerFacts {
    std::u16string name;
    uint16_t tid = 0, sid = 0;
    uint8_t version = 0, gender = 0, language = 0;
};

class Gen7bSave {
public:
    [[nodiscard]] static std::optional<Gen7bSave> parse(std::span<const uint8_t> bytes,
                                                        std::string_view assignedExactGame = {},
                                                        std::string* error = nullptr);
    // Strict structural check of a storage model (gapless list, valid PB7 records, pointer integrity).
    [[nodiscard]] static bool validateModel(const StorageModel& model, std::string* error = nullptr);
    [[nodiscard]] static PokeListHeader readHeader(std::span<const uint8_t> bytes) noexcept;
    [[nodiscard]] static StorageModel readModel(std::span<const uint8_t> bytes);
    [[nodiscard]] static std::optional<PK6::Pokemon6> decode(const RawRecord& raw);

    [[nodiscard]] std::span<const uint8_t> bytes() const noexcept { return bytes_; }
    [[nodiscard]] std::string_view exactGameId() const noexcept { return gameId_; }
    [[nodiscard]] const StorageModel& model() const noexcept { return model_; }
    [[nodiscard]] TrainerFacts trainer() const;

private:
    Gen7bSave() = default;
    std::vector<uint8_t> bytes_;
    std::string gameId_;
    StorageModel model_;
};

} // namespace PokeVault::Integration::Gen7b

#endif
