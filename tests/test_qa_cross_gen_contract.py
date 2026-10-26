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
import math
import re
import shutil
import subprocess
import sys
import tempfile
from dataclasses import dataclass, field
from functools import lru_cache
from pathlib import Path

DEFAULT_ROOT = Path(__file__).resolve().parent.parent

# Contract id -> owning lane, while the fix is not yet on the integration branch.
# Remove an entry when its contract XPASSes; it is then ENFORCED.
XFAIL: dict[str, str] = {
    # Round 4: owner round 3 FAIL items (ROUND3_RESULTS_62a2f8c3.md); owners per #307 MAIN_ACTIVE_HANDOFF.md §6b.
    # CG01-CG11 XPASSed on candidate 7fea1797 (app-qa-nro-audit run 38947904299) and are ENFORCED;
    # CG12 and CG14 XPASSed on #307 d104310 (batch 1), CG18 on 41cbe92 (batch 2), CG13 and CG15 on
    # d51c1ff (batch 3), CG16 and CG17 on 348129f (batch 4): ENFORCED.
    # Round 3: CG21/CG26/CG28 on #307 aeccd13, CG20/CG24/CG25/CG27 on 7ed1cc16, CG20b on 7ed1cc16,
    # CG18b on 11940524, CG19 on 8a84128e, CG20c/CG22/CG23 on lane P 8d533b52: ENFORCED.
    # Round 4: CG39 (P 05d1d79), CG57 / CG58 (coordinator c59ac178) on #307 3d72654f: ENFORCED.
    # Round 5: CG62 / CG63 (P 1821c921) and CG65-CG67 (C 4e1532d3) on #307 fc78fef9, CG61 (L 1eec6f18), CG68 (C 9f034431): ENFORCED.
    # Round 6: CG76 (R 8613b13d) on #307 c3d64423: ENFORCED.
    # Round 2 contracts are XFAIL only until their lane's push is on #307.
    # Round 5: owner Oct 24 FAIL / request gaps confirmed by their lane (via the coordinator).
    # Round 6: owner Oct 25 FAILs and requests (docs/hardware/ROUND5_RESULTS_1ca2faec.md), owners per the
    # coordinator's round 6 assignment. CG69 / CG70 / CG71 / CG74 / CG79 (S; CG74 Gen I/II C), CG72 (C), CG73 (L), CG76 / CG77 (R)
    # and CG75 / CG78 (P): ENFORCED.
    # Round 7: owner Oct 26 FAILs and requests (docs/hardware/ROUND6_RESULTS_26186464.md), owners per the
    # coordinator's round 7 assignment.
    "CG81-one-review-key-per-screen": "L (round 7 B08 defect 2)",
    "CG83-classic-trainer-gen5-tokens": "C (round 7 B06 / C06)",
    "CG84-gen2-special-lower-middle": "C (round 7 A22 / C01)",
    # CG87 (P e52b7d97) and CG88 (P e52b7d97, one early outside-tap dispatcher in SaveSelectScreen::update)
    # on #307 2de1c390, and CG85 (R 47b8097e, Gen2UnownLetter.h questions + Letter grid): ENFORCED.
    # CG80 (L 146511e2, the outer Gen II draw shows an open Review first) and CG82 (L ab46883e, this
    # profile's exports/gen5/account-<p> records, EXPORT_MANIFEST.txt) on #307 ab46883e: ENFORCED.
    # CG89 (S b9e8e517, include/UI/SharedPanelHeading.h for every Gen I-V editor heading): ENFORCED.
    "CG90-gen5-trainer-review-slot": "C (round 7 B06 / C06, Gen V)",
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


@lru_cache(maxsize=2048)
def function_definitions(code: str) -> tuple[tuple[str, int, str], ...]:
    """Parse an exact source string once; changed mutation inputs have different cache keys."""
    definitions = []
    for m in re.finditer(r"\b([A-Za-z_]\w*)\s*\([^;{}()]*(?:\([^;{}()]*\)[^;{}()]*)*\)\s*(?:const\s*)?(?:noexcept\s*)?\{", code):
        name = m.group(1)
        if name in ("if", "for", "while", "switch", "catch"):
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
        definitions.append((name, code.count("\n", 0, m.start()) + 1, code[m.end():j]))
    return tuple(definitions)


def function_bodies(code: str, name_rx: re.Pattern):
    """Yield the same definition tuples, filtered without reparsing unchanged source."""
    return (item for item in function_definitions(code) if name_rx.search(item[0]))


@dataclass
class Result:
    cid: str
    title: str
    violations: list[str] = field(default_factory=list)
    out_of_scope: list[str] = field(default_factory=list)


def round4_p2_contracts(sources):
    """Round 4 P2: owner round 3 FAIL items E03, R2-REROLL (Gen II Unown portrait), R2-SPECIAL, M04/M05,
    NEW-12 / R2-CHECK-011, NEW-14, NEW-15. Called from contracts()."""
    res = []
    U = ui(sources)

    def src(rel):
        return next((x for x in sources if x.rel == rel), None)

    def line_of(code, pos):
        return code.count("\n", 0, pos) + 1

    # Gen I-V UI only: Gen VI+ generic surfaces (PokemonDetailsModal.cpp) are CG11's out-of-scope list.
    GEN15_UI = [s for s in U if s.rel not in OUT_OF_SCOPE_WHEN_CG11_HOLDS]

    def gen_of(rel):
        name = Path(rel).name
        if name in ("SharedPokemonEditorSurface.inc",):
            return "IV/V"
        for g in (1, 2, 3, 4, 5):
            if GEN_FILE[g].search(name):
                return {1: "I", 2: "II", 3: "III", 4: "IV", 5: "V"}[g] if not (g == 1 and "Gen12" in name) else "I/II"
        return None

    # ------------------------------------------------------------------------------------------
    # E03a: Origin & Legality never shows a clean verdict, and every Legality value is the ONE
    # SharedEditorPresentation::legalityLabel(LegalityState) (one state per generation).
    # ------------------------------------------------------------------------------------------
    r = Result("CG48-origin-legality-truthful-state",
               "Every Gen I-V Origin & Legality / Legality value comes from SharedEditorPresentation::"
               "legalityLabel(); no clean verdict (\"... OK\" / PASS / Valid) under a Legality label; one "
               "LegalityState per generation (E03)")
    row_rx = re.compile(r'(?:"Legality"|[\w:]*kOriginLegalityHeading)\s*,\s*')
    label_rx = re.compile(r'(?:[\w:]*::)?legalityLabel\s*\(\s*(?:[\w:]*::)?LegalityState::(\w+)')
    claim_rx = re.compile(r"check|claim|legal|\bOK\b|PASS|valid|incomplete|verified", re.I)
    scope_extra = {"include/UI/Gen2WorkspacePresentation.h", "include/UI/Gen2NativePresentation.h",
                   "src/UI/TrainerViewScreenGSCOverlay.inc"}
    E03_FILES = [s for s in GEN15_UI if EDITOR_FILE_RX.search(s.rel) or s.rel in scope_extra]
    states: dict[str, set] = {}
    for s in E03_FILES:
        for m in row_rx.finditer(s.code):
            tail = s.code[m.end():m.end() + 300]
            if re.match(r"(?:Colors::|Color\(|TextStyle::|[\w:]*kOriginLegality|\w+\s*,)", tail):
                continue  # heading / caption drawText, not a value
            lm = label_rx.match(tail)
            ln = line_of(s.code, m.start())
            lit = re.match(r'"([^"]*)"', tail)
            if lm:
                g = gen_of(s.rel)
                if g:
                    states.setdefault(g, set()).add((lm.group(1), f"{s.rel}:{ln}"))
            elif (lit and claim_rx.search(lit.group(1))) or (not lit):
                r.violations.append(f"{s.rel}:{ln}: Legality value not from legalityLabel(): "
                                    f"{' '.join(tail.split())[:80]}")
        # hand-drawn Legality status next to the heading / a "Legality" caption
        for m in re.finditer(r'drawText\([^;]*?(?:kOriginLegalityHeading|"Legality")[^;]*;\s*\n\s*fb\.drawText\('
                             r'[^;]*?"(Not checked[^"]*|NOT CLAIMED[^"]*|Incomplete|OK|PASS|Valid)"', s.code):
            r.violations.append(f"{s.rel}:{line_of(s.code, m.start())}: hand-written Legality status "
                                f"\"{m.group(1)}\" instead of legalityLabel()")
    for ln, lit in [x for s in GEN15_UI for x in []]:
        pass
    r.violations += [v + "  (free-text legality status)" for v in
                     lit_hits(GEN15_UI, r"^Origin & Legality:\s")]
    # Any *Legality*Text() helper that returns a positive verdict word.
    for s in sources:
        if not s.rel.startswith(("include/UI/", "src/UI/")):
            continue
        for name, ln, body in function_bodies(s.code, re.compile(r"[Ll]egality\w*Text$")):
            for lm in re.finditer(r'return\s+"([^"]*)"', body):
                if re.search(r"\bOK\b|PASS|\bValid\b|\bLegal\b|No problems", lm.group(1)):
                    r.violations.append(f"{s.rel}:{ln + body.count(chr(10), 0, lm.start())}: {name}() "
                                        f"returns clean verdict \"{lm.group(1)}\"")
    for g, st in sorted(states.items()):
        if len({x[0] for x in st}) > 1:
            r.violations.append(f"Gen {g}: Legality rows use different states "
                                f"{sorted(f'{a}@{b}' for a, b in st)}")
    res.append(r)

    # E03b: one shared Origin & Legality panel. Every generation's modal draws its heading and rows
    # through ONE shared drawer instead of a per-generation hand layout (proposed name:
    # UI::SharedEditorChrome::drawOriginLegalityPanel, include/UI/SharedEditorChrome.h).
    r = Result("CG49-one-origin-legality-panel",
               "Gen I-V Origin & Legality modals are drawn by ONE shared panel (SharedEditorChrome::"
               "drawOriginLegalityPanel), not per-generation hand layouts (E03 'unify layout')")
    chrome = src("include/UI/SharedEditorChrome.h")
    if not chrome or not re.search(r"\bdrawOriginLegalityPanel\s*\(", chrome.code):
        r.violations.append("include/UI/SharedEditorChrome.h: no shared drawOriginLegalityPanel()")
    for s in GEN15_UI:
        if s.rel == "include/UI/SharedEditorChrome.h":
            continue
        for m in re.finditer(r"drawText\([^;]*kOriginLegalityHeading[^;]*TextStyle::Heading\s*\)", s.code):
            r.violations.append(f"{s.rel}:{line_of(s.code, m.start())}: per-generation Origin & Legality "
                                "panel (hand-drawn heading + rows)")
    res.append(r)

    # ------------------------------------------------------------------------------------------
    # R2-REROLL / Unown: the Gen II editor portrait shows the DV-derived Unown letter.
    # pret pokecrystal GetUnownLetter (engine/pokemon/unown? -> docs/REROLL_MECHANICS.md): letter =
    # ((Atk&6)<<5 | (Def&6)<<3 | (Spd&6)<<1 | (Spc&6)>>1) / 10. 14/10/10/10 -> V, 15/8/8/3 ->
    # ------------------------------------------------------------------------------------------
    r = Result("CG50-gen2-unown-portrait-follows-dvs",
               "Gen II editor portraits pass the DV-derived Unown letter to SpriteManager::getSprite"
               "(species, form, shiny); no species-only portrait in Gen II surfaces (R2-REROLL)")
    gen2_ui = [s for s in U if GEN_FILE[2].search(Path(s.rel).name) and "Gen12" not in s.rel]
    shell = src("include/UI/SharedPokemonShell.h")
    for s in gen2_ui:
        for m in re.finditer(r"SpriteManager::getSprite\(\s*([^,()]+(?:\([^()]*\))?)\s*,\s*([^,()]+(?:\([^()]*\))?)\s*\)",
                             s.code):
            r.violations.append(f"{s.rel}:{line_of(s.code, m.start())}: getSprite({m.group(1).strip()}, "
                                f"{m.group(2).strip()}) draws form 0 (Unown A) whatever the DVs")
        for m in re.finditer(r"SharedPokemonShell::drawPortrait\(([^;]*)\);", s.code):
            if len([a for a in re.split(r",(?![^()]*\))", m.group(1))]) < 6:
                r.violations.append(f"{s.rel}:{line_of(s.code, m.start())}: drawPortrait(...) without a form "
                                    "argument (SharedPokemonShell::drawPortrait uses getSprite(species, shiny))")
    if shell:
        for name, ln, body in function_bodies(shell.code, re.compile(r"^drawPortrait$")):
            if re.search(r"getSprite\(\s*species\s*,\s*shiny\s*\)", body):
                r.violations.append(f"{shell.rel}:{ln}: drawPortrait() has no form parameter")
    res.append(r)

    # ------------------------------------------------------------------------------------------
    # R2-SPECIAL: the Calculated column shows one number per row; Sp. Atk / Sp. Def are their own
    # labelled cells (pret pokecrystal CalcMonStats: one Special DV / Stat Exp, two stats).
    # ------------------------------------------------------------------------------------------
    r = Result("CG51-gen2-special-not-slashed",
               'Gen II never prints Sp. Atk and Sp. Def as one "a/b" Calculated cell (R2-SPECIAL)')
    gen2_files = gen2_ui + [x for x in [src("include/UI/Gen2WorkspacePresentation.h")] if x]
    seen = set()
    for s in gen2_files:
        if s.rel in seen:
            continue
        seen.add(s.rel)
        for m in re.finditer(r'\[\s*4\s*\][^;\n]*"\s*/\s*"[^;\n]*\[\s*5\s*\]|specialAttack[^;\n]*"\s*/\s*"[^;\n]*specialDefense',
                             s.code):
            r.violations.append(f"{s.rel}:{line_of(s.code, m.start())}: {s.code.splitlines()[line_of(s.code, m.start()) - 1].strip()}")
    # Round 6 7A (lane C e39a92e2) put SpD on a STATS grid row of its own (row 5). It stays a calculated
    # value: the native record holds exactly one Special DV and one Special Stat Exp (5-entry arrays: HP,
    # Atk, Def, Spe, Special; pret pokecrystal), the Calculated-column painter only paints (no touch
    # target, no DV / Stat Exp access), and the editable DV / Stat Exp rows stop at the Special row.
    rec = src("include/Integration/Gen2/Gen2ReadOnlySave.h")
    if not rec or not (re.search(r"std::array<uint16_t,\s*5>\s+statExperience\b", rec.code) and
                       re.search(r"std::array<uint8_t,\s*5>\s+dvs\b", rec.code)):
        r.violations.append("include/Integration/Gen2/Gen2ReadOnlySave.h: the Gen II record no longer holds exactly "
                            "one Special DV and one Special Stat Exp (5-entry dvs / statExperience)")
    for s in gen2_ui:
        for name, ln, body in function_definitions(s.code):
            if name == "drawGen2CalculatedColumn" and re.search(r"touchButtons|\bdvs\b|statExperience", body):
                r.violations.append(f"{s.rel}:{ln}: the Gen II Calculated column (SpA / SpD) is interactive or reads "
                                    "a DV / Stat Exp: SpD must stay a calculated value, not an editable stat")
        for m in re.finditer(r"\b(?:dvs|statExperience)\s*\[\s*(?:5|kSpecialDefenseRow|W::kSpecialDefenseRow)\s*\]", s.code):
            r.violations.append(f"{s.rel}:{line_of(s.code, m.start())}: indexes a sixth DV / Stat Exp (Gen II has one "
                                "Special DV / Stat Exp shared by SpA and SpD)")
    res.append(r)

    # ------------------------------------------------------------------------------------------
    # M04/M05 (candidate geometry tripwire, see report): the Gen IV/V Category cell keeps at least
    # Layout::kGap clear of the type badge to its left in the chooser row.
    # ------------------------------------------------------------------------------------------
    r = Result("CG52-gen45-move-category-spacing",
               "Gen IV/V move Category element keeps >= SharedMovesPanel::Layout::kGap from the type badge "
               "and the Pwr/Acc cells (M04/M05)")
    mp = src("include/UI/SharedMovesPanel.h")
    if mp:
        def off(fn):
            m = re.search(r"int\s+" + fn + r"\(\)\s*const\s*noexcept\s*\{\s*return\s+x\s*\+\s*w\s*-\s*(\d+)\s*;", mp.code)
            return int(m.group(1)) if m else None
        type_off, cat_off, pwr_off = off("typeX"), off("categoryX"), off("powerRight")
        badge_h = re.search(r"kChooserBadgeHeight\s*=\s*(\d+)", mp.code)
        gap = re.search(r"kGap\s*=\s*(\d+)", mp.code)
        adv_src = src("include/UI/NunitoMetrics.h")
        adv = [int(x) for x in re.search(r"kAsciiAdvance\[95\]\s*=\s*\{([^}]*)\}", adv_src.code).group(1).split(",")
               if x.strip()]

        def width(text, px=17):
            u = sum(adv[ord(c) - 32] if 32 <= ord(c) < 127 else 1091 for c in text)
            return -(-u * px // 1000)
        if None not in (type_off, cat_off, pwr_off) and badge_h and gap:
            badge_w, g = 5 * int(badge_h.group(1)), int(gap.group(1))  # 5:1 artwork (ClassicTypeBadges)
            left = (type_off - cat_off) - badge_w
            widest_cat = max(width(c) for c in ("Physical", "Special", "Status", "Category"))
            right = (cat_off - pwr_off) - widest_cat - max(width(v) for v in ("120", "250", "Pwr", "—"))
            if left < g:
                r.violations.append(f"include/UI/SharedMovesPanel.h: chooser Category starts {left} px after "
                                    f"the {badge_w} px type badge (< kGap {g})")
            if right < g:
                r.violations.append(f"include/UI/SharedMovesPanel.h: Category to Pwr gap {right} px (< kGap {g})")
    res.append(r)

    # ------------------------------------------------------------------------------------------
    # NEW-12 / R2-CHECK-011: Gen IV/V Met Location picker. Native ids stay offered (no false
    # rejection) but rows are not all stamped with the record's own Met Lv., and an encounter source
    # for the Gen IV/V games exists to distinguish encounter-backed rows (PKHeX Encounters4*/5*).
    # ------------------------------------------------------------------------------------------
    r = Result("CG53-gen45-met-location-context",
               "Gen IV/V Met Location rows are not all labelled with the record's own Met Lv.; Gen IV/V "
               "encounter templates exist to mark encounter-backed rows; native ids remain offered (NEW-12)")
    g45 = [x for x in (src("src/UI/Gen4SharedPokemonSurface.inc"), src("src/UI/Gen5SharedReadOnlySurface.inc")) if x]
    for s in g45:
        for m in re.finditer(r'"\s*\|\s*Met Lv\.\s*"\s*\+\s*std::to_string\(\s*\w+\s*->\s*metLevel\(\)\s*\)', s.code):
            r.violations.append(f"{s.rel}:{line_of(s.code, m.start())}: every location row shows the record's "
                                "current Met Lv. (not an encounter level)")
    # Follow the isolated IV/V context catalogue; legacy II/III guardrails stay unchanged.
    data = src("include/Integration/Encounter/EncounterContext45Data.inc")
    games = {"Gen IV": ("diamond_nds", "pearl_nds", "platinum_nds", "heartgold_nds", "soulsilver_nds"),
             "Gen V": ("black_nds", "white_nds", "black2_nds", "white2_nds")}
    have = {lit for _, lit in data.literals} if data else set()
    for g, ids in games.items():
        missing = [i for i in ids if i not in have]
        if missing:
            r.violations.append(f"{g}: no encounter templates for {', '.join(missing)} "
                                "(EncounterContext45Data.inc), so no Met Location row can be marked encounter-backed")
    g4 = src("src/UI/Gen4SharedPokemonSurface.inc")
    body4 = "".join(b for _, _, b in function_bodies(g4.code, re.compile(r"^openMetLocationPicker$"))) if g4 else ""
    if "isGen4NativeMetLocation" not in body4:
        r.violations.append("Gen IV: openMetLocationPicker no longer offers every native location (false rejection)")
    g5 = src("src/UI/Gen5SharedReadOnlySurface.inc")
    if g5 and "metLocationChoices()" not in g5.code:
        r.violations.append("Gen V: Met Location picker no longer offers every native location (false rejection)")
    shared = src("src/UI/SharedPokemonEditorSurface.inc")
    context = src("include/Integration/Encounter/EncounterContext45.h")
    for s in g45:
        if "openContextualMetLocationPicker(" not in s.code or "pickerMetContext.detail(value," not in s.code:
            r.violations.append(f"{s.rel}: Met Location does not use the real contextual picker and row details")
    for s, tokens in ((shared, ("EncounterContext45::Catalogue context(", "context.choices()",
                        "state.pickerMetContext.choices(state.pickerOtherHistories)",
                        "pickerMetContext.stored()", "!unchangedLocation && !A::applyPicker")),
                      (context, ('#include "Integration/Encounter/EncounterContext45Data.inc"',
                                 "for (const auto& entry : kWild45)", "ancestry(generation(origin), species)",
                                 "native_.push_back(stored)", '"Gift / static / other history; unverified"'))):
        if not s:
            r.violations.append("Missing real Gen IV/V contextual picker/catalogue")
        else:
            for token in tokens:
                if token not in s.code:
                    r.violations.append(f"{s.rel}: missing encounter-context behavior {token}")
    res.append(r)

    # ------------------------------------------------------------------------------------------
    # NEW-14: Gen V Language is a native staged edit (PK5 byte 0x17, PKHeX PK5.Language).
    # ------------------------------------------------------------------------------------------
    r = Result("CG54-gen5-language-editable",
               "Gen V Language is a staged native edit: Field::Language exists, detailEditable() allows it, "
               "detailLocked() does not lock it (NEW-14)")
    rec = src("include/Integration/Gen5/Gen5StagedPokemonRecord.h")
    m = re.search(r"enum\s+class\s+Field\s*:\s*uint8_t\s*\{([^}]*)\}", rec.code) if rec else None
    if not m or not re.search(r"\bLanguage\b", m.group(1)):
        r.violations.append("include/Integration/Gen5/Gen5StagedPokemonRecord.h: StagedPokemon5Record::Field has no Language")
    bridge = src("include/Integration/Gen5/Gen5SharedEditorBridge.h")
    if not bridge or not re.search(
            r"case Shared::FieldIdentity::Language:\s*return session\.stage\(NativeField::Language,stat,value,error\)",
            bridge.code):
        r.violations.append("Gen5SharedEditorBridge.h: shared Language field does not reach native staged Language")
    fields = src("include/Integration/Gen5/Gen5EditorFields.h")
    for name, ln, body in function_bodies(fields.code, re.compile(r"^detailEditable$")):
        cm = re.search(r"case\s+Language\b", body)
        if not cm:
            r.violations.append(f"{fields.rel}:{ln}: detailEditable() leaves Language in default: return false")
    for name, ln, body in function_bodies(fields.code, re.compile(r"^detailLocked$")):
        if re.search(r"case\s+Language\b[^;]*return\s+true", body):
            r.violations.append(f"{fields.rel}:{ln}: detailLocked() locks Language")
    r.violations += [v for v in lit_hits(U, r"Language and Origin are read-only") if "Gen5" in v]
    res.append(r)

    # ------------------------------------------------------------------------------------------
    # NEW-15: Create defaults are IVs 0 (Gen III-V) and DVs 0 (Gen I-II). Create paths only.
    # ------------------------------------------------------------------------------------------
    r = Result("CG55-create-defaults-zero-ivs-dvs",
               "New Pokemon start with IVs 0 (Gen III-V) and DVs 0 (Gen I-II); existing records untouched (NEW-15)")

    def nonzero(lst):
        return any(int(x) != 0 for x in re.findall(r"\d+", lst))
    for rel, struct in (("include/Integration/Gen1/Gen1StagedPokemonEditor.h", "BoxPokemonCreate"),
                        ("include/Integration/Gen2/Gen2StagedEditor.h", "BoxPokemonCreate"),
                        ("include/Integration/Gen3/Gen3StagedPokemonEditor.h", "BoxPokemonCreate")):
        f = src(rel)
        sm = re.search(r"struct\s+" + struct + r"\s*\{(.*?)\n\};", f.code, re.S) if f else None
        if not sm:
            continue
        for fm in re.finditer(r"\b(dvs|ivs)\s*\{([^}]*)\}", sm.group(1)):
            if nonzero(fm.group(2)):
                r.violations.append(f"{rel}:{line_of(f.code, sm.start(1) + fm.start())}: {struct}::{fm.group(1)}"
                                    f"{{{fm.group(2)}}} default is not 0")
    f = src("src/UI/Gen2PokemonEditorFoundation.inc")
    for name, ln, body in function_bodies(f.code, re.compile(r"^defaultCreateRecord$")):
        for fm in re.finditer(r"applyEditableDVs\(\s*\w+\s*,\s*\{([^}]*)\}", body):
            if nonzero(fm.group(1)):
                r.violations.append(f"{f.rel}:{ln + body.count(chr(10), 0, fm.start())}: Gen II Create DVs "
                                    f"{{{fm.group(1)}}}")
    f = src("src/UI/TrainerViewScreenGSCOverlay.inc")
    if f:
        for fm in re.finditer(r'promptNumber\(\s*"Add Gen II Pokemon - "[^;]*?"\s*DV"\s*,\s*(\d+)', f.code):
            if int(fm.group(1)) != 0:
                r.violations.append(f"{f.rel}:{line_of(f.code, fm.start())}: Add Gen II DV prompt defaults to {fm.group(1)}")
    f = src("include/Integration/Gen5/Gen5SharedScreenState.h")
    for name, ln, body in function_bodies(f.code, re.compile(r"^sharedCreateDefaults$")):
        for fm in re.finditer(r"\.ivs\s*=\s*\{([^}]*)\}", body):
            if nonzero(fm.group(1)):
                r.violations.append(f"{f.rel}:{ln + body.count(chr(10), 0, fm.start())}: Gen V Create IVs "
                                    f"{{{fm.group(1)}}}")
    res.append(r)
    return res


def round4_p0_contracts(sources):
    """Round 4 P0: owner round 3 FAIL items NEW-04, C01/NEW-16, B08, R2-EMUBACKUP/C07, S05. Called from
    contracts()."""
    res = []

    def src(rel):
        return next((x for x in sources if x.rel == rel), None)

    def defs(rel, name):
        """[(start_line, body, body_offset)] for definitions of `name` in rel."""
        f = src(rel)
        if not f:
            return []
        out = []
        for m in re.finditer(r"\b" + re.escape(name) + r"\s*\([^;{}()]*(?:\([^;{}()]*\)[^;{}()]*)*\)\s*(?:const\s*)?(?:noexcept\s*)?\{", f.code):
            depth, j = 0, m.end() - 1
            while j < len(f.code):
                if f.code[j] == "{": depth += 1
                elif f.code[j] == "}":
                    depth -= 1
                    if depth == 0: break
                j += 1
            out.append((f.code.count("\n", 0, m.start()) + 1, f.code[m.end():j], m.end()))
        return out

    def line_of(rel, offset):
        return src(rel).code.count("\n", 0, offset) + 1

    def block_after(code, start):
        """Body of the brace block whose '{' is the first one at/after `start` -> (open_index, body)."""
        i = code.index("{", start)
        depth, j = 0, i
        while j < len(code):
            if code[j] == "{": depth += 1
            elif code[j] == "}":
                depth -= 1
                if depth == 0: break
            j += 1
        return i, code[i + 1:j]

    def callees(body):
        return set(re.findall(r"\b([A-Za-z_]\w{4,})\s*\(", body)) - {"if", "for", "while", "switch", "return"}

    def reaches(body, pattern, depth=2, seen=None):
        """True if body, or a function it calls (by name, any source file), matches pattern within depth."""
        rx = re.compile(pattern)
        if rx.search(body):
            return True
        if depth == 0:
            return False
        seen = set() if seen is None else seen
        for name in callees(body) - seen:
            seen.add(name)
            for b in fn_index().get(name, ()):
                if reaches(b, pattern, depth - 1, seen):
                    return True
        return False

    index_cache: dict[str, list[str]] = {}

    def fn_index():
        # One pass over every source: function name -> bodies (scanning all files per callee is too slow).
        if not index_cache:
            for s in sources:
                for n, _, b in function_bodies(s.code, re.compile(r".")):
                    index_cache.setdefault(n, []).append(b)
        return index_cache

    # ------------------------------------------------------------------ NEW-04
    r = Result("CG56-gen2-carry-refresh-without-finalize",
               "Gen II PC Move: the carry refresh never routes through finalizedBytes(), which refuses while a carry is active")
    OV, GSC, ED = "src/UI/ClassicPackedMoveOverlay.inc", "src/Legacy/GSCReadOnlyTrainer.cpp", "src/Integration/Gen2/Gen2StagedEditor.cpp"
    fin = defs(ED, "finalizedBytes")
    if not any(re.search(r"\bpackedMove_\s*\.\s*active\b", b) for _, b, _ in fin):
        r.violations.append(f"{ED}: StagedEditor::finalizedBytes() no longer refuses while a carry is active "
                            "(an export must never publish a save with the carried Pokemon removed)")
    begin = defs(OV, "beginGen2")
    if not begin or not re.search(r"beginPackedMove\s*\([\s\S]*\brefreshGen2\s*\(", begin[0][1]):
        r.violations.append(f"{OV}: beginGen2() must still refresh the staged box presentation after beginPackedMove()")
    methods = set()
    for _, b, _ in defs(OV, "refreshGen2"):
        methods |= set(re.findall(r"\btrainer\s*\.\s*(\w+)\s*\(", b))
    if not methods:
        r.violations.append(f"{OV}: refreshGen2() calls no GSCReadOnlyTrainer presentation method")
    for name in sorted(methods):
        found = defs(GSC, name)
        if not found:
            r.violations.append(f"{GSC}: {name}() (called by refreshGen2) is not defined here")
        for ln, body, off in found:
            reach = [(body, off)] + [(b2, o2) for c in callees(body) for _, b2, o2 in defs(GSC, c)]
            for b, o in reach:
                # NEW-04's implemented ternary uses presentationBytes only while carrying;
                # finalizedBytes remains valid on the explicitly non-carry branch. Any unguarded
                # or inverted call still fails, as does removing finalize's carry refusal above.
                b = re.sub(r"\b\w+\s*->\s*packedMoveActive\s*\(\s*\)\s*\?\s*"
                           r"\w+\s*->\s*presentationBytes\s*\([^)]*\)\s*:\s*"
                           r"\w+\s*->\s*finalizedBytes\s*\([^)]*\)", "", b)
                m = re.search(r"\bfinalizedBytes\s*\(", b)
                if m:
                    r.violations.append(f"{GSC}:{line_of(GSC, o + m.start())}: {name}() rebuilds the box view from "
                                        "StagedEditor::finalizedBytes(), which refuses while packedMove_.active, so "
                                        "beginGen2() cancels every pickup (\"Place or cancel carried Pokemon before "
                                        "finalizing the Gen II staged save\")")
    res.append(r)

    # ------------------------------------------------------------------ C01 / NEW-16
    r = Result("CG57-gen5-home-a-opens-assigned-save",
               "Gen V Home A opens the currently assigned save directly; Choose Save only on failure")
    SS = "src/UI/SaveSelectScreen.cpp"
    sel = defs(SS, "selectCurrentTitle")
    if not sel:
        r.violations.append(f"{SS}: selectCurrentTitle() is missing")
    for ln, body, off in sel:
        m = re.search(r"if\s*\(\s*selected\.sourceKind\s*==\s*SelectedSourceKind::Gen5AssignedFile\s*\)", body)
        if not m:
            r.violations.append(f"{SS}:{ln}: selectCurrentTitle() has no Gen5AssignedFile branch")
            continue
        bo, blk = block_after(body, m.end())
        bl = line_of(SS, off + m.start())

        def in_branch(pattern):
            # The branch itself or a function it calls (e.g. the coordinator's openAssignedGen5Directly()).
            return bool(re.search(pattern, blk)) or any(
                re.search(pattern, b) for name in callees(blk) for b in fn_index().get(name, ()))

        opener_rx = r"\bGen5::openAssignedSource\s*\("
        if not in_branch(opener_rx):
            r.violations.append(f"{SS}:{bl}: Home A on a Gen V card never opens the assigned source "
                                "(no Gen5::openAssignedSource in the branch or a function it calls)")
        if not in_branch(r"\bselectedGen5Fingerprint\s*=") or not in_branch(r"\btitleSelected\s*=\s*true\b"):
            r.violations.append(f"{SS}:{bl}: the Gen V A branch never selects the assigned save "
                                "(selectedGen5Fingerprint / titleSelected=true not set)")
        # The chooser is only a fallback: allowed after `if (opener(...)) return;` whose callee really opens
        # the assigned source, or nested under a failure condition.
        guard_end = -1
        for g in re.finditer(r"if\s*\(\s*(\w+)\s*\([^;{}]*\)\s*\)\s*return\s*;", blk):
            if blk[:g.start()].count("{") == blk[:g.start()].count("}") and \
                    any(re.search(opener_rx, b) for b in fn_index().get(g.group(1), ())):
                guard_end = g.end()
                break
        for c in re.finditer(r"\b(openGen5Setup|discoverGen5Candidates)\s*\(", blk):
            depth = blk[:c.start()].count("{") - blk[:c.start()].count("}")
            if depth == 0 and not (0 <= guard_end <= c.start()):
                r.violations.append(f"{SS}:{line_of(SS, off + bo + 1 + c.start())}: unconditional {c.group(1)}() "
                                    "on Home A opens the Choose Save popup instead of the assigned save")
    res.append(r)

    # ------------------------------------------------------------------ B08
    r = Result("CG58-export-outcome-visible",
               "Every Gen I-V surface that offers Export shows the export result/error while it stays open")
    STATUS = r"drawText\s*\([^;]*\bstorageStatus\b|\bdrawGSCOverlay\s*\(|\bdrawLegacyBase\s*\("
    CI = "src/UI/ClassicInventoryOverlay.cpp"
    # A frame-wide guarantee also satisfies every surface: the top-level TrainerViewScreen::draw holds a
    # scope guard whose destructor (run after every return path, i.e. LAST) calls a painter that draws
    # storageStatus (coordinator's B08 design, drawTransientStatus).
    painter_ok = lambda name: any(re.search(r"drawText\s*\([^;]*\bstorageStatus\b", b) for b in fn_index().get(name, ()))
    frame_wide = False
    for _, body, _ in defs("src/UI/TrainerViewScreenCompositeOverlay.cpp", "draw"):
        for d in re.finditer(r"~\w+\s*\(\s*\)\s*\{([^{}]*)\}", body):
            frame_wide |= any(painter_ok(n) for n in re.findall(r"\b(\w+)\s*\(", d.group(1)))
    for ln, body, off in ([] if frame_wide else defs(CI, "drawOverlay")):
        m = re.search(r"if\s*\(\s*state\.optionsActive\s*\)", body)
        if not m:
            continue
        _, blk = block_after(body, m.end())
        closes = False
        for h_ln, h_body, _ in defs(CI, "handleInput"):
            e = re.search(r"\bexportStagedCopy\s*\(", h_body)
            if e:
                seg = h_body[e.start():h_body.find("break;", e.start())]
                closes = bool(re.search(r"\boptionsActive\s*=\s*false\b", seg))
        if not reaches(blk, STATUS, depth=1) and not closes:
            r.violations.append(f"{CI}:{line_of(CI, off + m.start())}: Inventory Options (Gen I-III Bag) stays open "
                                "over the status toast after Export Staged Save and never draws the result/error")
    for rel, exporter in (("src/UI/Gen3SharedPokemonSurface.inc", "exportStagedGen3Copy"),
                          ("src/UI/Gen4SharedPokemonSurface.inc", "exportStagedGen4Copy"),
                          ("src/UI/Gen5SharedReadOnlySurface.inc", None)):
        found = [] if frame_wide else defs(rel, "drawReview")
        if not found and not frame_wide:
            r.violations.append(f"{rel}: drawReview() is missing")
        for ln, body, off in found:
            fills = [x.end() for x in re.finditer(r"drawFilledRect\s*\(\s*0\s*,\s*0\s*,\s*fb\.getWidth\(\)\s*,\s*fb\.getHeight\(\)\s*,", body)]
            tail = body[fills[-1]:] if fills else body
            if not reaches(tail, STATUS, depth=1):
                r.violations.append(f"{rel}:{ln}: drawReview() owns the frame (Export Copy on X) but never draws the "
                                    f"status line that {exporter or 'the exporter'}() reports success/failure on")
    res.append(r)

    # ------------------------------------------------------------------ R2-EMUBACKUP / C07
    r = Result("CG59-backup-browser-actions",
               "Emulator Backups browser can open the current save and deliberately create a backup (not notice-only)")
    WRITE = r"\bcopyFile\s*\(|\bcopyDirectory\w*\s*\(|\bpublishVerified\w*\s*\(|AtomicTextFile::replace\s*\(|\bfwrite\s*\(|\bbackupSaveData\s*\("
    ups = defs(SS, "update")
    blk = None
    for ln, body, off in ups:
        m = re.search(r"if\s*\(\s*overlay\s*==\s*Overlay::EmulatorBackups\s*\)", body)
        if m:
            bo, blk = block_after(body, m.end())
            bl = line_of(SS, off + m.start())
    if blk is None:
        r.violations.append(f"{SS}: no Overlay::EmulatorBackups input handler")
    else:
        if not reaches(blk, r"\bselectCurrentTitle\s*\(|\btitleSelected\s*=\s*true\b", depth=2):
            r.violations.append(f"{SS}:{bl}: Backups browser cannot open/browse the current save (A only sets a notice)")
        # update -> createEmulatorBackup -> createBackup -> writeAll. Follow the real copy
        # boundary, rather than requiring fopen/fwrite directly inside the screen handler.
        if not reaches(blk, WRITE, depth=3):
            r.violations.append(f"{SS}:{bl}: Backups browser has no deliberate Create Backup action "
                                "(no path reaches a file write; rows are only listed export records)")
    res.append(r)

    # ------------------------------------------------------------------ S05 / R2-EMUBACKUP
    r = Result("CG60-no-backup-on-unchanged-open",
               "Opening a save (Home A / Items) never creates a backup before any change is made")
    UI_ = "src/UI/UI.cpp"
    for name in ("handleDefaultQuickOpen", "handleItemsQuickOpen", "handleLegacyFRLGView", "handleGen4View", "handleGen5View"):
        for ln, body, off in defs(UI_, name):
            m = re.search(r"\bbackupSaveData\s*\(|\bcopyFile\s*\(|\bcopyDirectory\w*\s*\(", body)
            if m:
                r.violations.append(f"{UI_}:{line_of(UI_, off + m.start())}: {name}() creates a backup on every open, "
                                    "before the user changed anything")
    res.append(r)
    return res


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
    modal_rx = re.compile(r"^(?:drawOriginLegalityPanel|draw(?!\w*(Frame|Chrome|Surface|Footer|Shell|Backdrop))\w*(Confirm|Decision|Modal|Dialog|Prompt)\w*)$")
    footer_rx = re.compile(r"\b(drawNavBar|drawFooter\w*|drawGlyphButton|drawControllerHints|drawPickerFooter\w*)\s*\(")
    # Bag wrappers delegate their in-dialog choices and blank covered footer
    # to drawConfirm. Follow that real call, rather than requiring a duplicate
    # footer at the generation-specific call site (owner NEW-03).
    bag_confirm = next((s for s in sources if s.rel == "include/UI/SharedBagAddPicker.h"), None)
    shared_bag_footers = set()
    if bag_confirm:
        confirm_bodies = [b for _, _, b in function_bodies(bag_confirm.code, re.compile(r"^drawConfirm$"))]
        if confirm_bodies and all(footer_rx.search(b) and STRICT_LAYER_RX.search(b) for b in confirm_bodies):
            for n, _, b in function_bodies(bag_confirm.code, re.compile(r"^draw(?:RemoveConfirm|KeyItemWarning)$")):
                if re.search(r"\bdrawConfirm\s*\(", b):
                    shared_bag_footers.add(n)

    def delegated_bag_footer(body):
        return any(re.search(r"\bSharedBagAddPicker::" + n + r"\s*\(", body)
                   for n in shared_bag_footers)
    origin_panel = next((s for s in sources if s.rel == "include/UI/SharedEditorChrome.h"), None)
    origin_owns_frame = origin_panel and any(
        footer_rx.search(b) and STRICT_LAYER_RX.search(b)
        for _, _, b in function_bodies(origin_panel.code, re.compile(r"^drawOriginLegalityPanel$")))

    review_owns_frame = origin_panel and any(
        footer_rx.search(b) and STRICT_LAYER_RX.search(b)
        for _, _, b in function_bodies(origin_panel.code, re.compile(r"^drawReviewDiscardConfirm$")))

    def delegated_origin_footer(body):
        return (origin_owns_frame and re.search(r"\bSharedEditorChrome::drawOriginLegalityPanel\s*\(", body)) or (
            review_owns_frame and re.search(r"\bSharedEditorChrome::drawReviewDiscardConfirm\s*\(", body))

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
            if not footer_rx.search(body) and not delegated_bag_footer(body) and not delegated_origin_footer(body) and not every_caller(name, s.rel, ln, 0, 30, footer_rx):
                r.violations.append(f"{s.rel}:{ln}: {name}() draws no footer of its own")
            if not layer_rx.search(body) and not delegated_origin_footer(body) and not every_caller(name, s.rel, ln, 20, 0, layer_rx):
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
            if fn == "paintSaveConfirmContents" and re.search(r"SharedEditorChrome::drawStagedExitPanel\s*\(", body):
                shared_exit = src("include/UI/SharedEditorChrome.h")
                panels = [b for _, _, b in function_bodies(shared_exit.code, re.compile(r"^drawStagedExitPanel$"))]
                if len(panels) != 1:
                    r.violations.append("The shared staged-exit presenter is missing or ambiguous")
                exit_part = "\n".join(panels)
                for glyph, label, touch_id in (("A", "Review Changes", 2), ("B", "Continue Editing", 0),
                                               ("X", "Export Copy", 3), ("Y", "Discard & Exit", 1)):
                    if not re.search(r'choice\([^;]*"'+glyph+r'"\s*,\s*"'+re.escape(label)+r'"\s*,\s*'+str(touch_id)+r'\s*\)', exit_part):
                        r.violations.append(f"Shared staged exit is missing {glyph} {label} / touch {touch_id}")
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
        if cut >= 0 and not ("ClassicPartyActions::open(" in prefix and "PokemonLocation::Party" in prefix):
            r.violations.append(f"{base.rel}:{ln}: openPokemonActionSheet() sends a Gen I-V Party target straight to "
                                "openActionSheetTargetDetails(true) (read-only View)")
    r.violations += [v + "  <- Gen III Party opens View" for v in lit_hits(sources, r"open in View")]
    PARTY = re.compile(r"ViewMode::Party\b|PokemonLocation::Party\b|TargetKind::Party\b|\bpartyEntry\w*")
    ACTIONS_OPEN = {1: r"\bmode\s*=\s*(?:UX2Mode|Mode)::Actions\b|\bopenActions\s*\(",
                    2: r"\bpokemonActions\s*=\s*true\b",
                    3: r"\bbeginActions\s*\(|\bactions\s*=\s*true\b",
                    4: r"\bbeginActions\s*\(|\bactions\s*=\s*true\b"}
    for g, rx in ACTIONS_OPEN.items():
        if g <= 3:
            # Party has one real popup before all older generation-specific action wrappers.
            common=src("src/UI/ClassicPartyActions.inc")
            composite=src("src/UI/TrainerViewScreenCompositeOverlay.cpp")
            cases="".join(b for n, _, b in function_bodies(common.code,re.compile(r"^open$"))) if common else ""
            action_rx=r"Gen1PokemonEditor::UX2Mode::Actions" if g==1 else rx
            if not common or f"case {g}:" not in cases or not re.search(action_rx,cases):
                r.violations.append(f"Gen {g}: shared Party sheet never opens its native Actions state")
            if not composite or '#include "ClassicPartyActions.inc"' not in composite.code or "ClassicPartyActions::handle(*this" not in composite.code or "ClassicPartyActions::draw(*this" not in composite.code:
                r.violations.append("Shared Party Actions is missing from actual frame input/draw")
            if common and "actionsForSlot(true,{false,false,true},false)" not in common.code:
                r.violations.append("Shared Party Actions no longer offers View/Edit/Origin/Cancel")
            backend=src(f"src/Integration/Gen{g}/Gen{g}StagedPokemonEditor.cpp" if g!=2 else "src/Integration/Gen2/Gen2StagedEditor.cpp")
            if not backend or "stagePartyPokemonEdit(" not in backend.code:
                r.violations.append(f"Gen {g}: Party Edit has no native staged writer")
            adapter=src("src/UI/Gen1PokemonEditorOverlayFoundation.inc" if g==1 else
                f"include/UI/Gen{g}{'PokemonSession' if g==2 else 'SharedPokemonSession'}.h")
            if not adapter or "stagePartyPokemonEdit(" not in adapter.code:
                r.violations.append(f"Gen {g}: real Keep never invokes the native Party staged writer")
            continue
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
    shared = src("include/UI/SharedEditorChrome.h")
    model = src("include/UI/SharedReviewPresentation.h")
    for generation, adapter in (("IV",g4),("V",g5)):
        review_bodies=[body for _,_,body in function_bodies(adapter.code,re.compile(r"^drawReview$"))]
        if not review_bodies or not all(re.search(r"SharedEditorChrome::drawReviewPanel\s*\(",body) for body in review_bodies):
            r.violations.append(f"Gen {generation}: Review does not use the common panel")
        if not re.search(r"SharedEditorChrome::drawReviewDiscardConfirm\s*\(",adapter.code):
            r.violations.append(f"Gen {generation}: Discard All does not use the common confirmation")
    panels=[body for _,_,body in function_bodies(shared.code,re.compile(r"^drawReviewPanel$"))] if shared else []
    if not panels or not all('"Review Pending Changes"' in body and
        re.search(r"drawFooter\s*\(\s*fb\s*,\s*R::footer\s*\(",body) and
        re.search(r"beginModal\s*\(",body) for body in panels):
        r.violations.append("Shared Review panel must own touch and draw the shared heading/footer")
    for glyph,label in (("X","Export Copy"),("Y","Discard All"),("B","Back")):
        if not model or not re.search(r'push\(\s*"'+glyph+r'"\s*,\s*"'+label+r'"\s*\)',model.code):
            r.violations.append(f"Shared Review footer is missing {glyph} {label}")
    confirms=[body for _,_,body in function_bodies(shared.code,re.compile(r"^drawReviewDiscardConfirm$"))] if shared else []
    if not confirms or not all('"Discard all staged changes?"' in body and
        re.search(r'\{\s*"Y"\s*,\s*"Discard All"',body) and
        re.search(r'\{\s*"B"\s*,\s*"Keep Changes"',body) and
        re.search(r"beginModal\s*\(",body) for body in confirms):
        r.violations.append("Shared Discard All question must own touch and offer Y Discard / B Keep")
    res.append(r)

    # Round 4 P3 (owner round 3 FAIL, UI polish): CG36-CG47.
    def bodies_ln(rel, name):
        f = src(rel)
        return [(ln, b) for _, ln, b in function_bodies(f.code, re.compile("^" + name + "$"))] if f else []


    def ln_of(code, pos):
        return code.count("\n", 0, pos) + 1


    def call_args(code, name_rx):
        """Yield (pos, argtext) for every call matching name_rx (balanced parentheses)."""
        for m in re.finditer(name_rx + r"\s*\(", code):
            d, j = 0, m.end() - 1
            while j < len(code):
                if code[j] == "(":
                    d += 1
                elif code[j] == ")":
                    d -= 1
                    if d == 0:
                        break
                j += 1
            yield m.start(), code[m.end():j]


    def block_after(code, pos):
        """Text from pos to the end of the enclosing { } block."""
        d, j = 0, pos
        while j < len(code):
            if code[j] == "{":
                d += 1
            elif code[j] == "}":
                d -= 1
                if d < 0:
                    break
            j += 1
        return code[pos:j]




    # ---------------------------------------------------------------- E05 / NEW-02 shared footer parse
    chrome = src("include/UI/ScreenChrome.h")
    glyph_body = next((b for _, _, b in function_bodies(chrome.code, re.compile(r"^buttonGlyph$"))), "")
    KNOWN = set(re.findall(r'btn\s*==\s*"([^"]+)"', glyph_body))
    if re.search(r"btn\[0\]\s*==\s*'A'", glyph_body):
        KNOWN |= {"A", "B", "X", "Y"}


    def footer_tokens(s):
        """(line, button, label) for every footer hint: drawNavBar {"btn","label"} pairs, and every
        'Btn: Label  |  Btn: Label' hint string (the drawNavHints grammar)."""
        out = []
        for pos, args in call_args(s.code, r"\bdrawNavBar"):
            for m in re.finditer(r'\{\s*"([^"]*)"\s*,\s*"([^"]*)"\s*\}', args):
                out.append((ln_of(s.code, pos), m.group(1), m.group(2)))
        for ln, lit in s.literals:
            segs = [t.strip() for t in lit.split("|")]
            if "|" not in lit or not all(segs):
                continue
            pairs = [t.split(":", 1) for t in segs if ":" in t]
            if pairs and any(b.strip() in KNOWN for b, _ in pairs):
                out += [(ln, b.strip(), l.strip()) for b, l in pairs]
        return out


    r = Result("CG36-footer-hints-have-glyphs",
               "Every footer hint names a button ScreenChrome::buttonGlyph draws as a badge (no plain-text "
               "'Touch: Back', 'A / B: Return', 'A/B: Cancel'), and no footer repeats Close Help on '-' "
               "(owner round 3 E05 / NEW-01 note)")
    if not KNOWN:
        r.violations.append("include/UI/ScreenChrome.h: buttonGlyph() button set not found")
    for s in U:
        for ln, b, l in footer_tokens(s):
            if b not in KNOWN and not re.fullmatch(r"(?i)touch|tap", b):   # Touch: CG31 (NEW-02)
                r.violations.append(f"{s.rel}:{ln}: footer hint '{b}: {l}' has no controller glyph (drawn as plain text)")
            if b in ("-", "Minus") and re.match(r"(?i)close help", l):
                r.violations.append(f"{s.rel}:{ln}: footer repeats '- {l}' (B already closes Help)")
    r.violations = dedupe(r.violations)
    res.append(r)

    # ---------------------------------------------------------------- NEW-02
    r = Result("CG37-no-touch-back-footer",
               "No footer carries the obsolete 'Touch: Back' hint; passive View keeps its touch Back target "
               "(owner round 3 NEW-02)")
    for s in U:
        for ln, b, l in footer_tokens(s):
            if re.fullmatch(r"(?i)touch|tap", b):
                r.violations.append(f"{s.rel}:{ln}: footer hint '{b}: {l}'")
        for ln, lit in s.literals:
            if re.search(r"(?i)\b(touch|tap)\s*:\s*back\b", lit):
                r.violations.append(f"{s.rel}:{ln}: \"{lit}\"")
    # keep touch: every view surface that publishes passiveBackTarget today still does
    for rel in ("src/UI/Gen2SharedPokemonSurface.inc", "src/UI/Gen2HardwareFinalFix.inc",
                "src/UI/Gen2SharedSurfaceParity.inc", "src/UI/Gen2UnifiedPokemonWorkspace.inc",
                "src/UI/Gen1PokemonEditorPassiveView.inc"):
        f = src(rel)
        if f and not re.search(r"touchButtons\s*\.\s*push_back\s*\(\s*\{\s*PokeBank::UIModel::passiveBackTarget", f.code):
            r.violations.append(f"{rel}: passive View no longer publishes its touch Back target (passiveBackTarget)")
    r.violations = dedupe(r.violations)
    res.append(r)

    # ---------------------------------------------------------------- NEW-03
    r = Result("CG38-bag-confirm-glyphs-a-commits",
               "Bag Remove / Key Item Warning: in-dialog choices are coloured glyph buttons, Remove commits on A "
               "(B cancels), and the dialog's choices are not repeated in the screen footer (owner round 3 NEW-03)")
    BAG = "include/UI/SharedBagAddPicker.h"
    for ln, b in bodies_ln(BAG, "drawConfirm"):
        if re.search(r"\bdrawText\s*\([^;]*glyphs\s*\[\s*i\s*\]\s*\.\s*glyph", b):
            r.violations.append(f"{BAG}:{ln}: drawConfirm() draws the choice glyph letter as plain text")
        if not re.search(r"\b(buttonGlyph|drawGlyphButton|SharedBoxedRows::drawGlyphChoices)\s*\(", b):
            r.violations.append(f"{BAG}:{ln}: drawConfirm() draws no controller badge (buttonGlyph / drawGlyphButton / drawGlyphChoices)")
        for pos, args in call_args(b, r"\bdrawNavBar"):
            if re.search(r"\b(commit|cancel)\s*\.\s*(glyph|label)", args):
                r.violations.append(f"{BAG}:{ln + b.count(chr(10), 0, pos)}: drawConfirm() repeats its choices in the footer")
    for ln, b in bodies_ln(BAG, "drawRemoveConfirm"):
        if re.search(r'\{\s*"Y"\s*,\s*"Remove"', b):
            r.violations.append(f"{BAG}:{ln}: drawRemoveConfirm() commits on Y (owner: A confirms, B cancels)")
    # callers must not draw a second footer naming the modal's own choices
    for s in U:
        for pos, _ in call_args(s.code, r"\bSharedBagAddPicker::(?:drawRemoveConfirm|drawKeyItemWarning)"):
            tail = block_after(s.code, pos)
            for p2, args in call_args(tail, r"\bdrawNavBar"):
                if re.search(r'"(?:Confirm Remove|Remove|Confirm|Cancel)"', args):
                    r.violations.append(f"{s.rel}:{ln_of(s.code, pos + p2)}: footer repeats the bag dialog's choices")
    # input: Remove commits on A
    classic = src("src/UI/ClassicInventoryOverlay.cpp")
    for m in re.finditer(r"classicRemoveConfirmStep\s*\(\s*\(down\s*&\s*HidNpadButton_(\w+)\)", classic.code):
        if m.group(1) != "A":
            r.violations.append(f"{classic.rel}:{ln_of(classic.code, m.start())}: Gen I-III bag Remove commits on {m.group(1)}")
    g4_r4d = src("src/UI/Gen4SharedPokemonSurface.inc")
    m = re.search(r"if\s*\(\s*state\.itemRemoveConfirmActive\s*\)\s*\{", g4_r4d.code)
    if m and re.search(r"if\s*\(\s*!\s*\(\s*down\s*&\s*HidNpadButton_Y\s*\)\s*\)\s*return", block_after(g4_r4d.code, m.end())):
        r.violations.append(f"{g4_r4d.rel}:{ln_of(g4_r4d.code, m.start())}: Gen IV bag Remove commits on Y")
    g5_r4d = src("src/UI/Gen5SharedReadOnlySurface.inc")
    m = re.search(r"case\s+BagSurface::ConfirmRemove\s*:", g5_r4d.code)
    if m and re.search(r"HidNpadButton_Y\s*\)\s*\{?\s*\n?\s*if\s*\(\s*!\s*bag\.confirmRemove|keys\s*&\s*HidNpadButton_Y", g5_r4d.code[m.end():m.end() + 400]):
        r.violations.append(f"{g5_r4d.rel}:{ln_of(g5_r4d.code, m.start())}: Gen V bag Remove commits on Y")
    base_r4d = src("src/UI/TrainerViewScreenBase.inc")
    for m in re.finditer(r"if\s*\(\s*itemRemoveConfirmActive\s*\)\s*\{", base_r4d.code):
        blk = block_after(base_r4d.code, m.end())
        y = re.search(r"const bool yCommits\s*=\s*PokeBank::UIModel::ClassicGenericUi::classicSession", blk)
        if y and "kDown" in blk:   # the input handler, not a draw site; Release (releaseConfirmActive) stays Y
            r.violations.append(f"{base_r4d.rel}:{ln_of(base_r4d.code, m.end() + y.start())}: Gen I-V item Remove confirm commits on Y (yCommits)")
    ied = "src/UI/Dialogs/ItemEditDialog.cpp"
    for ln, b in bodies_ln(ied, "drawItemRemoveConfirm"):
        if re.search(r'classicSessionDialog\s*\?\s*"Y"', b):
            r.violations.append(f"{ied}:{ln}: Gen I-V item Remove dialog button is Y (owner: A confirms)")
        if re.search(r'confirmFooter\s*\(\s*"[AY]"\s*,\s*"Remove"', b) or re.search(r'drawNavBar\s*\([^;]*"Remove"', b):
            r.violations.append(f"{ied}:{ln}: Gen I-V item Remove dialog repeats its choices in the footer")
    r.violations = dedupe(r.violations)
    res.append(r)

    # ---------------------------------------------------------------- NEW-05
    r = Result("CG39-species-search-keyboard-beside-list",
               "The species search key grid sits outside the list column (under the species art), so the full "
               "list stays visible while searching (owner round 3 NEW-05)")
    SP = "include/UI/SharedSpeciesPicker.h"
    sp = src(SP)
    consts = {}
    for k, expr in re.findall(r"inline constexpr int (\w+)\s*=\s*([^;]+);", sp.code):
        try:  # integer or an expression over earlier constants (kSearchVisibleRows = kVisibleRows)
            consts[k] = int(eval(expr, {"__builtins__": {}}, dict(consts)))
        except Exception:  # noqa: BLE001
            pass
    for need in ("kListX", "kListY", "kListW", "kVisibleRows", "kRowStep", "kSearchVisibleRows"):
        if need not in consts:
            r.violations.append(f"{SP}: constant {need} not found / not evaluable (update this contract)")
    kg = [b for _, b in bodies_ln(SP, "keyGridRect")]
    rect = None
    if kg:
        m = re.search(r"return\s*\{(.*)\}\s*;", kg[0], re.S)
        try:
            parts = [p.strip() for p in m.group(1).split(",")]
            rect = [eval(p, {}, dict(consts, modalX=0, modalY=0)) for p in parts]
        except Exception as e:  # noqa: BLE001
            r.violations.append(f"{SP}: keyGridRect() is not a constexpr rectangle this check can evaluate ({e})")
    else:
        r.violations.append(f"{SP}: keyGridRect() missing")
    if rect and all(k in consts for k in ("kListX", "kListY", "kListW", "kVisibleRows", "kRowStep")):
        gx, gy, gw, gh = rect
        lx, ly, lw = consts["kListX"], consts["kListY"], consts["kListW"]
        lh = consts["kVisibleRows"] * consts["kRowStep"]
        if gx < lx + lw and lx < gx + gw and gy < ly + lh and ly < gy + gh:
            r.violations.append(f"{SP}: key grid ({gx},{gy},{gw}x{gh}) overlaps the species list column "
                                f"({lx},{ly},{lw}x{lh})")
    if "kSearchVisibleRows" in consts and "kVisibleRows" in consts and \
            consts["kSearchVisibleRows"] < consts["kVisibleRows"]:
        r.violations.append(f"{SP}: the list shrinks from {consts['kVisibleRows']} to "
                            f"{consts['kSearchVisibleRows']} rows while searching")
    res.append(r)

    # ---------------------------------------------------------------- NEW-06
    r = Result("CG40-held-item-picker-search",
               "Every Gen II-V Held Item picker offers live search (SharedSearchInput) alongside Sort "
               "(owner round 3 NEW-06)")
    helper = src("include/UI/SharedHeldItemSearch.h")
    helper_ok = helper and all(re.search(pattern, helper.code) for pattern in (
        r"SharedSearchInput::handleKeys\s*\(", r"SharedSearchInput::handleTouch\s*\(",
        r"SharedSearchInput::drawGrid\s*\(", r"!refilter\s*\(\s*state\.input\.query\s*\)"))
    # Require the actual compiled per-generation owners, not just an unused
    # shared keyboard existing somewhere in the repository.
    for path in ("src/UI/SharedPokemonEditorSurface.inc", "src/UI/Gen3SharedPokemonSurface.inc",
                 "src/UI/Gen2PokemonEditorFoundation.inc"):
        owner = src(path)
        wired = owner and all(re.search(pattern, owner.code) for pattern in (
            r"SharedHeldItemSearch::handle\s*\([^;]*\brefilter\s*,",
            r"ItemPickerSearch::apply\s*\([^;]*SharedSearchInput::searchKey\s*\)",
            r"&state\.heldItemSearch\s*\)",
            r"columnsForSearch\s*\(state\.heldItemSearch\.open\(\)\)",
            r"ItemPickerSort::next\s*\("))
        if not helper_ok or not wired:
            r.violations.append(f"{path}: Held Item docked keyboard, live folded filter, result geometry or Sort disconnected")
    res.append(r)

    # ---------------------------------------------------------------- NEW-07
    r = Result("CG41-gen2-ot-gender-inline",
               "Crystal OT Gender: A cycles the native values inline; no 3-option selector popup "
               "(owner round 3 NEW-07)")
    for v in code_hits(U, r"\bGen2FieldSelector::otGender\s*\("):
        r.violations.append(v + "  <- opens the OT Gender popup")
    inline_apply = code_hits(U, r"\bapplyFieldSelector\s*\([^;]*Field::OtGender\s*,[^;]*\bnextOtGender\s*\(")
    if not (inline_apply or code_hits(U, r"\bcrystalOtGender\s*=\s*[^;]*\b(?:cycle\w*|next\w*)\s*\(")):
        r.violations.append("no Gen II UI code cycles state.crystalOtGender inline (cycle*/next* helper)")
    res.append(r)

    # ---------------------------------------------------------------- NEW-08
    r = Result("CG42-ball-picker-scrollbar-gutter",
               "Ball picker rows stop before the scrollbar's touch band (track x - SharedScrollbar::kTouchSlop), "
               "so the bar has its own gutter (owner round 3 NEW-08)")
    sb = src("include/UI/SharedScrollbar.h")
    slop = int(re.search(r"kTouchSlop\s*=\s*(\d+)", sb.code).group(1))
    for rel in ("src/UI/SharedPokemonEditorSurface.inc", "src/UI/Gen3SharedPokemonSurface.inc"):
        f = src(rel)
        for name, ln, b in function_bodies(f.code, re.compile(r"^drawValuePicker$")):
            ball = re.search(r"PickerTarget::Ball\)\s*\{(.*?)\n\s*\}", b, re.S)
            row = re.search(r"Rect\{\s*x\s*\+\s*(\d+)\s*,[^,]+,\s*w\s*-\s*(\d+)\s*,", ball.group(1)) if ball else None
            bar = re.search(r"drawAndPublish\s*\(\s*fb\s*,\s*state\.pickerScrollbar\s*,\s*x\s*\+\s*w\s*-\s*(\d+)\s*,\s*viewportTop",
                            b)
            if not (row and bar):
                # Current shared value lists use one rowW for drawing, clipping
                # and touch, with an explicit gap before the scrollbar track.
                gap = re.search(r"const int scrollbarGap\s*=\s*SharedScrollbar::kTouchSlop\s*\+\s*(\d+)\s*;", b)
                if (gap and int(gap.group(1)) >= 0 and
                    re.search(r"barGutter\s*=\s*count\s*>\s*visible\s*\?\s*SharedScrollbar::kTrackWidth\s*\+\s*scrollbarGap\s*:\s*0", b) and
                    re.search(r"rowW\s*=\s*w\s*-\s*44\s*-\s*barGutter", b) and
                    re.search(r"Rect\{x\s*\+\s*22,\s*yy\s*-\s*4,\s*rowW,", b) and
                    re.search(r"drawAndPublish\s*\(\s*fb\s*,\s*state\.pickerScrollbar\s*,\s*x\s*\+\s*22\s*\+\s*rowW\s*\+\s*scrollbarGap", b)):
                    continue
                r.violations.append(f"{rel}:{ln}: Ball row / scrollbar geometry not found (update this check)")
                continue
            inset = int(row.group(2)) - int(row.group(1))   # row right edge = x + w - inset
            track = int(bar.group(1))                        # track left edge = x + w - track
            if inset < track + slop:
                r.violations.append(f"{rel}:{ln}: Ball rows end {inset - track} px left of the track "
                                    f"(needs >= kTouchSlop {slop}; rows x+w-{inset}, track x+w-{track})")
    res.append(r)

    # ---------------------------------------------------------------- NEW-09
    r = Result("CG43-nature-stat-colours",
               "Gen III-V stat rows show the Nature's effect (raised stat green, lowered red) "
               "(owner round 3 NEW-09)")
    for rel in ("src/UI/Gen3SharedPokemonSurface.inc", "src/UI/SharedPokemonEditorSurface.inc"):
        f = src(rel)
        m = re.search(r'statLabels\{\{"HP"', f.code)
        loop = re.search(r"for\s*\(\s*int\s+row\s*=\s*0\s*;\s*row\s*<\s*6\s*;\s*\+\+row\s*\)\s*\{", f.code[m.end():]) if m else None
        if not loop:
            r.violations.append(f"{rel}: stat row loop not found (update this check)")
            continue
        start = m.end() + loop.end()
        body = block_after(f.code, start)
        if not re.search(r"(?i)nature", body):
            r.violations.append(f"{rel}:{ln_of(f.code, start)}: stat rows ignore the Nature (no +/- stat colouring)")
        elif not re.search(r"\bNatureStats::statColor\s*\(", body):
            r.violations.append(f"{rel}:{ln_of(f.code, start)}: stat rows do not colour through NatureStats::statColor "
                                "(owner Oct 24 PASS: raised green / lowered red)")
    # Round 5 (owner Oct 24 PASS, NEW-09): the colours themselves stay green for the raised stat and red for
    # the lowered one, in the stat rows and in the Nature picker's +10% / -10% effect line, in every theme.
    nat = src("include/UI/NatureStatPresentation.h")
    if not nat or not re.search(r"modifier\s*>\s*0\s*\?\s*Colors::Success\s*:\s*modifier\s*<\s*0\s*\?\s*Colors::Error\s*:\s*normal",
                                nat.code):
        r.violations.append("include/UI/NatureStatPresentation.h: statColor no longer maps raised -> Colors::Success "
                            "(green) and lowered -> Colors::Error (red)")
    elif not (re.search(r'"\+10% "[^;]*Colors::Success', nat.code) and re.search(r'"-10% "[^;]*Colors::Error', nat.code)):
        r.violations.append("include/UI/NatureStatPresentation.h: Nature effect line is not +10% green / -10% red")
    pal = src("include/UI/Common.h")
    palettes = re.findall(r"ThemePalette\s+(\w+)\s*\{([^}]*)\}", pal.code) if pal else []
    if not palettes:
        r.violations.append("include/UI/Common.h: no ThemePalette definitions found (update this check)")
    for name, body in palettes:
        rgb = [tuple(map(int, c)) for c in re.findall(r"Color\(\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*\)", body)]
        if len(rgb) != 15:
            r.violations.append(f"include/UI/Common.h: {name} has {len(rgb)} colours, expected 15 (update this check)")
            continue
        success, error = rgb[11], rgb[13]  # ThemePalette field order: ... divider, success, warning, error, info
        if not (success[1] > success[0] and success[1] > success[2]):
            r.violations.append(f"include/UI/Common.h: {name}.success {success} is not green")
        if not (error[0] > error[1] and error[0] > error[2]):
            r.violations.append(f"include/UI/Common.h: {name}.error {error} is not red")
    res.append(r)

    # ---------------------------------------------------------------- NEW-10
    r = Result("CG44-met-location-compact",
               "The Met Location window leaves no more space under its last row than the accepted Move chooser "
               "(SharedMenuGeometry padBottom + rowGap), Crystal included (owner round 3 NEW-10)")
    G = src("include/UI/SharedMenuGeometry.h")
    def rule_of(t):
        m = re.search(r"case MenuType::" + t + r":.*?return\s*\{([^}]*)\}", G.code, re.S)
        f = [x.strip() for x in m.group(1).split(",")]
        return dict(rowH=int(f[3]), gap=int(f[4]), header=int(f[6]), padBottom=int(f[8]))
    mv, loc = rule_of("MovePicker"), rule_of("LocationPicker")
    ref = mv["padBottom"] + mv["gap"]
    if loc["padBottom"] + loc["gap"] > ref:
        r.violations.append(f"include/UI/SharedMenuGeometry.h: LocationPicker leaves {loc['padBottom'] + loc['gap']} px "
                            f"under its last row (MovePicker: {ref})")
    gm = src("include/UI/Gen2PokemonPickerModel.h")
    b = [x for _, x in bodies_ln("include/UI/Gen2PokemonPickerModel.h", "locationPickerGeometry")]
    mh = re.search(r"g\.height\s*=\s*(\d+)\s*\+\s*g\.visible\s*\*\s*LocationPickerGeometry::rowStep\s*\+\s*(\d+)", b[0]) if b else None
    step = re.search(r"rowStep\s*=\s*(\d+)", gm.code); hgt = re.search(r"rowHeight\s*=\s*(\d+)", gm.code)
    if mh and step and hgt:
        slack = int(mh.group(2)) + int(step.group(1)) - int(hgt.group(1))
        if slack > ref:
            r.violations.append(f"include/UI/Gen2PokemonPickerModel.h: Crystal Met Location leaves {slack} px under its "
                                f"last row (MovePicker: {ref})")
    res.append(r)

    # ---------------------------------------------------------------- NEW-11
    r = Result("CG45-selected-species-name-first",
               "The selected species (DETAILS 'Species' value, species-picker heading) reads 'Pikachu' or "
               "'Pikachu #25', never a leading Dex number; list rows keep their numbers (owner round 3 NEW-11)")
    defs = {}
    for s in U:
        for name, ln, body in function_bodies(s.code, re.compile(r".")):
            defs.setdefault(name, []).append(body)
    STRUCT_LAMBDA = re.compile(r"\[[^\]]*\]\s*\([^)]*\)\s*\{(.*?)\}\s*$", re.S)

    def leads_with_number(expr, depth=0):
        e = expr.strip()
        e = re.sub(r"^return\s+", "", e)
        if re.match(r"(?:std::)?to_string\s*\(", e):
            return True
        if re.match(r'"#?\d', e):
            return True
        m = re.match(r"(?:\w+::)*(\w+)\s*\(", e)
        if not m or depth > 3 or m.group(1) in ("string", "getSpeciesName"):
            return False
        for body in defs.get(m.group(1), []):
            if re.search(r'snprintf\s*\([^;]*"%0?\d*u\s', body) or re.search(r'return\s+"#"\s*\+', body):
                return True
            for ret in re.findall(r"return\s+([^;]+);", body):
                if leads_with_number(ret, depth + 1):
                    return True
        return False

    for s in U:
        # DETAILS rows: {"Species", value} pairs, and detail/value functions' case 0 / case Species
        for m in re.finditer(r'\{\s*"Species"\s*,\s*([^}]+)\}', s.code):
            if leads_with_number(m.group(1)):
                r.violations.append(f"{s.rel}:{ln_of(s.code, m.start())}: Species value '{m.group(1).strip()[:60]}' leads with the Dex number")
        for name, ln, body in function_bodies(s.code, re.compile(r"(?i)(detail|main)\w*value$")):
            for m in re.finditer(r"(?:case\s+(?:0|Species)\s*:|if\s*\(\s*row\s*==\s*0\s*\))\s*return\s+([^;]+);", body):
                if leads_with_number(m.group(1)):
                    r.violations.append(f"{s.rel}:{ln + body.count(chr(10), 0, m.start())}: {name}() Species value leads with the Dex number")
        # Gen I left panel arrays: leftLabels{"Species", ...} with leftValues{first, ...}
        for m in re.finditer(r'leftLabels\{\s*"Species".*?leftValues\{\s*([^,]+),', s.code, re.S):
            if leads_with_number(m.group(1)):
                r.violations.append(f"{s.rel}:{ln_of(s.code, m.start(1))}: Species value '{m.group(1).strip()}' leads with the Dex number")
        # species-picker heading: the 2nd lambda (titleText) of SharedSpeciesPicker::drawContent
        for pos, args in call_args(s.code, r"\bSharedSpeciesPicker::drawContent"):
            lambdas = re.findall(r"\[[^\]]*\]\s*\([^)]*\)\s*\{((?:[^{}]|\{[^{}]*\})*)\}", args)
            if len(lambdas) >= 2:
                rets = re.findall(r"return\s+([^;]+);", lambdas[1])
                if any(leads_with_number(x) for x in rets):
                    r.violations.append(f"{s.rel}:{ln_of(s.code, pos)}: species-picker heading leads with the Dex number "
                                        f"('{rets[0].strip()[:60]}')")
    r.violations = dedupe(r.violations)
    res.append(r)

    # ---------------------------------------------------------------- NEW-13
    r = Result("CG46-ability-names-not-slots",
               "Gen III-V Ability picker rows show the ability name (+ short effect), never 'Slot 1/2' "
               "(owner round 3 NEW-13)")
    for rel in ("src/UI/Gen3SharedPokemonSurface.inc", "src/UI/Gen4SharedPokemonSurface.inc",
                "src/UI/Gen5SharedReadOnlySurface.inc"):
        f = src(rel)
        for name, ln, body in function_bodies(f.code, re.compile(r"^picker(?:Value)?Label$")):
            for a in list(re.finditer(r"PickerTarget::Ability|abilityForSlot\w*\s*\(", body))[:1]:
                seg = body[a.start():]
                end = re.search(r"getAbilityName[^;]*;", seg)
                seg = seg[:end.end()] if end else seg[:600]
                for lit in re.findall(r'"([^"]*\bSlot\b[^"]*)"', seg):
                    r.violations.append(f"{rel}:{ln + body.count(chr(10), 0, a.start())}: Ability row label \"{lit}\"")
    if not (code_hits(U, r"(?i)\b\w*ability\w*(?:desc|effect|summary)\w*\s*\(") or
            code_hits(U, r"\bAbilityEffects::row\s*\(")):
        r.violations.append("no ability effect-description source is used by any Gen III-V UI")
    r.violations = dedupe(r.violations)
    res.append(r)

    # ---------------------------------------------------------------- NEW-18
    r = Result("CG47-classic-trainer-playtime-pokedex",
               "The Gen I-IV Trainer card shows native Play Time and Pokedex seen/caught, like Gen V "
               "(owner round 3 NEW-18)")
    g5t = [b for _, b in bodies_ln("src/UI/Gen5SharedReadOnlySurface.inc", "drawTrainerEditor")]
    # Round 6 P2 (lane C): the classic card is drawClassicTrainerView, painting the rows of
    # include/UI/ClassicTrainerPage.h (Gen V design); drawTrainerView only routes Gen I-IV to it.
    # The guard follows the renderer: the classic body must paint the Play Time and Pokedex fields
    # with the native readers, and the model must label and always place both rows.
    page_h = next((x for x in sources if x.rel == "include/UI/ClassicTrainerPage.h"), None)
    for rel, fn in (("src/UI/TrainerViewScreenBase.inc", "drawClassicTrainerView"),):
        bs = bodies_ln(rel, fn)
        if not bs:
            r.violations.append(f"{rel}: {fn}() missing")
        for ln, b in bs:
            lits = [l for l in re.findall(r'"([^"]*)"', b)]
            model = page_h.code if page_h else ""
            model_lits = re.findall(r'"([^"]*)"', model)
            play = (any(re.search(r"(?i)play\s*time", l) for l in lits) or
                    ("Field::PlayTime" in b and "nativePlayTime" in b and
                     any(re.search(r"(?i)^play\s*time$", l) for l in model_lits) and
                     re.search(r"if \(!in\.playTimeEditable\) out\.push_back\(\{Field::PlayTime, false", model)))
            if not play:
                r.violations.append(f"{rel}:{ln}: {fn}() shows no Play Time row")
            dex = ((any(re.search(r"(?i)pok[eé]dex|\bseen\b", l) for l in lits) and
                    re.search(r"\bpokedexProgress\s*\(", b)) and
                   ("Field::Pokedex" not in b or
                    (any(re.search(r"(?i)^pok[eé]dex$", l) for l in model_lits) and
                     "out.push_back({Field::Pokedex, false, -1});" in model)))
            if not dex:
                r.violations.append(f"{rel}:{ln}: {fn}() shows no Pokedex seen / caught row")
    view = bodies_ln("src/UI/TrainerViewScreenBase.inc", "drawTrainerView")
    if not view or not all("drawClassicTrainerView(screen, fb, x, y, w, h, generation);" in b for _, b in view):
        r.violations.append("src/UI/TrainerViewScreenBase.inc: drawTrainerView() does not route Gen I-IV to drawClassicTrainerView")
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
    res.extend(round4_p2_contracts(sources))
    res.extend(round4_p0_contracts(sources))
    res.extend(round5_contracts(sources))
    res.extend(round6_contracts(sources))
    res.extend(round7_contracts(sources))
    return res


def round5_contracts(sources):
    """Round 5: gaps in the owner's Oct 24 FAIL items and requests that the lanes confirmed
    (docs/hardware/OCT24_REPAIRS_AND_RETEST.md). Called from contracts()."""
    res = []

    def src(rel):
        return next((x for x in sources if x.rel == rel), None)

    def body_of(f, name):
        return "\n".join(b for _, _, b in function_bodies(f.code, re.compile("^" + re.escape(name) + "$"))) if f else ""

    def block_after(code, pos):
        """Text from pos to the end of the enclosing { } block."""
        d, j = 0, pos
        while j < len(code):
            if code[j] == "{":
                d += 1
            elif code[j] == "}":
                d -= 1
                if d < 0:
                    break
            j += 1
        return code[pos:j]

    # ------------------------------------------------------------------------------------------
    # P2 / #322 (lane L): A on a native Switch backup opens THAT copy in an isolated staged session
    # (owner: "A opens that specific copy in an isolated staged session"), not a read-only preview and
    # never the backup folder itself. The backup stays byte-identical: the session is a copy in the exact
    # profile/game folder, and the listed snapshot must still match after copying.
    # ------------------------------------------------------------------------------------------
    r = Result("CG61-switch-backup-opens-staged-copy",
               "Switch Save Backups: A opens a copy of the selected backup in an isolated staged session "
               "(never a read-only preview, never the backup folder itself) (owner Oct 24 P2, #322)")
    uic = src("src/UI/UI.cpp")
    m = re.search(r'"Loading existing backup"', uic.code) if uic else None
    if not m:
        r.violations.append("src/UI/UI.cpp: Save Backups open path ('Loading existing backup') not found (update this check)")
    else:
        region = uic.code[m.start():m.start() + 6000]
        nxt = re.search(r"\n\s*bool\s+UIManager::", region)
        region = region[:nxt.start()] if nxt else region
        at = uic.code.count("\n", 0, m.start()) + 1
        if re.search(r"handleTrainerView\s*\([^;]*getSelectedBackupPath\s*\(\s*\)", region):
            r.violations.append(f"src/UI/UI.cpp:{at}: A on a backup opens the backup folder itself in the editor")
        if re.search(r"handleTrainerView\s*\([^;]*&\s*historical\b", region) or re.search(r"\bread-only\b", region):
            r.violations.append(f"src/UI/UI.cpp:{at}: A on a backup opens a read-only historical preview, "
                                "not an isolated staged session")
        # The session comes from a helper that copies via WorkingCopy::openBackupSession and re-checks the
        # listed snapshot; its result (not the backup path) is what the editor opens.
        helpers = [n for n, _, b in function_definitions(uic.code)
                   if re.search(r"\bopenBackupSession\s*\(", b) and re.search(r"ImmutableSnapshot::unchanged\s*\(", b)]
        call = next((re.search(r"(?:const\s+)?(?:std::string|auto)\s+(\w+)\s*=\s*" + re.escape(h) + r"\s*\(", region)
                     for h in helpers if re.search(r"\b" + re.escape(h) + r"\s*\(", region)), None)
        if not call:
            r.violations.append(f"src/UI/UI.cpp:{at}: no copy-into-session step (a helper calling "
                                "WorkingCopy::openBackupSession and re-checking ImmutableSnapshot::unchanged)")
        elif not re.search(r"handleTrainerView\s*\([^;]*\b" + re.escape(call.group(1)) + r"\b", region):
            r.violations.append(f"src/UI/UI.cpp:{at}: the editor is not opened on the copied session")
    wc = src("include/UI/WorkingCopyRefresh.h")
    ob = body_of(wc, "openBackupSession")
    if not ob:
        r.violations.append("include/UI/WorkingCopyRefresh.h: no openBackupSession")
    else:
        for rx, why in ((r"gameDir\s*\+\s*\"/\"\s*\+\s*kBackupSessionName", "the session lives in the exact game folder"),
                        (r"copyTree\s*\(\s*backupDir\s*,\s*session\s*\)", "the backup is copied INTO the session"),
                        (r"backupDir\s*==\s*session", "the session itself is never opened as a backup")):
            if not re.search(rx, ob):
                r.violations.append(f"include/UI/WorkingCopyRefresh.h: openBackupSession: {why} (missing)")
    bss = src("src/UI/BackupSelectionScreen.cpp")
    if bss:
        for line, lit in bss.literals:
            if lit == "Open read-only":
                r.violations.append(f"src/UI/BackupSelectionScreen.cpp:{line}: footer still says A 'Open read-only'")
        if not re.search(r"\bkBackupSessionName\b", bss.code):
            r.violations.append("src/UI/BackupSelectionScreen.cpp: the open-backup session folder is listed as a backup")
    res.append(r)

    # ------------------------------------------------------------------------------------------
    # 4A / NEW-10 (lane P): the compact 620 px Met Location window must not clip rows. The source
    # example (detail) gets the width the MEASURED place name leaves, not a fixed share (Gen IV/V
    # rowW - 260 clipped 5,308 of 16,958 default rows with an ellipsis; Gen III got half a row).
    # ------------------------------------------------------------------------------------------
    r = Result("CG62-met-location-rows-fit",
               "Gen III-V Met Location rows fit the compact window: the detail column is sized from the "
               "measured place name, never a fixed share of the row (owner Oct 24 NEW-10 / 4A)")
    for rel, name_expr in (("src/UI/SharedPokemonEditorSurface.inc", r"label\.substr\(\s*0\s*,\s*split\s*\)"),
                           ("src/UI/Gen3SharedPokemonSurface.inc", r"row\.first")):
        body = body_of(src(rel), "drawValuePicker")
        # The row-drawing branch: the Met Location test whose branch draws the row text.
        branches = []
        for m in re.finditer(r"pickerTarget\s*==\s*PickerTarget::MetLocation", body):
            b = body[m.start():m.start() + 3000]
            nxt = re.search(r"\}\s*else\s+if\s*\(\s*state\.pickerTarget|(?<=.)pickerTarget\s*==\s*PickerTarget::MetLocation", b, re.S)
            b = b[:nxt.start()] if nxt else b
            if re.search(r"\bfit\s*\(", b) and re.search(name_expr, b) and re.search(r"drawText\s*\(\s*x\s*\+\s*38\b", b):
                branches.append(b)
        if len(branches) != 1:
            r.violations.append(f"{rel}: drawValuePicker Met Location row branch not found (update this check)")
            continue
        branch = branches[0]
        if re.search(r"rowW\s*-\s*260\b", branch) or re.search(r"\(\s*rowW\s*-\s*32\s*\)\s*/\s*2", branch):
            r.violations.append(f"{rel}: Met Location detail has a fixed share of the row (clips long rows)")
        mw = re.search(r"measureText\s*\(\s*" + name_expr + r"\s*,\s*(\w+)", branch)
        if not mw or not re.search(r"\bfit\s*\([^;]*rowW\s*-\s*\d+\s*-\s*" + re.escape(mw.group(1)) + r"\b", branch):
            r.violations.append(f"{rel}: Met Location detail width is not derived from the measured place name")
    ctx = src("include/Integration/Encounter/EncounterContext45.h")
    if ctx and any(" variants)" in lit for _, lit in ctx.literals):
        r.violations.append("include/Integration/Encounter/EncounterContext45.h: '(+N variants)' detail does not fit 620 px")
    res.append(r)

    # ------------------------------------------------------------------------------------------
    # 4D (lane P): Help names only controls that act on that page in that generation. '+' opens Review
    # only in Gen V; in Gen I-IV it goes to Product Home, so their Pokemon / Items pages must not claim
    # '+ Review'. A help line must name a button ("Move  Place or cancel from the on-screen Move controls"
    # named none).
    # ------------------------------------------------------------------------------------------
    # Round 6 (lane L, B08 / P1-RETURN) retarget: + now opens the shared Review on Gen I-IV Boxes,
    # Party, Bag and Trainer too (ReviewEntry.h, read by openClassicBrowseReview and the Bag's +).
    # A '+ Review' line outside a generation==5 branch is accepted only while those handlers exist,
    # and never on the Gen I-IV save hub (there + still points to Product Home).
    r = Result("CG63-help-names-real-controls",
               "Classic Help: '+ Review' appears only where + opens Review (Gen V, and Gen I-IV editor pages "
               "per ReviewEntry.h); every line names a real control (owner Oct 24 4D, Oct 25 B08)")
    hp = src("include/UI/ClassicHelpPresentation.h")
    entry = src("include/UI/ReviewEntry.h")
    composite = src("src/UI/TrainerViewScreenCompositeOverlay.cpp")
    bag = src("src/UI/ClassicInventoryOverlay.cpp")
    classic_plus = bool(entry and re.search(r"plusOpensReview", entry.code) and
                        composite and "if (openClassicBrowseReview(*this, down)) return;" in composite.code and
                        bag and "if (screen.openStagedReview(false)) return true;" in bag.code)
    if not hp:
        r.violations.append("include/UI/ClassicHelpPresentation.h: missing")
    else:
        hub = re.search(r"case\s+Page::Hub:(.*?)break;", hp.code, re.S)
        for m in re.finditer(r'"\+\s+Review[^"]*"', hp.code):
            start = max(hp.code.rfind(";", 0, m.start()), hp.code.rfind("{", 0, m.start()),
                        hp.code.rfind(":\n", 0, m.start()))
            stmt = hp.code[start + 1:m.start()]
            line = hp.code.count(chr(10), 0, m.start()) + 1
            if re.search(r"generation\s*==\s*5\s*(?:\?|\))", stmt):
                continue
            if not classic_plus:
                r.violations.append(f"include/UI/ClassicHelpPresentation.h:{line}: "
                                    "'+ Review' is shown outside Gen V but no Gen I-IV handler opens Review on +")
        if hub:
            for m in re.finditer(r'(if\s*\(\s*generation\s*==\s*5\s*\)\s*)?out\.push_back\(([^;]*)\);', hub.group(1)):
                if ("kPlusReview" in m.group(2) or re.search(r'"\+\s+Review', m.group(2))) and \
                        not m.group(1) and not re.search(r"generation\s*==\s*5", m.group(2)):
                    r.violations.append("include/UI/ClassicHelpPresentation.h: the Gen I-IV save hub names "
                                        "'+ Review' (there + opens Product Home)")
        if any("on-screen Move controls" in lit for _, lit in hp.literals):
            r.violations.append("include/UI/ClassicHelpPresentation.h: Move help names no button")
    res.append(r)

    # ------------------------------------------------------------------------------------------
    # P3 (lane S f775db01, ENFORCED): Trainer Play Time (Gen II-IV) and Gender (Crystal, Gen III-IV)
    # are staged edits only: the stager writes the staged image / pending fields, Review lists the
    # change, Discard restores it, Export writes it; never the source. Gender is offered only where the
    # native format stores it (no Gen I, no Gold / Silver) and only when the stored byte is 0 / 1.
    # ------------------------------------------------------------------------------------------
    r = Result("CG64-trainer-playtime-gender-staged",
               "Gen II-IV Trainer Play Time and Gender are staged -> Review -> Export only; Gender only for "
               "Crystal / Gen III / Gen IV and only when the stored byte is 0 / 1 (owner Oct 24 P3)")
    SOURCE_WRITE = re.compile(r"\b(?:fopen|ofstream|fwrite|writeFile|writeSave|commitSave|injectSave|"
                              r"replaceFileAtomic|std::filesystem::(?:copy|rename|remove))\b")

    def fn(rel, name):
        return body_of(src(rel), name)
    g2 = "src/Integration/Gen2/Gen2StagedEditor.cpp"
    g3 = "include/Integration/Gen3/Gen3StagedTrainerEditor.h"
    g4 = "src/Integration/Gen4/Gen4StagedPokemonEditor.cpp"
    for rel, name, needs in (
            (g2, "stagePlayTime", (r"\bstaged_\s*\[", r'setChange\s*\(\s*"play_time"')),
            (g2, "stageTrainerGender", (r"\bstaged_\s*\[", r'setChange\s*\(\s*"trainer_gender"', r"gender\s*>\s*1")),
            (g2, "trainerGenderEditable", (r"VersionFamily::Crystal", r"original_\s*\[[^\]]*\]\s*<=\s*1")),
            (g3, "stageGender", (r"\bgender_\s*=\s*gender\b", r"gender\s*>\s*1", r"genderEditable\s*\(")),
            (g3, "genderEditable", (r"originalGender_\s*<=\s*1",)),
            (g3, "stagePlayTime", (r"\bplayTime_\s*=\s*time\b", r"kMaxPlayedHours")),
            (g3, "discard", (r"gender_\s*=\s*originalGender_", r"playTime_\s*=\s*originalPlayTime_")),
            (g4, "stageTrainerGender", (r"\bstageTrainerBytes\s*\(", r"gender\s*>\s*1", r"trainerGenderEditable\s*\(")),
            (g4, "stageTrainerPlayTime", (r"\bstageTrainerBytes\s*\(", r"kMaxPlayedHours")),
            (g4, "trainerGenderEditable", (r"<=\s*1",))):
        body = fn(rel, name)
        if not body:
            r.violations.append(f"{rel}: {name}() missing")
            continue
        for rx in needs:
            if not re.search(rx, body):
                r.violations.append(f"{rel}: {name}() lacks {rx!r}")
        if name.startswith("stage") and SOURCE_WRITE.search(body):
            r.violations.append(f"{rel}: {name}() writes a file; Trainer edits must stay staged")
    g3h = src(g3)
    if g3h and not ({"Trainer gender", "Play time"} <= {lit for _, lit in g3h.literals}):
        r.violations.append(f"{g3}: Gen III Review rows for Trainer gender / Play time missing")
    g4x = src("src/Integration/Gen4/Gen4ExportTransaction.cpp")
    if not g4x or not (re.search(r'"Trainer gender: "', g4x.code) and re.search(r'"Play time: "', g4x.code)):
        r.violations.append("src/Integration/Gen4/Gen4ExportTransaction.cpp: Gen IV Review lines for Trainer gender / Play time missing")
    # The UI offers a Gender row only when the generation supplies a gender stager.
    cin = src("include/UI/ClassicTrainerSessionInput.h")
    if not cin or not re.search(r"genderRow\s*=\s*static_cast<bool>\(\s*fields\.stageGender\s*\)", cin.code):
        r.violations.append("include/UI/ClassicTrainerSessionInput.h: Gender row not tied to a native gender stager")
    for rel in ("src/UI/Gen2TrainerSessionOverlay.inc", "src/UI/Gen3SharedPokemonSurface.inc", "src/UI/Gen4SharedPokemonSurface.inc"):
        f = src(rel)
        if f and re.search(r"fields\.stageGender\s*=", f.code) and not re.search(
                r"if\s*\(\s*\w+(?:->|\.)(?:trainerGenderEditable|genderEditable)\s*\(\s*\)\s*\)\s*fields\.stageGender\s*=", f.code):
            r.violations.append(f"{rel}: Gender stager is set without checking the stored byte is native (0 / 1)")
    # Gen I stores no trainer gender: its session never gets a gender stager.
    for f in sources:
        if re.search(r"(?:^|/)(?:Gen1|RBY)", f.rel) and re.search(r"\bfields\.stageGender\b|\bstageTrainerGender\b", f.code):
            r.violations.append(f"{f.rel}: Gen I offers a trainer gender edit (Gen I has no gender field)")
    # Gold / Silver: the Gen II gender stager is Crystal-only (checked above) and refuses others.
    if not re.search(r"Gold and Silver store no trainer gender", src(g2).code if src(g2) else ""):
        r.violations.append(f"{g2}: stageTrainerGender does not refuse Gen II Gold / Silver")
    res.append(r)

    # ------------------------------------------------------------------------------------------
    # P1 Gen I-IV (lane C 8f0e1210): only the shared Review presenter paints Gen I Review. The old
    # 760x240 "Discard Staged" panel and the old footer (A Export Copy; X exports in the shared panel)
    # were painted over SharedEditorChrome::drawReviewPanel / drawReviewDiscardConfirm.
    # ------------------------------------------------------------------------------------------
    r = Result("CG65-gen1-review-shared-presenter-only",
               "Gen I Review: nothing but the shared Review panel / Discard All confirmation paints it; no old "
               "Discard Staged panel or 'A Export Copy' footer on top (owner Oct 24 P1)")
    g1 = src("src/UI/Gen1PokemonEditorOverlayUXCleanup3.inc")
    if not g1:
        r.violations.append("src/UI/Gen1PokemonEditorOverlayUXCleanup3.inc: missing")
    else:
        if re.search(r"drawText\s*\([^;]*Gen12ReviewDiscard::kTitle", g1.code):
            r.violations.append(f"{g1.rel}: the old Gen I Discard Staged panel is still painted over the shared confirmation")
        foot = body_of(g1, "ux3FooterText")
        m = re.search(r"case\s+UX2Mode::Review\s*:\s*return\s*\"([^\"]*)\"", foot)
        if not m or re.search(r"\bA\s*:\s*Export", m.group(1)):
            r.violations.append(f"{g1.rel}: ux3FooterText Review names A as Export Copy (X exports in the shared Review)")
        nav = body_of(g1, "ux3DrawTouchNavBar")
        m = re.search(r"case\s+UX2Mode::Review\s*:(.*?)(?=case\s+UX2Mode::|\Z)", nav, re.S)
        if not m or re.search(r"\bdrawNavBar\s*\(", m.group(1)):
            r.violations.append(f"{g1.rel}: ux3DrawTouchNavBar paints a second Review footer over the shared one")
    res.append(r)

    # ------------------------------------------------------------------------------------------
    # P1 (lane C 8f0e1210): Gen II Review touch on the shared paged panel. reviewRow is the shared
    # presenter's PAGE: a tap selects nothing (rows are informational) and one completed vertical swipe
    # on the panel turns exactly one page (SharedReview::move by +/-1), inert while Discard All is asked.
    # ------------------------------------------------------------------------------------------
    r = Result("CG66-gen2-review-swipe-one-page",
               "Gen II Review touch: a tap never jumps pages; one vertical swipe turns one shared Review page; "
               "inert during Discard All (owner Oct 24 P1)")
    g2h = src("src/UI/Gen2HardwareWorkspaceFix.inc")
    body = body_of(g2h, "handleParityTouch")
    m = re.search(r"if\s*\(\s*legacy\.active\s*&&\s*legacy\.review\s*\)", body)
    if not m:
        r.violations.append("src/UI/Gen2HardwareWorkspaceFix.inc: handleParityTouch Review branch not found (update this check)")
    else:
        branch = block_after(body, body.index("{", m.end()) + 1)
        if re.search(r"reviewRow\s*=\s*i\b", branch) or re.search(r"justTapped\s*\(", branch):
            r.violations.append("src/UI/Gen2HardwareWorkspaceFix.inc: a Review tap still sets reviewRow (jumps pages)")
        if re.search(r"/\s*45\b|\bsteps\b", branch) or not re.search(
                r"SharedReview::move\s*\([^;]*\?\s*1\s*:\s*-1", branch):
            r.violations.append("src/UI/Gen2HardwareWorkspaceFix.inc: a Review swipe does not turn exactly one shared page")
        if not re.search(r"!\s*[^;]*reviewDiscard\.confirming", branch):
            r.violations.append("src/UI/Gen2HardwareWorkspaceFix.inc: Review swipe is live under the Discard All confirmation")
    res.append(r)

    # ------------------------------------------------------------------------------------------
    # P1 (lane C 9f034431): Gen I's shared Review gets the same touch rule as Gen II (CG66), now that
    # the Bag opens it. A tap selects nothing, and one completed vertical swipe on the panel turns one
    # page (SharedReviewTouch::swipePage -> SharedReview::move). No private row scroll survives.
    # ------------------------------------------------------------------------------------------
    r = Result("CG68-gen1-review-swipe-one-page",
               "Gen I Review touch: a tap never selects a row; one vertical swipe on the panel turns one shared "
               "Review page (owner Oct 24 P1, N03 / AUD-05 re-pin)")
    g1t = src("src/UI/Gen1PokemonEditorOverlayUXCleanup3.inc")
    body = body_of(g1t, "ux3DirectTouchInput")
    m = re.search(r"else\s+if\s*\(\s*state\.mode\s*==\s*UX2Mode::Review\s*\)\s*\{", body)
    if not m:
        r.violations.append("src/UI/Gen1PokemonEditorOverlayUXCleanup3.inc: ux3DirectTouchInput Review branch not found (update this check)")
    else:
        branch = block_after(body, m.end())
        if re.search(r"\bselectedId\s*\(|reviewRow\s*=\s*row\b", branch):
            r.violations.append(f"{g1t.rel}: a Gen I Review tap still selects a row")
        if re.search(r"reviewScroll\.updateVertical\s*\(", branch):
            r.violations.append(f"{g1t.rel}: Gen I Review still scrolls a private row list")
        if not (re.search(r"SharedReviewTouch::swipePage\s*\(\s*touch\.justReleased\s*\(\s*\)\s*,\s*touch\.dragged\s*\(\s*\)", branch)
                and re.search(r"SharedReview::move\s*\([^;]*\bpage\b", branch)):
            r.violations.append(f"{g1t.rel}: a completed Gen I Review swipe does not turn one shared page")
    srt = src("include/UI/SharedReviewTouch.h")
    sp = body_of(srt, "swipePage")
    for rx, why in ((r"!\s*justReleased\s*\|\|\s*!\s*dragged", "only a completed drag pages"),
                    (r"!\s*panel\.contains\s*\(\s*startX\s*,\s*startY\s*\)\s*\|\|\s*!\s*panel\.contains\s*\(\s*endX\s*,\s*endY\s*\)",
                     "the swipe starts and ends on the panel"),
                    (r"ady\s*<\s*kMinSwipe\s*\|\|\s*ady\s*<=\s*adx", "short or sideways drags do nothing"),
                    (r"dy\s*<\s*0\s*\?\s*1\s*:\s*-1", "one page per swipe")):
        if not re.search(rx, sp):
            r.violations.append(f"include/UI/SharedReviewTouch.h: swipePage: {why} (missing)")
    res.append(r)

    # ------------------------------------------------------------------------------------------
    # P1 (lane C a7244a64): a Review opened outside Box Actions (staged-exit Review Changes / Export
    # Copy, Trainer card, Bag) closes to the screen underneath, never to a stale slot's Actions sheet.
    # openStagedReview marks it standalone; every Gen I-IV Review exit that falls back to Actions takes
    # the mark.
    # ------------------------------------------------------------------------------------------
    r = Result("CG67-standalone-review-returns-underneath",
               "Gen I-IV Review opened from the exit menu / Trainer / Bag closes to the screen underneath, not "
               "to a stale Box Actions sheet (owner Oct 24 P1)")
    origin = src("include/UI/ClassicReviewOrigin.h")
    if not origin or not (re.search(r"\bmarkStandalone\s*\(", origin.code) and re.search(r"\btakeStandalone\s*\(", origin.code)):
        r.violations.append("include/UI/ClassicReviewOrigin.h: no standalone Review origin (markStandalone / takeStandalone)")
    comp = src("src/UI/TrainerViewScreenCompositeOverlay.cpp")
    if not re.search(r"ClassicReviewOrigin::markStandalone\s*\(", body_of(comp, "openStagedReview")):
        r.violations.append("src/UI/TrainerViewScreenCompositeOverlay.cpp: openStagedReview does not mark its Review standalone")
    FALLBACK = re.compile(r"selectedMode\s*[!=]=\s*TrainerViewScreen::ViewMode::Trainer")
    for rel, fn_name, anchor in (
            ("src/UI/Gen1PokemonEditorOverlayUXCleanup2.inc", "ux2HandleActiveInput", r"state\.mode\s*==\s*UX2Mode::Review\s*\)"),
            ("src/UI/Gen2SharedPokemonSurface.inc", "handleSharedReview", None),
            ("src/UI/Gen3SharedPokemonSurface.inc", "handleReview", None),
            ("src/UI/Gen4SharedPokemonSurface.inc", "handleReview", None)):
        body = body_of(src(rel), fn_name)
        if anchor:
            m = re.search(anchor, body)
            body = block_after(body, body.index("{", m.end()) + 1) if m else ""
        if not body:
            r.violations.append(f"{rel}: {fn_name} Review exit not found (update this check)")
            continue
        for fb in FALLBACK.finditer(body):
            if not re.search(r"ClassicReviewOrigin::takeStandalone\s*\(", body[fb.end():fb.end() + 400]):
                r.violations.append(f"{rel}: {fn_name}: a Review exit falls back to Actions without consulting "
                                    "ClassicReviewOrigin (stale Box Actions after a standalone Review)")
    res.append(r)
    return res


# GlyphInstruction::canonicalButton (include/UI/GlyphInstruction.h): the written control names a Help
# row's key part may use and still be drawn as glyph badges.
_GLYPH_TOKENS = {"A", "B", "X", "Y", "L", "R", "ZL", "ZR", "L/R", "ZL/ZR", "+", "-", "Plus", "Minus",
                 "D-pad/LeftStick", "D-pad/Stick", "D-Pad/LeftStick", "LeftStick", "D-pad", "D-Pad", "Arrows",
                 "Up/Down", "Left/Right", "Left", "Right", "Up", "Down"}


def _glyph_keys(keys: str) -> bool:
    """keyCluster(): the whole key part, or each '/'-separated part, is a control name."""
    if keys.replace(" ", "") in _GLYPH_TOKENS:
        return True
    parts = [k.strip().replace(" ", "") for k in keys.split("/")]
    return bool(parts) and all(k in _GLYPH_TOKENS for k in parts)


def round6_contracts(sources):
    """Round 6: owner Oct 25 FAIL items and requests (docs/hardware/ROUND5_RESULTS_1ca2faec.md).
    Called from contracts()."""
    res = []

    def src(rel):
        return next((x for x in sources if x.rel == rel), None)

    def body_of(f, name):
        return "\n".join(b for _, _, b in function_bodies(f.code, re.compile("^" + re.escape(name) + "$"))) if f else ""

    def block_after(code, pos):
        d, j = 0, pos
        while j < len(code):
            if code[j] == "{":
                d += 1
            elif code[j] == "}":
                d -= 1
                if d < 0:
                    break
            j += 1
        return code[pos:j]

    def block(f, anchor_rx):
        """Body of the first `if (...) {` block whose condition matches anchor_rx."""
        m = re.search(anchor_rx, f.code) if f else None
        if not m:
            return None, 0
        brace = f.code.find("{", m.end() - 1)
        return block_after(f.code, brace + 1), f.code.count("\n", 0, m.start()) + 1

    def stmt_at(f, pos):
        """The statement around pos: from the previous ';' / '{' / '}' to the next ';'."""
        start = max(f.code.rfind(";", 0, pos), f.code.rfind("{\n", 0, pos), f.code.rfind("}", 0, pos))
        return f.code[start + 1:f.code.find(";", pos) + 1]

    def ln_at(f, pos):
        return f.code.count("\n", 0, pos) + 1

    # ------------------------------------------------------------------------------------------
    # P1 / L06 (lane S): Pokerus is editable in Gen III and V like Gen II / IV. Gen III Create left row 14
    # (Pokerus) out of detailEditable, and Gen III's direct tap never revealed the row before the A gate
    # (Gen IV / V got that in 6A via Workspace::focusCompletedTap), so the first tap only scrolled.
    # ------------------------------------------------------------------------------------------
    r = Result("CG69-pokerus-editable-gen3-gen5",
               "Gen III / V Pokerus: editable in Create and Edit, and a completed tap on its row acts like A "
               "(owner Oct 25 L06)")
    g3 = src("src/UI/Gen3SharedPokemonSurface.inc")
    de = body_of(g3, "detailEditable")
    m = re.search(r"if\s*\(\s*state\.session\.mode\s*==\s*SessionModel::Mode::Create\s*\)\s*return([^;]*);", de)
    if not de:
        r.violations.append("src/UI/Gen3SharedPokemonSurface.inc: detailEditable not found (update this check)")
    elif m and not re.search(r"\brow\s*==\s*14\b", m.group(1)):
        r.violations.append("src/UI/Gen3SharedPokemonSurface.inc: Gen III Create leaves Pokerus (row 14) read-only "
                            "while drawing it as Editable")
    if g3:
        m = re.search(r"auto\s+applyDirectFocus\s*=\s*\[[^\]]*\]\s*\(\s*int\s+\w+\s*\)\s*\{", g3.code)
        body = block_after(g3.code, m.end()) if m else ""
        if not m:
            if not re.search(r"focusCompletedTap\s*\(", g3.code):
                r.violations.append("src/UI/Gen3SharedPokemonSurface.inc: Gen III detail tap path not found (update this check)")
        elif not re.search(r"\breveal\s*\(|focusCompletedTap\s*\(", body):
            r.violations.append(f"src/UI/Gen3SharedPokemonSurface.inc:{ln_at(g3, m.start())}: a Gen III detail tap does "
                                "not reveal its row before the A gate (first tap on Pokerus only scrolls)")
    g5f = src("include/Integration/Gen5/Gen5EditorFields.h")
    if g5f and not re.search(r"case\s+Pokerus\s*:", g5f.code):
        r.violations.append("include/Integration/Gen5/Gen5EditorFields.h: Gen V Pokerus is not an editable detail")
    g5 = src("src/UI/Gen5SharedReadOnlySurface.inc")
    ed5 = body_of(g5, "editDetail")
    if not re.search(r"stage\s*\(\s*screen\s*,\s*Field::Pokerus", ed5):
        r.violations.append("src/UI/Gen5SharedReadOnlySurface.inc: Gen V editDetail does not stage Field::Pokerus")
    res.append(r)

    # ------------------------------------------------------------------------------------------
    # P1 / L06 Held Item (lane P): A and a completed tap select a Held Item at the top, middle and bottom
    # of the list. The pickers publish hit targets for the partly visible row above and below, but a tap
    # there sets the row, then fails the "is the selection visible" A gate and only reveals it (owner: some
    # taps do nothing). A tap is already a visible, release-confirmed choice: the gate is for a controller
    # selection scrolled away by a finger, not for the tapped row itself.
    # ------------------------------------------------------------------------------------------
    r = Result("CG70-held-item-tap-commits-any-row",
               "Held Item picker: a completed tap on any published row commits it (no reveal-only gate for taps) in "
               "Gen III and the shared Gen IV / V picker (owner Oct 25 L06)")
    for rel in ("src/UI/Gen3SharedPokemonSurface.inc", "src/UI/SharedPokemonEditorSurface.inc"):
        body = body_of(src(rel), "handleValuePicker")
        if not body:
            r.violations.append(f"{rel}: handleValuePicker not found (update this check)")
            continue
        for m in re.finditer(r"if\s*\(([^{;]*!\s*state\.pickerViewport\.containsSelection\s*\([^)]*\)[^{;]*)\)\s*\{", body):
            if not re.search(r"(?i)tap|touch", m.group(1)):
                r.violations.append(f"{rel}: the picker's A gate also refuses a completed tap on a partly visible row "
                                    "(tap only reveals; press A again)")
    res.append(r)

    # ------------------------------------------------------------------------------------------
    # Gen III held item vs caught Ball (lane S, owner Oct 25 decision): Safari Ball (5) / Berry Juice (44)
    # are never offered as HELD items (PKHeX ItemStorage3RS.Unreleased); Safari stays a valid caught Ball;
    # a record that already holds one is kept and labelled honestly; Gen III Review names Gen III items.
    # ------------------------------------------------------------------------------------------
    r = Result("CG71-gen3-held-item-vs-caught-ball",
               "Gen III: unreleased held items never offered, a stored one is labelled honestly, Safari stays a caught "
               "Ball, Review uses Gen III item names (owner Oct 25 held-item policy)")
    hi = src("include/Integration/Gen3/Gen3HeldItems.h")
    if not hi or not re.search(r"kUnreleasedHeldItems\s*\{\{\s*5\s*,\s*44\s*\}\}", hi.code):
        r.violations.append("include/Integration/Gen3/Gen3HeldItems.h: Safari Ball / Berry Juice are not the unreleased held items")
    if g3 and not re.search(r"releasedHeldItem\s*\(", g3.code):
        r.violations.append("src/UI/Gen3SharedPokemonSurface.inc: the Held Item picker does not filter unreleased items")
    if g3 and not any(re.search(r"(?i)unreleased", lit) for _, lit in g3.literals):
        r.violations.append("src/UI/Gen3SharedPokemonSurface.inc: a stored unreleased held item (Safari Ball / Berry Juice) "
                            "is shown and offered with no honest label")
    bl = src("include/Enums/Ball.h")
    if bl and not re.search(r"\{\s*1\s*,\s*2\s*,\s*3\s*,\s*4\s*,\s*5\b", bl.code):
        r.violations.append("include/Enums/Ball.h: Gen III caught Balls no longer include Safari (5)")
    g3e = src("src/Integration/Gen3/Gen3StagedPokemonEditor.cpp")
    if g3e and re.search(r'field\s*\(\s*"Held Item"\s*,\s*Names::getItemName\s*\(', g3e.code):
        r.violations.append("src/Integration/Gen3/Gen3StagedPokemonEditor.cpp: Gen III Review names the held item from the "
                            "non-Gen III item table (getItemName, not getItemNameG3)")
    res.append(r)

    # ------------------------------------------------------------------------------------------
    # P2 (lane C): Gold / Silver store no trainer gender and Gen I has none. No Gender row, no stale
    # "Male" painted over the page (Gen2HardwareFinalFix drawGoldSilverTrainerGender used the old
    # geometry and overlapped), and the read-only trainer does not invent one.
    # ------------------------------------------------------------------------------------------
    r = Result("CG72-gold-silver-trainer-no-gender",
               "Gold / Silver and Gen I Trainer pages show no Gender row and nothing paints one over the page "
               "(owner Oct 25 R2-TRAINER / NEW-18)")
    tvb = src("src/UI/TrainerViewScreenBase.inc")
    if tvb:
        for line, lit in tvb.literals:
            if lit == "Male (fixed by game)":
                r.violations.append(f"src/UI/TrainerViewScreenBase.inc:{line}: Gold / Silver Trainer shows an invented Gender row")
    ff = src("src/UI/Gen2HardwareFinalFix.inc")
    if ff and re.search(r"\bdrawGoldSilverTrainerGender\s*\(", ff.code):
        r.violations.append("src/UI/Gen2HardwareFinalFix.inc: drawGoldSilverTrainerGender paints a stale Gender / Male row "
                            "over the Gold / Silver Trainer page")
    gsc = src("src/Legacy/GSCReadOnlyTrainer.cpp")
    if gsc and re.search(r'"gold_gbc"[^;{]*"silver_gbc"[^{]*\{[^}]*trainerGenderAvailable_\s*=\s*true', gsc.code):
        r.violations.append("src/Legacy/GSCReadOnlyTrainer.cpp: Gold / Silver report a trainer gender they do not store")
    res.append(r)

    # ------------------------------------------------------------------------------------------
    # P3 (lane L): a VISIBLE Review prompt on Trainer, Bag, Party, Boxes, Create and Edit in every
    # generation (owner Oct 25 B08 / P1-RETURN: Review was buried in + Options and the save-exit flow).
    # Each screen's footer must name Review; the Trainer page shows it whether or not a row is editable.
    # ------------------------------------------------------------------------------------------
    r = Result("CG73-review-entry-visible-gen1-5",
               "Trainer / Bag / Party / Boxes / Create / Edit footers show a Review prompt in Gen I-V "
               "(owner Oct 25 B08 / P1-RETURN)")
    cts = src("include/UI/ClassicTrainerSession.h")
    tp = body_of(cts, "trainerPageInstructions")
    if not tp:
        r.violations.append("include/UI/ClassicTrainerSession.h: trainerPageInstructions not found (update this check)")
    else:
        for lit in re.findall(r'"([^"]*)"', tp):
            if "Back" in lit and "Review" not in lit:
                r.violations.append(f"include/UI/ClassicTrainerSession.h: Gen I-IV Trainer footer {lit!r} hides Review")
    inv = src("include/UI/InventoryUIContract.h")
    if inv:
        for line, lit in inv.literals:
            if "Options" in lit and "Back" in lit and "Review" not in lit:
                r.violations.append(f"include/UI/InventoryUIContract.h:{line}: Gen I-III Bag footer {lit!r} shows no Review")
    tv = src("src/UI/TrainerViewScreenBase.inc")
    # Lane L (round 6): the old literal Party / Boxes / Storage footers in TrainerViewScreenBase serve only
    # editable (installed-title) sources. Every Gen I-IV emulator session is RetroArchLegacy / ExternalLegacy,
    # has no Edit, so the read-only override replaces them with ClassicBrowseFooter, whose Review slot is on
    # for Gen I-IV Party / Boxes / Trainer. The literals are exempt only while all of that is proven here.
    inv_ovr = body_of(inv, "useReadOnlyInventoryFooterOverride")
    pol = src("include/Safety/SourceMutationPolicy.h")
    uic = src("src/UI/UI.cpp")
    staged_kinds = [m.start() for m in re.finditer(r"SourceKind::(?:BackupOrStaged|AppOwnedStorage)\b", uic.code)] if uic else []
    htv = [(m.start(), m.start() + len(b_)) for m in re.finditer(r"bool\s+UIManager::handleTrainerView\s*\(", uic.code)
           for b_ in [block_after(uic.code, uic.code.find("{", m.end()) + 1)]] if uic else []
    override_proven = bool(
        re.search(r"return\s+sourceReadOnly\s*&&\s*!detailsActive\s*&&\s*!settingsMode\s*&&\s*"
                  r"!stagedInventoryActionsAvailable\s*;", inv_ovr) and
        pol and re.search(r"return\s+source\s*==\s*SourceKind::BackupOrStaged\s*\|\|\s*source\s*==\s*"
                          r"SourceKind::AppOwnedStorage\s*;", pol.code) and
        staged_kinds and all(any(lo <= k < hi for lo, hi in htv) for k in staged_kinds) and
        tv and re.search(r"spec\.review\s*=\s*true\s*;", tv.code) and
        re.search(r"selectedMode\s*==\s*ViewMode::Party\s*\|\|\s*selectedMode\s*==\s*ViewMode::Boxes\s*\|\|\s*"
                  r"selectedMode\s*==\s*ViewMode::Trainer", tv.code) and
        re.search(r"BrowseFooter::instructions\s*\(\s*spec\s*\)", tv.code))
    if tv:
        if not override_proven:
            for line, lit in tv.literals:
                if "A: Actions" in lit and "B: Back" in lit and "Review" not in lit:
                    r.violations.append(f"src/UI/TrainerViewScreenBase.inc:{line}: Gen I-IV Party / Boxes footer {lit!r} shows no Review")
        m = re.search(r'footer\.push\(\s*"A"\s*,\s*"Edit Quantity"\s*\)', tv.code)
        if m:
            region = block_after(tv.code, tv.code.rfind("{", 0, tv.code.rfind("Presentation::Footer footer;", 0, m.start())) + 1)
            if not re.search(r'footer\.push\(\s*"[^"]+"\s*,\s*"[^"]*Review', region):
                r.violations.append(f"src/UI/TrainerViewScreenBase.inc:{ln_at(tv, m.start())}: Gen IV Bag footer shows no Review")
    box = src("src/UI/ClassicPackedMoveOverlay.inc")
    if box:
        for m in re.finditer(r'drawNavBar\s*\(\s*fb\s*,\s*\{\{"A"\s*,\s*"Actions"\}[^;]*;', box.code):
            if "Review" not in m.group(0):
                r.violations.append(f"{box.rel}:{ln_at(box, m.start())}: Gen I-III Boxes footer shows no Review")
    # Create / Edit footers. Dead renderers are exempt only while they are proven unreachable:
    # - Gen II drawSharedWorkspace: its only caller is drawSharedSurface's mode != None branch, and the
    #   only caller of drawSharedSurface is drawUnifiedGen2Surface after it returned for mode != None;
    # - Gen I Cleanup3 Edit / AddDraft footers: Foundation's drawOverlayUX sends the main modes (View,
    #   Edit, AddDraft) to foundationDrawWorkspace, the composite calls drawFoundationPicker only with a
    #   picker or the Move editor, and ux3DrawTouchNavBar returns for every picker before the mode switch.
    g2s = src("src/UI/Gen2SharedPokemonSurface.inc")
    g2u = src("src/UI/Gen2UnifiedPokemonWorkspace.inc")
    callers = [f for f in sources if re.search(r"\bdrawSharedSurface\s*\(\s*screen\s*,\s*fb\s*\)\s*;", f.code)]
    dsu = body_of(g2u, "drawUnifiedGen2Surface")
    dss = body_of(g2s, "drawSharedSurface")
    g2_dead = bool(
        callers == [g2u] and dsu.count("drawSharedSurface(") == 1 and
        re.search(r"if\s*\(\s*stateFor\(screen\)\.mode\s*!=\s*Mode::None\s*\)\s*\{\s*drawUnifiedWorkspace\(screen,\s*fb\);\s*"
                  r"return\s+true;\s*\}\s*return\s+drawSharedSurface\(screen,\s*fb\);", dsu) and
        g2s and len(re.findall(r"\bdrawSharedWorkspace\s*\(\s*screen", g2s.code)) == 1 and
        re.search(r"if\s*\(\s*stateFor\(screen\)\.mode\s*!=\s*Mode::None\s*\)\s*\{\s*drawSharedWorkspace", dss))
    g1f = src("src/UI/Gen1PokemonEditorOverlayFoundation.inc")
    g1h = src("src/UI/Gen1PokemonEditorFoundationHardwareFix.inc")
    g1c = src("src/UI/Gen1PokemonEditorOverlayUXCleanup3.inc")
    g12 = src("src/UI/Gen1PokemonEditorOverlayUXCleanup2.inc")
    comp = src("src/UI/TrainerViewScreenCompositeOverlay.cpp")
    nav = body_of(g1c, "ux3DrawTouchNavBar")
    g1_dead = bool(
        g1f and re.search(r"return\s+mode\s*==\s*UX2Mode::View\s*\|\|\s*mode\s*==\s*UX2Mode::Edit\s*\|\|\s*"
                          r"mode\s*==\s*UX2Mode::AddDraft\s*;", g1f.code) and
        re.search(r"if\s*\(\s*foundationMainMode\(state\.mode\)\s*\)\s*\{\s*foundationDrawWorkspace\(screen,\s*fb\);\s*return;",
                  body_of(g1f, "drawOverlayUX")) and
        comp and len(re.findall(r"drawFoundationPicker\s*\(", comp.code)) == 1 and
        re.search(r"\(Gen1PokemonEditor::foundationPickerActive\(\*this\)\s*\|\|\s*Gen1PokemonEditor::foundationMoveEditorActive"
                  r"\(\*this\)\)\)\s*\{\s*Gen1PokemonEditor::drawFoundationPicker", comp.code) and
        g12 and re.search(r"enum\s+class\s+UX2Picker\s*:\s*uint8_t\s*\{\s*None\s*,\s*Species\s*,\s*Move\s*\}", g12.code) and
        re.search(r"UX2Picker::Species\)\s*\{[^}]*return;\s*\}\s*if\s*\(\s*state\.picker\s*==\s*UX2Picker::Move\s*\)\s*\{[^}]*return;",
                  nav, re.S) and
        g1h and re.search(r"if\s*\(\s*!foundationMainMode\(state\.mode\)\s*&&\s*state\.mode\s*!=\s*UX2Mode::MoveEditor\s*\)",
                          body_of(g1h, "drawFoundationPicker")))
    exempt = {}
    if g2_dead and g2s:
        exempt["src/UI/Gen2SharedPokemonSurface.inc"] = body_of(g2s, "drawSharedWorkspace")
    if g1_dead and g1c:
        exempt["src/UI/Gen1PokemonEditorOverlayUXCleanup3.inc"] = nav
    for rel in ("src/UI/Gen1PokemonEditorOverlayFoundation.inc", "src/UI/Gen1PokemonEditorFoundationHardwareFix.inc",
                "src/UI/Gen1PokemonEditorOverlayUXCleanup3.inc", "src/UI/Gen2SharedPokemonSurface.inc",
                "src/UI/Gen3SharedPokemonSurface.inc", "src/UI/SharedPokemonEditorSurface.inc"):
        f = src(rel)
        if not f:
            continue
        for m in re.finditer(r'drawNavBar\s*\(\s*fb\s*,\s*\{[^;]*\{"A"\s*,\s*"Edit"\}[^;]*\{"B"\s*,\s*"Exit"\}[^;]*;', f.code):
            if "Review" in m.group(0):
                continue
            if rel in exempt and m.group(0) in exempt[rel]:
                continue
            r.violations.append(f"{rel}:{ln_at(f, m.start())}: Create / Edit footer shows no Review")
    wl = src("include/UI/Gen2WorkspaceLayers.h")
    if not wl or not re.search(r'push\(\s*f\s*,\s*"[^"]+"\s*,\s*"Review"\s*\)', wl.code):
        r.violations.append("include/UI/Gen2WorkspaceLayers.h: the Gen II unified Create / Edit footer shows no Review")
    res.append(r)

    # ------------------------------------------------------------------------------------------
    # P3 Create Review details (lane S): a created Pokemon's Review shows what was created (moves, held
    # item, Ball, ... as stored), not only "Empty -> PIKACHU Lv 25". Each generation's Create change
    # builder must go on to describe the created record's fields.
    # ------------------------------------------------------------------------------------------
    r = Result("CG74-create-review-details",
               "Review of a created Pokemon lists its details (moves, held item, Ball, ...), not just Empty -> "
               "species (owner Oct 25 P3)")
    FIELD_RX = re.compile(r"(?i)\b(moves?|held\s*item|heldItem|ball|nature|ability|met\s*location|metLocation|"
                          r"ivs?|dvs?|ot\b|otName)\b")
    for rel, anchor_rx, what in (
            ("src/Integration/Gen1/Gen1StagedPokemonEditor.cpp", r'add\("presence",std::string\(C::kAddPokemon\)', "Gen I"),
            ("src/Integration/Gen2/Gen2StagedEditor.cpp", r'std::string\(C::kAddPokemon\)', "Gen II"),
            ("src/Integration/Gen3/Gen3StagedPokemonEditor.cpp", r'std::string\(C::kAddPokemon\)', "Gen III")):
        f = src(rel)
        m = re.search(anchor_rx, f.code) if f else None
        if not m:
            r.violations.append(f"{rel}: {what} Create Review row not found (update this check)")
            continue
        window = f.code[m.end():m.end() + 1200]
        cut = re.search(r"\breturn\b|\bcontinue\b", window)
        window = window[:cut.start()] if cut else window
        # Count the row labels the Review shows (string literals), not C++ identifiers such as moveList,
        # getMoveName or after->heldItem, which a row-less window still contains.
        labels = " ".join(re.findall(r'"((?:[^"\\]|\\.)*)"', window))
        if len(set(x.lower() for x in FIELD_RX.findall(labels))) < 3:
            r.violations.append(f"{rel}:{ln_at(f, m.start())}: {what} Create Review row stops at the species / level")
    # Gen IV builds Review rows in lambdas (describeStagedReviewChanges: compare / created). A slot that
    # was empty before (a.empty()) is a created Pokemon: before compare()'s early return for an empty or
    # unchanged slot, a created branch must return a describer whose body lists the record's fields.
    g4x = src("src/Integration/Gen4/Gen4ExportTransaction.cpp")

    def lambda_body(f, name):
        m = re.search(r"\bconst\s+auto\s+" + re.escape(name) + r"\s*=\s*\[[^\]]*\]\s*\([^)]*\)\s*\{", f.code) if f else None
        return block_after(f.code, m.end()) if m else ""
    cmp_ = lambda_body(g4x, "compare")
    if not cmp_:
        r.violations.append("src/Integration/Gen4/Gen4ExportTransaction.cpp: compare lambda not found (update this check)")
    else:
        early = re.search(r"if\s*\(([^;{]*\ba\.empty\(\)[^;{]*)\)\s*return\s*;", cmp_)
        head = cmp_[:early.start()] if early else cmp_
        made = re.search(r"if\s*\(([^;{]*\ba\.empty\(\)[^;{]*)\)\s*return\s+(\w+)\s*\(\s*b\b", head)
        if early and not made:
            r.violations.append("src/Integration/Gen4/Gen4ExportTransaction.cpp: compare() returns early for an empty "
                                "'before' slot, so a created Pokemon gets no Review detail")
        elif made:
            if re.search(r"!\s*a\.empty\(\)", made.group(1)) or not re.search(r"!\s*b\.empty\(\)", made.group(1)):
                r.violations.append("src/Integration/Gen4/Gen4ExportTransaction.cpp: the created branch is not 'empty before, "
                                    "filled after'")
            fields = lambda_body(g4x, made.group(2)) or body_of(g4x, made.group(2))
            if len(set(x.lower() for x in FIELD_RX.findall(fields))) < 3:
                r.violations.append(f"src/Integration/Gen4/Gen4ExportTransaction.cpp: {made.group(2)}() does not list the "
                                    "created Pokemon's fields")
    sr5 = src("include/Integration/Gen5/Gen5SharedReview.h")
    if sr5:
        m = re.search(r"Field::CreateBox", sr5.code)
        window = sr5.code[m.end():m.end() + 800] if m else ""
        if not m or len(set(x.lower() for x in FIELD_RX.findall(window))) < 3:
            r.violations.append("include/Integration/Gen5/Gen5SharedReview.h: Gen V Create Review row stops at the species")
    res.append(r)

    # ------------------------------------------------------------------------------------------
    # P5 (lane P): no two Met Location / event rows read the same. Gen V POKEPARK (40097-40108, twelve
    # identical names) and Festa rows (differ only by a number) must be told apart, and unknown provenance
    # stays honest. Evidence required: a host-test check that enumerates the Gen IV and Gen V native
    # choices through the display layer and requires the displayed rows to be distinct.
    # ------------------------------------------------------------------------------------------
    r = Result("CG75-met-location-rows-distinguishable",
               "Every Gen IV / V Met Location choice displays distinctly (POKEPARK / Festa rows told apart); "
               "a host test enumerates the native choices (owner Oct 25 R2-CHECK-011)")
    # Host tests and Makefiles are outside the scanned sources: read them from the same checkout.
    root = Path(str(sources[0].path)[:-len(sources[0].rel)]) if sources else Path(".")
    mk = "\n".join(m.read_text(errors="replace") for m in root.glob("Makefile.host*"))
    proof = []
    for t in sorted((root / "tests").glob("*.cpp")):
        code = t.read_text(errors="replace")
        if re.search(r"getMetLocationDisplayName|metLocationText5|metLocationDisplay", code) and \
                re.search(r"NativeMetLocations5|metLocationChoices", code) and \
                re.search(r"std::(?:unordered_)?set\s*<\s*std::string|adjacent_find|distinct|unique", code) and \
                re.search(r"\b" + re.escape(t.stem) + r"\b", mk):
            proof.append(t.name)
    if not proof:
        r.violations.append("no host test enumerates the Gen V native Met Location choices and requires distinct "
                            "displayed rows (POKEPARK x12, Festa)")
    res.append(r)

    # ------------------------------------------------------------------------------------------
    # P6 (lane R): the Gen II Unown letter shown is always f(current DVs). The workspace portrait must not
    # use the cached record form, and making a non-I/V Unown shiny must say the letter changes (a shiny
    # Gen II Unown can only be I or V: Def / Spd / Spc 10).
    # ------------------------------------------------------------------------------------------
    r = Result("CG76-gen2-unown-letter-from-dvs",
               "Gen II Unown: every portrait derives the letter from the current DVs, and Shiny says when it changes "
               "the letter (owner Oct 25 R2-REROLL)")
    # Lane R (8613b13d) keeps the draft letter in working.form and recomputes it from the DVs wherever the
    # draft DVs change; portraits may read that form. So: every Gen II UI function that writes the draft's
    # DVs must also set its form from unownLetter in the same function (no stale cached letter).
    for f in sources:
        if not re.search(r"^(?:src/UI/Gen2\w*\.inc|include/UI/Gen2\w*\.h)$", f.rel):
            continue
        for name, ln, body in function_definitions(f.code):
            if not re.search(r"\b(?:p|working|draft)\s*\.\s*dvs\s*\[[^\]]*\]\s*=[^=]", body):
                continue
            if not re.search(r"\.\s*form\s*=[^=][^;]*unownLetter\s*\(", body):
                r.violations.append(f"{f.rel}:{ln}: {name}() changes the draft DVs without recomputing the Unown "
                                    "letter from them (stale letter / sprite)")
    for f in sources:
        if not re.search(r"src/UI/Gen2\w*\.inc$", f.rel):
            continue
        for m in re.finditer(r"\bdrawPortrait\s*\(([^;]*)\)\s*;", f.code):
            arg = m.group(1)
            if re.search(r"\b\w+\s*->\s*form\b(?!\s*\()", arg) and "unownLetter" not in arg:
                r.violations.append(f"{f.rel}:{ln_at(f, m.start())}: Gen II portrait uses a record's stored form "
                                    "field directly, not the DV-derived letter")
    fnd = src("src/UI/Gen2PokemonEditorFoundation.inc")
    ts = body_of(fnd, "toggleShiny")
    if not ts:
        r.violations.append("src/UI/Gen2PokemonEditorFoundation.inc: toggleShiny not found (update this check)")
    elif not re.search(r"(?i)unownLetter|letter", ts):
        r.violations.append("src/UI/Gen2PokemonEditorFoundation.inc: toggleShiny changes Unown DVs without telling the "
                            "owner the letter changes")
    res.append(r)

    # ------------------------------------------------------------------------------------------
    # E05 / P4 (lane R): every control a footer or Help row shows has a glyph and a handler, and every
    # handled control is shown. Owner Oct 25: Game Browser Controls, Party footers and Help rows without
    # glyphs; stale Review lines.
    # ------------------------------------------------------------------------------------------
    r = Result("CG77-footer-help-controls-have-handlers",
               "Help rows name their control as a glyph-able button; Game Browser / Gen I-III Boxes footers list "
               "every control their handler acts on (owner Oct 25 E05 / P4)")
    # Round 7 (B04): the Bag's rows moved to the shared InventoryHelp.h, checked with the same rule.
    # An #include path is not a Help row.
    for help_rel in ("include/UI/ClassicHelpPresentation.h", "include/UI/InventoryHelp.h"):
        hp = src(help_rel)
        if not hp:
            r.violations.append(f"{help_rel}: missing")
            continue
        include_lines = {hp.code.count("\n", 0, m.start()) + 1
                         for m in re.finditer(r'^\s*#\s*include\s*"', hp.code, re.M)}
        for line, lit in hp.literals:
            if line in include_lines:
                continue
            if lit.startswith("Original save unchanged") or lit in ("Help", "Inventory Controls"):
                continue
            m = re.search(r" {2,}", lit)
            if not m or not _glyph_keys(lit[:m.start()]):
                r.violations.append(f"{help_rel}:{line}: Help row {lit!r} names no glyph "
                                    "control before its description (drawn as plain text)")
    ss = src("src/UI/SaveSelectScreen.cpp")
    if ss:
        m = re.search(r'\{"ZR"\s*,\s*"Launch"\}', ss.code)
        hb = re.search(r"if\s*\(\s*classicGamesActive\s*\)", ss.code)
        if not m:
            r.violations.append("src/UI/SaveSelectScreen.cpp: Game Browser footer not found (update this check)")
        elif hb and re.search(r"HidNpadButton_Minus", block_after(ss.code, ss.code.find("{", hb.end()) + 1)):
            stmt = ss.code[ss.code.rfind(";", 0, m.start()):ss.code.find(";", m.end())]
            if not re.search(r'\{"-"\s*,', stmt):
                r.violations.append("src/UI/SaveSelectScreen.cpp: Game Browser handles - (Help) but its footer "
                                    "does not show it")
    box = src("src/UI/ClassicPackedMoveOverlay.inc")
    if box:
        for m in re.finditer(r'drawNavBar\s*\(\s*fb\s*,\s*\{\{"A"\s*,\s*"Actions"\}[^;]*;', box.code):
            ln = box.code.count("\n", 0, m.start()) + 1
            if not re.search(r'\{"-"\s*,', m.group(0)):
                r.violations.append(f"{box.rel}:{ln}: Gen I-III Boxes footer drops - Help, which the screen handles")
            if not re.search(r'\{"ZL/ZR"\s*,', m.group(0)):
                r.violations.append(f"{box.rel}:{ln}: Gen I-III Boxes footer omits ZL / ZR jump, which the screen "
                                    "handles and Help claims")
    res.append(r)

    # ------------------------------------------------------------------------------------------
    # 7D (lane P): a completed tap outside a NON-destructive popup (Help, value pickers, Party Actions)
    # closes it and consumes the tap (no tap-through to what is underneath). Destructive confirmations
    # (Discard All, Release, Backup Delete) never close on an outside tap.
    # ------------------------------------------------------------------------------------------
    r = Result("CG78-tap-outside-dismisses-non-destructive",
               "A tap outside Help / pickers / Party Actions closes them and is consumed; Discard / Release / "
               "Delete confirmations ignore outside taps (owner Oct 25 7D)")
    OUTSIDE = re.compile(r"\b\w*[Oo]utside\w*\s*\(")
    non_destructive = []
    tv = src("src/UI/TrainerViewScreenBase.inc")
    b, ln = block(tv, r"if\s*\(\s*helpOverlayActive\s*\)\s*\{")
    non_destructive.append(("src/UI/TrainerViewScreenBase.inc", ln, "Help overlay", b))
    b, ln = block(ss, r"if\s*\(\s*overlay\s*==\s*Overlay::Help\s*\)\s*\{")
    non_destructive.append(("src/UI/SaveSelectScreen.cpp", ln, "Game Browser Help", b))
    for rel in ("src/UI/Gen3SharedPokemonSurface.inc", "src/UI/SharedPokemonEditorSurface.inc"):
        non_destructive.append((rel, 0, "value picker", body_of(src(rel), "handleValuePicker") or None))
    non_destructive.append(("src/UI/ClassicPartyActions.inc", 0, "Party Actions", body_of(src("src/UI/ClassicPartyActions.inc"), "handle") or None))
    for rel, ln, what, body in non_destructive:
        if body is None:
            r.violations.append(f"{rel}: {what} input handler not found (update this check)")
            continue
        m = OUTSIDE.search(body)
        if not m:
            r.violations.append(f"{rel}:{ln}: {what} does not close on a tap outside it")
        elif not re.search(r"\breturn\b", body[m.end():m.end() + 300]):
            r.violations.append(f"{rel}:{ln}: {what} outside tap is not consumed (tap-through)")
    destructive = []
    bs = src("src/UI/BackupSelectionScreen.cpp")
    b, ln = block(bs, r"if\s*\(\s*showDeleteConfirmation\s*\)\s*\{")
    destructive.append(("src/UI/BackupSelectionScreen.cpp", ln, "Backup Delete confirmation", b))
    rel = src("src/UI/ClassicReleaseActionFix.inc")
    b, ln = block(rel, r"if\s*\(\s*confirm\.active\s*\)\s*\{")
    destructive.append(("src/UI/ClassicReleaseActionFix.inc", ln, "Release confirmation", b))
    dis = src("include/UI/Gen12ReviewDiscardConfirm.h")
    destructive.append(("include/UI/Gen12ReviewDiscardConfirm.h", 0, "Discard All confirmation", dis.code if dis else None))
    for rel, ln, what, body in destructive:
        if body is not None and OUTSIDE.search(body):
            r.violations.append(f"{rel}:{ln}: {what} closes on an outside tap (destructive confirmations must not)")
    res.append(r)

    # ------------------------------------------------------------------------------------------
    # 7C (lane S): Gen V Language cycles inline; the changed value is the feedback, so there is no
    # "Language: ..." toast (Gen III Language and Crystal OT Gender post none).
    # ------------------------------------------------------------------------------------------
    r = Result("CG79-no-gen5-language-toast",
               "Gen V Language cycles inline without a redundant status toast (owner Oct 25 7C)")
    g5 = src("src/UI/Gen5SharedReadOnlySurface.inc")
    ed = body_of(g5, "editDetail")
    m = re.search(r"case\s+Language\s*:\s*\{", ed)
    if not m:
        r.violations.append("src/UI/Gen5SharedReadOnlySurface.inc: editDetail Language case not found (update this check)")
    else:
        branch = block_after(ed, m.end())
        if re.search(r"if\s*\(\s*stage\s*\([^;]*\)\s*\)\s*\{?\s*screen\.postStatus", branch) or \
                re.search(r'postStatus\s*\([^;]*"Language', branch):
            r.violations.append("src/UI/Gen5SharedReadOnlySurface.inc: Gen V Language posts a toast after a "
                                "successful inline change")
    res.append(r)
    return res


def _gen5_trainer_slot(te, tvb, sources):
    """CG90: replay Gen V drawTrainerEditor's fixed-pitch layout from its own constants at the real
    panel bounds; every row card and the Game language line must end at or above Review's top."""
    WHERE = "src/UI/Gen5SharedReadOnlySurface.inc"
    try:
        consts = dict(re.findall(r"constexpr\s+int\s+(LEFT_PANEL_Y|CONTENT_PANEL_HEIGHT)\s*=\s*(\d+)\s*;", tvb.code))
        env = {"y": int(consts["LEFT_PANEL_Y"]), "h": int(consts["CONTENT_PANEL_HEIGHT"])}
        assert re.search(r"\by\s*=\s*CONTENT_PANEL_Y\b", te) and re.search(r"\bh\s*=\s*CONTENT_PANEL_HEIGHT\b", te)
        env["hH"] = int(re.search(r"constexpr\s+int\s+hH\s*=\s*(\d+)\s*;", te).group(1))
        decl = re.search(r"constexpr\s+int\s+(cardW\s*=.*?);", te, re.S).group(1)
        for name, expr in re.findall(r"(\w+)\s*=\s*([^,]+)", decl):
            env[name] = eval(expr, {}, dict(env))
        fpx = re.search(r"kFontSizes\[[^\]]*\]\s*=\s*\{\s*([\d.]+)f?",
                        next(x for x in sources if x.rel == "src/UI/PKSEFramebuffer.cpp").code)
        caption_h = math.ceil(float(fpx.group(1)) * 1.364)   # Nunito line height (tests/host-ui/HostUi.h)
        cy = eval(re.search(r"int\s+cy\s*=\s*([^;]+);", te).group(1), {}, dict(env))
        review_y = eval(re.search(r"reviewY\s*=\s*([^;,]+)", te).group(1), {}, dict(env))
        start = re.search(r"int\s+cy\s*=", te).end()
        end = re.search(r"reviewY\s*=", te).start()
    except Exception as e:   # noqa: BLE001 - any parse miss means the page changed shape
        return [f"{WHERE}: Gen V Trainer layout not recognised ({type(e).__name__}; update this check)"]
    items, problems, pos = [], [], start
    step = re.compile(r"for\s*\(\s*const\s+TRow\s+row\s*:\s*\{([^}]*)\}\s*\)\s*\{|cy\s*\+=\s*([^;]+);|"
                      r"drawText\s*\(\s*cardX\s*\+\s*\d+\s*,\s*cy\s*\+\s*(\d+)\s*,[^;]*?\"Game language")
    while True:
        m = step.search(te, pos, end)
        if not m:
            break
        if m.group(1) is not None:
            d, j = 0, m.end()
            while j < len(te):
                d += te[j] == "{"
                if te[j] == "}":
                    if d == 0:
                        break
                    d -= 1
                j += 1
            body = te[m.end():j]
            hm = re.search(r"drawFilledRoundedRect\s*\(\s*cardX\s*,\s*cy\s*,\s*cardW\s*,\s*(\w+)", body)
            inc = re.search(r"cy\s*\+=\s*([^;]+);", body)
            if not hm or not inc:
                return [f"{WHERE}: Gen V Trainer row loop not recognised (update this check)"]
            for row in [x.strip().split("::")[-1] for x in m.group(1).split(",")]:
                items.append((row, cy + env[hm.group(1)]))
                cy += eval(inc.group(1), {}, dict(env))
            pos = j + 1
        elif m.group(2) is not None:
            cy += eval(m.group(2), {}, dict(env))
            pos = m.end()
        else:
            items.append(("Game language line", cy + int(m.group(3)) + caption_h))
            pos = m.end()
    if not items:
        return [f"{WHERE}: Gen V Trainer rows not recognised (update this check)"]
    for name, bottom in items:
        if bottom > review_y:
            problems.append(f"{WHERE}: Gen V Trainer {name} ends at y={bottom}, below Review's top y={review_y}")
    return problems


_TRAINER_PROBE_CACHE: dict = {}


def _trainer_review_slot(sources, tvb):
    """CG83: run include/UI/ClassicTrainerPage.h's layout at the real Trainer panel bounds for every
    Gen I-IV game and every Play Time / Gender writer combination; the Review button must sit inside
    the panel body, below every row (a measured slot under the last row, or the footer action area)."""
    page = next((x for x in sources if x.rel == "include/UI/ClassicTrainerPage.h"), None)
    if not page or not tvb:
        return ["include/UI/ClassicTrainerPage.h: page model not found (update this check)"]
    consts = dict(re.findall(r"constexpr\s+int\s+(LEFT_PANEL_X|LEFT_PANEL_Y|CONTENT_PANEL_HEIGHT)\s*=\s*(\d+)\s*;", tvb.code))
    call = re.search(r"drawTrainerView\s*\(\s*\*this\s*,\s*fb\s*,\s*contentX\s*,\s*CONTENT_PANEL_Y\s*,\s*"
                     r"contentPanelWidth\s*,\s*CONTENT_PANEL_HEIGHT\s*\)", tvb.code)
    cx = re.search(r"const\s+int\s+contentX\s*=\s*LEFT_PANEL_X\s*;", tvb.code)
    if len(consts) != 3 or not call or not cx or not re.search(r"CONTENT_PANEL_Y\s*=\s*LEFT_PANEL_Y\s*;", tvb.code):
        return ["src/UI/TrainerViewScreenBase.inc: Trainer panel bounds not found (update this check)"]
    x, y, h = int(consts["LEFT_PANEL_X"]), int(consts["LEFT_PANEL_Y"]), int(consts["CONTENT_PANEL_HEIGHT"])
    w = 1280 - x   # contentPanelWidth = fb.getWidth() - contentX on the 1280x720 framebuffer
    root = Path(str(page.path)[:-len(page.rel)]) if str(page.path).endswith(page.rel) else DEFAULT_ROOT
    cxx = shutil.which("g++") or shutil.which("clang++") or shutil.which("c++")
    if not cxx:
        return ["CG83 needs a host C++ compiler to run the Trainer page model (none found)"]
    games = (("red/blue/yellow", 1, False), ("gold/silver", 2, False), ("crystal", 2, True),
             ("ruby/sapphire/emerald/firered/leafgreen", 3, False),
             ("diamond/pearl/platinum/heartgold/soulsilver", 4, False))
    probe = """#include "UI/ClassicTrainerPage.h"
#include <cstdio>
namespace P = PokeBank::UIModel::ClassicTrainerPage;
int main() {
    struct G { const char* name; int gen; bool crystal; } games[] = {%s};
    for (const auto& g : games)
        for (int pt = 0; pt < 2; ++pt)
            for (int gd = 0; gd < 2; ++gd) {
                const auto page = P::layout(P::rows({g.gen, g.crystal, pt == 1, gd == 1}), %d, %d, %d, %d);
                const P::Rect body{%d, %d + P::kHeaderH, %d, %d - P::kHeaderH};
                int last = 0;
                for (const auto& placed : page.rows) {
                    if (placed.card.bottom() > last) last = placed.card.bottom();
                    if (placed.card.overlaps(page.review))
                        std::printf("%%s (play time %%s, gender %%s): Review overlaps the %%s row\\n", g.name,
                                    pt ? "editable" : "read-only", gd ? "editable" : "read-only",
                                    std::string(P::label(placed.row.field)).c_str());
                }
                if (!page.review.inside(body))
                    std::printf("%%s: Review is outside the Trainer panel body\\n", g.name);
                if (page.review.y < last)
                    std::printf("%%s (play time %%s, gender %%s): Review at y=%%d is not below the last row (bottom %%d)\\n",
                                g.name, pt ? "editable" : "read-only", gd ? "editable" : "read-only", page.review.y, last);
            }
}
""" % (", ".join('{"%s", %d, %s}' % (n, g, "true" if c else "false") for n, g, c in games), x, y, w, h, x, y, w, h)
    # The probe compiles the headers on disk, so its result is a function of their text and the bounds:
    # cache it (the mutation suites call contracts() dozens of times on one tree).
    key = (str(root), probe, tuple((p.name, p.read_bytes()) for p in sorted((root / "include/UI").glob("ClassicTrainer*.h"))))
    if key in _TRAINER_PROBE_CACHE:
        return list(_TRAINER_PROBE_CACHE[key])
    with tempfile.TemporaryDirectory() as tmp:
        cpp, exe = Path(tmp) / "probe.cpp", Path(tmp) / "probe"
        cpp.write_text(probe.replace("#include <cstdio>", "#include <cstdio>\n#include <string>"), encoding="utf-8")
        built = subprocess.run([cxx, "-std=c++20", "-I", str(root / "include"), str(cpp), "-o", str(exe)],
                               capture_output=True, text=True)
        if built.returncode:
            first = built.stderr.strip().splitlines()[0] if built.stderr.strip() else "no compiler output"
            return [f"include/UI/ClassicTrainerPage.h: page model probe does not build (update this check): {first}"]
        ran = subprocess.run([str(exe)], capture_output=True, text=True)
    out = [f"include/UI/ClassicTrainerPage.h: {line}" for line in dict.fromkeys(ran.stdout.splitlines())]
    if ran.returncode:
        out.append(f"include/UI/ClassicTrainerPage.h: page model probe exited {ran.returncode}")
    _TRAINER_PROBE_CACHE[key] = tuple(out)
    return out


def round7_contracts(sources):
    """Round 7: owner Oct 26 FAIL items and requests (docs/hardware/ROUND6_RESULTS_26186464.md).
    Called from contracts()."""
    res = []

    def src(rel):
        return next((x for x in sources if x.rel == rel), None)

    def body_of(f, name):
        return "\n".join(b for _, _, b in function_bodies(f.code, re.compile("^" + re.escape(name) + "$"))) if f else ""

    def block_after(code, pos):
        d, j = 0, pos
        while j < len(code):
            if code[j] == "{":
                d += 1
            elif code[j] == "}":
                d -= 1
                if d < 0:
                    break
            j += 1
        return code[pos:j]

    def block(f, anchor_rx):
        """Body of the first `if (...) {` block whose condition matches anchor_rx."""
        m = re.search(anchor_rx, f.code) if f else None
        if not m:
            return None, 0
        brace = f.code.find("{", m.end() - 1)
        return block_after(f.code, brace + 1), f.code.count("\n", 0, m.start()) + 1

    def missing(r, rel, what):
        r.violations.append(f"{rel}: {what} not found (update this check)")

    # ------------------------------------------------------------------------------------------
    # B08 defect 1 / P1-RETURN (lane L): Gen II Create -> + opens Review, but the final draw chain
    # tests the Create mode before the Review flag, so the Create workspace is repainted over the
    # Review that owns the input: an invisible screen that looks frozen. B in Review must return.
    # ------------------------------------------------------------------------------------------
    r = Result("CG80-gen2-create-review-visible",
               "Gen II Create / Edit -> Review is drawn (not occluded by the workspace) and B returns "
               "(owner Oct 26 B08, defect 1)")
    MODE_GATE = re.compile(r"if\s*\(([^{;]*?\bmode\s*!=\s*Mode::None[^{;]*?)\)\s*\{")

    def review_predicate(cond):
        """True when an if-condition tests the open Review: inline (`.review`) or through a predicate
        function whose own body tests it (lane L 146511e2: gen2WorkspaceReviewOpen)."""
        if re.search(r"\breview\b", cond):
            return True
        for name in re.findall(r"\b(\w+)\s*\(", cond):
            for f in sources:
                body = body_of(f, name) if re.search(r"\b" + name + r"\s*\(", f.code) else ""
                if re.search(r"\.\s*review\b", body) and re.search(r"\bmode\s*!=\s*Mode::None", body):
                    return True
        return False

    def review_first(body):
        """The body draws an open Review and returns before its first workspace paint (a Create / Edit
        mode gate, or a call into the base layer)."""
        first = min([m.start() for m in (MODE_GATE.search(body), re.search(r"drawFinalGen2SurfaceBase\s*\(", body)) if m],
                    default=len(body))
        for m in re.finditer(r"if\s*\(((?:[^()]|\([^()]*\))*)\)\s*\{", body[:first]):
            blk = block_after(body, m.end())
            if review_predicate(m.group(1)) and re.search(r"draw\w*Review\w*\s*\(", blk) and \
                    re.search(r"return\s+true\s*;", blk):
                return True
        return False

    # The frame is drawn by the outermost drawFinalGen2Surface (Gen2HardwareFinalFix.inc; the parity layer
    # is renamed drawFinalGen2SurfaceBase and only reached through it). The outer layer must draw an open
    # Review first; the base layer is exempt only while the outer one does.
    outer = body_of(src("src/UI/Gen2HardwareFinalFix.inc"), "drawFinalGen2Surface")
    base = body_of(src("src/UI/Gen2SharedSurfaceParity.inc"), "drawFinalGen2Surface")
    co = src("src/UI/TrainerViewScreenCompositeOverlay.cpp")
    renamed = co and re.search(r"#define\s+drawFinalGen2Surface\s+drawFinalGen2SurfaceBase", co.code)
    if not outer or not base or not renamed:
        missing(r, "src/UI/Gen2HardwareFinalFix.inc / Gen2SharedSurfaceParity.inc", "drawFinalGen2Surface layers")
    else:
        outer_ok = review_first(outer)
        if not outer_ok:
            r.violations.append("src/UI/Gen2HardwareFinalFix.inc: drawFinalGen2Surface paints the Create / Edit "
                                "workspace without first drawing an open Review (Review owns the input but is never visible)")
        if not outer_ok and not review_first(base):
            r.violations.append("src/UI/Gen2SharedSurfaceParity.inc: drawFinalGen2Surface paints the Create / Edit "
                                "workspace without first drawing an open Review")
    # The input owner hands keys to Review under the same test the draw uses.
    hu = body_of(src("src/UI/Gen2HardwareWorkspaceFix.inc"), "handleUnifiedGen2SurfaceInput")
    hm = re.search(r"if\s*\(((?:[^()]|\([^()]*\))*)\)\s*return\s+handleSharedReview\s*\(", hu)
    if not hm or not review_predicate(hm.group(1)):
        r.violations.append("src/UI/Gen2HardwareWorkspaceFix.inc: the Gen II input owner does not hand an open "
                            "Review its keys under the Review test")
    hr = body_of(src("src/UI/Gen2SharedPokemonSurface.inc"), "handleSharedReview")
    bm = re.search(r"if\s*\([^)]*HidNpadButton_B[^)]*\)\s*\{", hr)
    if not hr or not bm:
        missing(r, "src/UI/Gen2SharedPokemonSurface.inc", "handleSharedReview B branch")
    elif not re.search(r"\breview\s*=\s*false\b", block_after(hr, bm.end())):
        r.violations.append("src/UI/Gen2SharedPokemonSurface.inc: B in the Gen II Review does not close it")
    res.append(r)

    # ------------------------------------------------------------------------------------------
    # B08 defect 2 / P1-RETURN (lane L): each screen opens Review with exactly the one key its footer
    # shows. + must not secretly open Review where the footer shows Y (Party, Trainer), and a screen
    # where + opens Review must show it.
    # ------------------------------------------------------------------------------------------
    r = Result("CG81-one-review-key-per-screen",
               "One Review key per screen, matching its footer; no hidden + Review on Party / Trainer "
               "(owner Oct 26 B08, defect 2)")
    re_h = src("include/UI/ReviewEntry.h")
    plus = body_of(re_h, "plusOpensReview")
    if not plus:
        missing(r, "include/UI/ReviewEntry.h", "plusOpensReview")
    elif not re.search(r"\byOpensReview\b|Screen::(?:Party|Trainer)\b", plus):
        r.violations.append("include/UI/ReviewEntry.h: plusOpensReview is true on every screen, so + opens Review "
                            "on Party / Trainer where the footer shows only Y")
    tsi = src("include/UI/ClassicTrainerSessionInput.h")
    if tsi and re.search(r"HidNpadButton_Y\s*\|\s*HidNpadButton_Plus|HidNpadButton_Plus\s*\|\s*HidNpadButton_Y", tsi.code):
        r.violations.append("include/UI/ClassicTrainerSessionInput.h: Gen I-IV Trainer opens Review on + as well as "
                            "its footer's Y")
    g5 = src("src/UI/Gen5SharedReadOnlySurface.inc")
    if g5:
        for m in re.finditer(r"keys\s*&\s*\(\s*HidNpadButton_(?:Plus\s*\|\s*HidNpadButton_Y|Y\s*\|\s*HidNpadButton_Plus)\s*\)", g5.code):
            if "openReviewFromBrowse" in g5.code[m.end():m.end() + 300]:
                r.violations.append(f"src/UI/Gen5SharedReadOnlySurface.inc:{g5.code.count(chr(10), 0, m.start()) + 1}: "
                                    "Gen V Trainer opens Review on + as well as its footer's Y")
        for m in re.finditer(r"\(\s*keys\s*&\s*HidNpadButton_Plus\s*\)\s*\|\|[^;{]*?\bScreen::Party\b", g5.code):
            r.violations.append(f"src/UI/Gen5SharedReadOnlySurface.inc:{g5.code.count(chr(10), 0, m.start()) + 1}: "
                                "Gen V Party opens Review on + although its footer shows Y")
    co = src("src/UI/TrainerViewScreenCompositeOverlay.cpp")
    bf = body_of(co, "drawGen2ClassicBoxFooter")
    if co and bf and re.search(r"drawNavBar", bf) and not re.search(r"Review|footerHint", bf):
        r.violations.append("src/UI/TrainerViewScreenCompositeOverlay.cpp: Gen II Boxes footer hides the + Review "
                            "that the Boxes screen opens")
    res.append(r)

    # ------------------------------------------------------------------------------------------
    # C12 follow-up / G1 (lane L): Gen V exports are listed in Save Backups like Gen I-IV, with the
    # same ownership checks. Gen V writes exports/gen5/account-<id>/<stem>/EXPORT_MANIFEST.txt; the
    # listing reads exports/genN/<name>/EDIT_MANIFEST.txt, so no Gen V export is ever listed.
    # ------------------------------------------------------------------------------------------
    r = Result("CG82-gen5-exports-in-save-backups",
               "Gen V exports appear in Save Backups, with ownership enforced (owner Oct 26 C12 / G1)")
    cls = src("include/UI/SourceBackupClassification.h")
    opn = src("include/UI/SourceBackupOpen.h")
    g5x = src("src/Integration/Gen5/Gen5WorkingCopyExport.cpp")
    km = re.search(r'kManifestName\s*=\s*"([^"]+)"', cls.code) if cls else None
    if not (cls and opn and g5x and km):
        missing(r, "include/UI/SourceBackupClassification.h / src/Integration/Gen5/Gen5WorkingCopyExport.cpp",
                "export manifest name")
    else:
        CLS, OPN = "include/UI/SourceBackupClassification.h", "include/UI/SourceBackupOpen.h"
        def defn_body(f, name):
            """Body of `name`'s definition, for signatures function_definitions skips (multi-line, with
            `= {}` default arguments): balanced parameter list, then an opening brace."""
            for m in re.finditer(r"\b" + re.escape(name) + r"\s*\(", f.code):
                d, j = 1, m.end()
                while j < len(f.code) and d:
                    d += {"(": 1, ")": -1}.get(f.code[j], 0)
                    j += 1
                tail = re.match(r"\s*(?:const\s*)?(?:noexcept\s*)?\{", f.code[j:])
                if tail:
                    return block_after(f.code, j + tail.end())
            return ""

        lst, hist = defn_body(cls, "list"), defn_body(opn, "historicalSources")
        cd, osn = body_of(cls, "canDelete"), body_of(opn, "openSnapshot")
        # 1. The manifest Gen V writes is one Save Backups reads: the shared name, or (lane L ab46883e)
        #    a Gen V record name that the listing, the history scan and open / delete all read.
        g5_manifests = set(re.findall(r'"([A-Z0-9_]+MANIFEST\.txt)"', g5x.code))
        g5m = re.search(r'kGen5ManifestName\s*=\s*"([^"]+)"', cls.code)
        shared = re.search(r"\bkManifestName\b", g5x.code) or km.group(1) in g5_manifests
        own = g5m and g5m.group(1) in g5_manifests
        if not shared and not own:
            r.violations.append(f"src/Integration/Gen5/Gen5WorkingCopyExport.cpp: writes {sorted(g5_manifests)}; "
                                f"Save Backups reads {km.group(1)}")
        elif own and not shared:
            for rel, body, what in ((CLS, lst, "list"), (OPN, hist, "historicalSources")):
                if not re.search(r"kGen5ManifestName|manifestNameFor\s*\(", body):
                    r.violations.append(f"{rel}: {what} never reads the Gen V {g5m.group(1)}")
            for rel, body, what in ((CLS, cd, "canDelete"), (OPN, osn, "openSnapshot")):
                if not re.search(r"manifestNameFor\s*\(\s*\w+\.profileScoped\s*\)|kGen5ManifestName", body):
                    r.violations.append(f"{rel}: {what} does not read a Gen V record's own manifest")
        # 2. Gen V's account-<profile> folder is read for THIS profile only: one validated builder
        #    (32 hex), used by the listing and the history scan with the current profile; no scan of
        #    every account folder.
        if re.search(r'"account-"', g5x.code):
            acct = body_of(cls, "gen5AccountDir")
            vp = body_of(cls, "validProfile")
            if not acct or not re.search(r'"/gen5/account-"', acct) or "validProfile" not in acct or \
                    not re.search(r"size\s*\(\s*\)\s*!=\s*32", vp):
                r.violations.append(f"{CLS}: no validated exports/gen5/account-<32 hex profile> path "
                                    "(Gen V exports are never listed, or any profile's are)")
            if not re.search(r"gen5AccountDir\s*\(\s*exportsRoot\s*,\s*out\.profileIdentity\s*\)", lst):
                r.violations.append(f"{CLS}: list does not read this listing's Gen V account folder")
            if not re.search(r"gen5AccountDir\s*\(\s*exportsRoot\s*,\s*profileIdentity\s*\)", hist):
                r.violations.append(f"{OPN}: historicalSources does not read the current profile's Gen V folder")
            for rel, body in ((CLS, lst), (OPN, hist)):
                if re.search(r'children\s*\([^;]*"/gen5"', body) or re.search(r'starts_with\s*\(\s*"account-"|'
                                                                             r'compare\s*\([^;]*"account-"', body):
                    r.violations.append(f"{rel}: scans every Gen V account folder, not only the current profile's")
            ss = src("src/UI/SaveSelectScreen.cpp")
            if not ss or not re.search(r"SB::list\s*\([^;]*,\s*profile\s*\)", ss.code) or \
                    not re.search(r"SB::historicalSources\s*\([^;]*,\s*profile\s*\)", ss.code) or \
                    not re.search(r"const\s+std::string\s+profile\s*=\s*currentProfileIdentity\s*\(\s*\)", ss.code):
                r.violations.append("src/UI/SaveSelectScreen.cpp: Save Backups is not given the current profile, so "
                                    "no Gen V export is listed")
            # 3. Ownership: delete and open accept a Gen V record only under this listing's profile folder.
            for rel, body, what in ((CLS, cd, "canDelete"), (OPN, osn, "openSnapshot")):
                if not re.search(r"gen5AccountDir\s*\(\s*(?:listing\.exportsRoot|root)\s*,\s*listing\.profileIdentity\s*\)", body):
                    r.violations.append(f"{rel}: {what} does not confine a Gen V record to this profile's folder")
        # The export-record branch (after its manifest read), not the manual-backup branch, must check the
        # recorded source and the exact file set.
        er = re.search(r"readManifest\s*\(\s*dir\s*\+\s*\"/\"\s*\+\s*manifestName\s*\)", cd or "")
        tail = cd[er.end():] if er else ""
        if not cd or not er or "UnexpectedFiles" not in tail or not re.search(r"m\.sourcePath\s*!=\s*listing\.sourcePath", tail):
            r.violations.append("include/UI/SourceBackupClassification.h: canDelete no longer checks the record's "
                                "source and its exact files (ownership)")
    res.append(r)

    # ------------------------------------------------------------------------------------------
    # B06 / C06 NEW-18 (lane C): Gen I-IV Trainer uses Gen V's tokens: neutral values (no gold
    # read-only token), and the same Review button, which overlaps no row.
    # ------------------------------------------------------------------------------------------
    r = Result("CG83-classic-trainer-gen5-tokens",
               "Gen I-IV Trainer uses Gen V's value colour, no yellow values; Review sits in a measured slot "
               "below the rows and overlaps none, for every Gen I-IV game (owner Oct 26 B06 / C06)")
    tvb = src("src/UI/TrainerViewScreenBase.inc")
    ct = body_of(tvb, "drawClassicTrainerView")
    if not ct:
        missing(r, "src/UI/TrainerViewScreenBase.inc", "drawClassicTrainerView")
    else:
        if re.search(r"State::ReadOnly|ShinyStar|ClassicFieldColor|\bGold\b", ct):
            r.violations.append("src/UI/TrainerViewScreenBase.inc: Gen I-IV Trainer values use the gold read-only "
                                "token (Gen V uses Colors::Text)")
        if re.search(r'"[^"]*gold values', ct):
            r.violations.append("src/UI/TrainerViewScreenBase.inc: Gen I-IV Trainer caption still announces gold values")
    # Review button (coordinator round 7 scope): placed in a measured slot below the last row (or the
    # footer action area) and intersecting no row, computed with the page model at the real Trainer
    # panel bounds for every Gen I-IV game, and drawn exactly where the model puts it.
    if ct and not re.search(r"drawGlyphButton\s*\(\s*fb\s*,\s*page\.review\.x\s*,\s*page\.review\.y\s*,\s*"
                            r"page\.review\.w\s*,\s*page\.review\.h\s*,\s*\"Y\"", ct):
        r.violations.append("src/UI/TrainerViewScreenBase.inc: the Gen I-IV Trainer Review button is not drawn at "
                            "the page model's review rect")
    r.violations.extend(_trainer_review_slot(sources, tvb))
    res.append(r)

    # ------------------------------------------------------------------------------------------
    # A22 / C01 R2-SPECIAL (lane C): Gen II's calculated Sp. Atk / Sp. Def are NOT in the numeric
    # Calculated column; they sit in the lower-middle of the STATS panel, as Gen I's block does.
    # ------------------------------------------------------------------------------------------
    r = Result("CG84-gen2-special-lower-middle",
               "Gen II calculated Special is in the lower-middle STATS area like Gen I, not the numeric column "
               "(owner Oct 26 A22 / C01)")
    wp = src("include/UI/Gen2WorkspacePresentation.h")
    cc = body_of(wp, "calculatedCell")
    if cc and re.search(r'"Sp\.\s*(?:Atk|Def)"', cc):
        r.violations.append("include/UI/Gen2WorkspacePresentation.h: calculatedCell puts Sp. Atk / Sp. Def in the "
                            "numeric Calculated column")
    uw = src("src/UI/Gen2UnifiedPokemonWorkspace.inc")
    col = body_of(uw, "drawGen2CalculatedColumn")
    if col and re.search(r"row\s*<=\s*W::kSpecialDefenseRow", col):
        r.violations.append("src/UI/Gen2UnifiedPokemonWorkspace.inc: drawGen2CalculatedColumn draws the Special "
                            "rows inside the numeric column")
    md = src("src/UI/Modals/Gen2PokemonDetailsModal.cpp")
    if md and re.search(r'"A"\s*\+\s*std::to_string\([^)]*\)\s*\+\s*" / D"', md.code):
        r.violations.append("src/UI/Modals/Gen2PokemonDetailsModal.cpp: Special shown as \"A.. / D..\" in the Stat column")
    if not any(f and re.search(r'"CALCULATED SPECIAL', f.code) for f in
               (uw, src("src/UI/Gen2HardwareFinalFix.inc"), src("src/UI/Gen2HardwareWorkspaceFix.inc"))):
        r.violations.append("src/UI/Gen2*.inc: no calculated Special block in the STATS panel (Gen I reference: "
                            "\"CALCULATED SPECIAL STATS\" in Gen1PokemonEditorFoundationHardwareFix.inc)")
    res.append(r)

    # ------------------------------------------------------------------------------------------
    # B07 / C09 R2-REROLL, UNOWN-LETTER (lane R): the Unown letter, Shiny and Reroll never change
    # silently or do nothing: a change of letter or Shiny is confirmed first, choosing a letter says
    # the resulting shiny state, and a reroll names the DVs it changed.
    # ------------------------------------------------------------------------------------------
    r = Result("CG85-unown-letter-shiny-confirmed",
               "Gen II Unown: letter / Shiny changes are confirmed, a letter choice states shiny, reroll is "
               "never silent (owner Oct 26 B07 / C09)")
    # Lane R 47b8097e: the rules live in include/UI/Gen2UnownLetter.h (pick / requestShinyToggle return a
    # Question instead of applying; rerollStatus names what was kept and the DVs that changed) and the
    # unified Gen II workspace wires them (Letter grid, question A confirm / B cancel, Y reroll status).
    LM = "include/UI/Gen2UnownLetter.h"
    lm = src(LM)
    rq, pk = body_of(lm, "requestShinyToggle"), body_of(lm, "pick")
    rs, pt = body_of(lm, "rerollStatus"), body_of(lm, "pickerTitle")
    if not (rq and pk and rs and pt):
        missing(r, LM, "requestShinyToggle / pick / rerollStatus / pickerTitle")
    else:
        if not re.search(r"letterChanged[^;]*\)\s*return\s*\{\s*false", rq) or \
                not re.search(r"return\s*\{\s*true\s*,\s*current\s*,\s*\{\s*Ask::ChangeLetterForShiny", rq):
            r.violations.append(f"{LM}: Shiny on that changes the letter is not asked first (requestShinyToggle)")
        if not re.search(r"ShinyImpossible\s*\)\s*\{[^}]*PickAction::Ask\s*,\s*current\s*,\s*\{\s*Ask::DropShinyForLetter", pk):
            r.violations.append(f"{LM}: a letter that cannot be shiny is not offered with Shiny off (pick)")
        if not re.search(r"\bshiny\b", pt):
            r.violations.append(f"{LM}: the Letter picker does not state the current shiny state")
        if not re.search(r"dvChanges\s*\(\s*before\s*,\s*after\s*\)", rs) or not re.search(r"if\s*\(\s*!\s*changed\s*\)", rs):
            r.violations.append(f"{LM}: rerollStatus does not name the changed DVs and why nothing changed")
    act = body_of(uw, "activateUnifiedField")
    rqpos, tgpos = act.find("requestShinyToggle("), act.find("toggleShiny(screen, p)")
    if not act:
        missing(r, "src/UI/Gen2UnifiedPokemonWorkspace.inc", "activateUnifiedField")
    else:
        if rqpos < 0 or (tgpos >= 0 and rqpos > tgpos) or \
                not re.search(r"if\s*\(\s*request\.ask\s*\)\s*\{[^}]*unownQuestion\s*=\s*request\.question[^}]*return\s+true", act):
            r.violations.append("src/UI/Gen2UnifiedPokemonWorkspace.inc: the Shiny row toggles Unown before asking "
                                "about a letter change")
        if re.search(r"chooseUnownLetter|promptText\s*\(\s*\"Unown", act) or "openPicker(" not in act:
            r.violations.append("src/UI/Gen2UnifiedPokemonWorkspace.inc: the Letter row does not open the Unown grid")
    hq, hpk = body_of(uw, "handleUnownQuestion"), body_of(uw, "handleUnownPicker")
    if not hq or not hpk:
        missing(r, "src/UI/Gen2UnifiedPokemonWorkspace.inc", "handleUnownQuestion / handleUnownPicker")
    else:
        am = re.search(r"if\s*\([^{]*HidNpadButton_A[^{]*\)\s*\{", hq)
        bm = re.search(r"if\s*\([^{]*HidNpadButton_B[^{]*\)\s*\{", hq)
        a_body = block_after(hq, am.end()) if am else ""
        b_body = block_after(hq, bm.end()) if bm else ""
        if "applyUnownDVs" not in a_body or "applyUnownDVs" in b_body or "postStatus" not in b_body:
            r.violations.append("src/UI/Gen2UnifiedPokemonWorkspace.inc: the Unown question is not A = apply, "
                                "B = cancel with a message")
        if not re.search(r"PickAction::Ask\s*\)\s*\{[^}]*unownQuestion\s*=\s*result\.question", hpk) or \
                "applyUnownDVs" not in hpk:
            r.violations.append("src/UI/Gen2UnifiedPokemonWorkspace.inc: a Letter pick is applied without its question")
    ap = body_of(uw, "applyUnownDVs")
    if not ap or not re.search(r"shiny", ap) or "dvChanges" not in ap:
        r.violations.append("src/UI/Gen2UnifiedPokemonWorkspace.inc: an applied letter does not state shiny and the "
                            "changed DVs")
    hy, _ = block(uw, r"if\s*\(\s*down\s*&\s*HidNpadButton_Y\s*\)\s*\{(?=\s*if\s*\(\s*randomizeDVs)")
    if hy is None:
        missing(r, "src/UI/Gen2UnifiedPokemonWorkspace.inc", "Y reroll branch")
    elif len(re.findall(r"rerollStatus\s*\(", hy)) < 2 and not re.search(r"describeDVChanges|dvChanges", hy):
        r.violations.append("src/UI/Gen2UnifiedPokemonWorkspace.inc: Y reroll keeps the letter and sprite but does "
                            "not say so or name the changed DVs (success and no-change)")
    # The legacy Foundation input path (Gen2PokemonEditor::handleInput) is not dispatched by the composite
    # overlay; its Y branch counts only if something calls it again.
    legacy_called = any(re.search(r"Gen2PokemonEditor::handleInput\s*\(", f.code) for f in sources
                        if f.rel != "src/UI/Gen2PokemonEditorFoundation.inc") or \
        any(re.search(r"(?<![:\w])handleInput\s*\(\s*screen\b", f.code) for f in sources
            if re.match(r"src/UI/Gen2(?!PokemonEditorFoundation)\w*\.inc$", f.rel))
    fd = src("src/UI/Gen2PokemonEditorFoundation.inc")
    if legacy_called and fd and re.search(r"HidNpadButton_Y\s*\)\s*\{\s*randomizeDVs\s*\(\s*state\s*\)\s*;", fd.code):
        r.violations.append("src/UI/Gen2PokemonEditorFoundation.inc: Y reroll ignores randomizeDVs' result and "
                            "always reports success (and that input path is reachable)")
    res.append(r)

    # ------------------------------------------------------------------------------------------
    # B04 E05 (lane R): Inventory Help is titled "Inventory Controls" in every generation, has no
    # "X Add remains available", and describes + as Review where + opens Review.
    # ------------------------------------------------------------------------------------------
    r = Result("CG86-inventory-help-controls",
               "Inventory Help: titled \"Inventory Controls\", no \"X Add remains available\", + described as "
               "Review (owner Oct 26 B04)")
    # Round 7 (lane R f5ee4266): the Gen I-V Bag shares ONE Help, include/UI/InventoryHelp.h.
    ih = src("include/UI/InventoryHelp.h")
    ov = src("src/UI/ClassicInventoryOverlay.cpp")
    um = src("src/UI/ClassicInventoryUIModel.cpp")
    if not ih or not ov:
        missing(r, "include/UI/InventoryHelp.h / src/UI/ClassicInventoryOverlay.cpp", "Inventory Help")
    else:
        if not re.search(r'kTitle\s*=\s*"Inventory Controls"', ih.code):
            r.violations.append("include/UI/InventoryHelp.h: the Bag Help is not titled \"Inventory Controls\"")
        for f in (ih, um):
            if f and re.search(r"Add remains available", f.code):
                r.violations.append(f"{f.rel}: Inventory Help still says \"X Add remains available\"")
        plus_review = re.search(r"ClassicInventoryAction::Options\s*\)\s*\{[^}]*openStagedReview", ov.code)
        for _, line in ih.literals:
            if line.startswith("+") and plus_review and "Review" not in line.split("     ")[0]:
                r.violations.append(f"include/UI/InventoryHelp.h: Help says \"{line.split('  ')[0]} ...\" "
                                    "but + opens Review")
        if re.search(r'"Inventory Help"', ov.code) or not re.search(r"InvHelp::kTitle", ov.code):
            r.violations.append("src/UI/ClassicInventoryOverlay.cpp: Gen I-III Bag Help is not titled from "
                                "InventoryHelp::kTitle (\"Inventory Controls\")")
    if tvb:
        # The title is the drawInfoOverlay argument that the Items page Help rows are drawn with, and no
        # page-Help overlay is drawn under a fixed literal title.
        if not re.search(r"drawInfoOverlay\s*\(\s*fb\s*,\s*page\s*==\s*Help::Page::Items\s*\?\s*"
                         r"(?:PokeBank::UIModel::)?InventoryHelp::kTitle\s*:[^,]*,\s*Help::lines\s*\(\s*page", tvb.code):
            r.violations.append("src/UI/TrainerViewScreenBase.inc: the Gen IV / V Bag Help is not titled "
                                "\"Inventory Controls\" (InventoryHelp::kTitle)")
        for m in re.finditer(r'drawInfoOverlay\s*\(\s*fb\s*,\s*"([^"]+)"\s*,\s*Help::lines\s*\(\s*page', tvb.code):
            r.violations.append(f"src/UI/TrainerViewScreenBase.inc:{tvb.code.count(chr(10), 0, m.start()) + 1}: "
                                f"page Help (Bag included) drawn under the fixed title \"{m.group(1)}\"")
    # One shared Help: the Gen IV / V Items page and the Gen I-III overlay both draw InventoryHelp's rows.
    chp = src("include/UI/ClassicHelpPresentation.h")
    items = re.search(r"case\s+Page::Items\s*:(.*?)(?=case\s+Page::|default\s*:|\Z)", chp.code if chp else "", re.S)
    if not items or not re.search(r"return\s+(?:PokeBank::UIModel::)?InventoryHelp::lines\s*\(\s*generation\s*\)", items.group(1)):
        r.violations.append("include/UI/ClassicHelpPresentation.h: the Gen IV / V Items Help does not use the shared "
                            "InventoryHelp rows")
    if ov and not re.search(r"for\s*\(\s*const\s+auto\s*&\s*\w+\s*:\s*InvHelp::lines\s*\(\s*generation\s*\)\s*\)", ov.code):
        r.violations.append("src/UI/ClassicInventoryOverlay.cpp: the Gen I-III Bag Help does not draw the shared "
                            "InventoryHelp rows")
    res.append(r)

    # ------------------------------------------------------------------------------------------
    # B05 R2-CHECK-011 (lane P): event / special Met Location rows describe that choice; no single
    # generic "...; unverified" text returned for a whole range of location ids.
    # ------------------------------------------------------------------------------------------
    r = Result("CG87-event-locations-described",
               "Event / Met Location rows say what each choice is; no generic \"unverified\" text for a whole "
               "id range (owner Oct 26 B05)")
    ec = src("include/Integration/Encounter/EncounterContext45.h")
    det = body_of(ec, "detail")
    if not det:
        missing(r, "include/Integration/Encounter/EncounterContext45.h", "detail")
    else:
        for m in re.finditer(r"return\s+(?:tagged\s*\(\s*)?\"([^\"]*)\"", det):
            cond = det[det.rfind("if", 0, m.start()):m.start()]
            span = re.search(r"location\s*>=?\s*(\d+)\s*&&\s*location\s*<=?\s*(\d+)", cond)
            if span and int(span.group(2)) - int(span.group(1)) > 8:   # a handful of transfer ids may share one
                r.violations.append(f"include/Integration/Encounter/EncounterContext45.h: \"{m.group(1)}\" is the one "
                                    "description of every id in a range")
    res.append(r)

    # ------------------------------------------------------------------------------------------
    # C04 follow-up 7D (lane P): Quick Games closes on a completed tap outside the panel, and that tap
    # is consumed (no tap-through to the game card underneath).
    # ------------------------------------------------------------------------------------------
    r = Result("CG88-quick-games-tap-outside",
               "Quick Games closes on a tap outside it, with no tap-through (owner Oct 26 C04 follow-up)")
    ss = src("src/UI/SaveSelectScreen.cpp")
    hb, ln = None, 0
    for m in re.finditer(r"if\s*\(\s*overlay\s*==\s*Overlay::GamesDrawer\s*\)\s*\{", ss.code if ss else ""):
        b = block_after(ss.code, m.end())
        if "closeAnimated" in b and "HidNpadButton_B" in b:
            hb, ln = b, ss.code.count("\n", 0, m.start()) + 1
            break
    db, dln = block(ss, r"\}\s*else\s+if\s*\(\s*overlay\s*==\s*Overlay::GamesDrawer\s*\)\s*\{")
    if hb is None or db is None:
        missing(r, "src/UI/SaveSelectScreen.cpp", "Quick Games draw / input block")
    else:
        if "SharedPopupDismiss::publish" not in db:
            r.violations.append(f"src/UI/SaveSelectScreen.cpp:{dln}: Quick Games does not publish its panel frame")
        # Either the Quick Games input block checks the outside tap itself, or (lane P e52b7d97) one early
        # dispatcher in SaveSelectScreen::update does, ahead of every Quick Games touch path: its
        # GamesDrawer case must close the drawer and return so the tap never reaches the Home card.
        m = re.search(r"SharedPopupDismiss::tappedOutside\s*\(", hb)
        handled = bool(m and re.search(r"closeAnimated|overlay\s*=\s*Overlay::None", hb[m.end():m.end() + 300])
                       and re.search(r"\breturn\b", hb[m.end():m.end() + 400]))
        upd = body_of(ss, "update")
        for d in re.finditer(r"if\s*\(\s*SharedPopupDismiss::tappedOutside\s*\(\s*touch\s*\)\s*\)\s*\{\s*switch\s*\(\s*overlay\s*\)\s*\{",
                             upd):
            sw = block_after(upd, d.end())
            case = re.search(r"case\s+Overlay::GamesDrawer\s*:(.*?)(?=case\s+Overlay::|default\s*:|\Z)", sw, re.S)
            # Quick Games touch paths: the drag (arm / track / release), the tile hits (case GamesDrawer
            # outside this switch) and the Quick Games input block. The per-frame "closed" bookkeeping
            # at the top of update() reads no touch.
            sw_span = range(d.start(), d.end() + len(sw))
            touch_paths = [x.start() for x in re.finditer(
                r"quickDrawerMotion\s*\.\s*(?:arm|track|release)\s*\(|case\s+Overlay::GamesDrawer\s*:", upd)
                if x.start() not in sw_span]
            ib = re.search(r"if\s*\(\s*overlay\s*==\s*Overlay::GamesDrawer\s*\)\s*\{(?=(?:(?!\n\s*\}).)*?"
                           r"(?:closeAnimated|HidNpadButton_B))", upd, re.S)
            touch_paths += [ib.start()] if ib else []
            first_drawer_touch = min(touch_paths, default=len(upd))
            if case and re.search(r"quickDrawerMotion\s*\.\s*closeAnimated\s*\(|overlay\s*=\s*Overlay::None", case.group(1)) \
                    and re.search(r"\breturn\s*;", case.group(1)) and d.start() < first_drawer_touch:
                handled = True
        if not handled:
            r.violations.append(f"src/UI/SaveSelectScreen.cpp:{ln}: Quick Games does not close on a tap outside it "
                                "(only B or a drag closes it)")
    res.append(r)

    # ------------------------------------------------------------------------------------------
    # Typography (lane S): DETAILS / STATS / MOVES panel headings use one text style token in every
    # Gen I-V editor; the reference is the shared Gen IV / V editor's small coloured label.
    # ------------------------------------------------------------------------------------------
    r = Result("CG89-one-editor-heading-style",
               "DETAILS / STATS / MOVES headings all draw through the one shared heading helper "
               "(UI/SharedPanelHeading.h, the Gen IV / V style) in Gen I-V (owner Oct 26 typography)")
    HELPER = "include/UI/SharedPanelHeading.h"
    hp = src(HELPER)
    hd = body_of(hp, "draw")
    if not hp or not hd:
        missing(r, HELPER, "PanelHeading::draw")
    else:
        if not re.search(r"kStyle\s*=\s*UI::TextStyle::Caption\s*;", hp.code):
            r.violations.append(f"{HELPER}: the shared heading style is not the Gen IV / V Caption")
        if not re.search(r"return\s+UI::Colors::Accent\s*;", body_of(hp, "color")):
            r.violations.append(f"{HELPER}: the shared heading colour is not the Gen IV / V Accent")
        if not re.search(r"drawText\s*\([^;]*kInsetX[^;]*kInsetY[^;]*\btitle\b[^;]*color\s*\(\s*\)\s*,\s*kStyle\s*\)", hd):
            r.violations.append(f"{HELPER}: draw does not use the helper's inset, colour and style")
    HEADING = r'"(?:DETAILS|STATS|MOVES)"|\bstatsHeading\s*\(\s*\)'
    VIA_HELPER = re.compile(r"PanelHeading::draw\s*\(")

    def forwarder_body(f, name):
        """Body of the function or lambda `name` that a heading literal is passed to."""
        m = re.search(r"\b" + re.escape(name) + r"\s*=\s*\[[^\]]*\]\s*\([^)]*\)\s*\{", f.code)
        return block_after(f.code, m.end()) if m else body_of(f, name)

    if not re.search(r"PanelHeading::draw\s*\([^;]*\btitle\b", body_of(src("src/UI/SharedPokemonEditorSurface.inc"), "drawPanel")):
        r.violations.append("src/UI/SharedPokemonEditorSurface.inc: the Gen IV / V reference drawPanel does not use "
                            "the shared heading helper")
    for f in sources:
        if not EDITOR_FILE_RX.search(f.rel):
            continue
        for m in re.finditer(HEADING, f.code):
            stmt = f.code[max(f.code.rfind(";", 0, m.start()), f.code.rfind("{", 0, m.start()),
                              f.code.rfind("}", 0, m.start())) + 1:f.code.find(";", m.end()) + 1]
            ln = f.code.count("\n", 0, m.start()) + 1
            if VIA_HELPER.search(stmt):
                continue
            if re.search(r"\bdrawText\s*\(", stmt):
                r.violations.append(f"{f.rel}:{ln}: panel heading drawn with drawText, not the shared heading helper")
                continue
            call = re.match(r"\s*(?:\w+(?:::|\.))*(\w+)\s*\(", stmt)
            if not call or call.group(1) in ("return", "if", "static_assert", "assert"):
                continue   # not a draw (a value, a comparison, a static check)
            fb = forwarder_body(f, call.group(1))
            if not fb:
                r.violations.append(f"{f.rel}:{ln}: heading passed to {call.group(1)}, whose body was not found "
                                    "(update this check)")
            elif not VIA_HELPER.search(fb) or re.search(r"\bdrawText\s*\([^;]*\b(?:title|text)\b", fb):
                r.violations.append(f"{f.rel}:{ln}: heading passed to {call.group(1)}, which does not draw it "
                                    "through the shared heading helper")
    res.append(r)

    # ------------------------------------------------------------------------------------------
    # B06 / C06 NEW-18, Gen V (lane C; coordinator round 7 scope): Gen I-V Trainer share one
    # presentation and Review overlaps nothing. Gen V's page either renders through the
    # ClassicTrainerPage model (its measured Review slot, CG83's probe) or its own fixed-pitch rows,
    # divider and "Game language" line all end at or above the Review button's top.
    # ------------------------------------------------------------------------------------------
    r = Result("CG90-gen5-trainer-review-slot",
               "Gen V Trainer: every row and the Game language line end at or above Review, or the page uses "
               "the ClassicTrainerPage measured slot (owner Oct 26 B06 / C06)")
    te = body_of(g5, "drawTrainerEditor")
    if not te:
        missing(r, "src/UI/Gen5SharedReadOnlySurface.inc", "drawTrainerEditor")
    elif re.search(r"ClassicTrainerPage::layout\s*\(|\bPage::layout\s*\(", te):
        if not re.search(r"drawGlyphButton\s*\(\s*fb\s*,\s*\w+\.review\.x\s*,\s*\w+\.review\.y", te):
            r.violations.append("src/UI/Gen5SharedReadOnlySurface.inc: Gen V Trainer uses the page model but does not "
                                "draw Review at its review rect")
        lang = re.search(r"drawText\s*\([^;]*\"Game language", te)
        if lang and not re.search(r"\w+\.(?:rows|review|language)\b", te[max(0, lang.start() - 200):lang.end()]):
            r.violations.append("src/UI/Gen5SharedReadOnlySurface.inc: the Game language line is not placed by the "
                                "page model")
    else:
        r.violations.extend(_gen5_trainer_slot(te, tvb, sources))
    res.append(r)
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
