#!/usr/bin/env python3
"""Cross-check the Gen I legality tables against the pret disassemblies.

tools/gen_gb_legality_data.py takes its data from PKHeX. This script compares
that same PKHeX data with an independent source, the pret/pokered and
pret/pokeyellow disassemblies, so a transcription mistake on either side shows
up as a mismatch instead of a wrong verdict.

Checked per game (Red/Blue and Yellow): base stats, types, catch rate, growth
rate, level-1 moves, TM/HM compatibility, level-up learnsets, evolutions, and
the lowest wild level of every species (grass, surf and all three rods).

Usage:
    python3 tools/verify_gb_legality_data.py --pkhex PKHEX --pokered RED --pokeyellow YELLOW

Exits non-zero and lists every difference when the sources disagree.
"""

import argparse
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import gen_gb_legality_data as gen  # noqa: E402

RED, BLUE, YELLOW = "red", "blue", "yellow"

# PKHeX stores the catch-rate byte a Pokemon actually carries, which the game
# sometimes overrides after reading its base-stat table:
EXPLAINED = {
    # engine/pokemon/add_mon.asm: every Kadabra added in Yellow gets TWISTEDSPOON_GSC.
    "yellow #64 catch: PKHeX 96 pret 100",
    # scripts/OaksLab.asm: the starter Pikachu gets LIGHT_BALL_GSC, and Yellow
    # has no other way to obtain a Pikachu.
    "yellow #25 catch: PKHeX 163 pret 190",
}


def consts(path, start=0):
    """Parse a pret `const_def` block into name -> value."""
    values, value = {}, start
    for line in path.read_text().splitlines():
        line = line.split(";", 1)[0].strip()
        m = re.match(r"const_def\s*(\d+)?", line)
        if m:
            value = int(m.group(1) or 0)
            continue
        m = re.match(r"const_next\s+(\d+)", line)
        if m:
            value = int(m.group(1))
            continue
        if line.startswith("const_skip"):
            value += 1
            continue
        m = re.match(r"const\s+(\w+)", line)
        if m:
            values[m.group(1)] = value
            value += 1
    return values


