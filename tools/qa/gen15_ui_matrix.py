#!/usr/bin/env python3
"""Generate docs/qa/GEN1_5_UI_AUDIT_MATRIX.md: the 20-game Gen I-V audit matrix (Thread 4 QA).

Everything is derived from the repository, so re-running after each lane lands refreshes it:
  * per-field editor access per generation, parsed from fieldAccessForGeneration() in
    include/UI/SharedPokemonEditorContract.h (the access the shared editor declares);
  * cross-generation UI contract results per generation (tests/test_qa_cross_gen_contract.py);
  * host functional results per version from the acceptance matrix generated output
    (build-host/qa-*/GEN1_5_MATRIX.generated.md, else the copy embedded in docs/qa/GEN1_5_MATRIX.md).

Usage: gen15_ui_matrix.py [--root <repo>] [--out <file>] [--check]
  --check   exit 1 if the committed matrix differs from what the source now produces
"""
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tests"))
import test_qa_cross_gen_contract as cg  # noqa: E402

VERSIONS = [
    (1, "red"), (1, "blue"), (1, "yellow"),
    (2, "gold"), (2, "silver"), (2, "crystal"),
    (3, "ruby"), (3, "sapphire"), (3, "emerald"), (3, "firered"), (3, "leafgreen"),
    (4, "diamond"), (4, "pearl"), (4, "platinum"), (4, "heartgold"), (4, "soulsilver"),
    (5, "black"), (5, "white"), (5, "black2"), (5, "white2"),
]
TITLES = {"red": "Red", "blue": "Blue", "yellow": "Yellow", "gold": "Gold", "silver": "Silver",
          "crystal": "Crystal", "ruby": "Ruby", "sapphire": "Sapphire", "emerald": "Emerald",
          "firered": "FireRed", "leafgreen": "LeafGreen", "diamond": "Diamond", "pearl": "Pearl",
          "platinum": "Platinum", "heartgold": "HeartGold", "soulsilver": "SoulSilver",
          "black": "Black", "white": "White", "black2": "Black 2", "white2": "White 2"}

# Audit areas -> (acceptance-matrix cells, UI contracts that bear on them).
AREAS = [
    ("Home / Games / Instances", ["discovery", "linking", "provider", "identity", "home-refresh"], []),
    ("Party / Boxes / Trainer / Dex", ["party", "boxes", "trainer-name", "trainer-gender", "pokedex"], []),
    ("View / Create / Edit", ["navigation", "creation", "fields", "pickers"],
     ["CG01-no-make-shiny-quick-action", "CG01b-shiny-editing-still-present", "CG02-no-row-counters",
      "CG03-origin-legality-heading", "CG04-no-developer-wording", "CG05-picker-footer-helper",
      "CG06-reroll-without-confirmation", "CG10-single-navigation-repeat-engine"]),
    ("Bag Add / quantity / Remove / sort", ["bag"], ["CG09-item-quantity-policy"]),
    ("Clone / Move / Release", ["actions"], ["CG06-reroll-without-confirmation", "CG08-modal-footer-and-touch-layer"]),
    ("Review / Discard / Export Copy", ["working-copy", "review", "export"], ["CG08-modal-footer-and-touch-layer"]),
    ("Source-save immutability", ["immutability"], []),
    ("Launch / return", ["relaunch"], ["CG07-single-exit-and-launch-site"]),
]

FIELDS = [("Species", ["Species"]), ("Moves", ["Move", "MovePP", "MovePPUps"]), ("Nature", ["Nature"]),
          ("Gender", ["Gender"]), ("Shiny", ["Shiny"]), ("Ability", ["Ability"]), ("Pokérus", ["Pokerus"]),
          ("Language", ["Language"]), ("Met data", ["MetLevel", "MetLocation"]),
          ("Held item", ["HeldItem"]), ("Ball", ["Ball"]), ("Form", ["Form"]),
          ("DV / IV / EV", ["DV", "IV", "EV", "StatExperience"])]
SHORT = {"Editable": "Edit", "Derived": "Derived", "ReadOnly": "Read-only", "Hidden": "—"}


