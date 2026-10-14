#ifndef POKEBANK_GEN6_STAGED_SESSION_H
#define POKEBANK_GEN6_STAGED_SESSION_H

#include "Integration/Common/VerifiedStagedExport.h"
#include "Integration/Gen6/Gen6ExactFormatEditorProvider.h"
#include "Integration/Gen6/Gen6Save.h"

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace PokeVault::Integration::Gen6 {

using RecordRegion = Gen6EditorProvider::RecordRegion;
using Field = Gen6EditorProvider::Field;

struct SlotRef {
    RecordRegion region = RecordRegion::Box;
    size_t box = 0;  // zero for Party
    size_t slot = 0;

    [[nodiscard]] static constexpr SlotRef party(size_t slot) noexcept { return {RecordRegion::Party, 0, slot}; }
    [[nodiscard]] static constexpr SlotRef boxed(size_t box, size_t slot) noexcept {
        return {RecordRegion::Box, box, slot};
    }
    [[nodiscard]] constexpr bool canonical() const noexcept {
        return region == RecordRegion::Party ? (box == 0 && slot < Layout::PartySlots)
                                             : (box < Layout::BoxCount && slot < Layout::BoxSlots);
    }
    bool operator<(const SlotRef& o) const noexcept {
        if (region != o.region) return region < o.region;
        if (box != o.box) return box < o.box;
        return slot < o.slot;
    }
    bool operator==(const SlotRef& o) const noexcept {
        return region == o.region && box == o.box && slot == o.slot;
    }
};

enum class ChangeKind : uint8_t { Edit, Clone, Remove };

// One pending native record replacement, in review/manifest form.
struct StagedChange {
    SlotRef slot;
    ChangeKind kind = ChangeKind::Edit;
    std::optional<SlotRef> cloneSource;
    std::vector<uint8_t> originalPlain;  // decrypted source record (immutable baseline)
    std::vector<uint8_t> stagedPlain;    // decrypted staged record (checksum refreshed on export)
    std::vector<std::string> notes;      // human-readable field changes
};

enum class CreateSupport : uint8_t { Unsupported };
struct CreateCapability {
    CreateSupport support = CreateSupport::Unsupported;
    std::string_view reason;
};

struct Gen6ExportRequest {
    std::string rootDirectory;     // app-owned export root (host fixture or exports/gen6)
    std::string sourcePath;        // evidence only; never opened here
    std::string timestamp;         // YYYYMMDD_HHMMSS
    std::string applicationSha = "unknown";
    Common::ExportFailurePoint failurePoint = Common::ExportFailurePoint::None;
};

// Staged PK6 Edit / Clone / Remove on an app-owned copy of a strictly parsed
// SAV6. The source Gen6Save is held by value and never mutated; the only
// output is a candidate byte image that must pass verifyWorkingCopy() and,
// through VerifiedStagedExport, an app-owned working-copy export directory.
class Gen6StagedSession {
public:
    explicit Gen6StagedSession(Gen6Save source) : source_(std::move(source)) {}

    [[nodiscard]] const Gen6Save& source() const noexcept { return source_; }
    [[nodiscard]] Gen6EditorProvider::ValueLimits limits() const noexcept {
        return Gen6EditorProvider::limitsFor(source_.family());
    }

    // Numeric fields. `index` selects the stat (0..5: HP, Atk, Def, Spe, SpA, SpD,
    // PKHeX PK6 order) for IV/EV and is ignored otherwise.
    [[nodiscard]] bool stageValue(SlotRef slot, Field field, uint32_t value, size_t index = 0,
                                  std::string* error = nullptr);
    // Nickname / OriginalTrainer, 1..12 characters from a conservative ASCII set
    // whose Gen VI glyphs are identity-mapped (no StringConverter6 remapping).
    [[nodiscard]] bool stageText(SlotRef slot, Field field, std::string_view text,
                                 std::string* error = nullptr);
    // Copy one unmodified, occupied source record into a blank box slot.
    [[nodiscard]] bool cloneToBox(SlotRef from, size_t box, size_t slot, std::string* error = nullptr);
    // Replace one unmodified, occupied box record with the encrypted blank record.
    [[nodiscard]] bool removeFromBox(size_t box, size_t slot, std::string* error = nullptr);
    [[nodiscard]] static CreateCapability createCapability() noexcept {
        return {CreateSupport::Unsupported,
                "Gen VI Create is unsupported: encryption constant/PID, personal data, met, "
                "handler, memory and geolocation fields cannot be derived verifiably"};
    }

    bool undo(SlotRef slot) noexcept { return entries_.erase(slot) != 0; }
    void clear() noexcept { entries_.clear(); }
    [[nodiscard]] bool hasChanges() const noexcept { return !entries_.empty(); }
    [[nodiscard]] size_t changeCount() const noexcept { return entries_.size(); }
    [[nodiscard]] std::vector<StagedChange> changes() const;
    [[nodiscard]] std::vector<std::string> changeLines() const;

    // The record the shared editor should show: staged if pending, else original.
    [[nodiscard]] std::optional<PK6::Pokemon6> current(SlotRef slot) const;

    // Serialize staged records + refresh only affected block checksums, then
    // verify the result independently. Empty optional (with error) on refusal.
    [[nodiscard]] std::optional<std::vector<uint8_t>> buildWorkingCopy(std::string* error = nullptr) const;

    [[nodiscard]] Common::VerifiedExportResult exportWorkingCopy(const Gen6ExportRequest& request) const;

private:
    struct Entry {
        ChangeKind kind = ChangeKind::Edit;
        PK6::Pokemon6 record;
        std::optional<SlotRef> cloneSource;
        std::vector<std::string> notes;
    };
    [[nodiscard]] SlotRecord original(SlotRef slot) const;
    void commitEdit(SlotRef slot, PK6::Pokemon6 next, std::string note);

    Gen6Save source_;
    std::map<SlotRef, Entry> entries_;
};

// Independent verifier (also used by tests with tampered images): `edited`
// must strictly reparse as the same exact game, and every byte that differs
// from `source` must be (a) inside a staged record's range, at a plain offset
// the change kind is allowed to touch, or (b) the u16 checksum of a block that
// owns a staged record. Every staged record must decode to its staged plain.
[[nodiscard]] bool verifyWorkingCopy(const Gen6Save& source, std::span<const uint8_t> edited,
                                     const std::vector<StagedChange>& changes,
                                     std::string* error = nullptr);

} // namespace PokeVault::Integration::Gen6

#endif
