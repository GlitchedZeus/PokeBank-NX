#!/usr/bin/env python3
"""Generate exact-game Generation II move-compatibility bitsets.

This is intentionally a data-generation tool, not a build-time network dependency.
It is pinned to the same PKHeX snapshot already cited by the Generation II staged
editor.  The generated .inc is committed so normal host/devkitA64 builds stay
fully offline and reproducible.

Compatibility here means "this species can carry this move in the selected Gen II
ruleset according to our current learnability data".  It is deliberately narrower
than full encounter legality.  Pools include level-up, TM/HM, egg, inherited
pre-evolution moves, and Crystal's three move tutors.  Gen I Time Capsule carryover
is layered in by Gen2MoveCompatibility.cpp using the existing pinned Gen I table.
"""
from __future__ import annotations

import pathlib
import struct
import urllib.request

ROOT = pathlib.Path(__file__).resolve().parents[1]
OUT = ROOT / "src" / "Integration" / "Gen2" / "Gen2MoveCompatibilityData.inc"
PKHEX_REF = "77dcd3a7895bceaafbbff12d25bdf77c1acd8ca5"
RAW = f"https://raw.githubusercontent.com/kwsch/PKHeX/{PKHEX_REF}/PKHeX.Core/Resources/byte"

MAX_SPECIES = 251
MAX_MOVE = 251
WIDTH = (MAX_MOVE + 8) // 8
PERSONAL_SIZE = 0x20
TMHM_OFFSET = 0x18
MACHINE_MOVES = [
    223, 29, 174, 205, 46, 92, 192, 249, 244, 237,
    241, 230, 173, 59, 63, 196, 182, 240, 202, 203,
    218, 76, 231, 225, 87, 89, 216, 91, 94, 247,
    189, 104, 8, 207, 214, 188, 201, 126, 129, 111,
    9, 138, 197, 156, 213, 168, 211, 7, 210, 171,
    15, 19, 57, 70, 148, 250, 127,
]
CRYSTAL_TUTOR_MOVES = [53, 85, 58]  # Flamethrower, Thunderbolt, Ice Beam


def fetch(rel: str) -> bytes:
    url = f"{RAW}/{rel}"
    print(f"fetch {url}")
    with urllib.request.urlopen(url) as response:
        return response.read()


def bin_entries(data: bytes) -> list[bytes]:
    """PKHeX BinLinkerAccessor16: count + count+1 u16 offsets."""
    count = struct.unpack_from("<H", data, 2)[0]
    offsets = [struct.unpack_from("<H", data, 4 + i * 2)[0] for i in range(count + 1)]
    return [data[offsets[i]:offsets[i + 1]] for i in range(count)]


def read_levelup(data: bytes) -> list[list[int]]:
    out: list[list[int]] = []
    for entry in bin_entries(data):
        count = len(entry) // 3
        out.append(list(struct.unpack_from(f"<{count}H", entry, 0)) if count else [])
    return out


def read_move_source(data: bytes) -> list[list[int]]:
    out: list[list[int]] = []
    for entry in bin_entries(data):
        count = len(entry) // 2
        out.append(list(struct.unpack_from(f"<{count}H", entry, 0)) if count else [])
    return out


def reverse_evolutions(data: bytes) -> dict[int, int]:
    """Return first pre-evolution by destination species, matching PKHeX lineage use."""
    rev: dict[int, int] = {}
    entries = bin_entries(data)
    for source in range(1, min(MAX_SPECIES + 1, len(entries))):
        entry = entries[source]
        for off in range(0, len(entry) - 7, 8):
            destination = struct.unpack_from("<H", entry, off + 4)[0]
            if destination == 0:
                break
            rev.setdefault(destination, source)
    return rev


