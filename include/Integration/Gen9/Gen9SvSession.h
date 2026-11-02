#ifndef POKEBANK_GEN9_SV_SESSION_H
#define POKEBANK_GEN9_SV_SESSION_H

// Scarlet / Violet (SAV9SV) strict model + staged PK9 Edit / Clone / Move / Remove.
// Cited from PKHeX 6501f0ab:
//   Saves/Util/SaveUtil.cs  IsSizeGen9SV: the listed 1.0-1.2 sizes, or the Teal Mask / Indigo Disk
//                           ranges (+ SIZE_G9_202 / SIZE_G9_300), and a valid SwishCrypto hash
//   Saves/Access/SaveBlockAccessor9SV.cs  KBox 0x0D66012C, KParty 0x3AA1A9AD, KMyStatus 0xE3E89BD1,
//                           KTeamIndexes 0x33F39467, KTeamLocks 0x605EBC30
//   Saves/SAV9SV.cs         32 boxes x 30, PARTY-format records in boxes (0x158), Party9 count @6*0x158
//   Saves/Substructures/Gen9/SV/MyStatus9.cs  TID16 +0, SID16 +2, game +4 (Scarlet 50 / Violet 51),
//                           gender +5, language +7, OT +0x10 (0x1A)
//   PKM/PK9.cs              same size/encryption as PK8 and the SAME offsets for every field the shared
//                           editor writes (0x0A held item, 0x0C/0x0E IDs, 0x22 fateful, 0x32 Pokerus,
//                           0x58 nickname, 0x8C IV32, 0xC4 handler, 0xF8 OT, 0x112 friendship,
//                           0x124 ball, 0x125 OT gender); PK9-only reads: language 0xD5, Tera types
//                           0x94 / 0x95, scale 0x4A, stat nature 0x21.
// The verified PK8 codec and the shared Gen8RecordEdits whitelist are therefore reused unchanged.
// Z-A (SAV9ZA / PA9) is a different format and is never handled here.

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

namespace PokeVault::Integration::Gen9Sv {

namespace PK8 = Gen8::PK8;
namespace Swish = Gen8::Swish;
using Field = Gen8EditorProvider::Field;
using Region = Gen8EditorProvider::RecordRegion;
using Gen8::TeamSlot;

namespace Layout {
inline constexpr std::array<size_t, 18> FixedSizes = {
    0x31626F, 0x31627C, 0x319DB3, 0x319DC0, 0x319DC3, 0x319DD0, 0x31A2C0, 0x31A2CD, 0x31A2DD, 0x31A2D0,
    0x31CF7C, 0x31CA6F, 0x31CF7C - 0xD, 0x31CA6F - 0xD, 0x31CF7C + 0x5, 0x31CA6F + 0x5, 0x31CF7C - 0xD + 0x5,
    0x31CA6F - 0xD + 0x5};
inline constexpr size_t DlcMin1 = 0x4329A0, DlcMax1 = 0x4329C9, DlcMin2 = 0x432EB6, DlcMax2 = 0x432ED6;
inline constexpr size_t Add202 = 0xC8E, Add300 = 0x83AD;
[[nodiscard]] constexpr bool acceptedSize(size_t n) noexcept {
    for (const auto s : FixedSizes) if (s == n) return true;
    for (const size_t add : {size_t{0}, Add202, Add300})
        if ((n >= DlcMin1 + add && n <= DlcMax1 + add + 100) || (n >= DlcMin2 + add && n <= DlcMax2 + add + 100)) return true;
    return false;
}
inline constexpr uint32_t KeyBox = 0x0D66012C, KeyParty = 0x3AA1A9AD, KeyMyStatus = 0xE3E89BD1;
inline constexpr uint32_t KeyTeamIndexes = 0x33F39467, KeyTeamLocks = 0x605EBC30;
inline constexpr size_t PartySlots = 6, PartyCountOffset = 6 * PK8::PartySize, BoxCount = 32, BoxSlots = 30;
inline constexpr size_t MyTid = 0, MySid = 2, MyGame = 4, MyGender = 5, MyLanguage = 7, MyName = 0x10, MyNameBytes = 0x1A;
inline constexpr uint8_t VersionScarlet = 50, VersionViolet = 51;
[[nodiscard]] constexpr std::string_view gameIdFor(uint8_t v) noexcept {
    return v == VersionScarlet ? "scarlet_switch" : v == VersionViolet ? "violet_switch" : std::string_view{};
}
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
        return (region == Region::Party ? slot : box * Layout::BoxSlots + slot) * PK8::PartySize;
    }
    bool operator<(const SlotRef& o) const noexcept {
        if (region != o.region) return region < o.region;
        if (box != o.box) return box < o.box;
        return slot < o.slot;
    }
    bool operator==(const SlotRef& o) const noexcept { return region == o.region && box == o.box && slot == o.slot; }
};

class Gen9SvSave {
public:
    [[nodiscard]] static std::optional<Gen9SvSave> parse(std::span<const uint8_t> bytes, std::string_view assigned = {},
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

private:
    Gen9SvSave() = default;
    std::vector<uint8_t> bytes_;
    std::vector<Swish::Block> blocks_;
    std::string gameId_;
    size_t myStatus_ = 0, party_ = 0, box_ = 0;
    uint8_t partyCount_ = 0;
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
struct Gen9SvExportRequest {
    std::string rootDirectory, sourcePath, timestamp, applicationSha = "unknown";
};

[[nodiscard]] const Gen8Edits::Rules& rules();

class Gen9SvStagedSession {
public:
    explicit Gen9SvStagedSession(Gen9SvSave source) : source_(std::move(source)) {}
    [[nodiscard]] const Gen9SvSave& source() const noexcept { return source_; }
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
    [[nodiscard]] Common::VerifiedExportResult exportWorkingCopy(const Gen9SvExportRequest& request) const;
    static constexpr std::string_view kCreateReason =
        "Scarlet/Violet Create is unsupported: EC/PID, personal data, met, Tera type, handler, memory and cached "
        "stats cannot be derived verifiably";

private:
    struct Entry {
        ChangeKind kind = ChangeKind::Edit;
        PK8::Pokemon8 record;
        std::optional<SlotRef> cloneSource;
        std::vector<std::string> notes;
    };
    void commitEdit(SlotRef slot, PK8::Pokemon8 next, std::string note);
    Gen9SvSave source_;
    std::map<SlotRef, Entry> entries_;
};

[[nodiscard]] bool verifyWorkingCopy(const Gen9SvSave& source, std::span<const uint8_t> edited,
                                     const std::vector<StagedChange>& changes, std::string* error = nullptr);

} // namespace PokeVault::Integration::Gen9Sv

#endif
