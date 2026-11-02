#!/usr/bin/env python3
"""Negative controls for the enforced round 5 and round 7 cross-gen contracts (tests/test_qa_cross_gen_contract.py).

The unmutated tree must pass each enforced contract (CG43, CG64, CG76), and each deliberately broken
source path must be reported by its contract. CG61-CG63 stay XFAIL until their lane lands (the
pre-fix code is their own negative control); here they only need to exist.
"""
from __future__ import annotations

import sys
from dataclasses import replace
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import test_qa_cross_gen_contract as qa  # noqa: E402

ROOT = Path(__file__).resolve().parent.parent
SOURCES, _ = qa.load_sources(ROOT)


def result(sources, cid):
    return next(r for r in qa.contracts(sources) if r.cid == cid)


def mutated(rel, old, new):
    out, found = [], False
    for s in SOURCES:
        if s.rel == rel:
            assert old in s.code, (rel, old)
            out.append(replace(s, code=s.code.replace(old, new, 1)))
            found = True
        else:
            out.append(s)
    assert found, rel
    return out


# (description, contract, file, old, new)
MUTATIONS = [
    ("Nature colours swapped", "CG43-nature-stat-colours", "include/UI/NatureStatPresentation.h",
     "modifier > 0 ? Colors::Success : modifier < 0 ? Colors::Error", "modifier > 0 ? Colors::Error : modifier < 0 ? Colors::Success"),
    ("Gen III stat rows ignore statColor", "CG43-nature-stat-colours", "src/UI/Gen3SharedPokemonSurface.inc",
     "NatureStats::statColor(p.nature, row, selected ? Colors::SelectedText : Colors::TextDim)",
     "(selected ? Colors::SelectedText : Colors::TextDim)"),
    ("theme success colour no longer green", "CG43-nature-stat-colours", "include/UI/Common.h",
     "Color(82, 205, 137)", "Color(205, 82, 137)"),
    ("Gen III gender offered for any stored byte", "CG64-trainer-playtime-gender-staged",
     "include/Integration/Gen3/Gen3StagedTrainerEditor.h",
     "return originalGender_ <= 1;", "return true;"),
    ("Gen II play time skips Review", "CG64-trainer-playtime-gender-staged", "src/Integration/Gen2/Gen2StagedEditor.cpp",
     'setChange("play_time"', 'noteChange("play_time"'),
    ("Gen II gender no longer Crystal-only", "CG64-trainer-playtime-gender-staged", "src/Integration/Gen2/Gen2StagedEditor.cpp",
     "return metadata_.family == VersionFamily::Crystal && original_.size() > kCrystalGenderOffset &&",
     "return original_.size() > kCrystalGenderOffset &&"),
    ("Gen IV gender stager set without the native-byte check", "CG64-trainer-playtime-gender-staged",
     "src/UI/Gen4SharedPokemonSurface.inc",
     "if (trainer.genderEditable())\n        fields.stageGender", "if (true)\n        fields.stageGender"),
    ("Gen I offers a gender edit", "CG64-trainer-playtime-gender-staged", "src/UI/Gen1PokemonEditorOverlayFoundation.inc",
     "fields.nameEditable = editor->nameEditable();",
     "fields.nameEditable = editor->nameEditable();\n    fields.stageGender = nullptr;"),
    ("Gen II Unown letter not recomputed after a DV change", "CG76-gen2-unown-letter-from-dvs",
     "src/UI/Gen2PokemonEditorFoundation.inc",
     "    p.form = p.species == 201 ? static_cast<uint8_t>(PokeBank::UIModel::Gen12DVReroll::unownLetter(dvs)) : 0;\n",
     ""),
    ("Gen IV created slot falls into compare()'s early return (no Create detail)", "CG74-create-review-details",
     "src/Integration/Gen4/Gen4ExportTransaction.cpp",
     "if(a.valid() && a.empty() && b.valid() && !b.empty() && !inOriginal(b))return created(b,location);\n", ""),
    ("Gen III Create Review row stops at the species / level", "CG74-create-review-details",
     "src/Integration/Gen3/Gen3StagedPokemonEditor.cpp",
     "C::header(Names::getSpeciesName(after->species), after->level)});\n",
     "C::header(Names::getSpeciesName(after->species), after->level)});\n    return;\n"),
    ("Gen IV play time written straight to a file", "CG64-trainer-playtime-gender-staged",
     "src/Integration/Gen4/Gen4StagedPokemonEditor.cpp",
     "return stageTrainerBytes(kTrainerPlayTimeField, field,",
     "std::fopen(\"x\", \"wb\");\n    return stageTrainerBytes(kTrainerPlayTimeField, field,"),
    # Round 7 (enforced after lane P e52b7d97).
    ("one generic text returned for the whole Gen V event range", "CG87-event-locations-described",
     "include/Integration/Encounter/EncounterContext45.h",
     "const auto tagged = [&](std::string text) { return current ? \"Stored: \" + text : text; };",
     "const auto tagged = [&](std::string text) { return current ? \"Stored: \" + text : text; };\n"
     "        if (gen == 5 && location > 40000 && location < 50000) return tagged(\"Event / special location; unverified\");"),
    ("Quick Games outside tap returns without closing", "CG88-quick-games-tap-outside", "src/UI/SaveSelectScreen.cpp",
     "                    if (!quickDrawerMotion.interactive()) break;\n                    quickDrawerMotion.closeAnimated();\n",
     "                    if (!quickDrawerMotion.interactive()) break;\n"),
    ("Quick Games outside tap falls through to the Home card", "CG88-quick-games-tap-outside", "src/UI/SaveSelectScreen.cpp",
     "                    gamesDrawerTouchScroll.stop();\n                    return;\n                case Overlay::ProfilePicker:",
     "                    gamesDrawerTouchScroll.stop();\n                    break;\n                case Overlay::ProfilePicker:"),
    ("Quick Games outside tap checked after the drag", "CG88-quick-games-tap-outside", "src/UI/SaveSelectScreen.cpp",
     "        if (SharedPopupDismiss::tappedOutside(touch)) {\n            switch (overlay) {",
     "        quickDrawerMotion.arm(true, 0, 0, 1280, 664);\n        if (SharedPopupDismiss::tappedOutside(touch)) {\n            switch (overlay) {"),
    ("Shiny on changes the Unown letter without asking", "CG85-unown-letter-shiny-confirmed", "include/UI/Gen2UnownLetter.h",
     "    if (!t.letterChanged) return {false, t.dvs, {}};\n", "    return {false, t.dvs, {}};\n"),
    ("a letter that cannot be shiny is refused again", "CG85-unown-letter-shiny-confirmed", "include/UI/Gen2UnownLetter.h",
     "return {PickAction::Ask, current, {Ask::DropShinyForLetter, letter, plain.dvs}};", "return {PickAction::Close, current, {}};"),
    ("B on the Unown question is silent", "CG85-unown-letter-shiny-confirmed", "src/UI/Gen2UnifiedPokemonWorkspace.inc",
     "        screen.postStatus(\"Unown unchanged\", 200);\n", ""),
    ("Unown reroll no longer names the changed DVs", "CG85-unown-letter-shiny-confirmed", "include/UI/Gen2UnownLetter.h",
     "\" kept: \" + dvChanges(before, after)", "\" kept\""),
    ("Gen II Create Review drawn under the workspace again", "CG80-gen2-create-review-visible",
     "src/UI/Gen2HardwareFinalFix.inc",
     "    if (gen2WorkspaceReviewOpen(screen)) {\n        drawSharedReviewSurface(screen, fb);\n        return true;\n    }\n", ""),
    ("Gen II Review predicate stops testing review", "CG80-gen2-create-review-visible", "src/UI/Gen2HardwareWorkspaceFix.inc",
     "editorOverlayState(screen).active &&\n           editorOverlayState(screen).review;", "editorOverlayState(screen).active;"),
    ("Save Backups skips Gen V records", "CG82-gen5-exports-in-save-backups", "include/UI/SourceBackupClassification.h",
     "const std::string account = detail::gen5AccountDir(exportsRoot, out.profileIdentity);", "const std::string account;"),
    ("canDelete accepts another profile's Gen V folder", "CG82-gen5-exports-in-save-backups",
     "include/UI/SourceBackupClassification.h",
     "const std::string account = detail::gen5AccountDir(listing.exportsRoot, listing.profileIdentity);",
     "const std::string account = listing.exportsRoot + \"/gen5\";"),
    ("Gen V backup open accepts another profile's folder", "CG82-gen5-exports-in-save-backups", "include/UI/SourceBackupOpen.h",
     "const auto account=detail::gen5AccountDir(root,listing.profileIdentity);", "const auto account=root+\"/gen5\";"),
    ("canDelete drops the export record's source check", "CG82-gen5-exports-in-save-backups",
     "include/UI/SourceBackupClassification.h",
     "    if (!m.ok || m.sourcePath != listing.sourcePath) return Refusal::NotAnExportRecord;\n    for (const std::string& name : detail::children(dir))\n        if (name != manifestName",
     "    if (!m.ok) return Refusal::NotAnExportRecord;\n    for (const std::string& name : detail::children(dir))\n        if (name != manifestName"),
    ("Gen II Boxes occupied-slot footer drops + Review", "CG81-one-review-key-per-screen",
     "src/UI/TrainerViewScreenCompositeOverlay.cpp",
     '{{"A", "Actions"}, {"L/R", "Box"}, {"+", "Review"}, {"B", "Back"}}', '{{"A", "Actions"}, {"L/R", "Box"}, {"B", "Back"}}'),
    ("Gen I-IV Trainer takes + as well as Y again", "CG81-one-review-key-per-screen",
     "include/UI/ReviewEntry.h", "&& !yOpensReview(screen, generation)", ""),
    ("Quick Games stops publishing its frame", "CG88-quick-games-tap-outside", "src/UI/SaveSelectScreen.cpp",
     "SharedPopupDismiss::publish(x + offset, 0, w, h);", "(void)offset;"),
]


