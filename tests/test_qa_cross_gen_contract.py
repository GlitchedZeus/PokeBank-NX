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
    # Round 4: owner round 3 FAIL items (ROUND3_RESULTS_62a2f8c3.md). Owners are proposed from the round 3
    # lane areas until the coordinator's round 4 lane map lands.
    "CG30-trainer-page-opens-review": "C proposed (R2-TRAINER, R2-BUG-002)",
    "CG31-classic-exit-offers-review": "C proposed (NEW-19, R2-BUG-002)",
    "CG32-review-rows-readable-names": "C + S proposed (R2-REVIEWMOVE)",
    "CG33-party-opens-actions": "C proposed (L06, E02, NEW-20)",
    "CG34-gen5-key-tm-pockets-editable": "S / I proposed (NEW-17)",
    "CG35-gen5-review-matches-gen4": "S proposed (AUD-01)",
    # CG01-CG11 XPASSed on candidate 7fea1797 (app-qa-nro-audit run 38947904299) and are ENFORCED;
    # CG12 and CG14 XPASSed on #307 d104310 (batch 1), CG18 on 41cbe92 (batch 2), CG13 and CG15 on
    # d51c1ff (batch 3), CG16 and CG17 on 348129f (batch 4): ENFORCED.
    # Round 3: CG21/CG26/CG28 on #307 aeccd13, CG20/CG24/CG25/CG27 on 7ed1cc16, CG20b on 7ed1cc16,
    # CG18b on 11940524, CG19 on 8a84128e, CG20c/CG22/CG23 on lane P 8d533b52: ENFORCED.
    # Every contract is enforced.
    # Round 2 contracts are XFAIL only until their lane's push is on #307.
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

# CG13 / CG16 / CG17 (Lane S 0f132eb) and CG18 (Lane C 6d6c55d5).
MENU_GEOMETRY_HEADER = "include/UI/SharedMenuGeometry.h"
FIELD_STATE_HEADER = "include/UI/SharedFieldState.h"
FIELD_STATE_SURFACES = ("src/UI/Gen3SharedPokemonSurface.inc", "src/UI/SharedPokemonEditorSurface.inc")
EDITOR_FILE_RX = re.compile(r"src/UI/(?:Gen[1-5]\w*\.(?:inc|cpp)|SharedPokemonEditorSurface\.inc|"
                            r"Gen1PokemonDetailsPresentation\.cpp|Modals/Gen[12]PokemonDetailsModal\.cpp)$")
GAME_CONTEXT_HEADER = "include/UI/ClassicGameContext.h"
GEN15_GAME_IDS = ("red_gb", "blue_gb", "yellow_gb", "gold_gbc", "silver_gbc", "crystal_gbc", "ruby_gba",
                  "sapphire_gba", "emerald_gba", "firered_gba", "leafgreen_gba", "diamond_nds", "pearl_nds",
                  "platinum_nds", "heartgold_nds", "soulsilver_nds", "black_nds", "white_nds", "black2_nds",
                  "white2_nds")
