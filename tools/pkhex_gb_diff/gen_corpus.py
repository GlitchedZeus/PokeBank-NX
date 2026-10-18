#!/usr/bin/env python3
"""Generate a corpus of raw Gen I party records for the PKHeX differential test.

Each output line is one Pokemon exactly as a Red/Blue/Yellow save stores it:

    <id> <kind> <japanese 0|1> <44-byte party body hex> <OT name hex> <nickname hex>

Japanese records use 6-byte name fields and the Japanese character table, both read
from PKHeX's StringConverter1, and may come from Japanese Blue's own encounters.

The corpus mixes Pokemon built the way the games build them (wild/static/trade
encounters, then levelled up, evolved and taught TMs) with deliberately broken
variants (wrong move, level, catch rate, types or stat experience, renamed trade
Pokemon, over-long or empty names) and fully random records, so both "should be legal" and "should be illegal" paths
are exercised. Nothing here decides legality; tools/pkhex_gb_diff/run.sh feeds
the same bytes to PKHeX and to PokeBank NX and compares their verdicts.

Usage:
    python3 tools/pkhex_gb_diff/gen_corpus.py --pkhex PKHEX --pokered POKERED \
        [--count 20000] [--seed 1] > corpus.txt
"""

import argparse
import math
import random
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent))
import gen_gb_legality_data as gen  # noqa: E402
import verify_gb_legality_data as ver  # noqa: E402
sys.path.insert(0, str(HERE))
import gb_text as gbtext  # noqa: E402

RD, GN, BU, YW = gen.RD, gen.GN, gen.BU, gen.YW
CHARS = {c: 0x80 + i for i, c in enumerate("ABCDEFGHIJKLMNOPQRSTUVWXYZ")}
CHARS[" "] = 0x7F
TRADE_OT = 0x5D
TERMINATOR = 0x50
MACHINES = [5, 13, 14, 18, 25, 92, 32, 34, 36, 38, 61, 55, 58, 59, 63, 6, 66, 68, 69, 99,
            72, 76, 82, 85, 87, 89, 90, 91, 94, 100, 102, 104, 115, 117, 118, 120, 121, 126,
            129, 130, 135, 138, 143, 156, 86, 149, 153, 157, 161, 164, 15, 19, 57, 70, 148]
# Every byte value that is also a Gen II held item (PKHeX ItemConverter.IsCatchRateHeldItem).
HELD = bytes([0x3F, 0xFF, 0xFF, 0xFD, 0xFF, 0xDF, 0x3B, 0xD2, 0x03, 0xFF, 0xFF, 0xFB, 0xEF, 0xFF,
              0xE7, 0x7E, 0x18, 0x9C, 0xC5, 0xF1, 0xFB, 0x77, 0xF0, 0xBF, 0xF7, 0xFF, 0xFF, 0xEF,
              0xFF, 0xFF, 0x07, 0x00])
HELD_RATES = [r for r in range(256) if (HELD[r >> 3] >> (r & 7)) & 1]


def gb_text(s, length):
    out = [CHARS[c] for c in s.upper() if c in CHARS][:length - 1]
    out.append(TERMINATOR)
    return bytes(out + [TERMINATOR] * (length - len(out)))


def exp_for(growth, level):
    n = level
    if growth == 0:
        e = n ** 3
    elif growth == 3:
        e = (6 * n ** 3) // 5 - 15 * n ** 2 + 100 * n - 140
    elif growth == 4:
        e = (4 * n ** 3) // 5
    elif growth == 5:
        e = (5 * n ** 3) // 4
    else:
        e = n ** 3
    return max(0, e) if level > 1 else 0


def stat_exp(rng):
    """Stat experience a trained Pokemon can carry: none, vitamins only (2560 each, ten
    at most), or battles, which add the defeated Pokemon's base stats to all five."""
    roll = rng.random()
    if roll < 0.35:
        return [0] * 5
    if roll < 0.6:
        return [2560 * rng.randint(0, 10) for _ in range(5)]
    return [rng.randint(1, 65535) for _ in range(5)]


