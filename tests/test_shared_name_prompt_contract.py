#!/usr/bin/env python3
"""Round 15 keyboard lane (SHARED_UX_REQUIREMENTS.md P1-2 / P1-5): ONE name prompt.

Owner: "audit EVERY text-entry surface across Gen I-IX. Implement shared Japanese/Korean/Latin
input with native encoding validation. No ASCII-only keyboard when the underlying game supports
more." Every Gen I-V save-native name (nickname, OT, trainer name) opens Utils::promptName with the
field's capability (Text/NameFields.h makeNameField), so the pages, disabled keys, cap and live
check come from the same rule the writer applies. This contract fails when:
  * a name prompt goes back to a direct Utils::promptText / promptJapaneseName / promptKoreanName;
  * a Gen I-V surface stops passing the record's (or save's) language into its field;
  * the keyboard stops greying keys from the field, re-checking live, or checking the System
    keyboard's answer.
The remaining direct promptText calls are listed with the reason they are not Gen I-V save names.
"""
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]

# Direct promptText calls that are NOT Gen I-V save-native names (recorded follow-ups, not exemptions
# from the owner's rule): legacy PKSE Switch-title surfaces (Gen VI+ / Switch FRLG backups, out of the
# Gen I-V scope of this branch) and app-owned text (bank box names, backup folder names).
ALLOWED = {
    ("src/UI/TrainerViewScreenBase.inc", '"Rename Box"'),        # legacy Switch-title save box (§3.8 L3)
    ("src/UI/TrainerViewScreenBase.inc", '"Rename Bank Box"'),   # app-owned UTF-8 (§3.7 A5)
    ("src/UI/TrainerViewScreenBase.inc", '"Trainer Name"'),      # legacy Switch-title trainer (§3.8 L2)
    ("src/UI/TrainerViewScreenBase.inc", '"New Backup"'),        # backup folder name (§3.7 A6)
    ("src/UI/TrainerViewScreenBase.inc", '"Nickname"'),          # legacy Switch-title nickname (§3.8 L1)
}

failures = []


def need(cond, what):
    if not cond:
        failures.append(what)


def sources():
    for base in ("src", "include"):
        for path in (ROOT / base).rglob("*"):
            if path.suffix in (".cpp", ".h", ".inc") and path.is_file():
                yield path


for path in sources():
    rel = path.relative_to(ROOT).as_posix()
    if rel in ("src/Utils/Keyboard.cpp", "include/Utils/Keyboard.h"):
        continue
    text = path.read_text(encoding="utf-8", errors="replace")
    for legacy in ("promptJapaneseName(", "promptKoreanName("):
        for m in re.finditer(r"Utils::" + re.escape(legacy), text):
            failures.append(f"{rel}: {legacy} -- use Utils::promptName with the field's capability")
    for m in re.finditer(r"Utils::promptText\(\s*(\"[^\"]*\")", text):
        if (rel, m.group(1)) not in ALLOWED:
            failures.append(f"{rel}: direct Utils::promptText({m.group(1)}...) -- name prompts use Utils::promptName")

surfaces = {
    "src/UI/Gen3SharedPokemonSurface.inc": ["gen3NameField(screen, p, PokeBank::Text::FieldKind::Nickname)",
                                            "gen3NameField(screen, p, PokeBank::Text::FieldKind::OriginalTrainer)",
                                            "language = pokemon.otTextLanguage();",
                                            "Utils::promptName(*field, \"Generation III Nickname\""],
    "src/UI/Gen4SharedPokemonSurface.inc": ["PokeBank::Text::Format::Gen4, kind, mon.language(), false",
                                            "Utils::promptName(*field, \"Nickname\"",
                                            "Utils::promptName(*field, \"Original Trainer\""],
    "src/UI/Gen5SharedReadOnlySurface.inc": ["p.language(), false});",
                                             "Utils::promptName(*field, nickname ? \"Nickname\"",
                                             "PokeBank::Text::FieldKind::TrainerName,current->language,false});"],
    "include/UI/ClassicTrainerSessionInput.h": ["Utils::promptName(*fields.nameField, \"Trainer Name\", name)"],
    "include/UI/ClassicJapaneseNames.h": ["return Utils::promptName(*capability, header, initial);"],
}
for rel, tokens in surfaces.items():
    text = (ROOT / rel).read_text(encoding="utf-8")
    for token in tokens:
        need(token in text, f"{rel}: missing {token!r}")

dialog = (ROOT / "src/UI/Dialogs/KeyboardDialog.cpp").read_text(encoding="utf-8")
for token in ("profile = request.field->keyboard();",
              "key.lowerEnabled = request.field->glyphsAllowed(key.lowerLabel);",
              "refuseGlyph(shift != ShiftMode::Off ? key.upperLabel : key.lowerLabel);",
              "refreshVerdict();",
              "const T::TextVerdict verdict = request.field->check(text);",
              "if (!acceptable(typed.text, why))"):
    need(token in dialog, f"KeyboardDialog.cpp: missing {token!r}")

gen5 = (ROOT / "include/Integration/Gen5/Gen5StagedPokemonRecord.h").read_text(encoding="utf-8")
need("encodeName5(decoded,nickname,name,Pokemon5ReadOnly(staged_).language())" in gen5,
     "Gen V names: the writer is no longer the shared Gen V rule")

if failures:
    print("Shared name prompt contract: FAIL")
    for f in failures:
        print("  " + f)
    sys.exit(1)
print("Every Gen I-V save-native name prompt is Utils::promptName with its field: PASS")
