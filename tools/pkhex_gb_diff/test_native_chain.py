#!/usr/bin/env python3
"""Independent native construction and differential rejection controls."""
import argparse
from pathlib import Path
import compare
from compare_dual import CHAIN, HISTORY, classify
from native_chain import NativeChain

ap = argparse.ArgumentParser()
for name in ("pkhex", "pokered", "pokecrystal"):
    ap.add_argument("--" + name, required=True, type=Path)
args = ap.parse_args()
ctx = compare.Context.load(args.pkhex, args.pokered)
native = NativeChain(args.pokered, args.pokecrystal)
fixture = Path(__file__).resolve().parents[2] / "tests/fixtures/gen15_session2/breeding12.txt"
cases = compare.load_corpus(fixture, ctx)
for rid, record in cases.items():
    assert native.certifies(record, ctx) == (rid in ("9615", "9744")), rid

# A newer upstream rejection cannot override a constructible original-game path.
upstream = ("INVALID", "EncounterEgg2 ; Egg:EggMoveCombination")
old = ("VALID", "EncounterEgg2 ; ")
bad = ("INVALID", "egg ; | " + CHAIN)
assert classify(cases["9615"], old, bad, upstream, ctx, native, "")[0] == "differing"
assert classify(cases["7517"], old, bad, upstream, ctx, native, "")[0] == "deliberate"
# A different upstream complaint cannot be used as a chain-rule exception.
assert classify(cases["7517"], old, bad, ("INVALID", "Encounter ; Encounter:EncInvalid"), ctx, native, "")[0] == "differing"
# Missing historical evidence must be tied to an actual selected gift witness.
unknown = ("UNCERTAIN", "egg ; | " + HISTORY)
assert classify(cases["7527"], old, unknown, upstream, ctx, native, "")[0] == "differing"
# Clean differences require the exact chain complaint, not a substring in other failures.
clean = ("VALID", "egg ; ")
assert classify(cases["9615"], upstream, clean, upstream, ctx, native, "")[0] == "deliberate"
mixed = ("INVALID", "EncounterEgg2 ; Egg:EggMoveCombination Trainer:G1CharOT")
assert classify(cases["9615"], mixed, clean, mixed, ctx, native, "")[0] == "differing"
print("Native pret-only construction: 12 retained cases; positive chronology, false-Invalid, unrelated-complaint and missing-witness rejection controls PASS")
