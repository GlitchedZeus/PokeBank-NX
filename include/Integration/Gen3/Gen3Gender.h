#pragma once

#include <array>
#include <cstdint>

// Generation III gender from the PID (Home card / summaries).
//
// Gender ratio bytes for National Dex 0..386, generated verbatim from vendor/PKSM-Core (pinned
// aa22d7a4) personals/personal3, entry size 13, byte +0x8 (PKSM PersonalRSFRLGE::gender). This is
// the same raw byte as PKHeX PersonalInfo3 Gender: 0 = always male, 254 = always female,
// 255 = genderless, otherwise a PID low byte below the ratio means female (PKSM PK3::gender();
// PKHeX EntityGender.GetFromPIDAndRatio). test_gen3_gender cross-checks every row against the PKSM
// binary and PokeBank's PersonalInfo table.
namespace PokeVault::Integration::Gen3 {

inline constexpr std::array<uint8_t, 387> kGen3GenderRatio{{
      0,  31,  31,  31,  31,  31,  31,  31,  31,  31, 127, 127, 127, 127, 127, 127,
    127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 254, 254, 254,
      0,   0,   0, 191, 191, 191, 191, 191, 191, 127, 127, 127, 127, 127, 127, 127,
    127, 127, 127, 127, 127, 127, 127, 127, 127, 127,  63,  63, 127, 127, 127,  63,
     63,  63,  63,  63,  63, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
    127, 255, 255, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
    127, 127, 127, 127, 255, 255, 127, 127, 127, 127,   0,   0, 127, 127, 127, 127,
    127, 254, 127, 254, 127, 127, 127, 127, 255, 255, 127, 127, 254,  63,  63, 127,
      0, 127, 127, 127, 255,  31,  31,  31,  31, 255,  31,  31,  31,  31,  31,  31,
    255, 255, 255, 127, 127, 127, 255, 255,  31,  31,  31,  31,  31,  31,  31,  31,
     31, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 191, 191,  31,
     31, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
    127, 127, 127, 127,  31,  31, 127, 127, 127, 255, 127, 127, 127, 127, 127, 127,
    127, 191, 191, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 191, 127,
    127, 127, 127, 127, 127, 127, 127, 127, 127, 255, 127, 127,   0,   0, 254,  63,
     63, 254, 254, 255, 255, 255, 127, 127, 127, 255, 255, 255,  31,  31,  31,  31,
     31,  31,  31,  31,  31, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
    127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
    127, 127, 127, 127, 255, 127, 127, 127,  63,  63, 191, 127, 191, 191, 127, 127,
    127, 127, 127, 127, 127, 127, 127, 127, 127,   0, 254, 127, 127, 127, 127, 127,
    127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
    127, 255, 255, 127, 127, 127, 127, 255, 255,  31,  31,  31,  31, 127, 127, 127,
    127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
    127,  31, 191, 127, 127, 127, 255, 255, 255, 255, 255, 255, 254,   0, 255, 255,
    255, 255, 255,
}};

enum class Gen3Gender : uint8_t { Male = 0, Female = 1, Genderless = 2, Unknown = 3 };

// Unknown only for a species outside 1..386 (no Gen III data); never guessed.
[[nodiscard]] constexpr Gen3Gender gen3GenderFromPid(uint16_t species, uint32_t pid) noexcept {
    if (species == 0 || species >= kGen3GenderRatio.size()) return Gen3Gender::Unknown;
    const uint8_t ratio = kGen3GenderRatio[species];
    if (ratio == 0) return Gen3Gender::Male;
    if (ratio == 254) return Gen3Gender::Female;
    if (ratio == 255) return Gen3Gender::Genderless;
    return (pid & 0xFFu) < ratio ? Gen3Gender::Female : Gen3Gender::Male;
}

} // namespace PokeVault::Integration::Gen3
