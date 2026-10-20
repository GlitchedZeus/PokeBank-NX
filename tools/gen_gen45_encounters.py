#!/usr/bin/env python3
"""Generate include/Integration/Encounter/Gen45EncounterData.inc from PKHeX (pinned 6501f0ab).

NEW-12 / R2-CHECK-011 (Round 3): the Gen IV / V Met Location picker offered every native location
id (108 in HG/SS, 276 in B2/W2) with no context. This table lists where each species is actually
met in each Generation IV / V game, so the picker can show those first, with the method and level,
and keep every other native location as an egg / trade / event origin (never rejected).

Sources (PKHeX.Core @ 6501f0ab, fetched by tools/pkhex_source.py):
  * Wild slots: Resources/legality/wild/Gen4/encounter_{d,p,pt,hg,ss}.pkl and
    Resources/legality/wild/Gen5/encounter_{b,w,b2,w2}.pkl, BinLinkerAccessor (u32 offsets),
    decoded exactly as Legality/Encounters/Templates/Gen4/EncounterArea4.cs and
    Gen5/EncounterArea5.cs read them.
  * Static / gift encounters with a fixed met location: Legality/Encounters/Data/Gen4/
    Encounters4DPPt.cs, Encounters4HGSS.cs, Gen5/Encounters5BW.cs, Encounters5B2W2.cs
    (`new(<Version>) { ... Species = N, Level = L, Location = X ... }` initialisers). Roamers
    (IsRoaming) and egg gifts (Location = 0 / EggLocation) have no single met location and are
    left to the "other origin" rows, as are in-game trades, Dream World, Pokewalker and events.
  * Pre-evolutions: Resources/byte/evolve/evos_g4.pkl / evos_g5.pkl (EvolutionSet, 8-byte records;
    destination species at +4), reversed so an evolved Pokemon lists where its line is met.

Run by hand: python3 tools/gen_gen45_encounters.py   (writes the .inc; review the diff).
"""
import os
import re
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from pkhex_source import pkhex_path  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "include", "Integration", "Encounter", "Gen45EncounterData.inc")

# Game ids used by the table (Gen45Encounters.h maps PKHeX GameVersion to these).
GAMES = [("d", "Gen4", 1), ("p", "Gen4", 2), ("pt", "Gen4", 3), ("hg", "Gen4", 4), ("ss", "Gen4", 5),
         ("b", "Gen5", 6), ("w", "Gen5", 7), ("b2", "Gen5", 8), ("w2", "Gen5", 9)]
IDENT = {"d": b"da", "p": b"pe", "pt": b"pt", "hg": b"hg", "ss": b"ss",
         "b": b"51", "w": b"51", "b2": b"52", "w2": b"52"}

# Method codes (Gen45Encounters.h methodName): 0..14 = PKHeX SlotType4, 20..25 = SlotType5 + 20,
# 40 = static / gift.
STATIC = 40
VERSION_GAMES = {
    "D": [1], "P": [2], "Pt": [3], "DP": [1, 2], "DPPt": [1, 2, 3],
    "HG": [4], "SS": [5], "HGSS": [4, 5],
    "B": [6], "W": [7], "BW": [6, 7], "B2": [8], "W2": [9], "B2W2": [8, 9],
}
STATIC_FILES = ["Legality/Encounters/Data/Gen4/Encounters4DPPt.cs",
                "Legality/Encounters/Data/Gen4/Encounters4HGSS.cs",
                "Legality/Encounters/Data/Gen5/Encounters5BW.cs",
                "Legality/Encounters/Data/Gen5/Encounters5B2W2.cs"]
MAX_SPECIES = {4: 493, 5: 649}


def load(path):
    with open(path, "rb") as fh:
        return fh.read()


def bin_linker(data, ident):
    """PKHeX BinLinkerAccessor: 2-byte ident, u16 count, (count + 1) u32 offsets."""
    if data[:2] != ident:
        raise SystemExit("unexpected ident %r (want %r)" % (data[:2], ident))
    count = struct.unpack_from("<H", data, 2)[0]
    offs = [struct.unpack_from("<I", data, 4 + i * 4)[0] for i in range(count + 1)]
    return [data[offs[i]:offs[i + 1]] for i in range(count)]


