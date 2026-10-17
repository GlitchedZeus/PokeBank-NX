#!/usr/bin/env python3
"""Compare PKHeX and PokeBank NX verdicts on the same Gen I or Gen II corpus.

Usage:
    compare.py CORPUS PKHEX_OUT POKEBANK_OUT --pkhex PKHEX --pokered POKERED [--show N]
    compare.py --self-test

Every corpus record lands in exactly one class:
  matching legal, matching invalid   both programs agree;
  deliberately uncertain             PokeBank NX answers UNCERTAIN, and every problem it
                                     gives is one of the known non-definitive rules, each
                                     checked against the record's own bytes;
  deliberately different             a documented difference, checked against the record's
                                     bytes and against both programs' complaints;
  differing                          anything else: a failure.

The run fails (exit 1) on any differing record. It stops with exit 2 on a protocol error:
a malformed or duplicate corpus line, a malformed or duplicate verdict line, a verdict
for a record the corpus does not have, a record without a verdict from either program,
or a verdict word a program may not give. Explanatory text is never enough on its own:
an exception applies only when its predicate holds for the record.
"""

import argparse
import collections
import re
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent))


class ProtocolError(Exception):
    pass


# ---------------------------------------------------------------- record decoding

Record = collections.namedtuple(
    "Record", "id kind lang body ot nick egg format species level exp caught korean japanese")

HEX = re.compile(r"^(?:[0-9a-f]{2})+$")


def exp_for(growth, level):
    """Total EXP at `level` (Gen I/II formulas; growth codes 0 MF, 3 MS, 4 fast, 5 slow)."""
    n = level
    if level <= 1:
        return 0
    if growth == 3:
        return (6 * n ** 3) // 5 - 15 * n ** 2 + 100 * n - 140
    if growth == 4:
        return (4 * n ** 3) // 5
    if growth == 5:
        return (5 * n ** 3) // 4
    return n ** 3


def level_for(growth, exp):
    level = 1
    while level < 100 and exp >= exp_for(growth, level + 1):
        level += 1
    return level


class Context:
    """What the predicates need beyond the record: growth rates by national dex number and
    the Gen I internal species index."""

    def __init__(self, growth, internal_to_national):
        self.growth = growth
        self.internal_to_national = internal_to_national

    @staticmethod
    def load(pkhex, pokered):
        import gen_gb_legality_data as gen
        import verify_gb_legality_data as ver
        personal = gen.read_personal2(pkhex / "PKHeX.Core/Resources/byte/personal/personal_c")
        growth = {s: personal[s]["growth"] for s in range(1, 252)}
        pret = ver.Pret(pokered)
        by_internal = {v: k for k, v in pret.internal.items()}
        internal = {idx: pret.dex[name] for idx, name in by_internal.items() if name in pret.dex}
        return Context(growth, internal)


def parse_corpus_line(line, lineno, ctx):
    parts = line.split()
    if len(parts) not in (6, 7):
        raise ProtocolError(f"corpus line {lineno}: expected 6 or 7 fields, got {len(parts)}")
    rid, kind, lang = parts[0], parts[1], parts[2]
    if not rid.isdigit() or not re.match(r"^[A-Za-z0-9-]+$", kind) or lang not in ("0", "1", "8"):
        raise ProtocolError(f"corpus line {lineno}: malformed id, kind or language")
    for field in parts[3:6]:
        if not HEX.match(field):
            raise ProtocolError(f"corpus line {lineno}: malformed hex field")
    body, ot, nick = (bytes.fromhex(x) for x in parts[3:6])
    if len(body) == 44 and len(parts) == 6:
        fmt = 1
    elif len(body) == 48 and len(parts) == 7 and parts[6] in ("0", "1"):
        fmt = 2
    else:
        raise ProtocolError(f"corpus line {lineno}: body length {len(body)} does not fit the field count")
    if len(ot) != len(nick) or len(ot) not in (6, 11):
        raise ProtocolError(f"corpus line {lineno}: name fields must both be 6 or 11 bytes")
    japanese = len(ot) == 6
    if (lang == "1") != japanese:
        raise ProtocolError(f"corpus line {lineno}: language column disagrees with the name field length")
    if fmt == 1:
        species = ctx.internal_to_national.get(body[0], 0)
        level, exp, caught, egg = body[0x21], int.from_bytes(body[0x0E:0x11], "big"), 0, False
    else:
        species, level = body[0], body[31]
        exp, caught, egg = int.from_bytes(body[8:11], "big"), int.from_bytes(body[29:31], "big"), parts[6] == "1"
    korean = fmt == 2 and not japanese and ot[0] <= 11  # PK2.Korean
    if (lang == "8") != korean:
        raise ProtocolError(f"corpus line {lineno}: language column disagrees with the trainer name bytes")
    return Record(rid, kind, lang, body, ot, nick, egg, fmt, species, level, exp, caught, korean, japanese)


