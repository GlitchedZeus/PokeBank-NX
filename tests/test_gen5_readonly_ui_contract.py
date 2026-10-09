#!/usr/bin/env python3
"""Gen V production read-only routing regression lock.

Checks that the Gen V screen uses the existing shared renderer and that its
immutable input shield precedes inherited editor/bank/save paths. Native
compilation and hardware checks remain separate requirements.
"""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def require(text: str, *tokens: str) -> None:
    for token in tokens:
        assert token in text, f"Missing Generation V read-only contract: {token}"


surface = read("src/UI/Gen5SharedReadOnlySurface.inc")
composite = read("src/UI/TrainerViewScreenCompositeOverlay.cpp")
manager = read("src/UI/UI.cpp")
manager_header = read("include/UI/UI.h")
games = read("src/UI/SaveSelectScreen.cpp")
trainer = read("src/Legacy/Gen5ReadOnlyTrainer.cpp")

require(
    surface,
    "Gen5EditorProvider::isGen5NdsId(screen.sourceGameId)",
    "if(!isGen5Source(screen))return false;",
    "if(screen.details.active)",
    "details.readOnly=true;",
    "details.editSnapshot.clear();",
    "screen.closeDetailsModal();",
    "screen.drawGSCOverlay(fb)",
    "return true;",
    "No trainer renaming, money editing, source-save or bank commands.",
    "(void)touch;",
)
# The Gen V module itself must never invoke any generic source or bank mutation.
for forbidden in (
    "updateLegacyBase(", "updateGSCOverlay(", "stageParty(", "stageBox(",
    "saveChanges(", "saveToSource(", "injectSave(", "writeSave(",
    "storagePickup(", "openStorageEditor(", "openItemsShortcut(",
    "renameBox(", "editTrainerName(", "editTrainerMoney(",
):
    assert forbidden not in surface, f"Gen V surface uses unsupported mutation: {forbidden}"
require(
    composite,
    '#include "Gen5SharedReadOnlySurface.inc"',
    "Gen5SharedReadOnlySurface::handleInput(*this, down, held, stick.x, stick.y, touch)",
    "Gen5SharedReadOnlySurface::draw(*this, fb)",
)
assert composite.index("Gen5SharedReadOnlySurface::handleInput(") < composite.index(
    "Gen4SharedEditorSurface::handleInput("
)
assert composite.index("Gen5SharedReadOnlySurface::draw(") < composite.index(
    "Gen4SharedEditorSurface::draw("
)
require(
    manager,
    "bool UIManager::handleGen5View(",
    "identity->dataGeneration!=5",
    "identity->support!=PokeVault::Games::SourceSupport::Planned",
    "intent!=SaveSelectScreen::OpenIntent::Default",
    "Gen5::openAssignedSource(",
    "opened.ready()",
    "opened.instance.gameId!=gameId",
    "Gen5ReadOnlyTrainer::create(",
    "PokeVault::Safety::SourceKind::ExternalLegacy",
    "trainerScreen.detailViewActive=true;",
)
require(manager_header, "bool handleGen5View(")
require(
    games,
    "if (selected.sourceKind == SelectedSourceKind::Gen5AssignedFile)",
    "openGen5Setup(selectedGameId,",
    "discoverGen5Candidates();",
    "reopenValidatedSource(chosen)",
    "selectedSourceKind=SelectedSourceKind::Gen5AssignedFile;",
    "titleSelected=true;",
)
# The remembered assignment is never a silent direct-open shortcut: A must
# reach the chooser, while candidate selection revalidates before routing.
selector = games.split("if (selected.sourceKind == SelectedSourceKind::Gen5AssignedFile) {", 1)[1]
selector = selector.split("if (selected.sourceKind == SelectedSourceKind::Gen4AssignedFile)", 1)[0]
assert "discoverGen5Candidates();" in selector
assert "titleSelected=true;" not in selector
# The display bridge must draw from validated staged PK5, never serialize SAV5.
require(trainer, "Gen5ReadOnlyTrainer::rebuildPresentation(")
assert "fwrite(" not in trainer and "ofstream(" not in trainer

print("Gen V read-only shared UI guard contracts: PASS")