def stat_exp_defect(rng):
    """Stat experience no Pokemon can have: battle-gained values with one stat still zero."""
    if rng.random() < 0.5:  # more than ten vitamins' worth in one stat, nothing elsewhere
        values = [0] * 5
        values[rng.randrange(5)] = 2560 * rng.randint(11, 25)
        return values
    values = [rng.randint(1, 65535) for _ in range(5)]
    values[rng.randrange(5)] = 0
    return values


def letters(rng, n):
    return "".join(rng.choice("ABCDEFGHIJKLMNOPQRSTUVWXYZ") for _ in range(n))


def stat(base, dv, level, is_hp):
    value = ((base + dv) * 2 * level) // 100
    return value + level + 10 if is_hp else value + 5


class Data:
    def __init__(self, pkhex, pokered):
        res = pkhex / "PKHeX.Core/Resources"
        self.personal = {False: gen.read_personal1(res / "byte/personal/personal_rb"),
                         True: gen.read_personal1(res / "byte/personal/personal_y")}
        self.learn = {False: gen.read_learnsets(res / "byte/levelup/lvlmove_rb.pkl", b"rb"),
                      True: gen.read_learnsets(res / "byte/levelup/lvlmove_y.pkl", b"yw")}
        self.evos = gen.read_evolutions(res / "byte/evolve/evos_g1.pkl", b"g1")
        self.wild = []
        for fname, v in (("encounter_red.pkl", RD), ("encounter_blue.pkl", GN),
                         ("encounter_blue_jp.pkl", BU), ("encounter_yellow.pkl", YW)):
            for s in gen.read_areas(res / "legality/wild/Gen1" / fname, b"g1", 1):
                self.wild.append((s[0], s[1], s[2], v))
        enc1 = (pkhex / "PKHeX.Core/Legality/Encounters/Data/Gen1/Encounters1.cs").read_text()
        self.statics = []
        for name in ("StaticRBY", "StaticRB", "StaticYW", "StaticBU"):
            self.statics.extend(gen.parse_static1(enc1, name))
        self.trades = []
        for name in ("TradeGift_RB", "TradeGift_YW", "TradeGift_BU"):
            self.trades.extend(gen.parse_trade1(enc1, name))
        self.trade_names = gen.read_text_lines(res / "legality/gen1/text_traderby_en.txt")
        self.trade_names_eu = {lang: gen.read_text_lines(res / f"legality/gen1/text_traderby_{lang}.txt")
                               for lang in ("de", "fr", "it", "es")}
        self.pkhex, self.pokered = pkhex, pokered
        self._gen2 = None
        self.trade_names_ja = gen.read_text_lines(res / "legality/gen1/text_traderby_ja.txt")
        self.jp = gbtext.Encoder(gbtext.read_table(pkhex, 1, "TableJP"))
        self.jp_names = gbtext.japanese_names(self.jp)
        pret = ver.Pret(pokered)
        by_internal = {v: k for k, v in pret.internal.items()}
        self.internal = {}
        for idx, name in by_internal.items():
            if name in pret.dex:
                self.internal[pret.dex[name]] = idx

    def gen2(self):
        """The Gen II corpus data, loaded on first use (gen_corpus2 imports this module)."""
        if self._gen2 is None:
            import gen_corpus2
            self._gen2 = gen_corpus2.Data(self.pkhex, self.pokered)
        return self._gen2

    def tms(self, species, yellow):
        bits = self.personal[yellow][species]["tm"]
        return [MACHINES[i] for i in range(55) if bits[i >> 3] & (1 << (i & 7))]

    def evolutions(self, species):
        return [(t, to, lv) for t, _a, to, _f, lv in self.evos[species]]


class Mon:
    def __init__(self, species, level, moves, catch, yellow):
        self.species, self.level, self.moves = species, level, list(moves)
        self.catch, self.yellow = catch, yellow
        self.types = None
        self.dvs = None
        self.tid = None
        self.ot, self.nick, self.trade_ot = "ASH", "MON", False
        self.statexp = [0] * 5
        self.exp = None  # EXP when it is not the level's own total (a defect)
        self.fill_name = False  # Japanese only: the name fills its field with no terminator
        self.ot_raw = None  # the stored name bytes, when a kind sets them directly
        self.nick_raw = None


