#!/usr/bin/env python3
"""Generate the Generation V (Black/White/Black 2/White 2) legality tables from a
pinned PKHeX checkout.

PKHeX.Core is the reference legality implementation the project measures itself
against (docs/legality/PKHEX_FIRST_REUSE_POLICY.md), so its own Gen V resources are
the input:

  Resources/byte/personal/personal_bw, personal_b2w2      personal records (raw)
  Resources/byte/levelup/lvlmove_bw.pkl, lvlmove_b2w2.pkl  level-up learnsets
  Resources/byte/eggmove/eggmove_bw.pkl                    egg moves
  Resources/byte/evolve/evos_g5.pkl                        evolution methods
  Resources/legality/wild/Gen5/encounter_{b,w,b2,w2}.pkl   wild encounter areas
  Resources/legality/mgdb/pgf.pkl                          PGF Mystery Gift cards (raw)
  Legality/Encounters/Data/Gen5/Encounters5{BW,B2W2,DR}.cs static, gift, trade and
                                                           Dream World definitions
  Legality/Encounters/Templates/Gen5/EncounterStatic5N.cs  N's Pokemon fixed ball
  Resources/legality/gen5/text_trade{bw,b2w2}_<lang>.txt   in-game trade nicknames / OTs
  Resources/text/other/<lang>/text_Species_<lang>.txt      species names (9 languages)
  Resources/text/badwords/badwords_{switch,3ds,gen5}.txt   PKHeX word-filter lists
  Resources/legality/gen{3,4}/text_trade*_<lang>.txt,      Gen III/IV in-game trade names
  Legality/Encounters/Data/Gen3/Encounters3XD.cs,          (transferred Pokemon)
  Legality/Encounters/Templates/Gen4/EncounterTrade4RanchGift.cs

PKHeX is GPL-3.0; PokeBank NX is AGPL-3.0, which GPL-3.0 section 13 permits
combining with. The generated files carry that attribution.

Usage:
    python3 tools/gen_legality_gen5_data.py --pkhex /path/to/PKHeX [--check]

Writes include/Legality/Gen5LegalityData.h and src/Legality/Gen5LegalityData.cpp.
Output is deterministic; --check fails if the committed files differ.
"""

import argparse
import hashlib
import re
import struct
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
PINNED_PKHEX = "6501f0ab46e8f8ca048539dbaf8cae8cb104e722"

# Version bits (match Legality::Gen5Data::Version).
W, B, W2, B2 = 0x01, 0x02, 0x04, 0x08
VERSION_BITS = {"W": W, "B": B, "W2": W2, "B2": B2, "BW": W | B, "B2W2": W2 | B2,
                "Gen5": W | B | W2 | B2}

PERSONAL_BW_SIZE = 0x3C
PERSONAL_B2W2_SIZE = 0x4C
PGF_SIZE = 0xCC


def die(msg):
    sys.stderr.write(f"error: {msg}\n")
    sys.exit(1)


# --------------------------------------------------------------------------- readers

def binlinker16(data, ident):
    """BinLinkerAccessor16: 2-byte ident, u16 count, then count+1 u16 offsets."""
    if data[:2] != ident:
        die(f"bad BinLinker16 ident {data[:2]!r}, expected {ident!r}")
    count = struct.unpack_from("<H", data, 2)[0]
    out = []
    for i in range(count):
        start, end = struct.unpack_from("<HH", data, 4 + i * 2)
        if not (start <= end <= len(data)):
            die(f"BinLinker16 entry {i} out of range")
        out.append(data[start:end])
    return out


def binlinker32(data, ident):
    """BinLinkerAccessor: 2-byte ident, u16 count, then count+1 u32 offsets."""
    if data[:2] != ident:
        die(f"bad BinLinker ident {data[:2]!r}, expected {ident!r}")
    count = struct.unpack_from("<H", data, 2)[0]
    out = []
    for i in range(count):
        start, end = struct.unpack_from("<II", data, 4 + i * 4)
        if not (start <= end <= len(data)):
            die(f"BinLinker entry {i} out of range")
        out.append(data[start:end])
    return out


def read_personal(path, size):
    data = path.read_bytes()
    if len(data) % size:
        die(f"{path.name}: {len(data)} is not a multiple of {size}")
    return [data[i:i + size] for i in range(0, len(data), size)]


def read_learnsets(path, ident):
    """LearnsetReader: per entry, count u16 moves then count u8 levels."""
    result = []
    for entry in binlinker16(path.read_bytes(), ident):
        if len(entry) % 3:
            die(f"{path.name}: learnset entry length {len(entry)} not a multiple of 3")
        count = len(entry) // 3
        moves = struct.unpack_from(f"<{count}H", entry, 0)
        levels = entry[count * 2:count * 3]
        result.append(list(zip(moves, levels)))
    return result


def read_move_lists(path, ident):
    """MoveSource.GetArray: per entry, a little-endian u16 move list."""
    result = []
    for entry in binlinker16(path.read_bytes(), ident):
        if len(entry) % 2:
            die(f"{path.name}: move list length {len(entry)} is odd")
        result.append(list(struct.unpack_from(f"<{len(entry) // 2}H", entry, 0)))
    return result


def read_evolutions(path, ident):
    """EvolutionSet.GetMethod: 8-byte entries {type, -, arg u16, species u16, form, level}."""
    result = []
    for entry in binlinker16(path.read_bytes(), ident):
        if len(entry) % 8:
            die(f"{path.name}: evolution entry length {len(entry)} not a multiple of 8")
        methods = []
        for off in range(0, len(entry), 8):
            etype = entry[off]
            arg, species = struct.unpack_from("<HH", entry, off + 2)
            form, level = entry[off + 6], entry[off + 7]
            methods.append((etype, arg, species, form, level))
        result.append(methods)
    return result


def read_areas(path, ident):
    """EncounterArea5: location u16, SlotType5 u8, pad u8, then 4-byte slots
    {species|form<<11 u16, min u8, max u8}."""
    areas = []
    for entry in binlinker32(path.read_bytes(), ident):
        if len(entry) < 4 or (len(entry) - 4) % 4:
            die(f"{path.name}: area length {len(entry)} is malformed")
        location = struct.unpack_from("<H", entry, 0)[0]
        stype = entry[2]
        slots = []
        for off in range(4, len(entry), 4):
            raw, lo, hi = struct.unpack_from("<HBB", entry, off)
            slots.append((raw & 0x3FF, raw >> 11, lo, hi))
        areas.append((location, stype, slots))
    return areas


