#!/usr/bin/env python3
"""Summarize a joined PKHeX / PokeBank NX Gen III-IV run.

Input lines (tab-joined): <oracle line>\t<driver line>
  oracle: <id> <cart> <hex> VALID|INVALID <enc>:<species> ; <code> ...
  driver: <id> INVALID|NOPROBLEMS|INCOMPLETE|LOADFAIL <coverage> ; <issue> | ... | W: <warning> | ...
Record ids: b* base encounter, m*-<mutation> its mutations; grown corpus: g* grown Gen IV
record, h*-<mutation> its mutations, p* Pal Park transfer, q*-<mutation> its mutations.
Prints verdict tables and catch/certification metrics per game and the top discrepancy
categories. Exit status 1 when there is any false Invalid (PKHeX VALID, ours INVALID),
false Valid (PKHeX INVALID, ours NOPROBLEMS) or LOADFAIL.
"""
import re
import sys
from collections import Counter, defaultdict

SHOW = int(sys.argv[2]) if len(sys.argv) > 2 else 12
GAMES = ["R", "S", "E", "FR", "LG", "D", "P", "Pt", "HG", "SS"]
VERDICTS = ["INVALID", "INCOMPLETE", "NOPROBLEMS", "LOADFAIL"]


def norm(text):
    """Collapse numbers/names so issue texts group into categories."""
    text = re.sub(r"0x[0-9A-Fa-f]+", "#", text)
    text = re.sub(r"\d+", "#", text)
    text = re.sub(r"'[^']*'", "'*'", text)
    text = re.sub(r'"[^"]*"', '"*"', text)
    return text.strip()


def mutation(rid):
    if "-" not in rid:
        return {"b": "base", "g": "grown", "p": "palpark"}.get(rid[0], rid[0])
    return rid.split("-", 1)[1]


rows = []
with open(sys.argv[1]) as f:
    for line in f:
        left, right = line.rstrip("\n").split("\t")
        lhead, _, lcodes = left.partition(" ; ")
        rid, cart, hexdata, pverdict, enc = lhead.split(" ")[:5]
        rhead, _, rissues = right.partition(" ;")
        rparts = rhead.split(" ")
        if rparts[0] != rid:
            sys.exit(f"id mismatch {rid} {rparts[0]}")
        allissues = [i.strip() for i in rissues.split(" | ") if i.strip()]
        rows.append(dict(id=rid, cart=cart, hex=hexdata, p=pverdict, enc=enc, codes=lcodes.strip(),
                         ours=rparts[1], cov=rparts[2] if len(rparts) > 2 else "-",
                         issues=[i for i in allissues if not i.startswith("W: ")],
                         warnings=[i[3:] for i in allissues if i.startswith("W: ")]))

print(f"{len(rows)} records")
SUBSETS = [(lambda r: True, "all records")]
for prefix, label in (("b", "base encounters only"), ("g", "grown records only"), ("p", "Pal Park transfers only")):
    if any(r["id"].startswith(prefix) and "-" not in r["id"] for r in rows):
        SUBSETS.append((lambda r, x=prefix: r["id"].startswith(x) and "-" not in r["id"], label))
for subset, label in SUBSETS:
    print(f"\n== PKHeX verdict x PokeBank NX verdict per game ({label}) ==")
    print(f"{'game':5} {'PKHeX':8} " + " ".join(f"{v:>10}" for v in VERDICTS))
    table = defaultdict(Counter)
    for r in rows:
        if subset(r):
            table[(r["cart"], r["p"])][r["ours"]] += 1
    tot = defaultdict(Counter)
    for g in GAMES:
        for p in ("VALID", "INVALID"):
            c = table.get((g, p))
            if not c:
                continue
            tot[p].update(c)
            print(f"{g:5} {p:8} " + " ".join(f"{c[v]:>10}" for v in VERDICTS))
    for p in ("VALID", "INVALID"):
        print(f"{'ALL':5} {p:8} " + " ".join(f"{tot[p][v]:>10}" for v in VERDICTS))


def pct(n, d):
    return f"{100.0 * n / d:6.2f}%" if d else "     -"


print("\n== Per-game rates (PKHeX-invalid caught as Invalid; PKHeX-valid with No problems found) ==")
print(f"{'game':5} {'falseInvalid':>12} {'falseValid':>10} {'caught':>15} {'rate':>7} {'noProblems':>15} {'rate':>7}")
for g in GAMES + ["ALL"]:
    sel = [r for r in rows if g == "ALL" or r["cart"] == g]
    if not sel:
        continue
    valid = [r for r in sel if r["p"] == "VALID"]
    invalid = [r for r in sel if r["p"] == "INVALID"]
    fi = sum(r["ours"] == "INVALID" for r in valid)
    fv = sum(r["ours"] == "NOPROBLEMS" for r in invalid)
    caught = sum(r["ours"] == "INVALID" for r in invalid)
    clean = sum(r["ours"] == "NOPROBLEMS" for r in valid)
    print(f"{g:5} {fi:>12} {fv:>10} {f'{caught}/{len(invalid)}':>15} {pct(caught, len(invalid)):>7} "
          f"{f'{clean}/{len(valid)}':>15} {pct(clean, len(valid)):>7}")