def parse_field_access(root: Path) -> dict[int, dict[str, str]]:
    text = (root / "include/UI/SharedPokemonEditorContract.h").read_text(encoding="utf-8")
    start = text.index("fieldAccessForGeneration(")
    body = text[start:text.index("\n}\n", start)]
    out: dict[int, dict[str, str]] = {}
    blocks = re.split(r"if \(generation == Generation::Gen(\d)\)", body)
    for gen, block in zip(blocks[1::2], blocks[2::2]):
        access: dict[str, str] = {}
        pending: list[str] = []
        for tok in re.finditer(r"case FieldIdentity::(\w+):|return ([^;]+);|default:", block):
            if tok.group(1):
                pending.append(tok.group(1))
            elif tok.group(0) == "default:":
                pending.append("*")
            else:
                ret = tok.group(2).strip()
                m = re.fullmatch(r"FieldAccess::(\w+)", ret)
                if m:
                    value = m.group(1)
                else:
                    cond = re.match(r"(\w+)\s*\?\s*FieldAccess::(\w+)\s*:\s*FieldAccess::(\w+)", ret)
                    value = f"{cond.group(2)} if {cond.group(1)}, else {cond.group(3)}" if cond else ret
                for name in pending:
                    access.setdefault(name, value)
                pending = []
        out[int(gen)] = access
    return out


def field_cell(access: dict[str, str], names: list[str]) -> str:
    values = []
    for name in names:
        v = access.get(name, access.get("*", "Hidden"))
        v = SHORT.get(v, v.replace("Editable", "Edit").replace("ReadOnly", "Read-only").replace("Hidden", "—"))
        values.append(v if len(names) == 1 else f"{name} {v}")
    uniq = list(dict.fromkeys(values))
    return " / ".join(uniq)


def acceptance(root: Path) -> dict[tuple[str, str], str]:
    candidates = sorted(root.glob("build-host/qa-*/GEN1_5_MATRIX.generated.md")) + [root / "docs/qa/GEN1_5_MATRIX.md"]
    for path in candidates:
        if not path.is_file():
            continue
        lines = path.read_text(encoding="utf-8").splitlines()
        for i, line in enumerate(lines):
            if line.startswith("| Version |"):
                cols = [c.strip() for c in line.strip("|").split("|")]
                out = {}
                for row in lines[i + 2:]:
                    if not row.startswith("|"):
                        break
                    cells = [c.strip() for c in row.strip("|").split("|")]
                    for col, val in zip(cols[1:], cells[1:]):
                        out[(cells[0], col)] = val
                return out
    return {}


def gens_of(rel: str) -> set[int]:
    hit = {g for g, rx in cg.GEN_FILE.items() if rx.search(Path(rel).name)}
    return hit or {1, 2, 3, 4, 5}  # shared component: applies to every generation


def contract_by_gen(root: Path):
    sources, _ = cg.load_sources(root)
    results = {r.cid: r for r in cg.contracts(sources)}
    per: dict[str, dict[int, list[str]]] = {}
    for cid, res in results.items():
        per[cid] = {g: [] for g in range(1, 6)}
        for v in res.violations:
            rel = v.split(":", 1)[0]
            m = re.match(r"Gen (\d):", v)
            targets = {int(m.group(1))} if m else (gens_of(rel) if "/" in rel else {1, 2, 3, 4, 5})
            for g in targets:
                per[cid][g].append(v)
    return results, per


