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
XFAIL: dict[str, str] = {
    # CG01-CG11 XPASSed on candidate 7fea1797 (app-qa-nro-audit run 38947904299) and are ENFORCED;
    # CG12 and CG14 XPASSed on #307 d104310 (batch 1) and are ENFORCED.
    # Round 2 contracts are XFAIL only until their lane's push is on #307.
    "CG15-shared-species-search": "P (shared chooser 2876ffe; Gen IV kSpeciesSearch flag removal)",
}

# Gen VI+-only code (no Gen VI-IX work this campaign): hits here are listed as informational
# "out of scope" lines and never count as violations, in gate or candidate mode.
OUT_OF_SCOPE: dict[str, str] = {
    # Empty until lane S proves a file unreachable from every Gen I-V path (coordinator, 2026-10-16:
    # TrainerViewScreenBase's dialogs, ItemsPanel/StoragePanel and ActionSheetModel stay IN scope;
    # e.g. the base builds a gen4ItemView footer, so Gen IV bags may render through it).
}

# CG12 (Lane I 52017cd1): one Gen I-V Add flow, include/UI/SharedBagAddFlow.h.
BAG_ADD_HEADER = "include/UI/SharedBagAddFlow.h"
BAG_ADD_SURFACES = ("src/UI/ClassicInventoryOverlay.cpp", "src/UI/Gen4SharedPokemonSurface.inc",
                    "include/Integration/Gen5/Gen5BagScreen.h")

# CG15 (Lane P 2876ffe): live species search in the one shared chooser.
SEARCH_HEADER = "include/UI/SharedSearchInput.h"
SPECIES_PICKER_HEADER = "include/UI/SharedSpeciesPicker.h"
SPECIES_CHOOSER_OWNERS = (("Gen I", "src/UI/Gen1PokemonEditorOverlayUXCleanup3.inc"),
                          ("Gen II", "src/UI/Gen2PokemonPickerOverlay.inc"),
                          ("Gen III", "src/UI/Gen3SharedPokemonSurface.inc"),
                          ("Gen IV/V", "src/UI/SharedPokemonEditorSurface.inc"))

# CG14 (Lane P 30b6ea4 / 0867c9b8): include/UI/SharedScrollbar.h is the one scrollbar; the legacy
# entry points below only delegate to it.
SCROLLBAR_HEADER = "include/UI/SharedScrollbar.h"
SCROLLBAR_WRAPPERS = (("include/UI/ScreenChrome.h", "drawScrollbar"),
                      ("include/UI/SharedPokemonShell.h", "drawVerticalScrollIndicator"))

OUT_OF_SCOPE_WHEN_CG11_HOLDS: dict[str, str] = {
    "include/UI/ActionSheetModel.h": "generic Gen VI+ action sheet; unreachable from Gen I-V while CG11 passes",
    # S e86741c: the router clears pickerActive every frame in a Gen I-V session and CG11 binds every
    # screen-level pickerActive = true open.
    "src/UI/Dialogs/PickerDialog.cpp": "generic Gen VI+ picker; unreachable from Gen I-V while CG11 passes",
    "src/UI/Modals/PokemonDetailsModal.cpp": "generic Gen VI+ details modal; unreachable from Gen I-V while CG11 passes",
}

# CG11 (S 647917e): genericUiAllowedForSession(screen, ClassicGenericUi::GenericUi kind) in
# TrainerViewScreenBase.inc wraps ClassicGenericUi::genericUiAllowed(classicSession(gameId), kind)
# (include/UI/ClassicGenericUiGuard.h). Openers of the generic action sheet and details editor are
# guarded; a per-frame reset in update() closes any generic picker / stat dialog in a Gen I-V session.
GENERIC_UI_GUARD = re.compile(r"\bgenericUiAllowedForSession\s*\(")
GENERIC_UI_GUARDED_OPEN = re.compile(r"\bactionSheet\s*\.\s*open\s*\(|\bdetails\s*\.\s*active\s*=\s*true\b")
GENERIC_UI_GUARD_HEADER = "include/UI/ClassicGenericUiGuard.h"

