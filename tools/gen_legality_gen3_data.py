#!/usr/bin/env python3
"""Generate the Generation III (Ruby/Sapphire/Emerald/FireRed/LeafGreen) PK3 legality
tables from a pinned PKHeX checkout.

PKHeX.Core is the reference legality implementation the project measures itself
against (docs/legality/PKHEX_FIRST_REUSE_POLICY.md), so its own Gen III resources and
hand-written encounter tables are the input:

  Resources/byte/personal/personal_{rs,e,fr,lg}            personal records (raw)
  Resources/byte/personal/hmtm_g3.pkl, tutors_g3.pkl       TM/HM and move tutor flags
  Resources/byte/levelup/lvlmove_{rs,e,fr,lg}.pkl          level-up learnsets
  Resources/byte/eggmove/eggmove_rs.pkl                    egg moves (all Gen III games)
  Resources/byte/evolve/evos_g3.pkl                        evolution methods
  Resources/legality/wild/Gen3/encounter_*.pkl             wild areas, swarms, PCNY gifts
  Resources/legality/gen3/text_trade{rse,frlg}_*.txt       in-game trade names
  Resources/text/other/*/text_Species_*.txt                species names
  Resources/text/badwords/badwords_{switch,3ds}.txt        word filter patterns
  Legality/Encounters/Data/Gen3/*.cs                       static, trade, Colosseum and WC3 gifts
  plus small tables quoted from PKHeX C# sources (character maps, PP, EXP, items, ...)

PKHeX is GPL-3.0; PokeBank NX is AGPL-3.0, which GPL-3.0 section 13 permits
combining with. The generated files carry that attribution.

Usage:
    python3 tools/gen_legality_gen3_data.py --pkhex /path/to/PKHeX [--check]

Writes include/Legality/Gen3LegalityData.h and src/Legality/Gen3LegalityData.cpp.
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
MAX_SPECIES = 386
MAX_MOVE = 354
PERSONAL_SIZE = 0x1C

# Version bits (match Legality::Gen3Data::Version).
S, R, E, FR, LG, CXD = 0x01, 0x02, 0x04, 0x08, 0x10, 0x20
VERSION_BITS = {"S": S, "R": R, "E": E, "FR": FR, "LG": LG, "CXD": CXD, "RS": R | S,
                "RSE": R | S | E, "FRLG": FR | LG, "EFL": E | FR | LG,
                "Gen3": R | S | E | FR | LG | CXD}
LANGUAGES = {1: "ja", 2: "en", 3: "fr", 4: "it", 5: "de", 7: "es", 8: "ko", 9: "zh-Hans",
             10: "zh-Hant", 11: "es-419"}
GEN3_LANGUAGES = (1, 2, 3, 4, 5, 7)          # Language.Languages_3
GEN7_LANGUAGES = (1, 2, 3, 4, 5, 7, 8, 9, 10)  # Language.Languages_9

PID_TYPES = ["None", "Method_1", "Method_2", "Method_3", "Method_4", "Method_1_Unown",
             "Method_2_Unown", "Method_3_Unown", "Method_4_Unown", "Method_1_Roamer", "BACD",
             "BACD_A", "BACD_AX", "BACD_S", "BACD_EA", "BACD_EAX", "BACD_ES", "BACD_U",
             "BACD_U_AX", "BACD_R", "BACD_R_A", "BACD_RBCD", "BACD_TA", "BACD_TS", "BACD_M",
             "CuteCharm", "ChainShiny", "CXD", "CXDAnti", "CXD_ColoStarter", "Channel",
             "PokeSpot", "G4MGAntiShiny", "G5MGShiny", "Pokewalker"]
GIFT_GENDER = ["Recipient", "Only0", "Only1", "RandAlgo", "RandD3", "RandS3", "RandS7",
               "RandSG15", "RandD3_0", "RandD3_1"]
SHINY = {"Random": 0, "Never": 1, "Always": 2}
ABILITY = {"OnlyFirst": 1, "OnlySecond": 2}
LANG_NAMES = {"Japanese": 1, "English": 2, "French": 3, "Italian": 4, "German": 5, "Spanish": 7}


def die(msg):
    sys.stderr.write(f"error: {msg}\n")
    sys.exit(1)


# --------------------------------------------------------------------------- binary readers

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
    """EvolutionSet: 8-byte entries {type, -, arg u16, species u16, form, level}."""
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


def read_areas(path, ident, swarm):
    """EncounterArea3: location, -, type, rate, then 10-byte regular slots
    {species u16, form, slot, min, max, magnet idx, magnet count, static idx, static count}
    or 14-byte swarm slots {species u16, form, slot, min, max, 4x move u16}."""
    areas = []
    for entry in binlinker32(path.read_bytes(), ident):
        size = 14 if swarm else 10
        if len(entry) < 4:
            die(f"{path.name}: area length {len(entry)} is malformed")
        location, stype, rate = entry[0], (6 if swarm else entry[2]), entry[3]
        slots = []
        # EncounterArea3 reads count = length / size slots; trailing padding is ignored.
        for off in range(4, 4 + (len(entry) - 4) // size * size, size):
            species = struct.unpack_from("<H", entry, off)[0]
            form, slot, lo, hi = entry[off + 2], entry[off + 3], entry[off + 4], entry[off + 5]
            if swarm:
                moves = list(struct.unpack_from("<4H", entry, off + 6))
                slots.append((species, form, slot, lo, hi, 0, 0, 0, 0, moves))
            else:
                mi, mc, si, sc = entry[off + 6:off + 10]
                slots.append((species, form, slot, lo, hi, mi, mc, si, sc, [0, 0, 0, 0]))
        areas.append((location, stype, rate, slots))
    return areas


def text_lines(path):
    """ResourceUtil.FastSplit over a BOM-stripped StreamReader read."""
    text = path.read_text(encoding="utf-8-sig")
    if not text:
        return []
    return [line[:-1] if line.endswith("\r") else line for line in text.split("\n")]


# --------------------------------------------------------------------------- C# parsing

def strip_comments(text):
    out, i, n = [], 0, len(text)
    in_str = in_chr = False
    while i < n:
        c = text[i]
        if in_str:
            out.append(c)
            if c == "\\":
                out.append(text[i + 1])
                i += 2
                continue
            if c == '"':
                in_str = False
        elif in_chr:
            out.append(c)
            if c == "\\":
                out.append(text[i + 1])
                i += 2
                continue
            if c == "'":
                in_chr = False
        elif text.startswith("//", i):
            j = text.find("\n", i)
            i = n if j < 0 else j
            continue
        else:
            if c == '"':
                in_str = True
            elif c == "'":
                in_chr = True
            out.append(c)
        i += 1
    return "".join(out)


def cs_block(source, name, span=False):
    """Text between the '[' that opens array `name` and its matching ']'.
    span=True matches only a `Name => [` expression-bodied span property."""
    pattern = r"\b" + re.escape(name) + (r"\s*=>\s*\[" if span else r"\b[^=\n;]*=\s*\[")
    m = re.search(pattern, source)
    if not m:
        die(f"C# array {name} not found")
    depth, i, quote = 1, m.end(), None
    while depth:
        c = source[i]
        if quote:
            if c == "\\":
                i += 1
            elif c == quote:
                quote = None
        elif c in "\"'":
            quote = c
        elif source.startswith("//", i):
            i = source.index("\n", i)
            continue
        else:
            depth += c == "["
            depth -= c == "]"
        i += 1
    return source[m.end():i - 1]


def split_top(text, sep=","):
    """Split on `sep` at nesting depth 0 (parentheses, braces, brackets), outside strings."""
    parts, depth, cur, quote, escaped = [], 0, [], None, False
    for c in text:
        if quote:
            if escaped:
                escaped = False
            elif c == "\\":
                escaped = True
            elif c == quote:
                quote = None
            cur.append(c)
            continue
        if c in "\"'":
            quote = c
        else:
            if c in "([{":
                depth += 1
            elif c in ")]}":
                depth -= 1
        if c == sep and depth == 0 and not quote:
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


def cs_ints(block):
    return [cs_int(x) for x in split_top(strip_comments(block))]


def cs_string(text, constants):
    text = text.strip()
    if text in constants:
        return constants[text]
    if text == "string.Empty":
        return ""
    m = re.fullmatch(r'"((?:[^"\\]|\\.)*)"', text)
    if not m:
        die(f"not a string literal: {text!r}")
    return m.group(1).encode("utf-8").decode("unicode_escape").encode("latin-1").decode("utf-8") \
        if "\\" in m.group(1) else m.group(1)


def cs_char_table(block, constants):
    """A ReadOnlySpan<char> table of 'x' literals and named char constants."""
    out = []
    for tok in split_top(strip_comments(block)):
        m = re.fullmatch(r"'(\\.|[^'])'", tok)
        if m:
            lit = m.group(1)
            out.append({"\\'": "'", "\\\\": "\\"}.get(lit, lit))
        elif tok in constants:
            out.append(constants[tok])
        else:
            die(f"unknown char table token {tok!r}")
    return out


def moveset(text):
    m = re.fullmatch(r"new\(([\d,\s]*)\)", text.strip())
    if not m:
        die(f"bad moveset {text!r}")
    moves = [int(x) for x in m.group(1).split(",") if x.strip()]
    if len(moves) > 4:
        die(f"moveset longer than 4: {text!r}")
    return moves + [0] * (4 - len(moves))


def enum_values(source, name):
    """Sequential enum member values (implicit numbering only)."""
    m = re.search(r"enum\s+" + re.escape(name) + r"\b[^{]*\{(.*?)\}", source, re.S)
    if not m:
        die(f"enum {name} not found")
    values, cur = {}, -1
    for part in split_top(strip_comments(m.group(1))):
        part = re.sub(r"\[[^\]]*\]", "", part).strip()
        if "=" in part:
            key, val = (x.strip() for x in part.split("=", 1))
            cur = cs_int(val)
        else:
            key, cur = part, cur + 1
        values[key] = cur
    return values


# --------------------------------------------------------------------------- emit

def arr(values, width=16):
    values = list(values)
    lines = []
    for i in range(0, len(values), width):
        lines.append("    " + ", ".join(str(v) for v in values[i:i + width]) + ",")
    return "\n".join(lines) if lines else "    0,"


def u16str(s):
    """A C++ u"" literal; anything outside printable ASCII becomes a \\uXXXX escape."""
    out = []
    for c in s:
        u = ord(c)
        if 0xD800 <= u <= 0xDFFF:
            die(f"lone surrogate U+{u:04X}")
        if u > 0xFFFF:
            out.append(f"\\U{u:08X}")  # a surrogate pair in UTF-16, like the C# string
        elif c in '"\\':
            out.append("\\" + c)
        elif 0x20 <= u < 0x7F:
            out.append(c)
        else:
            out.append(f"\\u{u:04X}")
    return 'u"' + "".join(out) + '"'


def flatten(lists):
    flat, starts = [], [0]
    for items in lists:
        flat.extend(items)
        starts.append(len(flat))
    return flat, starts


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def lower_invariant(s):
    """char.ToLowerInvariant per UTF-16 unit (simple case mapping). Rejects characters
    whose Python lowering is not a single code point, so no ambiguous mapping is emitted."""
    out = []
    for c in s:
        if c in "İΣ":
            die(f"character U+{ord(c):04X} has culture/context dependent lowering")
        low = c.lower()
        if len(low) != 1:
            die(f"character U+{ord(c):04X} lowers to {len(low)} characters")
        out.append(low)
    return "".join(out)


def upper_invariant(s):
    out = []
    for c in s:
        up = c.upper()
        out.append(up if len(up) == 1 else c)  # .NET keeps characters without a 1:1 upper
    return "".join(out)


# --------------------------------------------------------------------------- word filter

REGEX_META = set("()[]{}?*+|^$.\\")


def validate_pattern(p):
    """Accept only the regex subset the C++ matcher implements: literals, '.', escapes of
    punctuation, [...] / [^...] classes (with ranges), (a|b) groups, '?', '*', '+' and
    '^'/'$' anchors at the ends. Anything else stops generation."""
    i, n, depth = 0, len(p), 0
    while i < n:
        c = p[i]
        if c == "\\":
            if i + 1 >= n or p[i + 1].isalnum():
                die(f"unsupported escape in word filter pattern {p!r}")
            i += 2
            continue
        if c == "[":
            j = i + 1
            if j < n and p[j] == "^":
                j += 1
            if j < n and p[j] == "]":
                j += 1
            while j < n and p[j] != "]":
                if p[j] == "\\":
                    if j + 1 >= n or p[j + 1].isalnum():
                        die(f"unsupported class escape in {p!r}")
                    j += 1
                elif p[j] == "[":
                    die(f"nested class in {p!r}")
                j += 1
            if j >= n:
                die(f"unterminated class in {p!r}")
            i = j + 1
            continue
        if c == "{" or c == "}":
            die(f"counted repetition in {p!r}")
        if c == "(":
            if i + 1 < n and p[i + 1] == "?":
                die(f"group construct in {p!r}")
            depth += 1
        elif c == ")":
            depth -= 1
            if depth < 0:
                die(f"unbalanced group in {p!r}")
        elif c == "^" and i != 0:
            die(f"inner '^' anchor in {p!r}")
        elif c == "$" and i != n - 1:
            die(f"inner '$' anchor in {p!r}")
        elif c in "?*+" and (i == 0 or p[i - 1] in "(|"):
            die(f"dangling quantifier in {p!r}")
        elif c in "?*+" and i + 1 < n and p[i + 1] in "?+":
            die(f"lazy/possessive quantifier in {p!r}")
        i += 1
    if depth:
        die(f"unbalanced group in {p!r}")


def load_patterns(path):
    """WordFilter.LoadPatterns: lowercase the whole resource, one regex per line."""
    text = path.read_text(encoding="utf-8-sig")
    lines = lower_invariant(text).split("\n")
    for line in lines:
        if "\r" in line:
            die(f"{path.name}: carriage return in pattern")
        validate_pattern(line)
    return lines


# --------------------------------------------------------------------------- generate

def generate(pkhex):
    core = pkhex / "PKHeX.Core"
    res = core / "Resources"
    inputs = {}

    def src(rel):
        p = core / rel
        if not p.is_file():
            die(f"missing {rel}")
        inputs[rel] = p
        return p

    def read_cs(rel):
        return src(rel).read_text(encoding="utf-8-sig")

    # Personal tables.
    personal = {}
    for game in ("rs", "e", "fr", "lg"):
        data = src(f"Resources/byte/personal/personal_{game}").read_bytes()
        if len(data) != (MAX_SPECIES + 1) * PERSONAL_SIZE:
            die(f"personal_{game}: unexpected size {len(data)}")
        personal[game] = [data[i:i + PERSONAL_SIZE] for i in range(0, len(data), PERSONAL_SIZE)]
    rs = personal["rs"]
    # PK3.PersonalInfo reads personal_rs while evolution/egg logic reads personal_e; the
    # analyzer stores one table, so every field it reads must agree across the four games.
    for game in ("e", "fr", "lg"):
        for sp in range(1, MAX_SPECIES + 1):
            for off, field in ((0x10, "gender ratio"), (0x11, "hatch cycles"), (0x12, "friendship"),
                               (0x13, "growth"), (0x16, "ability 1"), (0x17, "ability 2")):
                if personal[game][sp][off] != rs[sp][off]:
                    die(f"personal_{game} {field} differs from RS for species {sp}")
    machine = binlinker32(src("Resources/byte/personal/hmtm_g3.pkl").read_bytes(), b"g3")
    tutors = binlinker32(src("Resources/byte/personal/tutors_g3.pkl").read_bytes(), b"g3")
    if len(machine) != MAX_SPECIES + 1 or len(tutors) != MAX_SPECIES + 1:
        die("TM/tutor table count mismatch")
    machine_bits, tutor_bits = [0], [0]
    for sp in range(1, MAX_SPECIES + 1):
        machine_bits.append(int.from_bytes(machine[sp].ljust(8, b"\0")[:8], "little") & ((1 << 58) - 1))
        tutor_bits.append(int.from_bytes(tutors[sp].ljust(4, b"\0")[:4], "little") & ((1 << 30) - 1))

    # EntityGender must agree with the personal gender ratio (PKHeX computes PK3 gender from it).
    eg = read_cs("PKM/Util/EntityGender.cs")
    consts = {k: int(v) for k, v in re.findall(r"const byte (\w\w) = (\d+);", eg)}
    ratios = [consts[t] for t in split_top(strip_comments(cs_block(eg, "GenderRatios", span=True)))]
    for sp in range(1, MAX_SPECIES + 1):
        if ratios[sp] != rs[sp][0x10]:
            die(f"EntityGender ratio differs from personal_rs for species {sp}")

    # Learnsets, egg moves, evolutions.
    learn = {g: read_learnsets(src(f"Resources/byte/levelup/lvlmove_{g}.pkl"), ident)
             for g, ident in (("rs", b"rs"), ("e", b"em"), ("fr", b"fr"), ("lg", b"lg"))}
    for g, sets in learn.items():
        if len(sets) != MAX_SPECIES + 1:
            die(f"lvlmove_{g}: {len(sets)} entries")
    eggs = read_move_lists(src("Resources/byte/eggmove/eggmove_rs.pkl"), b"rs")
    evos = read_evolutions(src("Resources/byte/evolve/evos_g3.pkl"), b"g3")
    if len(evos) != MAX_SPECIES + 1:
        die("evos_g3 entry count")
    for sp, methods in enumerate(evos):
        for etype, arg, dest, form, level in methods:
            if form not in (0, 255) or not (1 <= dest <= MAX_SPECIES):
                die(f"unexpected Gen III evolution {sp}->{dest} form {form}")

    # Wild areas (enumeration order is the PKHeX array order).
    wild = []
    for name, ident, bit in (("r", b"ru", R), ("s", b"sa", S), ("e", b"em", E),
                             ("fr", b"fr", FR), ("lg", b"lg", LG)):
        for area in read_areas(src(f"Resources/legality/wild/Gen3/encounter_{name}.pkl"), ident, False):
            wild.append((bit,) + area)
    swarm = [(R | S | E,) + a for a in
             read_areas(src("Resources/legality/wild/Gen3/encounter_rse_swarm.pkl"), b"rs", True)]
    pcny_raw = src("Resources/legality/wild/Gen3/encounter_pcny.pkl").read_bytes()
    if len(pcny_raw) % 12:
        die("encounter_pcny.pkl size")
    pcny = []
    for off in range(0, len(pcny_raw), 12):
        species, = struct.unpack_from("<H", pcny_raw, off)
        level, dist = pcny_raw[off + 2], pcny_raw[off + 3]
        moves = list(struct.unpack_from("<4H", pcny_raw, off + 4))
        pcny.append((species, level, dist, moves))

    # Static, trade and Colosseum gift templates.
    rse = read_cs("Legality/Encounters/Data/Gen3/Encounters3RSE.cs")
    frlg = read_cs("Legality/Encounters/Data/Gen3/Encounters3FRLG.cs")
    statics = []  # (list, species, level, versions, location, form, roaming, egg, fateful, ball, moves)
    static_lists = [("StaticR", rse), ("StaticS", rse), ("StaticE", rse), ("StaticRSE", rse),
                    ("StaticFR", frlg), ("StaticLG", frlg), ("StaticFRLG", frlg)]
    static_ranges = {}
    for name, text in static_lists:
        start = len(statics)
        for args, p in cs_entries(cs_block(text, name)):
            known = {"FixedBall", "Location", "IsRoaming", "IsEgg", "FatefulEncounter", "Form", "Moves"}
            if set(p) - known:
                die(f"{name}: unhandled properties {sorted(set(p) - known)}")
            species, level, version = cs_int(args[0]), cs_int(args[1]), VERSION_BITS[args[2]]
            ball = {"Ball.Poke": 4}[p["FixedBall"]] if "FixedBall" in p else 0
            statics.append(dict(species=species, level=level, versions=version,
                                location=cs_int(p["Location"]), form=cs_int(p.get("Form", "0")),
                                roaming=p.get("IsRoaming") == "true", egg=p.get("IsEgg") == "true",
                                fateful=p.get("FatefulEncounter") == "true", ball=ball,
                                moves=moveset(p["Moves"]) if "Moves" in p else [0, 0, 0, 0]))
        static_ranges[name] = (start, len(statics) - start)

    def trade_contests(text):
        out = {}
        for nm, body in re.findall(r"(TradeContest_\w+)\s*=>\s*\[([^\]]*)\]", text):
            out[nm] = [int(x) for x in body.split(",") if x.strip()]
        return out

    trades, trade_ranges = [], {}
    for name, text, names_set in (("TradeGift_RS", rse, 0), ("TradeGift_E", rse, 0),
                                  ("TradeGift_FRLG", frlg, 1), ("TradeGift_FR", frlg, 1),
                                  ("TradeGift_LG", frlg, 1)):
        contests = trade_contests(text)
        start = len(trades)
        for args, p in cs_entries(cs_block(text, name)):
            _, index, version, pid, species, level = args
            ivs = [int(x) for x in re.fullmatch(r"new\(([\d,\s]+)\)", p["IVs"]).group(1).split(",")]
            trades.append(dict(index=cs_int(index), versions=VERSION_BITS[version], pid=cs_int(pid),
                               species=cs_int(species), level=cs_int(level),
                               ability=ABILITY[p["Ability"]], tid=cs_int(p["TID16"]),
                               sid=cs_int(p.get("SID16", "0")), ot_gender=cs_int(p["OTGender"]),
                               gender=cs_int(p["Gender"]), ivs=ivs, contest=contests[p["Contest"]],
                               names=names_set))
        trade_ranges[name] = (start, len(trades) - start)

    trade_names = []
    for set_name in ("traderse", "tradefrlg"):
        per_lang = {}
        for lang in (1, 2, 3, 4, 5, 7):
            per_lang[lang] = text_lines(src(f"Resources/legality/gen3/text_{set_name}_{LANGUAGES[lang]}.txt"))
        trade_names.append(per_lang)

    colo_consts = {}
    for nm, body in re.findall(r"string\[\]\s+(\w+)\s*=\s*\[([^\]]*)\];", rse):
        colo_consts[nm] = [cs_string(x, {}) for x in split_top(body)]
    colo = []
    for name, version in (("ColoGiftsR", R), ("ColoGiftsS", S)):
        for args, p in cs_entries(cs_block(rse, name)):
            colo.append(dict(species=cs_int(args[0]), level=cs_int(args[1]), names=colo_consts[args[2]],
                             version=VERSION_BITS[args[3]], location=cs_int(p["Location"]),
                             tid=cs_int(p["TID16"]), ot_gender=cs_int(p["OriginalTrainerGender"]),
                             moves=moveset(p["Moves"]) if "Moves" in p else [0, 0, 0, 0]))

    # WC3 gifts, in Encounter_WC3 order: Common, International, Japan, Eggs.
    wc3 = read_cs("Legality/Encounters/Data/Gen3/EncountersWC3.cs")
    wc3_consts = {nm: val for nm, val in re.findall(r'const string (\w+) = "([^"]*)";', wc3)}
    order = re.search(r"Encounter_WC3\s*=\s*\[([^\]]*)\]", wc3).group(1)
    order = [x.strip().lstrip(".").strip() for x in order.split(",")]
    if order != ["Common", "International", "Japan", "Eggs"]:
        die(f"unexpected Encounter_WC3 order {order}")
    gifts = []
    for name in order:
        for args, p in cs_entries(cs_block(wc3, name)):
            known = {"Moves", "Method", "ID32", "TID16", "Shiny", "OriginalTrainerName",
                     "OriginalTrainerGender", "Language", "FatefulEncounter", "RibbonNational"}
            if set(p) - known:
                die(f"WC3 {name}: unhandled properties {sorted(set(p) - known)}")
            pos = [a for a in args if ":" not in a]
            named = {k.strip(): v.strip() for k, v in (a.split(":", 1) for a in args if ":" in a)}
            species, level, version = cs_int(pos[0]), cs_int(pos[1]), VERSION_BITS[pos[2]]
            egg = len(pos) > 3 and pos[3] == "true"
            met = cs_int(pos[4]) if len(pos) > 4 else cs_int(named["met"]) if "met" in named else (0 if len(pos) > 3 else level)
            tid, sid = 0xFFFF, 0xFFFF
            if "ID32" in p:
                id32 = cs_int(p["ID32"])
                tid, sid = id32 & 0xFFFF, id32 >> 16
            if "TID16" in p:
                tid = cs_int(p["TID16"])
            lang = 0
            if "Language" in p:
                lang = LANG_NAMES[re.fullmatch(r"\(int\)(\w+)", p["Language"]).group(1)]
            gifts.append(dict(species=species, level=level, versions=version, egg=egg, met=met,
                              method=PID_TYPES.index(p["Method"]), moves=moveset(p["Moves"]),
                              ot=cs_string(p["OriginalTrainerName"], wc3_consts),
                              ot_gender=GIFT_GENDER.index(p.get("OriginalTrainerGender", "Recipient")),
                              tid=tid, sid=sid, language=lang, shiny=SHINY[p.get("Shiny", "Random")],
                              fateful=p.get("FatefulEncounter") == "true",
                              national=p.get("RibbonNational") == "true"))

    # PCJP (Gen3PCJP.GetArray order).
    pcjp_src = read_cs("Legality/Encounters/Templates/Gen3/Gifts/Distribution3JPN.cs")
    pcjp = []
    for dist, nm in enumerate(("Distro1", "Distro2", "Distro3", "Distro4", "Distro5", "Distro6")):
        for sp in cs_ints(cs_block(pcjp_src, nm, span=True)):
            pcjp.append((sp, dist))

    # Character tables (StringConverter3) and quote characters.
    sc3 = read_cs("PKM/Strings/StringConverter3.cs")
    sc4u = read_cs("PKM/Strings/StringConverter4Util.cs")
    char_consts = {"Terminator": "ÿ", "FGM": "♂", "FGF": "♀"}
    hgm = re.search(r"const char HGM = '\\u([0-9A-Fa-f]{4})'", sc4u).group(1)
    hgf = re.search(r"const char HGF = '\\u([0-9A-Fa-f]{4})'", sc4u).group(1)
    char_consts["HGM"], char_consts["HGF"] = chr(int(hgm, 16)), chr(int(hgf, 16))
    g3_en = cs_char_table(cs_block(sc3, "G3_EN", span=True), char_consts)
    g3_jp = cs_char_table(cs_block(sc3, "G3_JP", span=True), char_consts)
    if len(g3_en) != 256 or len(g3_jp) != 256:
        die("G3 character tables must have 256 entries")

    # Species names.
    species_lists = {}
    for lang, code in LANGUAGES.items():
        species_lists[lang] = text_lines(src(f"Resources/text/other/{code}/text_Species_{code}.txt"))
    farfetchd = 83

    def gen3_name(sp, lang):
        nick = species_lists[lang][sp]
        if lang == 1:
            return nick
        out = list(upper_invariant(nick))
        if lang == 3:  # StripDiacriticsFR4
            for i, c in enumerate(out):
                delta = ord(c) - ord("È")
                if 0 <= delta <= 7:
                    if delta < 3:
                        out[i] = "E"
                    elif delta == 7:
                        out[i] = "I"
        if sp == farfetchd and lang in (2, 4, 7):
            out[-2] = "'"
        return "".join(out)

    gen3_names = {lang: [""] + [gen3_name(sp, lang) for sp in range(1, MAX_SPECIES + 1)]
                  for lang in GEN3_LANGUAGES}

    def dictionary(langs, lower, max_species=None):
        """SpeciesName.GetDictionary per language: later species overwrite earlier keys."""
        out = {}
        for lang in langs:
            d = {}
            names = species_lists[lang]
            for sp in range(1, len(names)):
                key = lower_invariant(names[sp]) if lower else names[sp]
                d[key] = sp
            out[lang] = d
        return out

    dict_exact = dictionary(GEN3_LANGUAGES, False)
    dict_lower = dictionary(GEN7_LANGUAGES, True)

    # Small C# tables.
    exp_src = read_cs("PKM/Util/Experience.cs")
    exp = [cs_ints(cs_block(exp_src, f"Growth{g}", span=True)) for g in range(6)]
    for t in exp:
        if len(t) != 100:
            die("EXP table length")
    pp = cs_ints(cs_block(read_cs("Moves/MoveInfo3.cs"), "PP", span=True))
    if len(pp) != MAX_MOVE + 1:
        die("MoveInfo3.PP length")
    items = read_cs("Items/ItemStorage3RS.cs")
    held = set()
    for nm in ("General", "Balls", "Berry"):
        held |= set(cs_ints(cs_block(items, nm, span=True)))
    held |= set(cs_ints(cs_block(items, "Machine", span=True))[:50])
    unreleased = re.search(r"Unreleased\s*=>\s*\[([^\]]*)\]", items).group(1)
    held -= {cs_int(x) for x in unreleased.split(",") if x.strip()}
    max_item = int(re.search(r"MaxItemID_3_RS = (\d+);", read_cs("Legality/Legal.cs")).group(1))
    if max(held) > max_item:
        die("held item above MaxItemID_3_RS")

    trash_src = strip_comments(read_cs("PKM/Strings/Trash/TrashTrainer3.cs"))
    trash = []  # (version bit, language, gender, 7 bytes)
    for fn, bit in (("IsPatternDefaultR", R), ("IsPatternDefaultS", S), ("IsPatternDefaultE", E)):
        m = re.search(r"bool " + fn + r"\(.*?\) => language switch\s*\{(.*?)\n    \};", trash_src, re.S)
        if not m:
            die(f"{fn} not found")
        body = m.group(1)
        for lang_name, inner in re.findall(r"(\w+) => trash switch\s*\{(.*?)\}", body, re.S):
            lang = LANG_NAMES[lang_name]
            for raw, gender in re.findall(r"\[([0-9A-Fa-fx,\s]+)\]\s*=>\s*gender == (\d)", inner):
                values = [cs_int(x) for x in raw.split(",")]
                if len(values) != 7:
                    die("trash pattern length")
                trash.append((bit, lang, int(gender), values))
    if len(trash) < 50:
        die(f"suspiciously few default trainer trash patterns ({len(trash)})")

    hatch = cs_ints(cs_block(read_cs("Legality/Encounters/Verifiers/EggHatchLocation3.cs"), "LocationPermitted3", span=True))
    conv = read_cs("PKM/Util/Conversion/SpeciesConverter.cs")
    i2n = cs_ints(cs_block(conv, "Table3InternalToNational", span=True))
    first_internal = int(re.search(r"FirstUnalignedInternal3 = (\d+);", conv).group(1))
    internal_to_national = []
    for raw in range(first_internal + len(i2n)):
        if raw < 252:
            internal_to_national.append(raw)
        elif raw < first_internal:
            internal_to_national.append(0)
        else:
            internal_to_national.append(raw + i2n[raw - first_internal])
    natl = sorted(x for x in internal_to_national if x)
    if natl != list(range(1, MAX_SPECIES + 1)):
        die("internal species table is not a bijection onto 1-386")

    mew_seeds = cs_ints(cs_block(read_cs("Legality/RNG/ClassicEra/Gen3/MystryMew.cs"), "Seeds", span=True))
    if mew_seeds != sorted(mew_seeds):
        die("MystryMew seeds must be sorted (binary search)")
    amps = cs_ints(cs_block(read_cs("Editing/NatureAmp.cs"), "Table", span=True))
    if len(amps) != 125:
        die("NatureAmp table length")

    moves_enum = enum_values(read_cs("Game/Enums/Move.cs"), "Move")
    pi3 = read_cs("PersonalInfo/Info/PersonalInfo3.cs")
    tm = cs_ints(cs_block(pi3, "MachineMovesTechnical", span=True))
    hm = cs_ints(cs_block(pi3, "MachineMovesHidden", span=True))
    if len(tm) != 50 or len(hm) != 8:
        die("machine move lists")
    ls_e = read_cs("Legality/LearnSource/Sources/LearnSource3E.cs")
    tutor_e = cs_ints(cs_block(ls_e, "Tutor_E", span=True))
    if len(tutor_e) != 30:
        die("Tutor_E length")
    ls_rs = read_cs("Legality/LearnSource/Sources/LearnSource3RS.cs")
    mew_tutor = [moves_enum[x.strip().replace("(int)Move.", "")]
                 for x in split_top(strip_comments(cs_block(ls_rs, "Tutor_3Mew", span=True)))]
    xd_sd = cs_ints(cs_block(ls_rs, "SpecialTutors_XD_SelfDestruct", span=True))
    xd_sky = cs_ints(cs_block(ls_rs, "SpecialTutors_XD_SkyAttack", span=True))
    xd_nm = cs_ints(cs_block(ls_rs, "SpecialTutors_XD_Nightmare", span=True))
    for lst in (xd_sd, xd_sky, xd_nm):
        if lst != sorted(lst):
            die("XD tutor lists must be sorted (binary search)")
    named_moves = {k: moves_enum[k] for k in ("SelfDestruct", "SkyAttack", "Nightmare", "BlastBurn",
                                              "HydroCannon", "FrenzyPlant", "VoltTackle", "Wish",
                                              "Struggle", "Sketch")}

    badwords_switch = load_patterns(src("Resources/text/badwords/badwords_switch.txt"))
    badwords_3ds = load_patterns(src("Resources/text/badwords/badwords_3ds.txt"))
    max_species_7 = int(re.search(r"MaxSpeciesID_7_USUM = (\d+);", read_cs("Legality/Legal.cs")).group(1)) \
        if re.search(r"MaxSpeciesID_7_USUM = (\d+);", read_cs("Legality/Legal.cs")) else None
    if max_species_7 is None:
        die("MaxSpeciesID_7_USUM not found")

    pin = PINNED_PKHEX
    return render(locals())


def render(v):
    pin = v["pin"]
    header = ("// GENERATED by tools/gen_legality_gen3_data.py -- do not edit by hand.\n"
              f"// Source: kwsch/PKHeX@{pin} (PKHeX.Core legality resources, GPL-3.0).\n"
              "// Used under GPL-3.0 section 13 alongside this AGPL-3.0 project.\n")
    rs = v["personal"]["rs"]
    statics, trades, gifts, colo = v["statics"], v["trades"], v["gifts"], v["colo"]
    wild, swarm = v["wild"], v["swarm"]
    areas = wild + swarm

    h = [header, "#pragma once", "", "#include <cstddef>", "#include <cstdint>", "",
         "namespace Legality::Gen3Data {", "",
         "/// Version bits (GameVersion S=1, R=2, E=3, FR=4, LG=5, CXD=15).",
         "enum Version : uint8_t { S = 0x01, R = 0x02, E = 0x04, FR = 0x08, LG = 0x10, CXD = 0x20 };",
         "",
         "/// PersonalInfo3 fields read by the analyzer (personal_rs; PK3.PersonalInfo is RS).",
         "struct Personal { uint8_t gender; uint8_t hatchCycles; uint8_t friendship; uint8_t growth;",
         "                  uint8_t ability1; uint8_t ability2; };",
         f"inline constexpr std::size_t kMaxSpecies = {MAX_SPECIES};",
         f"inline constexpr std::size_t kMaxMove = {MAX_MOVE};",
         "extern const Personal kPersonal[kMaxSpecies + 1];",
         "/// hmtm_g3.pkl: TM01-TM50 then HM01-HM08 (bit i), shared by every Gen III table.",
         "extern const uint64_t kMachineBits[kMaxSpecies + 1];",
         "/// tutors_g3.pkl: one bit per LearnSource3E.Tutor_E entry.",
         "extern const uint32_t kTutorBits[kMaxSpecies + 1];",
         "extern const uint16_t kMachineMovesTechnical[50];",
         "extern const uint16_t kMachineMovesHidden[8];",
         "extern const uint16_t kTutorE[30];",
         "extern const uint16_t kTutorMew[6];",
         f"inline constexpr std::size_t kXDSelfDestructCount = {len(v['xd_sd'])};",
         f"inline constexpr std::size_t kXDSkyAttackCount = {len(v['xd_sky'])};",
         f"inline constexpr std::size_t kXDNightmareCount = {len(v['xd_nm'])};",
         "extern const uint16_t kXDSelfDestruct[]; extern const uint16_t kXDSkyAttack[];",
         "extern const uint16_t kXDNightmare[]; // ascending",
         ""]
    for k, val in v["named_moves"].items():
        h.append(f"inline constexpr uint16_t kMove{k} = {val};")
    h += ["",
          "struct LevelMove { uint16_t move; uint8_t level; };",
          "/// Learnset tables: 0 RS, 1 E, 2 FR, 3 LG; entries of species s are",
          "/// kLevelUp[g][kLevelUpStart[g][s] .. kLevelUpStart[g][s + 1]).",
          "extern const LevelMove* const kLevelUp[4];",
          "extern const uint16_t* const kLevelUpStart[4];",
          f"inline constexpr std::size_t kEggMoveCount = {len(v['eggs'])}; // species index bound",
          "extern const uint16_t kEggMoves[]; extern const uint16_t kEggMoveStart[];",
          "",
          "/// EvolutionSet entry: PKHeX EvolutionType, argument, target species/form, level.",
          "struct Evolution { uint8_t type; uint16_t argument; uint16_t species; uint8_t form; uint8_t level; };",
          "extern const Evolution kEvolutions[]; extern const uint16_t kEvolutionStart[kMaxSpecies + 2];",
          "",
          "/// EncounterArea3 / EncounterSlot3 (swarm slots carry their moves).",
          "struct Slot { uint16_t species; uint8_t form; uint8_t slot; uint8_t minLevel; uint8_t maxLevel;",
          "              uint8_t magnetIndex, magnetCount, staticIndex, staticCount; uint16_t moves[4]; };",
          "/// type: SlotType3 (0 grass, 1 surf, 2 old rod, 3 good rod, 4 super rod, 5 rock smash, 6 swarm grass).",
          "struct Area { uint8_t version; uint8_t location; uint8_t type; uint8_t rate; uint16_t slotStart; uint16_t slotCount; };",
          f"inline constexpr std::size_t kAreaCount = {len(wild)};       // R, S, E, FR, LG in PKHeX order",
          f"inline constexpr std::size_t kSwarmAreaCount = {len(swarm)};  // kAreas[kAreaCount ..]",
          "extern const Area kAreas[]; extern const Slot kSlots[];",
          "",
          "/// EncounterStatic3. ball 4 = fixed Poke Ball gift, 0 = not fixed.",
          "struct Static { uint16_t species; uint8_t level; uint8_t versions; uint8_t location; uint8_t form;",
          "                bool roaming; bool egg; bool fateful; uint8_t ball; uint16_t moves[4]; };",
          "extern const Static kStatics[];",
          "struct Range { uint16_t start; uint16_t count; };"]
    for name, (start, count) in v["static_ranges"].items():
        h.append(f"inline constexpr Range k{name}{{{start}, {count}}};")
    h += ["",
          "/// EncounterTrade3. ability 1 = OnlyFirst, 2 = OnlySecond. names: 0 RSE, 1 FRLG.",
          "struct Trade { uint16_t species; uint8_t level; uint8_t versions; uint32_t pid; uint8_t ability;",
          "               uint16_t tid; uint16_t sid; uint8_t otGender; uint8_t gender; uint8_t ivs[6];",
          "               uint8_t contest[6]; uint8_t nameIndex; uint8_t names; };",
          "extern const Trade kTrades[];"]
    for name, (start, count) in v["trade_ranges"].items():
        h.append(f"inline constexpr Range k{name}{{{start}, {count}}};")
    tn = v["trade_names"]
    h += [f"inline constexpr std::size_t kTradeNameCountRSE = {len(tn[0][2])};",
          f"inline constexpr std::size_t kTradeNameCountFRLG = {len(tn[1][2])};",
          "/// Trade nickname/OT strings per language id (0..7, empty for 0 and 6).",
          "extern const char16_t* const* const kTradeNames[2][8];",
          "extern const uint8_t kTradeNameCounts[2][8];",
          "",
          "/// EncounterGift3Colo (Colosseum bonus disc / Mt. Battle), names per language 0..7.",
          "struct ColoGift { uint16_t species; uint8_t level; uint8_t version; uint8_t location; uint16_t tid;",
          "                  uint8_t otGender; uint16_t moves[4]; const char16_t* names[8]; };",
          f"inline constexpr std::size_t kColoGiftCount = {len(colo)};",
          "extern const ColoGift kColoGifts[]; // ColoGiftsR then ColoGiftsS",
          "",
          "/// EncounterGift3 (EncountersWC3.Encounter_WC3 order). tid/sid 0xFFFF = unspecified.",
          "/// method: PIDType index; otGender: GiftGender3 index; shiny 0 random, 1 never, 2 always.",
          "struct Gift { uint16_t species; uint8_t level; uint8_t versions; bool egg; uint8_t metLevel;",
          "              uint8_t method; uint16_t moves[4]; const char16_t* ot; uint8_t otGender;",
          "              uint16_t tid; uint16_t sid; uint8_t language; uint8_t shiny; bool fateful; bool national; };",
          f"inline constexpr std::size_t kGiftCount = {len(gifts)};",
          "extern const Gift kGifts[];",
          "struct GiftNY { uint16_t species; uint8_t level; uint8_t distribution; uint16_t moves[4]; };",
          f"inline constexpr std::size_t kGiftNYCount = {len(v['pcny'])};",
          "extern const GiftNY kGiftsNY[];",
          "struct GiftJPN { uint16_t species; uint8_t distribution; };",
          f"inline constexpr std::size_t kGiftJPNCount = {len(v['pcjp'])};",
          "extern const GiftJPN kGiftsJPN[];",
          "",
          "/// StringConverter3 G3_EN / G3_JP (0x00FF terminates).",
          "extern const char16_t kCharsEN[256]; extern const char16_t kCharsJP[256];",
          "/// SpeciesName.GetSpeciesNameGeneration(species, language, 3) for languages 1-5, 7.",
          "extern const char16_t* const kSpeciesNames3[8][kMaxSpecies + 1];",
          "/// Species-name dictionaries (SpeciesName.SpeciesDict / SpeciesDictLower), sorted by key.",
          "struct NameKey { const char16_t* key; uint16_t species; };",
          "extern const NameKey* const kNameDict[11];      extern const uint16_t kNameDictCount[11];",
          "extern const NameKey* const kNameDictLower[11]; extern const uint16_t kNameDictLowerCount[11];",
          f"inline constexpr uint16_t kMaxSpecies7 = {v['max_species_7']};",
          "",
          "extern const uint32_t kExpTable[6][100];",
          "extern const uint8_t kMovePP[kMaxMove + 1];",
          f"inline constexpr std::size_t kHeldItemMax = {v['max_item']};",
          "extern const uint8_t kHeldItemBits[(kHeldItemMax + 8) / 8];",
          "struct TrashPattern { uint8_t version; uint8_t language; uint8_t gender; uint8_t bytes[7]; };",
          f"inline constexpr std::size_t kTrashPatternCount = {len(v['trash'])};",
          "extern const TrashPattern kTrashPatterns[];",
          f"inline constexpr std::size_t kHatchLocationCount = {len(v['hatch'])};",
          "extern const uint8_t kHatchLocations[]; // EggHatchLocation3: bit0 RS, bit1 E, bit2 FRLG",
          f"inline constexpr std::size_t kInternalSpeciesCount = {len(v['internal_to_national'])};",
          "extern const uint16_t kInternalToNational[];",
          f"inline constexpr std::size_t kMystryMewSeedCount = {len(v['mew_seeds'])};",
          "extern const uint16_t kMystryMewSeeds[];",
          "extern const int8_t kNatureAmps[125]; // NatureAmp.Table",
          f"inline constexpr std::size_t kBadwordSwitchCount = {len(v['badwords_switch'])};",
          f"inline constexpr std::size_t kBadword3DSCount = {len(v['badwords_3ds'])};",
          "/// WordFilterNX / WordFilter3DS patterns, already lowercased like WordFilter.LoadPatterns.",
          "extern const char16_t* const kBadwordsSwitch[]; extern const char16_t* const kBadwords3DS[];",
          "",
          "/// SHA-256 of every PKHeX input, so a regenerated table can be traced to its source.",
          "struct SourcePin { const char* path; const char* sha256; };",
          f"inline constexpr std::size_t kSourcePinCount = {len(v['inputs'])};",
          "extern const SourcePin kSourcePins[];",
          f'inline constexpr const char* kPkhexCommit = "{pin}";',
          "", "} // namespace Legality::Gen3Data", ""]

    c = [header, '#include "Legality/Gen3LegalityData.h"', "", "namespace Legality::Gen3Data {", ""]
    c.append("const Personal kPersonal[kMaxSpecies + 1] = {")
    for rec in rs:
        c.append(f"    {{{rec[0x10]}, {rec[0x11]}, {rec[0x12]}, {rec[0x13]}, {rec[0x16]}, {rec[0x17]}}},")
    c += ["};", ""]
    c += ["const uint64_t kMachineBits[kMaxSpecies + 1] = {",
          arr((f"0x{x:016X}ull" for x in v["machine_bits"]), 4), "};",
          "const uint32_t kTutorBits[kMaxSpecies + 1] = {", arr((f"0x{x:08X}u" for x in v["tutor_bits"]), 8), "};",
          "const uint16_t kMachineMovesTechnical[50] = {", arr(v["tm"], 10), "};",
          "const uint16_t kMachineMovesHidden[8] = {", arr(v["hm"]), "};",
          "const uint16_t kTutorE[30] = {", arr(v["tutor_e"], 15), "};",
          "const uint16_t kTutorMew[6] = {", arr(v["mew_tutor"]), "};",
          "const uint16_t kXDSelfDestruct[] = {", arr(v["xd_sd"]), "};",
          "const uint16_t kXDSkyAttack[] = {", arr(v["xd_sky"]), "};",
          "const uint16_t kXDNightmare[] = {", arr(v["xd_nm"]), "};", ""]
    for g in ("rs", "e", "fr", "lg"):
        flat, starts = flatten(v["learn"][g])
        c += [f"static const LevelMove kLevelUp_{g}[] = {{",
              "\n".join(f"    {{{m}, {lv}}}," for m, lv in flat), "};",
              f"static const uint16_t kLevelUpStart_{g}[] = {{", arr(starts), "};", ""]
    c += ["const LevelMove* const kLevelUp[4] = {kLevelUp_rs, kLevelUp_e, kLevelUp_fr, kLevelUp_lg};",
          "const uint16_t* const kLevelUpStart[4] = {kLevelUpStart_rs, kLevelUpStart_e, kLevelUpStart_fr, kLevelUpStart_lg};",
          ""]
    flat, starts = flatten(v["eggs"])
    c += ["const uint16_t kEggMoves[] = {", arr(flat), "};",
          "const uint16_t kEggMoveStart[] = {", arr(starts), "};", ""]
    flat, starts = flatten(v["evos"])
    c += ["const Evolution kEvolutions[] = {",
          "\n".join(f"    {{{t}, {a}, {s}, {f}, {lv}}}," for t, a, s, f, lv in flat), "};",
          "const uint16_t kEvolutionStart[kMaxSpecies + 2] = {", arr(starts), "};", ""]
    area_rows, slot_rows, start = [], [], 0
    for bit, location, stype, rate, slots in areas:
        area_rows.append(f"    {{{bit}, {location}, {stype}, {rate}, {start}, {len(slots)}}},")
        for sp, fm, sl, lo, hi, mi, mc, si, sc, mv in slots:
            slot_rows.append(f"    {{{sp}, {fm}, {sl}, {lo}, {hi}, {mi}, {mc}, {si}, {sc}, "
                             f"{{{mv[0]}, {mv[1]}, {mv[2]}, {mv[3]}}}}},")
        start += len(slots)
    c += ["const Area kAreas[] = {", "\n".join(area_rows), "};",
          "const Slot kSlots[] = {", "\n".join(slot_rows), "};", ""]
    c.append("const Static kStatics[] = {")
    for s in statics:
        mv = s["moves"]
        c.append(f"    {{{s['species']}, {s['level']}, {s['versions']}, {s['location']}, {s['form']}, "
                 f"{str(s['roaming']).lower()}, {str(s['egg']).lower()}, {str(s['fateful']).lower()}, "
                 f"{s['ball']}, {{{mv[0]}, {mv[1]}, {mv[2]}, {mv[3]}}}}},")
    c += ["};", ""]
    c.append("const Trade kTrades[] = {")
    for t in trades:
        c.append(f"    {{{t['species']}, {t['level']}, {t['versions']}, 0x{t['pid']:08X}u, {t['ability']}, "
                 f"{t['tid']}, {t['sid']}, {t['ot_gender']}, {t['gender']}, "
                 f"{{{', '.join(map(str, t['ivs']))}}}, {{{', '.join(map(str, t['contest']))}}}, "
                 f"{t['index']}, {t['names']}}},")
    c += ["};", ""]
    name_refs = []
    for si, per_lang in enumerate(tn):
        refs = []
        for lang in range(8):
            names = per_lang.get(lang, [])
            if names:
                c.append(f"static const char16_t* const kTradeNames_{si}_{lang}[] = {{")
                c += [f"    {u16str(n)}," for n in names]
                c.append("};")
                refs.append(f"kTradeNames_{si}_{lang}")
            else:
                refs.append("nullptr")
        name_refs.append(refs)
    c += ["const char16_t* const* const kTradeNames[2][8] = {"]
    c += ["    {" + ", ".join(r) + "}," for r in name_refs]
    c += ["};", "const uint8_t kTradeNameCounts[2][8] = {"]
    c += ["    {" + ", ".join(str(len(per_lang.get(lang, []))) for lang in range(8)) + "}," for per_lang in tn]
    c += ["};", ""]
    c.append("const ColoGift kColoGifts[] = {")
    for g in colo:
        mv = g["moves"]
        c.append(f"    {{{g['species']}, {g['level']}, {g['version']}, {g['location']}, {g['tid']}, "
                 f"{g['ot_gender']}, {{{mv[0]}, {mv[1]}, {mv[2]}, {mv[3]}}}, "
                 f"{{{', '.join(u16str(n) for n in g['names'])}}}}},")
    c += ["};", ""]
    c.append("const Gift kGifts[] = {")
    for g in gifts:
        mv = g["moves"]
        c.append(f"    {{{g['species']}, {g['level']}, {g['versions']}, {str(g['egg']).lower()}, {g['met']}, "
                 f"{g['method']}, {{{mv[0]}, {mv[1]}, {mv[2]}, {mv[3]}}}, {u16str(g['ot'])}, {g['ot_gender']}, "
                 f"{g['tid']}, {g['sid']}, {g['language']}, {g['shiny']}, {str(g['fateful']).lower()}, "
                 f"{str(g['national']).lower()}}},")
    c += ["};", ""]
    c.append("const GiftNY kGiftsNY[] = {")
    for sp, lv, dist, mv in v["pcny"]:
        c.append(f"    {{{sp}, {lv}, {dist}, {{{mv[0]}, {mv[1]}, {mv[2]}, {mv[3]}}}}},")
    c += ["};", "const GiftJPN kGiftsJPN[] = {"]
    c += [f"    {{{sp}, {dist}}}," for sp, dist in v["pcjp"]]
    c += ["};", ""]
    c += ["const char16_t kCharsEN[256] = {", arr((f"0x{ord(x):04X}" for x in v["g3_en"]), 16), "};",
          "const char16_t kCharsJP[256] = {", arr((f"0x{ord(x):04X}" for x in v["g3_jp"]), 16), "};", ""]
    c.append("const char16_t* const kSpeciesNames3[8][kMaxSpecies + 1] = {")
    for lang in range(8):
        if lang not in GEN3_LANGUAGES:
            c.append("    {},")
            continue
        c.append("    {")
        c += [f"        {u16str(n)}," for n in v["gen3_names"][lang]]
        c.append("    },")
    c += ["};", ""]

    def emit_dict(name, dicts):
        out = []
        refs, counts = [], []
        for lang in range(11):
            d = dicts.get(lang)
            if not d:
                refs.append("nullptr")
                counts.append("0")
                continue
            keys = sorted(d.items(), key=lambda kv: kv[0].encode("utf-16-be"))
            out.append(f"static const NameKey {name}_{lang}[] = {{")
            out += [f"    {{{u16str(k)}, {sp}}}," for k, sp in keys]
            out.append("};")
            refs.append(f"{name}_{lang}")
            counts.append(str(len(keys)))
        out.append(f"const NameKey* const {name}[11] = {{{', '.join(refs)}}};")
        out.append(f"const uint16_t {name}Count[11] = {{{', '.join(counts)}}};")
        return out

    c += emit_dict("kNameDict", v["dict_exact"])
    c += emit_dict("kNameDictLower", v["dict_lower"])
    c.append("")
    c.append("const uint32_t kExpTable[6][100] = {")
    for t in v["exp"]:
        c += ["    {", arr(t, 10), "    },"]
    c += ["};", "const uint8_t kMovePP[kMaxMove + 1] = {", arr(v["pp"], 20), "};"]
    bits = [0] * ((v["max_item"] + 8) // 8)
    for it in v["held"]:
        bits[it >> 3] |= 1 << (it & 7)
    c += ["const uint8_t kHeldItemBits[(kHeldItemMax + 8) / 8] = {", arr((f"0x{b:02X}" for b in bits), 16), "};", ""]
    c.append("const TrashPattern kTrashPatterns[] = {")
    for bit, lang, gender, raw in v["trash"]:
        c.append(f"    {{{bit}, {lang}, {gender}, {{{', '.join(f'0x{b:02X}' for b in raw)}}}}},")
    c += ["};", "const uint8_t kHatchLocations[] = {", arr(v["hatch"], 20), "};",
          "const uint16_t kInternalToNational[] = {", arr(v["internal_to_national"], 20), "};",
          "const uint16_t kMystryMewSeeds[] = {", arr((f"0x{x:04X}" for x in v["mew_seeds"]), 10), "};",
          "const int8_t kNatureAmps[125] = {", arr(v["amps"], 5), "};", ""]
    for name, pats in (("kBadwordsSwitch", v["badwords_switch"]), ("kBadwords3DS", v["badwords_3ds"])):
        c.append(f"const char16_t* const {name}[] = {{")
        c += [f"    {u16str(p)}," for p in pats]
        c += ["};", ""]
    c.append("const SourcePin kSourcePins[] = {")
    for rel, path in sorted(v["inputs"].items()):
        c.append(f'    {{"PKHeX.Core/{rel}", "{sha256(path)}"}},')
    c += ["};", "", "} // namespace Legality::Gen3Data", ""]
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
    targets = ((REPO / "include/Legality/Gen3LegalityData.h", header),
               (REPO / "src/Legality/Gen3LegalityData.cpp", source))
    if args.check:
        stale = [str(p.relative_to(REPO)) for p, text in targets
                 if not p.is_file() or p.read_text(encoding="utf-8") != text]
        if stale:
            die("generated Gen III tables are stale: " + ", ".join(stale))
        print("Gen III legality tables match the pinned PKHeX resources")
        return
    for path, text in targets:
        path.write_text(text, encoding="utf-8")
        print(f"wrote {path.relative_to(REPO)}")


if __name__ == "__main__":
    main()
