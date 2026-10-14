#!/usr/bin/env python3
"""Compare PKHeX and PokeBank NX verdicts on the same Gen I or Gen II corpus.

Usage: compare.py CORPUS PKHEX_OUT POKEBANK_OUT [--show N]
Prints an agreement table per corpus kind and the first N disagreements of each
direction, with both programs' explanations. Exits 1 if any verdict differs, if
either program skipped a corpus record, or if either printed a record the corpus
does not have.

PokeBank NX may also answer UNCERTAIN: every origin was excluded, but only by a rule
it does not treat as definitive. Those records are counted in their own columns, and
each one must cite a reason from UNCERTAIN_REASONS; any other UNCERTAIN fails.

A few verdicts differ from PKHeX's GB-era settings on purpose (DELIBERATE): each is
counted on its own and must name its documented origin; any other difference fails.
Every record lands in exactly one of: matching legal, matching invalid, deliberately
different, deliberately uncertain, or differing (a failure).
"""

import argparse
import collections

# The only rules PokeBank NX reports as not definitive (Problem::definitive == false).
UNCERTAIN_REASONS = (
    # LearnGroup1.FlagEvolutionSlots: PKHeX states the rule but never reaches it for PK1,
    # because GetPrevious never returns null, so no independent implementation checks it.
    "pre-evolution could not still have been at",
    # Mew and Celebi: no recorded distribution matches, and the GB-era event list is not
    # known to be complete. PKHeX answers INVALID here.
    "was only distributed at events",
    # Korean Gold/Silver: PKHeX allows no Crystal, Stadium 2 or Gen I content; Bulbapedia
    # documents Korean-Western cartridge trades that bring it in. PKHeX answers INVALID.
    "Needs a trade between Korean Gold/Silver and a Western game",
    # Korean Gold/Silver with Crystal met data: Gold/Silver zero those bytes (pokegold), but
    # the Korean ROM is not disassembled. PKHeX ignores them and may answer VALID.
    "Korean Gold/Silver do not record met data",
    # A Korean trainer name with one-byte-page characters: PKHeX's G1CharOT, which has not
    # been checked against the Korean naming screen. PKHeX answers INVALID.
    "Korean trainer name has characters outside the Hangul pages",
)


# (PKHeX verdict, PokeBank NX verdict, text the PokeBank explanation must contain).
DELIBERATE = (
    # Japanese Crystal's GS Ball Celebi (Mobile System GB, 2001-2002): PKHeX models the
    # Ilex Forest Celebi for Virtual Console only (Encounters2.CelebiVC).
    ("INVALID", "VALID", "GS Ball Celebi in Japanese Crystal"),
    # A level-100 Gen II Pokemon has exactly the level-100 EXP total (pokecrystal caps EXP
    # in battle and Rare Candy sets it). PKHeX rejects EXP above it, not below it.
    ("VALID", "INVALID", "A level-100 Pokemon has exactly"),
)

# Differences named by PKHeX's own result: (PKHeX verdict, PokeBank NX verdict, the result
# codes that must be PKHeX's only complaints).
DELIBERATE_PKHEX = (
    # PKHeX's word filter (a PKHeX setting) rejects some nicknames; Game Boy games filter
    # nothing, and PokeBank NX does not judge words.
    ("INVALID", "VALID", ("WordFilterFlaggedPattern", "WordFilterTooManyNumbers")),
)


def deliberate_pkhex(a, b):
    """True when every PKHeX complaint is one of a DELIBERATE_PKHEX entry's codes."""
    codes = a[1].partition(";")[2].split()
    for pv, ov, allowed in DELIBERATE_PKHEX:
        if a[0] == pv and b[0] == ov and codes and all(any(x in c for x in allowed) for c in codes):
            return True
    return False