def load_corpus(path, ctx):
    records = {}
    with open(path, encoding="utf-8") as f:
        for lineno, line in enumerate(f, 1):
            if not line.strip():
                raise ProtocolError(f"corpus line {lineno}: empty line")
            rec = parse_corpus_line(line, lineno, ctx)
            if rec.id in records:
                raise ProtocolError(f"corpus line {lineno}: duplicate record id {rec.id}")
            records[rec.id] = rec
    if not records:
        raise ProtocolError("corpus is empty")
    return records


VERDICT_LINE = re.compile(r"^(\d+) (VALID|INVALID|UNCERTAIN)(?: (.*))?$")


def load_verdicts(path, allowed, records, who):
    out = {}
    with open(path, encoding="utf-8") as f:
        for lineno, line in enumerate(f, 1):
            m = VERDICT_LINE.match(line.rstrip("\n"))
            if not m:
                raise ProtocolError(f"{who} line {lineno}: malformed verdict line")
            rid, verdict, detail = m.group(1), m.group(2), m.group(3) or ""
            if verdict not in allowed:
                raise ProtocolError(f"{who} line {lineno}: {who} may not answer {verdict}")
            if rid in out:
                raise ProtocolError(f"{who} line {lineno}: second verdict for record {rid}")
            if rid not in records:
                raise ProtocolError(f"{who} line {lineno}: verdict for record {rid}, which the corpus does not have")
            out[rid] = (verdict, detail)
    missing = [rid for rid in records if rid not in out]
    if missing:
        raise ProtocolError(f"{who}: {len(missing)} corpus records have no verdict (first: {missing[:5]})")
    return out


def pkhex_codes(detail):
    return detail.partition(";")[2].split()


def pokebank_problems(detail):
    return [p.strip() for p in detail.partition(";")[2].split(" | ") if p.strip()]


# ---------------------------------------------------------------- exceptions

def korean_one_byte_char(ot):
    """True when a Korean trainer name holds a one-byte-page character (StringConverter2KOR)."""
    i = 0
    while i < len(ot):
        b = ot[i]
        if b in (0, 0x50):
            return False
        if b <= 11:
            i += 2
            continue
        return True
    return False


# The rules PokeBank NX reports as not definitive (Problem::definitive == false): the text
# that names each, and what must be true of the record for it to apply.
UNCERTAIN_REASONS = (
    # LearnGroup1.FlagEvolutionSlots: PKHeX states the rule but never reaches it for PK1.
    ("gen1-evolution-order", "pre-evolution could not still have been at",
     lambda r, ctx: sum(1 for m in move_ids(r) if m) >= 2),
    # Mew and Celebi: no recorded distribution matches; the event list is not known complete.
    ("event-only", "was only distributed at events", lambda r, ctx: r.species in (151, 251)),
    # Korean Gold/Silver: needs a Korean-Western cartridge trade (Bulbapedia), which PKHeX bans.
    ("korean-western-trade", "Needs a trade between Korean Gold/Silver and a Western game",
     lambda r, ctx: r.korean),
    ("korean-met-data", "Korean Gold/Silver do not record met data", lambda r, ctx: r.korean and r.caught != 0),
    # EffortValueVerifier.VerifyEVsGB (EVs:EffortShouldBeZero): PKHeX's "simple sanity check".
    # pret/pokered DivideExpDataByNumMonsGainingExp floors each enemy base stat by the
    # recipient count (halved first under Exp. All), so a stat can gain 0 in a battle.
    ("gb-divided-stat-exp-zero", "Stat experience from battles with one stat still at zero",
     lambda r, ctx: not r.egg and battle_stat_exp_with_zero(r)),
    ("korean-trainer-name", "Korean trainer name has characters outside the Hangul pages",
     lambda r, ctx: r.korean and korean_one_byte_char(r.ot)),
    # A Pokemon still in the Day Care: its EXP grows a step at a time while the stored level
    # stays what it was on deposit (pokered engine/overworld/daycare_exp.asm, pokecrystal
    # engine/events/happiness_egg.asm DayCareStep). PKHeX derives the level from EXP (VALID).
    ("daycare-stale-level", "is below the level the EXP gives", lambda r, ctx: daycare_stale_level(r, ctx)),
    # The same slot past the level-100 total, within the step counter's reach ($50FFFF); Gen II
    # only while the stored level is below 100. PKHeX: Level:LevelEXPTooHigh (INVALID).
    ("daycare-exp-over-cap", "only a Pokemon still in the Day Care carries that",
     lambda r, ctx: daycare_exp_over_cap(r, ctx)),
)

