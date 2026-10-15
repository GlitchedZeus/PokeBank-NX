#ifndef POKEBANK_GEN8_SAVE_H
#define POKEBANK_GEN8_SAVE_H

#include "Integration/Gen8/Gen8PK8.h"
#include "Integration/Gen8/Gen8SaveLayout.h"
#include "Integration/Gen8/Gen8SwishCrypto.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace PokeVault::Integration::Gen8 {

enum class SlotState : uint8_t {
    Occupied,  // strict PK8 decode, species != 0
    Empty,     // strict PK8 decode, species == 0
    Invalid,   // fails PK8 sanity/checksum (quarantined, never written over)
};

struct SlotRecord {
    SlotState state = SlotState::Invalid;
    PK8::DecodeStatus status = PK8::DecodeStatus::UnknownSize;
    std::optional<PK8::Pokemon8> pokemon;

    [[nodiscard]] bool occupied() const noexcept { return state == SlotState::Occupied; }
    // Empty AND decrypts to an all-zero record (PKHeX's BlankPKM).
    [[nodiscard]] bool blank() const noexcept {
        return state == SlotState::Empty && pokemon && pokemon->allZero();
    }
};

struct TrainerReadOnly {
    std::u16string rawName;  // raw UTF-16 units; no glyph remapping is invented here
    uint16_t tid = 0, sid = 0;
    uint8_t version = 0, gender = 0, language = 0;
};

struct ReadDiagnostics {
    size_t blockCount = 0;
    size_t occupiedBoxRecords = 0;
    size_t emptyBoxRecords = 0;
    size_t invalidBoxRecords = 0;
    size_t invalidUndeclaredPartyRecords = 0;
};

// One battle-team reference into box storage (TeamIndexes8).
struct TeamSlot {
    size_t team = 0;
    size_t box = 0;
    size_t slot = 0;
    bool locked = false;
};

// Strict, read-only Sword/Shield `main` model. parse() copies the input file
// bytes and decrypts them into an owned block list; nothing here can write to a
// source file, mount, or installed save. The only serializer is Gen8StagedSession.
class Gen8Save {
public:
    [[nodiscard]] static std::optional<Gen8Save> parse(std::span<const uint8_t> bytes,
                                                       std::string_view assignedExactGame = {},
                                                       std::string* error = nullptr);

    [[nodiscard]] std::string_view exactGameId() const noexcept { return gameId_; }
    [[nodiscard]] uint8_t version() const noexcept { return version_; }
    [[nodiscard]] std::span<const uint8_t> bytes() const noexcept { return bytes_; }
    [[nodiscard]] const std::vector<Swish::Block>& blocks() const noexcept { return blocks_; }
    [[nodiscard]] uint8_t partyCount() const noexcept { return partyCount_; }
    [[nodiscard]] const ReadDiagnostics& diagnostics() const noexcept { return diagnostics_; }

    [[nodiscard]] TrainerReadOnly trainer() const;
    [[nodiscard]] SlotRecord party(size_t slot) const;
    [[nodiscard]] SlotRecord box(size_t box, size_t slot) const;

    // Battle-team references. Empty optional when the TeamIndexes/TeamLocks blocks are
    // absent or malformed, in which case storage-changing operations must be refused.
    [[nodiscard]] const std::optional<std::vector<TeamSlot>>& teams() const noexcept { return teams_; }

    [[nodiscard]] const Swish::Block* find(uint32_t key) const noexcept;
    [[nodiscard]] std::optional<size_t> indexOf(uint32_t key) const noexcept;

    [[nodiscard]] static size_t partyRecordOffset(size_t slot) noexcept { return slot * Layout::RecordSize; }
    [[nodiscard]] static size_t boxRecordOffset(size_t box, size_t slot) noexcept {
        return (box * Layout::BoxSlots + slot) * Layout::RecordSize;
    }

private:
    Gen8Save() = default;
    [[nodiscard]] static SlotRecord record(std::span<const uint8_t> encrypted);

    std::vector<uint8_t> bytes_;
    std::vector<Swish::Block> blocks_;
    std::string gameId_;
    uint8_t version_ = 0;
    uint8_t partyCount_ = 0;
    size_t myStatus_ = 0, party_ = 0, box_ = 0;
    std::optional<std::vector<TeamSlot>> teams_;
    ReadDiagnostics diagnostics_{};
};

} // namespace PokeVault::Integration::Gen8

#endif