# --------------------------------------------------------------------------- C# parsing

def cs_block(source, name, span=False):
    """Text between the '[' that opens array `name` and its matching ']'.
    span=True matches only a `Name => [` expression-bodied span property."""
    pattern = r"\b" + re.escape(name) + (r"\s*=>\s*\[" if span else r"\b[^=]*=[^\[]*\[")
    m = re.search(pattern, source)
    if not m:
        die(f"C# array {name} not found")
    depth, i = 1, m.end()
    while depth:
        c = source[i]
        depth += c == "["
        depth -= c == "]"
        i += 1
    return source[m.end():i - 1]


def strip_comments(text):
    return re.sub(r"//[^\n]*", "", text)


def split_top(text, sep=","):
    """Split on `sep` at nesting depth 0 (parentheses, braces, brackets)."""
    parts, depth, cur = [], 0, []
    for c in text:
        if c in "([{":
            depth += 1
        elif c in ")]}":
            depth -= 1
        if c == sep and depth == 0:
            parts.append("".join(cur))
            cur = []
        else:
            cur.append(c)
    if "".join(cur).strip():
        parts.append("".join(cur))
    return [p.strip() for p in parts if p.strip()]


def cs_entries(block):
    """Each top-level `new(...) { ... }` / `new(...)` element of an array block."""
    entries = []
    for part in split_top(strip_comments(block)):
        m = re.match(r"new\s*\((.*?)\)\s*(\{(.*)\})?\s*$", part, re.S)
        if not m:
            die(f"unparsed C# element: {part[:80]!r}")
        args = split_top(m.group(1)) if m.group(1).strip() else []
        props = {}
        if m.group(3):
            for prop in split_top(m.group(3)):
                k, _, v = prop.partition("=")
                props[k.strip()] = v.strip()
        entries.append((args, props))
    return entries


def cs_int(text):
    text = text.strip()
    if re.fullmatch(r"0x[0-9A-Fa-f]+", text):
        return int(text, 16)
    if re.fullmatch(r"-?\d+", text):
        return int(text)
    die(f"not an integer literal: {text!r}")


# --------------------------------------------------------------------------- emit

def arr(values, width=16):
    values = list(values)
    lines = []
    for i in range(0, len(values), width):
        lines.append("    " + ", ".join(str(v) for v in values[i:i + width]) + ",")
    return "\n".join(lines)


def hexbytes(data, width=24):
    return arr((f"0x{b:02X}" for b in data), width)


def flatten(lists):
    flat, starts = [], [0]
    for items in lists:
        flat.extend(items)
        starts.append(len(flat))
    return flat, starts


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


# --------------------------------------------------------------------------- C# encounter tables

ABILITY = {"Any12H": -1, "Any12": 0, "OnlyFirst": 1, "OnlySecond": 2, "OnlyHidden": 4}
SHINY = {"Shiny.Random": 0, "Shiny.Never": 1, "Shiny.Always": 2}
BALL = {"Ball.Poke": 4, "Ball.Dream": 25}
NATURES = ["Hardy", "Lonely", "Brave", "Adamant", "Naughty", "Bold", "Docile", "Relaxed",
           "Impish", "Lax", "Timid", "Hasty", "Serious", "Jolly", "Naive", "Modest", "Mild",
           "Quiet", "Bashful", "Rash", "Calm", "Gentle", "Sassy", "Careful", "Quirky"]

# Static encounter kinds (match Legality::Gen5Data::StaticKind).
K_STATIC, K_ROAMER, K_N, K_ENTREE, K_RADAR = 0, 1, 2, 3, 4
FLAG_ROAMING = 0x01


def version_of(text):
    text = text.strip()
    if text not in VERSION_BITS:
        die(f"unknown GameVersion {text!r}")
    return VERSION_BITS[text]


def nature_of(text):
    m = re.fullmatch(r"Nature\.(\w+)", text.strip())
    if not m or m.group(1) not in NATURES:
        die(f"unknown nature {text!r}")
    return NATURES.index(m.group(1))


def parse_static5(source, name, kind=K_STATIC):
    """EncounterStatic5 { Species, Level, Location, EggLocation, Form, Gender, Ability,
    Shiny, FixedBall, IsRoaming } with PKHeX's defaults (Gender random, Ability Any12)."""
    out = []
    for args, p in cs_entries(cs_block(source, name)):
        if len(args) != 1:
            die(f"{name}: expected one constructor argument, got {args}")
        known = {"Species", "Level", "Location", "EggLocation", "Form", "Gender", "Ability",
                 "Shiny", "FixedBall", "IsRoaming"}
        if set(p) - known:
            die(f"{name}: unhandled properties {sorted(set(p) - known)}")
        roaming = p.get("IsRoaming", "false") == "true"
        out.append(dict(
            kind=K_ROAMER if roaming else kind, versions=version_of(args[0]),
            species=cs_int(p["Species"]), level=cs_int(p["Level"]),
            location=cs_int(p.get("Location", "0")), egg_location=cs_int(p.get("EggLocation", "0")),
            form=cs_int(p.get("Form", "0")), gender=cs_int(p.get("Gender", "-1")),
            ability=ABILITY[p.get("Ability", "Any12")], shiny=SHINY[p.get("Shiny", "Shiny.Random")],
            ball=BALL[p["FixedBall"]] if "FixedBall" in p else 0,
            pid=0, nature=-1, move=0))
    return out


def parse_static5n(source, name):
    """EncounterStatic5N(pid) { Species, Level, Location, Ability, Nature }."""
    out = []
    for args, p in cs_entries(cs_block(source, name)):
        if len(args) != 1:
            die(f"{name}: expected the fixed PID argument")
        out.append(dict(
            kind=K_N, versions=VERSION_BITS["B2W2"], species=cs_int(p["Species"]),
            level=cs_int(p["Level"]), location=cs_int(p["Location"]), egg_location=0, form=0,
            gender=-1, ability=ABILITY[p["Ability"]], shiny=0, ball=0, pid=cs_int(args[0]),
            nature=nature_of(p["Nature"]), move=0))
    return out


