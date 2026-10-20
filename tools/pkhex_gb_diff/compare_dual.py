#!/usr/bin/env python3
"""Keep the old differential predicates and additionally audit the newer oracle.

Raw verdicts are never rewritten. Newly added upstream egg-chain rejections need
the newer oracle's exact complaint AND no independent native witness. Clean
differences against the newer oracle require a constructive pret-only witness.
Unverified gift ancestry stays separately counted as UNCERTAIN.
"""
import argparse
import collections
import re
from pathlib import Path
import compare
from native_chain import NativeChain

CHAIN = "No single compatible father ancestry supplies the inherited move combination"
HISTORY = "The compatible father ancestry requires a catalogued fixed-move gift whose historical distribution is not authenticated"


def classify(record, oracle, nx, current, context, native, proof):
    original = compare.classify(record, oracle, nx, context)
    if original[0] != "differing":
        return original
    if nx[0] == "UNCERTAIN" and HISTORY in nx[1]:
        # The selected witness must really contain a catalogued gift seed.
        if record.format not in (1, 2) or not any(step.startswith("1:") for step in proof.split()[2:]):
            return original
        reduced = (nx[0], nx[1].replace(" | " + HISTORY, ""))
        rest = compare.pokebank_problems(reduced[1])
        if rest and compare.classify(record, oracle, reduced, context)[0] != "uncertain":
            return original
        if not native.certifies(record, context):
            return "uncertain", "unauthenticated-parent-gift"
    if (oracle[0], nx[0]) == ("VALID", "INVALID"):
        if (current[0] == "INVALID" and
                all(re.fullmatch(r"Egg:EggMoveCombination|Move[1-4]:(?:Unobtainable(?:Expect)?|None)", issue) for issue in current[1].split(";",1)[-1].split() if not issue.startswith("~")) and
                current[1].split(";",1)[-1].strip() and
                compare.pokebank_problems(nx[1]) and
                all(problem == CHAIN or native.move_failure_matches(record, problem) for problem in compare.pokebank_problems(nx[1])) and
                not native.certifies(record, context)):
            return "deliberate", "newer-upstream-joint-parent-rule"
    if (oracle[0], nx[0]) == ("INVALID", "VALID"):
        if oracle[1].split(";", 1)[-1].strip() == "Egg:EggMoveCombination" and native.certifies(record, context):
            return "deliberate", "constructive-original-game-parent-ancestry"
    return original


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    for name in ("corpus", "pinned", "current", "pokebank", "proofs"):
        ap.add_argument(name, type=Path)
    for name in ("pkhex", "pokered", "pokecrystal"):
        ap.add_argument("--" + name, required=True, type=Path)
    args = ap.parse_args()
    ctx = compare.Context.load(args.pkhex, args.pokered)
    records = compare.load_corpus(args.corpus, ctx)
    pinned = compare.load_verdicts(args.pinned, ("VALID", "INVALID"), records, "pinned oracle")
    current = compare.load_verdicts(args.current, ("VALID", "INVALID"), records, "current oracle")
    nx = compare.load_verdicts(args.pokebank, ("VALID", "INVALID", "UNCERTAIN"), records, "NX")
    proof = {}
    for line in args.proofs.read_text().splitlines():
        rid = line.split()[0]
        if rid in proof or rid not in records:
            raise compare.ProtocolError("duplicate or unknown proof id " + rid)
        proof[rid] = line
    native = NativeChain(args.pokered, args.pokecrystal)
    failures = []
    for label, oracle in (("pinned", pinned), ("current", current)):
        raw, audited = collections.Counter(), collections.Counter()
        for rid, record in records.items():
            raw[oracle[rid][0] + "/" + nx[rid][0]] += 1
            verdict, reason = classify(record, oracle[rid], nx[rid], current[rid], ctx, native, proof.get(rid, ""))
            audited[verdict + (":" + reason if reason else "")] += 1
            if verdict == "differing":
                failures.append((label, rid, oracle[rid], nx[rid]))
        print(label, "raw", dict(raw), "accounted", sum(raw.values()))
        print(label, "audited", dict(audited))
    for failure in failures[:25]:
        print("FAIL", failure)
    print("unexplained", len(failures))
    return bool(failures)


if __name__ == "__main__":
    raise SystemExit(main())
