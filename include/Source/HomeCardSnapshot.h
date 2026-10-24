#ifndef POKEBANK_SOURCE_HOME_CARD_SNAPSHOT_H
#define POKEBANK_SOURCE_HOME_CARD_SNAPSHOT_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

#include <sys/stat.h>

#include "Legacy/LegacySourceBindings.h"

namespace PokeVault::Source {

// Generation-neutral, presentation-only summary of ONE strictly validated
// save, shown on the Home card and hero without opening an editor. A
// snapshot never grants write access; editor Open always revalidates the
// exact bytes again.
struct HomeCardPartySlot {
    uint16_t species = 0;
    uint8_t level = 0; // 0 = the native record exposes no level for previews
    uint8_t form = 0;
    bool shiny = false;
    // 0 male / 1 female / 2 genderless; GenderUnknown when the format (Gen I)
    // does not define it or the species has no data for its generation.
    static constexpr uint8_t GenderUnknown = 0xFF;
    uint8_t gender = GenderUnknown;
};

enum class SnapshotState : uint8_t {
    Unavailable, // no validated data at all; nothing is shown as current
    Current,     // validated from the file's present size/mtime/bytes
    Stale,       // last validated data retained, but the file has since changed
                 // and did not re-validate; never presented as current
};

struct FileStamp {
    uint64_t size = 0;
    int64_t modifiedTime = 0;
    [[nodiscard]] bool operator==(const FileStamp& other) const noexcept {
        return size == other.size && modifiedTime == other.modifiedTime;
    }
    [[nodiscard]] bool operator!=(const FileStamp& other) const noexcept {
        return !(*this == other);
    }
};

struct HomeCardSnapshot {
    SnapshotState state = SnapshotState::Unavailable;
    std::string reason; // why Unavailable/Stale; empty when Current
    std::string gameId;
    std::string sourceIdentity;
    std::string contentFingerprint;
    std::string trainerName;
    uint8_t trainerGender = 0;
    bool trainerGenderKnown = false;
    bool newGameNoTrainer = false;
    uint16_t dexSeen = 0;
    uint16_t dexCaught = 0;
    uint16_t dexTotal = 0;
    uint8_t partyCount = 0;
    std::array<HomeCardPartySlot, 6> party{};