def parse_dream_world(source, name, versions, personal_b2w2):
    """DreamWorldEntry(species, level, move1..3, Form:, Gender:) split per special move,
    ability OnlyHidden when B2W2 personal has a distinct hidden ability (DreamWorldEntry.AddTo)."""
    block = re.search(re.escape(name) + r"\s*=\s*DreamWorldEntry\.GetArray\((\w+),\s*\[(.*?)\]\);",
                      source, re.S)
    if not block:
        die(f"Dream World array {name} not found")
    if version_of(block.group(1)) != versions:
        die(f"{name}: version {block.group(1)} unexpected")
    out = []
    for args, p in cs_entries(block.group(2)):
        pos = [a for a in args if ":" not in a]
        named = dict(a.split(":", 1) for a in args if ":" in a)
        named = {k.strip(): v.strip() for k, v in named.items()}
        species, level = cs_int(pos[0]), cs_int(pos[1])
        moves = [cs_int(x) for x in pos[2:]]
        form = cs_int(named.get("Form", "0"))
        gender = cs_int(named.get("Gender", "-1"))
        rec = personal_b2w2[species]
        hidden = rec[0x1A] != rec[0x18]
        ability = ABILITY["OnlyHidden"] if hidden else ABILITY["OnlyFirst"]
        for move in (moves or [0]):
            out.append(dict(kind=K_ENTREE, versions=versions, species=species, level=level,
                            location=0, egg_location=0, form=form, gender=gender,
                            ability=ability, shiny=1, ball=0, pid=0, nature=-1, move=move))
    return out


def parse_radar(source, name):
    """EncounterStatic5Radar(species, form, ability = OnlyHidden)."""
    out = []
    for args, p in cs_entries(cs_block(source, name)):
        ability = ABILITY[args[2].strip()] if len(args) > 2 else ABILITY["OnlyHidden"]
        out.append(dict(kind=K_RADAR, versions=VERSION_BITS["B2W2"], species=cs_int(args[0]),
                        level=0, location=30015, egg_location=0, form=cs_int(args[1]),
                        gender=-1, ability=ability, shiny=1, ball=25, pid=0, nature=-1, move=0))
    return out


def parse_ivs(text):
    m = re.fullmatch(r"new\(([\d,\s]+)\)", text.strip())
    if not m:
        die(f"bad IV set {text!r}")
    ivs = [int(x) for x in m.group(1).split(",")]
    if len(ivs) != 6:
        die(f"IV set needs 6 values: {text!r}")
    return ivs


def parse_trades(source, name, bw, constants):
    out = []
    for args, p in cs_entries(cs_block(source, name)):
        if bw:
            names, index, version, pid = args
            pid = cs_int(pid)
            tid = cs_int(p["TID16"])
            id32 = tid
        else:
            if len(args) == 3:
                names, index, version = args
            else:
                names, version = args
                index = "-1"
            pid = 0
            raw = p["ID32"]
            id32 = constants[raw] if raw in constants else cs_int(raw)
        out.append(dict(
            names=names.strip(),
            versions=version_of(version), species=cs_int(p["Species"]), level=cs_int(p["Level"]),
            form=cs_int(p.get("Form", "0")), ability=ABILITY[p["Ability"]],
            id32=id32, ot_gender=cs_int(p["OTGender"]), gender=cs_int(p.get("Gender", "-1")),
            ivs=parse_ivs(p["IVs"]) if "IVs" in p else [-1] * 6,
            nature=nature_of(p["Nature"]) if "Nature" in p else -1, pid=pid,
            fixed_nickname=0 if p.get("IsFixedNickname", "true") == "false" or index == "-1" else 1,
            name_index=cs_int(index), bw=int(bw)))
    return out


def cs_constants(source):
    return {m.group(1): cs_int(m.group(2))
            for m in re.finditer(r"const\s+u?(?:short|int)\s+(\w+)\s*=\s*(\w+)\s*;", source)}


def cs_string_array(source, name):
    """A `string[] Name = ["a", string.Empty, ...];` field."""
    m = re.search(r"\b" + re.escape(name) + r"\s*=\s*\[(.*?)\];", source, re.S)
    if not m:
        die(f"C# string array {name} not found")
    items = []
    for part in split_top(m.group(1)):
        if part == "string.Empty":
            items.append("")
            continue
        mm = re.fullmatch(r'"([^"\\]*)"', part)
        if not mm:
            die(f"{name}: unparsed string {part!r}")
        items.append(mm.group(1))
    return items


# --------------------------------------------------------------------------- text

# PKHeX LanguageID -> resource language code (EncounterUtil.GetLanguageStrings and
# SpeciesName.SpeciesLang); 0 and 6 are unused ids.
LANG_CODES = {1: "ja", 2: "en", 3: "fr", 4: "it", 5: "de", 7: "es", 8: "ko", 9: "zh-Hans", 10: "zh-Hant"}
TRADE_LANGS = [1, 2, 3, 4, 5, 7, 8]
SPECIES_LANGS = [1, 2, 3, 4, 5, 7, 8, 9, 10]
# Gen III/IV in-game trade text (EncounterUtil.GetLanguageStrings in Encounters3RSE,
# Encounters3FRLG, Encounters4DPPt, Encounters4HGSS).
TRANSFER_TRADE_TEXT = [("gen3", "traderse", [1, 2, 3, 4, 5, 7]), ("gen3", "tradefrlg", [1, 2, 3, 4, 5, 7]),
                       ("gen4", "tradedppt", TRADE_LANGS), ("gen4", "tradehgss", TRADE_LANGS)]
SPECIES_NAME_COUNT = 1026  # 0 (egg) .. 1025


def read_lines(path):
    """Util.GetStringList: StreamReader text (BOM dropped) split on LF, CR trimmed."""
    text = path.read_bytes().decode("utf-8-sig")
    if not text:
        return []
    return [line[:-1] if line.endswith("\r") else line for line in text.split("\n")]


def lower_invariant(text):
    """char.ToLowerInvariant per UTF-16 unit for the characters the filter lists use.
    tools/pkhex_gen5_diff/run.sh compares the result with .NET."""
    out = []
    for c in text:
        if c == "İ":  # .NET invariant casing leaves capital dotted I unchanged
            out.append(c)
            continue
        low = c.lower()
        if len(low) != 1:
            die(f"no single-unit lower case for U+{ord(c):04X}")
        out.append(low)
    return "".join(out)


def read_patterns(path):
    """WordFilter.LoadPatterns: the list lower-cased, one regular expression per line."""
    text = path.read_bytes().decode("utf-8-sig")
    if any(c in text for c in "\r\x0b\x0c\x85  "):
        die(f"{path.name}: unexpected line separator")
    return [lower_invariant(line) for line in text.split("\n")]


