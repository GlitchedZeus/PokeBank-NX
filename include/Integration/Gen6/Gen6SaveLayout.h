#ifndef POKEBANK_GEN6_SAVE_LAYOUT_H
#define POKEBANK_GEN6_SAVE_LAYOUT_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

// SAV6 geometry for X/Y and Omega Ruby/Alpha Sapphire retail saves (the
// 3DS save archive's "main" file). Sources, pinned:
//   PKHeX 6501f0ab46e8f8ca048539dbaf8cae8cb104e722
//     Saves/Util/SaveUtil.cs              SIZE_G6XY=0x65600, SIZE_G6ORAS=0x76000,
//                                         SIZE_G6ORASDEMO=0x5A00, HasSaveFooterBEEF
//                                         (u32 0x42454546 at size-0x1F0)
//     Saves/Access/SaveBlockAccessor6XY.cs / SaveBlockAccessor6AO.cs
//                                         BlockMetadataOffset = size-0x200, block tables
//     Saves/Blocks/BlockInfo3DS.cs        checksum u16 at metadata+0x14+id*8+6;
//                                         BlockInfo6 uses Checksums.CRC16_CCITT
//     Saves/SAV6.cs, SAV6XY.cs, SAV6AO.cs Party=0x14200 (count at Party+6*0x104),
//                                         Box=0x22600 (XY) / 0x33000 (ORAS), 31 boxes x 30
//     Saves/Substructures/Gen6/MyStatus6.cs  TID 0, SID 2, Game 4, Gender 5,
//                                         Language 0x2D, OT name 0x48 (0x1A bytes)
//     Game/Enums/GameVersion.cs           X=24, Y=25, AS=26, OR=27
//   PKSM-Core aa22d7a4 include/sav/SavXY.hpp / SavORAS.hpp (chkofs/chklen; csoff 0x6541A /
//   0x75E1A) and source/sav/Sav6.cpp -- an independent table that must agree (tested).
namespace PokeVault::Integration::Gen6::Layout {

enum class Family : uint8_t { XY, ORAS };

struct Block {
    uint32_t offset;
    uint32_t length;
};

inline constexpr std::array<Block, 55> XYBlocks = {{
    {0x00000, 0x002C8}, {0x00400, 0x00B88}, {0x01000, 0x0002C}, {0x01200, 0x00038},
    {0x01400, 0x00150}, {0x01600, 0x00004}, {0x01800, 0x00008}, {0x01A00, 0x001C0},
    {0x01C00, 0x000BE}, {0x01E00, 0x00024}, {0x02000, 0x02100}, {0x04200, 0x00140},
    {0x04400, 0x00440}, {0x04A00, 0x00574}, {0x05000, 0x04E28}, {0x0A000, 0x04E28},
    {0x0F000, 0x04E28}, {0x14000, 0x00170}, {0x14200, 0x0061C}, {0x14A00, 0x00504},
    {0x15000, 0x006A0}, {0x15800, 0x00644}, {0x16000, 0x00104}, {0x16200, 0x00004},
    {0x16400, 0x00420}, {0x16A00, 0x00064}, {0x16C00, 0x003F0}, {0x17000, 0x0070C},
    {0x17800, 0x00180}, {0x17A00, 0x00004}, {0x17C00, 0x0000C}, {0x17E00, 0x00048},
    {0x18000, 0x00054}, {0x18200, 0x00644}, {0x18A00, 0x005C8}, {0x19000, 0x002F8},
    {0x19400, 0x01B40}, {0x1B000, 0x001F4}, {0x1B200, 0x001F0}, {0x1B400, 0x00216},
    {0x1B800, 0x00390}, {0x1BC00, 0x01A90}, {0x1D800, 0x00308}, {0x1DC00, 0x00618},
    {0x1E400, 0x0025C}, {0x1E800, 0x00834}, {0x1F200, 0x00318}, {0x1F600, 0x007D0},
    {0x1FE00, 0x00C48}, {0x20C00, 0x00078}, {0x20E00, 0x00200}, {0x21000, 0x00C84},
    {0x21E00, 0x00628}, {0x22600, 0x34AD0}, {0x57200, 0x0E058},
}};

inline constexpr std::array<Block, 58> ORASBlocks = {{
    {0x00000, 0x002C8}, {0x00400, 0x00B90}, {0x01000, 0x0002C}, {0x01200, 0x00038},
    {0x01400, 0x00150}, {0x01600, 0x00004}, {0x01800, 0x00008}, {0x01A00, 0x001C0},
    {0x01C00, 0x000BE}, {0x01E00, 0x00024}, {0x02000, 0x02100}, {0x04200, 0x00130},
    {0x04400, 0x00440}, {0x04A00, 0x00574}, {0x05000, 0x04E28}, {0x0A000, 0x04E28},
    {0x0F000, 0x04E28}, {0x14000, 0x00170}, {0x14200, 0x0061C}, {0x14A00, 0x00504},
    {0x15000, 0x011CC}, {0x16200, 0x00644}, {0x16A00, 0x00104}, {0x16C00, 0x00004},
    {0x16E00, 0x00420}, {0x17400, 0x00064}, {0x17600, 0x003F0}, {0x17A00, 0x0070C},
    {0x18200, 0x00180}, {0x18400, 0x00004}, {0x18600, 0x0000C}, {0x18800, 0x00048},
    {0x18A00, 0x00054}, {0x18C00, 0x00644}, {0x19400, 0x005C8}, {0x19A00, 0x002F8},
    {0x19E00, 0x01B40}, {0x1BA00, 0x001F4}, {0x1BC00, 0x003E0}, {0x1C000, 0x00216},
    {0x1C400, 0x00640}, {0x1CC00, 0x01A90}, {0x1E800, 0x00400}, {0x1EC00, 0x00618},
    {0x1F400, 0x0025C}, {0x1F800, 0x00834}, {0x20200, 0x00318}, {0x20600, 0x007D0},
    {0x20E00, 0x00C48}, {0x21C00, 0x00078}, {0x21E00, 0x00200}, {0x22000, 0x00C84},
    {0x22E00, 0x00628}, {0x23600, 0x00400}, {0x23A00, 0x07AD0}, {0x2B600, 0x078B0},
    {0x33000, 0x34AD0}, {0x67C00, 0x0E058},
}};

inline constexpr size_t XYSize = 0x65600;
inline constexpr size_t ORASSize = 0x76000;
// The ORAS Special Demo save is a different, explicitly unsupported family.
inline constexpr size_t ORASDemoSize = 0x5A00;

inline constexpr uint32_t BeefMagic = 0x42454546;  // PKHeX "BEEF" footer magic (read as a little-endian u32)
inline constexpr size_t MetadataFromEnd = 0x200;
inline constexpr size_t MagicWithinMetadata = 0x10;
inline constexpr size_t EntriesWithinMetadata = 0x14;
inline constexpr size_t EntrySize = 8;   // u32 length, u16 id, u16 checksum
inline constexpr size_t ChecksumWithinEntry = 6;

inline constexpr size_t MyStatusOffset = 0x14000;   // block 17 in both families
inline constexpr size_t MyStatusBlock = 17;
inline constexpr size_t PlayTimeOffset = 0x01800;   // block 6
inline constexpr size_t PartyOffset = 0x14200;      // block 18
inline constexpr size_t PartyBlock = 18;
inline constexpr size_t PartySlots = 6;
inline constexpr size_t BoxCount = 31;
inline constexpr size_t BoxSlots = 30;

inline constexpr uint8_t VersionX = 24;
inline constexpr uint8_t VersionY = 25;
inline constexpr uint8_t VersionAlphaSapphire = 26;
inline constexpr uint8_t VersionOmegaRuby = 27;

[[nodiscard]] constexpr size_t saveSize(Family f) noexcept { return f == Family::XY ? XYSize : ORASSize; }
[[nodiscard]] constexpr size_t metadataOffset(Family f) noexcept { return saveSize(f) - MetadataFromEnd; }
[[nodiscard]] constexpr size_t boxOffset(Family f) noexcept { return f == Family::XY ? 0x22600 : 0x33000; }
[[nodiscard]] constexpr size_t boxBlock(Family f) noexcept { return f == Family::XY ? 53 : 56; }
[[nodiscard]] inline std::span<const Block> blocks(Family f) noexcept {
    return f == Family::XY ? std::span<const Block>(XYBlocks) : std::span<const Block>(ORASBlocks);
}
[[nodiscard]] constexpr size_t checksumOffset(Family f, size_t blockId) noexcept {
    return metadataOffset(f) + EntriesWithinMetadata + blockId * EntrySize + ChecksumWithinEntry;
}

// Exact title identity comes only from the MyStatus game byte of a
// checksum-valid save of the matching size, never from a path or file name.
[[nodiscard]] constexpr std::string_view gameIdForVersion(uint8_t version) noexcept {
    switch (version) {
        case VersionX: return "x_3ds";
        case VersionY: return "y_3ds";
        case VersionAlphaSapphire: return "alphasapphire_3ds";
        case VersionOmegaRuby: return "omegaruby_3ds";
        default: return {};
    }
}
[[nodiscard]] constexpr bool versionMatchesFamily(uint8_t version, Family f) noexcept {
    return f == Family::XY ? (version == VersionX || version == VersionY)
                           : (version == VersionAlphaSapphire || version == VersionOmegaRuby);
}
[[nodiscard]] constexpr bool isGen6GameId(std::string_view id) noexcept {
    return id == "x_3ds" || id == "y_3ds" || id == "omegaruby_3ds" || id == "alphasapphire_3ds";
}
[[nodiscard]] constexpr Family familyForGameId(std::string_view id) noexcept {
    return (id == "x_3ds" || id == "y_3ds") ? Family::XY : Family::ORAS;
}

// 3DS title IDs, cited from FlagBrew/PKSM 9e18c8068ede315f73a99f4c834819630393b5c5
// 3ds/source/titles/loader.cpp (X/Y/OR/AS) and common/source/ConfigurationFile.cpp
// (version 24..27 -> title id). Used only to cross-check provider evidence.
inline constexpr uint64_t TitleIdX = 0x0004000000055D00ull;
inline constexpr uint64_t TitleIdY = 0x0004000000055E00ull;
inline constexpr uint64_t TitleIdOmegaRuby = 0x000400000011C400ull;
inline constexpr uint64_t TitleIdAlphaSapphire = 0x000400000011C500ull;

[[nodiscard]] constexpr uint64_t titleIdForGameId(std::string_view id) noexcept {
    if (id == "x_3ds") return TitleIdX;
    if (id == "y_3ds") return TitleIdY;
    if (id == "omegaruby_3ds") return TitleIdOmegaRuby;
    if (id == "alphasapphire_3ds") return TitleIdAlphaSapphire;
    return 0;
}
[[nodiscard]] constexpr std::string_view gameIdForTitleId(uint64_t titleId) noexcept {
    switch (titleId) {
        case TitleIdX: return "x_3ds";
        case TitleIdY: return "y_3ds";
        case TitleIdOmegaRuby: return "omegaruby_3ds";
        case TitleIdAlphaSapphire: return "alphasapphire_3ds";
        default: return {};
    }
}

} // namespace PokeVault::Integration::Gen6::Layout

#endif