# Structural defects shared by the PK1 and PK2 corpora: what a hex editor produces and the
# games never do. The "-ok" variants stay legal (the boundaries the rules must not cross).
STRUCTURAL = ("dup-move", "dup-move-all", "exp-zero", "exp-below", "exp-next", "exp-top-ok",
              "exp-cap-ok", "exp-cap-plus1", "exp-cap-big", "exp-cap-minus1",
              # Day Care slots (pokered IncrementDayCareMonExp, pokecrystal DayCareStep): EXP ahead
              # of the stored level, and EXP past the level-100 total up to the step counter's
              # reach ($50FFFF) or just past it.
              "exp-daycare-stale", "exp-daycare-99", "exp-daycare-reach", "exp-daycare-past")

DAY_CARE_EXP_MAX = 0x50FFFF


def structural_defect(mon, rng, variant, growth):
    """Apply one STRUCTURAL variant to `mon` (moves, level and exp attributes)."""
    if variant == "dup-move":
        if len(mon.moves) < 4:
            mon.moves.append(rng.choice(mon.moves))
        else:
            i, j = sorted(rng.sample(range(4), 2))
            mon.moves[j] = mon.moves[i]
    elif variant == "dup-move-all":
        mon.moves = [mon.moves[0]] * rng.randint(2, 4)
    elif variant == "exp-zero":
        mon.exp = 0
    elif variant == "exp-below":  # one short of the level's total
        mon.exp = exp_for(growth, mon.level) - 1 if mon.level > 1 else 0
    elif variant == "exp-next":  # the next level's total, level unchanged
        mon.exp = exp_for(growth, min(100, mon.level + 1)) + (1 if mon.level == 100 else 0)
    elif variant == "exp-top-ok":  # one short of the next level: still this level
        mon.exp = exp_for(growth, mon.level + 1) - 1 if mon.level < 100 else exp_for(growth, 100)
    elif variant == "exp-daycare-stale":  # a later level's total, the deposit level unchanged
        mon.level = min(mon.level, 99)
        mon.exp = exp_for(growth, rng.randint(mon.level + 1, 100))
    elif variant == "exp-daycare-99":  # a level-99 deposit carried past the level-100 total
        mon.level = 99
        mon.exp = rng.randint(exp_for(growth, 100) + 1, DAY_CARE_EXP_MAX)
    elif variant == "exp-daycare-reach":  # a level-100 deposit at the counter's reach (Gen I only)
        mon.level = 100
        mon.exp = rng.choice((DAY_CARE_EXP_MAX, rng.randint(exp_for(growth, 100) + 1, DAY_CARE_EXP_MAX)))
    elif variant == "exp-daycare-past":  # past what any Day Care slot reaches
        mon.level = rng.choice((99, 100))
        mon.exp = rng.choice((DAY_CARE_EXP_MAX + 1, rng.randint(DAY_CARE_EXP_MAX + 1, 0xFFFFFF)))
    else:
        mon.level = 100
        top = exp_for(growth, 100)
        mon.exp = {"exp-cap-ok": top, "exp-cap-plus1": top + 1, "exp-cap-big": top + rng.randint(2, 4000000),
                   "exp-cap-minus1": top - 1}[variant]


def learn(mon, move, rng):
    if move in mon.moves:
        return
    if len(mon.moves) < 4:
        mon.moves.append(move)
    elif rng.random() < 0.8:
        mon.moves[rng.randrange(4)] = move


def encounter_moves(d, species, level, yellow):
    moves = [m for m in d.personal[yellow][species]["moves"] if m]
    for lv, mv in d.learn[yellow][species]:
        if lv > level:
            break
        if mv not in moves:
            if len(moves) == 4:
                moves.pop(0)
            moves.append(mv)
    return moves


