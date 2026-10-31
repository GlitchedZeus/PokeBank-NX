#ifndef POKEBANK_GEN8A_SESSION_H
#define POKEBANK_GEN8A_SESSION_H

// Legends: Arceus (SAV8LA) strict model + staged PA8 Edit / Clone / Move / Remove.
// Cited from PKHeX 6501f0ab: SaveUtil SIZE_G8LA 0x136DDE / SIZE_G8LA_1 0x13AD06 (SwishCrypto hash
// must be valid); SaveBlockAccessor8LA KMyStatus 0xF25C070E, KParty 0x2985FE5D, KBox 0x47E1CEAB;
// Party8a = 6 x 0x178 + count byte; SAV8LA box slots are STORED-size PA8 (0x168), 32 boxes x 30;
// MyStatus8a TID +0x10, SID +0x12, game +0x14 (Legends: Arceus = 47), gender +0x15, OT +0x20.
// The SwishCrypto container is the verified Gen VIII one (Integration/Gen8/Gen8SwishCrypto.h).
// Nothing here writes a source or installed save; export goes through VerifiedStagedExport.

#include "Integration/Common/VerifiedStagedExport.h"
#include "Integration/Gen8/Gen8SwishCrypto.h"
#include "Integration/Gen8a/Gen8aPA8.h"
#include "UI/ExactFormatEditorProvider.h"
#include "UI/SharedPokemonEditorContract.h"

#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace PokeVault::Integration::Gen8a {

namespace Swish = Gen8::Swish;
using Field = PokeBank::UIModel::SharedPokemonEditor::FieldIdentity;
using FieldState = PokeBank::UIModel::ExactFormatEditor::FieldState;

namespace Layout {
inline constexpr std::array<size_t, 2> Sizes = {0x136DDE, 0x13AD06};
inline constexpr uint32_t KeyMyStatus = 0xF25C070E, KeyParty = 0x2985FE5D, KeyBox = 0x47E1CEAB;
inline constexpr size_t PartySlots = 6, PartyCountOffset = 6 * PA8::PartySize;
inline constexpr size_t BoxCount = 32, BoxSlots = 30;
inline constexpr size_t MyTid = 0x10, MySid = 0x12, MyGame = 0x14, MyGender = 0x15, MyLanguage = 0x17, MyName = 0x20,
                        MyNameBytes = 0x1A;
inline constexpr uint8_t VersionPLA = 47;
} // namespace Layout

enum class Region : uint8_t { Box, Party };

[[nodiscard]] constexpr FieldState fieldState(Field field, Region region) noexcept {
    switch (field) {
        case Field::Nickname:
        case Field::OriginalTrainer:
        case Field::OriginalTrainerGender:
        case Field::TrainerId:
        case Field::SecretId:
        case Field::Friendship:
        case Field::Ball:
        case Field::FatefulEncounter:
            return FieldState::Editable;
        case Field::Shiny:
            return FieldState::Derived;
        case Field::CalculatedStats:
            return region == Region::Party ? FieldState::Derived : FieldState::Hidden;
        // Effort Levels, IVs, nature and level feed stats; Alpha/Noble/mastery are encounter data.
        // Held items are stored but Pokemon cannot hold items in Legends: Arceus.
        case Field::HeldItem:
        case Field::EffortLevel:
        case Field::IV:
        case Field::Nature:
        case Field::StatNature:
        case Field::Level:
        case Field::Species:
        case Field::Form:
        case Field::Gender:
        case Field::Language:
        case Field::Experience:
        case Field::Ability:
        case Field::PersonalityId:
        case Field::OriginGame:
        case Field::MetLevel:
        case Field::MetLocation:
        case Field::MetDate:
        case Field::Egg:
        case Field::EggLocation:
        case Field::EggDate:
        case Field::Move:
        case Field::MovePP:
        case Field::MovePPUps:
        case Field::MoveCompatibility:
        case Field::EncounterLegality:
        case Field::Provenance:
            return FieldState::ReadOnly;
        case Field::Pokerus:  // stored, unused by Legends: Arceus
        case Field::EV:       // stored, unused (Effort Levels replace EVs)
        case Field::AV:
        case Field::DV:
        case Field::StatExperience:
            return FieldState::Hidden;
    }
    return FieldState::Hidden;
}
[[nodiscard]] constexpr bool editable(Field f) noexcept { return fieldState(f, Region::Box) == FieldState::Editable; }

