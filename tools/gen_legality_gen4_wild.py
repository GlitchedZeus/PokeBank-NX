#!/usr/bin/env python3
"""Generate compact Gen IV wild-encounter legality evidence from pinned PKHeX data.

Slot numbers are retained so Method J/K frame correlation can prove the selected wild slot.
Static / Magnet Pull eligible-slot index/count metadata is retained in a parallel table so
lead-ability RNG paths can be reconstructed without changing the packed encounter-row ABI.
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
        for slot_index in range(slot_count):
            offset = 6 + slot_index * 10
            species = struct.unpack_from("<H", area, offset)[0]
            form = area[offset + 2]
            slot = area[offset + 3]
            minimum = area[offset + 4]
            maximum = area[offset + 5]
            magnet_index = area[offset + 6]
            magnet_count = area[offset + 7]
            static_index = area[offset + 8]
            static_count = area[offset + 9]
            key = (
                game_index, species, location, minimum, maximum,
                method, form, slot, rate
            )
            lead_meta = (
                magnet_index, magnet_count, static_index, static_count
            )
            # Multiple PKHeX area records can collapse to the same persisted
            # encounter identity while differing only in permitted ground tiles.
            # Preserve one canonical row and OR positive Radar capability across
            # those aliases; ground tile itself is not stored in PK4 encounter data.
            # Static/Magnet metadata, however, changes the source RNG slot table.
            # If aliases ever disagree, fail generation rather than silently choose
            # one history and create false-negative legality evidence.
            if key in rows:
                old_radar, old_lead_meta = rows[key]
                if old_lead_meta != lead_meta:
                    raise ValueError(
                        "Gen IV persisted encounter aliases disagree on Static/Magnet metadata: %r"
                        % (key,)
                    )
                rows[key] = (old_radar or radar_capable, old_lead_meta)
            else:
                rows[key] = (radar_capable, lead_meta)
    return {
        key + (radar_capable,) + lead_meta
        for key, (radar_capable, lead_meta) in rows.items()
    }


def pack(row):
    (
        game, species, location, minimum, maximum, method, form, slot, rate,
        radar_capable, _magnet_index, _magnet_count, _static_index, _static_count,
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
    magnet_index, magnet_count, static_index, static_count = row[-4:]
    return (
        magnet_index
        | (magnet_count << 8)
        | (static_index << 16)
        | (static_count << 24)
    )


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

    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with open(OUT, "w", encoding="utf-8", newline="\n") as handle:
        handle.write("\n".join(lines))
    print(
        "wrote %d unique Gen IV wild encounter rows plus Static/Magnet metadata to %s"
        % (len(ordered), OUT)
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