def grow(d, mon, target, rng):
    """Level up one level at a time, learning moves and sometimes evolving."""
    while mon.level < target:
        mon.level += 1
        for lv, mv in d.learn[mon.yellow][mon.species]:
            if lv == mon.level:
                learn(mon, mv, rng)
        for etype, to, lv in d.evolutions(mon.species):
            if etype == 4 and mon.level >= lv and rng.random() < 0.85:
                mon.species = to
                break
            if etype in (5, 8) and rng.random() < 0.05:
                mon.species = to
                break
    if rng.random() < 0.6:
        mon.statexp = stat_exp(rng)
    if rng.random() < 0.4:
        options = d.tms(mon.species, mon.yellow)
        for _ in range(rng.randrange(1, 3)):
            if options:
                learn(mon, rng.choice(options), rng)


def finish(d, mon, rng, japanese):
    p = d.personal[False][mon.species]
    if mon.types is None:
        mon.types = p["types"]
    if mon.dvs is None:
        mon.dvs = [rng.randrange(16) for _ in range(4)]
    if mon.tid is None:
        mon.tid = rng.randrange(65536)
    atk, df, spe, spc = mon.dvs
    hp_dv = ((atk & 1) << 3) | ((df & 1) << 2) | ((spe & 1) << 1) | (spc & 1)
    body = bytearray(44)
    body[0] = d.internal.get(mon.species, 0)
    stats = p["stats"]
    hp = stat(stats[0], hp_dv, mon.level, True)
    body[1:3] = hp.to_bytes(2, "big")
    body[3] = mon.level
    body[5], body[6] = mon.types
    body[7] = mon.catch
    for i, m in enumerate((mon.moves + [0, 0, 0, 0])[:4]):
        body[8 + i] = m
    body[0x0C:0x0E] = mon.tid.to_bytes(2, "big")
    exp = mon.exp if mon.exp is not None else exp_for(p["growth"], mon.level)
    body[0x0E:0x11] = exp.to_bytes(3, "big")
    for i, value in enumerate(mon.statexp):  # HP ATK DEF SPE SPC
        body[0x11 + i * 2:0x13 + i * 2] = value.to_bytes(2, "big")
    body[0x1B:0x1D] = ((atk << 12) | (df << 8) | (spe << 4) | spc).to_bytes(2, "big")
    body[0x21] = mon.level
    body[0x22:0x24] = hp.to_bytes(2, "big")
    for i, (b, dv) in enumerate(((stats[1], atk), (stats[2], df), (stats[3], spe), (stats[4], spc))):
        body[0x24 + i * 2:0x26 + i * 2] = stat(b, dv, mon.level, False).to_bytes(2, "big")
    length = 6 if japanese else 11
    if japanese:
        def text(s):
            return d.jp.encode(s, length, terminate=not mon.fill_name or len(s) < length)
    else:
        def text(s):
            return gb_text(s, length)
    ot = bytes([TRADE_OT, TERMINATOR] + [TERMINATOR] * (length - 2)) if mon.trade_ot else text(mon.ot)
    nick = text(mon.nick)
    if mon.ot_raw is not None:
        ot = mon.ot_raw
    if mon.nick_raw is not None:
        nick = mon.nick_raw
    return body, ot, nick


def with_byte(rng, text, byte, length=11):
    """`text` (A-Z, space) in a `length`-byte field with `byte` inserted at a random place.
    A byte on the Hangul pages (<= 0x0B) never goes first: there it would make the record
    Korean (PK1/PK2.Korean, StringConverter2KOR.IsHangul) instead of a damaged Western one."""
    core = [CHARS[c] for c in text]
    core.insert(rng.randrange(1 if byte <= 0x0B else 0, len(core) + 1), byte)
    core = core[:length - 1]
    return bytes(core + [TERMINATOR] * (length - len(core)))


def with_byte_jp(rng, encoder, text, byte):
    """The Japanese counterpart: `text` in a 6-byte field with `byte` inserted."""
    core = [encoder.index[c] for c in text]
    core.insert(rng.randrange(1 if byte <= 0x0B else 0, len(core) + 1), byte)
    core = core[:5]
    return bytes(core + [TERMINATOR] * (6 - len(core)))


# Bytes PKHeX's StringConverter1 tables give no character (TableEN / TableJP NUL entries):
# the games write none into a name, and a decoder stops at one.
GLYPHLESS_EN = (0x05, 0x4E, 0x60, 0x74, 0xD8, 0xE4)
GLYPHLESS_JP = (0x01, 0x14, 0x3F, 0x7A, 0xEC)