def personal_records(data: bytes) -> list[bytes]:
    if len(data) % PERSONAL_SIZE:
        raise SystemExit(f"personal data size {len(data)} is not a multiple of {PERSONAL_SIZE}")
    rows = [data[i:i + PERSONAL_SIZE] for i in range(0, len(data), PERSONAL_SIZE)]
    if len(rows) != MAX_SPECIES + 1:
        raise SystemExit(f"expected 252 Gen II personal records, got {len(rows)}")
    return rows


def bit_set(record: bytes, index: int) -> bool:
    return bool(record[TMHM_OFFSET + (index >> 3)] & (1 << (index & 7)))


def direct_pools(crystal: bool) -> list[set[int]]:
    suffix = "c" if crystal else "gs"
    personal = personal_records(fetch(f"personal/personal_{suffix}"))
    levelup = read_levelup(fetch(f"levelup/lvlmove_{suffix}.pkl"))
    egg = read_move_source(fetch(f"eggmove/eggmove_{suffix}.pkl"))
    if len(levelup) <= MAX_SPECIES or len(egg) <= MAX_SPECIES:
        raise SystemExit(f"{suffix}: learnability arrays do not cover all 251 species")

    pools = [set() for _ in range(MAX_SPECIES + 1)]
    for species in range(1, MAX_SPECIES + 1):
        pool = pools[species]
        pool.update(move for move in levelup[species] if 0 < move <= MAX_MOVE)
        pool.update(move for move in egg[species] if 0 < move <= MAX_MOVE)
        record = personal[species]
        for index, move in enumerate(MACHINE_MOVES):
            if bit_set(record, index):
                pool.add(move)
        if crystal:
            for tutor_index, move in enumerate(CRYSTAL_TUTOR_MOVES, start=len(MACHINE_MOVES)):
                if bit_set(record, tutor_index):
                    pool.add(move)
    return pools


def with_pre_evolutions(pools: list[set[int]], rev: dict[int, int]) -> list[set[int]]:
    result = [set(values) for values in pools]
    for species in range(1, MAX_SPECIES + 1):
        current = species
        seen = {species}
        for _ in range(4):
            previous = rev.get(current)
            if previous is None or previous in seen:
                break
            result[species].update(pools[previous])
            seen.add(previous)
            current = previous
    return result


def encode(pools: list[set[int]]) -> list[int]:
    raw = [0] * ((MAX_SPECIES + 1) * WIDTH)
    for species, moves in enumerate(pools):
        # Empty move slot is always compatible.
        raw[species * WIDTH] |= 1
        for move in moves:
            raw[species * WIDTH + (move >> 3)] |= 1 << (move & 7)
    return raw


def emit_array(name: str, values: list[int]) -> str:
    lines = [f"static constexpr std::array<uint8_t, {len(values)}> {name}{{{{"]
    for i in range(0, len(values), 24):
        chunk = ", ".join(f"0x{value:02X}" for value in values[i:i + 24])
        lines.append(f"    {chunk},")
    lines.append("}};")
    return "\n".join(lines)


def main() -> None:
    rev = reverse_evolutions(fetch("evolve/evos_g2.pkl"))
    gs = encode(with_pre_evolutions(direct_pools(False), rev))
    crystal = encode(with_pre_evolutions(direct_pools(True), rev))
    if gs == crystal:
        raise SystemExit("Gold/Silver and Crystal compatibility unexpectedly identical")

    text = f"""// AUTO-GENERATED by tools/gen_gen2_move_compatibility.py. DO NOT EDIT.\n// PKHeX pinned source: {PKHEX_REF}\n// 252 species rows (0..251), 252 move bits (0..251), {WIDTH} bytes per row.\n#include <array>\n#include <cstdint>\n\nstatic constexpr std::size_t kGen2MoveCompatibilityWidth = {WIDTH};\n\n{emit_array('kGen2GSCompatibility', gs)}\n\n{emit_array('kGen2CrystalCompatibility', crystal)}\n"""
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(text, encoding="utf-8")
    print(f"wrote {OUT} ({len(text)} bytes)")


if __name__ == "__main__":
    main()
