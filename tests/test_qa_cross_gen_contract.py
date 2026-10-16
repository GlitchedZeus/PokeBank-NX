#!/usr/bin/env python3
"""Gen I-V cross-generation regression contracts (Thread 4 QA, owner campaign after #320).

Static contracts over the REAL production source: every compiled .cpp/.h plus every .inc reachable
from them through #include (an unreachable .inc is dead code and is listed, not checked). String
literals and code are read with comments stripped.

Each contract is either ENFORCED (a violation fails now) or XFAIL-until-landed: it is pinned to the
lane that owns the fix, and a violation is reported loudly but does not fail the default gate.
In --candidate mode (release-candidate audit) EVERY violation fails, XFAIL or not. An XFAIL contract
that no longer finds violations prints XPASS: drop its marker so it is enforced from then on.

Usage:
  test_qa_cross_gen_contract.py [--candidate] [--root <repo>] [--list]
Exit status: 0 when nothing fails in the chosen mode, 1 otherwise.
"""
from __future__ import annotations

import argparse
import re
import sys
from dataclasses import dataclass, field
from pathlib import Path

DEFAULT_ROOT = Path(__file__).resolve().parent.parent

# Contract id -> owning lane, while the fix is not yet on the integration branch.
# Remove an entry when its contract XPASSes; it is then ENFORCED.
XFAIL = {
    "CG01-no-make-shiny-quick-action": "S (Gen III-V) + C (Gen I/II)",
    "CG02-no-row-counters": "S (shared shell) + C (Gen I)",
    "CG03-origin-legality-heading": "S + C",
    "CG04-no-developer-wording": "S + C (+ Names for '(RSE)')",
    "CG05-picker-footer-helper": "S (SharedEditorChrome drawPickerFooter adoption)",
    "CG08-modal-footer-and-touch-layer": "S + C (editor modals); 2R (Home, done)",
    "CG09-item-quantity-policy": "I + C (QuantityPolicy adoption)",
}

# Gen VI+-only code (no Gen VI-IX work this campaign): hits here are listed as informational
# "out of scope" lines and never count as violations, in gate or candidate mode.
OUT_OF_SCOPE = {
    "include/UI/ActionSheetModel.h": "generic TrainerViewScreen / MutationTargetPolicy sheet for the modern Gen VI+ screens (coordinator, 2026-10-16)",
    "src/UI/Panels/StoragePanel.cpp": "modern storage panel, reached only from TrainerViewScreenBase.inc (Gen VI+)",
    "src/UI/Dialogs/StatEditDialog.cpp": "modern stat dialog, reached only from TrainerViewScreenBase.inc (Gen VI+)",
    "src/UI/Dialogs/ItemEditDialog.cpp": "modern item dialog, reached only from TrainerViewScreenBase.inc (Gen VI+)",
    "src/UI/Dialogs/SaveConfirmDialog.cpp": "modern save dialog, reached only from TrainerViewScreenBase.inc (Gen VI+)",
    "src/UI/Dialogs/PickerDialog.cpp": "modern picker dialog, reached only from TrainerViewScreenBase.inc (Gen VI+)",
}

# Names the lanes are introducing. Update when the lane lands if the final name differs.
# S: include/UI/SharedEditorPresentation.h pickerFooter(PickerFooterSpec) / confirmFooter and
# include/UI/SharedEditorChrome.h drawPickerFooter / drawFooter / beginModal / drawOriginLegalityHeading.
PICKER_FOOTER_HELPER_FILE = re.compile(r"include/UI/SharedEditor(Presentation|Chrome)\.h$")
# I: include/Inventory/ItemQuantityPolicy.h, PokeVault::Inventory::QuantityPolicy::quantityRule / mayAddItem;
# C keeps one classicItemQuantity entry that delegates to it.
QUANTITY_POLICY_SYMBOL = "QuantityPolicy"
QUANTITY_POLICY_USE = re.compile(r"\bQuantityPolicy\b|\bQuantityPolicy::quantityRule\s*\(|\bmayAddItem\s*\(")
# Per-format wrappers are accepted only where their own definition calls QuantityPolicy.
QUANTITY_POLICY_WRAPPERS = ("gen4QuantityRule", "gen5QuantityRule", "classicItemQuantity", "quantityRule")  # quantityRule: ClassicInventoryCatalog
QUANTITY_POLICY_HEADER = "include/Inventory/ItemQuantityPolicy.h"
REPEAT_ENGINE_FILE = "include/UI/NavigationRepeat.h"