def evolution_order_pairs(d):
    """(pre-evolution, evolved form, version, evolved-only move and level, pre-evolution-only
    move and level) where the evolved form's level-up move comes earlier: the order Gen I's
    own games cannot produce, which Stadium 2's reminder can."""
    out = []
    for s1 in range(1, 152):
        for _etype, s2, _lv in d.evolutions(s1):
            for yellow in (False, True):
                first1, first2 = {}, {}
                for lv, mv in d.learn[yellow][s1]:
                    first1.setdefault(mv, lv)
                for lv, mv in d.learn[yellow][s2]:
                    first2.setdefault(mv, lv)
                for a, la in first2.items():
                    if a in first1 or la <= 1:
                        continue
                    for b, lb in first1.items():
                        if b not in first2 and lb > la:
                            out.append((s1, s2, yellow, a, la, b, lb))
    return out


def gen2_origin_mon(d, rng):
    """A Pokemon hatched or caught in Gold/Silver/Crystal, raised there, then sent to
    Red/Blue/Yellow through the Time Capsule: Gen I species and moves only, and its held
    item now sits in the catch-rate byte."""
    import gen_corpus2 as g2
    d2 = d.gen2()
    wild_gs = [w for w in d2.wild if w[6] != g2.C]
    for _ in range(100):
        mon2 = g2.egg_mon(d2, rng, True) if rng.random() < 0.5 else g2.wild_mon(d2, rng, rng.choice(wild_gs))
        g2.grow(d2, mon2, min(100, mon2.level + rng.randrange(0, 30)), rng)
        if mon2.species <= 151 and mon2.moves and all(m <= 165 for m in mon2.moves):
            break
    else:
        return None
    mon = Mon(mon2.species, mon2.level, mon2.moves, rng.choice(HELD_RATES), False)
    mon.dvs = list(mon2.dvs) if mon2.dvs else None
    mon.tid = mon2.tid
    mon.statexp = list(mon2.statexp)
    return mon


