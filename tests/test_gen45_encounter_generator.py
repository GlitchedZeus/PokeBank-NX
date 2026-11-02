#!/usr/bin/env python3
"""Offline bounds/provenance tests; reproduction against the pinned source is --check."""
import importlib.util
import json
from pathlib import Path
import re
import struct

root = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("generator", root / "tools/gen_gen45_encounter_context.py")
generator = importlib.util.module_from_spec(spec)
spec.loader.exec_module(generator)
for width, code in ((2,"H"),(4,"I")):
    start = 4 + width * 3
    data = b"g4" + struct.pack("<H", 2) + struct.pack("<"+code*3, start,start+3,start+5) + b"abcde"
    assert generator.entries(data,width) == [b"abc",b"de"]
    for broken in (b"", data[:4], data[:-1], data + b"extra",
                   data[:4]+struct.pack("<"+code*3,0,start+3,start+5)+data[start:],
                   data[:4]+struct.pack("<"+code*3,start,start+5,start+3)+data[start:]):
        try:
            generator.entries(broken,width)
        except ValueError:
            pass
        else:
            raise AssertionError("accepted corrupt BinLinker offsets")
manifest = json.loads((root / "docs/data/gen45-encounter-context-sources.json").read_text())
data = (root / "include/Integration/Encounter/EncounterContext45Data.inc").read_text()
assert manifest["revision"] == generator.PIN and generator.PIN in data
assert len(manifest["inputs_sha256"]) == 17
for digest in manifest["inputs_sha256"].values():
    assert re.fullmatch("[0-9a-f]{64}",digest)
rows = re.findall(r'\{"([a-z0-9_]+)", (\d+), (\d+), (\d+), (\d+), EncounterGuardrails::Method::(\w+), 0\}',data)
assert len(rows) == len(set(rows)) == sum(manifest["counts"].values())
for game, count in manifest["counts"].items():
    assert sum(row[0] == game for row in rows) == count
assert len(re.findall(r'\{[45], \d+, \d+\}',data)) == manifest["evolution_edges"]
print("Gen IV/V encounter generator: corrupt offsets rejected; exact committed counts and source hashes PASS")