def u16_literal(text):
    """A C++ u"" literal with everything outside printable ASCII escaped."""
    out = ['u"']
    for c in text:
        o = ord(c)
        if o > 0xFFFF:
            out.append(f"\\U{o:08X}")
        elif c in '"\\?':
            out.append("\\" + c)
        elif 0x20 <= o < 0x7F:
            out.append(c)
        else:
            out.append(f"\\u{o:04X}")
    out.append('"')
    return "".join(out)


def string_rows(values, width=6):
    lines = []
    for i in range(0, len(values), width):
        lines.append("    " + ", ".join(u16_literal(v) for v in values[i:i + width]) + ",")
    return "\n".join(lines)


def g3_table(source, name):
    """StringConverter3.G3_EN / G3_JP: the Gen III character table as 256 characters."""
    m = re.search(r"\b" + re.escape(name) + r"\s*=>\s*\[(.*?)\];", strip_comments(source), re.S)
    if not m:
        die(f"StringConverter3.{name} not found")
    named = {"HGM": "\u246D", "HGF": "\u246E", "FGM": "\u2642", "FGF": "\u2640", "Terminator": "\xff"}
    table = []
    for tok in re.finditer(r"'(\\.|[^'\\])'|\b([A-Z][A-Za-z]*)\b", m.group(1)):
        if tok.group(1) is not None:
            ch = tok.group(1)
            table.append(ch[1] if ch.startswith("\\") else ch)
        elif tok.group(2) in named:
            table.append(named[tok.group(2)])
        else:
            die(f"StringConverter3.{name}: unknown entry {tok.group(2)}")
    if len(table) != 256:
        die(f"StringConverter3.{name} has {len(table)} entries")
    return table


def transfer_glyphs34(text, language, g3_en, g3_jp):
    """StringConverter345.TransferGlyphs34 for a decoded name: Pal Park reads Gen III
    glyphs 0x01-0xA0 and 0xB7 with the Japanese table whatever the language."""
    if language == 1:
        return text
    out = []
    for c in text:
        glyph = g3_en.index(c) if c in g3_en and c != "\u201C" else 0xFF
        out.append(g3_jp[glyph] if 0x01 <= glyph <= 0xA0 or glyph == 0xB7 else c)
    return "".join(out)


def trade_names(t, lists):
    """A trade's nickname and OT for LanguageID 0..8 (EncounterUtil.GetNamesForLanguage);
    nicknames are empty when the trade's nickname is not fixed."""
    names = lists[t["names"]]
    if t["name_index"] >= 0:
        half = len(names[1]) >> 1
        idx = t["name_index"]
        nick = [n[idx] if idx < len(n) else "" for n in names]
        ot = [n[idx + half] if idx + half < len(n) else "" for n in names]
    else:
        nick = [""] * len(names)
        ot = list(names)
    if len(nick) != 9 or len(ot) != 9:
        die("trade name lists must cover LanguageID 0..8")
    if not t["fixed_nickname"]:
        nick = [""] * 9
    return nick, ot


# --------------------------------------------------------------------------- main

HEADER = """// GENERATED by tools/gen_legality_gen5_data.py -- do not edit by hand.
// Source: kwsch/PKHeX@{pin} (PKHeX.Core legality resources, GPL-3.0).
// Used under GPL-3.0 section 13 alongside this AGPL-3.0 project.
"""


def parse_move_tables(moveinfo, personal_bw, personal_b2w2, move_enum):
    """Base PP, TM/HM order, type tutor order and B2W2 tutor lists (PKHeX C# spans)."""
    pp = [cs_int(x) for x in split_top(strip_comments(cs_block(moveinfo, "PP", span=True))) if x.strip()]
    if len(pp) != 560:
        die(f"MoveInfo5.PP has {len(pp)} entries, expected 560")
    machines = [cs_int(x) for x in split_top(strip_comments(cs_block(personal_bw, "MachineMoves", span=True))) if x.strip()]
    if len(machines) != 101:
        die(f"MachineMoves has {len(machines)} entries, expected 101")
    body = strip_comments(move_enum[move_enum.index("{") + 1:])
    names = [n.strip().split("=")[0].strip() for n in body.split(",")]
    index = {n: i for i, n in enumerate(names) if n}
    type_tutors = []
    for x in split_top(strip_comments(cs_block(personal_bw, "TypeTutorMoves", span=True))):
        x = x.strip()
        if x:
            name = x.replace("(int)Move.", "").strip()
            if name not in index:
                die(f"unknown move enum {name}")
            type_tutors.append(index[name])
    if len(type_tutors) != 8:
        die("TypeTutorMoves must have 8 entries")
    tutors = []
    for n in range(1, 5):
        t = [cs_int(x) for x in split_top(strip_comments(cs_block(personal_b2w2, f"TutorMoves{n}", span=True))) if x.strip()]
        if not 0 < len(t) <= 32:
            die(f"TutorMoves{n} size {len(t)}")
        tutors.append(t)
    return pp, machines, type_tutors, tutors


def static_row(s):
    return ("    {{{species}, {form}, {level}, {location}, {egg_location}, {versions}, {ability}, "
            "{gender}, {shiny}, {ball}, {kind}, {nature}, {move}, 0x{pid:08X}u}},").format(**s)


def trade_row(t):
    ivs = ", ".join(str(v) for v in t["ivs"])
    return ("    {{{species}, {form}, {level}, {versions}, {ability}, {id32}u, {ot_gender}, {gender}, "
            "{{{iv_text}}}, {nature}, 0x{pid:08X}u, {fixed_nickname}, {name_index}, {bw}}},").format(iv_text=ivs, **t)