DAY_CARE_EXP_MAX = 0x50FFFF


def daycare_stale_level(r, ctx):
    if r.egg or r.species not in ctx.growth:
        return False
    growth = ctx.growth[r.species]
    return r.exp <= exp_for(growth, 100) and level_for(growth, r.exp) > r.level


def daycare_exp_over_cap(r, ctx):
    if r.egg or r.species not in ctx.growth:
        return False
    top = exp_for(ctx.growth[r.species], 100)
    return top < r.exp <= DAY_CARE_EXP_MAX and (r.format == 1 or r.level < 100)


def stat_exp(r):
    """The five big-endian stat experience words: PK1 body 0x11, PK2 body 11."""
    at = 0x11 if r.format == 1 else 11
    return [(r.body[at + 2 * i] << 8) | r.body[at + 2 * i + 1] for i in range(5)]


def battle_stat_exp_with_zero(r):
    """EffortValues.VitaminBoost12 2560, MaxVitamins12 25600."""
    values = stat_exp(r)
    battle = any(v > 25600 or v % 2560 for v in values)
    return battle and 0 in values


def move_ids(r):
    return r.body[8:12] if r.format == 1 else r.body[2:6]


def gs_ball(r, ctx, pk, pb):
    location, met_level = r.caught & 0x7F, (r.caught >> 8) & 0x3F
    return (r.format == 2 and r.species == 251 and r.japanese and location == 14 and met_level == 30
            and set(pkhex_codes(pk[1])) <= {"Encounter:EncInvalid", "Fateful:FatefulGiftMissing"}
            and pb[1].startswith("GS Ball Celebi in Japanese Crystal"))


LEVEL_EXP = re.compile(r"^Level (\d+) does not match EXP \(expected (\d+)\)$")


def level_exp(r, ctx, pk, pb):
    if r.egg or r.species not in ctx.growth:
        return False
    growth = ctx.growth[r.species]
    if r.exp > exp_for(growth, 100):
        return False
    expected = level_for(growth, r.exp)
    problems = pokebank_problems(pb[1])
    # Only the stored level ahead of its EXP is Invalid; EXP ahead of the level is a Day Care
    # slot (UNCERTAIN_REASONS daycare-stale-level).
    if expected >= r.level or not problems:
        return False
    for p in problems:
        m = LEVEL_EXP.match(p)
        if not m or int(m.group(1)) != r.level or int(m.group(2)) != expected:
            return False
    return True


def word_filter(r, ctx, pk, pb):
    codes = pkhex_codes(pk[1])
    return bool(codes) and all(c.split(":")[-1].startswith("WordFilter") for c in codes)


# (name, PKHeX verdict, PokeBank NX verdict, predicate over the record and both outputs).
DELIBERATE = (
    # Japanese Crystal's GS Ball Celebi (Mobile System GB, 2001-2002): PKHeX models the
    # Ilex Forest Celebi for Virtual Console only (Encounters2.CelebiVC).
    ("gs-ball-celebi", "INVALID", "VALID", gs_ball),
    # The stored level is never ahead of its EXP (battle, Rare Candy and Day Care withdrawal in
    # pokered/pokecrystal keep EXP at or above the level's total); PKHeX derives the level from
    # EXP and does not compare them.
    ("level-exp", "VALID", "INVALID", level_exp),
    # PKHeX's word filter (a PKHeX setting) rejects some nicknames; Game Boy games filter none.
    ("pkhex-word-filter", "INVALID", "VALID", word_filter),
)


