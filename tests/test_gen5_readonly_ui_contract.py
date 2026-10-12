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
    "trainer.sharedScreen()",
    "trainer.sharedScreen().openActions",
    "state.activateSelected(staged,error)",
    "state.adjustField(delta,error)",
    "trainer.sharedScreen().keepWithPresentation(",
    "state.discardDraft()",
    "state.requestExit(staged)",
    "state.confirmDiscardAndExit(staged)",
    "state.discardAllWithPresentation(staged,",
    "state.moveReviewPage(1,staged)",
    "verifyStagedReview(change,&error)",
    "reviewedFieldLabel(verified->fields[field])",
    "screen.drawGSCOverlay(fb)",
    "screen.details.active=false;",
    "drawVerifiedSummary(screen,fb,&view",
    "return true;",
    "All inherited generic storage/bank/source mutation routes stay barred.",
    "(void)touch;",
)
# Passive PK5 View must not invoke the legacy all-generation legality report.
# Unknown/unsupported Gen V history is unresolved, not hard-invalid.
assert "Gen5SharedReview.h" in surface
assert "verifyStagedReview(" in read("include/Integration/Gen5/Gen5SharedReview.h")
assert "Legality::analyze(" not in surface
assert surface.index("screen.details.active=false;") < surface.index("drawVerifiedSummary(screen,fb,&view")
assert "Gen5SharedScreenState" in read("include/Legacy/Gen5ReadOnlyTrainer.h")
assert "Gen5SharedPokemonSession draft_;" in read("include/Integration/Gen5/Gen5SharedScreenState.h")
assert "requestExit(const Workspace& workspace)" in read("include/Integration/Gen5/Gen5SharedScreenState.h")

# A successful action now includes atomic PK5 presentation acceptance; a
# rejected renderer must preserve the active draft and previous stage.
screen_model = read("include/Integration/Gen5/Gen5SharedScreenState.h")
require(screen_model, "baselineWorkspace=workspace", "baselineDraft=draft_",
        "workspace=std::move(baselineWorkspace);",
        "surface_=baselineSurface;",
        "discardAllWithPresentation(Workspace& workspace")

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
# The same physical Gen V SAV may contain two valid, different retail
# copies. The user must choose Primary/Backup explicitly; selection must
# survive discovery, source assignment and the final exact-game reopen.
discovery = read("src/Integration/Gen5/Gen5SourceDiscovery.cpp")
instance = read("include/Source/SaveInstance.h")
selector_header = read("include/UI/SaveSelectScreen.h")
ui_header = read("include/UI/UI.h")
require(instance, "std::string sourceVariant;",
        "existing.sourceVariant != instance.sourceVariant",
        "shown.sourceVariant == fresh.sourceVariant")
require(discovery, 'row.diagnostic.find("two valid but different")',
        "SaveCopySelection::Primary",
        "SaveCopySelection::Backup",
        'selected.sourceVariant=="gen5-copy-0-primary"',
        'selected.sourceVariant=="gen5-copy-1-backup"',
        "Gen V copy choice conflicts with validated Save Instance")
require(games, "gen5Instances=catalog.rows;",
        'chosen.sourceVariant=="gen5-copy-0-primary"',
        'chosen.sourceVariant=="gen5-copy-1-backup"',
        "selectedGen5Copy=", "gen5CandidateScroll=0;")
require(selector_header, "getSelectedGen5Copy() const",
        "SaveCopySelection selectedGen5Copy")
require(ui_header, "SaveCopySelection selectedCopy")
require(manager, "selectScreen.getSelectedGen5Copy()",
        "legacySourceBindings,profileIdentity(userUid),gameId,selectedCopy")
# A file with two different checksum-valid copies cannot open through an
# automatic persisted assignment that does not record which copy was chosen.
require(games, '"CHOOSE COPY"', "SaveCopySelection::Automatic",
        "const bool separateGen5Copy =",
        "instance.generation==5 && !instance.sourceVariant.empty()",
        "if (!separateGen5Copy &&")


# The display bridge must draw from validated staged PK5, never serialize SAV5.
require(trainer, "Gen5ReadOnlyTrainer::rebuildPresentation(")
assert "fwrite(" not in trainer and "ofstream(" not in trainer

# The candidate serializer may only build an app-owned byte array; no provider
# write, Inject Save or generic source mutation is licensed by this milestone.
candidate = read("include/Integration/Gen5/Gen5StagedSaveCandidate.h")
require(candidate, "verifyStagedReview(change, &reviewError)",
        "Gen5ReadOnlySave::parse(output.bytes, source.exactGameId()",
        "output.bytes.assign(original.begin(), original.end());",
        "Gen V export modified bytes outside verified PK5 and CRC ranges",
        "const auto selection = source.selectedBackupPartition()",
        "buildStagedSaveCandidateForObservedSource(",
        "Gen V source changed since staging; reopen and revalidate the Save Instance",
        "std::equal(observedBytes.begin(),observedBytes.end(),baseline.begin())")
for forbidden in ("fwrite(", "ofstream", "std::filesystem", "injectSave("):
    assert forbidden not in candidate, f"Gen V candidate must not write sources: {forbidden}"

print("Gen V read-only shared UI guard contracts: PASS")
