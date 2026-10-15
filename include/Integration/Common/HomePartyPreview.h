#ifndef POKEBANK_COMMON_HOME_PARTY_PREVIEW_H
#define POKEBANK_COMMON_HOME_PARTY_PREVIEW_H

#include "Conversion/Gen3PidSearch.h"
#include "Integration/Gen1/Gen1ReadOnlySave.h"
#include "Integration/Gen2/Gen2PersonalData.h"
#include "Integration/Gen2/Gen2ReadOnlySave.h"
#include "Integration/Gen3/PKSMGen3Adapter.h"
#include "Pokemon/Pokemon4ReadOnly.h"
#include "Source/HomeCardSnapshot.h"

#include <cstdint>

// ONE Home party-icon rule for Gen I-V: every generation fills the same
// species / level / form / shiny / gender fields from its own strictly
// parsed record, using only what that format defines. Nothing here reads or
// writes a save; each adapter takes an already validated record.
namespace PokeVault::Integration::HomePartyPreview {

using Slot = Source::HomeCardPartySlot;

// Gen I: no shiny, gender or forms exist in the format.
[[nodiscard]] inline Slot fromGen1(const Gen1::PokemonRecord& p) noexcept {
    return {p.species, p.level, 0, false, Source::HomeCardPartySlot::GenderUnknown};
}

// Gen II: shiny and the Unown letter come from the DVs (Gen2ReadOnlySave),
// gender from the Attack DV against the Gen II ratio (Gen2PersonalData).
[[nodiscard]] inline Slot fromGen2(const Gen2::PokemonRecord& p) noexcept {
    const auto gender = Gen2::genderFromAttackDV(p.species, p.dvs[1]);
    return {p.species, p.level, p.form, p.shiny,
            gender == Gen2::PokemonGender::Unknown ? Source::HomeCardPartySlot::GenderUnknown
                                                   : static_cast<uint8_t>(gender)};
}

// Gen III: PKHeX 6501f0ab PK3.cs, Stat_Level = Data[0x54] in the unencrypted
// 100-byte party extension; boxed (80-byte) records have no stored level.
// Shiny: (TID ^ SID ^ PIDhi ^ PIDlo) < 8. Unown letter: EntityPID
// GetUnownForm3 (Conversion::Gen3PidSearch::unownForm). Gen III gender needs a
// Gen III personal table that this adapter does not expose, so it stays unknown.
inline constexpr std::size_t kGen3PartySize = 100;
inline constexpr std::size_t kGen3PartyLevelOffset = 0x54;
inline constexpr uint16_t kUnown = 201;
[[nodiscard]] inline bool gen3Shiny(uint32_t pid, uint16_t tid, uint16_t sid) noexcept {
    return (tid ^ sid ^ (pid >> 16) ^ (pid & 0xFFFFu)) < 8u;
}
[[nodiscard]] inline Slot fromGen3(const Gen3::PokemonRecord& p) noexcept {
    const uint8_t level = p.originalBytes.size() == kGen3PartySize
        ? p.originalBytes[kGen3PartyLevelOffset] : uint8_t{0};
    return {p.species, level <= 100 ? level : uint8_t{0},
            p.species == kUnown ? Conversion::Gen3PidSearch::unownForm(p.pid) : uint8_t{0},
            gen3Shiny(p.pid, p.tid, p.sid), Source::HomeCardPartySlot::GenderUnknown};
}

// Gen IV: the strict PK4 record's own level/form/gender; shiny by the same
// PID/TID/SID rule as Pokemon4ReadOnlyView::isShiny.
[[nodiscard]] inline Slot fromGen4(const ::Pokemon::Pokemon4ReadOnly& p) noexcept {
    return {p.species(), p.partyLevel(), p.form(),
            gen3Shiny(p.pid(), p.tid(), p.sid()), p.gender()};
}

// Gen V fills the same fields in Gen5GameCardPreview.h (PK5 0x40 gender/form,
// PID/TID/SID shiny, party-extension level).

} // namespace PokeVault::Integration::HomePartyPreview
#endif