def classify(r, pk, pb, ctx):
    """(class, detail): 'legal', 'invalid', ('uncertain', reasons), ('deliberate', name) or
    ('differing', why)."""
    if pb[0] == "UNCERTAIN":
        problems = pokebank_problems(pb[1])
        if not problems:
            return "differing", "UNCERTAIN without a problem"
        reasons = []
        for p in problems:
            hit = [name for name, text, pred in UNCERTAIN_REASONS if text in p and pred(r, ctx)]
            if not hit:
                return "differing", "UNCERTAIN problem without a verified reason: " + p
            reasons.append(hit[0])
        return "uncertain", "+".join(sorted(set(reasons)))
    if pk[0] == pb[0]:
        return ("legal" if pk[0] == "VALID" else "invalid"), ""
    for name, pv, ov, pred in DELIBERATE:
        if pk[0] == pv and pb[0] == ov and pred(r, ctx, pk, pb):
            return "deliberate", name
    return "differing", f"PKHeX {pk[0]}, PokeBank {pb[0]}"


# ---------------------------------------------------------------- report

def compare(corpus, pkhex_out, pokebank_out, ctx, show, out=sys.stdout):
    records = load_corpus(corpus, ctx)
    pk = load_verdicts(pkhex_out, ("VALID", "INVALID"), records, "PKHeX")
    pb = load_verdicts(pokebank_out, ("VALID", "INVALID", "UNCERTAIN"), records, "PokeBank")
    table = collections.defaultdict(collections.Counter)
    listed = collections.defaultdict(list)
    for rid, r in records.items():
        cls, detail = classify(r, pk[rid], pb[rid], ctx)
        table[r.kind][cls] += 1
        if cls not in ("legal", "invalid"):
            listed[(cls, detail, pk[rid][0])].append(rid)
    total = collections.Counter()
    print(f"{'kind':24} {'legal':>8} {'invalid':>8} {'uncertain':>10} {'deliberate':>11} {'differing':>10}", file=out)
    for kind in sorted(table):
        c = table[kind]
        total.update(c)
        print(f"{kind:24} {c['legal']:8} {c['invalid']:8} {c['uncertain']:10} {c['deliberate']:11} "
              f"{c['differing']:10}", file=out)
    n = sum(total.values())
    print(f"\nmatching legal {total['legal']}, matching invalid {total['invalid']}, differing {total['differing']}, "
          f"deliberately uncertain {total['uncertain']}, deliberately different {total['deliberate']}", file=out)
    # Every record outside the two matching classes, by class and reason.
    for (cls, detail, pkv), ids in sorted(listed.items()):
        print(f"\n== {cls}: {detail} (PKHeX {pkv}): {len(ids)}", file=out)
        print("   ids: " + " ".join(ids), file=out)
        for rid in ids[:show]:
            print(f"   #{rid} [{records[rid].kind}]\n      PKHeX:    {pk[rid][1]}\n      PokeBank: {pb[rid][1]}",
                  file=out)
    print(f"\naccounted for {n}/{len(records)} corpus records", file=out)
    return total["differing"] == 0 and n == len(records)


# ---------------------------------------------------------------- self-test

