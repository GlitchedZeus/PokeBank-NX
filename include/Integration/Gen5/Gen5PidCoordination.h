#ifndef POKEBANK_GEN5_PID_COORDINATION_H
#define POKEBANK_GEN5_PID_COORDINATION_H

#include <cstdint>
#include <optional>

namespace PokeVault::Integration::Gen5 {

// Native PID relationships of a Generation V PK5. Every rule below is taken
// from PKHeX 6501f0ab46e8f8ca048539dbaf8cae8cb104e722:
//   PKM/PK5.cs          Gender is stored at 0x40 bits 1-2, Nature at 0x41 (NOT
//                       PID-derived in Gen V), HiddenAbility at 0x42 bit 0,
//                       PSV = ((PID>>16) ^ (PID&0xFFFF)) >> 3, TSV = (TID^SID) >> 3,
//                       and the record is shiny when PSV == TSV.
//   PKM/PKM.cs          PIDAbility = (PID >> 16) & 1 for Gen V origins and
//                       PID & 1 for Gen III/IV origins.
//   PKM/Util/EntityGender.cs  GetFromPID: (PID & 0xFF) < ratio ? Female : Male;
//                       ratios 0 / 254 / 255 are fixed male / female / genderless.
//   PKM/Util/EntityPID.cs     GetRandomPID (Gen V): a replacement PID keeps the
//                       ability bit (0x0001_0000) and the PID-derived gender.
// PokeBank NX never rerolls randomly. The procedures here change the fewest PID
// bits that give the requested trait, keep every other PID-linked trait, and
// are deterministic so Keep and Review replay to identical bytes. They prove
// only native consistency, never encounter or RNG legality.

// Black = 21, White = 20, Black 2 = 23, White 2 = 22 (PKHeX GameVersion).
[[nodiscard]] constexpr bool gen5Origin5(uint8_t version) noexcept {
    return version >= 20 && version <= 23;
}
// Gen III / IV origins (PKHeX GameVersion S R E FR LG HG SS D P Pt CXD), whose
// nature is PID % 25 and whose ability is PID bit 0.
[[nodiscard]] constexpr bool earlierGenerationOrigin5(uint8_t version) noexcept {
    return (version >= 1 && version <= 5) || version == 7 || version == 8 ||
           (version >= 10 && version <= 12) || version == 15;
}
[[nodiscard]] constexpr bool variableGenderRatio5(uint8_t ratio) noexcept {
    return ratio != 0 && ratio != 254 && ratio != 255;
}
[[nodiscard]] constexpr uint8_t genderFromPid5(uint32_t pid, uint8_t ratio) noexcept {
    if (ratio == 255) return 2;
    if (ratio == 254) return 1;
    if (ratio == 0) return 0;
    return (pid & 0xFFu) < ratio ? 1 : 0;
}
[[nodiscard]] constexpr uint32_t shinyXor5(uint32_t pid, uint16_t tid, uint16_t sid) noexcept {
    return static_cast<uint32_t>(tid ^ sid ^ static_cast<uint16_t>(pid) ^
                                 static_cast<uint16_t>(pid >> 16));
}
[[nodiscard]] constexpr bool shinyPid5(uint32_t pid, uint16_t tid, uint16_t sid) noexcept {
    return shinyXor5(pid, tid, sid) < 8;
}
[[nodiscard]] constexpr uint32_t abilityBit5(uint32_t pid) noexcept { return (pid >> 16) & 1u; }

// Changes only PID bits 17..31 (bits 1..15 of the high half), so the low half
// (gender) and bit 16 (ability) are untouched.
[[nodiscard]] constexpr uint32_t withShiny5(uint32_t pid, uint16_t tid, uint16_t sid,
                                            bool shiny) noexcept {
    if (shinyPid5(pid, tid, sid) == shiny) return pid;
    const uint32_t low = pid & 0xFFFFu;
    uint32_t high = pid >> 16;
    if (shiny) {
        // XOR of the result is 0 or 1 (< 8): shiny, with the ability bit kept.
        high = ((static_cast<uint32_t>(tid ^ sid) ^ low) & 0xFFFEu) | (high & 1u);
    } else {
        // The current XOR is < 8; flipping bit 3 of the high half makes it 8..15.
        high ^= 0x0008u;
    }
    return (high << 16) | low;
}

// Gender from the PID low byte. The nearest low byte inside the requested
// gender's range is used, then the previous shiny state is restored through
// the high half. Fixed ratios accept only their own gender.
[[nodiscard]] constexpr std::optional<uint32_t> withGender5(
    uint32_t pid, uint8_t ratio, uint8_t gender, uint16_t tid, uint16_t sid) noexcept {
    if (gender > 2) return std::nullopt;
    if (!variableGenderRatio5(ratio))
        return genderFromPid5(pid, ratio) == gender ? std::optional<uint32_t>(pid) : std::nullopt;
    if (gender == 2) return std::nullopt;
    if (genderFromPid5(pid, ratio) == gender) return pid;
    const bool shiny = shinyPid5(pid, tid, sid);
    const uint32_t lowByte = gender == 1 ? static_cast<uint32_t>(ratio) - 1u : ratio;
    const uint32_t next = (pid & ~0xFFu) | lowByte;
    return withShiny5(next, tid, sid, shiny);
}

// Normal ability slot 0/1 lives in PID bit 16; flipping it changes the XOR by
// one, which never moves it across the shiny threshold of 8.
[[nodiscard]] constexpr uint32_t withAbilityBit5(uint32_t pid, uint32_t bit) noexcept {
    return (pid & ~(1u << 16)) | ((bit & 1u) << 16);
}

// After a TID / SID change, keep the record's previous shiny state.
[[nodiscard]] constexpr uint32_t keepShinyForIds5(uint32_t pid, uint16_t oldTid, uint16_t oldSid,
                                                  uint16_t newTid, uint16_t newSid) noexcept {
    return withShiny5(pid, newTid, newSid, shinyPid5(pid, oldTid, oldSid));
}

static_assert(genderFromPid5(0x00000010u, 31) == 1 && genderFromPid5(0x0000001Fu, 31) == 0);
static_assert(shinyPid5(withShiny5(0x12345678u, 1234, 5678, true), 1234, 5678));
static_assert(!shinyPid5(withShiny5(withShiny5(0x12345678u, 1, 2, true), 1, 2, false), 1, 2));
static_assert(abilityBit5(withShiny5(0x00010000u, 7, 9, true)) == 1);
static_assert((withShiny5(0xABCD1234u, 7, 9, true) & 0xFFFFu) == 0x1234u);
static_assert(genderFromPid5(*withGender5(0x000000FFu, 127, 1, 0, 0), 127) == 1);
static_assert(genderFromPid5(*withGender5(0x00000000u, 127, 0, 0, 0), 127) == 0);
static_assert(!withGender5(0u, 255, 0, 0, 0) && withGender5(0u, 255, 2, 0, 0));

} // namespace PokeVault::Integration::Gen5
#endif
