#ifndef POKEBANK_GEN7_SAVE_H
#define POKEBANK_GEN7_SAVE_H

#include "Integration/Gen6/Gen6PK6.h"
#include "Integration/Gen7/Gen7SaveLayout.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace PokeVault::Integration::Gen7 {

// PK7 uses the identical 232/260-byte Decrypt67/Encrypt67 codec and checksum
// (PKHeX 6501f0ab PokeCrypto.cs, G6PKM.cs; PK7 derives from G6PKM). Every
// accessor Pokemon6 exposes sits at the same offset in PKHeX PK7.cs; the
// PK6/PK7 differences (markings u16 at 0x16, form-argument packing, hyper
// training 0xDE, dirt 0xED/0xEE) are not read or written here.
namespace PK7 = Gen6::PK6;

enum class SlotState : uint8_t { Occupied, Empty, Invalid };

struct SlotRecord {
    SlotState state = SlotState::Invalid;
    PK7::DecodeStatus status = PK7::DecodeStatus::UnknownSize;
    std::optional<PK7::Pokemon6> pokemon;
    [[nodiscard]] bool occupied() const noexcept { return state == SlotState::Occupied; }
};

struct TrainerReadOnly {
    std::u16string rawName;
    uint16_t tid = 0, sid = 0;
    uint8_t version = 0, gender = 0, language = 0;
    uint16_t playedHours = 0;
    uint8_t playedMinutes = 0, playedSeconds = 0;
};

// What the MemeCrypto payload looked like after verification. The pinned
// references re-sign differently (PKHeX: hash + zero tail; PKSM-Core: hash +
// RSA-reversed cleared region), so the observed tail is reported as evidence.
enum class SignatureTail : uint8_t { Zero, NonZero };

struct ReadDiagnostics {
    size_t occupiedBoxRecords = 0;
    size_t emptyBoxRecords = 0;
    size_t invalidBoxRecords = 0;
    size_t metadataMismatches = 0;
    SignatureTail signatureTail = SignatureTail::Zero;
};

enum class SignaturePolicy : uint8_t {
    Require,  // production: RSA payload must verify and start with SHA256(checksum table)
    Ignore,   // diagnostics only (e.g. explaining why a save failed); never for staging
};

// Strict, read-only SAV7 (SM/USUM). Staged editing/export of SAV7 is
// explicitly unsupported until the re-signing payload is confirmed on a real
// save (see docs/gen7/README.md).
class Gen7Save {
public:
    [[nodiscard]] static std::optional<Gen7Save> parse(std::span<const uint8_t> bytes,
                                                       std::string_view assignedExactGame = {},
                                                       std::string* error = nullptr,
                                                       SignaturePolicy policy = SignaturePolicy::Require);

    [[nodiscard]] Layout::Family family() const noexcept { return family_; }
    [[nodiscard]] std::string_view exactGameId() const noexcept { return gameId_; }
    [[nodiscard]] std::span<const uint8_t> bytes() const noexcept { return bytes_; }
    [[nodiscard]] uint8_t partyCount() const noexcept { return partyCount_; }
    [[nodiscard]] bool signatureVerified() const noexcept { return signatureVerified_; }
    [[nodiscard]] const ReadDiagnostics& diagnostics() const noexcept { return diagnostics_; }
    [[nodiscard]] TrainerReadOnly trainer() const;
    [[nodiscard]] SlotRecord party(size_t slot) const;
    [[nodiscard]] SlotRecord box(size_t box, size_t slot) const;

    [[nodiscard]] static size_t partyRecordOffset(Layout::Family f, size_t slot) noexcept {
        return Layout::blocks(f)[Layout::PartyBlock].offset + slot * Layout::PartyRecordSize;
    }
    [[nodiscard]] static size_t partyCountOffset(Layout::Family f) noexcept {
        return partyRecordOffset(f, Layout::PartySlots);
    }
    [[nodiscard]] static size_t boxRecordOffset(Layout::Family f, size_t box, size_t slot) noexcept {
        return Layout::blocks(f)[Layout::BoxBlock].offset + (box * Layout::BoxSlots + slot) * Layout::BoxRecordSize;
    }
    // CRC16Invert of a block, computed with the MemeCrypto signature region zeroed
    // (PKHeX ClearMemeCrypto runs before ChecksumsValid; PKSM resign clears it first).
    [[nodiscard]] static uint16_t computeBlockChecksum(std::span<const uint8_t> save, Layout::Family f,
                                                       size_t blockId);
    [[nodiscard]] static size_t validBlockCount(std::span<const uint8_t> save, Layout::Family f);
    // SHA256 of the signed checksum table.
    [[nodiscard]] static std::array<uint8_t, 32> checksumTableHash(std::span<const uint8_t> save, Layout::Family f);
    // RSA verify via vendored memecrypto; fills the 0x80-byte decrypted payload.
    [[nodiscard]] static bool verifySignature(std::span<const uint8_t> save, Layout::Family f,
                                              std::array<uint8_t, Layout::SignatureLength>* payload = nullptr);

private:
    Gen7Save() = default;
    [[nodiscard]] SlotRecord record(size_t offset, size_t size) const;

    std::vector<uint8_t> bytes_;
    std::string gameId_;
    Layout::Family family_ = Layout::Family::SunMoon;
    uint8_t partyCount_ = 0;
    bool signatureVerified_ = false;
    ReadDiagnostics diagnostics_{};
};

} // namespace PokeVault::Integration::Gen7

#endif
