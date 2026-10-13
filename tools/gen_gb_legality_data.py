#!/usr/bin/env python3
"""Generate the Game Boy (Gen I/II) legality tables from a pinned PKHeX checkout.

PKHeX.Core is the reference legality implementation the project measures itself
against, so its own Gen I/II resources are the input: personal data, level-up and
egg-move learnsets, evolution trees, wild encounter areas, Gen I event gifts and
the static / in-game-trade encounter lists from its C# encounter definitions.

PKHeX is GPL-3.0; PokeBank NX is AGPL-3.0, which GPL-3.0 section 13 permits
combining with. The generated files carry that attribution.

Usage:
    python3 tools/gen_gb_legality_data.py --pkhex /path/to/PKHeX

Writes include/Legality/GBLegalityData.h and src/Legality/GBLegalityData.cpp.
Output is deterministic; rerunning against the same PKHeX commit is a no-op.
"""

import argparse
import re
import struct
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent

# Version bits shared by Gen I and Gen II data (match Legality::GBData::Version).
RD, GN, BU, YW = 0x01, 0x02, 0x04, 0x08   # Red, Green(JP)/Blue(INT), Blue(JP), Yellow
GD, SI, C = 0x10, 0x20, 0x40               # Gold, Silver, Crystal
VERSION_GROUPS = {
    "RD": RD, "GN": GN, "BU": BU, "YW": YW,
    "RB": RD | GN | BU, "RBY": RD | GN | BU | YW,
    "GD": GD, "SI": SI, "C": C, "GS": GD | SI, "GSC": GD | SI | C,
}

LANG_CODES = {1: "ja", 2: "en", 3: "fr", 4: "it", 5: "de", 7: "es", 8: "ko"}


def die(msg):
    sys.stderr.write(f"error: {msg}\n")
    sys.exit(1)


# --------------------------------------------------------------------------- readers

def binlinker16(data, ident):
    if data[:2] != ident:
        die(f"bad BinLinker16 ident {data[:2]!r}, expected {ident!r}")
    count = struct.unpack_from("<H", data, 2)[0]
    out = []
    for i in range(count):
        start, end = struct.unpack_from("<HH", data, 4 + i * 2)
        out.append(data[start:end])
    return out


def binlinker32(data, ident):
    if data[:2] != ident:
        die(f"bad BinLinker ident {data[:2]!r}, expected {ident!r}")
    count = struct.unpack_from("<H", data, 2)[0]
    out = []
    for i in range(count):
        start, end = struct.unpack_from("<II", data, 4 + i * 4)
        out.append(data[start:end])
    return out


def read_learnsets(path, ident):
    """species -> [(level, move)] in game order."""
    entries = binlinker16(path.read_bytes(), ident)
    result = [[]]
    for entry in entries[1:]:
        count = len(entry) // 3
        moves = struct.unpack_from(f"<{count}H", entry, 0)
        levels = entry[count * 2:count * 3]
        result.append(list(zip(levels, moves)))
    return result


def read_move_lists(path, ident):
    entries = binlinker16(path.read_bytes(), ident)
    result = [[]]
    for entry in entries[1:]:
        result.append(list(struct.unpack_from(f"<{len(entry) // 2}H", entry, 0)))
    return result


def read_evolutions(path, ident):
    """species -> [(type, argument, toSpecies, form, level)]."""
    entries = binlinker16(path.read_bytes(), ident)
    result = []
    for entry in entries:
        evos = []
        for off in range(0, len(entry), 8):
            etype = entry[off]
            arg, species = struct.unpack_from("<HH", entry, off + 2)
            evos.append((etype, arg, species, entry[off + 6], entry[off + 7]))
        result.append(evos)
    return result


