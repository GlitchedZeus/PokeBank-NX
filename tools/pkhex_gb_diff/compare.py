#!/usr/bin/env python3
"""Compare PKHeX and PokeBank NX verdicts on the same Gen I or Gen II corpus.

Usage: compare.py CORPUS PKHEX_OUT POKEBANK_OUT [--show N]
Prints an agreement table per corpus kind and the first N disagreements of each
direction, with both programs' explanations. Exits 1 if any verdict differs, if
either program skipped a corpus record, or if either printed a record the corpus
does not have.
"""

import argparse
import collections

# PokeBank NX checks that PKHeX states but never reaches for PK1 files. A difference
# whose PokeBank explanation contains one of these is reported, not counted as a failure.
KNOWN_STRICTER = (
    # LearnGroup1.FlagEvolutionSlots: unreachable for PK1 because GetPrevious never returns null.
    "pre-evolution could not still have been at",
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
        if a[0] != b[0]:
            known = a[0] == "VALID" and any(k in b[1] for k in KNOWN_STRICTER)
            diffs[("known",) if known else (a[0], b[0])].append((key, kind, a[1], b[1]))
    total = collections.Counter()
    print(f"{'kind':18} {'both valid':>10} {'both invalid':>13} {'PKHeX only valid':>17} {'ours only valid':>16}")
    for kind in sorted(table):
        c = table[kind]
        total.update(c)
        print(f"{kind:18} {c[('VALID', 'VALID')]:10} {c[('INVALID', 'INVALID')]:13} "
              f"{c[('VALID', 'INVALID')]:17} {c[('INVALID', 'VALID')]:16}")
    agree = total[("VALID", "VALID")] + total[("INVALID", "INVALID")]
    n = sum(v for k, v in total.items() if k != "missing")
    print(f"\nagreement {agree}/{n} = {100.0 * agree / max(n, 1):.2f}%  "
          f"(PKHeX-only-valid {total[('VALID', 'INVALID')]}, ours-only-valid {total[('INVALID', 'VALID')]})")
    for direction, items in diffs.items():
        label = ("Known: PokeBank enforces a rule PKHeX skips for PK1" if direction == ("known",) else
                 "PKHeX VALID, PokeBank INVALID" if direction == ("VALID", "INVALID") else
                 "PKHeX INVALID, PokeBank VALID")
        print(f"\n== {label}: {len(items)}")
        for key, kind, a, b in items[:args.show]:
            print(f"  #{key} [{kind}]\n     PKHeX:    {a}\n     PokeBank: {b}")
    if missing or extra:
        print(f"\n== Corpus accounting: {len(missing)} records without both verdicts, "
              f"{len(extra)} verdicts for unknown records (first: {(missing + extra)[:5]})")
    print(f"\naccounted for {n}/{len(kinds)} corpus records")
    raise SystemExit(1 if missing or extra or any(k != ("known",) for k in diffs) else 0)


if __name__ == "__main__":
    main()