GEN_FILE = {
    1: re.compile(r"(Gen1|Gen12|RBY)", re.I),
    2: re.compile(r"(Gen2|Gen12|GSC)", re.I),
    3: re.compile(r"Gen3"),
    4: re.compile(r"Gen4"),
    5: re.compile(r"Gen5"),
}


@dataclass
class Source:
    path: Path
    rel: str
    code: str                       # comments blanked, string literals kept
    literals: list[tuple[int, str]]  # (line, decoded-ish literal text)


def lex(text: str) -> tuple[str, list[tuple[int, str]]]:
    """Blank comments (keeping newlines) and collect string literals with their line numbers."""
    out = []
    lits: list[tuple[int, str]] = []
    i, n, line = 0, len(text), 1
    while i < n:
        c = text[i]
        if c == "\n":
            line += 1
            out.append(c)
            i += 1
        elif text.startswith("//", i):
            j = text.find("\n", i)
            j = n if j < 0 else j
            out.append(" " * (j - i))
            i = j
        elif text.startswith("/*", i):
            j = text.find("*/", i + 2)
            j = n if j < 0 else j + 2
            chunk = text[i:j]
            out.append(re.sub(r"[^\n]", " ", chunk))
            line += chunk.count("\n")
            i = j
        elif c == 'R' and text.startswith('R"', i) and (i == 0 or not (text[i - 1].isalnum() or text[i - 1] == "_")):
            m = re.match(r'R"([^(\s]*)\(', text[i:])
            if not m:
                out.append(c)
                i += 1
                continue
            end = text.find(")" + m.group(1) + '"', i)
            end = n if end < 0 else end + len(m.group(1)) + 2
            chunk = text[i:end]
            lits.append((line, chunk[len(m.group(0)):-(len(m.group(1)) + 2)]))
            out.append(chunk)
            line += chunk.count("\n")
            i = end
        elif c == '"':
            j = i + 1
            buf = []
            while j < n and text[j] != '"':
                if text[j] == "\\" and j + 1 < n:
                    buf.append(text[j:j + 2])
                    j += 2
                    continue
                if text[j] == "\n":
                    break
                buf.append(text[j])
                j += 1
            lits.append((line, "".join(buf)))
            out.append(text[i:j + 1])
            i = j + 1
        elif c == "'":
            j = i + 1
            while j < n and text[j] != "'" and text[j] != "\n":
                j += 2 if text[j] == "\\" else 1
            out.append(text[i:j + 1])
            i = j + 1
        else:
            out.append(c)
            i += 1
    return "".join(out), lits


def load_sources(root: Path) -> tuple[list[Source], list[str]]:
    roots = [p for p in (root / "src").rglob("*") if p.suffix in (".cpp", ".h")]
    roots += [p for p in (root / "include").rglob("*") if p.suffix in (".h", ".hpp")]
    seen: dict[Path, Source] = {}
    queue = list(roots)
    include_re = re.compile(r'^\s*#\s*include\s+"([^"]+)"', re.M)
    while queue:
        path = queue.pop()
        path = path.resolve()
        if path in seen or not path.is_file():
            continue
        text = path.read_text(encoding="utf-8", errors="replace")
        code, lits = lex(text)
        seen[path] = Source(path, path.relative_to(root.resolve()).as_posix(), code, lits)
        for inc in include_re.findall(code):
            if not inc.endswith(".inc"):
                continue
            for base in (path.parent, root / "src" / "UI", root / "src", root / "include"):
                cand = (base / inc)
                if cand.is_file():
                    queue.append(cand)
                    break
    all_inc = {p.resolve() for p in (root / "src").rglob("*.inc")} | {p.resolve() for p in (root / "include").rglob("*.inc")}
    dead = sorted(p.relative_to(root.resolve()).as_posix() for p in all_inc - set(seen))
    return list(seen.values()), dead


def ui(sources: list[Source]) -> list[Source]:
    return [s for s in sources if s.rel.startswith(("src/UI/", "include/UI/"))]