def self_test():
    """Corrupt a small valid run every way the protocol forbids; each must fail."""
    ctx = Context({1: 3, 151: 3, 251: 3, 25: 0}, {})

    def pk2(species, level, exp, caught=0, moves=(33, 45), stat_exp=(0, 0, 0, 0, 0)):
        body = bytearray(48)
        body[0] = species
        body[2:2 + len(moves)] = bytes(moves)
        body[8:11] = exp.to_bytes(3, "big")
        for i, v in enumerate(stat_exp):
            body[11 + 2 * i:13 + 2 * i] = v.to_bytes(2, "big")
        body[29:31] = caught.to_bytes(2, "big")
        body[31] = level
        return body.hex()

    intl = "868e8b8350505050505050"
    jp = "8a938b505050"
    kor = "0161034550505050505050"  # 골드
    lines = [
        f"0 a 0 {pk2(1, 5, 135)} {intl} {intl} 0",
        f"1 b 0 {pk2(1, 5, 135)} {intl} {intl} 0",
        f"2 mew 0 {pk2(151, 5, 135)} {intl} {intl} 0",
        f"3 lvl 0 {pk2(1, 50, 0)} {intl} {intl} 0",
        f"4 gs 1 {pk2(251, 30, exp_for(3, 30), caught=(30 << 8) | 14)} {jp} {jp} 0",
        f"5 kor 8 {pk2(1, 5, 135)} {kor} {kor} 0",
        f"6 sx 0 {pk2(1, 5, 135, stat_exp=(65535, 0, 1, 32768, 65534))} {intl} {intl} 0",
        f"7 dc 0 {pk2(1, 5, exp_for(3, 30))} {intl} {intl} 0",
        f"8 dc2 0 {pk2(1, 99, exp_for(3, 100) + 1)} {intl} {intl} 0",
    ]
    pkhex = ["0 VALID x ; ", "1 INVALID x ; Move1:Duplicate", "2 INVALID x ; Encounter:EncInvalid",
             "3 VALID x ; ", "4 INVALID x ; Encounter:EncInvalid Fateful:FatefulGiftMissing",
             "5 INVALID x ; Move2:None", "6 INVALID x ; EVs:EffortShouldBeZero", "7 VALID x ; ",
             "8 INVALID x ; Level:LevelEXPTooHigh"]
    pokebank = ["0 VALID x ;", "1 INVALID x ; | Duplicate move: Tackle",
                "2 UNCERTAIN no known origin ; | Mew was only distributed at events; the event list is not known",
                "3 INVALID x ; | Level 50 does not match EXP (expected 1)",
                "4 VALID GS Ball Celebi in Japanese Crystal (level 30) ;",
                "5 UNCERTAIN x ; | Needs a trade between Korean Gold/Silver and a Western game (Bulbapedia)",
                "6 UNCERTAIN x ; | Stat experience from battles with one stat still at zero needs every one",
                "7 UNCERTAIN x ; | Level 5 is below the level the EXP gives (30); only a Pokemon still in the Day Care",
                "8 UNCERTAIN x ; | EXP 1059861 is above the level-100 total of 1059860; only a Pokemon still in the "
                "Day Care carries that"]

    def run(c, a, b):
        with tempfile.TemporaryDirectory() as tmp:
            paths = []
            for name, content in (("c", c), ("a", a), ("b", b)):
                p = Path(tmp) / name
                p.write_text("".join(x + "\n" for x in content), encoding="utf-8")
                paths.append(p)
            try:
                with open(Path(tmp) / "out", "w") as sink:
                    return "pass" if compare(*paths, ctx, 0, out=sink) else "differ"
            except ProtocolError:
                return "protocol"

    def edit(items, i, new):
        items = list(items)
        items[i] = new
        return items

    cases = [
        ("valid run", "pass", lines, pkhex, pokebank),
        ("duplicate corpus id", "protocol", lines + [lines[0]], pkhex, pokebank),
        ("malformed corpus line", "protocol", lines + ["9 x 0 zz"], pkhex, pokebank),
        ("odd hex", "protocol", edit(lines, 0, lines[0].replace(" 0 ", " 0 0", 1)), pkhex, pokebank),
        ("language column lies", "protocol", edit(lines, 0, lines[0].replace(" a 0 ", " a 1 ")), pkhex, pokebank),
        ("duplicate PKHeX verdict", "protocol", lines, pkhex + [pkhex[0]], pokebank),
        ("duplicate PokeBank verdict, different answer", "protocol", lines, pkhex, pokebank + ["0 INVALID x ;"]),
        ("malformed verdict line", "protocol", lines, edit(pkhex, 0, "0 MAYBE"), pokebank),
        ("PKHeX may not be UNCERTAIN", "protocol", lines, edit(pkhex, 0, "0 UNCERTAIN x ;"), pokebank),
        ("missing verdict", "protocol", lines, pkhex[:-1], pokebank),
        ("verdict for an unknown record", "protocol", lines, pkhex + ["99 VALID x ;"], pokebank),
        ("empty corpus line", "protocol", lines + [""], pkhex, pokebank),
        ("plain disagreement", "differ", lines, edit(pkhex, 0, "0 INVALID x ; Move1:None"), pokebank),
        ("event reason on a non-event species", "differ",
         edit(lines, 2, lines[2].replace(pk2(151, 5, 135), pk2(25, 5, 125))), pkhex, pokebank),
        ("uncertain with an unknown reason", "differ", lines, pkhex, edit(pokebank, 2, "2 UNCERTAIN x ; | made up")),
        ("level-exp exception when level and EXP agree", "differ",
         edit(lines, 3, lines[3].replace(pk2(1, 50, 0), pk2(1, 50, exp_for(3, 50)))), pkhex, pokebank),
        ("level-exp exception with another problem", "differ", lines, pkhex,
         edit(pokebank, 3, "3 INVALID x ; | Level 50 does not match EXP (expected 1) | Bulbasaur cannot learn X")),
        # The stored level ahead of its EXP is the deliberate class; EXP ahead of the level is not.
        ("level-exp exception claimed for a stale Day Care level", "differ",
         edit(lines, 3, lines[3].replace(pk2(1, 50, 0), pk2(1, 5, exp_for(3, 30)))), pkhex,
         edit(pokebank, 3, "3 INVALID x ; | Level 5 does not match EXP (expected 30)")),
        ("stale-level reason when the level is ahead", "differ",
         edit(lines, 7, lines[7].replace(pk2(1, 5, exp_for(3, 30)), pk2(1, 50, exp_for(3, 30)))), pkhex, pokebank),
        ("stale-level reason when level and EXP agree", "differ",
         edit(lines, 7, lines[7].replace(pk2(1, 5, exp_for(3, 30)), pk2(1, 30, exp_for(3, 30)))), pkhex, pokebank),
        ("Day Care over-cap reason at level 100 (DayCareStep skips it)", "differ",
         edit(lines, 8, lines[8].replace(pk2(1, 99, exp_for(3, 100) + 1), pk2(1, 100, exp_for(3, 100) + 1))),
         pkhex, pokebank),
        ("Day Care over-cap reason past $50FFFF", "differ",
         edit(lines, 8, lines[8].replace(pk2(1, 99, exp_for(3, 100) + 1), pk2(1, 99, 0x510000))), pkhex, pokebank),
        ("Day Care over-cap reason on an egg", "differ",
         edit(lines, 8, lines[8][:-1] + "1"), pkhex, pokebank),
        ("GS Ball exception at the wrong place", "differ",
         edit(lines, 4, lines[4].replace(pk2(251, 30, exp_for(3, 30), caught=(30 << 8) | 14),
                                         pk2(251, 30, exp_for(3, 30), caught=(30 << 8) | 15))), pkhex, pokebank),
        ("GS Ball exception with another PKHeX complaint", "differ", lines,
         edit(pkhex, 4, "4 INVALID x ; Encounter:EncInvalid Move1:None"), pokebank),
        ("Korean reason on a non-Korean record", "differ",
         edit(lines, 5, lines[5].replace(f" kor 8 {pk2(1, 5, 135)} {kor} {kor}", f" kor 0 {pk2(1, 5, 135)} {intl} {intl}")),
         pkhex, pokebank),
        ("divided stat-exp reason without a zero stat", "differ",
         edit(lines, 6, lines[6].replace(pk2(1, 5, 135, stat_exp=(65535, 0, 1, 32768, 65534)),
                                         pk2(1, 5, 135, stat_exp=(65535, 7, 1, 32768, 65534)))), pkhex, pokebank),
        ("divided stat-exp reason on vitamin-only stat exp", "differ",
         edit(lines, 6, lines[6].replace(pk2(1, 5, 135, stat_exp=(65535, 0, 1, 32768, 65534)),
                                         pk2(1, 5, 135, stat_exp=(2560, 0, 5120, 0, 0)))), pkhex, pokebank),
        ("word-filter exception with a real complaint", "differ", lines,
         edit(pkhex, 0, "0 INVALID x ; Nickname:WordFilterFlaggedPattern_01 Move1:None"), pokebank),
    ]
    failures = 0
    for name, expect, c, a, b in cases:
        got = run(c, a, b)
        ok = got == expect
        failures += not ok
        print(f"{'ok  ' if ok else 'FAIL'} {name}: expected {expect}, got {got}")
    print(f"compare.py self-test: {len(cases) - failures}/{len(cases)} PASS")
    return failures == 0


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("corpus", nargs="?")
    ap.add_argument("pkhex_out", nargs="?")
    ap.add_argument("pokebank_out", nargs="?")
    ap.add_argument("--pkhex", type=Path)
    ap.add_argument("--pokered", type=Path)
    ap.add_argument("--show", type=int, default=8)
    ap.add_argument("--self-test", action="store_true")
    args = ap.parse_args()
    if args.self_test:
        raise SystemExit(0 if self_test() else 1)
    if not (args.corpus and args.pkhex_out and args.pokebank_out and args.pkhex and args.pokered):
        ap.error("CORPUS PKHEX_OUT POKEBANK_OUT --pkhex and --pokered are required")
    try:
        ok = compare(Path(args.corpus), Path(args.pkhex_out), Path(args.pokebank_out),
                     Context.load(args.pkhex, args.pokered), args.show)
    except ProtocolError as e:
        print(f"PROTOCOL ERROR: {e}")
        raise SystemExit(2)
    raise SystemExit(0 if ok else 1)


if __name__ == "__main__":
    main()