def generate(pkhex):
    core = pkhex / "PKHeX.Core"
    res = core / "Resources"
    inputs = {
        "personal_bw": res / "byte/personal/personal_bw",
        "personal_b2w2": res / "byte/personal/personal_b2w2",
        "lvlmove_bw": res / "byte/levelup/lvlmove_bw.pkl",
        "lvlmove_b2w2": res / "byte/levelup/lvlmove_b2w2.pkl",
        "eggmove_bw": res / "byte/eggmove/eggmove_bw.pkl",
        "evos_g5": res / "byte/evolve/evos_g5.pkl",
        "encounter_b": res / "legality/wild/Gen5/encounter_b.pkl",
        "encounter_w": res / "legality/wild/Gen5/encounter_w.pkl",
        "encounter_b2": res / "legality/wild/Gen5/encounter_b2.pkl",
        "encounter_w2": res / "legality/wild/Gen5/encounter_w2.pkl",
        "pgf": res / "legality/mgdb/pgf.pkl",
        "Encounters5BW.cs": core / "Legality/Encounters/Data/Gen5/Encounters5BW.cs",
        "Encounters5B2W2.cs": core / "Legality/Encounters/Data/Gen5/Encounters5B2W2.cs",
        "Encounters5DR.cs": core / "Legality/Encounters/Data/Gen5/Encounters5DR.cs",
        "EncounterStatic5N.cs": core / "Legality/Encounters/Templates/Gen5/EncounterStatic5N.cs",
        "MoveInfo5.cs": core / "Moves/MoveInfo5.cs",
        "PersonalInfo5BW.cs": core / "PersonalInfo/Info/PersonalInfo5BW.cs",
        "PersonalInfo5B2W2.cs": core / "PersonalInfo/Info/PersonalInfo5B2W2.cs",
        "Move.cs": core / "Game/Enums/Move.cs",
        "ItemStorage5.cs": core / "Items/ItemStorage5.cs",
        "badwords_switch": res / "text/badwords/badwords_switch.txt",
        "badwords_3ds": res / "text/badwords/badwords_3ds.txt",
        "badwords_gen5": res / "text/badwords/badwords_gen5.txt",
    }
    for lang in TRADE_LANGS:
        for kind in ("tradebw", "tradeb2w2"):
            inputs[f"{kind}_{LANG_CODES[lang]}"] = res / f"legality/gen5/text_{kind}_{LANG_CODES[lang]}.txt"
    for lang in SPECIES_LANGS:
        code = LANG_CODES[lang]
        inputs[f"species_{code}"] = res / f"text/other/{code}/text_Species_{code}.txt"
    for folder, kind, langs in TRANSFER_TRADE_TEXT:
        for lang in langs:
            inputs[f"{kind}_{LANG_CODES[lang]}"] = res / f"legality/{folder}/text_{kind}_{LANG_CODES[lang]}.txt"
    inputs["Encounters3XD.cs"] = core / "Legality/Encounters/Data/Gen3/Encounters3XD.cs"
    inputs["EncounterTrade4RanchGift.cs"] = core / "Legality/Encounters/Templates/Gen4/EncounterTrade4RanchGift.cs"
    inputs["StringConverter3.cs"] = core / "PKM/Strings/StringConverter3.cs"
    inputs["StringConverter345.cs"] = core / "PKM/Strings/StringConverter345.cs"
    for key, path in inputs.items():
        if not path.is_file():
            die(f"missing PKHeX input {key}: {path}")

    personal_bw = read_personal(inputs["personal_bw"], PERSONAL_BW_SIZE)
    personal_b2w2 = read_personal(inputs["personal_b2w2"], PERSONAL_B2W2_SIZE)
    learn_bw = read_learnsets(inputs["lvlmove_bw"], b"51")
    learn_b2w2 = read_learnsets(inputs["lvlmove_b2w2"], b"52")
    eggs = read_move_lists(inputs["eggmove_bw"], b"bw")
    evos = read_evolutions(inputs["evos_g5"], b"g5")
    wild = []
    for key, bit, ident in (("encounter_b", B, b"51"), ("encounter_w", W, b"51"),
                            ("encounter_b2", B2, b"52"), ("encounter_w2", W2, b"52")):
        for location, stype, slots in read_areas(inputs[key], ident):
            wild.append((bit, location, stype, slots))
    pgf = inputs["pgf"].read_bytes()
    if len(pgf) % PGF_SIZE:
        die(f"pgf.pkl size {len(pgf)} is not a multiple of {PGF_SIZE}")

    bw_src = inputs["Encounters5BW.cs"].read_text(encoding="utf-8")
    b2_src = inputs["Encounters5B2W2.cs"].read_text(encoding="utf-8")
    dr_src = inputs["Encounters5DR.cs"].read_text(encoding="utf-8")
    # PKHeX EncounterEnumerator5 picks lists by the Pokemon's game and never
    # filters by a template's own Version, so a template's games are the
    # games whose lists contain it.
    def listed(rows, games):
        for r in rows:
            r["versions"] = VERSION_BITS[games]
        return rows

    statics = []
    statics += listed(parse_static5(bw_src, "Encounter_BW"), "BW")
    statics += listed(parse_static5(bw_src, "StaticB"), "B")
    statics += listed(parse_static5(bw_src, "StaticW"), "W")
    statics += listed(parse_static5(b2_src, "Encounter_B2W2_Regular"), "B2W2")
    statics += listed(parse_static5(b2_src, "StaticB2"), "B2")
    statics += listed(parse_static5(b2_src, "StaticW2"), "W2")
    statics += parse_static5n(b2_src, "Encounter_B2W2_N")
    statics += parse_dream_world(bw_src, "DreamWorld_BW", VERSION_BITS["BW"], personal_b2w2)
    statics += parse_dream_world(b2_src, "DreamWorld_B2W2", VERSION_BITS["B2W2"], personal_b2w2)
    statics += parse_dream_world(dr_src, "DreamWorld_Common", VERSION_BITS["Gen5"], personal_b2w2)
    statics += parse_radar(dr_src, "Encounter_DreamRadar")
    consts = cs_constants(b2_src)
    trades = []
    trades += listed(parse_trades(bw_src, "TradeGift_BW", True, consts), "BW")
    trades += listed(parse_trades(bw_src, "TradeGift_B", True, consts), "B")
    trades += listed(parse_trades(bw_src, "TradeGift_W", True, consts), "W")
    trades += listed(parse_trades(b2_src, "TradeGift_B2W2", False, consts), "B2W2")
    trades += listed(parse_trades(b2_src, "TradeGift_B2", False, consts), "B2")
    trades += listed(parse_trades(b2_src, "TradeGift_W2", False, consts), "W2")

    # EncounterUtil.GetLanguageStrings(file, 8): LanguageID 0 and 6 have no list.
    def lang_lists(kind):
        lists = [[] for _ in range(9)]
        for lang in TRADE_LANGS:
            lists[lang] = read_lines(inputs[f"{kind}_{LANG_CODES[lang]}"])
        return lists
    trade_lists = {
        1: {"TradeNames": lang_lists("tradebw")},
        0: {"TradeNames": lang_lists("tradeb2w2"),
            "TradeOT_B2W2_F": cs_string_array(b2_src, "TradeOT_B2W2_F"),
            "TradeOT_B2W2_M": cs_string_array(b2_src, "TradeOT_B2W2_M")},
    }
    trade_text = []
    for t in trades:
        lists = trade_lists[t["bw"]]
        if t["names"] not in lists:
            die(f"unknown trade name list {t['names']}")
        trade_text.append(trade_names(t, lists))
    species_names = []
    for lang in SPECIES_LANGS:
        names = read_lines(inputs[f"species_{LANG_CODES[lang]}"])
        if len(names) != SPECIES_NAME_COUNT:
            die(f"species list {LANG_CODES[lang]} has {len(names)} names")
        species_names.append(names)
    # EncounterStatic5N.FixedBall: one of N's Pokemon (Zorua) is a gift in a Poke Ball;
    # the others are caught in any ball.
    m = re.search(r"FixedBall\s*=>\s*Species\s*==\s*\(int\)Core\.Species\.(\w+)\s*\?\s*(Ball\.\w+)\s*:\s*Ball\.None;",
                  inputs["EncounterStatic5N.cs"].read_text(encoding="utf-8"))
    if not m or m.group(2) not in BALL or m.group(1) not in species_names[SPECIES_LANGS.index(2)]:
        die("EncounterStatic5N.FixedBall has an unexpected form")
    n_ball_species = species_names[SPECIES_LANGS.index(2)].index(m.group(1))
    fixed = [s for s in statics if s["kind"] == K_N and s["species"] == n_ball_species]
    if not fixed:
        die("EncounterStatic5N.FixedBall names a species N does not have")
    for s in fixed:
        s["ball"] = BALL[m.group(2)]
    # Gen III/IV in-game trade names (nicknames and OTs of every language) and the XD
    # trade's fixed nickname: a transferred Pokemon may carry one of them.
    # Pal Park re-reads some Gen III glyphs as Japanese (TransferGlyphs34) and Poke
    # Transfer turns glyphs Gen V lacks into '?' (TransferGlyphs45); both forms are kept.
    sc3 = inputs["StringConverter3.cs"].read_text(encoding="utf-8")
    g3_en, g3_jp = g3_table(sc3, "G3_EN"), g3_table(sc3, "G3_JP")
    m45 = re.search(r'InvalidSearchVal45\s*=\s*SearchValues\.Create\("([^"\\]*)"\)',
                    inputs["StringConverter345.cs"].read_text(encoding="utf-8"))
    if not m45:
        die("StringConverter345.InvalidSearchVal45 not found")
    invalid45 = set(m45.group(1))
    def transfer_glyphs45(text):
        return "".join("?" if c in invalid45 else c for c in text)
    zaprong = cs_string_array(inputs["Encounters3XD.cs"].read_text(encoding="utf-8"), "Zaprong")
    gen3_names = {(lang, name) for lang, name in enumerate(zaprong)}
    for folder, kind, langs in TRANSFER_TRADE_TEXT:
        if folder == "gen3":
            for lang in langs:
                gen3_names.update((lang, name) for name in read_lines(inputs[f"{kind}_{LANG_CODES[lang]}"]))
    fixed34 = set()
    for lang, name in gen3_names:
        fixed34.update((name, transfer_glyphs34(name, lang, g3_en, g3_jp)))
    ranch = re.search(r"GetTrainerName\(int language\)\s*=>\s*language\s+switch\s*\{(.*?)\};",
                      strip_comments(inputs["EncounterTrade4RanchGift.cs"].read_text(encoding="utf-8")), re.S)
    if not ranch:
        die("EncounterTrade4RanchGift.GetTrainerName not found")
    ranch_names = re.findall(r'\b\d+\s*=>\s*"([^"\\]*)"', ranch.group(1))
    if len(ranch_names) != 6:
        die(f"expected 6 My Pokemon Ranch OT names, found {len(ranch_names)}")
    fixed34.update(ranch_names)
    for folder, kind, langs in TRANSFER_TRADE_TEXT:
        if folder == "gen4":
            for lang in langs:
                fixed34.update(read_lines(inputs[f"{kind}_{LANG_CODES[lang]}"]))
    fixed34.update([transfer_glyphs45(name) for name in fixed34])
    fixed34.discard("")
    fixed34 = sorted(fixed34, key=lambda s: s.encode("utf-16-be"))
    if any(ord(ch) > 0xFFFF for s in fixed34 for ch in s):
        die("Gen III/IV trade names must be BMP text")
    bad_switch = read_patterns(inputs["badwords_switch"])
    bad_3ds = read_patterns(inputs["badwords_3ds"])
    bad_gen5 = read_lines(inputs["badwords_gen5"])

    move_pp, machines, type_tutors, tutors = parse_move_tables(
        inputs["MoveInfo5.cs"].read_text(encoding="utf-8"),
        inputs["PersonalInfo5BW.cs"].read_text(encoding="utf-8"),
        inputs["PersonalInfo5B2W2.cs"].read_text(encoding="utf-8"),
        inputs["Move.cs"].read_text(encoding="utf-8"))

    items_src = strip_comments(inputs["ItemStorage5.cs"].read_text(encoding="utf-8"))
    def item_list(name):
        return [cs_int(x) for x in split_top(cs_block(items_src, name, span=True)) if x.strip()]
    unreleased = set(item_list("Unreleased"))
    held = sorted(set(item_list("General") + item_list("Medicine") + item_list("Berry")) - unreleased)
    if not held or held[-1] > 638:
        die("ItemStorage5 held items out of the Gen V range")

    pin = PINNED_PKHEX
    h = [HEADER.format(pin=pin), "#pragma once", "", "#include <cstddef>", "#include <cstdint>", "",
         "namespace Legality::Gen5Data {", "",
         "/// Version bits for Gen V encounter data.",
         "enum Version : uint8_t { W = 0x01, B = 0x02, W2 = 0x04, B2 = 0x08, BW = 0x03, B2W2 = 0x0C, AllGen5 = 0x0F };",
         "",
         "/// Static encounter kind: in-game static/gift, roaming legendary, N's Pokemon (B2W2),",
         "/// Entree Forest / Dream World (Entralink), Pokemon Dream Radar (B2W2).",
         "enum class StaticKind : uint8_t { Static = 0, Roamer = 1, N = 2, Entree = 3, Radar = 4 };",
         "",
         "/// PKHeX AbilityPermission: -1 any of 1/2/H, 0 either of 1/2, 1 first, 2 second, 4 hidden.",
         "struct Static5 {",
         "    uint16_t species; uint8_t form; uint8_t level; // level 0: any level the source allows",
         "    uint16_t location; uint16_t eggLocation;",
         "    uint8_t versions; int8_t ability; int8_t gender; // gender -1: random",
         "    uint8_t shiny; // 0 random, 1 never, 2 always",
         "    uint8_t ball;  // 0: not fixed",
         "    uint8_t kind;  // StaticKind",
         "    int8_t nature; // -1: random",
         "    uint16_t move; // Entree Forest: the one special move this entry teaches",
         "    uint32_t pid;  // N's Pokemon fixed PID template (0 otherwise)",
         "};",
         "",
         "struct Trade5 {",
         "    uint16_t species; uint8_t form; uint8_t level; uint8_t versions; int8_t ability;",
         "    uint32_t id32; uint8_t otGender; int8_t gender; int8_t ivs[6]; // -1: random",
         "    int8_t nature; uint32_t pid; // BW trades have a fixed PID; 0 for B2W2",
         "    uint8_t fixedNickname; int8_t nameIndex; uint8_t bw;",
         "};",
         "",
         "struct Slot5 { uint16_t species; uint8_t form; uint8_t minLevel; uint8_t maxLevel; };",
         "/// SlotType5: 0 standard, 1 grass, 2 surf, 3 super rod, 4 swarm, 5 hidden grotto.",
         "struct Area5 { uint8_t version; uint16_t location; uint8_t type; uint16_t slotStart; uint16_t slotCount; };",
         "",
         "struct LevelMove { uint16_t move; uint8_t level; };",
         "/// EvolutionSet entry: PKHeX EvolutionType, argument, target species/form, level.",
         "struct Evolution { uint8_t type; uint16_t argument; uint16_t species; uint8_t form; uint8_t level; };",
         "",
         f"inline constexpr std::size_t kPersonalBWSize = 0x{PERSONAL_BW_SIZE:X};",
         f"inline constexpr std::size_t kPersonalB2W2Size = 0x{PERSONAL_B2W2_SIZE:X};",
         f"inline constexpr std::size_t kPersonalBWCount = {len(personal_bw)};",
         f"inline constexpr std::size_t kPersonalB2W2Count = {len(personal_b2w2)};",
         "extern const uint8_t kPersonalBW[];   // raw PersonalInfo5BW records",
         "extern const uint8_t kPersonalB2W2[]; // raw PersonalInfo5B2W2 records",
         "",
         f"inline constexpr std::size_t kLearnsetBWCount = {len(learn_bw)};",
         f"inline constexpr std::size_t kLearnsetB2W2Count = {len(learn_b2w2)};",
         "extern const LevelMove kLevelUpBW[];   extern const uint16_t kLevelUpBWStart[];",
         "extern const LevelMove kLevelUpB2W2[]; extern const uint16_t kLevelUpB2W2Start[];",
         f"inline constexpr std::size_t kEggMoveCount = {len(eggs)};",
         "extern const uint16_t kEggMoves[]; extern const uint16_t kEggMoveStart[];",
         f"inline constexpr std::size_t kEvolutionCount = {len(evos)};",
         "extern const Evolution kEvolutions[]; extern const uint16_t kEvolutionStart[];",
         "",
         f"inline constexpr std::size_t kAreaCount = {len(wild)};",
         f"inline constexpr std::size_t kSlotCount = {sum(len(a[3]) for a in wild)};",
         "extern const Area5 kAreas[]; extern const Slot5 kSlots[];",
         f"inline constexpr std::size_t kStaticCount = {len(statics)};",
         "extern const Static5 kStatics[];",
         f"inline constexpr std::size_t kTradeCount = {len(trades)};",
         "extern const Trade5 kTrades[];",
         "",
         f"inline constexpr std::size_t kPgfSize = 0x{PGF_SIZE:X};",
         f"inline constexpr std::size_t kPgfCount = {len(pgf) // PGF_SIZE};",
         "extern const uint8_t kPgf[]; // raw PGF cards from PKHeX mgdb/pgf.pkl",
         "",
         f"inline constexpr std::size_t kMoveCount = {len(move_pp)}; // move ids 0..{len(move_pp) - 1}",
         "extern const uint8_t kMovePP[];      // MoveInfo5.PP base PP",
         f"inline constexpr std::size_t kMachineCount = {len(machines)}; // TM01-TM95 then HM01-HM06",
         "extern const uint16_t kMachineMoves[]; // PersonalInfo5BW.MachineMoves (personal bits at 0x28)",
         f"inline constexpr std::size_t kTypeTutorCount = {len(type_tutors)};",
         "extern const uint16_t kTypeTutorMoves[]; // personal byte 0x38 bit order",
         "/// B2W2 move tutors: Driftveil (0x3C), Lentimas (0x40), Humilau (0x44), Nacrene (0x48).",
         f"inline constexpr std::size_t kTutorCounts[4] = {{{', '.join(str(len(t)) for t in tutors)}}};",
         "extern const uint16_t kTutorMoves[4][32];",
         "",
         "/// PKHeX ItemStorage5 General + Medicine + Berry minus Unreleased: items a Gen V Pokemon can hold.",
         f"inline constexpr std::size_t kHeldItemCount = {len(held)};",
         "extern const uint16_t kHeldItems[]; // ascending",
         "",
         "/// In-game trade names by PKHeX LanguageID 0..8 (rows match kTrades). A nickname is",
         "/// empty when the trade's nickname is not fixed.",
         "extern const char16_t* const kTradeNicknames[][9];",
         "extern const char16_t* const kTradeTrainers[][9];",
         "",
         "/// Species names (SpeciesName.SpeciesLang) for kSpeciesNameLanguages; [0] is the egg name.",
         f"inline constexpr std::size_t kSpeciesNameCount = {SPECIES_NAME_COUNT};",
         f"inline constexpr std::size_t kSpeciesLanguageCount = {len(SPECIES_LANGS)};",
         f"inline constexpr uint8_t kSpeciesNameLanguages[kSpeciesLanguageCount] = {{{', '.join(str(x) for x in SPECIES_LANGS)}}};",
         "extern const char16_t* const kSpeciesNames[kSpeciesLanguageCount][kSpeciesNameCount];",
         "",
         "/// Every Gen III/IV in-game trade nickname and OT name (all languages), the XD trade's",
         "/// fixed nickname and the My Pokemon Ranch OT names, sorted by UTF-16 code units.",
         f"inline constexpr std::size_t kTransferTradeNameCount = {len(fixed34)};",
         "extern const char16_t* const kTransferTradeNames[];",
         "",
         "/// PKHeX word filters: the Switch and 3DS regular expressions, lower-cased as",
         "/// WordFilter.LoadPatterns does, and the Gen V dictionary (whole words).",
         f"inline constexpr std::size_t kBadWordsSwitchCount = {len(bad_switch)};",
         "extern const char16_t* const kBadWordsSwitch[];",
         f"inline constexpr std::size_t kBadWords3DSCount = {len(bad_3ds)};",
         "extern const char16_t* const kBadWords3DS[];",
         f"inline constexpr std::size_t kBadWordsGen5Count = {len(bad_gen5)};",
         "extern const char16_t* const kBadWordsGen5[];",
         "",
         "/// SHA-256 of every PKHeX input, so a regenerated table can be traced to its source.",
         "struct SourcePin { const char* path; const char* sha256; };",
         f"inline constexpr std::size_t kSourcePinCount = {len(inputs)};",
         "extern const SourcePin kSourcePins[];",
         f'inline constexpr const char* kPkhexCommit = "{pin}";',
         "", "} // namespace Legality::Gen5Data", ""]

    c = [HEADER.format(pin=pin), '#include "Legality/Gen5LegalityData.h"', "",
         "namespace Legality::Gen5Data {", ""]
    c += ["const uint8_t kPersonalBW[] = {", hexbytes(b"".join(personal_bw)), "};", ""]
    c += ["const uint8_t kPersonalB2W2[] = {", hexbytes(b"".join(personal_b2w2)), "};", ""]
    for name, sets in (("BW", learn_bw), ("B2W2", learn_b2w2)):
        flat, starts = flatten(sets)
        if starts[-1] > 0xFFFF:
            die("learnset offsets exceed u16")
        c += [f"const LevelMove kLevelUp{name}[] = {{",
              "\n".join(f"    {{{m}, {lv}}}," for m, lv in flat), "};",
              f"const uint16_t kLevelUp{name}Start[] = {{", arr(starts), "};", ""]
    flat, starts = flatten(eggs)
    c += ["const uint16_t kEggMoves[] = {", arr(flat) if flat else "    0,", "};",
          "const uint16_t kEggMoveStart[] = {", arr(starts), "};", ""]
    flat, starts = flatten(evos)
    c += ["const Evolution kEvolutions[] = {",
          "\n".join(f"    {{{t}, {a}, {s}, {f}, {lv}}}," for t, a, s, f, lv in flat), "};",
          "const uint16_t kEvolutionStart[] = {", arr(starts), "};", ""]
    area_rows, slot_rows, start = [], [], 0
    for bit, location, stype, slots in wild:
        area_rows.append(f"    {{{bit}, {location}, {stype}, {start}, {len(slots)}}},")
        for sp, fm, lo, hi in slots:
            slot_rows.append(f"    {{{sp}, {fm}, {lo}, {hi}}},")
        start += len(slots)
    c += ["const Area5 kAreas[] = {", "\n".join(area_rows), "};",
          "const Slot5 kSlots[] = {", "\n".join(slot_rows), "};", ""]
    c += ["const Static5 kStatics[] = {", "\n".join(static_row(s) for s in statics), "};", ""]
    c += ["const Trade5 kTrades[] = {", "\n".join(trade_row(t) for t in trades), "};", ""]
    c += ["const uint8_t kPgf[] = {", hexbytes(pgf), "};", ""]
    c += ["const uint8_t kMovePP[] = {", arr(move_pp), "};", ""]
    c += ["const uint16_t kMachineMoves[] = {", arr(machines), "};", ""]
    c += ["const uint16_t kTypeTutorMoves[] = {", arr(type_tutors), "};", ""]
    c += ["const uint16_t kTutorMoves[4][32] = {"]
    for t in tutors:
        c.append("    {" + ", ".join(str(m) for m in t) + "},")
    c += ["};", ""]
    c += ["const uint16_t kHeldItems[] = {", arr(held), "};", ""]
    c += ["const char16_t* const kTradeNicknames[][9] = {"]
    c += ["    {" + ", ".join(u16_literal(n) for n in nick) + "}," for nick, _ in trade_text]
    c += ["};", "const char16_t* const kTradeTrainers[][9] = {"]
    c += ["    {" + ", ".join(u16_literal(n) for n in ot) + "}," for _, ot in trade_text]
    c += ["};", ""]
    c += ["const char16_t* const kSpeciesNames[kSpeciesLanguageCount][kSpeciesNameCount] = {"]
    for lang, names in zip(SPECIES_LANGS, species_names):
        c += [f"  {{ // {LANG_CODES[lang]}", string_rows(names), "  },"]
    c += ["};", ""]
    c += ["const char16_t* const kTransferTradeNames[] = {", string_rows(fixed34), "};", ""]
    c += ["const char16_t* const kBadWordsSwitch[] = {", string_rows(bad_switch, 4), "};", ""]
    c += ["const char16_t* const kBadWords3DS[] = {", string_rows(bad_3ds, 4), "};", ""]
    c += ["const char16_t* const kBadWordsGen5[] = {", string_rows(bad_gen5, 8), "};", ""]
    c += ["const SourcePin kSourcePins[] = {"]
    for key, path in inputs.items():
        rel = path.relative_to(pkhex).as_posix()
        c.append(f'    {{"{rel}", "{sha256(path)}"}},')
    c += ["};", "", "} // namespace Legality::Gen5Data", ""]
    return "\n".join(h), "\n".join(c)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--pkhex", required=True, type=Path)
    ap.add_argument("--check", action="store_true", help="fail if the committed output differs")
    args = ap.parse_args()
    try:
        head = subprocess.run(["git", "-C", str(args.pkhex), "rev-parse", "HEAD"], check=True,
                              capture_output=True, text=True).stdout.strip()
    except (OSError, subprocess.CalledProcessError):
        die(f"{args.pkhex} is not a git checkout")
    if head != PINNED_PKHEX:
        die(f"PKHeX checkout is at {head}, expected pinned {PINNED_PKHEX}")
    header, source = generate(args.pkhex)
    targets = ((REPO / "include/Legality/Gen5LegalityData.h", header),
               (REPO / "src/Legality/Gen5LegalityData.cpp", source))
    if args.check:
        stale = [str(p.relative_to(REPO)) for p, text in targets
                 if not p.is_file() or p.read_text(encoding="utf-8") != text]
        if stale:
            die("generated Gen V tables are stale: " + ", ".join(stale))
        print("Gen V legality tables match the pinned PKHeX resources")
        return
    for path, text in targets:
        path.write_text(text, encoding="utf-8")
        print(f"wrote {path.relative_to(REPO)}")


if __name__ == "__main__":
    main()