def render(root: Path) -> str:
    access = parse_field_access(root)
    acc = acceptance(root)
    results, per = contract_by_gen(root)
    lines = [
        "# Gen I–V 20-game audit matrix (Thread 4)",
        "",
        "Generated by `python3 tools/qa/gen15_ui_matrix.py` from the repository (do not edit by hand; re-run",
        "after each lane lands). Sources: the host acceptance matrix (`make qa-matrix`, real production paths",
        "on synthetic saves), the cross-generation UI contracts (`tests/test_qa_cross_gen_contract.py`) and the",
        "per-field access the shared editor declares (`fieldAccessForGeneration`).",
        "",
        "Legend: **✅** host-verified and no open UI contract for that generation · **⚠ CGnn** host-verified",
        "but the named UI contract is still violated (see §3) · **❌** host failure · **N/A** the format has",
        "no such data. Every cell is host evidence only: on-device behaviour (input, rendering, touch,",
        "launch/return) is **NOT DEVICE TESTED** until the owner tests the exact candidate NRO.",
        "",
        "## 1. Games × areas",
        "",
        "| Game | " + " | ".join(a for a, _, _ in AREAS) + " |",
        "| --- |" + " --- |" * len(AREAS),
    ]
    for gen, ver in VERSIONS:
        row = [f"{TITLES[ver]} (Gen {gen})"]
        for _, cells, cids in AREAS:
            states = [acc.get((ver, c), "?") for c in cells]
            if any(s == "FAIL" for s in states):
                cell = "❌"
            elif all(s == "N/A" for s in states):
                cell = "N/A"
            elif any(s in ("?", "MISSING") for s in states):
                cell = "?"
            else:
                cell = "✅"
            open_cids = [cid.split("-")[0] for cid in cids if per.get(cid, {}).get(gen)]
            if open_cids and cell == "✅":
                cell = "⚠ " + ", ".join(dict.fromkeys(open_cids))
            row.append(cell)
        lines.append("| " + " | ".join(row) + " |")
    lines += [
        "",
        "## 2. Editor fields × generation (declared access)",
        "",
        "Edit = editable in the shared editor · Derived = computed from other fields · Read-only = shown,",
        "not editable · — = not shown. Moves and PP are edited in the move panel even where the field",
        "contract lists them as not shown; differences between generations are flagged for the owners.",
        "",
        "| Field | Gen I (R/B/Y) | Gen II (G/S/C) | Gen III (R/S/E/FR/LG) | Gen IV (D/P/Pt/HG/SS) | Gen V (B/W/B2/W2) |",
        "| --- | --- | --- | --- | --- | --- |",
    ]
    for label, names in FIELDS:
        lines.append(f"| {label} | " + " | ".join(field_cell(access.get(g, {}), names) for g in range(1, 6)) + " |")
    lines += [
        "",
        "## 3. Cross-generation UI contracts by generation",
        "",
        "| Contract | State | Gen I | Gen II | Gen III | Gen IV | Gen V |",
        "| --- | --- | --- | --- | --- | --- | --- |",
    ]
    for cid, res in results.items():
        lane = cg.XFAIL.get(cid)
        state = ("XFAIL until " + lane) if (lane and res.violations) else ("XPASS" if lane else ("PASS" if not res.violations else "FAIL"))
        counts = [str(len(per[cid][g])) if per[cid][g] else "0" for g in range(1, 6)]
        lines.append(f"| {cid}: {res.title} | {state} | " + " | ".join(counts) + " |")
    lines += [
        "",
        "Counts are violations attributed to each generation; a violation in a shared component (no",
        "generation in its file name) counts for every generation. Full lists:",
        "`python3 tests/test_qa_cross_gen_contract.py --list`.",
        "",
        "## 4. Hardware-only (cannot be settled on the host)",
        "",
        "Controller/touch parity, scroll repeat feel, picker and modal rendering, the on-device leave",
        "dialog, SD-card export semantics and the emulator launch/return round trip. These are checked only",
        "by the owner's hardware round on the exact candidate NRO (exact-binary audit: QA-F9 workflow).",
        "",
    ]
    return "\n".join(lines)


def main(argv: list[str]) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--root", type=Path, default=ROOT)
    ap.add_argument("--out", type=Path, default=None)
    ap.add_argument("--check", action="store_true")
    args = ap.parse_args(argv[1:])
    out = args.out or (args.root / "docs/qa/GEN1_5_UI_AUDIT_MATRIX.md")
    text = render(args.root)
    if args.check:
        current = out.read_text(encoding="utf-8") if out.is_file() else ""
        if current != text:
            print(f"{out} is out of date: re-run tools/qa/gen15_ui_matrix.py")
            return 1
        print(f"{out} is current")
        return 0
    out.write_text(text, encoding="utf-8")
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
