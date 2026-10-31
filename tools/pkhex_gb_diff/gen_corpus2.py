#!/usr/bin/env python3
"""Generate a corpus of raw Gen II party records for the PKHeX differential test.

Each output line is one Pokemon exactly as a Gold/Silver/Crystal save stores it:

    <id> <kind> <language> <48-byte party body hex> <OT name hex> <nickname hex> <egg 0|1>

<language> is PKHeX's LanguageID for the name encoding: 1 Japanese, 8 Korean, 0 any other.

Like gen_corpus.py (Gen I), it mixes Pokemon built the way the games build them
(wild slots with and without Crystal met data, statics, in-game trades, event
gifts, daycare eggs, Odd Eggs, Gen I Pokemon brought over by the Time Capsule),
then levelled, evolved and taught TMs, with deliberately broken variants and
random records. Nothing here decides legality; run.sh compares PKHeX's verdict
with PokeBank NX's on the same bytes. Japanese records use 6-byte name fields and
the Japanese character table from PKHeX's StringConverter2, including the
Japanese-only event gifts and in-game trade names. Korean Gold/Silver records use
the two-byte Hangul tables from PKHeX's StringConverter2KOR (the trainer name's
first byte, a Hangul page 1-11, is what marks a PK2 as Korean); the kor2-* kinds
aim at one Korean rule each.

Usage:
    python3 tools/pkhex_gb_diff/gen_corpus2.py --pkhex PKHEX --pokered POKERED \
        [--count 20000] [--seed 1] > corpus2.txt
"""

import argparse
import random
import re
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent))
sys.path.insert(0, str(HERE))
import gen_gb_legality_data as gen  # noqa: E402
import gen_corpus as g1  # noqa: E402
import gb_text as gbtext  # noqa: E402

GD, SI, C = gen.GD, gen.SI, gen.C
TERMINATOR = 0x50
TEXT = {c: 0x80 + i for i, c in enumerate("ABCDEFGHIJKLMNOPQRSTUVWXYZ")}
TEXT.update({c: 0xA0 + i for i, c in enumerate("abcdefghijklmnopqrstuvwxyz")})
TEXT[" "] = 0x7F
NO_HATCH = {132, 144, 145, 146, 150, 151, 201, 243, 244, 245, 249, 250, 251}  # Breeding.IsAbleToHatchFromEgg
HEADBUTT = (6, 7)
TREE_LOCATIONS = [2, 4, 5, 8, 11, 12, 14, 15, 18, 20, 21, 25, 26, 34, 37, 38, 39, 91, 92]
TREES = [0x3FF3FF, 0x0FF3FF, 0x3FE3FF, 0x3EE3FF, 0x2403FF, 0x37F3FF, 0x3FF3FF, 0x0013FE, 0x2613FF, 0x3FF3FF,
         0x2B93FF, 0x3FF3FF, 0x1843FF, 0x3FF3FF, 0x3FF3FF, 0x3FF3FF, 0x2FF3FF, 0x2001FF, 0x2BB3FF]
ROAM = [2, 4, 5, 8, 11, 15, 18, 20, 21, 25, 26, 34, 37, 39, 43, 45]


def text(s, length):
    out = [TEXT[c] for c in s if c in TEXT][:length - 1]
    out.append(TERMINATOR)
    return bytes(out + [TERMINATOR] * (length - len(out)))


def tree_ok(location, stype, tid):
    if location not in TREE_LOCATIONS:
        return False
    perm = TREES[TREE_LOCATIONS.index(location)]
    bit = tid % 10 + (0 if stype == 6 else 12)
    return (perm >> bit) & 1 == 1