TRAINER_EDITABILITY_HEADER = "include/UI/ClassicTrainerEditability.h"
TRAINER_SESSION_HEADER = "include/UI/ClassicTrainerSession.h"

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

    r = Result("CG13-shared-menu-geometry",
               "Every menu type lays out through SharedMenuGeometry (one rule per type, used by a real "
               "picker in every generation)")
    geo = next((x for x in sources if x.rel == MENU_GEOMETRY_HEADER), None)
    if geo is None:
        r.violations.append(f"{MENU_GEOMETRY_HEADER}: missing")
    else:
        enum = re.search(r"enum\s+class\s+MenuType\b[^{]*\{(.*?)\}", geo.code, re.S)
        types = [t for t in re.findall(r"\b([A-Z]\w*)\b\s*(?:=\s*\d+\s*)?,", enum.group(1) + ",")
                 if t != "Count"] if enum else []
        if not types:
            r.violations.append(f"{MENU_GEOMETRY_HEADER}: MenuType enum not found")
        for t in types:
            if not re.search(r"\bMenuType::" + t + r"\b", geo.code.split("enum class MenuType", 1)[1].split("}", 1)[1]):
                r.violations.append(f"{MENU_GEOMETRY_HEADER}: MenuType::{t} has no rule")
            users = [x.rel for x in U if x.rel != MENU_GEOMETRY_HEADER and re.search(r"\bMenuType::" + t + r"\b", x.code)]
            if not users:
                r.violations.append(f"{MENU_GEOMETRY_HEADER}: MenuType::{t} is not used by any menu")
    res.append(r)

    r = Result("CG16-shared-field-state",
               "Locked values are gold through SharedFieldState and never editable; no field label says "
               "\"(read-only)\"; no editor value is drawn in raw gold")
    fs_hdr = next((x for x in sources if x.rel == FIELD_STATE_HEADER), None)
    if fs_hdr is None or "contractHolds()" not in fs_hdr.code or "static_assert" not in fs_hdr.code:
        r.violations.append(f"{FIELD_STATE_HEADER}: the compile-time styling contract (gold never activates) is missing")
    for rel in FIELD_STATE_SURFACES:
        src = next((x for x in sources if x.rel == rel), None)
        if src is None or not re.search(r"\bSharedFieldState::State::ReadOnly\b", src.code):
            r.violations.append(f"{rel}: DETAILS rows do not take their state from SharedFieldState")
    for s in U:
        for ln, lit in s.literals:
            if re.fullmatch(r"[A-Z][\w ]{0,24} \(read-only\)", lit):
                r.violations.append(f"{s.rel}:{ln}: \"{lit}\" (read-only is a gold field state, not label text)")
        if not EDITOR_FILE_RX.search(s.rel):
            continue
        for m in re.finditer(r"\bdrawText\s*\([^;]*?\bColors::ShinyStar\b", s.code):
            ln = s.code.count("\n", 0, m.start()) + 1
            r.violations.append(f"{s.rel}:{ln}: editor value drawn in raw gold (Colors::ShinyStar); use SharedFieldState")
    res.append(r)

    r = Result("CG17-exact-source-game-header",
               "Create / Edit / View headers name the exact source game in all 20 Gen I-V versions")
    ctx = next((x for x in sources if x.rel == GAME_CONTEXT_HEADER), None)
    if ctx is None:
        r.violations.append(f"{GAME_CONTEXT_HEADER}: missing")
    else:
        for gid in GEN15_GAME_IDS:
            if f'"{gid}"' not in ctx.code:
                r.violations.append(f"{GAME_CONTEXT_HEADER}: exactGameName has no name for {gid}")
        line = [b for _, _, b in function_bodies(ctx.code, re.compile(r"^contextLine$"))]
        if not line or "exactGameName(" not in line[0]:
            r.violations.append(f"{GAME_CONTEXT_HEADER}: contextLine() does not name the exact game")
    shell = next((x for x in sources if x.rel == "include/UI/SharedPokemonShell.h"), None)
    chrome = [b for _, _, b in function_bodies(shell.code, re.compile(r"^drawChrome$"))] if shell else []
    if not chrome or "contextLine(" not in chrome[0]:
        r.violations.append("include/UI/SharedPokemonShell.h: drawChrome() does not draw the shared context line")
    for s in U:
        if s.rel == GAME_CONTEXT_HEADER:
            continue
        lines = s.code.splitlines()
        for ln, lit in s.literals:
            if "Source save immutable" not in lit:
                continue
            window = "\n".join(lines[max(0, ln - 6):ln + 4])
            if not re.search(r"\b(?:exactGameName|contextLine|sourceGameTitle|sharedGameName)\s*\(", window):
                r.violations.append(f"{s.rel}:{ln}: header without the exact source game")
    res.append(r)

    r = Result("CG18-trainer-edit-footer",
               "The Trainer page shows \"A: Edit\" only when Name / Money are editable in this session "
               "(Gen I staged Trainer editor included)")
    ed = next((x for x in sources if x.rel == TRAINER_EDITABILITY_HEADER), None)
    body = [b for _, _, b in function_bodies(ed.code, re.compile(r"^trainerFieldsEditable$"))] if ed else []
    if not body or "GameVersion::RBY" not in body[0] or "GameVersion::GSC" not in body[0]:
        r.violations.append(f"{TRAINER_EDITABILITY_HEADER}: trainerFieldsEditable() does not cover Gen I and Gen II")
    sess = next((x for x in sources if x.rel == TRAINER_SESSION_HEADER), None)
    instr = [b for _, _, b in function_bodies(sess.code, re.compile(r"^trainerPageInstructions$"))] if sess else []
    if not instr or not re.search(r"\beditable\s*\?", instr[0]) or instr[0].count("A: Edit") != 1:
        r.violations.append(f"{TRAINER_SESSION_HEADER}: trainerPageInstructions() must offer A: Edit only when editable")
    base = next((x for x in U if x.rel == "src/UI/TrainerViewScreenBase.inc"), None)
    # The footer branch is the Trainer block that assigns `instructions`.
    blocks = [m.group(1) for m in re.finditer(r"selectedMode\s*==\s*ViewMode::Trainer\)\s*\{(.*?)\n\s*\}",
                                              base.code if base else "", re.S) if "instructions" in m.group(1)]
    if len(blocks) != 1 or not re.search(r"trainerPageInstructions\s*\(\s*[\w:]*trainerFieldsEditable\s*\(",
                                         blocks[0]) or "A: Edit" in blocks[0]:
        r.violations.append("src/UI/TrainerViewScreenBase.inc: the Trainer page footer is not "
                            "trainerPageInstructions(trainerFieldsEditable(...))")
    res.append(r)

    r = Result("CG18b-trainer-footer-not-overridden",
               "Once the Trainer page offers \"A: Edit\", no later footer rule on the same frame replaces it "
               "(R2-BUG-001: Gen I staged Trainer showed no A: Edit)")
    # CG18 checked only the Trainer branch's own assignment. In TrainerViewScreenBase.inc the
    # read-only footer override (useReadOnlyInventoryFooterOverride) runs after it and, for a read-only
    # source with a staged Trainer editor (Gen I), replaced the footer with "Arrows: Navigate | B: Back".
    # Every later override must exclude an editable Trainer page.
    if base:
        assign = re.search(r"trainerPageInstructions\s*\(", base.code)
        for m in re.finditer(r"\buseReadOnlyInventoryFooterOverride\s*\(", base.code):
            if assign is None or m.start() < assign.start():
                continue
            ln = base.code.count("\n", 0, m.start()) + 1
            depth, j = 0, m.end() - 1
            while j < len(base.code):
                depth += (base.code[j] == "(") - (base.code[j] == ")")
                if depth == 0:
                    break
                j += 1
            args = base.code[m.end():j]
            if not re.search(r"\btrainerFieldsEditable\s*\(|ViewMode::Trainer\b", args):
                r.violations.append(f"src/UI/TrainerViewScreenBase.inc:{ln}: the read-only footer override can "
                                    "replace the Trainer page's A: Edit footer (no editable-Trainer exclusion)")
    res.append(r)

    r = Result("CG19-species-change-resets-moves",
               "Every Gen I-V editor path that changes a Pokemon's species also resets its moves through the "
               "shared SpeciesMoveDefaults policy (R2-BUG-004)")
    # Lane R (round 3): one shared species-change policy, called once from every species apply path.
    change_rx = re.compile(r"(?<![\w.>])(?:[\w.>-]*(?:\.|->))?(?:setSpecies|applySpecies)\s*\(")
    # A reference alone (e.g. SpeciesMoveDefaults::forSpecies or a version lookup) is not enough: the
    # body must call one of the policy's move WRITERS, so dropping the write itself fails here.
    MOVE_WRITE_RX = re.compile(r"\bSpeciesMoveDefaults::(?:writeArrays|writeEdit|refreshArrays|setMoves|stageMoves)\s*[<(]")
    # The Gen II / Gen III draft sessions own the species change itself: every UI path (Gen II picker
    # and number entry, Gen III picker) calls Session::setSpecies, which applies the policy. A call to
    # `session.setSpecies(` / `state.setSpecies(` counts when every setSpecies() defined in those
    # session headers applies SpeciesMoveDefaults (so removing it from a session fails here).
    session_headers = ("include/UI/Gen2PokemonSession.h", "include/UI/Gen3SharedPokemonSession.h")

    def defined_bodies(code: str, name: str) -> list[str]:
        # Member definitions with default arguments (`= {}`) defeat function_bodies(); match the
        # parameter list by parenthesis depth and the body by brace depth instead.
        out = []
        for m in re.finditer(rf"\b{name}\s*\(", code):
            depth, i = 1, m.end()
            while i < len(code) and depth:
                depth += (code[i] == "(") - (code[i] == ")")
                i += 1
            head = re.match(r"\s*(?:const\s*)?(?:noexcept\s*)?\{", code[i:])
            if not head:
                continue  # a call or a declaration
            j = i + head.end() - 1
            start, depth = j, 0
            while j < len(code):
                depth += (code[j] == "{") - (code[j] == "}")
                if depth == 0:
                    break
                j += 1
            out.append(code[start:j])
        return out

    session_set = [b for x in sources if x.rel in session_headers for b in defined_bodies(x.code, "setSpecies")]
    session_resets = bool(session_set) and all(re.search(MOVE_WRITE_RX, b) for b in session_set)
    seen_fn = set()
    for s in U:
        if not EDITOR_FILE_RX.search(s.rel):
            continue
        fns = list(function_bodies(s.code, re.compile(r".")))
        by_name = {}
        for name, _, body in fns:
            by_name.setdefault(name, []).append(body)

        def resets(body: str) -> bool:
            # Satisfied by the body itself or by a same-file helper it calls (applyPicker -> applySpecies).
            if re.search(MOVE_WRITE_RX, body):
                return True
            if session_resets and re.search(r"\b(?:session|state)\s*\.\s*setSpecies\s*\(", body):
                return True
            return any(re.search(MOVE_WRITE_RX, b)
                       for callee in re.findall(r"\b(\w+)\s*\(", body) if callee in by_name
                       for b in by_name[callee] if b is not body)

        for name, ln, body in fns:
            if not (change_rx.search(body) or re.search(r"(?i)applyspecies", name)):
                continue
            if (s.rel, name) in seen_fn:
                continue
            seen_fn.add((s.rel, name))
            if not resets(body):
                r.violations.append(f"{s.rel}:{ln}: {name}() changes the species but keeps the old moves "
                                    "(no SpeciesMoveDefaults move-writer call)")
    res.append(r)

    r = Result("CG22-move-chooser-type-badges",
               "The Move chooser draws each move's type with the shared summary type-badge renderer, not plain "
               "text (R2-BUG-007)")
    panel = next((x for x in sources if x.rel == "include/UI/SharedMovesPanel.h"), None)
    rows = [b for _, _, b in function_bodies(panel.code, re.compile(r"^drawChooserRow$"))] if panel else []
    if not rows:
        r.violations.append("include/UI/SharedMovesPanel.h: drawChooserRow() missing")
    for b in rows:
        if not re.search(r"\bClassicTypeBadges::\w+\s*\(|\bdrawTypeBadge\s*\(", b):
            r.violations.append("include/UI/SharedMovesPanel.h: drawChooserRow() draws the move type as text, "
                                "not with the shared type badge")
        # Text is allowed only as the fallback of a badge draw that could not render (Hidden Power, Curse,
        # missing art): `if (!badgeable || ClassicTypeBadges::drawCanonical(...) == 0) fb.drawText(...typeLabel...)`.
        text = len(re.findall(r"drawText\s*\([^;]*typeLabel\s*\(", b))
        fallback = len(re.findall(r"if\s*\([^;{}]*(?:\bClassicTypeBadges::\w+|\bdrawTypeBadge)\s*\([^;{}]*\)\s*\{?\s*"
                                  r"fb\.drawText\s*\([^;]*typeLabel\s*\(", b))
        if text > fallback:
            r.violations.append("include/UI/SharedMovesPanel.h: drawChooserRow() prints typeLabel() as text outside "
                                "the badge fallback")
    res.append(r)

    r = Result("CG24-gen34-trainer-staged-export",
               "Gen III and Gen IV Trainer Name / Money edits go through staged Trainer editors (exportable), "
               "never the mutable-workspace fallback (R2-BUG-002)")
    ed = next((x for x in sources if x.rel == TRAINER_EDITABILITY_HEADER), None)
    body = [b for _, _, b in function_bodies(ed.code, re.compile(r"^trainerFieldsEditable$"))] if ed else []
    for group, games in (("Gen III", ("RS", "E", "FRLG")), ("Gen IV", ("DP", "PT", "HGSS"))):
        covered = body and any(re.search(r"GameVersion::" + g + r"\b", body[0]) for g in games)
        if not covered:
            r.violations.append(f"{TRAINER_EDITABILITY_HEADER}: trainerFieldsEditable() has no {group} staged "
                                "Trainer case (falls back to mutableWorkspace)")
    for gen in ("Gen3", "Gen4"):
        if not any(re.search(r"\bclass\s+\w*" + gen + r"\w*StagedTrainer\w*|\bstruct\s+\w*" + gen + r"\w*StagedTrainer\w*",
                             x.code) for x in sources):
            r.violations.append(f"no {gen} staged Trainer editor (Name / Money edits have no export path)")
    res.append(r)

    def src(rel):
        return next((x for x in sources if x.rel == rel), None)

    def bodies(rel, name):
        f = src(rel)
        return [b for _, _, b in function_bodies(f.code, re.compile("^" + name + "$"))] if f else []

    # Coordinator ruling (round 3): an editor's FIELD focus on its own work surface stays unboxed; only
    # interactive modal rows use SharedBoxedRows. Exempt exactly these Gen I/II field-focus painters (lane S
    # list, checked against 7ed1cc16); any other drawSelectionHighlight in the files still fails.
    FIELD_FOCUS_EXEMPT = {
        "src/UI/Gen1PokemonEditorOverlayFoundation.inc": ("foundationDrawIdentityRow", "foundationDrawWorkspace"),
        "src/UI/Gen1PokemonEditorOverlayUX.inc": ("drawRow",),
        "src/UI/Gen1PokemonEditorOverlayUXCleanup2.inc": ("ux2DrawLogicalRow",),
        "src/UI/Gen2SharedPokemonSurface.inc": ("drawSharedDetails", "drawSharedMoves"),
    }
    # Files with no modal rows at all (Foundation's only modal, the exit confirm, is drawGlyphButton choices).
    NO_MODAL_ROWS = {"src/UI/Gen1PokemonEditorOverlayFoundation.inc"}
    # R2-BUG-012 (lane S): one shared boxed-row style for every focused modal row / cell.
    boxed_api = r"\bSharedBoxedRows::(?:drawBox|drawRow|drawGlyphChoices)\s*\(|\bdrawMenuCell\s*\("
    for cid, title, files in (
            ("CG20-shared-boxed-modal-rows",
             "Gen III-V editor pickers (value lists, Move chooser / editor, Held Item grid, species picker, Gen V "
             "Pokedex) draw focused rows through SharedBoxedRows, never drawSelectionHighlight (R2-BUG-012)",
             ("src/UI/SharedPokemonEditorSurface.inc", "src/UI/Gen3SharedPokemonSurface.inc",
              "src/UI/Gen5SharedReadOnlySurface.inc",
              "include/UI/SharedHeldItemPickerPresentation.h",
              "include/UI/SharedEditorChrome.h")),
            ("CG20c-shared-boxed-species-picker",
             "The shared species picker draws its focused row through SharedBoxedRows / drawMenuCell, never "
             "drawSelectionHighlight (R2-BUG-012, owner P)",
             ("include/UI/SharedSpeciesPicker.h",)),
            ("CG20b-shared-boxed-modal-rows-gen12-classic",
             "Gen I/II editor overlays and the Classic inventory draw focused rows through SharedBoxedRows, never "
             "drawSelectionHighlight (R2-BUG-012, second batch)",
             ("src/UI/Gen1PokemonEditorOverlayFoundation.inc", "src/UI/Gen1PokemonEditorOverlayUX.inc",
              "src/UI/Gen1PokemonEditorOverlayUXCleanup2.inc", "src/UI/Gen1PokemonEditorOverlayUXCleanup3.inc",
              "src/UI/Gen2SharedPokemonSurface.inc", "src/UI/Gen2UnifiedPokemonWorkspace.inc",
              "src/UI/Gen2SharedSurfaceParity.inc", "src/UI/Gen2PokemonPickerOverlay.inc",
              "src/UI/Gen2HardwarePickerFix.inc", "src/UI/TrainerViewScreenGSCOverlay.inc",
              "src/UI/ClassicInventoryOverlay.cpp"))):
        r = Result(cid, title)
        if not src("include/UI/SharedBoxedRows.h"):
            r.violations.append("include/UI/SharedBoxedRows.h (UI::SharedBoxedRows::drawBox) does not exist")
        for rel in files:
            f = src(rel)
            if not f:
                continue  # removed / renamed files are CG11's concern
            spans = [(n, ln, ln + b.count("\n")) for n, ln, b in function_bodies(f.code, re.compile(r"."))]
            exempt = FIELD_FOCUS_EXEMPT.get(rel, ())
            for i, line in enumerate(f.code.splitlines(), 1):
                if re.search(r"\bdrawSelectionHighlight\s*\(", line):
                    inner = [n for n, a, b in spans if a <= i <= b]
                    if inner and inner[-1] in exempt:
                        continue  # the editor's own field focus, not a modal row
                    r.violations.append(f"{rel}:{i}: drawSelectionHighlight() instead of SharedBoxedRows")
            if rel not in NO_MODAL_ROWS and not re.search(boxed_api, f.code):
                r.violations.append(f"{rel}: no SharedBoxedRows::drawBox / drawRow / drawMenuCell call")
        res.append(r)

    # R2-BUG-013 (2R): button letters in help / instruction prose become controller glyphs.
    r = Result("CG21-help-lines-use-button-glyphs",
               "Help overlays draw every line through the shared glyph renderer (drawGlyphHelpLine over "
               "GlyphInstruction), so no plain-text button letters remain where glyphs apply (R2-BUG-013)")
    if not src("include/UI/GlyphInstruction.h"):
        r.violations.append("include/UI/GlyphInstruction.h (PokeBank::UIModel::GlyphInstruction) does not exist")
    chrome = src("include/UI/ScreenChrome.h")
    for fn in ("drawGlyphText", "drawGlyphSentence", "drawGlyphHelpLine"):
        if not chrome or not list(function_bodies(chrome.code, re.compile("^" + fn + "$"))):
            r.violations.append(f"include/UI/ScreenChrome.h: {fn}() missing")
    overlay = bodies("include/UI/ScreenChrome.h", "drawInfoOverlay")
    if not overlay:
        r.violations.append("include/UI/ScreenChrome.h: drawInfoOverlay() missing")
    for b in overlay:
        if re.search(r"\bdrawText\s*\([^;]*\bline\b", b):
            r.violations.append("include/UI/ScreenChrome.h: drawInfoOverlay() draws help lines with plain drawText")
        if not re.search(r"\bdrawGlyphHelpLine\s*\(", b):
            r.violations.append("include/UI/ScreenChrome.h: drawInfoOverlay() does not call drawGlyphHelpLine")
    res.append(r)

    # R2-BUG-006 (lane P): long item catalogs open the shared grid picker.
    r = Result("CG23-bag-add-grid-picker",
               "Every Bag Add surface (Gen I-III Classic inventory, Gen IV, Gen V) opens the shared item grid "
               "(UI::SharedItemGridPicker), whose cells are SharedEditorChrome::drawMenuCell boxes (R2-BUG-006)")
    grid = src("include/UI/SharedItemGridPicker.h")
    if not grid:
        r.violations.append("include/UI/SharedItemGridPicker.h (UI::SharedItemGridPicker) does not exist")
    else:
        # The generic grid hands each cell to a caller-supplied painter; the Bag Add adapter passes drawMenuCell.
        adapter = src("include/UI/SharedBagAddPicker.h")
        if not any(f and re.search(r"\bdrawMenuCell\s*\(", f.code) for f in (grid, adapter)):
            r.violations.append("include/UI/SharedItemGridPicker.h / SharedBagAddPicker.h: grid cells are not "
                                "drawn with SharedEditorChrome::drawMenuCell")
    for rel in ("src/UI/ClassicInventoryOverlay.cpp", "src/UI/Gen4SharedPokemonSurface.inc",
                "src/UI/Gen5SharedReadOnlySurface.inc"):
        f = src(rel)
        if f and not re.search(r"\bSharedItemGridPicker::|\bSharedBagAddPicker::pickerGrid\s*\(|\bAddGrid::navigate\s*\(",
                               f.code):
            r.violations.append(f"{rel}: Bag Add still opens the single-column item list, not the shared grid")
    res.append(r)

    # Part 9 report 6 (lane I): Gen V machines and Feathers have labels and art.
    r = Result("CG25-gen5-machine-and-feather-art",
               "Black/White and Black 2/White 2 TM/HM items resolve through the one machine table (label + type "
               "art), and the seven Gen V Feathers map to the pinned *-wing art (Part 9 report 6)")
    md = src("src/Names/MachineDisplay.cpp")
    if not md or not re.search(r"\bGameVersion::(?:B|W|B2|W2|BW|B2W2)\b", md.code):
        r.violations.append("src/Names/MachineDisplay.cpp: getMachineDescriptor has no Black/White or "
                            "Black 2/White 2 case (all 101 Gen V TMs/HMs have no icon; HM01-06 read TM93-98)")
    key = src("include/UI/ItemSpriteKey.h")
    lits = {lit for _, lit in key.literals} if key else set()
    for feather in ("health", "muscle", "resist", "genius", "clever", "swift", "pretty"):
        if f"{feather}-wing" not in lits:
            r.violations.append(f"include/UI/ItemSpriteKey.h: no {feather}-feather -> {feather}-wing alias")
    res.append(r)

    # Part 9 report 3 (2R): provider names on the Trainer / Game Browser surfaces.
    r = Result("CG26-trainer-provider-display-name",
               "The Trainer / Game Browser source header names the provider through providerDisplayName(), never a "
               "raw label or a generic RETROARCH / EXTERNAL placeholder (Part 9 report 3)")
    body = bodies("include/UI/TrainerViewScreenBase.h", "legacyProviderLabel")
    if not body:
        r.violations.append("include/UI/TrainerViewScreenBase.h: legacyProviderLabel() missing")
    for b in body:
        if not re.search(r"\bproviderDisplayName\s*\(", b):
            r.violations.append("include/UI/TrainerViewScreenBase.h: legacyProviderLabel() returns the raw "
                                "provider label instead of providerDisplayName()")
        if re.search(r"\breturn\s+sourceProviderLabel\b|\?\s*\"?RETROARCH|\"EXTERNAL\"", b) or \
                re.search(r"return\s+[^;]*\bsourceProviderLabel\s*;", b):
            r.violations.append("include/UI/TrainerViewScreenBase.h: legacyProviderLabel() passes "
                                "sourceProviderLabel / RETROARCH / EXTERNAL through")
    res.append(r)

    # Part 9 report 4 (lane L): the RetroArch core follows the save's own evidence. Lane L's fix:
    # retroArchCoreForSave(gameId, sourcePath) reads the save's evidence and falls back to the old
    # game-id default only when there is none; both launch paths take their core from it.
    r = Result("CG27-retroarch-core-from-save-evidence",
               "Every RetroArch launch path takes its core from retroArchCoreForSave(gameId, sourcePath); the "
               "game-id default (defaultRetroArchCore) is only its no-evidence fallback, the descriptor's "
               "empty-core fallback, or link-time validation (Part 9 report 4)")
    gl = src("src/UI/GameLauncher.cpp")
    fns = {}
    for name, ln, body in (function_bodies(gl.code, re.compile(r".")) if gl else []):
        fns.setdefault(name, []).append((ln, body))
    sig = re.search(r"\bretroArchCoreForSave\s*\(([^)]*)\)\s*\{", gl.code) if gl else None
    if not sig:
        r.violations.append("src/UI/GameLauncher.cpp: retroArchCoreForSave() is not defined")
    elif not re.search(r"\bsourcePath\b", sig.group(1)):
        r.violations.append("src/UI/GameLauncher.cpp: retroArchCoreForSave(" + sig.group(1).strip() +
                            ") does not take the save's path")
    for launch in ("descriptorFromStored", "resolveRetroArch"):
        bodies_ = fns.get(launch, [])
        if not bodies_:
            r.violations.append(f"src/UI/GameLauncher.cpp: RetroArch launch path {launch}() missing")
        for ln, body in bodies_:
            if not re.search(r"\bretroArchCoreForSave\s*\(", body):
                r.violations.append(f"src/UI/GameLauncher.cpp:{ln}: {launch}() does not take its core from "
                                    "retroArchCoreForSave()")
    allowed = {
        "retroArchCoreForSave": None,                     # its own no-evidence / ambiguous fallback
        "saveGameLaunchBinding": None,                    # link-time validation that a core is installed
        # descriptor builder: only when the caller passed no core
        "directDescriptor": re.compile(r"\.empty\(\)\s*\?\s*defaultRetroArchCore\s*\("),
    }
    for name, entries in fns.items():
        for ln, body in entries:
            calls = len(re.findall(r"\bdefaultRetroArchCore\s*\(", body))
            if not calls or name == "defaultRetroArchCore":
                continue
            if name not in allowed:
                r.violations.append(f"src/UI/GameLauncher.cpp:{ln}: {name}() picks the core from the game id "
                                    "(defaultRetroArchCore) instead of the save's evidence")
            elif allowed[name] is not None and len(allowed[name].findall(body)) != calls:
                r.violations.append(f"src/UI/GameLauncher.cpp:{ln}: {name}() calls defaultRetroArchCore() outside "
                                    "its empty-core fallback")
    res.append(r)

    # Part 9 report 9 (2R): re-confirming the current profile keeps the focused game.
    r = Result("CG28-same-profile-keeps-game-focus",
               "Re-confirming the already selected Switch profile does not reset the focused game (Part 9 report 9)")
    body = bodies("src/UI/SaveSelectScreen.cpp", "setUser")
    if not body:
        r.violations.append("src/UI/SaveSelectScreen.cpp: SaveSelectScreen::setUser() missing")
    for b in body:
        reset = re.search(r"\btitleIndex\s*=\s*0\s*;", b)
        guard = re.search(r"if\s*\([^)]*==\s*userIndex[^)]*\)\s*(?:\{[^}]*)?return\b|"
                          r"if\s*\([^)]*userIndex\s*==[^)]*\)\s*(?:\{[^}]*)?return\b", b)
        if reset and (not guard or guard.start() > reset.start()):
            r.violations.append("src/UI/SaveSelectScreen.cpp: setUser() sets titleIndex = 0 even when the "
                                "profile is unchanged")
    res.append(r)

    # Owner round 3 NEW-01 PASS (B closes Help): every Help overlay's input handler closes on B.
    r = Result("CG29-help-closes-with-b",
               "Every Help overlay input handler (Trainer view, Home / Save Select, app shell) closes on B "
               "(owner round 3 NEW-01 PASS)")
    sites = 0
    for rel in ("src/UI/TrainerViewScreenBase.inc", "src/UI/SaveSelectScreen.cpp", "src/UI/AppShellScreen.cpp"):
        f = src(rel)
        if not f:
            r.violations.append(f"{rel}: missing")
            continue
        found = 0
        for m in re.finditer(r"if \((?:helpOverlayActive|overlay == Overlay::Help[^)]*)\)\s*\{", f.code):
            block = f.code[m.end():m.end() + 260]
            if "kDown" not in block.split("return")[0]:
                continue  # a draw site, not input
            found += 1
            if "HidNpadButton_B" not in block.split("return")[0]:
                ln = f.code.count("\n", 0, m.start()) + 1
                r.violations.append(f"{rel}:{ln}: the Help input handler does not close on B")
        if not found:
            r.violations.append(f"{rel}: no Help input handler found")
        sites += found
    res.append(r)

    # Round 4 (owner round 3 FAIL items): helpers for the CG30+ contracts.
    def gen_files(g):
        return [s for s in U if GEN_FILE[g].search(Path(s.rel).name)]

    def lineof(s, idx):
        return s.code.count("\n", 0, idx) + 1

    # ---------- CG30: R2-TRAINER / R2-BUG-002 -- Trainer page reaches Review in Gen I-IV ----------
    r = Result("CG30-trainer-page-opens-review",
               "Gen I-IV Trainer page (Name / Money) opens the generation's own Review, as Gen II (Y) and Gen V (+) do")
    TRAINER_PAGE = re.compile(r"\btrainerMainPage\s*\(|\bgen1TrainerMainPage\s*\(|ViewMode::Trainer\b|Surface::Trainer\b")
    REVIEW_OPEN = {1: r"\bmode\s*=\s*(?:UX2Mode|Mode)::Review\b",
                   2: r"\breview\s*=\s*true\b",
                   3: r"\breview\s*=\s*true\b",
                   4: r"\breview\s*=\s*true\b"}
    shared_handle = src("include/UI/ClassicTrainerSessionInput.h")
    for g, rx in REVIEW_OPEN.items():
        ok = False
        for s in gen_files(g):
            for name, ln, body in function_bodies(s.code, re.compile(r".")):
                if TRAINER_PAGE.search(body) and re.search(rx, body):
                    ok = True; break
            if ok: break
        if not ok:
            r.violations.append(f"Gen {['','I','II','III','IV'][g]}: no Trainer-page input path opens its Review "
                                f"(no function both guarded by the Trainer page and setting {rx})")
    for name, ln, body in function_bodies(shared_handle.code, re.compile(r"^handle$")):
        if not re.search(r"HidNpadButton_(?:Plus|Y|X)\b", body):
            r.violations.append(f"{shared_handle.rel}:{ln}: ClassicTrainerSession::handle (Gen I/II/III/IV Trainer page) "
                                "handles only Up/Down/A; no Review input")
    ts = src("include/UI/ClassicTrainerSession.h")
    for name, ln, body in function_bodies(ts.code, re.compile(r"^trainerPageInstructions$")):
        if "Review" not in body:
            r.violations.append(f"{ts.rel}:{ln}: trainerPageInstructions() footer offers no Review")
    res.append(r)

    # ---------- CG31: NEW-19 / R2-BUG-002 -- Gen I-IV staged exit menu offers Review ----------
    r = Result("CG31-classic-exit-offers-review",
               "Leaving a Gen I-IV session with staged changes offers Review / Continue / Discard (Gen V ConfirmExit parity), not only Cancel / Discard & Exit")
    dlg = src("src/UI/Dialogs/SaveConfirmDialog.cpp")
    for fn in ("classicSaveConfirmFooter", "paintSaveConfirmContents"):
        for name, ln, body in function_bodies(dlg.code, re.compile("^" + fn + "$")):
            m = re.search(r"if\s*\(\s*screen\.exitingWithUnsavedChanges\s*(?:\)|&&)", body)
            exit_part = body
            if not re.search(r'"[^"\n]*\bReview\b[^"\n]*"', exit_part):
                r.violations.append(f"{dlg.rel}:{ln}: {fn}() exit-with-staged-changes choices have no Review "
                                    "(only Keep Editing/Cancel and Discard & Exit)")
    base = src("src/UI/TrainerViewScreenBase.inc")
    m = re.search(r"if\s*\(\s*saveConfirmActive\s*\)\s*\{\s*if\s*\(\s*exitingWithUnsavedChanges\s*\)\s*\{", base.code)
    if not m:
        r.violations.append(f"{base.rel}: classic exit handler not found (update this contract)")
    else:
        depth, j = 1, m.end()
        while depth and j < len(base.code):
            depth += {"{": 1, "}": -1}.get(base.code[j], 0); j += 1
        block = base.code[m.end():j]
        im = re.search(r"if\s*\(\s*itemsShortcutActive\s*\)\s*\{", block)
        if im:  # drop the Product Home -> Items shortcut prompt; the session exit is what follows it
            d, k = 1, im.end()
            while d and k < len(block):
                d += {"{": 1, "}": -1}.get(block[k], 0); k += 1
            block = block[k:]
        if not re.search(r"HidNpadButton_(?:X|Plus)\b|[Rr]eview\w*\s*\(|[Rr]eview\w*\s*=\s*true", block):
            r.violations.append(f"{base.rel}:{lineof(base, m.start())}: exit-with-unsaved-changes input handles only "
                                "A (Discard & Exit) and B (Cancel); no Review route")
    res.append(r)

    # ---------- CG32: R2-REVIEWMOVE -- Review rows name moves / species / items ----------
    r = Result("CG32-review-rows-readable-names",
               "Review rows show move / species / item NAMES, never raw numeric IDs (owner R2-REVIEWMOVE)")
    g1 = src("src/Integration/Gen1/Gen1StagedPokemonEditor.cpp")
    for mm in re.finditer(r'\bnumber\s*\(\s*"move"', g1.code):
        r.violations.append(f"{g1.rel}:{lineof(g1, mm.start())}: Gen I Review 'Move n' row formats the move ID with number()/to_string")
    g1ui = src("src/UI/Gen1PokemonEditorOverlay.cpp")
    for name, ln, body in function_bodies(g1ui.code, re.compile(r"^pendingLines$")):
        if re.search(r"std::to_string\s*\(\s*change\.itemId\s*\)", body):
            r.violations.append(f"{g1ui.rel}:{ln}: Gen I Review inventory row prints 'Inventory item <id>' (numeric item ID)")
    g2 = src("src/Integration/Gen2/Gen2StagedEditor.cpp")
    for mm in re.finditer(r'setChange\s*\([^;]*"(Moves|Species)"[^;]*;', g2.code):
        stmt = mm.group(0)
        if re.search(r"\barray4\s*\(\s*\w+\.moves\b|std::to_string\s*\(\s*[\w>.\-]*species\s*\)", stmt):
            r.violations.append(f"{g2.rel}:{lineof(g2, mm.start())}: Gen II Review '{mm.group(1)}' row prints numeric IDs")
    g5 = src("src/UI/Gen5SharedReadOnlySurface.inc")
    for name, ln, body in function_bodies(g5.code, re.compile(r"^reviewedFieldLabel$")):
        generic = re.search(r"std::to_string\s*\(\s*c\.before\s*\)", body)
        for field, lookup in (("Move", r"\bgetMoveName\s*\(|\bmove\w*(?:Name|Label|Display)\w*\s*\("),
                              ("HeldItem", r"\bgetItemName\w*\s*\(|\b\w*[iI]temLabel\w*\s*\(|\bmachineDisplayLabel\s*\("),
                              ("Species", r"\bgetSpeciesName\s*\(")):
            if generic and not re.search(lookup, body):
                r.violations.append(f"{g5.rel}:{ln}: Gen V reviewedFieldLabel prints Field::{field} as raw "
                                    "'before -> after' numbers (no name lookup)")
    res.append(r)

    # ---------- CG33: L06 / E02 / NEW-20 -- Gen I-III Party A / tap opens Actions ----------
    r = Result("CG33-party-opens-actions",
               "Gen I-III Party A / tap opens the Pokemon Actions popup (View / Edit / Origin & Legality / Cancel), never straight to View")
    for name, ln, body in function_bodies(base.code, re.compile(r"^openPokemonActionSheet$")):
        cut = body.find("openActionSheetTargetDetails")
        prefix = body if cut < 0 else body[:cut]
        if cut >= 0 and not re.search(r"PokemonLocation::Party\b[\s\S]*?\breturn\b", prefix):
            r.violations.append(f"{base.rel}:{ln}: openPokemonActionSheet() sends a Gen I-V Party target straight to "
                                "openActionSheetTargetDetails(true) (read-only View)")
    r.violations += [v + "  <- Gen III Party opens View" for v in lit_hits(sources, r"open in View")]
    PARTY = re.compile(r"ViewMode::Party\b|PokemonLocation::Party\b|TargetKind::Party\b|\bpartyEntry\w*")
    ACTIONS_OPEN = {1: r"\bmode\s*=\s*(?:UX2Mode|Mode)::Actions\b|\bopenActions\s*\(",
                    2: r"\bpokemonActions\s*=\s*true\b",
                    3: r"\bbeginActions\s*\(|\bactions\s*=\s*true\b",
                    4: r"\bbeginActions\s*\(|\bactions\s*=\s*true\b"}
    for g, rx in ACTIONS_OPEN.items():
        if not any(PARTY.search(b) and re.search(rx, b)
                   for s in gen_files(g) for _, _, b in function_bodies(s.code, re.compile(r"."))):
            r.violations.append(f"Gen {['','I','II','III','IV'][g]}: no Party input path opens its Pokemon Actions ({rx})")
    res.append(r)

    # ---------- CG34: NEW-17 -- Gen V Key Items / TMs & HMs are editable ----------
    r = Result("CG34-gen5-key-tm-pockets-editable",
               "Gen V Bag Key Items and TMs & HMs offer staged Add / Remove (with the shared Key Item warning), no blanket view-only")
    bag = src("include/Integration/Gen5/Gen5Bag.h")
    scr = src("include/Integration/Gen5/Gen5BagScreen.h")
    POCKET_BAN = re.compile(r"BagPocket5::KeyItems\)[\s\S]{0,80}BagPocket5::Machines\)")
    for f, fn in ((scr, "mutablePocket"), (bag, "gen5BagItemAddable"), (bag, "remove")):
        for name, ln, body in function_bodies(f.code, re.compile("^" + fn + "$")):
            if POCKET_BAN.search(body):
                r.violations.append(f"{f.rel}:{ln}: {fn}() refuses the whole Key Items / TMs & HMs pocket")
    r.violations += lit_hits([s for s in sources if "Gen5" in s.rel], r"(Key Items?|TMs?)[^\n]*view only|story progress", re.I)
    g5files = [s for s in sources if re.search(r"Gen5", s.rel)]
    if not any(re.search(r"\bneedsKeyItemWarning\s*\(|\bdrawKeyItemWarning\s*\(", s.code) for s in g5files):
        r.violations.append("Gen V Bag never asks the shared Key Item warning (UI/SharedBagAddFlow.h needsKeyItemWarning)")
    res.append(r)

    # ---------- CG35: AUD-01 -- Gen V Review matches Gen IV Review ----------
    r = Result("CG35-gen5-review-matches-gen4",
               "Gen IV and Gen V Review show the same heading, footer actions and Discard-all confirmation")
    g4 = src("src/UI/Gen4SharedPokemonSurface.inc")
    def review_text(f, fns):
        heading, footer, confirm = set(), set(), set()
        for fn in fns:
            for name, ln, body in function_bodies(f.code, re.compile("^" + fn + "$")):
                for lit in re.findall(r'"([^"\n]*Review Pending Changes[^"\n]*)"', body): heading.add(lit)
                for nb in re.finditer(r"drawNavBar\s*\(\s*fb\s*,\s*\{([\s\S]*?)\}\s*\)\s*;", body):
                    footer |= set(re.findall(r'\{\s*"([^"]+)"\s*,\s*"([^"]+)"\s*\}', nb.group(1)))
                for lit in re.findall(r'"([^"\n]*[Dd]iscard [Aa]ll[^"\n]*\?)"', body): confirm.add(lit.lower() and lit)
        return heading, footer, confirm
    h4, f4, _ = review_text(g4, ("drawReview",))
    h5, f5, _ = review_text(g5, ("drawReview",))
    def pairs(text): return set(re.findall(r'\{\s*"([^"]+)"\s*,\s*"([^"]+)"\s*\}', text))
    c4 = set()
    for _, _, b in function_bodies(g4.code, re.compile(r"^drawDiscardAllConfirm$")):
        c4 = {(t.lower(), frozenset(pairs(nb))) for t in re.findall(r'"(Discard all[^"]*\?)"', b, re.I)
              for nb in re.findall(r"drawNavBar\s*\(\s*fb\s*,\s*\{([\s\S]*?)\}\s*\)\s*;", b)}
    c5 = {(m.group(1).lower(), frozenset(pairs(m.group(2)))) for m in re.finditer(
          r'drawDecision\s*\(\s*screen\s*,\s*fb\s*,\s*"(Discard All[^"]*\?)"[\s\S]*?(\{\{[\s\S]*?\}\})\s*\)\s*;', g5.code, re.I)}
    if h4 != h5: r.violations.append(f"Review heading: Gen IV {sorted(h4)} vs Gen V {sorted(h5)}")
    act = lambda f: {(g, l) for g, l in f if g in ("A", "B", "X", "Y", "+")}  # actions, not paging hints
    f4, f5 = act(f4), act(f5)
    if f4 != f5: r.violations.append(f"Review footer: Gen IV {sorted(f4)} vs Gen V {sorted(f5)}")
    if c4 != c5:
        fmt = lambda c: [(t, sorted(p)) for t, p in c]
        r.violations.append(f"Discard-all confirmation: Gen IV {fmt(c4)} vs Gen V {fmt(c5)}")
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
