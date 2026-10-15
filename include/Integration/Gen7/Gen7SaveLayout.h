#ifndef POKEBANK_GEN7_SAVE_LAYOUT_H
#define POKEBANK_GEN7_SAVE_LAYOUT_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

// SAV7 geometry for Sun/Moon and Ultra Sun/Ultra Moon (3DS archive "main").
// Let's Go Pikachu/Eevee is a separate save family and is NOT handled here.
// Sources, pinned:
//   PKHeX 6501f0ab46e8f8ca048539dbaf8cae8cb104e722
//     Saves/Util/SaveUtil.cs            SIZE_G7SM=0x6BE00, SIZE_G7USUM=0x6CC00, BEEF footer
//     Saves/Access/SaveBlockAccessor7SM.cs / SaveBlockAccessor7USUM.cs   block tables
//     Saves/Blocks/BlockInfo3DS.cs      checksum u16 at metadata+0x14+id*8+6; BlockInfo7 uses
//                                       Checksums.CRC16Invert
//     Saves/SAV7.cs, SAV7SM.cs, SAV7USUM.cs  Party = block 4, Box = block 14, 32 boxes x 30,
//                                       MemeCrypto block 36, signature at +0x100 (0x80 bytes),
//                                       cleared before checksums are validated (ClearMemeCrypto)
//     Saves/Encryption/MemeCrypto/MemeCrypto.cs  SignInPlace: SHA256 over the checksum table
//                                       [size-0x200, +0x140 (SM) / +0x150 (USUM)]
//     Saves/Substructures/Gen7/MyStatus7.cs  TID 0, SID 2, Game 4, Gender 5, Language 0x35,
//                                       OT name 0x38 (0x1A bytes)
//     Game/Enums/GameVersion.cs         SN=30, MN=31, US=32, UM=33
//   PKSM-Core aa22d7a4 include/sav/SavSUMO.hpp / SavUSUM.hpp (chkofs/chklen), source/sav/
//   SavSUMO.cpp / SavUSUM.cpp (csoff 0x6BC1A / 0x6CA1A, memecrypto offsets 0x6BB00 / 0x6C100).
namespace PokeVault::Integration::Gen7::Layout {

enum class Family : uint8_t { SunMoon, UltraSunUltraMoon };

struct Block {
    uint32_t offset;
    uint32_t length;
};

inline constexpr std::array<Block, 37> SMBlocks = {{
    {0x00000, 0x00DE0}, {0x00E00, 0x0007C}, {0x01000, 0x00014}, {0x01200, 0x000C0},
    {0x01400, 0x0061C}, {0x01C00, 0x00E00}, {0x02A00, 0x00F78}, {0x03A00, 0x00228},
    {0x03E00, 0x00104}, {0x04000, 0x00200}, {0x04200, 0x00020}, {0x04400, 0x00004},
    {0x04600, 0x00058}, {0x04800, 0x005E6}, {0x04E00, 0x36600}, {0x3B400, 0x0572C},
    {0x40C00, 0x00008}, {0x40E00, 0x01080}, {0x42000, 0x01A08}, {0x43C00, 0x06408},
    {0x4A200, 0x06408}, {0x50800, 0x03998}, {0x54200, 0x00100}, {0x54400, 0x00100},
    {0x54600, 0x10528}, {0x64C00, 0x00204}, {0x65000, 0x00B60}, {0x65C00, 0x03F50},
    {0x69C00, 0x00358}, {0x6A000, 0x00728}, {0x6A800, 0x00200}, {0x6AA00, 0x00718},
    {0x6B200, 0x001FC}, {0x6B400, 0x00200}, {0x6B600, 0x00120}, {0x6B800, 0x001C8},
    {0x6BA00, 0x00200},
}};

inline constexpr std::array<Block, 39> USUMBlocks = {{
    {0x00000, 0x00E28}, {0x01000, 0x0007C}, {0x01200, 0x00014}, {0x01400, 0x000C0},
    {0x01600, 0x0061C}, {0x01E00, 0x00E00}, {0x02C00, 0x00F78}, {0x03C00, 0x00228},
    {0x04000, 0x0030C}, {0x04400, 0x001FC}, {0x04600, 0x0004C}, {0x04800, 0x00004},
    {0x04A00, 0x00058}, {0x04C00, 0x005E6}, {0x05200, 0x36600}, {0x3B800, 0x0572C},
    {0x41000, 0x00008}, {0x41200, 0x01218}, {0x42600, 0x01A08}, {0x44200, 0x06408},
    {0x4A800, 0x06408}, {0x50E00, 0x03998}, {0x54800, 0x00100}, {0x54A00, 0x00100},
    {0x54C00, 0x10528}, {0x65200, 0x00204}, {0x65600, 0x00B60}, {0x66200, 0x03F50},
    {0x6A200, 0x00358}, {0x6A600, 0x00728}, {0x6AE00, 0x00200}, {0x6B000, 0x00718},
    {0x6B800, 0x001FC}, {0x6BA00, 0x00200}, {0x6BC00, 0x00120}, {0x6BE00, 0x001C8},
    {0x6C000, 0x00200}, {0x6C200, 0x0039C}, {0x6C600, 0x00400},
}};

inline constexpr size_t SMSize = 0x6BE00;
inline constexpr size_t USUMSize = 0x6CC00;
inline constexpr uint32_t BeefMagic = 0x42454546;
inline constexpr size_t MetadataFromEnd = 0x200;
inline constexpr size_t MagicWithinMetadata = 0x10;
inline constexpr size_t EntriesWithinMetadata = 0x14;
inline constexpr size_t EntrySize = 8;
inline constexpr size_t ChecksumWithinEntry = 6;

inline constexpr size_t MyStatusBlock = 3;
inline constexpr size_t PartyBlock = 4;
inline constexpr size_t BoxBlock = 14;
inline constexpr size_t PlayTimeBlock = 16;
inline constexpr size_t MemeCryptoBlock = 36;
inline constexpr size_t SignatureWithinBlock = 0x100;
inline constexpr size_t SignatureLength = 0x80;
inline constexpr size_t PartySlots = 6;
inline constexpr size_t BoxCount = 32;
inline constexpr size_t BoxSlots = 30;
inline constexpr size_t PartyRecordSize = 0x104;
inline constexpr size_t BoxRecordSize = 0xE8;

inline constexpr uint8_t VersionSun = 30;
inline constexpr uint8_t VersionMoon = 31;
inline constexpr uint8_t VersionUltraSun = 32;
inline constexpr uint8_t VersionUltraMoon = 33;

[[nodiscard]] constexpr size_t saveSize(Family f) noexcept { return f == Family::SunMoon ? SMSize : USUMSize; }
[[nodiscard]] constexpr size_t metadataOffset(Family f) noexcept { return saveSize(f) - MetadataFromEnd; }
[[nodiscard]] inline std::span<const Block> blocks(Family f) noexcept {
    return f == Family::SunMoon ? std::span<const Block>(SMBlocks) : std::span<const Block>(USUMBlocks);
}
[[nodiscard]] constexpr size_t checksumOffset(Family f, size_t blockId) noexcept {
    return metadataOffset(f) + EntriesWithinMetadata + blockId * EntrySize + ChecksumWithinEntry;
}
// MemeCrypto.SignInPlace: SHA256 input = checksum table [size-0x200, +0x140/+0x150).
[[nodiscard]] constexpr size_t signedTableLength(Family f) noexcept {
    return f == Family::SunMoon ? 0x140 : 0x150;
}
[[nodiscard]] inline size_t signatureOffset(Family f) noexcept {
    return blocks(f)[MemeCryptoBlock].offset + SignatureWithinBlock;
}

[[nodiscard]] constexpr std::string_view gameIdForVersion(uint8_t version) noexcept {
    switch (version) {
        case VersionSun: return "sun_3ds";
        case VersionMoon: return "moon_3ds";
        case VersionUltraSun: return "ultrasun_3ds";
        case VersionUltraMoon: return "ultramoon_3ds";
        default: return {};
    }
}
[[nodiscard]] constexpr bool versionMatchesFamily(uint8_t version, Family f) noexcept {
    return f == Family::SunMoon ? (version == VersionSun || version == VersionMoon)
                                : (version == VersionUltraSun || version == VersionUltraMoon);
}
[[nodiscard]] constexpr bool isGen7GameId(std::string_view id) noexcept {
    return id == "sun_3ds" || id == "moon_3ds" || id == "ultrasun_3ds" || id == "ultramoon_3ds";
}

// FlagBrew/PKSM 9e18c8068ede315f73a99f4c834819630393b5c5 3ds/source/titles/loader.cpp (SN/MN/US/UM).
inline constexpr uint64_t TitleIdSun = 0x0004000000164800ull;
inline constexpr uint64_t TitleIdMoon = 0x0004000000175E00ull;
inline constexpr uint64_t TitleIdUltraSun = 0x00040000001B5000ull;
inline constexpr uint64_t TitleIdUltraMoon = 0x00040000001B5100ull;
[[nodiscard]] constexpr uint64_t titleIdForGameId(std::string_view id) noexcept {
    if (id == "sun_3ds") return TitleIdSun;
    if (id == "moon_3ds") return TitleIdMoon;
    if (id == "ultrasun_3ds") return TitleIdUltraSun;
    if (id == "ultramoon_3ds") return TitleIdUltraMoon;
    return 0;
}

} // namespace PokeVault::Integration::Gen7::Layout

#endif