def wild_rows(key, gen, game):
    path = pkhex_path("Resources/legality/wild/%s/encounter_%s.pkl" % (gen, key))
    rows = []
    for area in bin_linker(load(path), IDENT[key]):
        if gen == "Gen4":   # EncounterArea4: location u8, unused, SlotType4, rate, u16 tile; 10-byte slots
            location, method, body, size = area[0], area[2], area[6:], 10
        else:               # EncounterArea5: location u16, SlotType5, unused; 4-byte slots
            location, method, body, size = struct.unpack_from("<H", area, 0)[0], 20 + area[2], area[4:], 4
        for off in range(0, len(body) - size + 1, size):
            if gen == "Gen4":
                species = struct.unpack_from("<H", body, off)[0]
                lo, hi = body[off + 4], body[off + 5]
            else:
                species = struct.unpack_from("<H", body, off)[0] & 0x3FF
                lo, hi = body[off + 2], body[off + 3]
            if species == 0 or species > MAX_SPECIES[4 if gen == "Gen4" else 5]:
                continue
            rows.append((game, species, location, lo, max(lo, hi), method))
    return rows


STATIC_RE = re.compile(r"new\((\w+)\s*\)\s*\{([^{}]*)\}")


def field(body, name):
    m = re.search(r"\b%s\s*=\s*(\w+)" % name, body)
    return m.group(1) if m else None


def static_rows():
    rows = []
    for rel in STATIC_FILES:
        text = load(pkhex_path(rel)).decode("utf-8")
        text = re.sub(r"//[^\n]*", "", text)   # commented-out (unreleased) entries stay out
        for version, body in STATIC_RE.findall(text):
            games = VERSION_GAMES.get(version)
            species, level, location = field(body, "Species"), field(body, "Level"), field(body, "Location")
            if not games or species is None or level is None or location is None:
                continue
            if field(body, "IsRoaming") == "true" or field(body, "EggLocation") is not None:
                continue
            species, level, location = int(species), int(level), int(location)
            if location == 0:
                continue
            for game in games:
                rows.append((game, species, location, level, level, STATIC))
    return rows


def pre_evolutions():
    """{species: parent} from evos_g5.pkl (a superset of g4 for species 1..493)."""
    parent = {}
    for key, maxsp in (("g4", 493), ("g5", 649)):
        data = load(pkhex_path("Resources/byte/evolve/evos_%s.pkl" % key))
        count = struct.unpack_from("<H", data, 2)[0]
        entries = [data[s:e] for (s, e) in (struct.unpack_from("<HH", data, 4 + i * 2) for i in range(count))]
        for sp in range(1, min(maxsp, len(entries) - 1) + 1):
            e = entries[sp]
            for off in range(0, len(e) - 7, 8):
                dest = struct.unpack_from("<H", e, off + 4)[0]
                if 0 < dest <= maxsp and dest not in parent:
                    parent[dest] = sp
    return parent


def main():
    rows = []
    for key, gen, game in GAMES:
        rows += wild_rows(key, gen, game)
    rows += static_rows()
    # Merge identical (game, species, location, method) slots into one level range.
    merged = {}
    for game, species, location, lo, hi, method in rows:
        k = (game, species, location, method)
        if k in merged:
            a, b = merged[k]
            merged[k] = (min(a, lo), max(b, hi))
        else:
            merged[k] = (lo, hi)
    out = sorted((k[0], k[1], k[2], v[0], v[1], k[3]) for k, v in merged.items())
    parent = pre_evolutions()
    with open(OUT, "w", encoding="utf-8") as fh:
        fh.write("// GENERATED by tools/gen_gen45_encounters.py from PKHeX.Core 6501f0ab -- do not edit.\n")
        fh.write("// {game, species, met location, min level, max level, method}; see Gen45Encounters.h.\n")
        fh.write("inline constexpr Gen45Encounter kGen45Encounters[] = {\n")
        for r in out:
            fh.write("    {%d, %d, %d, %d, %d, %d},\n" % r)
        fh.write("};\n\n")
        fh.write("// Pre-evolution (0 = none) per National Dex species 0..649 (PKHeX evos_g4 / evos_g5).\n")
        fh.write("inline constexpr uint16_t kGen45PreEvolution[650] = {\n")
        vals = [parent.get(sp, 0) for sp in range(650)]
        for i in range(0, 650, 16):
            fh.write("    " + ", ".join(str(v) for v in vals[i:i + 16]) + ",\n")
        fh.write("};\n")
    print("wrote %s: %d encounter rows, %d pre-evolution links" % (OUT, len(out), len(parent)))


if __name__ == "__main__":
    main()