def japanize(d, mon, rng):
    """Give a Pokemon a Japanese trainer and nickname (the caller emits it as Japanese)."""
    mon.ot = rng.choice(d.jp_names)
    mon.nick = rng.choice(("ポケモン", "ピカ", "あいぼう", "ゴン"))


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

    def emit(kind, mon, japanese=False):
        nonlocal made
        body, ot, nick = finish(d, mon, rng, japanese)
        # PKHeX LanguageID as the bytes show it (PK1.Korean: a Hangul page first byte); 0 = other.
        lang = 1 if japanese else 8 if 1 <= ot[0] <= 11 else 0
        out.write(f"{made} {kind} {lang} {body.hex()} {ot.hex()} {nick.hex()}\n")
        made += 1

    def personal_catch(species, yellow):
        return d.personal[yellow][species]["catch"]

    # Every wild slot, as caught.
    for species, lmin, lmax, v in d.wild:
        yellow = v == YW
        level = rng.randint(lmin, lmax)
        mon = Mon(species, level, encounter_moves(d, species, level, yellow), personal_catch(species, yellow), yellow)
        if v == BU:
            japanize(d, mon, rng)
        emit("wild-jp" if v == BU else "wild", mon, v == BU)

    # Every static / gift, as received.
    for species, level, v in d.statics:
        yellow = v == YW
        catch = 163 if (v == YW and species == 25 and level == 5) else personal_catch(species, yellow)
        mon = Mon(species, level, encounter_moves(d, species, level, yellow), catch, yellow)
        if v == BU:
            japanize(d, mon, rng)
        emit("static-jp" if v == BU else "static", mon, v == BU)

    # In-game trades with the right and the wrong trainer / nickname.
    for species, v, _rby, gsc, idx, evolve in d.trades:
        yellow = v == YW
        for japanese in (False, True):
            names = d.trade_names_ja if japanese else d.trade_names
            if idx >= len(names) or (v == BU and not japanese):
                continue
            for variant in ("trade", "trade-wrong-ot", "trade-wrong-nick"):
                got = species + 1 if evolve else species
                mon = Mon(got, max(gsc, 5) + rng.randrange(10), [], personal_catch(species, yellow), yellow)
                mon.moves = encounter_moves(d, species, mon.level, yellow)
                mon.trade_ot = variant != "trade-wrong-ot"
                if japanese:
                    japanize(d, mon, rng)
                    mon.nick = names[idx] if variant != "trade-wrong-nick" else "ニセモノ"
                else:
                    mon.nick = names[idx] if variant != "trade-wrong-nick" else "NOTRADE"
                emit(variant + ("-jp" if japanese else ""), mon, japanese)

    # Structural defects and their legal boundaries, 30 records each, on grown wild Pokemon.
    for variant in STRUCTURAL:
        for _ in range(30):
            species, lmin, lmax, v = rng.choice([w for w in d.wild if w[3] != BU])
            yellow = v == YW
            level = rng.randint(lmin, lmax)
            mon = Mon(species, level, encounter_moves(d, species, level, yellow),
                      personal_catch(species, yellow), yellow)
            grow(d, mon, min(100, level + rng.randrange(0, 40)), rng)
            structural_defect(mon, rng, variant, d.personal[False][mon.species]["growth"])
            emit("struct-" + variant, mon)

    # Name bytes and characters, 30 records per kind on grown wild Pokemon: a digit (no
    # English naming screen offers one), a byte with no glyph in the font, a Japanese trainer
    # name outside the kana (PKHeX G1CharOT), the trade marker followed by a letter, and
    # Korean Gold/Silver names on a Red/Blue/Yellow record, both through a Western
    # Gold/Silver and the Time Capsule (kor1-tradeback) and as a Red/Blue/Yellow native,
    # which no Korean name can be (kor1-rby).
    def grown_wild():
        species, lmin, lmax, v = rng.choice([w for w in d.wild if w[3] != BU])
        yellow = v == YW
        level = rng.randint(lmin, lmax)
        mon = Mon(species, level, encounter_moves(d, species, level, yellow), personal_catch(species, yellow), yellow)
        grow(d, mon, min(100, level + rng.randrange(0, 40)), rng)
        return mon

    for _ in range(30):
        mon = grown_wild()
        mon.ot_raw = with_byte(rng, letters(rng, rng.randint(1, 5)), 0xF6 + rng.randrange(10))
        emit("defect-ot-digit", mon)
        mon = grown_wild()
        mon.nick_raw = with_byte(rng, letters(rng, rng.randint(1, 8)), 0xF6 + rng.randrange(10))
        emit("defect-nick-digit", mon)
        mon = grown_wild()
        if rng.random() < 0.5:
            mon.ot_raw = with_byte(rng, letters(rng, rng.randint(1, 5)), rng.choice(GLYPHLESS_EN))
        else:
            mon.nick_raw = with_byte(rng, letters(rng, rng.randint(1, 8)), rng.choice(GLYPHLESS_EN))
        emit("defect-name-glyphless", mon)
        mon = grown_wild()
        japanize(d, mon, rng)
        if rng.random() < 0.5:
            mon.ot_raw = with_byte_jp(rng, d.jp, mon.ot[:3], rng.choice(GLYPHLESS_JP))
        else:
            mon.nick_raw = with_byte_jp(rng, d.jp, mon.nick[:3], rng.choice(GLYPHLESS_JP))
        emit("defect-name-glyphless-jp", mon, True)
        mon = grown_wild()
        japanize(d, mon, rng)
        mon.ot = rng.choice(("サ１", "ゴ？", "ア×", "サ！", "ミ０", "カ。"))
        emit("defect-jp-ot-charset", mon, True)
    for species, v, _rby, gsc, idx, evolve in d.trades:
        if v == BU or idx >= len(d.trade_names):
            continue
        for _ in range(3):
            got = species + 1 if evolve else species
            mon = Mon(got, max(gsc, 5) + rng.randrange(10), [], personal_catch(species, v == YW), v == YW)
            mon.moves = encounter_moves(d, species, mon.level, v == YW)
            mon.nick = d.trade_names[idx]
            mon.ot_raw = bytes([TRADE_OT, 0x80 + rng.randrange(26)] + [TERMINATOR] * 9)
            emit("trade-ot-bad-term", mon)
    d2 = d.gen2()
    import gen_corpus2 as g2
    for _ in range(40):
        mon = gen2_origin_mon(d, rng)
        if mon is None:
            continue
        mon.ot_raw = d2.kor.encode(rng.choice(d2.kor_names), 11)
        mon.nick_raw = d2.kor.encode(rng.choice(g2.KOREAN_NICKS), 11)
        emit("kor1-tradeback", mon)
        mon = grown_wild()
        mon.ot_raw = d2.kor.encode(rng.choice(d2.kor_names), 11)
        mon.nick_raw = d2.kor.encode(rng.choice(g2.KOREAN_NICKS), 11)
        emit("kor1-rby", mon)
    for _ in range(15):
        mon = gen2_origin_mon(d, rng)
        if mon is None:
            continue
        mon.ot_raw = d2.kor.encode(rng.choice(d2.kor_names)[:2] + rng.choice(("A", "Z9", " B", "x", "♂", "1")), 11)
        mon.nick_raw = d2.kor.encode(rng.choice(g2.KOREAN_NICKS), 11)
        emit("kor1-ot-one-byte", mon)
        mon = gen2_origin_mon(d, rng)
        if mon is None:
            continue
        mon.ot_raw = d2.kor.encode("".join(rng.choice("한별민수지훈") for _ in range(5)) + "A", 11, terminate=False)
        mon.nick_raw = d2.kor.encode(rng.choice(g2.KOREAN_NICKS), 11)
        emit("kor1-ot-long", mon)

    # The same trades in German, French, Italian and Spanish Red/Blue/Yellow.
    for species, v, _rby, gsc, idx, evolve in d.trades:
        if v == BU:
            continue
        yellow = v == YW
        for lang, names in d.trade_names_eu.items():
            if idx >= len(names):
                continue
            for variant in ("trade", "trade-wrong-nick"):
                got = species + 1 if evolve else species
                mon = Mon(got, max(gsc, 5) + rng.randrange(10), [], personal_catch(species, yellow), yellow)
                mon.moves = encounter_moves(d, species, mon.level, yellow)
                mon.trade_ot = True
                mon.nick = names[idx] if variant == "trade" else "NOTRADE"
                emit(f"{variant}-{lang}", mon)

    # Every evolution-order pair once, with the pre-evolution's own catch rate (no Gen II
    # visit): caught at its lowest wild level, it learns the late move b before evolving,
    # and only Pokemon Stadium 2's reminder then gives the evolved form its earlier move a.
    lowest = {}
    for species, lmin, _lmax, v in d.wild:
        if v != BU:
            key = (species, v == YW)
            lowest[key] = min(lowest.get(key, 101), lmin)
    for s1, s2, yellow, a, la, b, lb in evolution_order_pairs(d):
        start = lowest.get((s1, yellow))
        if start is None or start > lb:
            continue
        mon = Mon(s1, start, encounter_moves(d, s1, start, yellow), personal_catch(s1, yellow), yellow)
        for lv in range(start + 1, lb + 1):
            for learn_lv, mv in d.learn[yellow][s1]:
                if learn_lv == lv and mv not in mon.moves:
                    if len(mon.moves) < 4:
                        mon.moves.append(mv)
                    else:
                        mon.moves[mon.moves.index(min(mon.moves, key=lambda m: m == b))] = mv
        mon.level = min(100, max(la, lb) + rng.randrange(0, 8))
        mon.species = s2
        if a not in mon.moves:
            if len(mon.moves) < 4:
                mon.moves.append(a)
            else:
                mon.moves[next(i for i, m in enumerate(mon.moves) if m != b)] = a
        emit("evo-order", mon)

    while made < args.count:
        roll = rng.random()
        japanese = rng.random() < 0.25
        jp = "-jp" if japanese else ""
        if roll < 0.45:  # natural growth from a random wild slot
            species, lmin, lmax, v = rng.choice(d.wild)
            if v == BU and not japanese:
                continue
            yellow = v == YW
            level = rng.randint(lmin, lmax)
            mon = Mon(species, level, encounter_moves(d, species, level, yellow),
                      personal_catch(species, yellow), yellow)
            grow(d, mon, min(100, level + rng.randrange(0, 60)), rng)
            if japanese:
                japanize(d, mon, rng)
            emit("grown" + jp, mon, japanese)
        elif roll < 0.55:  # natural growth from a static
            species, level, v = rng.choice(d.statics)
            if v == BU and not japanese:
                continue
            yellow = v == YW
            mon = Mon(species, level, encounter_moves(d, species, level, yellow),
                      personal_catch(species, yellow), yellow)
            grow(d, mon, min(100, level + rng.randrange(0, 50)), rng)
            if japanese:
                japanize(d, mon, rng)
            emit("grown-static" + jp, mon, japanese)
        elif roll < 0.85:  # one deliberate defect on a natural Pokemon
            species, lmin, lmax, v = rng.choice(d.wild)
            if v == BU and not japanese:
                continue
            yellow = v == YW
            level = rng.randint(lmin, lmax)
            mon = Mon(species, level, encounter_moves(d, species, level, yellow),
                      personal_catch(species, yellow), yellow)
            grow(d, mon, min(100, level + rng.randrange(0, 40)), rng)
            if japanese:
                japanize(d, mon, rng)
            defect = rng.choice(("move", "level", "catch", "catch-item", "types", "drop-move",
                                 "statexp", "ot-long", "ot-empty", "nick-empty", "structural")
                                + (("nick-long",) if japanese else ()))
            if defect == "structural":
                defect = rng.choice(STRUCTURAL)
                structural_defect(mon, rng, defect, d.personal[False][mon.species]["growth"])
            if defect == "move":
                mon.moves[rng.randrange(len(mon.moves))] = rng.randint(1, 165)
                mon.moves = list(dict.fromkeys(mon.moves))
            elif defect == "level":
                mon.level = rng.randint(2, 100)
            elif defect == "catch":
                mon.catch = rng.randrange(256)
            elif defect == "catch-item":
                mon.catch = rng.choice(HELD_RATES)
            elif defect == "types":
                mon.types = (rng.choice((0, 1, 2, 3, 4, 5, 7, 8, 20, 21, 22, 23, 24, 25, 26)),
                             rng.choice((0, 1, 2, 3, 4, 5, 7, 8, 20, 21, 22, 23, 24, 25, 26)))
            elif defect == "drop-move" and len(mon.moves) > 1:
                mon.moves.pop()
            elif defect == "statexp":
                mon.statexp = stat_exp_defect(rng)
            elif defect == "ot-long":  # seven letters (five in Japanese) is the limit
                if japanese:
                    mon.ot = "".join(rng.choice("アイウエオカキクケコ") for _ in range(rng.randint(5, 6)))
                    mon.fill_name = True
                else:
                    mon.ot = letters(rng, rng.randint(7, 10))
            elif defect == "nick-long":  # a Japanese nickname filling all six bytes
                mon.nick = "".join(rng.choice("アイウエオカキクケコ") for _ in range(rng.randint(5, 6)))
                mon.fill_name = True
            elif defect == "ot-empty":
                mon.ot = ""
            elif defect == "nick-empty":
                mon.nick = ""
            emit("defect-" + defect + jp, mon, japanese)
        elif roll < 0.92:  # bred or caught in Gen II, back in Gen I
            mon = gen2_origin_mon(d, rng)
            if mon is None:
                continue
            if japanese:
                japanize(d, mon, rng)
            emit("from-gen2" + jp, mon, japanese)
        else:  # fully random
            species = rng.randint(1, 151)
            moves = list(dict.fromkeys(rng.randint(1, 165) for _ in range(rng.randint(1, 4))))
            catch = rng.choice((personal_catch(species, False), rng.randrange(256), rng.choice(HELD_RATES)))
            mon = Mon(species, rng.randint(2, 100), moves, catch, False)
            mon.statexp = rng.choice((stat_exp(rng), [rng.randrange(65536) for _ in range(5)]))
            if japanese:
                japanize(d, mon, rng)
            emit("random" + jp, mon, japanese)


if __name__ == "__main__":
    main()
