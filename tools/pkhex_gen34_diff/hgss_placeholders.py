#!/usr/bin/env python3
"""Original-source necessary-level proofs and explicit secondary release changes.

The old broad placeholder-membership allowance was unsafe: it excused simulated
false Invalids on six of seven retained positive controls. A discarded candidate
cannot establish that all real candidates fail. No production oracle is patched;
all raw verdicts/counts remain and reference-version changes are labelled separately.
"""
import json
import pathlib
import re
import sys
from collections import Counter

REPO = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0,str(REPO/'tools/legality_completion'))
from hgss_native_disagreement import necessary_level_proof
from current_release_replay import changed_in_release
def native(cart, hexdata, enc, issues):
    """An independently proven native level absence, or a measured release change.

    Merely fitting a discarded placeholder is not proof. In particular, valid
    ordinary/Safari slots can overlap its level range and must remain protected.
    """
    if not issues:return None
    proven=necessary_level_proof(hexdata,enc)
    if proven:return proven['basis']
    if changed_in_release(hexdata):return 'reference-version-change:6501-to261018'
    return None


def transferred(hexdata, enc, issues):
    """Lost origin fields cannot be reconstructed from an old placeholder row."""
    if not issues:return None
    proven=necessary_level_proof(hexdata,enc,transferred=True)
    if proven:return proven['basis']
    if changed_in_release(hexdata):return 'reference-version-change:6501-to261018'
    return None


def gen5(path):
    counts = Counter()
    for line in open(path):
        left, right = line.rstrip("\n").split("\t")
        a, b = left.split(" ; ")[0].split(" "), right.split(" ")
        if a[2] != "VALID" or b[1] != "INVALID":
            continue
        issues = [i.strip() for i in right.partition(" ;")[2].split(" | ") if i.strip() and not i.strip().startswith("W: ")]
        name = transferred(a[1], a[3], issues)
        if name:
            counts[name] += 1
        else:
            print(line, end="")
    for name, n in sorted(counts.items()):
        print(f"deliberately stricter ({name}): {n}", file=sys.stderr)


if __name__ == "__main__":
    if len(sys.argv) != 3 or sys.argv[1] != "gen5":
        sys.exit(__doc__)
    gen5(sys.argv[2])