# Names the lanes are introducing. Update when the lane lands if the final name differs.
# S: include/UI/SharedEditorPresentation.h pickerFooter(PickerFooterSpec) / confirmFooter and
# include/UI/SharedEditorChrome.h drawPickerFooter / drawFooter / beginModal / drawOriginLegalityHeading.
PICKER_FOOTER_HELPER_FILE = re.compile(r"include/UI/SharedEditor(Presentation|Chrome)\.h$")
# I: include/Inventory/ItemQuantityPolicy.h, PokeVault::Inventory::QuantityPolicy::quantityRule / mayAddItem;
# C keeps one classicItemQuantity entry that delegates to it.
QUANTITY_POLICY_SYMBOL = "QuantityPolicy"
QUANTITY_POLICY_USE = re.compile(r"\bQuantityPolicy\b|\bQuantityPolicy::quantityRule\s*\(|\bmayAddItem\s*\(")
# Per-format wrappers are accepted only where their own definition calls QuantityPolicy.
STRICT_LAYER_RX = re.compile(r"\bbeginTopTouchLayer\s*\(|\bbeginModal\s*\(")


def wrapper_call(name: str) -> str:
    """A call to the wrapper itself: unqualified or qualified by its own namespace, never through another
    alias (QP::quantityRule is the shared policy's own function, not ClassicInventoryCatalog's)."""
    return r"(?<![\w:])(?:(?:PokeVault::Inventory|PokeBank::UIModel)::)?" + re.escape(name) + r"\s*\("


GEN4_BAG_QUANTITY_HEADER = "include/UI/Gen4BagQuantity.h"
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


