#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Auditable source-pin and extraction check for L1 Gen V/VI PKHeX tables.

Usage:
  python3 tools/legality/l1_verify_pkhex_gen56_sources.py --self-test
  python3 tools/legality/l1_verify_pkhex_gen56_sources.py --source-root /path/to/PKHeX-master

The owner-supplied zip is not committed to PokeBank NX. If it isn't available,
the source parity gate is BLOCKED, never skipped as "passed". Unmodified
PKHeX.Core C# remains the host behavior oracle for future differential runs.
"""
import argparse
import hashlib
import re
from pathlib import Path

UPSTREAM = "66ef5983a5d349efa2dd615f2926a1b1ee97306e"
GENERATOR_SHA256 = {
    "EncounterGenerator5.cs": "cae4866450951cfaa78c1f75e31ef99d7d2dabfa6aac544b9695d5daa7b59bf9",
    "EncounterGenerator6.cs": "ce04375f3b148553b1900d0996ef6a79fbbe31ce0e6d31a0b3a8f0688d49f03e",
}
FILES = {
    "Gen5/Encounters5BW.cs": "9ab0abd941e189261cab2ee302c3a7fd98da5a8b",
    "Gen5/Encounters5B2W2.cs": "9876a9cd700f2fcc337be05741029d1fc88c30f2",
    "Gen6/Encounters6XY.cs": "147d623de132e7210bf8449c40825cd70cb75da0",
    "Gen6/Encounters6AO.cs": "b59214398c756be172107dc09cdff9bf9684ff9c",
}
MASKS = {"W": 1, "B": 2, "W2": 4, "B2": 8, "X": 16, "Y": 32,
         "AS": 64, "OR": 128, "BW": 3, "B2W2": 12, "XY": 48,
         "ORAS": 192, "AO": 192}
TABLE_RE = re.compile(
    r"\b(?:public|internal|private)\s+static\s+readonly\s+"
    r"Encounter(?:Static5|Static5N|Static6|Trade5[A-Za-z0-9]*|Trade6)"
    r"\[\]\s+(\w+)\s*=\s*\[([\s\S]*?)\];")
ROW_RE = re.compile(
    r"\{Table::(\w+),(\d+),(\d+),(\d+),(\d+),(\d+),"
    r"(\d+),(\d+),(\d+),(\d+),(\d+)\},")
CTOR_RE = re.compile(
    r"(?:^|,)(B2W2|ORAS|BW|XY|B2|W2|AS|OR|W|B|X|Y)(?:,|$)")


def git_blob(data: bytes) -> str:
    return hashlib.sha1(b"blob " + str(len(data)).encode() + b"\0" + data).hexdigest()


def split_items(text: str) -> list[str]:
    text = re.sub(r"//[^\n]*", "", text)
    items, start, braces, parens, brackets, quote = [], 0, 0, 0, 0, False
    for i, ch in enumerate(text):
        if ch == '"' and (i == 0 or text[i - 1] != "\\"):
            quote = not quote
        if quote:
            continue
        if ch == "{":
            braces += 1
        elif ch == "}":
            braces -= 1
        elif ch == "(":
            parens += 1
        elif ch == ")":
            parens -= 1
        elif ch == "[":
            brackets += 1
        elif ch == "]":
            brackets -= 1
        if ch == "," and braces == parens == brackets == 0:
            item = text[start:i].strip()
            if item:
                items.append(item)
            start = i + 1
    tail = text[start:].strip()
    if tail:
        items.append(tail)
    return items


def field(text: str, name: str):
    found = re.search(r"\b" + re.escape(name) + r"\s*=\s*(\d+)", text)
    return int(found.group(1)) if found else None


def extract(text: str, file_number: int):
    result, tables = [], []
    default_mask = (3, 12, 48, 192)[file_number]
    for name, raw in TABLE_RE.findall(text):
        kind = 1 if name.startswith("TradeGift_") else 0
        entries = split_items(raw)
        tables.append((name, len(entries), kind))
        for i, entry in enumerate(entries):
            cosplay = entry.startswith("BaseCosplay")
            if not entry.startswith("new(") and not cosplay:
                raise ValueError(f"Unsupported PKHeX element {name}[{i}]: {entry[:80]}")
            content_match = re.search(r"\{([\s\S]*)\}", entry)
            body = content_match.group(1) if content_match else ""
            species = 25 if cosplay else field(body, "Species")
            level = 20 if cosplay else field(body, "Level")
            if species is None or level is None:
                raise ValueError(f"Missing literal species/level in {name}[{i}]")
            ctor_match = re.search(r"^new\s*\(([^)]*)\)", entry)
            ctor = "".join(ctor_match.group(1).split()) if ctor_match else ""
            version = CTOR_RE.search(ctor)
            game_mask = MASKS[version.group(1)] if version else default_mask
            if kind and not version:
                raise ValueError(f"Unknown NPC trade original-game selector in {name}[{i}]")
            location = None if cosplay else field(body, "Location")
            egg = None if cosplay else field(body, "EggLocation")
            form = field(body, "Form")
            flawless = 3 if cosplay else (field(body, "FlawlessIVCount") or 0)
            if cosplay and form is None:
                form = 6
            shiny = 1 if cosplay or re.search(r"\bShiny\s*=\s*Shiny\.Never", body) else (
                2 if re.search(r"\bShiny\s*=\s*Shiny\.Always", body) else 0)
            roaming = bool(re.search(r"\bIsRoaming\s*=\s*true", body))
            flags = (int(location is not None) |
                     (int(egg is not None) << 1) |
                     (int(form is not None) << 2) |
                     (int(cosplay) << 3) |
                     (int(roaming) << 4))
            result.append((name, i, species, level, location or 0, egg or 0,
                           game_mask, form or 0, shiny, flawless, flags))
    return tables, result


def self_test() -> None:
    toy = ("""internal static readonly EncounterStatic5[] StaticW = [
      new(W) { Species = 644, Level = 50, Location = 45, Shiny = Shiny.Never },
      new(BW) { Species = 495, Level = 5, Location = 4 }, // comment
    ];""")
    tables, rows = extract(toy, 0)
    assert tables == [("StaticW", 2, 0)]
    assert rows[0] == ("StaticW", 0, 644, 50, 45, 0, 1, 0, 1, 0, 1)
    assert rows[1] == ("StaticW", 1, 495, 5, 4, 0, 3, 0, 0, 0, 1)
    example = ("internal static readonly EncounterStatic6[] Encounter_AO_Regular = ["
               "BaseCosplay with { Form = 1, Moves = new(1, 2, 3, 4) }, BaseCosplay, ];")
    tables, rows = extract(example, 3)
    assert len(rows) == 2 and rows[0][:4] == ("Encounter_AO_Regular", 0, 25, 20)
    assert rows[1][-1] == 12
    print("L1 Phase A PKHeX source parser self-test PASS (not owner archive verification)")


def verify(source_root: Path, catalog: Path) -> None:
    if not source_root.is_dir():
        raise SystemExit("BLOCKED: supply the actual owner PKHeX-master source directory")
    base = source_root / "PKHeX.Core" / "Legality" / "Encounters"
    for generator, expected in GENERATOR_SHA256.items():
        path = base / "Generator" / "ByGeneration" / generator
        raw = path.read_bytes()
        actual = hashlib.sha256(raw).hexdigest()
        if actual != expected:
            raise SystemExit(f"FAIL pinned archive SHA256 {generator}: {actual} != {expected}")
    actual_rows, actual_tables = [], []
    for i, (relative, expected_blob) in enumerate(FILES.items()):
        path = base / "Data" / relative
        raw = path.read_bytes()
        blob = git_blob(raw)
        if blob != expected_blob:
            raise SystemExit(f"FAIL upstream pinned Git blob {relative}: {blob} != {expected_blob}")
        groups, rows = extract(raw.decode("utf-8-sig"), i)
        actual_tables += groups
        actual_rows += rows
    expected_rows = [
        (name, *(int(v) for v in values))
        for name, *values in ROW_RE.findall(catalog.read_text(encoding="utf-8"))
    ]
    if len(actual_rows) != 278 or len(actual_tables) != 21:
        raise SystemExit(f"FAIL expected 278 rows/21 groups; got {len(actual_rows)}/{len(actual_tables)}")
    if actual_rows != expected_rows:
        index = next((i for i, (a, b) in enumerate(zip(actual_rows, expected_rows))
                      if a != b), min(len(actual_rows), len(expected_rows)))
        raise SystemExit(f"FAIL generated native row differs from PKHeX pinned source: row {index}")
    for name, count, _ in actual_tables:
        if sum(r[0] == name for r in actual_rows) != count:
            raise SystemExit(f"FAIL row index coverage for {name}")
    print(f"L1 Phase A PKHeX SOURCE PIN PASS: {len(actual_rows)} exact rows / "
          f"{len(actual_tables)} arrays and 2 owner-archive generator SHA-256s")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--source-root", type=Path)
    parser.add_argument("--catalog", type=Path, default=Path(
        "include/Legality/Gen56PkhexSourceCatalog.generated.h"))
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        self_test()
    else:
        if args.source_root is None:
            parser.error("--source-root required; without PKHeX source, gate is BLOCKED")
        verify(args.source_root, args.catalog)
