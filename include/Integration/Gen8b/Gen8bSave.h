#ifndef POKEBANK_GEN8B_SAVE_H
#define POKEBANK_GEN8B_SAVE_H

// Strict, read-only Brilliant Diamond / Shining Pearl save model (SAV8BS, one flat file).
//
// Every offset cites PKHeX 6501f0ab46e8f8ca048539dbaf8cae8cb104e722 (the reference the existing
// Trainer8BDSP reader already cites):
//   Saves/Util/SaveUtil.cs            SIZE_G8BDSP_0..3 = 0xE9828 / 0xEDC20 / 0xEED8C / 0xEF0A4, and the
//                                     u32 at offset 0 must be the matching Gem8Version (0x25/0x2C/0x32/0x34)
//   Saves/SAV8BS.cs                   Party8b @0x14098, BoxLayout8b @0x148AA (0x64A), Box[40] @0x14EF4
//                                     (party-format 0x158 records, 30 per box), MyStatus8b @0x79BB4,
//                                     MD5 of the whole file (hash zeroed) @0xE9818 for every revision
//   Substructures/Gen8/BS/Party8b.cs   count @ +6*0x158
//   Substructures/Gen8/BS/BoxLayout8b.cs  box names 0x22 each, battle teams @+0x5D4 (36 x int16,
//                                     (box << 8) | slot, negative = none), locked-team bits @+0x61C
//   Substructures/Gen8/BS/MyStatus8b.cs   OT 0x1A @+0, TID16 @+0x1C, SID16 @+0x1E, Male @+0x24, ROMCode @+0x2B
// PB8 shares the G8PKM (PK8) record layout, so the verified Gen VIII PK8 codec is reused.
// There is ONE save file per slot: nothing here chooses between paired files.

#include "Integration/Gen8/Gen8PK8.h"
#include "Integration/Gen8/Gen8Save.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace PokeVault::Integration::Gen8b {

namespace PK8 = Gen8::PK8;
using Gen8::SlotRecord;
using Gen8::SlotState;
using Gen8::TeamSlot;

namespace Layout {
struct Revision { size_t size; uint32_t version; const char* label; };
inline constexpr std::array<Revision, 4> Revisions = {{
    {0xE9828, 0x25, "1.0"}, {0xEDC20, 0x2C, "1.1"}, {0xEED8C, 0x32, "1.2"}, {0xEF0A4, 0x34, "1.3"},
}};
inline constexpr size_t HashOffset = 0xE9818, HashBytes = 16;
inline constexpr size_t Party = 0x14098, PartySlots = 6, PartyCount = Party + 6 * 0x158;
inline constexpr size_t BoxLayout = 0x148AA, BoxNameBytes = 0x22;
inline constexpr size_t TeamPositions = BoxLayout + 0x5D4, TeamCount = 6, TeamSlotCount = 6;
inline constexpr size_t LockedTeam = BoxLayout + 0x61C;
inline constexpr size_t Box = 0x14EF4, BoxCount = 40, BoxSlots = 30;
inline constexpr size_t RecordSize = 0x158;
inline constexpr size_t MyStatus = 0x79BB4, OtBytes = 0x1A, Tid = MyStatus + 0x1C, Sid = MyStatus + 0x1E;
inline constexpr size_t Male = MyStatus + 0x24, RomCode = MyStatus + 0x2B;

[[nodiscard]] constexpr size_t partyOffset(size_t slot) noexcept { return Party + slot * RecordSize; }
[[nodiscard]] constexpr size_t boxOffset(size_t box, size_t slot) noexcept { return Box + (box * BoxSlots + slot) * RecordSize; }
[[nodiscard]] constexpr std::string_view gameIdForRom(uint8_t rom) noexcept {
    return rom == 0 ? "brilliant_diamond_switch" : rom == 1 ? "shining_pearl_switch" : std::string_view{};
}
} // namespace Layout

struct TrainerFacts {
    std::u16string name;
    uint16_t tid = 0, sid = 0;
    uint8_t gender = 0;
};

class Gen8bSave {
public:
    [[nodiscard]] static std::optional<Gen8bSave> parse(std::span<const uint8_t> bytes,
                                                        std::string_view assignedExactGame = {},
                                                        std::string* error = nullptr);
    [[nodiscard]] static bool hashValid(std::span<const uint8_t> bytes);
    static void refreshHash(std::vector<uint8_t>& bytes);

    [[nodiscard]] std::span<const uint8_t> bytes() const noexcept { return bytes_; }
    [[nodiscard]] std::string_view exactGameId() const noexcept { return gameId_; }
    [[nodiscard]] std::string_view revision() const noexcept { return revision_; }
    [[nodiscard]] uint8_t partyCount() const noexcept { return partyCount_; }
    [[nodiscard]] SlotRecord party(size_t slot) const;
    [[nodiscard]] SlotRecord box(size_t box, size_t slot) const;
    [[nodiscard]] const std::optional<std::vector<TeamSlot>>& teams() const noexcept { return teams_; }
    [[nodiscard]] TrainerFacts trainer() const;
    [[nodiscard]] std::u16string boxName(size_t box) const;

private:
    Gen8bSave() = default;
    [[nodiscard]] static SlotRecord record(std::span<const uint8_t> encrypted);
    std::vector<uint8_t> bytes_;
    std::string gameId_;
    std::string revision_;
    uint8_t partyCount_ = 0;
    std::optional<std::vector<TeamSlot>> teams_;
};

} // namespace PokeVault::Integration::Gen8b

#endif
