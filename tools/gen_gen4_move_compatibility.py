#!/usr/bin/env python3
"""Regenerate the Generation IV move-compatibility bitsets.

Pinned source: PokeAPI/pokeapi @ 168b1e89467054cda2e7df43ccebbb69b459497a
Normal builds do not run this script and require no network access.
"""

from __future__ import annotations
import csv
import io
import urllib.request

SHA = "168b1e89467054cda2e7df43ccebbb69b459497a"
BASE = f"https://raw.githubusercontent.com/PokeAPI/pokeapi/{SHA}/data/v2/csv"
GROUPS = (8, 9, 10)
METHODS = {1, 2, 3, 4, 6}
MAX_SPECIES = 493
MAX_MOVE = 467

def rows(name: str):
    with urllib.request.urlopen(f"{BASE}/{name}", timeout=30) as response:
        return list(csv.DictReader(io.TextIOWrapper(response, encoding="utf-8")))

def main() -> None:
    pokemon = rows("pokemon.csv")
    species_rows = rows("pokemon_species.csv")
    moves = rows("pokemon_moves.csv")

    default_species = {
        int(r["id"]): int(r["species_id"])
        for r in pokemon
        if r["is_default"] == "1" and int(r["species_id"]) <= MAX_SPECIES
    }
    parent = [0] * (MAX_SPECIES + 1)
    for r in species_rows:
        species = int(r["id"])
        if species <= MAX_SPECIES:
            parent[species] = int(r["evolves_from_species_id"] or 0)

    pools = {g: [set() for _ in range(MAX_SPECIES + 1)] for g in GROUPS}
    for r in moves:
        species = default_species.get(int(r["pokemon_id"]))
        if not species:
            continue
        group = int(r["version_group_id"])
        move = int(r["move_id"])
        method = int(r["pokemon_move_method_id"])
        if group in GROUPS and 1 <= move <= MAX_MOVE and method in METHODS:
            pools[group][species].add(move)

    for group in GROUPS:
        for species in range(1, MAX_SPECIES + 1):
            ancestor = parent[species]
            guard = 0
            while ancestor and guard < 24:
                pools[group][species].update(pools[group][ancestor])
                ancestor = parent[ancestor]
                guard += 1

    for group, name in ((8, "DiamondPearl"), (9, "Platinum"), (10, "HeartGoldSoulSilver")):
        print(f"// {name}")
        for species in range(MAX_SPECIES + 1):
            words = [0] * 8
            for move in pools[group][species]:
                words[move // 64] |= 1 << (move % 64)
            values = ", ".join(f"0x{x:016x}ULL" for x in words)
            print(f"MoveBits{{{{{values}}}}},")

if __name__ == "__main__":
    main()
