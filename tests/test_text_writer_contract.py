#!/usr/bin/env python3
"""Round 14 text safety contract (SHARED_UX_REQUIREMENTS.md P0-1 / P0-2 / P0-3 / P0-5 / P1).

Source guards that keep the all-or-nothing name writers from regressing:
  * no save-data writer calls the truncating Utils::setString (only setStringChecked);
  * the Gen III / Gen IV encoders go through the shared TextFieldCapability and never break,
    cut or substitute '?' (0x01AC);
  * the Gen III staged OT edit validates in otTextLanguage() and reads the result back;
  * Gen IV box names are reported unsupported (no writer exists);
  * the System keyboard's answer is validated before it is accepted;
  * the backup-name prompt refuses instead of filtering.
"""
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent


def read(rel: str) -> str:
    return (ROOT / rel).read_text(encoding="utf-8")


def body(text: str, start: str, end: str) -> str:
    i = text.index(start)
    return text[i:text.index(end, i)]


def main() -> None:
    # P0-3: no direct setString outside its own definition.
    offenders = []
    for path in list((ROOT / "src").rglob("*.cpp")) + list((ROOT / "src").rglob("*.inc")) + \
            list((ROOT / "include").rglob("*.h")):
        rel = path.relative_to(ROOT).as_posix()
        if rel in ("src/Utils/StringHelpers.cpp", "include/Utils/StringHelpers.h"):
            continue
        for n, line in enumerate(path.read_text(encoding="utf-8", errors="replace").splitlines(), 1):
            if re.search(r"(?<![A-Za-z_])setString\(", line):
                offenders.append(f"{rel}:{n}")
    assert not offenders, "truncating Utils::setString used for save data: " + ", ".join(offenders)
    helpers = read("src/Utils/StringHelpers.cpp")
    checked = body(helpers, "CheckedTextResult checkStringFits(", "CheckedTextResult setStringChecked(")
    assert "value.size() > fieldUnits" in checked, "setStringChecked must refuse text longer than the field"

    # P0-2: Gen III / Gen IV encoders.
    pk3 = read("src/Pokemon/Pokemon3FRLG.cpp")
    g3 = body(pk3, "static bool g3EncodeName(", "void Pokemon3FRLG::setOTName(")
    assert "Gen3NameField" in g3 and "break" not in g3
    assert "canStoreOTName" in pk3 and "otTextLanguage()" in body(pk3, "bool Pokemon3FRLG::canStoreOTName(", "bool Pokemon3FRLG::canStoreNickname(")
    tr3 = body(read("src/Trainer/Trainer3FRLG.cpp"), "bool g3Encode(", "// Preserve an existing 100-byte party tail")
    assert "Gen3NameField" in tr3 and "break" not in tr3
    codec = read("include/Utils/Gen4TextCodec.h")
    assert "0x01AC" not in codec, "Gen IV codec must not substitute '?'"
    assert "std::min({value.size()" not in codec, "Gen IV codec must not cut to the cap"
    gen4text = body(read("include/Utils/Gen4Text.h"), "inline bool writeGen4Field(", "#endif")
    assert "return false;" in gen4text and "writableCount" not in gen4text
    conv = body(read("src/Conversion/Convert.cpp"), "bool utf16ToG3Name(", "bool g3NameEqualsIgnoringTrash(")
    assert "Gen3NameField" in conv and "if (gi >= writable) break;" not in conv
    assert "textRefused = true;" in read("src/Conversion/Convert.cpp")

    # P0-1: the staged Gen III OT edit (the shared Edit path for all five games).
    editor = read("src/Integration/Gen3/Gen3StagedPokemonEditor.cpp")
    ot = body(editor, "    if (edit.otName) {\n        // Round 14 P0-1", "    // (A language change was applied above")
    assert "canStoreOTName(value)" in ot and "maxOtLength(pokemon.otTextLanguage())" in ot
    assert "gen3FoldWidth(pokemon.otName()) != Utils::gen3FoldWidth(value)" in ot
    assert "pokemon.setSaveLanguage(trainer_.language);" in editor

    # Gen IV box names: no writer, so not offered.
    assert "bool supportsBoxNames() const noexcept override { return false; }" in read("include/Legacy/Gen4ReadOnlyTrainer.h")

    # P0-5: System keyboard answers pass the same rules.
    dialog = read("src/UI/Dialogs/KeyboardDialog.cpp")
    handoff = body(dialog, "void KeyboardState::handOffToSystemKeyboard()", "void KeyboardState::applyKanaModifier(")
    assert handoff.index("acceptable(typed.text, why)") < handoff.index("text = typed.text;")

    # P1: backup names are refused, never filtered.
    view = read("src/UI/TrainerViewScreenBase.inc")
    assert "PokeBank::UIModel::backupFolderName(res.text)" in view
    named = body(view, "TrainerViewScreen::createNamedBackupDir", "std::vector<int> TrainerViewScreen::visibleItemIndices")
    assert "leaf += c;" not in named

    # Trainer renames ask the trainer-name rule, not the box-name rule.
    assert "trainer.canStoreTrainerName(res.text)" in view
    print("Text writer contract (all-or-nothing writers, no setString, System keyboard, backup names): PASS")


if __name__ == "__main__":
    main()