class Data:
    def __init__(self, pkhex, pokered):
        core = pkhex / "PKHeX.Core"
        res = core / "Resources"
        self.g1 = g1.Data(pkhex, pokered)
        self.personal = gen.read_personal2(res / "byte/personal/personal_c")
        self.learn = {False: gen.read_learnsets(res / "byte/levelup/lvlmove_gs.pkl", b"gs"),
                      True: gen.read_learnsets(res / "byte/levelup/lvlmove_c.pkl", b"c\0")}
        self.eggs = {False: gen.read_move_lists(res / "byte/eggmove/eggmove_gs.pkl", b"gs"),
                     True: gen.read_move_lists(res / "byte/eggmove/eggmove_c.pkl", b"c\0")}
        for table in self.eggs.values():
            table.extend([] for _ in range(252 - len(table)))
        self.evos = gen.read_evolutions(res / "byte/evolve/evos_g2.pkl", b"g2")
        self.pre = {}
        for frm, entries in enumerate(self.evos):
            for _t, _a, to, _f, _lv in entries:
                self.pre.setdefault(to, frm)
        self.wild = []
        for fname, v in (("encounter_gold.pkl", GD), ("encounter_silver.pkl", SI),
                         ("encounter_crystal.pkl", C)):
            for s in gen.read_areas(res / "legality/wild/Gen2" / fname, b"g2", 2):
                self.wild.append(s + (v,))  # species, min, max, type, location, time, version
        self.wild = list(dict.fromkeys(self.wild))  # same slot in several areas: keep one
        enc2 = (core / "Legality/Encounters/Data/Gen2/Encounters2.cs").read_text(encoding="utf-8")
        self.statics = []
        for name in ("StaticGSC", "StaticGS", "StaticGD", "StaticSI", "StaticC", "StaticOddEggC"):
            self.statics.extend(gen.parse_static2(enc2, name))
        self.trades = gen.parse_trade2(enc2)
        self.trade_names = gen.read_text_lines(res / "legality/gen2/text_tradegsc_en.txt")
        self.trade_names_ja = gen.read_text_lines(res / "legality/gen2/text_tradegsc_ja.txt")
        self.jp = gbtext.Encoder(gbtext.read_table(pkhex, 2, "TableJP"))
        # PK2 keeps no language: PKHeX decodes a lone international record with TableEN,
        # which spells every accented letter of the European trade names (HÜHOTTI,
        # FALCÁN, MANÍA) at the same byte as the French/German and Italian/Spanish tables.
        self.en = gbtext.Encoder(gbtext.read_table(pkhex, 2, "TableEN"))
        self.trade_names_eu = {lang: gen.read_text_lines(res / f"legality/gen2/text_tradegsc_{lang}.txt")
                               for lang in ("de", "fr", "it", "es")}
        self.jp_names = gbtext.japanese_names(self.jp)
        # Korean Gold/Silver: two-byte Hangul from PKHeX's StringConverter2KOR tables.
        self.kor = gbtext.KoreanEncoder(gbtext.read_korean_tables(pkhex))
        self.kor_names = gbtext.korean_names(self.kor)
        self.trade_names_ko = gen.read_text_lines(res / "legality/gen2/text_tradegsc_ko.txt")
        raw = res.joinpath("legality/mgdb/event2.pkl").read_bytes()
        self.gifts = [raw[i:i + 12] for i in range(0, len(raw), 12)]
        header = (HERE.parent.parent / "include/Legality/GBLegalityData.h").read_text()
        body = re.search(r"kMachineMoves2\[57\] = \{(.*?)\};", header, re.S).group(1)
        self.machines = [int(x) for x in re.findall(r"\d+", body)]
        self.tutors = [53, 85, 58]
        self.pp = gen.read_pp(core, "MoveInfo2.cs")
        self.held = sorted(set(gen.cs_numbers(self._items(core), "General") +
                               gen.cs_numbers(self._items(core), "Balls") +
                               gen.cs_numbers(self._items(core), "Machine")[:50]))

    @staticmethod
    def _items(core):
        path = core / "Saves/Storage/Items/ItemStorage2.cs"
        if not path.exists():
            path = next(core.rglob("ItemStorage2.cs"))
        return path.read_text(encoding="utf-8")

    def tms(self, species, crystal):
        bits = self.personal[species]["tm"]
        out = [self.machines[i] for i in range(57) if bits[i >> 3] & (1 << (i & 7))]
        if crystal:
            out += [self.tutors[i] for i in range(3) if bits[(57 + i) >> 3] & (1 << ((57 + i) & 7))]
        return out

    def base(self, species):
        while species in self.pre:
            species = self.pre[species]
        return species


class Mon:
    def __init__(self, species, level, moves, crystal):
        self.species, self.level, self.moves = species, level, list(moves)
        self.crystal = crystal
        self.item = 0
        self.dvs = None
        self.tid = None
        self.ot, self.nick = "GOLD", "MON"
        self.caught = 0
        self.egg = False
        self.friendship = 70
        self.exp = None
        self.pp = None  # list of (pp, ups); filled in by finish() unless a defect sets it
        self.statexp = [0] * 5
        self.pokerus = 0
        self.japanese = False
        self.korean = False
        self.fill_name = False  # the name fills its field with no terminator
        self.egg_name = None  # an unhatched egg's name when it is not the game's own
        self.ot_raw = None  # the stored name bytes, when a kind sets them directly
        self.nick_raw = None
        self.max_hp_delta = 0  # added to the stored party max HP (a defect)


def encounter_moves(d, species, level, crystal):
    moves = []
    for lv, mv in d.learn[crystal][species]:
        if lv > level:
            break
        if mv in moves:
            continue
        if len(moves) == 4:
            moves.pop(0)
        moves.append(mv)
    return moves


def caught(time, level, otg, location):
    return (time << 14) | ((level & 0x3F) << 8) | (otg << 7) | (location & 0x7F)


def grow(d, mon, target, rng):
    while mon.level < target:
        mon.level += 1
        for lv, mv in d.learn[mon.crystal][mon.species]:
            if lv == mon.level:
                g1.learn(mon, mv, rng)
        for etype, _arg, to, _f, lv in d.evos[mon.species]:
            if etype in (4, 9, 10, 11) and mon.level >= lv and rng.random() < 0.85:
                mon.species = to
                break
            if etype in (1, 2, 3, 5, 6, 8) and rng.random() < 0.04:
                mon.species = to
                break
    if rng.random() < 0.4:
        options = d.tms(mon.species, mon.crystal)
        for _ in range(rng.randrange(1, 3)):
            if options:
                g1.learn(mon, rng.choice(options), rng)
    if rng.random() < 0.3:
        mon.item = rng.choice(d.held)
    if rng.random() < 0.6:
        mon.statexp = g1.stat_exp(rng)
    if rng.random() < 0.15:
        mon.pokerus = pokerus(rng)


def pokerus(rng):
    """A Pokerus byte the games can produce: strain 0-8, then at most (strain & 3) + 1 days."""
    strain = rng.randint(0, 8)
    return (strain << 4) | rng.randint(0, (strain & 3) + 1)


