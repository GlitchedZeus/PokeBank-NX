#!/usr/bin/env python3
"""Generate only DP/Pt/HGSS personal tables using tools/pkhex_source.py's pinned oracle.
Adapted from kiasta/PKSE 55039848bbeeda114484614a9ed2e1d29804dc47 (AGPL-3.0).
No other generation or legality tables are generated.
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from pkhex_source import pkhex_path  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
INC = os.path.join(ROOT, "include", "Pokemon")
SRC = os.path.join(ROOT, "src", "Pokemon")

TYPE_NONE = 255

# Gen 1 / Gen 2 ROM type id -> PKSE TYPE_* (the contiguous MoveType numbering in PokemonTypes.h).
G1_TYPE_TO_PKSE = {0: 0, 1: 1, 2: 2, 3: 3, 4: 4, 5: 5, 7: 6, 8: 7,
                   20: 9, 21: 10, 22: 11, 23: 12, 24: 13, 25: 14, 26: 15}
G2_TYPE_TO_PKSE = dict(G1_TYPE_TO_PKSE)
G2_TYPE_TO_PKSE.update({9: 8, 27: 16})   # Steel and Dark are Gen 2's additions

# group code -> layout. Offsets verified against PKHeX.Core/PersonalInfo/Info/PersonalInfo*.cs
# at the pinned ref. `stats` is the offset of the first of HP/ATK/DEF/SPE/SPA/SPD in PKHeX's
# order -- NOT the order PersonalRecord declares them in, which is why decode() names each one.
#   fields: resource, size, stats, type1, type2, catch, gender, friend, growth,
#           ability1, ability2, abilityHidden, abilityWidth, formCount, formIndex, maxSpecies, typemap
GROUPS = [
    ("4DP",    "personal_dp",   0x2C, 0x00, 0x06, 0x07, 0x08, 0x10, 0x12, 0x13, 0x16, 0x17, None, 1, 0x29, 0x2A,  493, None),
    ("4PT",    "personal_pt",   0x2C, 0x00, 0x06, 0x07, 0x08, 0x10, 0x12, 0x13, 0x16, 0x17, None, 1, 0x29, 0x2A,  493, None),
    ("4HGSS",  "personal_hgss", 0x2C, 0x00, 0x06, 0x07, 0x08, 0x10, 0x12, 0x13, 0x16, 0x17, None, 1, 0x29, 0x2A,  493, None),
]

KEYS = ("code resource size stats type1 type2 catch gender friend growth "
        "ability1 ability2 abilityHidden abilityWidth formCount formIndex maxSpecies typemap").split()


def layout(entry):
    return dict(zip(KEYS, entry))


def u16(entry_bytes, offset):
    return entry_bytes[offset] | (entry_bytes[offset + 1] << 8)


def convert_type(typemap, raw, code):
    if typemap is None:
        return raw
    table = G1_TYPE_TO_PKSE if typemap == "g1" else G2_TYPE_TO_PKSE
    if raw not in table:
        raise SystemExit(f"{code}: unmapped ROM type id {raw}. Extend the conversion table.")
    return table[raw]


def decode(lay, entry_bytes):
    stats = lay["stats"]
    singleSpecial = lay["typemap"] == "g1"   # Gen 1 stores ONE Special where the rest store two
    hp, atk, dfn, spe = (entry_bytes[stats + k] for k in range(4))
    if singleSpecial:
        spa = spd = entry_bytes[stats + 4]
    else:
        spa, spd = entry_bytes[stats + 4], entry_bytes[stats + 5]

    def ability(offset):
        if offset is None:
            return 0
        return u16(entry_bytes, offset) if lay["abilityWidth"] == 2 else entry_bytes[offset]

    type1 = convert_type(lay["typemap"], entry_bytes[lay["type1"]], lay["code"])
    type2 = convert_type(lay["typemap"], entry_bytes[lay["type2"]], lay["code"])
    if type2 == type1:
        type2 = TYPE_NONE   # PKSE's convention for a single-typed species

    return dict(
        hp=hp, atk=atk, dfn=dfn, spe=spe, spa=spa, spd=spd,
        type1=type1, type2=type2,
        ability1=ability(lay["ability1"]),
        ability2=ability(lay["ability2"]),
        abilityHidden=ability(lay["abilityHidden"]),
        # Gen 2 stores no per-species base friendship; PKHeX hard-codes 70 for it.
        genderRatio=entry_bytes[lay["gender"]],
        baseFriendship=70 if lay["friend"] is None else entry_bytes[lay["friend"]],
        growthRate=entry_bytes[lay["growth"]],
        catchRate=0 if lay["catch"] is None else entry_bytes[lay["catch"]],
        # A generation with no alternate forms reads as one form and never redirects.
        formCount=1 if lay["formCount"] is None else max(1, entry_bytes[lay["formCount"]]),
        formIndex=0 if lay["formIndex"] is None else u16(entry_bytes, lay["formIndex"]),
    )


def sanity(code, rows):
    """Pin the values a wrong entry size or a wrong offset would break.

    AN ALL-ZERO ROW IS REAL DATA, NOT A BAD READ. From Gen 8 on, a personal table carries a
    row for every National Dex id but ZEROES the ones that game does not have -- Sword/Shield
    has no Weedle, Legends: Arceus no Bulbasaur. That is precisely the dex-presence answer, and
    it is why the checks below skip empty rows instead of failing on them: a wrong offset shows
    up as most of the table being empty, which the population count catches, while a handful of
    empty rows is the game telling the truth about its own roster.
    """
    # PKHeX's gender byte is a closed set: always-male 0, the four thresholds, 87.5% female,
    # always-female 254, genderless 255. Anything else means the byte read is not the ratio.
    allowed = {0, 31, 63, 127, 191, 225, 254, 255}
    present = 0
    for species, row in enumerate(rows):
        if species == 0:
            continue
        if row["hp"] == 0 and row["atk"] == 0 and row["def" if False else "dfn"] == 0:
            continue   # this game does not have this species
        present += 1
        if row["genderRatio"] not in allowed:
            raise SystemExit(f"{code}: species {species} gender byte {row['genderRatio']} is not a "
                             f"ratio -- the entry size or the gender offset is wrong.")
        if row["growthRate"] > 5:
            raise SystemExit(f"{code}: species {species} growth rate {row['growthRate']} is out of "
                             f"range 0-5 -- the growth offset is wrong.")
    # A wrong stat offset empties the table; a real roster never does. The floor is deliberately
    # low (Let's Go carries only 153 species) and is a smoke test, not a roster check.
    if present < 100:
        raise SystemExit(f"{code}: only {present} species have base stats -- the stat offset or "
                         f"the entry size is wrong.")
    return present


def emit(lay, rows, formRows):
    code, count = lay["code"], len(rows) + len(formRows)
    guard = f"POKEMON_PERSONALINFO{code.upper()}_H"
    header = [
        f"/** Generated from PKHeX {lay['resource']} at 6501f0ab46e8f8ca048539dbaf8cae8cb104e722.\n"
        f" * Regenerate with python3 tools/gen_gen4_personal.py. Unavailable fields are zero. */\n",
        f"#ifndef {guard}\n#define {guard}\n\n",
        '#include "Pokemon/PersonalRecord.h"\n\nnamespace Pokemon\n{\n',
        f"    /// Highest National Dex id this group has a row for.\n"
        f"    inline constexpr uint16_t PERSONAL_MAX_SPECIES_{code} = {lay['maxSpecies']};\n\n",
        f"    /// Rows in the table: the form-0 entries, then the alternate-form entries a base\n"
        f"    /// row's formIndex redirects into.\n"
        f"    inline constexpr size_t PERSONAL_COUNT_{code} = {count};\n\n",
        f"    extern const PersonalRecord PERSONAL_{code}[PERSONAL_COUNT_{code}];\n\n",
        f"    /// Row for (species, form), mirroring PKHeX's FormStatsIndex redirection. An unknown\n"
        f"    /// species or a form this group does not define falls back to the species' form-0 row;\n"
        f"    /// a species this group does not have at all returns the empty record, whose zero base\n"
        f"    /// stats mean NO DATA and must not be computed against.\n"
        f"    const PersonalRecord &getPersonalInfo{code}(uint16_t species, uint8_t form) noexcept;\n",
        "}\n\n#endif  // " + guard + "\n",
    ]

    def row_text(row):
        return ("        {%3d,%4d,%4d,%4d,%4d,%4d, %3d,%4d, %4d,%4d,%4d, %3d,%4d,%2d,%4d, %2d,%5d},"
                % (row["hp"], row["atk"], row["dfn"], row["spe"], row["spa"], row["spd"],
                   row["type1"], row["type2"], row["ability1"], row["ability2"], row["abilityHidden"],
                   row["genderRatio"], row["baseFriendship"], row["growthRate"], row["catchRate"],
                   row["formCount"], row["formIndex"]))

    source = [
        f"/**\n * GENERATED from PKHeX's {lay['resource']}; regenerate with\n"
        f" * `python tools/gen_gen4_personal.py`.\n */\n",
        f'#include "Pokemon/PersonalInfo{code}.h"\n\nnamespace Pokemon\n{{\n',
        f"    // hp atk def spe spa spd | type1 type2 | ability1 ability2 abilityHidden |\n"
        f"    // genderRatio baseFriendship growthRate catchRate | formCount formIndex\n",
        f"    const PersonalRecord PERSONAL_{code}[PERSONAL_COUNT_{code}] = {{\n",
    ]
    for species, row in enumerate(rows):
        source.append(row_text(row) + f"  // {species}\n")
    if formRows:
        source.append(f"        // alternate-form rows, indexed by a base row's formIndex\n")
        for offset, row in enumerate(formRows):
            source.append(row_text(row) + f"  // form row {len(rows) + offset}\n")
    source.append("    };\n\n")
    source.append(
        f"    const PersonalRecord &getPersonalInfo{code}(uint16_t species, uint8_t form) noexcept\n"
        f"    {{\n"
        f"        if (species == 0 || species > PERSONAL_MAX_SPECIES_{code})\n"
        f"        {{\n            return PERSONAL_RECORD_EMPTY;\n        }}\n"
        f"        const PersonalRecord &base = PERSONAL_{code}[species];\n"
        f"        // Guarding formIndex == 0 matters: index 0 is a real row (species 0) and would\n"
        f"        // otherwise be returned as though it were a form.\n"
        f"        if (form == 0 || base.formIndex == 0 || form >= base.formCount)\n"
        f"        {{\n            return base;\n        }}\n"
        f"        const size_t formRowIndex = static_cast<size_t>(base.formIndex) + form - 1;\n"
        f"        if (formRowIndex >= PERSONAL_COUNT_{code})\n"
        f"        {{\n            return base;\n        }}\n"
        f"        return PERSONAL_{code}[formRowIndex];\n"
        f"    }}\n}}\n")

    with open(os.path.join(INC, f"PersonalInfo{code}.h"), "w", encoding="utf-8", newline="\n") as fh:
        fh.write("".join(header))
    with open(os.path.join(SRC, f"PersonalInfo{code}.cpp"), "w", encoding="utf-8", newline="\n") as fh:
        fh.write("".join(source))
    return count


def main():
    total = 0
    for entry in GROUPS:
        lay = layout(entry)
        with open(pkhex_path("Resources/byte/personal/" + lay["resource"]), "rb") as fh:
            raw = fh.read()
        size = lay["size"]
        if len(raw) % size:
            raise SystemExit(f"{lay['code']}: {lay['resource']} is {len(raw)} bytes, not a multiple "
                             f"of the entry size 0x{size:X} -- the size is wrong.")
        entries = [raw[i * size:(i + 1) * size] for i in range(len(raw) // size)]
        maxSpecies = lay["maxSpecies"]
        if len(entries) <= maxSpecies:
            raise SystemExit(f"{lay['code']}: {lay['resource']} holds {len(entries)} rows but the "
                             f"group claims species up to {maxSpecies}.")
        rows = [decode(lay, entries[i]) for i in range(maxSpecies + 1)]
        formRows = [decode(lay, entries[i]) for i in range(maxSpecies + 1, len(entries))]
        present = sanity(lay["code"], rows)
        count = emit(lay, rows, formRows)
        total += count
        print(f"  {lay['code']:<7} {lay['resource']:<15} {present:>4} of {maxSpecies:>4} species "
              f"present + {len(formRows):>4} form rows = {count:>5} rows")



if __name__ == "__main__":
    main()
