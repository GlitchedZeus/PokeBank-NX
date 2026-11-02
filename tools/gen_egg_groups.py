#!/usr/bin/env python3
"""Generate include/Data/EggGroups.h: Gen II-V egg groups, gender ratios and pre-evolutions.

Round 13 owner decision 5 ("Add proper egg-group and breeding compatibility data. No generic egg
histories presented as verified."). The table feeds include/Integration/Encounter/BreedingPolicy.h.

PRIMARY SOURCE (pinned, tools/pkhex_source.py):
  kwsch/PKHeX 6501f0ab46e8f8ca048539dbaf8cae8cb104e722
    PKHeX.Core/Resources/byte/personal/personal_{gs,c}       PersonalInfo2.cs  size 0x20, gender 0x0D,
                                                              egg groups 0x17 (high / low nibble)
    PKHeX.Core/Resources/byte/personal/personal_{rs,e,fr,lg}  PersonalInfo3.cs  size 0x1C, gender 0x10,
                                                              egg groups 0x14 / 0x15
    PKHeX.Core/Resources/byte/personal/personal_{dp,pt,hgss}  PersonalInfo4.cs  size 0x2C, gender 0x10,
                                                              egg groups 0x14 / 0x15
    PKHeX.Core/Resources/byte/personal/personal_{bw,b2w2}     PersonalInfo5BW / 5B2W2.cs  size 0x3C /
                                                              0x4C, gender 0x12, egg groups 0x16 / 0x17
    PKHeX.Core/Resources/byte/evolve/evos_g4.pkl, evos_g5.pkl (pre-evolution edges; Gen II / III
    use the Gen IV edges restricted to species <= 251 / <= 386: no Gen II/III family gained a member
    numbered <= 386 after its own generation)
  Every game of a generation must agree (G/S = C, R/S = E = FR = LG, D/P = Pt = HG/SS, B/W = B2/W2);
  the script stops if one does not.

CROSS-CHECKS (independent decompilations; the script stops on any egg-group mismatch):
  pret/pokecrystal 3c813b0b4d82b39bf62185aceffb76fd6026c70b
    data/pokemon/base_stats.asm + data/pokemon/base_stats/*.asm ("dn EGG_x, EGG_y ; egg groups")
    constants/pokemon_data_constants.asm (EGG_* numbering)
  pret/pokeemerald 731ad5bfd6e6f265508d0efcca0ba42f9dcf5881
    src/data/pokemon/species_info.h (.eggGroups), include/constants/pokemon.h (EGG_GROUP_*)
  Species names for the Emerald mapping: PKHeX Resources/text/other/en/text_Species_en.txt.

Egg-group numbering is the one all three sources share: 1 Monster, 2 Water 1, 3 Bug, 4 Flying,
5 Field, 6 Fairy, 7 Grass, 8 Human-Like, 9 Water 3, 10 Mineral, 11 Amorphous, 12 Water 2,
13 Ditto, 14 Dragon, 15 Undiscovered. Gender: 0 male-only, 254 female-only, 255 genderless.

Usage: python3 tools/gen_egg_groups.py [--check]   (network on first run; cached afterwards)
"""
import os
import re
import struct
import sys
import urllib.request

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from pkhex_source import pkhex_path, _REF as PKHEX_REF  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "include", "Data", "EggGroups.h")
PRET_CACHE = os.path.join(ROOT, "tools", ".pret_cache")

POKECRYSTAL = ("pret/pokecrystal", "3c813b0b4d82b39bf62185aceffb76fd6026c70b")
POKEEMERALD = ("pret/pokeemerald", "731ad5bfd6e6f265508d0efcca0ba42f9dcf5881")

# generation -> (max species, [(personal resource, entry size, gender offset, egg offset, nibbles)])
GENERATIONS = {
    2: (251, [("personal_gs", 0x20, 0x0D, 0x17, True), ("personal_c", 0x20, 0x0D, 0x17, True)]),
    3: (386, [(n, 0x1C, 0x10, 0x14, False) for n in ("personal_rs", "personal_e", "personal_fr", "personal_lg")]),
    4: (493, [(n, 0x2C, 0x10, 0x14, False) for n in ("personal_dp", "personal_pt", "personal_hgss")]),
    5: (649, [("personal_bw", 0x3C, 0x12, 0x16, False), ("personal_b2w2", 0x4C, 0x12, 0x16, False)]),
}
EGG_NAMES = {1: "Monster", 2: "Water 1", 3: "Bug", 4: "Flying", 5: "Field", 6: "Fairy", 7: "Grass",
             8: "Human-Like", 9: "Water 3", 10: "Mineral", 11: "Amorphous", 12: "Water 2", 13: "Ditto",
             14: "Dragon", 15: "Undiscovered"}