def _is_absence_assert(sources, hit: str) -> bool:
    rel, ln = hit.split(":")[0], int(hit.split(":")[1])
    src = next(s for s in sources if s.rel == rel)
    line = src.code.splitlines()[ln - 1] if ln <= len(src.code.splitlines()) else ""
    return re.search(r"static_assert\s*\(\s*!\s*[\w:]*hasLabel\s*\(", line) is not None


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
    # A compile-time absence check (static_assert(!...hasLabel(..., "Make Shiny"))) is a test, not a label.
    r.violations = [v for v in lit_hits(U, r"Make\s+(Non-?)?Shiny", re.I) if not _is_absence_assert(sources, v)]
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
    # Strict (coordinator, 2026-10-16): clearing screen touchButtons alone leaves g_touchGlyphHits /
    # g_navHits live (the P0 #320 tap-through class), so only a real top touch layer counts.
    layer_rx = STRICT_LAYER_RX

    def every_caller(name: str, def_rel: str, def_ln: int, before: int, after: int, rx: re.Pattern) -> bool:
        """True when the modal is called at least once and EVERY call site supplies rx itself: the
        layer in the `before` lines leading up to the call, or the footer in the `after` lines that
        follow it (drawn over the modal, the way TrainerViewScreenBase draws the save confirm)."""
        call_rx = re.compile(r"(?<![\w.>])(?:\w+::)*" + re.escape(name) + r"\s*\(")
        sites = 0
        # File-local helpers share names across generations (drawDiscardAllConfirm in Gen III and IV):
        # a name defined in more than one file is only called from its own file.
        defined_in = [c.rel for c in U if any(True for _ in function_bodies(c.code, re.compile("^" + re.escape(name) + "$")))]
        callers = [c for c in U if c.rel == def_rel] if len(defined_in) > 1 else U
        for c in callers:
            lines = c.code.splitlines()
            for m in call_rx.finditer(c.code):
                ln = c.code.count("\n", 0, m.start()) + 1
                if c.rel == def_rel and ln == def_ln:
                    continue  # the definition itself
                if re.match(r"\s*(?:void|bool|int|static|inline)\b", lines[ln - 1]):
                    continue  # another definition / declaration
                sites += 1
                # After the call: keep reading while control can still reach the drawn frame (leaving an
                # enclosing switch/if is fine), but never into a sibling `else` branch or past a return.
                # A line counts while it is in the call's own block, or, once that block is left, at
                # the level reached (not inside a later, separately guarded block).
                tail, depth, low, stop = [], 0, 0, False
                for line in lines[ln:ln + after]:
                    start_depth = depth
                    for k, ch in enumerate(line):
                        depth += (ch == "{") - (ch == "}")
                        low = min(low, depth)
                        if ch == "}" and depth < 0 and re.match(r"\s*else\b", line[k + 1:]):
                            stop = True
                            break
                    if stop:
                        break
                    if low == 0 or start_depth <= low:
                        tail.append(line)
                    if re.search(r"\breturn\b", line) or (depth < 0 and re.match(r"\}", line)):
                        break  # the caller's own function ends here
                head = lines[max(0, ln - 1 - before):ln - 1]
                for k in range(len(head) - 1, -1, -1):
                    if re.match(r"\}", head[k]) or re.search(r"\}\s*else\b", head[k]):
                        head = head[k + 1:]  # previous function / sibling branch
                        break
                window = "\n".join(head + [lines[ln - 1]] + tail)
                if not rx.search(window):
                    return False
        return sites > 0

    for s in U:
        for name, ln, body in function_bodies(s.code, modal_rx):
            # A router draws no pixel itself and only delegates to presenters, each checked on its own
            # (S e86741c: PokemonDetailsDispatcher -> Gen1/Gen2/Modern details modals).
            # It passes only through its Gen I-V presenters (coordinator, 2026-10-16): each must draw its
            # own footer AND open a real top touch layer (beginModal / beginTopTouchLayer; clearing
            # screen touchButtons alone leaves the covered glyph hits live). Modern* is Gen VI+.
            if not re.search(r"\bfb\s*\.\s*\w+\s*\(|\bdim\w*\s*\(", body) and \
                    re.findall(r"\b(draw\w+)\s*\(", body) and \
                    all(modal_rx.search(t) for t in re.findall(r"\b(draw\w+)\s*\(", body)) and \
                    not footer_rx.search(body) and not layer_rx.search(body):
                for target in dedupe(re.findall(r"\b(draw\w+)\s*\(", body)):
                    if "Modern" in target:
                        continue
                    defs = [(c.rel, b) for c in U for _, _, b in function_bodies(c.code, re.compile("^" + re.escape(target) + "$"))]
                    if not defs:
                        r.violations.append(f"{s.rel}:{ln}: {name}() routes to {target}(), which is not defined")
                    for drel, dbody in defs:
                        if not footer_rx.search(dbody):
                            r.violations.append(f"{s.rel}:{ln}: {name}() routes to {target}() ({drel}), which draws no footer")
                        if not STRICT_LAYER_RX.search(dbody):
                            r.violations.append(f"{s.rel}:{ln}: {name}() routes to {target}() ({drel}), which opens no "
                                                "beginModal / beginTopTouchLayer")
                continue
            if not footer_rx.search(body) and not every_caller(name, s.rel, ln, 0, 30, footer_rx):
                r.violations.append(f"{s.rel}:{ln}: {name}() draws no footer of its own")
            if not layer_rx.search(body) and not every_caller(name, s.rel, ln, 20, 0, layer_rx):
                r.violations.append(f"{s.rel}:{ln}: {name}() leaves the covered footer's touch targets live")
    res.append(r)

    r = Result("CG09-item-quantity-policy",
               "Item quantity editors consult ItemQuantityPolicy (unique items never get max > 1)")
    qty_rx = re.compile(r"\b(stageInventoryQuantity|stageSetQuantity|stageBagQuantity|setQuantity|stageQuantity)\s*\(")
    # A wrapper counts when its own body uses QuantityPolicy, or (transitively) delegates to a wrapper
    # that does: classicItemQuantity -> quantityRule, gen4QuantityRule -> Gen4BagQuantity::rule.
    bodies = []
    for src in sources:
        for name, _, body in function_bodies(src.code, re.compile("^(" + "|".join(QUANTITY_POLICY_WRAPPERS) + ")$")):
            bodies.append((name, body))
        if src.rel == GEN4_BAG_QUANTITY_HEADER:
            for _, _, body in function_bodies(src.code, re.compile(r"^rule$")):
                bodies.append(("Gen4BagQuantity::rule", body))
    valid_wrappers = set()
    changed = True
    while changed:
        changed = False
        for name, body in bodies:
            if name in valid_wrappers:
                continue
            calls_valid = any(re.search(wrapper_call(w), body) for w in valid_wrappers)
            if re.search(r"\bQuantityPolicy\b", body) or calls_valid:
                valid_wrappers.add(name)
                changed = True
    wrapper_use = re.compile("|".join(wrapper_call(w) for w in sorted(valid_wrappers))) if valid_wrappers else None
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

    r = Result("CG11-no-generic-modern-ui-in-gen1-5",
               "In a Gen I-V session no path opens the generic action sheet, details modal, PickerDialog or StatEditDialog")
    for s in U:
        for m in GENERIC_UI_GUARDED_OPEN.finditer(s.code):
            ln = s.code.count("\n", 0, m.start()) + 1
            lines = s.code.splitlines()
            # The guard may precede the open, or follow it within the same few lines (the base opener
            # opens, then re-routes / closes before anything is drawn).
            window = "\n".join(lines[max(0, ln - 40):ln + 6])
            # A target hand-off that is never drawn: open, read-only details, close.
            handoff = "\n".join(lines[ln - 1:ln + 4])
            is_handoff = (re.search(r"openActionSheetTargetDetails\s*\(\s*true\s*\)", handoff)
                          and re.search(r"actionSheet\s*\.\s*close\s*\(", handoff))
            if not GENERIC_UI_GUARD.search(window) and not is_handoff:
                r.violations.append(f"{s.rel}:{ln}: {lines[ln - 1].strip()[:120]} (no genericUiAllowedForSession guard)")
        # The generic PickerDialog (screen-level pickerActive, not a surface's own state.pickerActive):
        # guarded like the openers above, or opened inside the generic details editor's input block,
        # which a Gen I-V session never enters (its details.active opens are bound above).
        detail_blocks = []
        for m in re.finditer(r"\bif\s*\(\s*details\s*\.\s*active\s*\)\s*\{", s.code):
            depth, j = 0, m.end() - 1
            while j < len(s.code):
                depth += (s.code[j] == "{") - (s.code[j] == "}")
                if depth == 0:
                    break
                j += 1
            detail_blocks.append((m.start(), j))
        for m in re.finditer(r"(?<![.\w>])pickerActive\s*=\s*true\b", s.code):
            ln = s.code.count("\n", 0, m.start()) + 1
            lines = s.code.splitlines()
            window = "\n".join(lines[max(0, ln - 40):ln + 6])
            in_details = any(a < m.start() < b for a, b in detail_blocks)
            if not GENERIC_UI_GUARD.search(window) and not in_details:
                r.violations.append(f"{s.rel}:{ln}: {lines[ln - 1].strip()[:120]} (generic picker; no genericUiAllowedForSession guard)")
    base = next((s for s in U if s.rel == "src/UI/TrainerViewScreenBase.inc"), None)
    reset = base and re.search(
        r"if\s*\(\s*!\s*genericUiAllowedForSession\s*\([^)]*\)\s*\)\s*\{(?=[^}]*pickerActive\s*=\s*false)(?=[^}]*statEdit\s*\.\s*dialogActive\s*=\s*false)",
        base.code)
    if not reset:
        r.violations.append("src/UI/TrainerViewScreenBase.inc: no per-frame reset closing pickerActive / statEdit.dialogActive "
                            "when genericUiAllowedForSession(...) is false")
    hdr = next((s for s in sources if s.rel == GENERIC_UI_GUARD_HEADER), None)
    if hdr is None or not re.search(r"\bgenericUiAllowed\s*\(", hdr.code):
        r.violations.append(f"{GENERIC_UI_GUARD_HEADER}: ClassicGenericUi::genericUiAllowed missing")
    res.append(r)
    cg11 = r

    r = Result("CG12-single-bag-add-flow",
               "Gen I-V Add Item goes through SharedBagAddFlow: staged at quantity 1 with no numpad; "
               "a quantity editor opens only from an inventory row the rule lets you edit")
    hdr = next((x for x in sources if x.rel == BAG_ADD_HEADER), None)
    if hdr is None or not all(re.search(p_, hdr.code) for p_ in
                              (r"\bdecidePick\s*\(", r"\bdecideRow\s*\(", r"d\.quantity\s*=\s*rule\.minimum")):
        r.violations.append(f"{BAG_ADD_HEADER}: decidePick / decideRow (new item staged at rule.minimum) missing")
    for rel in BAG_ADD_SURFACES:
        src = next((x for x in sources if x.rel == rel), None)
        if src is None or not re.search(r"\bdecidePick\s*\(", src.code):
            r.violations.append(f"{rel}: Add does not decide through BagAdd::decidePick")
    add_rx = re.compile(r"\b(?:decidePick|stageBagAdd|pickAdd|confirmAdd)\s*\(")
    qty_stage_rx = re.compile(r"\b(?:stageBagQuantity|stageInventoryQuantity|stageSetQuantity|stageQuantity|"
                              r"setBagQuantity|applyMutation|bagSetQuantity)\s*\(")
    # Each numpad in a bag function belongs to whichever decision precedes it most closely: after a
    # decidePick / stageBagAdd it is a numpad on the Add path; a quantity numpad must follow decideRow.
    row_rx = re.compile(r"\bdecideRow\s*\(")
    for s in U:
        for name, ln, body in function_bodies(s.code, re.compile(r".")):
            if "promptNumber" not in body or not (add_rx.search(body) or row_rx.search(body)
                                                   or qty_stage_rx.search(body)):
                continue
            marks = sorted([(m.start(), "add") for m in add_rx.finditer(body)] +
                           [(m.start(), "row") for m in row_rx.finditer(body)])
            for pm in re.finditer(r"\bpromptNumber\s*\(", body):
                before = [kind for pos, kind in marks if pos < pm.start()]
                pln = ln + body.count("\n", 0, pm.start())
                if before and before[-1] == "add":
                    r.violations.append(f"{s.rel}:{pln}: {name}() opens a numpad on the Add path")
                elif not before and qty_stage_rx.search(body[pm.start():pm.start() + 1500]):
                    r.violations.append(f"{s.rel}:{pln}: {name}() edits an item quantity without BagAdd::decideRow")
    res.append(r)

    r = Result("CG15-shared-species-search",
               "Species live search (SharedSearchInput prefix filter) is in the one shared chooser for all five "
               "generations; no generation switches it off")
    srch = next((x for x in sources if x.rel == SEARCH_HEADER), None)
    if srch is None or not re.search(r"\bmatchesPrefix\s*\(", srch.code):
        r.violations.append(f"{SEARCH_HEADER}: SharedSearchInput::matchesPrefix missing")
    picker = next((x for x in sources if x.rel == SPECIES_PICKER_HEADER), None)
    if picker is None or not re.search(r"\bstruct\s+Chooser\b", picker.code) or \
            not re.search(r"\brefilter\s*\(", picker.code) or "SharedSearchInput::" not in picker.code:
        r.violations.append(f"{SPECIES_PICKER_HEADER}: Chooser / refilter over SharedSearchInput missing")
    for gens, rel in SPECIES_CHOOSER_OWNERS:
        src = next((x for x in sources if x.rel == rel), None)
        if src is None or not re.search(r"\bSharedSpeciesPicker::refilter\s*\(", src.code) or \
                not re.search(r"\bSharedSpeciesPicker::handle(?:Keys|Touch)?\s*\(", src.code):
            r.violations.append(f"{rel}: the {gens} species picker does not filter and route input through the "
                                "shared chooser")
    r.violations += code_hits(U, r"\bkSpeciesSearch\s*=\s*false\b")
    res.append(r)

    r = Result("CG14-shared-draggable-scrollbar",
               "Every scrollbar is the one SharedScrollbar: drawn through it, never privately, and draggable "
               "(its State is hit-tested with handleTouch)")
    # 1. The two legacy entry points only delegate.
    for rel, name in SCROLLBAR_WRAPPERS:
        src = next((x for x in sources if x.rel == rel), None)
        bodies = [b for _, _, b in function_bodies(src.code, re.compile("^" + name + "$"))] if src else []
        if not bodies:
            r.violations.append(f"{rel}: {name}() is missing")
        for b in bodies:
            if not re.search(r"\bSharedScrollbar::(paint|drawAndPublish)\s*\(", b) or \
                    re.search(r"\bfb\s*\.\s*draw\w*\s*\(", b):
                r.violations.append(f"{rel}: {name}() does not simply delegate to SharedScrollbar")
    # 2. No private scrollbar: a scroll-bar/thumb/indicator drawer outside SharedScrollbar.h must
    #    delegate, and nothing else computes a thumb rectangle of its own.
    for s in U:
        if s.rel == SCROLLBAR_HEADER:
            continue
        for name, ln, body in function_bodies(s.code, re.compile(r"(?i)^draw\w*scroll\w*(bar|thumb|indicator|track)\w*$")):
            if re.search(r"\bfb\s*\.\s*draw\w*\s*\(", body) and "SharedScrollbar::" not in body:
                r.violations.append(f"{s.rel}:{ln}: {name}() draws a private scrollbar")
        for m in re.finditer(r"\b(thumb(?:Y|H|Top|Height|Len|Length|Size))\s*=", s.code):
            ln = s.code.count("\n", 0, m.start()) + 1
            r.violations.append(f"{s.rel}:{ln}: private thumb geometry ({m.group(1)})")
    # 3. Every draw site outside the wrappers carries a State, and that State is hit-tested.
    draw_rx = re.compile(r"\b(?:SharedScrollbar::drawAndPublish|SharedScrollbar::paint|drawScrollbar|"
                         r"drawVerticalScrollIndicator)\s*\(")
    touched = set()
    for s in U:
        for m in re.finditer(r"\bhandleTouch\s*\(\s*([\w.>\-\[\]()*&]+?)\s*,", s.code):
            touched.add(re.split(r"\.|->", m.group(1))[-1].strip("*&() "))
    wrapper_files = {rel for rel, _ in SCROLLBAR_WRAPPERS} | {SCROLLBAR_HEADER}
    for s in U:
        if s.rel in wrapper_files:
            continue
        for m in draw_rx.finditer(s.code):
            ln = s.code.count("\n", 0, m.start()) + 1
            call = s.code[m.end():m.end() + 400]
            args = [a.strip() for a in call.split(",")[:2]]
            state = None
            if m.group(0).startswith(("SharedScrollbar::drawAndPublish", "drawScrollbar")) and len(args) > 1 \
                    and re.search(r"[Ss]crollbar\b|[Ss]crollbar\)?$|Bar$", args[1]):
                state = re.split(r"\.|->", args[1])[-1].strip("*&() ")
            elif m.group(0).startswith("SharedScrollbar::paint"):
                cap = re.search(r"([\w.>\-]+)\s*\.\s*capturing\s*\(\s*\)", s.code[m.end():m.end() + 600])
                state = re.split(r"\.|->", cap.group(1))[-1] if cap else None
            if state is None:
                r.violations.append(f"{s.rel}:{ln}: scrollbar drawn without a SharedScrollbar::State (not draggable)")
            elif state not in touched:
                r.violations.append(f"{s.rel}:{ln}: scrollbar State '{state}' is never hit-tested with handleTouch")
    res.append(r)

    # The generic action-sheet model only renders in a Gen VI+ session once CG11 holds (coordinator,
    # 2026-10-16): its wording leaves scope then, and returns to scope if CG11 regresses.
    scope = dict(OUT_OF_SCOPE)
    if not cg11.violations:
        scope.update(OUT_OF_SCOPE_WHEN_CG11_HOLDS)
    for r in res:
        r.out_of_scope = [v for v in r.violations if v.split(":", 1)[0] in scope]
        r.violations = [v for v in r.violations if v.split(":", 1)[0] not in scope]
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
