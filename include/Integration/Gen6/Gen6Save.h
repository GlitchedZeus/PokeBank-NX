#ifndef POKEBANK_GEN6_SAVE_H
#define POKEBANK_GEN6_SAVE_H

#include "Integration/Gen6/Gen6PK6.h"
#include "Integration/Gen6/Gen6SaveLayout.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace PokeVault::Integration::Gen6 {

enum class SlotState : uint8_t {
    Occupied, // strict PK6 decode, species != 0
    Empty,    // strict PK6 decode, species == 0
    Invalid,  // fails PK6 sanity/checksum (quarantined, never written over)
};

struct SlotRecord {
    SlotState state = SlotState::Invalid;
    PK6::DecodeStatus status = PK6::DecodeStatus::UnknownSize;
    std::optional<PK6::Pokemon6> pokemon;
    std::span<const uint8_t> encrypted;  // view into the parsed save's own immutable copy

    [[nodiscard]] bool occupied() const noexcept { return state == SlotState::Occupied; }
    // Empty AND decrypts to an all-zero record (the blank PKHeX writes).
    [[nodiscard]] bool blank() const noexcept {
        return state == SlotState::Empty && pokemon && pokemon->allZero();
    }
};

struct TrainerReadOnly {
    std::u16string rawName;  // raw UTF-16 units; no glyph remapping is invented here
    uint16_t tid = 0, sid = 0;
    uint8_t version = 0, gender = 0, language = 0;
    uint16_t playedHours = 0;
    uint8_t playedMinutes = 0, playedSeconds = 0;
};

struct ReadDiagnostics {
    size_t occupiedBoxRecords = 0;
    size_t emptyBoxRecords = 0;
    size_t invalidBoxRecords = 0;
    size_t invalidUndeclaredPartyRecords = 0;
    // Stored u32 length / u16 id in the BEEF metadata that differ from the cited
    // table. Reported only: the pinned references validate checksums, not these.
    size_t metadataMismatches = 0;
};

// Strict, read-only SAV6 model. parse() copies the input; nothing here can
// write to a source file or provider. The only serialization path is
// Gen6StagedSession, which reuses the static helpers below.
class Gen6Save {
public:
    [[nodiscard]] static std::optional<Gen6Save> parse(std::span<const uint8_t> bytes,
                                                       std::string_view assignedExactGame = {},
                                                       std::string* error = nullptr);

    [[nodiscard]] Layout::Family family() const noexcept { return family_; }
    [[nodiscard]] std::string_view exactGameId() const noexcept { return gameId_; }
    [[nodiscard]] uint8_t version() const noexcept { return version_; }
    [[nodiscard]] std::span<const uint8_t> bytes() const noexcept { return bytes_; }
    [[nodiscard]] uint8_t partyCount() const noexcept { return partyCount_; }
    [[nodiscard]] const ReadDiagnostics& diagnostics() const noexcept { return diagnostics_; }
    [[nodiscard]] TrainerReadOnly trainer() const;

    [[nodiscard]] SlotRecord party(size_t slot) const;
    [[nodiscard]] SlotRecord box(size_t box, size_t slot) const;

    // ---- Geometry helpers shared with the staged serializer ----
    [[nodiscard]] static size_t partyRecordOffset(size_t slot) noexcept {
        return Layout::PartyOffset + slot * PK6::PartySize;
    }
    [[nodiscard]] static size_t partyCountOffset() noexcept {
        return Layout::PartyOffset + Layout::PartySlots * PK6::PartySize;
    }
    [[nodiscard]] static size_t boxRecordOffset(Layout::Family f, size_t box, size_t slot) noexcept {
        return Layout::boxOffset(f) + (box * Layout::BoxSlots + slot) * PK6::StoredSize;
    }
    // The single block that fully contains [offset, offset+length), if any.
    [[nodiscard]] static std::optional<size_t> owningBlock(Layout::Family f, size_t offset,
                                                           size_t length) noexcept;
    [[nodiscard]] static uint16_t computeBlockChecksum(std::span<const uint8_t> save,
                                                       Layout::Family f, size_t blockId) noexcept;
    [[nodiscard]] static uint16_t storedBlockChecksum(std::span<const uint8_t> save,
                                                      Layout::Family f, size_t blockId) noexcept;
    static void refreshBlockChecksum(std::span<uint8_t> save, Layout::Family f,
                                     size_t blockId) noexcept;
    // Number of blocks whose stored checksum is valid (diagnostic + strict gate).
    [[nodiscard]] static size_t validBlockCount(std::span<const uint8_t> save,
                                                Layout::Family f) noexcept;

private:
    Gen6Save() = default;
    [[nodiscard]] SlotRecord record(size_t offset, size_t size) const;

    std::vector<uint8_t> bytes_;
    std::string gameId_;
    Layout::Family family_ = Layout::Family::XY;
    uint8_t version_ = 0;
    uint8_t partyCount_ = 0;
    ReadDiagnostics diagnostics_{};
};

} // namespace PokeVault::Integration::Gen6

#endif