def fetch_pret(repo, ref, path):
    dest = os.path.join(PRET_CACHE, repo.replace("/", "_"), ref, *path.split("/"))
    if not os.path.exists(dest):
        url = f"https://raw.githubusercontent.com/{repo}/{ref}/{path}"
        sys.stderr.write(f"  fetch {url}\n")
        with urllib.request.urlopen(url) as resp:
            data = resp.read()
        os.makedirs(os.path.dirname(dest), exist_ok=True)
        with open(dest + ".part", "wb") as fh:
            fh.write(data)
        os.replace(dest + ".part", dest)
    with open(dest, "rb") as fh:
        return fh.read().decode("utf-8")


def personal(resource, size, gender_at, egg_at, nibbles, maximum):
    with open(pkhex_path(f"Resources/byte/personal/{resource}"), "rb") as fh:
        data = fh.read()
    if len(data) < (maximum + 1) * size:
        raise SystemExit(f"{resource}: short table")
    rows = [None]
    for species in range(1, maximum + 1):
        entry = data[species * size:(species + 1) * size]
        if nibbles:
            # pret "dn EGG_a, EGG_b" stores a in the HIGH nibble (PKHeX's EggGroup2); keep the
            # decomp's order so Gen II rows read like Gen III's (Bulbasaur: Monster, Grass).
            egg1, egg2 = entry[egg_at] >> 4, entry[egg_at] & 0xF
        else:
            egg1, egg2 = entry[egg_at], entry[egg_at + 1]
        if egg1 not in EGG_NAMES or egg2 not in EGG_NAMES:
            raise SystemExit(f"{resource}: species {species} egg group {egg1}/{egg2} out of range")
        rows.append((egg1, egg2, entry[gender_at]))
    return rows


def binlinker(data):
    count = struct.unpack_from("<H", data, 2)[0]
    offsets = struct.unpack_from("<" + "H" * (count + 1), data, 4)
    return [data[a:b] for a, b in zip(offsets, offsets[1:])]


def prevos(gen, maximum):
    # Same format tools/gen_gen45_encounter_context.py reads: one entry per species, 8 bytes per
    # evolution, the evolved species at +4.
    with open(pkhex_path(f"Resources/byte/evolve/evos_g{gen}.pkl"), "rb") as fh:
        raw = fh.read()
    parent = [0] * (maximum + 1)
    for species, data in enumerate(binlinker(raw)):
        if len(data) % 8:
            raise SystemExit(f"evos_g{gen}: bad entry size")
        for offset in range(0, len(data), 8):
            child = struct.unpack_from("<H", data, offset + 4)[0]
            if not (0 < species <= maximum and 0 < child <= maximum):
                continue
            if parent[child] not in (0, species):
                raise SystemExit(f"evos_g{gen}: species {child} has two pre-evolutions")
            parent[child] = species
    return parent


def crystal_cross_check(rows):
    consts = fetch_pret(*POKECRYSTAL, "constants/pokemon_data_constants.asm")
    names = re.findall(r"^\s*const (EGG_\w+)\s*;\s*([0-9a-f]+)", consts, re.M)
    egg = {name: int(value, 16) for name, value in names}
    index = fetch_pret(*POKECRYSTAL, "data/pokemon/base_stats.asm")
    files = re.findall(r'INCLUDE "(data/pokemon/base_stats/[\w]+\.asm)"', index)
    if len(files) != 251:
        raise SystemExit(f"pokecrystal: expected 251 base stats files, found {len(files)}")
    for path in files:
        text = fetch_pret(*POKECRYSTAL, path)
        dex = int(re.search(r"^\s*db \w+ ; (\d+)", text, re.M).group(1))
        a, b = re.search(r"^\s*dn (EGG_\w+), (EGG_\w+) ; egg groups", text, re.M).groups()
        if (egg[a], egg[b]) != rows[dex][:2]:
            raise SystemExit(f"pokecrystal {path}: {a}/{b} != PKHeX {rows[dex][:2]}")
    return len(files)


def norm(name):
    name = name.replace("♀", "F").replace("♂", "M")
    return re.sub(r"[^A-Z0-9]", "", name.upper())


def emerald_cross_check(rows):
    consts = fetch_pret(*POKEEMERALD, "include/constants/pokemon.h")
    egg = {m.group(1): int(m.group(2)) for m in re.finditer(r"#define (EGG_GROUP_\w+)\s+(\d+)", consts)}
    info = fetch_pret(*POKEEMERALD, "src/data/pokemon/species_info.h")
    with open(pkhex_path("Resources/text/other/en/text_Species_en.txt"), encoding="utf-8-sig") as fh:
        species_names = [line.strip() for line in fh]
    dex_by_name = {norm(species_names[i]): i for i in range(1, 387)}
    checked = 0
    # Each "[SPECIES_X] =" entry runs to the next one (the first and last entries included).
    parts = re.split(r"\[SPECIES_(\w+)\]\s*=", info)
    for name, body in zip(parts[1::2], parts[2::2]):
        key = re.sub(r"[^A-Z0-9]", "", name)
        groups = re.search(r"\.eggGroups\s*=\s*\{\s*(EGG_GROUP_\w+)\s*,\s*(EGG_GROUP_\w+)", body)
        if key not in dex_by_name or not groups:
            continue
        dex = dex_by_name[key]
        got = (egg[groups.group(1)], egg[groups.group(2)])
        if got != rows[dex][:2]:
            raise SystemExit(f"pokeemerald SPECIES_{name}: {got} != PKHeX {rows[dex][:2]}")
        checked += 1
    if checked != 386:
        raise SystemExit(f"pokeemerald: only {checked} species cross-checked")
    return checked


