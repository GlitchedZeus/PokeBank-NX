/** Generated from PKHeX personal_hgss at 6501f0ab46e8f8ca048539dbaf8cae8cb104e722.
 * Regenerate with python3 tools/gen_gen4_personal.py. Unavailable fields are zero. */
#ifndef POKEMON_PERSONALINFO4HGSS_H
#define POKEMON_PERSONALINFO4HGSS_H

#include "Pokemon/PersonalRecord.h"

namespace Pokemon
{
    /// Highest National Dex id this group has a row for.
    inline constexpr uint16_t PERSONAL_MAX_SPECIES_4HGSS = 493;

    /// Rows in the table: the form-0 entries, then the alternate-form entries a base
    /// row's formIndex redirects into.
    inline constexpr size_t PERSONAL_COUNT_4HGSS = 508;

    extern const PersonalRecord PERSONAL_4HGSS[PERSONAL_COUNT_4HGSS];

    /// Row for (species, form), mirroring PKHeX's FormStatsIndex redirection. An unknown
    /// species or a form this group does not define falls back to the species' form-0 row;
    /// a species this group does not have at all returns the empty record, whose zero base
    /// stats mean NO DATA and must not be computed against.
    const PersonalRecord &getPersonalInfo4HGSS(uint16_t species, uint8_t form) noexcept;
}

#endif  // POKEMON_PERSONALINFO4HGSS_H
