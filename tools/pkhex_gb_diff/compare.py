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
)


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
            diffs[(a[0], b[0])].append((key, kind, a[1], b[1]))
    total = collections.Counter()
    print(f"{'kind':18} {'both valid':>10} {'both invalid':>13} {'PKHeX only valid':>17} {'ours only valid':>16}"
          f" {'uncertain':>10}")
    for kind in sorted(table):
        c = table[kind]
        total.update(c)
        print(f"{kind:18} {c[('VALID', 'VALID')]:10} {c[('INVALID', 'INVALID')]:13} "
              f"{c[('VALID', 'INVALID')]:17} {c[('INVALID', 'VALID')]:16} "
              f"{c[('VALID', 'UNCERTAIN')] + c[('INVALID', 'UNCERTAIN')]:10}")
    agree = total[("VALID", "VALID")] + total[("INVALID", "INVALID")]
    uncertain = total[("VALID", "UNCERTAIN")] + total[("INVALID", "UNCERTAIN")]
    n = sum(v for k, v in total.items() if k != "missing")
    decided = n - uncertain
    print(f"\nagreement {agree}/{decided} decided records = {100.0 * agree / max(decided, 1):.2f}%  "
          f"(PKHeX-only-valid {total[('VALID', 'INVALID')]}, ours-only-valid {total[('INVALID', 'VALID')]})")
    print(f"uncertain {uncertain}/{n} (PKHeX VALID {total[('VALID', 'UNCERTAIN')]}, "
          f"PKHeX INVALID {total[('INVALID', 'UNCERTAIN')]})")
    for direction, items in diffs.items():
        label = (f"Uncertain, PKHeX {direction[1]}" if direction[0] == "uncertain" else
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
    raise SystemExit(1 if missing or extra or any(k[0] != "uncertain" for k in diffs) else 0)


if __name__ == "__main__":
    main()
