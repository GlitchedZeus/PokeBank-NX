#!/usr/bin/env python3
"""Recompute stricter Gen IV met-field cases directly from native source inputs."""
import argparse
import hashlib
import json
import re
import subprocess
from pathlib import Path

ap = argparse.ArgumentParser(description=__doc__)
for name in ("platinum", "heartgold", "oracle", "nx", "output"):
    ap.add_argument("--" + name, type=Path, required=True)
ap.add_argument("--expected", type=int, required=True)
args = ap.parse_args()
native = (args.platinum / "src/unk_02017038.c").read_text()
body = native[native.index("BOOL sub_0201708C"):]
ranges = [tuple(map(int, pair)) for pair in re.findall(r"param0 >= (\d+)\) && \(param0 <= (\d+)\)", body)]
assert len(ranges) == 3
hg = (args.heartgold / "src/pokemon.c").read_text()
setter = hg[hg.index("case MON_DATA_MET_LOCATION:", hg.index("blockD->EggLocation_DP = METLOC_FARAWAY_PLACE")):]
setter = setter[:setter.index("case MON_DATA_POKERUS:")]
assert "blockD->MetLocation_DP = VALUE(u16);" in setter
assert "blockB->MetLocation_PtHGSS = VALUE(u16);" in setter
assert "blockD->MetLocation_DP = METLOC_FARAWAY_PLACE;" in setter
constants = (args.heartgold / "include/constants/map_sections.h").read_text()
faraway = int(re.search(r"#define METLOC_FARAWAY_PLACE\s+(\d+)", constants)[1])
oracle, nx = args.oracle.read_text().splitlines(), args.nx.read_text().splitlines()
assert len(oracle) == len(nx)
seen, rows = set(), []
for a, b in zip(oracle, nx):
    left, right = a.split(), b.split()
    assert left[0] == right[0] and left[0] not in seen
    seen.add(left[0])
    if left[3] != "VALID" or right[1] != "INVALID":
        continue
    raw = bytes.fromhex(left[2])
    assert len(raw) == 136
    word = lambda offset: int.from_bytes(raw[offset:offset+2], "little")
    ext, dp = word(0x46), word(0x80)
    assert raw[0x5F] in (7, 8, 10, 11, 12) and ext
    assert word(0x44) == word(0x7E) == 0
    assert not (int.from_bytes(raw[0x38:0x3C], "little") & (1 << 30))
    expected = ext if any(low <= ext <= high for low, high in ranges) else faraway
    assert dp != expected, left[0]
    rows.append(dict(id=left[0], version=raw[0x5F], dp_met_field=dp,
                     extended_met_field=ext, native_setter_requires_dp_field=expected))
assert len(rows) == args.expected
pins = {}
for name, root, paths in (("platinum", args.platinum, ("src/pokemon.c", "src/unk_02017038.c")),
                          ("heartgold", args.heartgold, ("src/pokemon.c", "include/constants/map_sections.h"))):
    pins[name] = dict(commit=subprocess.check_output(["git", "-C", str(root), "rev-parse", "HEAD"], text=True).strip(),
                      files={p: hashlib.sha256((root/p).read_bytes()).hexdigest() for p in paths})
args.output.write_text(json.dumps(dict(records=len(rows), source_pins=pins,
    policy="All raw stricter cases independently recomputed from pinned native ranges and coupled setters; no ID controls the predicate.",
    native_compatibility_ranges=ranges, faraway_value=faraway, rows=rows), indent=2)+"\n")
print(f"Native Gen IV coupled met-field audit: {len(rows)}/{args.expected} contradictions confirmed")