struct SlotRef {
    Region region = Region::Box;
    size_t box = 0, slot = 0;
    [[nodiscard]] static constexpr SlotRef party(size_t s) noexcept { return {Region::Party, 0, s}; }
    [[nodiscard]] static constexpr SlotRef boxed(size_t b, size_t s) noexcept { return {Region::Box, b, s}; }
    [[nodiscard]] constexpr bool canonical() const noexcept {
        return region == Region::Party ? (box == 0 && slot < Layout::PartySlots) : (box < Layout::BoxCount && slot < Layout::BoxSlots);
    }
    [[nodiscard]] constexpr size_t recordSize() const noexcept { return region == Region::Party ? PA8::PartySize : PA8::StoredSize; }
    [[nodiscard]] constexpr size_t offset() const noexcept {
        return region == Region::Party ? slot * PA8::PartySize : (box * Layout::BoxSlots + slot) * PA8::StoredSize;
    }
    bool operator<(const SlotRef& o) const noexcept {
        if (region != o.region) return region < o.region;
        if (box != o.box) return box < o.box;
        return slot < o.slot;
    }
    bool operator==(const SlotRef& o) const noexcept { return region == o.region && box == o.box && slot == o.slot; }
};

struct Record {
    bool valid = false;               // strict decode
    std::optional<PA8::Pokemon8a> pokemon;
    [[nodiscard]] bool occupied() const noexcept { return valid && pokemon && !pokemon->empty(); }
    [[nodiscard]] bool blank() const noexcept { return valid && pokemon && pokemon->allZero(); }
};

class Gen8aSave {
public:
    [[nodiscard]] static std::optional<Gen8aSave> parse(std::span<const uint8_t> bytes, std::string_view assignedExactGame = {},
                                                        std::string* error = nullptr);
    [[nodiscard]] std::span<const uint8_t> bytes() const noexcept { return bytes_; }
    [[nodiscard]] const std::vector<Swish::Block>& blocks() const noexcept { return blocks_; }
    [[nodiscard]] std::string_view exactGameId() const noexcept { return "legends_arceus_switch"; }
    [[nodiscard]] uint8_t partyCount() const noexcept { return partyCount_; }
    [[nodiscard]] Record at(SlotRef slot) const;
    [[nodiscard]] const Swish::Block& myStatus() const noexcept { return blocks_[myStatus_]; }
    [[nodiscard]] size_t partyIndex() const noexcept { return party_; }
    [[nodiscard]] size_t boxIndex() const noexcept { return box_; }

private:
    Gen8aSave() = default;
    std::vector<uint8_t> bytes_;
    std::vector<Swish::Block> blocks_;
    size_t myStatus_ = 0, party_ = 0, box_ = 0;
    uint8_t partyCount_ = 0;
};

enum class ChangeKind : uint8_t { Edit, Clone, Remove };
struct StagedChange {
    SlotRef slot;
    ChangeKind kind = ChangeKind::Edit;
    std::optional<SlotRef> cloneSource;
    std::vector<uint8_t> originalPlain, stagedPlain;
    std::vector<std::string> notes;
};
struct Gen8aExportRequest {
    std::string rootDirectory, sourcePath, timestamp, applicationSha = "unknown";
};

class Gen8aStagedSession {
public:
    explicit Gen8aStagedSession(Gen8aSave source) : source_(std::move(source)) {}
    [[nodiscard]] const Gen8aSave& source() const noexcept { return source_; }
    [[nodiscard]] std::optional<PA8::Pokemon8a> current(SlotRef slot) const;
    [[nodiscard]] bool stageValue(SlotRef slot, Field field, uint32_t value, std::string* error = nullptr);
    [[nodiscard]] bool stageText(SlotRef slot, Field field, std::string_view text, std::string* error = nullptr);
    [[nodiscard]] bool cloneToBox(SlotRef from, SlotRef to, std::string* error = nullptr);
    [[nodiscard]] bool removeFromBox(SlotRef at, std::string* error = nullptr);
    bool undo(SlotRef slot) noexcept { return entries_.erase(slot) != 0; }
    [[nodiscard]] bool isStaged(SlotRef slot) const noexcept { return entries_.count(slot) != 0; }
    struct EntrySnapshot {
        bool present = false;
        ChangeKind kind = ChangeKind::Edit;
        std::optional<PA8::Pokemon8a> record;
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
    [[nodiscard]] Common::VerifiedExportResult exportWorkingCopy(const Gen8aExportRequest& request) const;
    static constexpr std::string_view kCreateReason =
        "Legends: Arceus Create is unsupported: EC/PID, personal data, met, Alpha/Noble, move mastery, size and "
        "cached stats cannot be derived verifiably";

private:
    struct Entry {
        ChangeKind kind = ChangeKind::Edit;
        PA8::Pokemon8a record;
        std::optional<SlotRef> cloneSource;
        std::vector<std::string> notes;
    };
    void commitEdit(SlotRef slot, PA8::Pokemon8a next, std::string note);
    Gen8aSave source_;
    std::map<SlotRef, Entry> entries_;
};

[[nodiscard]] bool verifyWorkingCopy(const Gen8aSave& source, std::span<const uint8_t> edited,
                                     const std::vector<StagedChange>& changes, std::string* error = nullptr);

} // namespace PokeVault::Integration::Gen8a

#endif