def visible(sources: list[Source]) -> list[Source]:
    return [s for s in sources if s.rel.startswith(("src/UI/", "include/UI/", "src/Names/"))]


def dedupe(items):
    return list(dict.fromkeys(items))


def lit_hits(sources, pattern, flags=0):
    rx = re.compile(pattern, flags)
    return dedupe(f"{s.rel}:{ln}: \"{lit}\"" for s in sources for ln, lit in s.literals if rx.search(lit))


def code_hits(sources, pattern, flags=0, exclude=()):
    rx = re.compile(pattern, flags)
    out = []
    for s in sources:
        if s.rel in exclude:
            continue
        for m in rx.finditer(s.code):
            ln = s.code.count("\n", 0, m.start()) + 1
            out.append(f"{s.rel}:{ln}: {s.code.splitlines()[ln - 1].strip()[:140]}")
    return dedupe(out)


def function_bodies(code: str, name_rx: re.Pattern):
    """Yield (name, start_line, body) for C++ function definitions whose name matches."""
    for m in re.finditer(r"\b([A-Za-z_]\w*)\s*\([^;{}()]*(?:\([^;{}()]*\)[^;{}()]*)*\)\s*(?:const\s*)?(?:noexcept\s*)?\{", code):
        name = m.group(1)
        if not name_rx.search(name) or name in ("if", "for", "while", "switch", "catch"):
            continue
        depth, j = 0, m.end() - 1
        while j < len(code):
            if code[j] == "{":
                depth += 1
            elif code[j] == "}":
                depth -= 1
                if depth == 0:
                    break
            j += 1
        yield name, code.count("\n", 0, m.start()) + 1, code[m.end():j]


@dataclass
class Result:
    cid: str
    title: str
    violations: list[str] = field(default_factory=list)
    out_of_scope: list[str] = field(default_factory=list)