print("\n== Metrics per game (all records) ==")
print("(clean = PKHeX VALID with no Fishy result; PKHeX's Fishy is our Warning, which blocks certification)")
print(f"{'game':5} {'PKHeX-valid':>11} {'certified':>10} {'cert%':>7} {'clean':>7} {'cert-clean%':>11} "
      f"{'PKHeX-inv':>10} {'caught':>8} {'catch%':>7} {'falseInv':>8} {'falseVal':>8}")
metric_total = Counter()
for g in GAMES + ["ALL"]:
    rs = [r for r in rows if g == "ALL" or r["cart"] == g]
    if not rs:
        continue
    clean = [r for r in rs if r["p"] == "VALID" and "~" not in r["codes"]]
    m = Counter(valid=sum(r["p"] == "VALID" for r in rs),
                certified=sum(r["p"] == "VALID" and r["ours"] == "NOPROBLEMS" for r in rs),
                clean=len(clean), clean_certified=sum(r["ours"] == "NOPROBLEMS" for r in clean),
                invalid=sum(r["p"] == "INVALID" for r in rs),
                caught=sum(r["p"] == "INVALID" and r["ours"] == "INVALID" for r in rs),
                false_invalid=sum(r["p"] == "VALID" and r["ours"] == "INVALID" for r in rs),
                false_valid=sum(r["p"] == "INVALID" and r["ours"] == "NOPROBLEMS" for r in rs))
    print(f"{g:5} {m['valid']:>11} {m['certified']:>10} {pct(m['certified'], m['valid']):>7} {m['clean']:>7} "
          f"{pct(m['clean_certified'], m['clean']):>11} {m['invalid']:>10} "
          f"{m['caught']:>8} {pct(m['caught'], m['invalid']):>7} {m['false_invalid']:>8} {m['false_valid']:>8}")


def report(title, selected, keyfn):
    print(f"\n== {title}: {len(selected)} records ==")
    cats = defaultdict(list)
    for r in selected:
        for k in keyfn(r):
            cats[k].append(r)
    for k, rs in sorted(cats.items(), key=lambda kv: -len(kv[1]))[:SHOW]:
        ex = rs[0]
        games = Counter(r["cart"] for r in rs)
        muts = Counter(mutation(r["id"]) for r in rs)
        print(f"\n[{len(rs)}] {k}")
        print(f"   games: {dict(games.most_common(10))}")
        print(f"   mutations: {dict(muts.most_common(6))}")
        print(f"   e.g. {ex['id']} {ex['cart']} {ex['enc']} PKHeX={ex['p']} codes=[{ex['codes']}]")
        print(f"   ours: {' | '.join(ex['issues']) or '(no Invalid issue)'}  coverage={ex['cov']}")
        if ex["warnings"]:
            print(f"   warnings: {' | '.join(ex['warnings'])}")
        print(f"   hex: {ex['hex']}")


false_invalid = [r for r in rows if r["p"] == "VALID" and r["ours"] == "INVALID"]
report("FALSE INVALID (PKHeX VALID, ours Invalid) by our issue", false_invalid,
       lambda r: sorted({norm(i) for i in r["issues"]}))
false_valid = [r for r in rows if r["p"] == "INVALID" and r["ours"] == "NOPROBLEMS"]
report("FALSE VALID (PKHeX INVALID, ours NoProblemsFound) by PKHeX code", false_valid,
       lambda r: sorted({c.split(":")[0] + ":" + c.split(":")[1] for c in r["codes"].split() if not c.startswith("~")}
                        or {"(none)"}))
incomplete_inv = [r for r in rows if r["p"] == "INVALID" and r["ours"] == "INCOMPLETE"]
report("PKHeX INVALID, ours Incomplete (acceptable) by PKHeX code", incomplete_inv,
       lambda r: sorted({c for c in r["codes"].split() if not c.startswith("~")} or {"(none)"}))
valid_incomplete = Counter(r["cov"] for r in rows if r["p"] == "VALID" and r["ours"] == "INCOMPLETE")
print("\n== PKHeX VALID, ours Incomplete: non-Complete coverage fields ==")
for k, n in valid_incomplete.most_common(SHOW):
    print(f"  {n:6} {k}")
report("PKHeX VALID, ours Incomplete (not certified) by our warning",
       [r for r in rows if r["p"] == "VALID" and r["ours"] == "INCOMPLETE"],
       lambda r: sorted({norm(w) for w in r["warnings"]} or {"(no warning)"}))
loadfail = [r for r in rows if r["ours"] == "LOADFAIL"]
if loadfail:
    report("LOADFAIL (C++ side could not load)", loadfail, lambda r: [norm(" ".join(r["issues"]))])
sys.exit(1 if false_invalid or false_valid or loadfail else 0)