class Pret:
    def __init__(self, root):
        self.root = root
        self.moves = consts(root / "constants/move_constants.asm")
        self.types = consts(root / "constants/type_constants.asm")
        self.dex = {k[4:]: v for k, v in consts(root / "constants/pokedex_constants.asm").items()
                    if k.startswith("DEX_")}
        self.internal = consts(root / "constants/pokemon_constants.asm")
        growth = consts(root / "constants/pokemon_data_constants.asm")
        self.growth = {k: v for k, v in growth.items() if k.startswith("GROWTH_")}

    def species(self, name):
        return self.dex[name]

    def base_stats(self):
        out = {}
        for f in sorted((self.root / "data/pokemon/base_stats").glob("*.asm")):
            text = f.read_text()
            dex = self.dex[re.search(r"db DEX_(\w+)", text).group(1)]
            dbs = [l.split(";", 1)[0].strip() for l in text.splitlines()]
            dbs = [l[3:].strip() for l in dbs if l.startswith("db ")]
            stats = [int(x) for x in dbs[1].split(",")]
            t1, t2 = [x.strip() for x in dbs[2].split(",")]
            catch = int(dbs[3])
            moves = [self.moves[x.strip()] for x in dbs[5].split(",")]
            growth = self.growth[dbs[6]]
            tm_text = re.search(r"tmhm(.*?); end", text, re.S).group(1)
            # Mew sets the unused 56th bit, which pret spells UNUSED.
            tms = [self.moves[x] for x in re.findall(r"[A-Z][A-Z0-9_]+", tm_text) if x != "UNUSED"]
            out[dex] = {"stats": stats, "types": (self.types[t1], self.types[t2]),
                        "catch": catch, "moves": moves, "growth": growth, "tms": sorted(tms)}
        return out

    def evos_moves(self):
        text = (self.root / "data/pokemon/evos_moves.asm").read_text()
        order = re.findall(r"^\s*dw (\w+)EvosMoves", text, re.M)
        by_internal = {v: k for k, v in self.internal.items()}
        label_to_dex = {}
        for i, label in enumerate(order, start=1):
            name = by_internal.get(i)  # MissingNo slots have no constant
            if name in self.dex:
                label_to_dex[label] = self.dex[name]
        evos, learn = {}, {}
        for label, body in re.findall(r"^(\w+)EvosMoves:\n(.*?)(?=^\w+EvosMoves:|\Z)", text, re.M | re.S):
            if label not in label_to_dex:
                continue
            dex = label_to_dex[label]
            lines = [l.split(";", 1)[0].strip() for l in body.splitlines()]
            lines = [l[3:].strip() for l in lines if l.startswith("db ")]
            stop = lines.index("0")
            ev = []
            for l in lines[:stop]:
                parts = [p.strip() for p in l.split(",")]
                if parts[0] == "EVOLVE_LEVEL":
                    ev.append((4, int(parts[1]), self.dex[parts[2]]))
                elif parts[0] == "EVOLVE_ITEM":
                    ev.append((8, None, self.dex[parts[3]]))
                elif parts[0] == "EVOLVE_TRADE":
                    ev.append((5, None, self.dex[parts[2]]))
            ls = []
            for l in lines[stop + 1:]:
                if l == "0":
                    break
                lv, mv = [p.strip() for p in l.split(",")]
                ls.append((int(lv), self.moves[mv]))
            evos[dex], learn[dex] = sorted(ev), ls
        return evos, learn

    def wild_min(self, version):
        """species -> lowest wild level for one version."""
        best = {}

        def add(species, level):
            dex = self.dex[species]
            best[dex] = min(level, best.get(dex, 255))

        for f in sorted((self.root / "data/wild/maps").glob("*.asm")):
            active = [True]
            for raw in f.read_text().splitlines():
                line = raw.split(";", 1)[0].strip()
                m = re.match(r"IF DEF\(_(\w+)\)", line)
                if m:
                    active.append(m.group(1).lower() == version)
                    continue
                if line == "ELSE":
                    active[-1] = not active[-1]
                    continue
                if line == "ENDC":
                    active.pop()
                    continue
                m = re.match(r"db\s+(\d+),\s*(\w+)", line)
                if m and all(active):
                    add(m.group(2), int(m.group(1)))
        add("MAGIKARP", 5)  # Old Rod: fixed in the item handler
        for lv, sp in re.findall(r"db (\d+), (\w+)", (self.root / "data/wild/good_rod.asm").read_text()):
            add(sp, int(lv))
        rod = (self.root / "data/wild/super_rod.asm").read_text()
        if version == YELLOW:
            for line in re.findall(r"^\s*db [A-Z_0-9]+, (.*)$", rod, re.M):
                parts = [p.strip() for p in line.split(",")]
                for sp, lv in zip(parts[0::2], parts[1::2]):
                    add(sp, int(lv))
        else:
            used = set(re.findall(r"dbw \w+,\s*\.(Group\d+)", rod))
            for group, body in re.findall(r"^\.(Group\d+):\n(.*?)(?=^\.Group|\Z)", rod, re.M | re.S):
                if group in used:
                    for lv, sp in re.findall(r"db (\d+), (\w+)", body):
                        add(sp, int(lv))
        return best


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--pkhex", required=True, type=Path)
    ap.add_argument("--pokered", required=True, type=Path)
    ap.add_argument("--pokeyellow", required=True, type=Path)
    args = ap.parse_args()

    res = args.pkhex / "PKHeX.Core/Resources"
    pk_personal = {RED: gen.read_personal1(res / "byte/personal/personal_rb"),
                   YELLOW: gen.read_personal1(res / "byte/personal/personal_y")}
    pk_learn = {RED: gen.read_learnsets(res / "byte/levelup/lvlmove_rb.pkl", b"rb"),
                YELLOW: gen.read_learnsets(res / "byte/levelup/lvlmove_y.pkl", b"yw")}
    pk_evos = gen.read_evolutions(res / "byte/evolve/evos_g1.pkl", b"g1")
    pk_wild = {}
    for fname, ver in (("encounter_red.pkl", RED), ("encounter_blue.pkl", BLUE),
                       ("encounter_yellow.pkl", YELLOW)):
        best = {}
        for s in gen.read_areas(res / "legality/wild/Gen1" / fname, b"g1", 1):
            best[s[0]] = min(s[1], best.get(s[0], 255))
        pk_wild[ver] = best

    errors = []
    checked = 0
    for ver, root in ((RED, args.pokered), (YELLOW, args.pokeyellow)):
        pret = Pret(root)
        for dex, bs in pret.base_stats().items():
            pk = pk_personal[ver][dex]
            tms = sorted(m for i, m in enumerate(
                [5, 13, 14, 18, 25, 92, 32, 34, 36, 38, 61, 55, 58, 59, 63, 6, 66, 68, 69, 99,
                 72, 76, 82, 85, 87, 89, 90, 91, 94, 100, 102, 104, 115, 117, 118, 120, 121, 126,
                 129, 130, 135, 138, 143, 156, 86, 149, 153, 157, 161, 164, 15, 19, 57, 70, 148])
                if pk["tm"][i >> 3] & (1 << (i & 7)))
            mine = {"stats": pk["stats"], "types": pk["types"], "catch": pk["catch"],
                    "moves": pk["moves"], "growth": pk["growth"], "tms": tms}
            for key, value in bs.items():
                checked += 1
                if mine[key] != value:
                    errors.append(f"{ver} #{dex} {key}: PKHeX {mine[key]} pret {value}")
        evos, learn = pret.evos_moves()
        for dex in range(1, 152):
            checked += 2
            if pk_learn[ver][dex] != learn[dex]:
                errors.append(f"{ver} #{dex} learnset: PKHeX {pk_learn[ver][dex]} pret {learn[dex]}")
            if ver == YELLOW:  # PKHeX keeps one Gen I evolution table, built from Yellow
                mine = sorted((t, lv if t == 4 else None, to) for t, _a, to, _f, lv in pk_evos[dex])
                if mine != evos[dex]:
                    errors.append(f"evolutions #{dex}: PKHeX {mine} pret {evos[dex]}")
        for wver in ((RED, BLUE) if ver == RED else (YELLOW,)):
            want = pret.wild_min(wver)
            have = pk_wild[wver]
            for dex in sorted(set(want) | set(have)):
                checked += 1
                if want.get(dex) != have.get(dex):
                    errors.append(f"{wver} wild #{dex} min level: PKHeX {have.get(dex)} pret {want.get(dex)}")

    explained = [e for e in errors if e in EXPLAINED]
    errors = [e for e in errors if e not in EXPLAINED]
    for e in explained:
        print("explained", e)
    for e in errors:
        print("MISMATCH", e)
    print(f"{checked} Gen I comparisons, {len(explained)} explained, {len(errors)} mismatches")
    sys.exit(1 if errors else 0)


if __name__ == "__main__":
    main()
