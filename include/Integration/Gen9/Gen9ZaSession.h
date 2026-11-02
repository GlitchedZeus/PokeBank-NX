#ifndef POKEBANK_GEN9_ZA_SESSION_H
#define POKEBANK_GEN9_ZA_SESSION_H

// Legends: Z-A (SAV9ZA) strict model + staged PA9 Edit / Clone / Move / Remove. Z-A is its OWN format:
// nothing here assumes the Scarlet/Violet layout. Cited from PKHeX 6501f0ab:
//   Saves/Util/SaveUtil.cs  IsSizeGen9ZA: 0x2F3284 / 0x2F3289 / 0x309FA6 / 0x309FB3, plus a valid SwishCrypto hash
//   Saves/Access/SaveBlockAccessor9ZA.cs  KBox 0x0D66012C, KParty 0x3AA1A9AD, KMyStatus 0xE3E89BD1,
//                           KTeamIndexes 0x33F39467, KTeamLocks 0x605EBC30, KSaveRevision 0x0926555A (u64)
//   Saves/SAV9ZA.cs         box slot = 0x158 party record + 0x40 gap (stride 0x198), 32 x 30; on DLC saves
//                           (SaveRevision != 0) writing a box slot sets byte +0x158 to 1 ("present")
//   Saves/Substructures/Gen9/ZA/Party9a.cs  party slot stride 0x1E0; no count byte (leading present slots)
//   PKM/PA9.cs              PK8 size/encryption; every field the shared editor writes sits at the PK8/PK9
//                           offset; Alpha flag is the byte at 0x23; language 0xD5
//   Game/Enums/GameVersion.cs  ZA = 52
// Party storage and party footers are never changed (edits touch record bytes only).

#include "Integration/Common/VerifiedStagedExport.h"
#include "Integration/Gen8/Gen8RecordEdits.h"
#include "Integration/Gen8/Gen8Save.h"
#include "Integration/Gen8/Gen8SwishCrypto.h"

#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace PokeVault::Integration::Gen9Za {

namespace PK8 = Gen8::PK8;
namespace Swish = Gen8::Swish;
using Field = Gen8EditorProvider::Field;
using Region = Gen8EditorProvider::RecordRegion;
using Gen8::TeamSlot;

namespace Layout {
inline constexpr std::array<size_t, 4> Sizes = {0x2F3284, 0x2F3289, 0x309FA6, 0x309FB3};
[[nodiscard]] constexpr bool acceptedSize(size_t n) noexcept {
    for (const auto s : Sizes) if (s == n) return true;
    return false;
}
inline constexpr uint32_t KeyBox = 0x0D66012C, KeyParty = 0x3AA1A9AD, KeyMyStatus = 0xE3E89BD1;
inline constexpr uint32_t KeyTeamIndexes = 0x33F39467, KeyTeamLocks = 0x605EBC30, KeySaveRevision = 0x0926555A;
inline constexpr size_t PartySlots = 6, BoxCount = 32, BoxSlots = 30;
inline constexpr size_t BoxStride = 0x158 + 0x40, PartyStride = 0x158 + 0x48 + 0x40, PresentFlag = 0x158;
inline constexpr size_t MyTid = 0, MySid = 2, MyGame = 4, MyGender = 5, MyLanguage = 7, MyName = 0x10, MyNameBytes = 0x1A;
inline constexpr uint8_t VersionZA = 52;
[[nodiscard]] constexpr std::string_view gameIdFor(uint8_t v) noexcept { return v == VersionZA ? "legends_za_switch" : std::string_view{}; }
} // namespace Layout

struct SlotRef {
    Region region = Region::Box;
    size_t box = 0, slot = 0;
    [[nodiscard]] static constexpr SlotRef party(size_t s) noexcept { return {Region::Party, 0, s}; }
    [[nodiscard]] static constexpr SlotRef boxed(size_t b, size_t s) noexcept { return {Region::Box, b, s}; }
    [[nodiscard]] constexpr bool canonical() const noexcept {
        return region == Region::Party ? (box == 0 && slot < Layout::PartySlots) : (box < Layout::BoxCount && slot < Layout::BoxSlots);
    }
    [[nodiscard]] constexpr size_t offset() const noexcept {
        return region == Region::Party ? slot * Layout::PartyStride : (box * Layout::BoxSlots + slot) * Layout::BoxStride;
    }
    bool operator<(const SlotRef& o) const noexcept {
        if (region != o.region) return region < o.region;
        if (box != o.box) return box < o.box;
        return slot < o.slot;
    }
    bool operator==(const SlotRef& o) const noexcept { return region == o.region && box == o.box && slot == o.slot; }
};

