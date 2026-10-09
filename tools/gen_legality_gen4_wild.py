#!/usr/bin/env python3
"""Generate compact Gen IV wild-encounter legality evidence from pinned PKHeX data.

Slot numbers are retained so Method J/K frame correlation can prove the selected wild slot.
Static / Magnet Pull eligible-slot index/count metadata and exact source-area PressureLevel
are retained in parallel tables so lead-ability RNG paths can be reconstructed without
changing the packed encounter-row ABI.
"""

from __future__ import annotations

import os
import struct

import pkhex_source

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "include", "Legality", "Gen4WildEncounterData.inc")

GAMES = (
    ("diamond_nds", "Resources/legality/wild/Gen4/encounter_d.pkl", 0),
    ("pearl_nds", "Resources/legality/wild/Gen4/encounter_p.pkl", 1),
    ("platinum_nds", "Resources/legality/wild/Gen4/encounter_pt.pkl", 2),
    ("heartgold_nds", "Resources/legality/wild/Gen4/encounter_hg.pkl", 3),
    ("soulsilver_nds", "Resources/legality/wild/Gen4/encounter_ss.pkl", 4),
)


def entries(data: bytes):
    if len(data) < 8:
        raise ValueError("BinLinker resource is truncated")
    count = struct.unpack_from("<H", data, 2)[0]
    offsets = struct.unpack_from("<%dI" % (count + 1), data, 4)
    for i in range(count):
        start, end = offsets[i], offsets[i + 1]
        if start > end or end > len(data):
            raise ValueError("BinLinker entry offset is invalid")
        yield data[start:end]


def parse_game(path: str, game_index: int):
    with open(pkhex_source.pkhex_path(path), "rb") as handle:
        data = handle.read()

    # A persisted PK4 identity does not retain EncounterArea4's ground-tile/source
    # area identity. Multiple source areas can therefore collapse to the same saved
    # species/location/level/method/slot tuple while carrying different lead-history
    # metadata. Preserve every distinct Static/Magnet/Pressure source-area alias;
    # choosing one would create false negatives. Radar capability is positive-only
    # evidence, so it is ORed across all indistinguishable aliases.
    rows = {}
    for area_index, area in enumerate(entries(data)):
        if len(area) < 6:
            raise ValueError("truncated Gen IV encounter area %d" % area_index)
        location = area[0]
        method = area[2]
        rate = area[3]
        ground_tile = struct.unpack_from("<H", area, 4)[0]
        # PKHeX EncounterSlot4.CanUseRadar at the pinned reference:
        # D/P/Pt only, Grass ground tile allowed, and never the Great Marsh.
        radar_capable = (
            game_index <= 2
            and (ground_tile & (1 << 2)) != 0
            and location != 52
        )
        # Mirror PKHeX EncounterArea4.ReadRegularSlots exactly: it integer-divides
        # the post-header length by 10 and ignores any trailing container bytes.
        slot_count = (len(area) - 6) // 10
        area_slots = []
        for slot_index in range(slot_count):
            offset = 6 + slot_index * 10
            area_slots.append((
                struct.unpack_from("<H", area, offset)[0],  # species
                area[offset + 2],  # form
                area[offset + 3],  # slot number
                area[offset + 4],  # minimum level
                area[offset + 5],  # maximum level
                area[offset + 6],  # Magnet Pull index
                area[offset + 7],  # Magnet Pull count
                area[offset + 8],  # Static index
                area[offset + 9],  # Static count
            ))

        # Pinned EncounterSlot4.PressureLevel:
        #   Type != Grass ? LevelMax : Parent.GetPressureMax(Species, LevelMax)
        # GetPressureMax scans only this exact EncounterArea4 source and raises the
        # row maximum to the highest LevelMax of the same species in that area.
        pressure_max = {}
        if method == 0:  # SlotType4.Grass
            for species, _form, _slot, _minimum, maximum, *_lead in area_slots:
                pressure_max[species] = max(pressure_max.get(species, 0), maximum)

        for (
            species, form, slot, minimum, maximum,
            magnet_index, magnet_count, static_index, static_count,
        ) in area_slots:
            pressure_level = pressure_max.get(species, maximum)
            key = (
                game_index, species, location, minimum, maximum,
                method, form, slot, rate
            )
            history_meta = (
                magnet_index, magnet_count, static_index, static_count,
                pressure_level,
            )
            state = rows.setdefault(key, {"radar": False, "history": set()})
            state["radar"] = state["radar"] or radar_capable
            state["history"].add(history_meta)

    out = set()
    for key, state in rows.items():
        for history_meta in state["history"]:
            out.add(key + (state["radar"],) + history_meta)
    return out