def main() -> int:
    failures = 0
    base = {r.cid: r for r in qa.contracts(SOURCES)}
    for cid in ("CG43-nature-stat-colours", "CG64-trainer-playtime-gender-staged", "CG76-gen2-unown-letter-from-dvs",
                "CG74-create-review-details", "CG87-event-locations-described", "CG88-quick-games-tap-outside",
                "CG85-unown-letter-shiny-confirmed", "CG80-gen2-create-review-visible", "CG82-gen5-exports-in-save-backups",
                "CG81-one-review-key-per-screen"):
        if base[cid].violations:
            print(f"FAIL  unmutated tree: {cid}: {base[cid].violations[:2]}")
            failures += 1
    for cid in ("CG61-switch-backup-opens-staged-copy", "CG62-met-location-rows-fit", "CG63-help-names-real-controls",
                "CG68-gen1-review-swipe-one-page"):
        if cid not in base:
            print(f"FAIL  {cid} is missing")
            failures += 1
    for desc, cid, rel, old, new in MUTATIONS:
        r = result(mutated(rel, old, new), cid)
        if r.violations:
            print(f"PASS  {desc}: {cid} fails ({r.violations[0][:100]})")
        else:
            print(f"FAIL  {desc}: {cid} did not report it")
            failures += 1
    print(f"{'FAIL' if failures else 'PASS'}: round 5 contract mutations ({failures} failure(s))")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
