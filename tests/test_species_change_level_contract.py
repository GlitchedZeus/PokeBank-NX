#!/usr/bin/env python3
"""Round 13 owner decision 9: species change keeps an EXISTING Pokemon's level (Gen I-V).

"Existing Pokemon retain their current level on species change. Reconcile EXP correctly."
Source-level lock for the paths the host tests cannot reach (the Switch UI .inc surfaces):
every generation's species-change apply goes through the one shared rule
(include/Pokemon/SpeciesChangeExperience.h keepLevel), a Create draft keeps the default-level
policy, and every surface names an EXP change in its status. The behaviour itself is covered by
tests/test_species_change_level_gen1..5.cpp.
"""
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def require(text: str, token: str, where: str) -> None:
    assert token in text, f"{where}: missing {token!r}"


def forbid(text: str, token: str, where: str) -> None:
    assert token not in text, f"{where}: must not contain {token!r}"


def between(text: str, start: str, end: str) -> str:
    a = text.index(start)
    return text[a:text.index(end, a)]


# The shared rule.
rule = read("include/Pokemon/SpeciesChangeExperience.h")
require(rule, "constexpr Result keepLevel(", "SpeciesChangeExperience.h")
require(rule, "Round 13 owner decision 9", "SpeciesChangeExperience.h")
require(read("include/UI/SpeciesChangeLevelPolicy.h"), '#include "Pokemon/SpeciesChangeExperience.h"',
        "SpeciesChangeLevelPolicy.h")

# Every generation's backend species change uses it.
for path in ("src/Integration/Gen1/Gen1StagedPokemonEditor.cpp",
             "src/Integration/Gen2/Gen2StagedEditor.cpp",
             "src/Integration/Gen3/Gen3StagedPokemonEditor.cpp",
             "src/Pokemon/Pokemon4Mutable.cpp",
             "include/Integration/Gen5/Gen5StagedPokemonRecord.h"):
    require(read(path), "SpeciesChangeExperience::keepLevel(", path)

# Gen I: the Edit branch of ux3ApplySpeciesPicker stages no level (the editor keeps it); the
# AddDraft branch keeps the Create default-level policy.
gen1 = read("src/UI/Gen1PokemonEditorOverlayUXCleanup3.inc")
apply = between(gen1, "bool ux3ApplySpeciesPicker(", "PokeBank::UIModel::SharedPickerPaging::Keys ux3PickerKeys")
add_branch = between(apply, "if (state.mode == UX2Mode::AddDraft)", "if (state.mode == UX2Mode::Edit)")
edit_branch = apply[apply.index("if (state.mode == UX2Mode::Edit)"):]
require(add_branch, "SpeciesChangeLevelPolicy::defaultLevel(", "Gen I AddDraft")
forbid(edit_branch, "defaultLevel(", "Gen I Edit species change")
assert not re.search(r"\bedit\.level\s*=", edit_branch), "Gen I Edit species change must not set edit.level"
require(edit_branch, "namespace Exp = ::Pokemon::SpeciesChangeExperience;", "Gen I Edit status")
require(edit_branch, "Exp::note(", "Gen I Edit status")

# Gen II / III sessions: Create takes the default level, Edit keeps the level.
for path in ("include/UI/Gen2PokemonSession.h", "include/UI/Gen3SharedPokemonSession.h"):
    text = read(path)
    body = between(text, "bool setSpecies(", "// Round 11 (owner H28)")
    require(body, "Create) {", path)
    require(body, "SpeciesChangeLevelPolicy::defaultLevel(", path)
    require(body, "SpeciesChangeExperience::keepLevel(", path)
    require(body, "progression = ProgressionSource::Experience;", path)
    require(text, "speciesChangeExperienceNote()", path)
require(read("include/UI/Gen4SharedPokemonSession.h"), "speciesChangeExperienceNote()", "Gen4 session")

# Every species-apply surface names an EXP change.
for path in ("src/UI/Gen2PokemonEditorFoundation.inc", "src/UI/Gen2PokemonPickerOverlay.inc",
             "src/UI/Gen3SharedPokemonSurface.inc", "src/UI/Gen4SharedPokemonSurface.inc"):
    require(read(path), "speciesChangeExperienceNote()", path)
require(read("src/UI/Gen5SharedReadOnlySurface.inc"), "SpeciesChangeExperience::note(", "Gen V surface")

# Gen V: a different EXP growth group is no longer refused.
for path in ("include/Integration/Gen5/Gen5SharedScreenState.h", "include/Integration/Gen5/Gen5StagedPokemonRecord.h"):
    text = read(path)
    forbid(text, "different EXP growth group", path)
    forbid(text, "must share the source EXP growth group", path)

print("test_species_change_level_contract: OK")