def load(path):
    out = {}
    for line in open(path, encoding="utf-8"):
        head, _, rest = line.rstrip("\n").partition(" ")
        verdict, _, detail = rest.partition(" ")
        out[head] = (verdict, detail)
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("corpus")
    ap.add_argument("pkhex")
    ap.add_argument("pokebank")
    ap.add_argument("--show", type=int, default=8)
    args = ap.parse_args()
    kinds = {}
    for line in open(args.corpus):
        parts = line.split()
        kinds[parts[0]] = parts[1]
    pk, pb = load(args.pkhex), load(args.pokebank)
    missing = [k for k in kinds if k not in pk or k not in pb]
    extra = sorted((set(pk) | set(pb)) - set(kinds))
    table = collections.defaultdict(collections.Counter)
    diffs = collections.defaultdict(list)
    for key, kind in kinds.items():
        a, b = pk.get(key), pb.get(key)
        if not a or not b:
            table[kind]["missing"] += 1
            continue
        table[kind][(a[0], b[0])] += 1
        if b[0] == "UNCERTAIN":
            known = any(k in b[1] for k in UNCERTAIN_REASONS)
            diffs[("uncertain", a[0]) if known else ("unexplained",)].append((key, kind, a[1], b[1]))
        elif a[0] != b[0]:
            deliberate = (any(a[0] == pv and b[0] == ov and text in b[1] for pv, ov, text in DELIBERATE)
                          or deliberate_pkhex(a, b))
            if deliberate:
                table[kind]["deliberate"] += 1
                table[kind][(a[0], b[0])] -= 1
            diffs[("deliberate", a[0], b[0]) if deliberate else (a[0], b[0])].append((key, kind, a[1], b[1]))
    total = collections.Counter()
    print(f"{'kind':18} {'both valid':>10} {'both invalid':>13} {'PKHeX only valid':>17} {'ours only valid':>16}"
          f" {'uncertain':>10} {'deliberate':>11}")
    for kind in sorted(table):
        c = table[kind]
        total.update(c)
        print(f"{kind:18} {c[('VALID', 'VALID')]:10} {c[('INVALID', 'INVALID')]:13} "
              f"{c[('VALID', 'INVALID')]:17} {c[('INVALID', 'VALID')]:16} "
              f"{c[('VALID', 'UNCERTAIN')] + c[('INVALID', 'UNCERTAIN')]:10} {c['deliberate']:11}")
    agree = total[("VALID", "VALID")] + total[("INVALID", "INVALID")]
    uncertain = total[("VALID", "UNCERTAIN")] + total[("INVALID", "UNCERTAIN")]
    deliberate = total["deliberate"]
    differing = total[("VALID", "INVALID")] + total[("INVALID", "VALID")]
    n = sum(v for k, v in total.items() if k != "missing")
    decided = n - uncertain - deliberate
    print(f"\nmatching legal {total[('VALID', 'VALID')]}, matching invalid {total[('INVALID', 'INVALID')]}, "
          f"differing {differing}, deliberately uncertain {uncertain}, deliberately different {deliberate}")
    print(f"agreement {agree}/{decided} decided records = {100.0 * agree / max(decided, 1):.2f}%  "
          f"(PKHeX-only-valid {total[('VALID', 'INVALID')]}, ours-only-valid {total[('INVALID', 'VALID')]})")
    print(f"uncertain {uncertain}/{n} (PKHeX VALID {total[('VALID', 'UNCERTAIN')]}, "
          f"PKHeX INVALID {total[('INVALID', 'UNCERTAIN')]})")
    for direction, items in diffs.items():
        label = (f"Uncertain, PKHeX {direction[1]}" if direction[0] == "uncertain" else
                 f"Deliberately different, PKHeX {direction[1]}, PokeBank {direction[2]}"
                 if direction[0] == "deliberate" else
                 "UNCERTAIN without a known reason" if direction == ("unexplained",) else
                 "PKHeX VALID, PokeBank INVALID" if direction == ("VALID", "INVALID") else
                 "PKHeX INVALID, PokeBank VALID")
        print(f"\n== {label}: {len(items)}")
        for key, kind, a, b in items[:args.show]:
            print(f"  #{key} [{kind}]\n     PKHeX:    {a}\n     PokeBank: {b}")
    if missing or extra:
        print(f"\n== Corpus accounting: {len(missing)} records without both verdicts, "
              f"{len(extra)} verdicts for unknown records (first: {(missing + extra)[:5]})")
    print(f"\naccounted for {n}/{len(kinds)} corpus records")
    raise SystemExit(1 if missing or extra or any(k[0] not in ("uncertain", "deliberate") for k in diffs) else 0)


if __name__ == "__main__":
    main()
