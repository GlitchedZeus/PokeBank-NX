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
    # Round 4: owner round 3 FAIL items (ROUND3_RESULTS_62a2f8c3.md); owners per #307 MAIN_ACTIVE_HANDOFF.md §6b.
    # CG01-CG11 XPASSed on candidate 7fea1797 (app-qa-nro-audit run 38947904299) and are ENFORCED;
    # CG12 and CG14 XPASSed on #307 d104310 (batch 1), CG18 on 41cbe92 (batch 2), CG13 and CG15 on
    # d51c1ff (batch 3), CG16 and CG17 on 348129f (batch 4): ENFORCED.
    # Round 3: CG21/CG26/CG28 on #307 aeccd13, CG20/CG24/CG25/CG27 on 7ed1cc16, CG20b on 7ed1cc16,
    # CG18b on 11940524, CG19 on 8a84128e, CG20c/CG22/CG23 on lane P 8d533b52: ENFORCED.
    # Round 4: CG39 (P 05d1d79), CG57 / CG58 (coordinator c59ac178) on #307 3d72654f: ENFORCED.
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
    HELD_SHARED = ("include/UI/SharedHeldItemPickerPresentation.h", "include/UI/Gen2HeldItemPicker.h",
                   "include/UI/SharedHeldItemPicker.h")
    shared_search = any(src(h) and re.search(r"\bSharedSearchInput::(?:drawField|handleKeys|handleTouch)\s*\(", src(h).code)
                        for h in HELD_SHARED)
    for s in U:
        if s.rel in HELD_SHARED or not re.search(r"\bSharedHeldItemPickerPresentation::draw(?:Heading|Grid)\s*\(", s.code):
            continue
        # The integrated Held Item path uses the live promptText callback to
        # ItemPickerSearch, retaining Sort and restoring the old query on B.
        # Require all of that wiring rather than a particular keyboard widget.
        live_prompt = (re.search(r'promptText\s*\(\s*"Search Held Items"[^;]*,\s*refilter\s*\)', s.code) and
                       re.search(r'\bItemPickerSearch::apply\s*\(', s.code) and
                       re.search(r'refilter\s*\(\s*result\.accepted\s*\?\s*result\.text\s*:\s*previous', s.code) and
                       re.search(r'\bItemPickerSearch::tag\s*\(', s.code) and
                       re.search(r'minusLabel\s*=\s*"Search"', s.code))
        if not (shared_search or live_prompt or re.search(r"\bSharedSearchInput::(?:drawField|handleKeys|handleTouch)\s*\(", s.code)):
            r.violations.append(f"{s.rel}: Held Item picker has Sort but no search (no SharedSearchInput field / input)")
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
    for rel, fn in (("src/UI/TrainerViewScreenBase.inc", "drawTrainerView"),):
        bs = bodies_ln(rel, fn)
        if not bs:
            r.violations.append(f"{rel}: {fn}() missing")
        for ln, b in bs:
            lits = [l for l in re.findall(r'"([^"]*)"', b)]
            if not any(re.search(r"(?i)play\s*time", l) for l in lits):
                r.violations.append(f"{rel}:{ln}: {fn}() shows no Play Time row")
            if not (any(re.search(r"(?i)pok[eé]dex|\bseen\b", l) for l in lits) and
                    re.search(r"\bdexProgress\s*\(|\bseen\b", b)):
                r.violations.append(f"{rel}:{ln}: {fn}() shows no Pokedex seen / caught row")
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