def finish(d, mon, rng):
    p = d.personal[mon.species]
    if mon.dvs is None:
        mon.dvs = [rng.randrange(16) for _ in range(4)]
    if mon.tid is None:
        mon.tid = rng.randrange(65536)
    atk, df, spe, spc = mon.dvs
    hp_dv = ((atk & 1) << 3) | ((df & 1) << 2) | ((spe & 1) << 1) | (spc & 1)
    body = bytearray(48)
    body[0] = mon.species
    body[1] = mon.item
    for i, m in enumerate((mon.moves + [0, 0, 0, 0])[:4]):
        body[2 + i] = m
    body[6:8] = mon.tid.to_bytes(2, "big")
    exp = mon.exp if mon.exp is not None else g1.exp_for(p["growth"], mon.level)
    body[8:11] = exp.to_bytes(3, "big")
    for i, value in enumerate(mon.statexp):  # HP ATK DEF SPE SPC
        body[11 + i * 2:13 + i * 2] = value.to_bytes(2, "big")
    body[21] = (atk << 4) | df
    body[22] = (spe << 4) | spc
    moves = (mon.moves + [0, 0, 0, 0])[:4]
    if mon.pp is None:
        mon.pp = []
        for m in moves:
            if mon.egg:
                mon.pp.append((d.pp[m], 0))
                continue
            ups = 0 if m in (0, 166) or rng.random() < 0.5 else rng.randrange(4)
            top = d.pp[m] + ups * min(7, d.pp[m] // 5)
            mon.pp.append((rng.choice((top, rng.randint(0, top))), ups))
    for i, (pp, ups) in enumerate(mon.pp):
        body[23 + i] = (ups << 6) | pp
    body[27] = mon.friendship
    body[28] = mon.pokerus
    body[29:31] = mon.caught.to_bytes(2, "big")
    body[31] = mon.level
    stats = p["stats"]  # HP ATK DEF SPE SPA SPD
    hp = g1.stat(stats[0], hp_dv, mon.level, True)
    body[34:36] = hp.to_bytes(2, "big")
    body[36:38] = (hp + mon.max_hp_delta).to_bytes(2, "big")
    for i, (b, dv) in enumerate(((stats[1], atk), (stats[2], df), (stats[3], spe), (stats[4], spc), (stats[5], spc))):
        body[38 + i * 2:40 + i * 2] = g1.stat(b, dv, mon.level, False).to_bytes(2, "big")
    if mon.korean:
        def kor(s):
            return d.kor.encode(s, 11, terminate=not mon.fill_name)
        ot, nick = kor(mon.ot), kor((mon.egg_name or "알") if mon.egg else mon.nick)
    elif mon.japanese:
        def jp(s):
            return d.jp.encode(s, 6, terminate=not mon.fill_name or len(s) < 6)
        ot, nick = jp(mon.ot), jp((mon.egg_name or "タマゴ") if mon.egg else mon.nick)
    else:
        def intl(s):
            return text(s, 11) if all(c in TEXT for c in s) else d.en.encode(s, 11)
        ot, nick = intl(mon.ot), intl((mon.egg_name or "EGG") if mon.egg else mon.nick)
    if mon.ot_raw is not None:
        ot = mon.ot_raw
    if mon.nick_raw is not None:
        nick = mon.nick_raw
    return body, ot, nick


def japanize(d, mon, rng):
    """Make this a Japanese-game Pokemon with a Japanese trainer and nickname."""
    mon.japanese = True
    if mon.ot == "ODD":
        mon.ot = rng.choice(("なぞナゾ", "なぞ"))
    else:
        mon.ot = rng.choice(d.jp_names)
    mon.nick = rng.choice(("ポケモン", "ピカ", "あいぼう", "ゴン"))


def met_time(slot_time, rng):
    allowed = [t for t in (1, 2, 3) if slot_time == 0 or (slot_time >> t) & 1]
    return rng.choice(allowed)


def wild_mon(d, rng, slot):
    species, lmin, lmax, stype, location, time, v = slot
    crystal = v == C
    level = rng.randint(lmin, lmax)
    mon = Mon(species, level, encounter_moves(d, species, level, crystal), crystal)
    if stype in HEADBUTT:
        tid = rng.randrange(65536)
        for _ in range(50):
            if tree_ok(location, stype, tid):
                break
            tid = rng.randrange(65536)
        mon.tid = tid
    if crystal:
        otg = rng.randrange(2)
        mon.caught = caught(met_time(time, rng), level, otg, location)
    return mon


def egg_mon(d, rng, hatched, crystal=None):
    while True:
        species = rng.randrange(1, 252)
        if species not in NO_HATCH and d.base(species) == species:
            break
    if crystal is None:
        crystal = rng.random() < 0.5
    mon = Mon(species, 5, encounter_moves(d, species, 5, crystal), crystal)
    pool = [m for m in d.eggs[crystal][species] if m and m not in mon.moves]
    for _ in range(rng.randrange(0, 3)):
        if pool:
            g1.learn(mon, rng.choice(pool), rng)
    if hatched:
        mon.friendship = 120
        if crystal:
            mon.caught = caught(rng.randint(1, 3), 1, rng.randrange(2), rng.randint(1, 95))
    else:
        mon.egg = True
        mon.friendship = rng.randint(1, max(1, d.personal[species]["hatch"]))
    return mon


def static_mon(d, rng, s):
    crystal = bool(s["version"] & C) and (s["version"] == C or rng.random() < 0.5)
    level = s["level"]
    moves = list(s["moves"]) if s["moves"] else encounter_moves(d, s["species"], level, crystal)
    mon = Mon(s["species"], level, moves, crystal)
    if s["dvs"]:
        hp, atk, df, spe, spa, _spd = s["dvs"]
        mon.dvs = [atk, df, spe, spa]
    elif s["shinyAlways"]:
        mon.dvs = [rng.choice((2, 3, 6, 7, 10, 11, 14, 15)), 10, 10, 10]
    if s["egg"]:
        odd = bool(s["moves"])
        if rng.random() < 0.5:
            mon.egg = True
            mon.friendship = rng.randint(1, 20 if odd else d.personal[s["species"]]["hatch"])
            if odd:
                mon.exp = 125
                mon.ot = "ODD"
        else:
            mon.friendship = 120
            if crystal:
                mon.caught = caught(rng.randint(1, 3), 1, rng.randrange(2), rng.randint(1, 95))
        return mon
    if crystal:
        roamer = s["species"] in (243, 244, 245) and s["location"] != 23
        location = rng.choice(ROAM) if roamer else s["location"]
        mon.caught = caught(rng.randint(1, 3), level, rng.randrange(2), location)
    return mon


def trade_mon(d, rng, t, variant, japanese=False, lang=None):
    crystal = rng.random() < 0.5
    mon = Mon(t["species"], t["level"], encounter_moves(d, t["species"], t["level"], crystal), crystal)
    if t["dvs"]:
        hp, atk, df, spe, spa, _spd = t["dvs"]
        mon.dvs = [atk, df, spe, spa]
        mon.tid = t["tid"]
    names = d.trade_names_ja if japanese else d.trade_names_eu[lang] if lang else d.trade_names
    wrong = "ニセモノ" if japanese else None
    mon.japanese = japanese
    mon.nick = names[t["index"]] if variant != "trade2-wrong-nick" else (wrong or "NOTRADE")
    mon.ot = names[t["index"] + 11] if variant != "trade2-wrong-ot" else (wrong or "WRONG")
    if crystal:
        mon.caught = caught(0, 0, t["otGender"] if t["dvs"] else 0, 126)
    return mon


def gift_mon(d, rng, g):
    species, level, m1, m2, m3, m4, location, current, shiny, egg, language, trainer = g
    mon = Mon(species, max(level, current), [m for m in (m1, m2, m3, m4) if m], location != 0)
    if shiny:
        mon.dvs = [rng.choice((2, 3, 6, 7, 10, 11, 14, 15)), 10, 10, 10]
    if language == 1:
        japanize(d, mon, rng)
    if trainer == 1:
        mon.ot, mon.tid = "スタジアム", 2000
    elif trainer == 2:
        mon.ot, mon.tid = "Stadium", 2000
    elif trainer == 3:
        mon.ot, mon.tid = rng.choice(("Stadion", "Stade", "Stadio", "Estadio")), 2001
    elif trainer == 4:
        mon.ot, mon.tid = rng.choice(("PCNYa", "PCNYb", "PCNYc", "PCNYd")), rng.randrange(10, 200)
    if egg and rng.random() < 0.5:
        mon.egg = True
        mon.level = 5
        mon.friendship = rng.randint(1, 10)
        return mon
    if location != 0:
        mon.caught = caught(0, 1 if egg else level, 0, 0 if egg else location)
        if egg:
            mon.caught = caught(rng.randint(1, 3), 1, 0, rng.randint(1, 95))
    return mon


# Japanese Crystal's GS Ball Celebi: Ilex Forest shrine (location 14), wild level 30.
def gsball_mon(d, rng, variant):
    mon = Mon(251, 30, encounter_moves(d, 251, 30, True), True)
    level, location = 30, 14
    if variant == "wrong-level":
        level = rng.choice((29, 31))
    elif variant == "wrong-place":
        location = rng.choice((13, 15, 33))
    mon.caught = caught(rng.randint(1, 3), level, rng.randrange(2), location)
    if variant == "grown":
        grow(d, mon, 30 + rng.randrange(1, 40), rng)
    if variant != "international":
        japanize(d, mon, rng)
    return mon


def time_capsule_mon(d, rng, japanese=False):
    """A Gen I Pokemon (moves learned in Red/Blue/Yellow) moved to Gen II."""
    species, lmin, lmax, v = rng.choice(d.g1.wild)
    while v == g1.BU and not japanese:
        species, lmin, lmax, v = rng.choice(d.g1.wild)
    yellow = v == g1.YW
    level = rng.randint(lmin, lmax)
    m1 = g1.Mon(species, level, g1.encounter_moves(d.g1, species, level, yellow),
                d.g1.personal[yellow][species]["catch"], yellow)
    g1.grow(d.g1, m1, min(100, level + rng.randrange(0, 40)), rng)
    mon = Mon(m1.species, m1.level, m1.moves, rng.random() < 0.5)
    if rng.random() < 0.5:
        grow(d, mon, min(100, mon.level + rng.randrange(0, 20)), rng)
    return mon


# Gen I statics with no Gen II source at all: the birds and Mewtwo (Gen II breeds starters
# and fossils from Time Capsule parents, so an egg origin stays open for them).
GEN1_ONLY_STATIC = (144, 145, 146, 150)

KOREAN_NICKS = ("피카", "친구", "꼬마", "별님", "용용이", "BUDDY", "Mon")


def koreanize(d, mon, rng, keep_met=False):
    """Make this a Korean Gold/Silver Pokemon: Hangul trainer and nickname. There is no
    Korean Crystal, so a Korean player's Pokemon carries no met data unless a defect adds it."""
    mon.korean = True
    mon.crystal = False
    mon.ot = rng.choice(d.kor_names)
    mon.nick = rng.choice(KOREAN_NICKS)
    if not keep_met:
        mon.caught = 0


def gen1_only_moves(d, species):
    """Moves a Gen I species learns in Red/Blue/Yellow (TM or level-up) that no form of it
    learns in Gold/Silver/Crystal by TM, tutor, level-up or egg."""
    family, sp = set(), species
    while True:
        family.add(sp)
        if sp not in d.pre:
            break
        sp = d.pre[sp]
    gen2 = set()
    for sp in family:
        gen2.update(d.tms(sp, True))
        for crystal in (False, True):
            gen2.update(mv for _lv, mv in d.learn[crystal][sp])
            gen2.update(d.eggs[crystal][sp])
    gen1 = set()
    for sp in family:
        if sp > 151:
            continue
        for yellow in (False, True):
            gen1.update(d.g1.tms(sp, yellow))
    return sorted(m for m in gen1 - gen2 if m)


# Korean Gold/Silver records, each aimed at one rule: what Korean Gold/Silver alone make
# (PKHeX and PokeBank NX agree), what needs a Korean-Western cartridge trade (PKHeX
# INVALID, PokeBank NX UNCERTAIN), and what no route makes (both INVALID).
KOREAN_VARIANTS = ("wild", "egg", "egg-unhatched", "static", "trade", "trade-wrong-nick", "crystal-wild",
                   "met-data", "crystal-move", "crystal-egg", "odd-egg", "gen1-move", "stadium", "pcny-egg",
                   "event", "nick-latin", "nick-long", "nick-latin-long", "ot-long", "ot-latin", "time-capsule")


def korean_mon(d, rng, variant):
    gs_wild = [w for w in d.wild if w[6] != C]
    if variant in ("trade", "trade-wrong-nick"):
        t = rng.choice([t for t in d.trades if d.trade_names_ko[t["index"]]])
        mon = Mon(t["species"], t["level"], encounter_moves(d, t["species"], t["level"], False), False)
        if t["dvs"]:
            _hp, atk, df, spe, spa, _spd = t["dvs"]
            mon.dvs = [atk, df, spe, spa]
            mon.tid = t["tid"]
        mon.korean = True
        mon.nick = d.trade_names_ko[t["index"]] if variant == "trade" else "가짜"
        mon.ot = d.trade_names_ko[t["index"] + 11]
        return mon
    if variant in ("egg", "egg-unhatched"):
        mon = egg_mon(d, rng, variant == "egg", crystal=False)
        koreanize(d, mon, rng)
        if variant == "egg" and rng.random() < 0.6:
            grow(d, mon, min(100, 5 + rng.randrange(0, 50)), rng)
        return mon
    if variant == "crystal-egg":  # bred in Crystal (Crystal-only egg move), hatched in Korean Gold/Silver
        while True:
            mon = egg_mon(d, rng, True, crystal=True)
            pool = [m for m in d.eggs[True][mon.species] if m and m not in d.eggs[False][mon.species]]
            if pool:
                break
        g1.learn(mon, rng.choice(pool), rng)
        koreanize(d, mon, rng)
        return mon
    if variant == "odd-egg":  # Crystal's Odd Egg, hatched by a Korean player
        s = rng.choice([s for s in d.statics if s["egg"] and s["moves"]])
        mon = Mon(s["species"], 5, list(s["moves"]), False)
        mon.friendship = 120
        if s["dvs"]:  # the shiny Odd Eggs carry fixed DVs
            _hp, atk, df, spe, spa, _spd = s["dvs"]
            mon.dvs = [atk, df, spe, spa]
        koreanize(d, mon, rng)
        if rng.random() < 0.5:
            grow(d, mon, min(100, 5 + rng.randrange(1, 40)), rng)
        return mon
    if variant in ("pcny-egg", "event"):
        egg = variant == "pcny-egg"
        g = rng.choice([g for g in d.gifts if g[9] == egg and g[10] == 0])
        species, level, m1, m2, m3, m4 = g[:6]
        mon = Mon(species, max(level, g[7]), [m for m in (m1, m2, m3, m4) if m], False)
        if g[8]:
            mon.dvs = [rng.choice((2, 3, 6, 7, 10, 11, 14, 15)), 10, 10, 10]
        if egg:
            mon.friendship = 120
        koreanize(d, mon, rng)
        return mon
    if variant == "static" or variant == "stadium":
        legend = variant == "stadium"
        pool = [s for s in d.statics if not s["egg"] and s["version"] != C and
                (not legend or s["species"] in NO_HATCH)]
        s = rng.choice(pool)
        mon = static_mon(d, rng, s)
        if not s["moves"]:
            mon.moves = encounter_moves(d, s["species"], s["level"], False)
        koreanize(d, mon, rng)
        if legend:  # a level-up move pushed out before the encounter: only a reminder restores it
            forgotten = [mv for lv, mv in d.learn[False][s["species"]] if lv <= s["level"] and mv not in mon.moves]
            if forgotten:
                mon.moves[rng.randrange(len(mon.moves))] = rng.choice(forgotten)
                mon.moves = list(dict.fromkeys(mon.moves))
        elif rng.random() < 0.5:
            grow(d, mon, min(100, mon.level + rng.randrange(0, 30)), rng)
        return mon
    if variant == "crystal-wild":
        mon = wild_mon(d, rng, rng.choice([w for w in d.wild if w[6] == C]))
        koreanize(d, mon, rng)
        return mon
    if variant == "time-capsule":
        mon = time_capsule_mon(d, rng)
        koreanize(d, mon, rng)
        return mon
    if variant == "gen1-move":
        while True:
            slot = rng.choice(gs_wild)
            if slot[0] <= 151 and gen1_only_moves(d, slot[0]):
                break
        mon = wild_mon(d, rng, slot)
        g1.learn(mon, rng.choice(gen1_only_moves(d, slot[0])), rng)
        koreanize(d, mon, rng)
        return mon
    mon = wild_mon(d, rng, rng.choice(gs_wild))
    grow(d, mon, min(100, mon.level + rng.randrange(0, 40)), rng)
    koreanize(d, mon, rng)
    if variant == "met-data":
        mon.caught = caught(rng.randint(1, 3), max(2, mon.level - 5), 0, rng.randint(1, 95))
    elif variant == "crystal-move":
        tutor = [m for m in d.tms(mon.species, True) if m not in d.tms(mon.species, False)]
        levelup = [mv for lv, mv in d.learn[True][mon.species]
                   if lv <= mon.level and mv not in {m for _l, m in d.learn[False][mon.species]}]
        if tutor or levelup:
            g1.learn(mon, rng.choice(tutor + levelup), rng)
    elif variant == "nick-latin":
        mon.nick = g1.letters(rng, rng.randint(1, 10))
    elif variant == "nick-latin-long":
        mon.nick = g1.letters(rng, 11)
        mon.fill_name = True
    elif variant == "nick-long":  # five Hangul and a letter: six characters in the 11-byte field
        mon.nick = "".join(rng.choice("피카츄꼬마별") for _ in range(5)) + "A"
        mon.fill_name = True
    elif variant == "ot-long":
        mon.ot = "".join(rng.choice("한별민수지훈") for _ in range(5)) + "A"
        mon.fill_name = True
    elif variant == "ot-latin":  # Hangul first (what marks the record Korean), then the one-byte page
        mon.ot = rng.choice(d.kor_names)[:2] + rng.choice(("A", "Z9", " B", "x", "♂", "1"))
        if rng.random() < 0.3:  # punctuation the Hangul pages do have
            mon.ot = rng.choice(d.kor_names)[:2] + rng.choice(("!", "-", "?"))
    return mon


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--pkhex", required=True, type=Path)
    ap.add_argument("--pokered", required=True, type=Path)
    ap.add_argument("--count", type=int, default=20000)
    ap.add_argument("--seed", type=int, default=1)
    args = ap.parse_args()
    d = Data(args.pkhex, args.pokered)
    rng = random.Random(args.seed)
    out = sys.stdout
    made = 0

    def emit(kind, mon):
        nonlocal made
        body, ot, nick = finish(d, mon, rng)
        # PKHeX LanguageID as the bytes show it (PK2.Korean: a Hangul page first byte); 0 = other.
        lang = 1 if mon.japanese else 8 if mon.korean and ot[0] <= 11 else 0
        out.write(f"{made} {kind} {lang} {body.hex()} {ot.hex()} {nick.hex()} {int(mon.egg)}\n")
        made += 1

    for slot in d.wild:
        emit("wild2", wild_mon(d, rng, slot))
    for s in d.statics:
        emit("static2", static_mon(d, rng, s))
    for t in d.trades:
        for japanese in (False, True):
            for variant in ("trade2", "trade2-wrong-ot", "trade2-wrong-nick"):
                emit(variant + ("-jp" if japanese else ""), trade_mon(d, rng, t, variant, japanese))
    for g in d.gifts:
        emit("gift2-jp" if g[10] == 1 else "gift2", gift_mon(d, rng, g))
    # European in-game trades, right and renamed (PKHeX accepts any international language).
    for lang in ("de", "fr", "it", "es"):
        for t in d.trades:
            for variant in ("trade2", "trade2-wrong-nick"):
                emit(f"{variant}-{lang}", trade_mon(d, rng, t, variant, False, lang))
    # Japanese Crystal's GS Ball Celebi, with the met data that make it one and without.
    for variant, count in (("caught", 6), ("grown", 6), ("international", 3), ("wrong-level", 3),
                           ("wrong-place", 3)):
        for _ in range(count):
            emit("gsball2-" + variant, gsball_mon(d, rng, variant))

    # Korean Gold/Silver, a fixed number of each targeted kind.
    for variant in KOREAN_VARIANTS:
        for _ in range(40):
            emit("kor2-" + variant, korean_mon(d, rng, variant))

    # Structural defects and their legal boundaries, 30 records each, on grown Gold/Silver/
    # Crystal wild Pokemon (met data dropped when a defect moves the level off the met level).
    for variant in g1.STRUCTURAL:
        for _ in range(30):
            mon = wild_mon(d, rng, rng.choice(d.wild))
            grow(d, mon, min(100, mon.level + rng.randrange(0, 40)), rng)
            g1.structural_defect(mon, rng, variant, d.personal[mon.species]["growth"])
            emit("struct2-" + variant, mon)

    # Name bytes and characters on grown wild Pokemon, as in the PK1 corpus: digits, a byte
    # with no glyph (StringConverter2 TableEN / TableJP NUL entries), a Japanese trainer name
    # outside the kana -- ？ and ！ pass in Gen II (StringConverter2.GetIsJapanese), so
    # jp-ot-mark-ok is the legal boundary -- and the Gen I trade marker followed by a letter.
    def grown_wild():
        mon = wild_mon(d, rng, rng.choice([w for w in d.wild if w[6] != C]))
        grow(d, mon, min(100, mon.level + rng.randrange(0, 40)), rng)
        return mon

    for _ in range(30):
        mon = grown_wild()
        mon.ot_raw = g1.with_byte(rng, g1.letters(rng, rng.randint(1, 5)), 0xF6 + rng.randrange(10))
        emit("defect2-ot-digit", mon)
        mon = grown_wild()
        mon.nick_raw = g1.with_byte(rng, g1.letters(rng, rng.randint(1, 8)), 0xF6 + rng.randrange(10))
        emit("defect2-nick-digit", mon)
        mon = grown_wild()
        if rng.random() < 0.5:
            mon.ot_raw = g1.with_byte(rng, g1.letters(rng, rng.randint(1, 5)), rng.choice((0x05, 0x4F, 0x60, 0x74, 0xD8, 0xDE)))
        else:
            mon.nick_raw = g1.with_byte(rng, g1.letters(rng, rng.randint(1, 8)), rng.choice((0x05, 0x4F, 0x60, 0x74, 0xD8, 0xDE)))
        emit("defect2-name-glyphless", mon)
        mon = grown_wild()
        japanize(d, mon, rng)
        if rng.random() < 0.5:
            mon.ot_raw = g1.with_byte_jp(rng, d.jp, mon.ot[:3], rng.choice(g1.GLYPHLESS_JP))
        else:
            mon.nick_raw = g1.with_byte_jp(rng, d.jp, mon.nick[:3], rng.choice(g1.GLYPHLESS_JP))
        emit("defect2-name-glyphless-jp", mon)
        mon = grown_wild()
        japanize(d, mon, rng)
        mon.ot = rng.choice(("サ１", "ア×", "ミ０", "カ。", "ゴ♂"))
        emit("defect2-jp-ot-charset", mon)
        mon = grown_wild()
        japanize(d, mon, rng)
        mon.ot = rng.choice(("サ？", "ゴ！", "ミナ？"))
        emit("jp-ot-mark-ok", mon)
        # The same marks on a Pokemon only Gen I produces (a fossil, starter, bird or Mewtwo
        # moved through the Time Capsule): PKHeX's Gen I kana set applies to its one origin.
        species, level, v = rng.choice([s for s in d.g1.statics if s[0] in GEN1_ONLY_STATIC and s[2] != g1.BU])
        yellow = v == g1.YW
        m1 = g1.Mon(species, level, g1.encounter_moves(d.g1, species, level, yellow),
                    d.g1.personal[yellow][species]["catch"], yellow)
        g1.grow(d.g1, m1, min(100, level + rng.randrange(0, 30)), rng)
        mon = Mon(m1.species, m1.level, m1.moves, False)
        japanize(d, mon, rng)
        mon.ot = rng.choice(("サ？", "ゴ！", "ミナ？"))
        emit("jp-ot-mark-gen1", mon)
        mon = grown_wild()
        mon.ot_raw = bytes([g1.TRADE_OT, 0x80 + rng.randrange(26)] + [g1.TERMINATOR] * 9)
        emit("defect2-trade-ot-bad-term", mon)

    def emit_maybe_jp(kind, mon, japanese, korean=False):
        if japanese:
            japanize(d, mon, rng)
        elif korean:
            koreanize(d, mon, rng, keep_met=kind == "random2")
        emit(kind + ("-jp" if japanese else "-kor" if korean else ""), mon)

    while made < args.count:
        roll = rng.random()
        japanese = rng.random() < 0.25
        # Korean Gold/Silver: about one record in twelve, on the same kinds.
        korean = not japanese and rng.random() < 0.11
        if roll < 0.30:
            mon = wild_mon(d, rng, rng.choice(d.wild))
            grow(d, mon, min(100, mon.level + rng.randrange(0, 60)), rng)
            emit_maybe_jp("grown2", mon, japanese, korean)
        elif roll < 0.40:
            hatched = rng.random() < 0.7
            mon = egg_mon(d, rng, hatched, crystal=False if korean else None)
            if hatched and rng.random() < 0.7:
                grow(d, mon, min(100, 5 + rng.randrange(0, 50)), rng)
            emit_maybe_jp("egg2" if not mon.egg else "egg2-unhatched", mon, japanese, korean)
        elif roll < 0.48:
            mon = static_mon(d, rng, rng.choice(d.statics))
            if not mon.egg:
                grow(d, mon, min(100, mon.level + rng.randrange(0, 40)), rng)
            emit_maybe_jp("grown-static2", mon, japanese, korean and not mon.egg)
        elif roll < 0.58:
            emit_maybe_jp("time-capsule", time_capsule_mon(d, rng, japanese), japanese, korean)
        elif roll < 0.88:
            mon = wild_mon(d, rng, rng.choice(d.wild))
            grow(d, mon, min(100, mon.level + rng.randrange(0, 40)), rng)
            defect = rng.choice(("move", "level", "item", "caught", "wipe", "time", "otgender",
                                 "egg-cycles", "egg-item", "pp", "egg-pp", "egg-moves", "egg-name",
                                 "statexp", "egg-statexp", "pokerus", "ot-long", "ot-empty", "nick-empty",
                                 "exp-100", "structural")
                                + (("nick-long",) if japanese else ()))
            if japanese:
                japanize(d, mon, rng)
            elif korean:
                koreanize(d, mon, rng)
            if defect == "move":
                mon.moves[rng.randrange(len(mon.moves))] = rng.randint(1, 251)
                mon.moves = list(dict.fromkeys(mon.moves))
            elif defect == "level":
                mon.level = rng.randint(2, 100)
            elif defect == "item":
                mon.item = rng.randrange(256)
            elif defect == "caught":
                mon.caught = rng.randrange(65536)
            elif defect == "wipe":
                mon.caught = 0
            elif defect == "time":
                mon.caught = (mon.caught & 0x3FFF) | (rng.randrange(4) << 14)
            elif defect == "otgender":
                mon.caught ^= 0x80
            elif defect == "statexp":
                mon.statexp = g1.stat_exp_defect(rng)
            elif defect == "structural":
                defect = rng.choice(g1.STRUCTURAL)
                g1.structural_defect(mon, rng, defect, d.personal[mon.species]["growth"])
            elif defect == "exp-100":  # level 100 with EXP off the cap, or exactly on it
                grow(d, mon, 100, rng)
                top = g1.exp_for(d.personal[mon.species]["growth"], 100)
                mon.exp = top + rng.choice((0, 0, 1, rng.randint(2, 50000), -1, -rng.randint(2, 20000)))
            elif defect == "pokerus":
                mon.pokerus = rng.randrange(256)
            elif defect == "ot-long":  # seven letters (five in Japanese and Korean) is the limit
                if korean:  # five Hangul and a letter fill the 11-byte field
                    mon.ot = "".join(rng.choice("한별민수지훈") for _ in range(5)) + "A"
                    mon.fill_name = True
                elif japanese:
                    mon.ot = "".join(rng.choice("アイウエオカキクケコ") for _ in range(rng.randint(5, 6)))
                    mon.fill_name = True
                else:
                    mon.ot = g1.letters(rng, rng.randint(7, 10))
            elif defect == "nick-long":  # a Japanese nickname filling all six bytes
                mon.nick = "".join(rng.choice("アイウエオカキクケコ") for _ in range(rng.randint(5, 6)))
                mon.fill_name = True
            elif defect == "ot-empty":
                mon.ot = ""
            elif defect == "nick-empty":
                mon.nick = ""
            elif defect == "pp":
                moves = (mon.moves + [0, 0, 0, 0])[:4]
                mon.pp = [(rng.randrange(64), rng.randrange(4)) for _ in moves]
            elif defect in ("egg-cycles", "egg-item", "egg-pp", "egg-moves", "egg-statexp", "egg-name"):
                mon = egg_mon(d, rng, False, crystal=False if korean else None)
                if japanese:
                    japanize(d, mon, rng)
                elif korean:
                    koreanize(d, mon, rng)
                if defect == "egg-name":  # another language's egg name, or a name no game gives an egg
                    if japanese:
                        mon.egg_name = rng.choice(("ポケモン", "タマ"))
                    elif korean:
                        mon.egg_name = rng.choice(("알알", "친구"))
                    else:
                        mon.egg_name = rng.choice(("EI", "OEUF", "UOVO", "HUEVO", "EGGS", "MON", g1.letters(rng, 5)))
                elif defect == "egg-statexp":  # vitamins cannot be used on an egg either
                    mon.statexp = rng.choice((g1.stat_exp(rng), [2560 * rng.randint(1, 10)] + [0] * 4))
                elif defect == "egg-cycles":
                    mon.friendship = rng.choice((0, d.personal[mon.species]["hatch"] + 1, 255))
                elif defect == "egg-item":
                    mon.item = rng.choice(d.held)
                elif defect == "egg-pp":
                    mon.pp = [(d.pp[m] if rng.random() < 0.5 else rng.randrange(64), rng.randrange(2))
                              for m in (mon.moves + [0, 0, 0, 0])[:4]]
                else:  # shuffle the order, or add a level-up move the egg cannot know yet
                    if rng.random() < 0.5:
                        rng.shuffle(mon.moves)
                    else:
                        later = [mv for lv, mv in d.learn[mon.crystal][mon.species] if lv > 5]
                        if later:
                            g1.learn(mon, rng.choice(later), rng)
            emit("defect2-" + defect + ("-jp" if japanese else "-kor" if korean else ""), mon)
        else:
            species = rng.randint(1, 251)
            moves = list(dict.fromkeys(rng.randint(1, 251) for _ in range(rng.randint(1, 4))))
            mon = Mon(species, rng.randint(2, 100), moves, rng.random() < 0.5)
            mon.item = rng.choice((0, rng.choice(d.held), rng.randrange(256)))
            if rng.random() < 0.5:
                mon.caught = rng.randrange(65536)
            mon.statexp = rng.choice((g1.stat_exp(rng), [rng.randrange(65536) for _ in range(5)]))
            mon.pokerus = rng.choice((0, pokerus(rng), rng.randrange(256)))
            emit_maybe_jp("random2", mon, japanese, korean)

    # A party max HP one off the only value a fresh catch can hold (no stat experience yet):
    # pokecrystal CalcMonStatC gives one value, and PKHeX does not compare stored party stats.
    # Appended after the seeded stream above, so its records keep their ids.
    for _ in range(max(1, args.count // 200)):
        mon = wild_mon(d, rng, rng.choice(d.wild))
        mon.statexp = [0] * 5
        mon.max_hp_delta = rng.choice((-1, 1))
        emit("defect2-party-max-hp", mon)


if __name__ == "__main__":
    main()
