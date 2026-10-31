#ifndef POKEBANK_GEN8B_STAGED_SESSION_H
#define POKEBANK_GEN8B_STAGED_SESSION_H

// Staged PB8 Edit / Clone / Remove on an app-owned copy of a strictly parsed BD/SP save.
// Same rules as Sword/Shield (Gen8RecordEdits): provider-whitelisted fields only, box-only storage
// changes, battle-team members never moved/removed, locked-team members never edited, unreadable
// team data refuses every box change. Export writes only staged record ranges plus the whole-file
// MD5; verifyWorkingCopy() proves it independently. Nothing here writes a source or installed save.

#include "Integration/Common/VerifiedStagedExport.h"
#include "Integration/Gen8/Gen8RecordEdits.h"
#include "Integration/Gen8b/Gen8bSave.h"

#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace PokeVault::Integration::Gen8b {

using Field = Gen8EditorProvider::Field;
using RecordRegion = Gen8EditorProvider::RecordRegion;

struct SlotRef {
    RecordRegion region = RecordRegion::Box;
    size_t box = 0, slot = 0;
    [[nodiscard]] static constexpr SlotRef party(size_t s) noexcept { return {RecordRegion::Party, 0, s}; }
    [[nodiscard]] static constexpr SlotRef boxed(size_t b, size_t s) noexcept { return {RecordRegion::Box, b, s}; }
    [[nodiscard]] constexpr bool canonical() const noexcept {
        return region == RecordRegion::Party ? (box == 0 && slot < Layout::PartySlots)
                                             : (box < Layout::BoxCount && slot < Layout::BoxSlots);
    }
    bool operator<(const SlotRef& o) const noexcept {
        if (region != o.region) return region < o.region;
        if (box != o.box) return box < o.box;
        return slot < o.slot;
    }
    bool operator==(const SlotRef& o) const noexcept { return region == o.region && box == o.box && slot == o.slot; }
};

enum class ChangeKind : uint8_t { Edit, Clone, Remove };

struct StagedChange {
    SlotRef slot;
    ChangeKind kind = ChangeKind::Edit;
    std::optional<SlotRef> cloneSource;
    std::vector<uint8_t> originalPlain, stagedPlain;
    std::vector<std::string> notes;
};

struct Gen8bExportRequest {
    std::string rootDirectory, sourcePath, timestamp, applicationSha = "unknown";
    Common::ExportFailurePoint failurePoint = Common::ExportFailurePoint::None;
};

[[nodiscard]] const Gen8Edits::Rules& rules();

class Gen8bStagedSession {
public:
    explicit Gen8bStagedSession(Gen8bSave source) : source_(std::move(source)) {}
    [[nodiscard]] const Gen8bSave& source() const noexcept { return source_; }

    [[nodiscard]] std::string_view editBlockedReason(SlotRef slot) const noexcept;
    [[nodiscard]] std::string_view storageBlockedReason(SlotRef slot) const noexcept;

    [[nodiscard]] bool stageValue(SlotRef slot, Field field, uint32_t value, std::string* error = nullptr);
    [[nodiscard]] bool stageText(SlotRef slot, Field field, std::string_view text, std::string* error = nullptr);
    [[nodiscard]] bool cloneToBox(SlotRef from, size_t box, size_t slot, std::string* error = nullptr);
    [[nodiscard]] bool removeFromBox(size_t box, size_t slot, std::string* error = nullptr);

    [[nodiscard]] static std::string_view createUnsupportedReason() noexcept {
        return "BD/SP Create is unsupported: encryption constant/PID, personal data, met, handler, memory and "
               "cached battle stats cannot be derived verifiably";
    }
    bool undo(SlotRef slot) noexcept { return entries_.erase(slot) != 0; }
    [[nodiscard]] bool isStaged(SlotRef slot) const noexcept { return entries_.count(slot) != 0; }
    struct EntrySnapshot {
        bool present = false;
        ChangeKind kind = ChangeKind::Edit;
        std::optional<PK8::Pokemon8> record;
        std::optional<SlotRef> cloneSource;
        std::vector<std::string> notes;
    };
    [[nodiscard]] EntrySnapshot snapshotEntry(SlotRef slot) const;
    void restoreEntry(SlotRef slot, const EntrySnapshot& snapshot);
    [[nodiscard]] bool entryMatches(SlotRef slot, const EntrySnapshot& snapshot) const;
    void clear() noexcept { entries_.clear(); }
    [[nodiscard]] bool hasChanges() const noexcept { return !entries_.empty(); }
    [[nodiscard]] size_t changeCount() const noexcept { return entries_.size(); }
    [[nodiscard]] std::vector<StagedChange> changes() const;
    [[nodiscard]] std::vector<std::string> changeLines() const;
    [[nodiscard]] std::optional<PK8::Pokemon8> current(SlotRef slot) const;
    [[nodiscard]] SlotRecord original(SlotRef slot) const;

    [[nodiscard]] std::optional<std::vector<uint8_t>> buildWorkingCopy(std::string* error = nullptr) const;
    [[nodiscard]] Common::VerifiedExportResult exportWorkingCopy(const Gen8bExportRequest& request) const;

private:
    struct Entry {
        ChangeKind kind = ChangeKind::Edit;
        PK8::Pokemon8 record;
        std::optional<SlotRef> cloneSource;
        std::vector<std::string> notes;
    };
    void commitEdit(SlotRef slot, PK8::Pokemon8 next, std::string note);
    Gen8bSave source_;
    std::map<SlotRef, Entry> entries_;
};

[[nodiscard]] bool verifyWorkingCopy(const Gen8bSave& source, std::span<const uint8_t> edited,
                                     const std::vector<StagedChange>& changes, std::string* error = nullptr);

} // namespace PokeVault::Integration::Gen8b

#endif