def pack(row):
    (
        game, species, location, minimum, maximum, method, form, slot, rate,
        radar_capable, _magnet_index, _magnet_count, _static_index, _static_count,
        _pressure_level,
    ) = row
    if slot > 0x0F:
        raise ValueError("Gen IV wild slot number exceeds packed 4-bit field")
    return (
        species
        | (location << 9)
        | (minimum << 17)
        | (maximum << 24)
        | (method << 31)
        | (form << 35)
        | (game << 43)
        | (slot << 46)
        | (rate << 50)
        | (int(radar_capable) << 58)
    )


def pack_lead_meta(row):
    magnet_index, magnet_count, static_index, static_count = row[-5:-1]
    return (
        magnet_index
        | (magnet_count << 8)
        | (static_index << 16)
        | (static_count << 24)
    )


def pressure_level(row):
    return row[-1]


def main() -> int:
    rows = set()
    for _name, path, game in GAMES:
        rows.update(parse_game(path, game))
    ordered = sorted(rows)

    lines = [
        "// GENERATED by tools/gen_legality_gen4_wild.py.",
        "// Source: PKHeX @ %s" % pkhex_source._REF,
        "// Resources: encounter_d/p/pt/hg/ss.pkl (BinLinker Gen IV wild slots).",
        "// Packed layout: species[0:8], location[9:16], min[17:23], max[24:30],",
        "// method[31:34], form[35:42], game[43:45], slot[46:49], rate[50:57],",
        "// radar-capable[58] (pinned EncounterSlot4.CanUseRadar positive evidence).",
        "// Parallel lead metadata layout: MagnetPullIndex[0:7], MagnetPullCount[8:15],",
        "// StaticIndex[16:23], StaticCount[24:31] from EncounterArea4.ReadRegularSlot.",
        "// Parallel PressureLevel bytes mirror pinned EncounterSlot4.PressureLevel,",
        "// including parent-area GetPressureMax for Grass slots.",
        "// Persisted-identity aliases with different lead/pressure histories are repeated,",
        "// because PK4 does not retain the source EncounterArea4/ground-tile identity.",
        "// This is wild-slot evidence only. Static/gift/trade/event encounters are separate,",
        "// so absence from this table MUST NOT be interpreted as illegal.",
        "inline constexpr uint64_t kPackedGen4WildEncounters[] = {",
    ]
    packed = [pack(row) for row in ordered]
    for i in range(0, len(packed), 6):
        chunk = ", ".join("0x%012xULL" % value for value in packed[i:i + 6])
        if i + 6 < len(packed):
            chunk += ","
        lines.append("    " + chunk)
    lines.append("};")
    lines.append("")

    lines.append("inline constexpr uint32_t kPackedGen4WildLeadMeta[] = {")
    lead_meta = [pack_lead_meta(row) for row in ordered]
    for i in range(0, len(lead_meta), 8):
        chunk = ", ".join("0x%08xU" % value for value in lead_meta[i:i + 8])
        if i + 8 < len(lead_meta):
            chunk += ","
        lines.append("    " + chunk)
    lines.append("};")
    lines.append("")

    lines.append("inline constexpr uint8_t kPackedGen4WildPressureLevel[] = {")
    pressure = [pressure_level(row) for row in ordered]
    for i in range(0, len(pressure), 16):
        chunk = ", ".join("%d" % value for value in pressure[i:i + 16])
        if i + 16 < len(pressure):
            chunk += ","
        lines.append("    " + chunk)
    lines.append("};")
    lines.append("")

    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with open(OUT, "w", encoding="utf-8", newline="\n") as handle:
        handle.write("\n".join(lines))
    print(
        "wrote %d Gen IV persisted-identity/lead-history rows to %s"
        % (len(ordered), OUT)
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
