#!/usr/bin/env python3
"""Validate L3's Gen I retail catalog against pinned pret Git histories.

Usage: python3 tools/legality/verify_l3_gen1_retail_catalog.py \
          --pokered /path/to/pokered --pokeyellow /path/to/pokeyellow

Both locations must be actual Git checkouts holding the exact pinned commits.
No network access, save data, or ROM is required. Source fixtures are not proof
of a particular Pokémon's personal history. This does not run PKHeX C#.
"""
import argparse
import pathlib
import re
import subprocess

ROOT = pathlib.Path(__file__).resolve().parents[2]
HEADER = ROOT / "include/Legality/Gen1RbyRetailPackedCatalog.h"
PINS = {
    "pokered": (
        "af519899719f0754965776faac0e836a3b906e6d",
        "6a6704f98b8b075f2fd81c91d41a1dda0f2a047d",
    ),
    "pokeyellow": (
        "e89ead154b9968aa50eed9328ff2b38b6c194382",
        "8bee626ae17d052a17562be9959a13b6102146f7",
    ),
}
ALPHABET = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_"
STONES = {"MOON_STONE": 1, "FIRE_STONE": 2, "THUNDER_STONE": 3,
          "WATER_STONE": 4, "LEAF_STONE": 5}


def read_git(folder: pathlib.Path, rev: str, path: str) -> str:
    return subprocess.check_output(
        ["git", "-C", str(folder), "show", f"{rev}:{path}"],
        stderr=subprocess.PIPE,
    ).decode("utf-8")


def blob_at(folder: pathlib.Path, rev: str, path: str) -> str:
    return subprocess.check_output(
        ["git", "-C", str(folder), "rev-parse", f"{rev}:{path}"],
        stderr=subprocess.PIPE,
        text=True,
    ).strip()


def encoding(number: int) -> str:
    assert 0 <= number < (1 << 18)
    return "".join(ALPHABET[(number >> shift) & 63] for shift in (12, 6, 0))


def parse(root: pathlib.Path, pin: str, expected_blob: str):
    evo_path = "data/pokemon/evos_moves.asm"
    assert blob_at(root, pin, evo_path) == expected_blob, "evolution source pin mismatch"
    dex_source = read_git(root, pin, "constants/pokedex_constants.asm")
    move_source = read_git(root, pin, "constants/move_constants.asm")
    content = read_git(root, pin, evo_path)
    dex = {name.replace("_", ""): int(number)
           for name, number in re.findall(
               r"^\s*const\s+DEX_([A-Z0-9_]+)\s*;\s*(\d+)",
               dex_source, re.M)}
    assert len(dex) == 151
    moves = {}
    index = 0
    for line in move_source.splitlines():
        if (match := re.match(r"^\s*const\s+([A-Z0-9_]+)\b", line)):
            moves[match[1]] = index
            index += 1
        elif re.match(r"^\s*const_skip\b", line):
            index += 1
    assert moves["THUNDER_WAVE"] == 86
    labels = list(re.finditer(r"^([A-Za-z0-9]+)EvosMoves:", content, re.M))
    levelups = [[] for _ in range(152)]
    evolutions = [[] for _ in range(152)]
    for i, label in enumerate(labels):
        name = label[1].upper()
        if name not in dex:  # original internal MissingNo. labels
            continue
        species = dex[name]
        stop = labels[i + 1].start() if i + 1 < len(labels) else len(content)
        body = content[label.end():stop]
        evolution_text, learn_text = body.split("; Learnset", 1)
        for level_text, move_name in re.findall(
                r"^\s*db\s+(\d+)\s*,\s*([A-Z0-9_]+)", learn_text, re.M):
            assert move_name in moves, (species, move_name)
            level = int(level_text)
            assert 1 <= level <= 100
            levelups[species].append((level, moves[move_name]))
        # Stable level order, NOT assembly text order (Yellow Primeape has
        # level 46 listed before its level 45 Screech entry).
        levelups[species].sort(key=lambda entry: entry[0])
        for method, x, y, dest in re.findall(
                r"^\s*db\s+(EVOLVE_(?:LEVEL|ITEM|TRADE))\s*,\s*"
                r"([A-Z0-9_]+)\s*,\s*([A-Z0-9_]+)(?:\s*,\s*([A-Z0-9_]+))?",
                evolution_text, re.M):
            target = dex[(dest or y).replace("_", "")]
            if method == "EVOLVE_LEVEL":
                kind, condition = 1, int(x)
            elif method == "EVOLVE_ITEM":
                kind, condition = 2, STONES[x]
            else:
                kind, condition = 3, 0
            evolutions[species].append((target << 10) | (kind << 7) | condition)
    assert all(isinstance(x, list) for x in levelups[1:])
    levels_encoded = "".join(
        "".join(encoding((level << 8) | move) for level, move in row) + ";"
        for row in levelups[1:])
    evolution_encoded = "".join(
        "".join(encoding(evo) for evo in row) + ";"
        for row in evolutions[1:])
    return levels_encoded, evolution_encoded, sum(map(len, levelups)), sum(map(len, evolutions))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--pokered", required=True, type=pathlib.Path)
    parser.add_argument("--pokeyellow", required=True, type=pathlib.Path)
    args = parser.parse_args()
    result = {}
    for name, folder in (("pokered", args.pokered), ("pokeyellow", args.pokeyellow)):
        result[name] = parse(folder, *PINS[name])
    source = HEADER.read_text(encoding="utf-8")
    for source_name, name in (("pokered", "kRB"), ("pokeyellow", "kYellow")):
        match = re.search(rf"inline constexpr sv {name}=\s*\n?\"([^\"]+)\";", source)
        assert match and match[1] == result[source_name][0], f"{name}: source mismatch"
    match = re.search(r'inline constexpr sv kEvo=\s*\n?"([^"]+)";', source)
    assert match and match[1] == result["pokered"][1] == result["pokeyellow"][1], "evolution mismatch"
    assert result["pokered"][2:] == (728, 72)
    assert result["pokeyellow"][2:] == (755, 72)
    print("Gen I L3 retail source catalog matches both pinned original-game histories: PASS")
    print("Red/Blue: 728 offers; Yellow: 755 offers; each: 72 evolution records / 151 species")


if __name__ == "__main__":
    main()
