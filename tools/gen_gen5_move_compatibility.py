#!/usr/bin/env python3
"""Regenerate BW/B2W2 MoveBits rows from pinned PokeAPI CSVs.
PIN: 168b1e89467054cda2e7df43ccebbb69b459497a
usage: python3 tools/gen_gen5_move_compatibility.py /path/to/pinned/data/v2/csv
Prints two 650-row tables; do not network-fetch during application builds.
"""
import csv,pathlib,sys
def rows(root,name):
    with (root/name).open(newline="",encoding="utf-8") as f:return list(csv.DictReader(f))
def main():
    if len(sys.argv)!=2:raise SystemExit(__doc__)
    root=pathlib.Path(sys.argv[1])
    ids={int(r["id"]):int(r["species_id"]) for r in rows(root,"pokemon.csv")
         if r["is_default"]=="1" and int(r["species_id"])<=649}
    parents={int(r["id"]):int(r["evolves_from_species_id"] or 0)
         for r in rows(root,"pokemon_species.csv") if int(r["id"])<=649}
    pools={g:[set() for _ in range(650)] for g in (11,14)}
    for r in rows(root,"pokemon_moves.csv"):
        species=ids.get(int(r["pokemon_id"]));group=int(r["version_group_id"])
        move=int(r["move_id"]);method=int(r["pokemon_move_method_id"])
        if species and group in pools and 1<=move<=559 and method in (1,2,3,4,6):
            pools[group][species].add(move)
    for group in (11,14):
        for species in range(1,650):
            p=parents.get(species,0);seen=set()
            while p and p not in seen and len(seen)<24:
                seen.add(p);pools[group][species].update(pools[group][p])
                p=parents.get(p,0)
    for group,name in ((11,"BlackWhite"),(14,"Black2White2")):
        print("//",name)
        for species in range(650):
            words=[0]*9
            for move in pools[group][species]:words[move//64]|=1<<(move%64)
            print("    MoveBits{{"+", ".join(f"0x{x:016x}ULL" for x in words)+"}}, //",species)
if __name__=="__main__":main()