def contracts(sources: list[Source]) -> list[Result]:
    U, V = ui(sources), visible(sources)
    res: list[Result] = []

    r = Result("CG01-no-make-shiny-quick-action",
               'No "Make Shiny" / "Make Non-Shiny" quick action in any Gen I-V action sheet')
    r.violations = lit_hits(U, r"Make\s+(Non-?)?Shiny", re.I)
    res.append(r)

    r = Result("CG01b-shiny-editing-still-present",
               "Shiny remains editable in every generation's editor (native entry point referenced by the UI)")
    # Each generation's own shiny edit entry point, searched only in that generation's UI files.
    evidence = {
        1: r"Gen1Shiny\.h|nearestShinyAttack|attackIsShinyCompatible",
        2: r"stageBoxPokemonShiny|\btoggleShiny\s*\(screen",   # Gen II DV re-derivation on the draft (e59df02)
        3: r"requestedShiny",
        4: r"setShiny\s*\(",
        5: r"Field::Shiny\b|Fields::toggleShiny\s*\(",
    }
    for gen, pat in evidence.items():
        gen_files = [s for s in U if GEN_FILE[gen].search(Path(s.rel).name)]
        if not any(re.search(pat, s.code) for s in gen_files):
            r.violations.append(f"Gen {gen}: no UI reference to its shiny edit entry point ({pat})")
    res.append(r)

    r = Result("CG02-no-row-counters", 'No "Rows N-M/T" or "Row x/y" counters drawn in editors')
    r.violations = lit_hits(U, r"^\s*Rows?\s+\d") + code_hits(U, r'"Rows?\s+"\s*\+\s*std::to_string')
    res.append(r)

    r = Result("CG03-origin-legality-heading",
               'Right-panel heading is exactly "Origin & Legality" in all 5 gens; no "Encounter legality" / "Legality & Provenance"')
    r.violations = lit_hits(V, r"Encounter legality|Legality\s*&\s*Provenance", re.I)
    if not lit_hits(U, r"^Origin & Legality$"):
        r.violations.append('no "Origin & Legality" heading literal in the UI')
    res.append(r)

    r = Result("CG04-no-developer-wording",
               'No "(DV-derived)", "commits to draft"-style wording, "(RSE)" or "(ID nn)"/raw-ID suffixes in user-visible strings')
    r.violations = (lit_hits(V, r"\(DV-derived\)|commits? to draft", re.I)
                    + lit_hits(V, r"\((RSE|FRLG|DPPt|HGSS|BW2?)\)")
                    + lit_hits(V, r"\(ID \d+\)")
                    + code_hits(V, r'"[^"\n]*\(?\bID\s*"\s*\+\s*std::to_string'))
    res.append(r)

    r = Result("CG05-picker-footer-helper",
               "Every picker footer goes through the shared green-A Select / red-B Cancel helper (no hand-rolled pairs)")
    # A picker footer is any footer offering an A-Select/Choose/Pick action.
    pair = r'\{\s*"A"\s*,\s*"(Select|Choose|Pick)"\s*\}'
    r.violations = [h for h in code_hits(U, pair) if not PICKER_FOOTER_HELPER_FILE.search(h.split(":", 1)[0])]
    if not any(s.rel == "include/UI/SharedEditorChrome.h" and re.search(r"\bdrawPickerFooter\s*\(", s.code) for s in U):
        r.violations.append("include/UI/SharedEditorChrome.h: shared drawPickerFooter helper missing")
    res.append(r)

    r = Result("CG06-reroll-without-confirmation",
               "No confirmation modal on draft-only DV/IV/EV reroll; Release / Delete / Discard confirmations still exist")
    r.violations = code_hits(U, r"\breroll\s*\.\s*confirming\b|\w*Reroll\w*Confirm\w*|Reroll::Outcome::AskConfirm")
    for word in ("Release", "Discard"):
        if not lit_hits(U, rf"\b{word}\b[^\"]*\?"):
            r.violations.append(f'no "{word} ...?" confirmation prompt left in the UI (must still exist)')
    res.append(r)

    r = Result("CG07-single-exit-and-launch-site",
               "appExitRequested=true and requestGameLaunch each have exactly one call site, behind the Y-confirmed leave dialog")
    exits = code_hits(U, r"\bappExitRequested\s*=\s*true\b")
    launches = [h for h in code_hits(sources, r"\brequestGameLaunch\s*\(")
                if not re.search(r"\bbool\s+requestGameLaunch\s*\(", h)]
    for label, hits in (("appExitRequested = true", exits), ("requestGameLaunch(", launches)):
        if len(hits) != 1:
            r.violations.append(f"{label}: {len(hits)} call sites (expected exactly 1): " + "; ".join(hits))
    # The Y gate is the host-tested decision table HomeQuitConfirm.h (tests/test_home_input_guard.cpp):
    # both sites must sit in its Decision::Quit branch, and only Y may produce Decision::Quit.
    for hit in exits + launches:
        rel, ln = hit.split(":")[0], int(hit.split(":")[1])
        src = next(s for s in sources if s.rel == rel)
        window = "\n".join(src.code.splitlines()[max(0, ln - 40):ln])
        if not re.search(r"HomeQuit::decide\s*\(", window) or "Decision::Quit" not in window:
            r.violations.append(f"{rel}:{ln}: not inside the HomeQuit::decide(...) == Decision::Quit branch")
    gate = next((s for s in sources if s.rel == "include/UI/HomeQuitConfirm.h"), None)
    # HomeQuitConfirm.h is libnx-free: Y is its own kButtonY (libnx HidNpadButton_Y = BIT(3)).
    if gate is None or not re.search(r"kButtonY\s*=\s*1ull\s*<<\s*3", gate.code) or \
            not re.search(r"pressed\s*&\s*kButtonY", gate.code):
        r.violations.append("include/UI/HomeQuitConfirm.h: the leave decision does not key on Y (BIT(3))")
    res.append(r)

    r = Result("CG08-modal-footer-and-touch-layer",
               "Every modal draws its own footer, and the covered footer is not tappable while it is open")
    modal_rx = re.compile(r"^draw(?!\w*(Frame|Chrome|Surface|Footer|Shell|Backdrop))\w*(Confirm|Decision|Modal|Dialog|Prompt)\w*$")
    footer_rx = re.compile(r"\b(drawNavBar|drawFooter\w*|drawGlyphButton|drawControllerHints|drawPickerFooter\w*)\s*\(")
    for s in U:
        for name, ln, body in function_bodies(s.code, modal_rx):
            if not footer_rx.search(body):
                r.violations.append(f"{s.rel}:{ln}: {name}() draws no footer of its own")
            if not re.search(r"beginTopTouchLayer\s*\(|beginModal\s*\(|touchButtons\s*\.\s*clear\s*\(", body):
                r.violations.append(f"{s.rel}:{ln}: {name}() leaves the covered footer's touch targets live")
    res.append(r)

    r = Result("CG09-item-quantity-policy",
               "Item quantity editors consult ItemQuantityPolicy (unique items never get max > 1)")
    qty_rx = re.compile(r"\b(stageInventoryQuantity|stageSetQuantity|stageBagQuantity|setQuantity|stageQuantity)\s*\(")
    valid_wrappers = set()
    for src in sources:
        for name, _, body in function_bodies(src.code, re.compile("^(" + "|".join(QUANTITY_POLICY_WRAPPERS) + ")$")):
            if re.search(r"\bQuantityPolicy\b", body):
                valid_wrappers.add(name)
    wrapper_use = re.compile(r"\b(" + "|".join(sorted(valid_wrappers)) + r")\s*\(") if valid_wrappers else None
    for s in U:
        uses_policy = QUANTITY_POLICY_USE.search(s.code) or (wrapper_use and wrapper_use.search(s.code))
        if qty_rx.search(s.code) and not uses_policy:
            ln = s.code.count("\n", 0, qty_rx.search(s.code).start()) + 1
            r.violations.append(f"{s.rel}:{ln}: edits item quantities without {QUANTITY_POLICY_SYMBOL}")
    header = next((s for s in sources if s.rel == QUANTITY_POLICY_HEADER), None)
    if header is None or "namespace PokeVault::Inventory::QuantityPolicy" not in header.code:
        r.violations.append(f"{QUANTITY_POLICY_HEADER} does not define PokeVault::Inventory::QuantityPolicy")
    res.append(r)

    r = Result("CG10-single-navigation-repeat-engine",
               "Navigation repeat timings live only in the shared NavigationRepeat engine")
    r.violations = code_hits(
        U, r"\b(?:constexpr|const|static|int|unsigned|uint\d+_t|u64|u32)\b[^;\n]*\b\w*(?:REPEAT|Repeat|repeat)\w*(?:DELAY|INTERVAL|Delay|Interval|FRAMES|Frames|Ticks|Ms)\w*\s*=",
        exclude=(REPEAT_ENGINE_FILE,))
    res.append(r)
    for r in res:
        r.out_of_scope = [v for v in r.violations if v.split(":", 1)[0] in OUT_OF_SCOPE]
        r.violations = [v for v in r.violations if v.split(":", 1)[0] not in OUT_OF_SCOPE]
    return res


