#ifndef POKEBANK_GEN8_SAVE_LAYOUT_H
#define POKEBANK_GEN8_SAVE_LAYOUT_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

// Sword/Shield `main` geometry. Every constant cites the pinned references:
//   PKHeX 6501f0ab46e8f8ca048539dbaf8cae8cb104e722
//     PKHeX.Core/Saves/Util/SaveUtil.cs              SIZE_G8SWSH* (IsSizeGen8SWSH)
//     PKHeX.Core/Saves/Access/SaveBlockAccessor8SWSH.cs  block keys
//     PKHeX.Core/Saves/SAV8SWSH.cs                   party format in boxes, 30 slots per box
//     PKHeX.Core/Saves/Substructures/Gen8/SWSH/BoxLayout8.cs  BoxCount = 32
//     PKHeX.Core/Saves/Substructures/Gen8/SWSH/Party8.cs      count byte at 6 * SIZE_8PARTY
//     PKHeX.Core/Saves/Substructures/Gen8/SWSH/MyStatus8.cs   trainer offsets
//     PKHeX.Core/Saves/Substructures/Gen8/SWSH/TeamIndexes8.cs battle team slots / locks
//     PKHeX.Core/Game/Enums/GameVersion.cs           SW = 44, SH = 45
namespace PokeVault::Integration::Gen8::Layout {

// Every accepted SWSH `main` size, one per patch history (SaveUtil.IsSizeGen8SWSH).
inline constexpr std::array<size_t, 8> SwordShieldSizes = {
    0x1716B3,  // 1.0
    0x17195E,  // 1.0 -> 1.1
    0x180B19,  // 1.0 -> 1.1 -> 1.2
    0x180AD0,  // 1.0 -> 1.2
    0x1876B1,  // 1.0 -> 1.1 -> 1.2 -> 1.3
    0x187693,  // 1.0 -> 1.1 -> 1.3
    0x187668,  // 1.0 -> 1.2 -> 1.3
    0x18764A,  // 1.0 -> 1.3
};
[[nodiscard]] constexpr bool isSwordShieldSize(size_t size) noexcept {
    for (const size_t s : SwordShieldSizes) if (s == size) return true;
    return false;
}

// SaveBlockAccessor8SWSH block keys used by this lane.
inline constexpr uint32_t KeyMyStatus = 0xF25C070E;
inline constexpr uint32_t KeyParty = 0x2985FE5D;
inline constexpr uint32_t KeyBox = 0x0D66012C;
inline constexpr uint32_t KeyBoxLayout = 0x19722C89;
inline constexpr uint32_t KeyItem = 0x1177C2C4;
inline constexpr uint32_t KeyMisc = 0x1B882B09;
inline constexpr uint32_t KeyCurrentBox = 0x017C3CBB;
inline constexpr uint32_t KeyBoxesUnlocked = 0x71825204;
inline constexpr uint32_t KeyTeamIndexes = 0x33F39467;
inline constexpr uint32_t KeyTeamLocks = 0x605EBC30;

inline constexpr size_t PartySlots = 6;
inline constexpr size_t BoxCount = 32;
inline constexpr size_t BoxSlots = 30;
inline constexpr size_t RecordSize = 0x158;  // PokeCrypto.SIZE_8PARTY, also used in boxes
inline constexpr size_t PartyCountOffset = PartySlots * RecordSize;  // Party8.PartyCount
inline constexpr size_t PartyBlockMinimum = PartyCountOffset + 1;
inline constexpr size_t BoxBlockMinimum = BoxCount * BoxSlots * RecordSize;

// MyStatus8 offsets.
inline constexpr size_t MyStatusTid = 0xA0;
inline constexpr size_t MyStatusSid = 0xA2;
inline constexpr size_t MyStatusGame = 0xA4;
inline constexpr size_t MyStatusGender = 0xA5;
inline constexpr size_t MyStatusLanguage = 0xA7;
inline constexpr size_t MyStatusName = 0xB0;  // OriginalTrainerTrash, 0x1A bytes
inline constexpr size_t MyStatusNameBytes = 0x1A;
inline constexpr size_t MyStatusMinimum = MyStatusName + MyStatusNameBytes;

// TeamIndexes8: 6 teams x 6 signed u16 entries, (box << 8) | slot, negative = none.
inline constexpr size_t TeamCount = 6;
inline constexpr size_t TeamIndexesMinimum = TeamCount * 6 * 2;

inline constexpr uint8_t VersionSword = 44;
inline constexpr uint8_t VersionShield = 45;

[[nodiscard]] constexpr std::string_view gameIdForVersion(uint8_t version) noexcept {
    switch (version) {
        case VersionSword: return "sword_switch";
        case VersionShield: return "shield_switch";
        default: return {};
    }
}
[[nodiscard]] constexpr bool isGen8SwordShieldGameId(std::string_view id) noexcept {
    return id == "sword_switch" || id == "shield_switch";
}

} // namespace PokeVault::Integration::Gen8::Layout

#endif