def read_personal1(path):
    data = path.read_bytes()
    size = 0x1C
    out = []
    for i in range(len(data) // size):
        r = data[i * size:(i + 1) * size]
        out.append({
            "stats": list(r[1:6]),           # HP ATK DEF SPE SPC
            "types": (r[6], r[7]),
            "catch": r[8],
            "moves": list(r[0x0F:0x13]),
            "growth": r[0x13],
            "tm": list(r[0x14:0x14 + 7]),     # 55 bits
        })
    return out


def read_personal2(path):
    data = path.read_bytes()
    size = 0x20
    out = []
    for i in range(len(data) // size):
        r = data[i * size:(i + 1) * size]
        out.append({
            "stats": list(r[1:7]),           # HP ATK DEF SPE SPA SPD
            "types": (r[7], r[8]),
            "catch": r[9],
            "items": (r[0x0B], r[0x0C]),
            "gender": r[0x0D],
            "hatch": r[0x0F],
            "growth": r[0x16],
            "eggGroups": r[0x17],
            "tm": list(r[0x18:0x18 + 8]),     # 57 TM/HM bits then 3 Crystal tutor bits
        })
    return out


def read_areas(path, ident, gen):
    """Flatten encounter areas into per-slot tuples."""
    slots = []
    for area in binlinker32(path.read_bytes(), ident):
        location = area[0]
        if gen == 1:
            time, stype, data = 0, area[2], area[4:]
            per = 4
            count = len(data) // per
            body = data
        else:
            time, stype = area[1], area[2]
            data = area[4:]
            if stype > 1:  # not grass/surf: per-slot rates precede the slots
                count = len(data) // 5
                body = data[count:]
            else:
                count = len(data) // 4
                body = data
        for i in range(count):
            species, _slot, lmin, lmax = body[i * 4:i * 4 + 4]
            slots.append((species, lmin, lmax, stype, location, time))
    return slots


def read_text_lines(path):
    return path.read_text(encoding="utf-8-sig").splitlines()  # some files carry a BOM


def read_stadium2(path):
    """species -> [(level, move, sourceFlags)] from Stadium 2's move reminder table."""
    result = []
    for entry in binlinker16(path.read_bytes(), b"s2"):
        # Entries are padded to an even length; like PKHeX, ignore the leftover byte.
        result.append([tuple(entry[i:i + 3]) for i in range(0, len(entry) - len(entry) % 3, 3)])
    return result


# --------------------------------------------------------------------------- C# parsing

def cs_array(source, name):
    """Return the body of `name = [ ... ];` from a C# file with // comments removed."""
    m = re.search(rf"\b{name}\s*=\s*\[(.*?)\n\s*\];", source, re.S)
    if not m:
        die(f"cannot find C# array {name}")
    lines = []
    for line in m.group(1).splitlines():
        lines.append(line.split("//", 1)[0])
    return "\n".join(lines)


def parse_static1(source, name):
    body = cs_array(source, name)
    out = []
    for species, level, ver in re.findall(r"new\(\s*(\d+)\s*,\s*(\d+)\s*,\s*(\w+)\s*\)", body):
        out.append((int(species), int(level), VERSION_GROUPS[ver]))
    return out


def parse_trade1(source, name):
    body = cs_array(source, name)
    out = []
    pat = re.compile(r"new\(TradeNames,\s*(\d+),\s*(\d+),\s*(\w+),\s*(\d+)(?:,\s*(\d+))?\s*\)"
                     r"(\s*\{\s*EvolveOnTrade\s*=\s*true\s*\})?")
    for idx, species, ver, rby, gsc, evolve in pat.findall(body):
        out.append((int(species), VERSION_GROUPS[ver], int(rby), int(gsc or rby),
                    int(idx), bool(evolve)))
    return out


MOVE_IDS = {
    "Wrap": 35, "ThunderWave": 86, "Twister": 239, "ExtremeSpeed": 245,
    "ThunderShock": 84, "Charm": 204, "DizzyPunch": 146, "Pound": 1, "Sing": 47,
    "Tackle": 33, "Lick": 122, "QuickAttack": 98, "Leer": 43, "Ember": 52,
}


def parse_static2(source, name):
    body = cs_array(source, name)
    out = []
    # One entry per `new(...)` up to the matching `}` or `,` that closes it.
    for m in re.finditer(r"new\(\s*(\d+)\s*,\s*(\d+)\s*,\s*(\w+)\s*\)\s*(\{[^}]*\})?", body):
        species, level, ver, init = int(m.group(1)), int(m.group(2)), m.group(3), m.group(4) or ""
        loc = re.search(r"Location\s*=\s*(\d+)", init)
        moves = re.search(r"Moves\s*=\s*new\(((?:\s*\(int\)Move\.\w+\s*,?)+)\s*\)", init)
        mv = []
        if moves:
            for tok in moves.group(1).split(","):
                tok = tok.strip().replace("(int)Move.", "")
                if tok:
                    mv.append(MOVE_IDS[tok])
        dvs = re.search(r"IVs\s*=\s*(new\(([^)]*)\)|AllZero|Shiny2)", init)
        fixed = None
        if dvs:
            if dvs.group(1) == "AllZero":
                fixed = (0, 0, 0, 0, 0, 0)
            elif dvs.group(1) == "Shiny2":
                fixed = (0, 2, 10, 10, 10, 10)
            else:
                fixed = tuple(int(x) for x in dvs.group(2).split(","))
        out.append({
            "species": species, "level": level, "version": VERSION_GROUPS[ver],
            "location": int(loc.group(1)) if loc else 0,
            "egg": "IsEgg = true" in init,
            "shinyAlways": "Shiny = Shiny.Always" in init,
            "shinyNever": "Shiny = Shiny.Never" in init,
            "gender": int(re.search(r"Gender\s*=\s*(\d+)", init).group(1)) if "Gender" in init else 255,
            "moves": mv, "dvs": fixed,
        })
    return out


def cs_numbers(source, name):
    """Numbers of a `Name => [ ... ];` C# span, comments removed."""
    m = re.search(rf"\b{name}\s*=>\s*\[(.*?)\];", source, re.S)
    if not m:
        die(f"cannot find C# span {name}")
    body = "\n".join(line.split("//", 1)[0] for line in m.group(1).splitlines())
    return [int(x) for x in re.findall(r"\d+", body)]


def parse_trade2(source):
    body = cs_array(source, "TradeGift_GSC")
    out = []
    pat = re.compile(r"new\(TradeNames,\s*(\d+),\s*(\d+),\s*(\d+),\s*(\d+)\)\s*\{([^}]*)\}")
    for idx, species, level, tid, init in pat.findall(body):
        dvs = re.search(r"IVs\s*=\s*new\(([^)]*)\)", init)
        gender = re.search(r"Gender\s*=\s*(\d+)", init)
        otg = re.search(r"OTGender\s*=\s*(\d+)", init)
        out.append({
            "index": int(idx), "species": int(species), "level": int(level), "tid": int(tid),
            "gender": int(gender.group(1)) if gender else 255,
            "otGender": int(otg.group(1)) if otg else 0,
            "dvs": tuple(int(x) for x in dvs.group(1).split(",")) if dvs else None,
        })
    return out


def read_pp(core, name):
    """Base PP per move id from PKHeX.Core/Moves/MoveInfoN.cs."""
    src = (core / "Moves" / name).read_text(encoding="utf-8")
    m = re.search(r"PP\s*=>\s*\[(.*?)\];", src, re.S)
    if not m:
        die(f"cannot find the PP table in {name}")
    return [int(x) for x in re.findall(r"\d+", m.group(1))]


# --------------------------------------------------------------------------- emit helpers

def arr(values, width=16):
    values = list(values)
    lines = []
    for i in range(0, len(values), width):
        lines.append("    " + ", ".join(str(v) for v in values[i:i + width]) + ",")
    return "\n".join(lines)


def cstr(s):
    return '"' + s.replace("\\", "\\\\").replace('"', '\\"') + '"'


def flat_learnsets(learnsets, name):
    idx, flat = [], []
    for entry in learnsets:
        idx.append(len(flat))
        flat.extend(entry)
    idx.append(len(flat))
    body = ",\n".join(f"    {{{lv}, {mv}}}" for lv, mv in flat)
    return (f"const uint16_t {name}Index[{len(idx)}] = {{\n{arr(idx)}\n}};\n"
            f"const LevelMove {name}[{max(1, len(flat))}] = {{\n{body}\n}};\n")


def flat_moves(lists, name):
    idx, flat = [], []
    for entry in lists:
        idx.append(len(flat))
        flat.extend(entry)
    idx.append(len(flat))
    return (f"const uint16_t {name}Index[{len(idx)}] = {{\n{arr(idx)}\n}};\n"
            f"const uint8_t {name}[{max(1, len(flat))}] = {{\n{arr(flat)}\n}};\n")


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--pkhex", required=True, type=Path, help="PKHeX repository checkout")
    ap.add_argument("--out-h", type=Path, default=REPO / "include/Legality/GBLegalityData.h")
    ap.add_argument("--out-cpp", type=Path, default=REPO / "src/Legality/GBLegalityData.cpp")
    args = ap.parse_args()

    core = args.pkhex / "PKHeX.Core"
    res = core / "Resources"
    if not res.is_dir():
        die(f"{res} is not a PKHeX.Core resource directory")
    try:
        commit = subprocess.check_output(["git", "-C", str(args.pkhex), "rev-parse", "HEAD"],
                                         text=True).strip()
    except (OSError, subprocess.CalledProcessError):
        commit = "unknown"

    p1 = {"rb": read_personal1(res / "byte/personal/personal_rb"),
          "y": read_personal1(res / "byte/personal/personal_y")}
    p2 = {"gs": read_personal2(res / "byte/personal/personal_gs"),
          "c": read_personal2(res / "byte/personal/personal_c")}
    learn = {
        "RB": read_learnsets(res / "byte/levelup/lvlmove_rb.pkl", b"rb"),
        "Y": read_learnsets(res / "byte/levelup/lvlmove_y.pkl", b"yw"),
        "GS": read_learnsets(res / "byte/levelup/lvlmove_gs.pkl", b"gs"),
        "C": read_learnsets(res / "byte/levelup/lvlmove_c.pkl", b"c\0"),
    }
    eggs = {
        "GS": read_move_lists(res / "byte/eggmove/eggmove_gs.pkl", b"gs"),
        "C": read_move_lists(res / "byte/eggmove/eggmove_c.pkl", b"c\0"),
    }
    for table in eggs.values():  # the files stop at the last species that has egg moves
        table.extend([] for _ in range(252 - len(table)))
    evo1 = read_evolutions(res / "byte/evolve/evos_g1.pkl", b"g1")
    evo2 = read_evolutions(res / "byte/evolve/evos_g2.pkl", b"g2")

    wild = []
    for fname, ver in (("Gen1/encounter_red.pkl", RD), ("Gen1/encounter_blue.pkl", GN),
                       ("Gen1/encounter_blue_jp.pkl", BU), ("Gen1/encounter_yellow.pkl", YW)):
        for s in read_areas(res / "legality/wild" / fname, b"g1", 1):
            wild.append(s + (ver,))
    for fname, ver in (("Gen2/encounter_gold.pkl", GD), ("Gen2/encounter_silver.pkl", SI),
                       ("Gen2/encounter_crystal.pkl", C)):
        for s in read_areas(res / "legality/wild" / fname, b"g2", 2):
            wild.append(s + (ver,))
    # Identical slots across areas add nothing to a legality search; keep each once.
    wild = sorted(set(wild), key=lambda s: (s[6], s[0], s[3], s[1], s[2], s[4], s[5]))

    gift1 = res.joinpath("legality/mgdb/event1.pkl").read_bytes()
    gifts1 = [gift1[i:i + 8] for i in range(0, len(gift1), 8)]
    gift2 = res.joinpath("legality/mgdb/event2.pkl").read_bytes()
    gifts2 = [gift2[i:i + 12] for i in range(0, len(gift2), 12)]
    stadium2 = read_stadium2(res / "byte/levelup/reminder_stad2.pkl")

    enc1 = (core / "Legality/Encounters/Data/Gen1/Encounters1.cs").read_text(encoding="utf-8")
    statics1 = []
    for name in ("StaticRBY", "StaticRB", "StaticYW", "StaticBU"):
        statics1.extend(parse_static1(enc1, name))
    trades1 = []
    for name in ("TradeGift_RB", "TradeGift_YW", "TradeGift_BU"):
        trades1.extend(parse_trade1(enc1, name))

    enc2 = (core / "Legality/Encounters/Data/Gen2/Encounters2.cs").read_text(encoding="utf-8")
    statics2 = []
    for name in ("StaticGSC", "StaticGS", "StaticGD", "StaticSI", "StaticC", "StaticOddEggC"):
        statics2.extend(parse_static2(enc2, name))
    trades2 = parse_trade2(enc2)

    items2 = (core / "Saves/Storage/Items/ItemStorage2.cs")
    if not items2.exists():
        items2 = next(core.rglob("ItemStorage2.cs"))
    items_src = items2.read_text(encoding="utf-8")
    held2 = sorted(set(cs_numbers(items_src, "General") + cs_numbers(items_src, "Balls") +
                       cs_numbers(items_src, "Machine")[:50]))

    pp1, pp2 = read_pp(core, "MoveInfo1.cs"), read_pp(core, "MoveInfo2.cs")

    names1 = {lang: read_text_lines(res / f"legality/gen1/text_traderby_{code}.txt")
              for lang, code in LANG_CODES.items() if code != "ko"}
    names2 = {lang: read_text_lines(res / f"legality/gen2/text_tradegsc_{code}.txt")
              for lang, code in LANG_CODES.items()}

    # Sanity checks against values that are not in question.
    assert len(p1["rb"]) == 152 and len(p2["c"]) == 252, "unexpected personal table size"
    assert p1["rb"][150]["catch"] == 3 and p1["rb"][129]["catch"] == 255
    # Gen I level-1 moves live in the personal record; the learnset starts after them.
    assert p1["rb"][1]["moves"] == [33, 45, 0, 0], p1["rb"][1]["moves"]   # Bulbasaur: Tackle, Growl
    assert learn["RB"][1][:2] == [(7, 73), (13, 22)], learn["RB"][1][:2]  # Leech Seed 7, Vine Whip 13
    assert any(e[2] == 6 and e[4] == 36 for e in evo1[5]), evo1[5]       # Charmeleon -> Charizard @36
    assert (150, 70, VERSION_GROUPS["RBY"]) in statics1
    assert len(statics1) == 35 and len(trades1) == 26, (len(statics1), len(trades1))
    assert len(statics2) == 58 and len(trades2) == 11, (len(statics2), len(trades2))
    assert len(gifts1) * 8 == len(gift1) and len(gifts2) * 12 == len(gift2)
    assert len(stadium2) == 252, len(stadium2)
    assert len(cs_numbers(items_src, "Balls")) == 11 and len(cs_numbers(items_src, "Machine")) == 57
    assert 1 in held2 and 191 in held2 and 240 in held2       # Master Ball, TM01, TM50
    assert 7 not in held2 and 243 not in held2               # Bicycle (key item), HM01
    assert names2[2][0] == "ROCKY" and len(names2[2]) == 22, names2[2][:2]
    assert len(pp1) == 166 and len(pp2) == 252, (len(pp1), len(pp2))
    assert pp1[1] == pp2[1] == 35 and pp1[165] == 10 and pp2[165] == 1 and pp2[251] == 10  # Pound, Struggle, Beat Up

    out = []
    w = out.append
    w("// GENERATED by tools/gen_gb_legality_data.py -- do not edit by hand.\n")
    w(f"// Source: kwsch/PKHeX@{commit} (PKHeX.Core legality resources, GPL-3.0).\n")
    w("// Used under GPL-3.0 section 13 alongside this AGPL-3.0 project.\n\n")
    w('#include "Legality/GBLegalityData.h"\n\nnamespace Legality::GBData {\n\n')

    def personal1(table, cname):
        rows = []
        for p in table:
            rows.append("    {{{{{}}}, {{{}, {}}}, {}, {}, {{{}}}, {{{}}}}}".format(
                ", ".join(map(str, p["stats"])), p["types"][0], p["types"][1], p["catch"],
                p["growth"], ", ".join(map(str, p["moves"])), ", ".join(map(str, p["tm"]))))
        w(f"const Personal1 {cname}[{len(table)}] = {{\n" + ",\n".join(rows) + "\n};\n\n")

    def personal2(table, cname):
        rows = []
        for p in table:
            rows.append("    {{{{{}}}, {{{}, {}}}, {}, {}, {}, {}, {}, {{{}}}}}".format(
                ", ".join(map(str, p["stats"])), p["types"][0], p["types"][1], p["catch"],
                p["growth"], p["gender"], p["eggGroups"], p["hatch"],
                ", ".join(map(str, p["tm"]))))
        w(f"const Personal2 {cname}[{len(table)}] = {{\n" + ",\n".join(rows) + "\n};\n\n")

    personal1(p1["rb"], "kPersonalRB")
    personal1(p1["y"], "kPersonalY")
    personal2(p2["gs"], "kPersonalGS")
    personal2(p2["c"], "kPersonalC")
    for key in ("RB", "Y", "GS", "C"):
        w(flat_learnsets(learn[key], f"kLevelUp{key}") + "\n")
    for key in ("GS", "C"):
        w(flat_moves(eggs[key], f"kEggMoves{key}") + "\n")

    def evolutions(table, cname):
        rows = []
        for src, evos in enumerate(table):
            for etype, arg, dst, _form, level in evos:
                rows.append(f"    {{{src}, {dst}, {etype}, {level}, {arg}}}")
        w(f"const Evolution {cname}[{len(rows)}] = {{\n" + ",\n".join(rows) + "\n};\n")
        w(f"const std::size_t {cname}Count = {len(rows)};\n\n")

    evolutions(evo1, "kEvolutions1")
    evolutions(evo2, "kEvolutions2")

    rows = [f"    {{{s[0]}, {s[1]}, {s[2]}, {s[3]}, {s[4]}, {s[5]}, {s[6]}}}" for s in wild]
    w(f"const WildSlot kWildSlots[{len(rows)}] = {{\n" + ",\n".join(rows) + "\n};\n")
    w(f"const std::size_t kWildSlotCount = {len(rows)};\n\n")

    rows = [f"    {{{s}, {lv}, {v}}}" for s, lv, v in statics1]
    w(f"const Static1 kStatics1[{len(rows)}] = {{\n" + ",\n".join(rows) + "\n};\n")
    w(f"const std::size_t kStatics1Count = {len(rows)};\n\n")

    rows = [f"    {{{s}, {v}, {rby}, {gsc}, {idx}, {str(evo).lower()}}}"
            for s, v, rby, gsc, idx, evo in trades1]
    w(f"const Trade1 kTrades1[{len(rows)}] = {{\n" + ",\n".join(rows) + "\n};\n")
    w(f"const std::size_t kTrades1Count = {len(rows)};\n\n")

    rows = []
    for g in gifts1:
        rows.append(f"    {{{g[0]}, {g[1]}, {{{g[2]}, {g[3]}, {g[4]}, {g[5]}}}, {g[6]}, {g[7]}}}")
    w(f"const Gift1 kGifts1[{len(rows)}] = {{\n" + ",\n".join(rows) + "\n};\n")
    w(f"const std::size_t kGifts1Count = {len(rows)};\n\n")

    rows = []
    for s in statics2:
        mv = (s["moves"] + [0, 0, 0, 0])[:4]
        dv = s["dvs"] or (255,) * 6
        rows.append("    {{{}, {}, {}, {}, {}, {}, {}, {}, {{{}}}, {{{}}}}}".format(
            s["species"], s["level"], s["version"], s["location"], str(s["egg"]).lower(),
            str(s["shinyAlways"]).lower(), str(s["shinyNever"]).lower(), s["gender"],
            ", ".join(map(str, mv)), ", ".join(map(str, dv))))
    w(f"const Static2 kStatics2[{len(rows)}] = {{\n" + ",\n".join(rows) + "\n};\n")
    w(f"const std::size_t kStatics2Count = {len(rows)};\n\n")

    rows = []
    for t in trades2:
        dv = t["dvs"] or (255,) * 6
        rows.append("    {{{}, {}, {}, {}, {}, {}, {{{}}}}}".format(
            t["species"], t["level"], t["tid"], t["index"], t["gender"], t["otGender"],
            ", ".join(map(str, dv))))
    w(f"const Trade2 kTrades2[{len(rows)}] = {{\n" + ",\n".join(rows) + "\n};\n")
    w(f"const std::size_t kTrades2Count = {len(rows)};\n\n")

    def names(table, cname):
        langs = sorted(table)
        rows = []
        for lang in langs:
            for i, n in enumerate(table[lang]):
                rows.append(f"    {{{lang}, {i}, {cstr(n)}}}")
        w(f"const TradeName {cname}[{len(rows)}] = {{\n" + ",\n".join(rows) + "\n};\n")
        w(f"const std::size_t {cname}Count = {len(rows)};\n\n")

    rows = []
    for g in gifts2:
        rows.append("    {{{}, {}, {{{}, {}, {}, {}}}, {}, {}, {}, {}, {}, {}}}".format(*g))
    w(f"const Gift2 kGifts2[{len(rows)}] = {{\n" + ",\n".join(rows) + "\n};\n")
    w(f"const std::size_t kGifts2Count = {len(rows)};\n\n")

    idx, flat = [], []
    for entry in stadium2:
        idx.append(len(flat))
        flat.extend(entry)
    idx.append(len(flat))
    w(f"const uint16_t kStadium2RelearnIndex[{len(idx)}] = {{\n{arr(idx)}\n}};\n")
    body = ",\n".join(f"    {{{lv}, {mv}, {src}}}" for lv, mv, src in flat)
    w(f"const Stadium2Move kStadium2Relearn[{len(flat)}] = {{\n{body}\n}};\n\n")

    bits = [0] * 32
    for item in held2:
        bits[item >> 3] |= 1 << (item & 7)
    w(f"// Gen II items a Pokemon can hold: PKHeX ItemStorage2 General + Balls + TM01-TM50.\n")
    w(f"const uint8_t kHeldItems2[32] = {{\n{arr(bits)}\n}};\n\n")

    w("// Base PP per move (PKHeX MoveInfo1.PP / MoveInfo2.PP).\n")
    w(f"const uint8_t kMovePP1[{len(pp1)}] = {{\n{arr(pp1)}\n}};\n")
    w(f"const uint8_t kMovePP2[{len(pp2)}] = {{\n{arr(pp2)}\n}};\n\n")

    names(names1, "kTradeNames1")
    names(names2, "kTradeNames2")
    w("} // namespace Legality::GBData\n")

    args.out_cpp.write_text("".join(out), encoding="utf-8")
    print(f"wrote {args.out_cpp.relative_to(REPO)} from PKHeX {commit[:12]}: "
          f"{len(wild)} wild slots, {len(statics1)}+{len(statics2)} statics, "
          f"{len(trades1)}+{len(trades2)} trades, {len(gifts1)}+{len(gifts2)} event gifts")


if __name__ == "__main__":
    main()