def main(argv: list[str]) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--candidate", action="store_true", help="release-candidate mode: XFAIL does not excuse violations")
    ap.add_argument("--root", type=Path, default=DEFAULT_ROOT)
    ap.add_argument("--list", action="store_true", help="print every violation (default: first 8 per contract)")
    args = ap.parse_args(argv[1:])
    sources, dead = load_sources(args.root)
    results = contracts(sources)
    failed = 0
    print(f"Gen I-V cross-generation contracts ({'CANDIDATE' if args.candidate else 'gate'} mode): "
          f"{len(sources)} active source files, {len(dead)} unreachable .inc ignored")
    for res in results:
        xfail = XFAIL.get(res.cid)
        if not res.violations:
            state = "XPASS (drop the XFAIL marker)" if xfail else "PASS"
        elif xfail and not args.candidate:
            state = f"XFAIL until {xfail}"
        else:
            state = "FAIL" + (f" (XFAIL lane: {xfail})" if xfail else "")
            failed += 1
        print(f"{state:<34} {res.cid}: {res.title} [{len(res.violations)} violation(s)]")
        for v in res.violations if args.list else res.violations[:8]:
            print(f"      {v}")
        if not args.list and len(res.violations) > 8:
            print(f"      ... {len(res.violations) - 8} more (use --list)")
        for v in res.out_of_scope:
            print(f"      out of scope (Gen VI+, informational): {v}")
    if dead:
        print("Unreachable .inc (not compiled, not checked): " + ", ".join(dead))
    print(f"{failed} contract(s) failing")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