def generate():
    tables, parents, notes = {}, {}, []
    gen4_parent = prevos(4, 493)
    for gen, (maximum, resources) in GENERATIONS.items():
        reference = None
        for resource, size, gender_at, egg_at, nibbles in resources:
            rows = personal(resource, size, gender_at, egg_at, nibbles, maximum)
            if reference is None:
                reference = rows
            elif rows != reference:
                diff = [s for s in range(1, maximum + 1) if rows[s] != reference[s]]
                raise SystemExit(f"Gen {gen}: {resource} disagrees with {resources[0][0]} for {diff[:10]}")
        tables[gen] = reference
        parents[gen] = prevos(5, 649) if gen == 5 else [p if p <= maximum and s <= maximum else 0
                                                       for s, p in enumerate(gen4_parent[:maximum + 1])]
    crystal = crystal_cross_check(tables[2])
    emerald = emerald_cross_check(tables[3])
    for gen in (3, 4, 5):
        prior = tables[gen - 1]
        changed = [s for s in range(1, len(prior)) if prior[s][:2] != tables[gen][s][:2]]
        notes.append(f"Gen {gen} egg groups differing from Gen {gen - 1}: {changed if changed else 'none'}")
    lines = [
        "#pragma once",
        "// GENERATED by tools/gen_egg_groups.py; do not edit.",
        f"// Primary source: kwsch/PKHeX {PKHEX_REF} personal_{{gs,c,rs,e,fr,lg,dp,pt,hgss,bw,b2w2}},",
        "//   evos_g4.pkl / evos_g5.pkl (pre-evolutions; Gen II / III = Gen IV edges within their dex).",
        f"// Cross-checked: pret/pokecrystal {POKECRYSTAL[1]} ({crystal} species),",
        f"//   pret/pokeemerald {POKEEMERALD[1]} ({emerald} species): egg groups identical.",
        "// Every game of a generation agrees (G/S = C, R/S = E = FR/LG, D/P = Pt = HG/SS, B/W = B2/W2).",
    ] + [f"// {n}" for n in notes] + [
        "// Row: {egg group 1, egg group 2, gender ratio, pre-evolution (0 = none in this generation)}.",
        "// Egg groups: 1 Monster 2 Water1 3 Bug 4 Flying 5 Field 6 Fairy 7 Grass 8 Human-Like 9 Water3",
        "//   10 Mineral 11 Amorphous 12 Water2 13 Ditto 14 Dragon 15 Undiscovered.",
        "// Gender ratio: 0 male only, 254 female only, 255 genderless (PKHeX PersonalInfo.Gender).",
        "#include <cstdint>",
        "",
        "namespace PokeVault::Data::EggGroups {",
        "",
        "struct Row { uint8_t egg1, egg2, gender; uint16_t prevo; };",
        "",
    ]
    for gen, (maximum, _) in GENERATIONS.items():
        lines.append(f"inline constexpr Row kGen{gen}[{maximum + 1}] = {{")
        lines.append("    {0, 0, 0, 0},")
        rows = tables[gen]
        for s in range(1, maximum + 1):
            e1, e2, g = rows[s]
            lines.append(f"    {{{e1}, {e2}, {g}, {parents[gen][s]}}},  // {s}")
        lines.append("};")
        lines.append("")
    lines += [
        "// The table of a generation (2-5) and its National Dex size; nullptr / 0 otherwise.",
        "constexpr const Row* table(uint8_t generation) noexcept {",
        "    return generation == 2 ? kGen2 : generation == 3 ? kGen3 : generation == 4 ? kGen4 :",
        "           generation == 5 ? kGen5 : nullptr;",
        "}",
        "constexpr uint16_t maxSpecies(uint8_t generation) noexcept {",
        "    return generation == 2 ? 251 : generation == 3 ? 386 : generation == 4 ? 493 :",
        "           generation == 5 ? 649 : 0;",
        "}",
        "",
        "} // namespace PokeVault::Data::EggGroups",
        "",
    ]
    return "\n".join(lines)


if __name__ == "__main__":
    content = generate()
    if "--check" in sys.argv:
        with open(OUT, encoding="utf-8") as fh:
            if fh.read() != content:
                raise SystemExit(f"not reproducible: {OUT}")
        print("EggGroups.h reproducible")
    else:
        os.makedirs(os.path.dirname(OUT), exist_ok=True)
        with open(OUT, "w", encoding="utf-8") as fh:
            fh.write(content)
        print(f"wrote {OUT}")
