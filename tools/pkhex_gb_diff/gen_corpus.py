#!/usr/bin/env python3
"""Generate a corpus of raw Gen I party records for the PKHeX differential test.

Each output line is one Pokemon exactly as a Red/Blue/Yellow save stores it:

    <id> <kind> <japanese 0|1> <44-byte party body hex> <OT name hex> <nickname hex>

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
        pret = ver.Pret(pokered)
        by_internal = {v: k for k, v in pret.internal.items()}
        self.internal = {}
        for idx, name in by_internal.items():
            if name in pret.dex:
                self.internal[pret.dex[name]] = idx

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
    body[0x0E:0x11] = exp_for(p["growth"], mon.level).to_bytes(3, "big")
    for i, value in enumerate(mon.statexp):  # HP ATK DEF SPE SPC
        body[0x11 + i * 2:0x13 + i * 2] = value.to_bytes(2, "big")
    body[0x1B:0x1D] = ((atk << 12) | (df << 8) | (spe << 4) | spc).to_bytes(2, "big")
    body[0x21] = mon.level
    body[0x22:0x24] = hp.to_bytes(2, "big")
    for i, (b, dv) in enumerate(((stats[1], atk), (stats[2], df), (stats[3], spe), (stats[4], spc))):
        body[0x24 + i * 2:0x26 + i * 2] = stat(b, dv, mon.level, False).to_bytes(2, "big")
    length = 6 if japanese else 11
    ot = bytes([TRADE_OT, TERMINATOR] + [TERMINATOR] * (length - 2)) if mon.trade_ot else gb_text(mon.ot, length)
    nick = gb_text(mon.nick, length)
    return body, ot, nick


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
        out.write(f"{made} {kind} {int(japanese)} {body.hex()} {ot.hex()} {nick.hex()}\n")
        made += 1

    def personal_catch(species, yellow):
        return d.personal[yellow][species]["catch"]

    # Every wild slot, as caught.
    for species, lmin, lmax, v in d.wild:
        if v == BU:
            continue
        yellow = v == YW
        level = rng.randint(lmin, lmax)
        emit("wild", Mon(species, level, encounter_moves(d, species, level, yellow),
                         personal_catch(species, yellow), yellow))

    # Every static / gift, as received.
    for species, level, v in d.statics:
        if v == BU:
            continue
        yellow = v == YW
        catch = 163 if (v == YW and species == 25 and level == 5) else personal_catch(species, yellow)
        emit("static", Mon(species, level, encounter_moves(d, species, level, yellow), catch, yellow))

    # In-game trades with the right and the wrong trainer / nickname.
    for species, v, _rby, gsc, idx, evolve in d.trades:
        if v == BU or idx >= len(d.trade_names):
            continue
        yellow = v == YW
        for variant in ("trade", "trade-wrong-ot", "trade-wrong-nick"):
            got = species + 1 if evolve else species
            mon = Mon(got, max(gsc, 5) + rng.randrange(10), [], personal_catch(species, yellow), yellow)
            mon.moves = encounter_moves(d, species, mon.level, yellow)
            mon.trade_ot = variant != "trade-wrong-ot"
            mon.nick = d.trade_names[idx] if variant != "trade-wrong-nick" else "NOTRADE"
            emit(variant, mon)

    while made < args.count:
        roll = rng.random()
        if roll < 0.45:  # natural growth from a random wild slot
            species, lmin, lmax, v = rng.choice(d.wild)
            if v == BU:
                continue
            yellow = v == YW
            level = rng.randint(lmin, lmax)
            mon = Mon(species, level, encounter_moves(d, species, level, yellow),
                      personal_catch(species, yellow), yellow)
            grow(d, mon, min(100, level + rng.randrange(0, 60)), rng)
            emit("grown", mon)
        elif roll < 0.55:  # natural growth from a static
            species, level, v = rng.choice(d.statics)
            if v == BU:
                continue
            yellow = v == YW
            mon = Mon(species, level, encounter_moves(d, species, level, yellow),
                      personal_catch(species, yellow), yellow)
            grow(d, mon, min(100, level + rng.randrange(0, 50)), rng)
            emit("grown-static", mon)
        elif roll < 0.85:  # one deliberate defect on a natural Pokemon
            species, lmin, lmax, v = rng.choice(d.wild)
            if v == BU:
                continue
            yellow = v == YW
            level = rng.randint(lmin, lmax)
            mon = Mon(species, level, encounter_moves(d, species, level, yellow),
                      personal_catch(species, yellow), yellow)
            grow(d, mon, min(100, level + rng.randrange(0, 40)), rng)
            defect = rng.choice(("move", "level", "catch", "catch-item", "types", "drop-move",
                                 "statexp", "ot-long", "ot-empty", "nick-empty"))
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
            elif defect == "ot-long":  # seven letters is the limit; seven is still fine
                mon.ot = letters(rng, rng.randint(7, 10))
            elif defect == "ot-empty":
                mon.ot = ""
            elif defect == "nick-empty":
                mon.nick = ""
            emit("defect-" + defect, mon)
        else:  # fully random
            species = rng.randint(1, 151)
            moves = list(dict.fromkeys(rng.randint(1, 165) for _ in range(rng.randint(1, 4))))
            catch = rng.choice((personal_catch(species, False), rng.randrange(256), rng.choice(HELD_RATES)))
            mon = Mon(species, rng.randint(2, 100), moves, catch, False)
            mon.statexp = rng.choice((stat_exp(rng), [rng.randrange(65536) for _ in range(5)]))
            emit("random", mon)


if __name__ == "__main__":
    main()
