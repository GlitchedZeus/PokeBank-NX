#ifndef POKEBANK_GEN8_STAGED_SESSION_H
#define POKEBANK_GEN8_STAGED_SESSION_H

// Staged PK8 Edit / Clone / Remove on an app-owned copy of a strictly parsed Sword/Shield `main`.
//
// The source Gen8Save is held by value and never mutated. Edits live in app memory as decrypted
// PK8 records; the only output is a candidate SwishCrypto image that buildWorkingCopy() re-encrypts
// from the immutable block list with ONLY the staged record ranges of the Party/Box blocks
// replaced, and that verifyWorkingCopy() then proves byte for byte (strict reparse, canonical
// re-encryption, identical block keys/types/sizes, every other block identical, every staged
// record exactly the reviewed plain record inside the provider's whitelist). Export goes through
// VerifiedStagedExport into an app-owned directory; nothing here can write a source or installed
// save, mount a save filesystem, or inject anything.
//
// Battle teams (TeamIndexes8 / TeamLocks) are respected: a record referenced by a battle team is
// never removed or moved (that would leave a dangling team reference), a record in a LOCKED team is
// never edited (PKHeX treats those slots as locked), a clone never lands on a team-referenced slot,
// and when the team blocks cannot be read every box change is refused.

#include "Integration/Common/VerifiedStagedExport.h"
#include "Integration/Gen8/Gen8EditorProvider.h"
#include "Integration/Gen8/Gen8Save.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace PokeVault::Integration::Gen8 {

using RecordRegion = Gen8EditorProvider::RecordRegion;
using Field = Gen8EditorProvider::Field;

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

struct StagedChange {
    SlotRef slot;
    ChangeKind kind = ChangeKind::Edit;
    std::optional<SlotRef> cloneSource;
    std::vector<uint8_t> originalPlain;  // decrypted source record (immutable baseline)
    std::vector<uint8_t> stagedPlain;    // decrypted staged record, checksum refreshed
    std::vector<std::string> notes;
};

struct Gen8ExportRequest {
    std::string rootDirectory;     // app-owned export root (host fixture or exports/gen8)
    std::string sourcePath;        // evidence only; never opened here
    std::string timestamp;         // YYYYMMDD_HHMMSS
    std::string applicationSha = "unknown";
    Common::ExportFailurePoint failurePoint = Common::ExportFailurePoint::None;
};

class Gen8StagedSession {
public:
    explicit Gen8StagedSession(Gen8Save source) : source_(std::move(source)) {}

    [[nodiscard]] const Gen8Save& source() const noexcept { return source_; }

    // Why a slot may not be changed at all (edit, clone destination, remove); empty when free.
    // Party slots are never team-referenced (TeamIndexes8 stores box positions only).
    [[nodiscard]] std::string_view editBlockedReason(SlotRef slot) const noexcept;
    [[nodiscard]] std::string_view storageBlockedReason(SlotRef slot) const noexcept;

    [[nodiscard]] bool stageValue(SlotRef slot, Field field, uint32_t value, size_t index = 0,
                                  std::string* error = nullptr);
    // Nickname / OT name: 1..12 printable ASCII characters. PK8 names are UTF-16 code units and
    // ASCII maps to itself, so no glyph table is invented.
    [[nodiscard]] bool stageText(SlotRef slot, Field field, std::string_view text, std::string* error = nullptr);
    // Copy one unmodified, occupied BOX record into a verified blank, team-free box slot.
    [[nodiscard]] bool cloneToBox(SlotRef from, size_t box, size_t slot, std::string* error = nullptr);
    // Replace one unmodified, occupied, team-free box record with the encrypted blank PK8.
    [[nodiscard]] bool removeFromBox(size_t box, size_t slot, std::string* error = nullptr);
    [[nodiscard]] static std::string_view createUnsupportedReason() noexcept {
        return "Sword/Shield Create is unsupported: encryption constant/PID, personal data, met, "
               "handler, memory and cached battle stats cannot be derived verifiably";
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
    [[nodiscard]] EntrySnapshot snapshotEntry(SlotRef slot) const {
        EntrySnapshot out;
        const auto it = entries_.find(slot);
        if (it == entries_.end()) return out;
        out.present = true;
        out.kind = it->second.kind;
        out.record = it->second.record;
        out.cloneSource = it->second.cloneSource;
        out.notes = it->second.notes;
        return out;
    }
    void restoreEntry(SlotRef slot, const EntrySnapshot& snapshot) {
        if (!snapshot.present || !snapshot.record) {
            entries_.erase(slot);
            return;
        }
        entries_.insert_or_assign(slot, Entry{snapshot.kind, *snapshot.record, snapshot.cloneSource, snapshot.notes});
    }
    [[nodiscard]] bool entryMatches(SlotRef slot, const EntrySnapshot& snapshot) const {
        const auto it = entries_.find(slot);
        if (it == entries_.end()) return !snapshot.present;
        if (!snapshot.present || !snapshot.record) return false;
        const auto a = it->second.record.plain(), b = snapshot.record->plain();
        return it->second.kind == snapshot.kind && std::equal(a.begin(), a.end(), b.begin(), b.end()) &&
               it->second.cloneSource == snapshot.cloneSource && it->second.notes == snapshot.notes;
    }
    void clear() noexcept { entries_.clear(); }
    [[nodiscard]] bool hasChanges() const noexcept { return !entries_.empty(); }
    [[nodiscard]] size_t changeCount() const noexcept { return entries_.size(); }
    [[nodiscard]] std::vector<StagedChange> changes() const;
    [[nodiscard]] std::vector<std::string> changeLines() const;

    [[nodiscard]] std::optional<PK8::Pokemon8> current(SlotRef slot) const;
    [[nodiscard]] SlotRecord original(SlotRef slot) const;

    [[nodiscard]] std::optional<std::vector<uint8_t>> buildWorkingCopy(std::string* error = nullptr) const;
    [[nodiscard]] Common::VerifiedExportResult exportWorkingCopy(const Gen8ExportRequest& request) const;

private:
    struct Entry {
        ChangeKind kind = ChangeKind::Edit;
        PK8::Pokemon8 record;
        std::optional<SlotRef> cloneSource;
        std::vector<std::string> notes;
    };
    void commitEdit(SlotRef slot, PK8::Pokemon8 next, std::string note);

    Gen8Save source_;
    std::map<SlotRef, Entry> entries_;
};

// Independent verifier (also used by tests with tampered images). `edited` must be the same size,
// strictly reparse as the same exact game with the same party count, be the canonical SwishCrypto
// encryption of its own block list (no stray bytes, valid hash), keep every block's key, type,
// subtype and size, keep every block except Party/Box byte-identical, and differ inside Party/Box
// only within staged record ranges. Each staged record must decode strictly to its reviewed plain
// record, inside the provider's per-byte whitelist relative to its immutable baseline/donor.
[[nodiscard]] bool verifyWorkingCopy(const Gen8Save& source, std::span<const uint8_t> edited,
                                     const std::vector<StagedChange>& changes, std::string* error = nullptr);

} // namespace PokeVault::Integration::Gen8

#endif