    [[nodiscard]] bool current() const noexcept { return state == SnapshotState::Current; }
    [[nodiscard]] bool hasData() const noexcept { return state != SnapshotState::Unavailable; }
};

[[nodiscard]] inline HomeCardSnapshot unavailableSnapshot(std::string reason) {
    HomeCardSnapshot out;
    out.reason = std::move(reason);
    return out;
}

// What one Home card currently points at. `bindingSignature` captures the
// profile/game assignment (path, provider, copy, persisted fingerprint) so
// re-linking or Forget always invalidates, even when size/mtime match.
struct SnapshotRequest {
    std::string key; // profile|game
    std::string sourcePath;
    std::string bindingSignature;
};

[[nodiscard]] inline std::string homeSnapshotKey(std::string_view profile, std::string_view gameId) {
    std::string key(profile);
    key += '|';
    key += gameId;
    return key;
}

// Request for an explicitly assigned Gen IV/V file. Every persisted field of
// the assignment participates, so Forget/relink/copy changes always reparse.
[[nodiscard]] inline SnapshotRequest assignedSnapshotRequest(
    std::string_view profile, std::string_view gameId, const Legacy::AssignedFile& assigned) {
    SnapshotRequest request;
    request.key = homeSnapshotKey(profile, gameId);
    if (assigned.status != Legacy::AssignedFileStatus::Ready) return request;
    const auto& binding = assigned.binding;
    request.sourcePath = binding.sourcePath;
    for (const std::string* part : {&assigned.sourceIdentity, &binding.sourceType,
             &binding.expectedRawFamily, &binding.sourceVariant, &binding.physicalIdentity,
             &binding.contentFingerprint}) {
        request.bindingSignature += *part;
        request.bindingSignature += '\t';
    }
    return request;
}

// Cheap change detector: a regular file's size and modification time.
[[nodiscard]] inline std::optional<FileStamp> statRegularFile(const std::string& path) {
    struct stat st{};
    if (path.empty() || ::stat(path.c_str(), &st) != 0 || !S_ISREG(st.st_mode))
        return std::nullopt;
    return FileStamp{static_cast<uint64_t>(st.st_size), static_cast<int64_t>(st.st_mtime)};
}

enum class SnapshotRefresh : uint8_t {
    Reused,     // stamp unchanged: no file read, no parse
    Parsed,     // stamp or assignment changed: strictly re-validated
    Missing,    // the assigned file is gone
    Unassigned, // no explicit assignment for this profile/game
};

// Cache keyed by profile|game. The cheap file stamp (size + mtime) is checked
// on every refresh; the expensive strict parser runs only when the stamp or
// the assignment changes. The parser computes the content fingerprint, so a
// rewrite with identical bytes is recognised as the same validated data.
class HomeSnapshotCache {
public:
    template <class StatFn, class ParseFn>
    const HomeCardSnapshot& refresh(const SnapshotRequest& request, StatFn&& stat,
                                    ParseFn&& parse, SnapshotRefresh* outcome = nullptr) {
        Entry& entry = entries_[request.key];
        auto report = [&](SnapshotRefresh value) -> const HomeCardSnapshot& {
            if (outcome) *outcome = value;
            return entry.snapshot;
        };
        if (request.sourcePath.empty()) {
            entry = Entry{};
            entry.snapshot = unavailableSnapshot("No save linked to this game.");
            return report(SnapshotRefresh::Unassigned);
        }
        const std::optional<FileStamp> before = stat(request.sourcePath);
        if (!before) {
            // A missing file has no current data. Do not show the old party
            // as if it were still the save; the card states it is missing.
            entry = Entry{};
            entry.sourcePath = request.sourcePath;
            entry.bindingSignature = request.bindingSignature;
            entry.snapshot = unavailableSnapshot("Linked save file is missing.");
            return report(SnapshotRefresh::Missing);
        }
        if (entry.valid && entry.sourcePath == request.sourcePath &&
            entry.bindingSignature == request.bindingSignature && entry.stamp == *before)
            return report(SnapshotRefresh::Reused);

        HomeCardSnapshot fresh = parse();
        ++parseCount_;
        const bool sameAssignment = entry.valid && entry.sourcePath == request.sourcePath &&
            entry.bindingSignature == request.bindingSignature;
        if (fresh.state != SnapshotState::Current) {
            fresh.state = SnapshotState::Unavailable;
            if (fresh.reason.empty()) fresh.reason = "Save did not pass validation.";
            if (sameAssignment && entry.snapshot.hasData()) {
                // Keep the last validated data visible but explicitly stale.
                HomeCardSnapshot stale = entry.snapshot;
                stale.state = SnapshotState::Stale;
                stale.reason = "Save changed and could not be re-validated: " + fresh.reason;
                fresh = std::move(stale);
            }
        } else {
            fresh.reason.clear();
        }
        entry.valid = true;
        entry.sourcePath = request.sourcePath;
        entry.bindingSignature = request.bindingSignature;
        // If the emulator wrote while we parsed, record a stamp that cannot
        // match so the next refresh re-validates instead of trusting this read.
        const std::optional<FileStamp> after = stat(request.sourcePath);
        entry.stamp = (after && *after == *before) ? *before : FileStamp{~uint64_t{0}, -1};
        entry.snapshot = std::move(fresh);
        return report(SnapshotRefresh::Parsed);
    }

    // Presentation lookup only: no stat, no parse (safe on every input frame).
    [[nodiscard]] const HomeCardSnapshot* find(std::string_view key) const {
        const auto it = entries_.find(std::string(key));
        return it == entries_.end() ? nullptr : &it->second.snapshot;
    }
    void invalidate(std::string_view key) { entries_.erase(std::string(key)); }
    void clear() { entries_.clear(); }
    [[nodiscard]] size_t parseCount() const noexcept { return parseCount_; }

private:
    struct Entry {
        bool valid = false;
        std::string sourcePath;
        std::string bindingSignature;
        FileStamp stamp;
        HomeCardSnapshot snapshot;
    };
    std::unordered_map<std::string, Entry> entries_;
    size_t parseCount_ = 0;
};

// Home party-strip status for one snapshot. Current data reads exactly like
// Gen I-IV ("Current save party"); never a stale-cache wording.
[[nodiscard]] inline std::string homePartyStatus(const HomeCardSnapshot& snapshot) {
    if (snapshot.state == SnapshotState::Unavailable)
        return snapshot.reason.empty() ? std::string("Party preview unavailable.") : snapshot.reason;
    if (snapshot.state == SnapshotState::Stale) return "STALE - " + snapshot.reason;
    if (snapshot.newGameNoTrainer) return "New-game save: no trainer or party yet.";
    for (const auto& slot : snapshot.party)
        if (slot.species != 0) return "Current save party";
    return "No active party Pokémon.";
}

} // namespace PokeVault::Source

#endif