class Gen9ZaSave {
public:
    [[nodiscard]] static std::optional<Gen9ZaSave> parse(std::span<const uint8_t> bytes, std::string_view assigned = {},
                                                         std::string* error = nullptr);
    [[nodiscard]] std::span<const uint8_t> bytes() const noexcept { return bytes_; }
    [[nodiscard]] const std::vector<Swish::Block>& blocks() const noexcept { return blocks_; }
    [[nodiscard]] std::string_view exactGameId() const noexcept { return gameId_; }
    [[nodiscard]] uint8_t partyCount() const noexcept { return partyCount_; }
    [[nodiscard]] Gen8::SlotRecord at(SlotRef slot) const;
    [[nodiscard]] const std::optional<std::vector<TeamSlot>>& teams() const noexcept { return teams_; }
    [[nodiscard]] const std::string& teamDiagnostic() const noexcept { return teamDiagnostic_; }
    [[nodiscard]] const Swish::Block& myStatus() const noexcept { return blocks_[myStatus_]; }
    [[nodiscard]] size_t partyIndex() const noexcept { return party_; }
    [[nodiscard]] size_t boxIndex() const noexcept { return box_; }
    [[nodiscard]] uint64_t revision() const noexcept { return revision_; }

private:
    Gen9ZaSave() = default;
    std::vector<uint8_t> bytes_;
    std::vector<Swish::Block> blocks_;
    std::string gameId_;
    size_t myStatus_ = 0, party_ = 0, box_ = 0;
    uint8_t partyCount_ = 0;
    uint64_t revision_ = 0;
    std::optional<std::vector<TeamSlot>> teams_;
    std::string teamDiagnostic_;   // why teams_ is unreadable
};

enum class ChangeKind : uint8_t { Edit, Clone, Remove };
struct StagedChange {
    SlotRef slot;
    ChangeKind kind = ChangeKind::Edit;
    std::optional<SlotRef> cloneSource;
    std::vector<uint8_t> originalPlain, stagedPlain;
    std::vector<std::string> notes;
};
struct Gen9ZaExportRequest {
    std::string rootDirectory, sourcePath, timestamp, applicationSha = "unknown";
};

[[nodiscard]] const Gen8Edits::Rules& rules();

class Gen9ZaStagedSession {
public:
    explicit Gen9ZaStagedSession(Gen9ZaSave source) : source_(std::move(source)) {}
    [[nodiscard]] const Gen9ZaSave& source() const noexcept { return source_; }
    [[nodiscard]] std::string_view editBlockedReason(SlotRef slot) const noexcept;
    [[nodiscard]] std::string_view storageBlockedReason(SlotRef slot) const noexcept;
    [[nodiscard]] std::optional<PK8::Pokemon8> current(SlotRef slot) const;
    [[nodiscard]] bool stageValue(SlotRef slot, Field field, uint32_t value, std::string* error = nullptr);
    [[nodiscard]] bool stageText(SlotRef slot, Field field, std::string_view text, std::string* error = nullptr);
    [[nodiscard]] bool cloneToBox(SlotRef from, SlotRef to, std::string* error = nullptr);
    [[nodiscard]] bool removeFromBox(SlotRef at, std::string* error = nullptr);
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
    void restoreEntry(SlotRef slot, const EntrySnapshot& s);
    [[nodiscard]] bool entryMatches(SlotRef slot, const EntrySnapshot& s) const;
    void clear() noexcept { entries_.clear(); }
    [[nodiscard]] bool hasChanges() const noexcept { return !entries_.empty(); }
    [[nodiscard]] size_t changeCount() const noexcept { return entries_.size(); }
    [[nodiscard]] std::vector<StagedChange> changes() const;
    [[nodiscard]] std::vector<std::string> changeLines() const;
    [[nodiscard]] std::optional<std::vector<uint8_t>> buildWorkingCopy(std::string* error = nullptr) const;
    [[nodiscard]] Common::VerifiedExportResult exportWorkingCopy(const Gen9ZaExportRequest& request) const;
    static constexpr std::string_view kCreateReason =
        "Legends: Z-A Create is unsupported: EC/PID, personal data, met, Alpha, Plus flags, handler and cached "
        "stats cannot be derived verifiably";

private:
    struct Entry {
        ChangeKind kind = ChangeKind::Edit;
        PK8::Pokemon8 record;
        std::optional<SlotRef> cloneSource;
        std::vector<std::string> notes;
    };
    void commitEdit(SlotRef slot, PK8::Pokemon8 next, std::string note);
    Gen9ZaSave source_;
    std::map<SlotRef, Entry> entries_;
};

[[nodiscard]] bool verifyWorkingCopy(const Gen9ZaSave& source, std::span<const uint8_t> edited,
                                     const std::vector<StagedChange>& changes, std::string* error = nullptr);

} // namespace PokeVault::Integration::Gen9Za

#endif
