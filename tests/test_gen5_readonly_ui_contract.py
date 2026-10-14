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
    "screen.touchedButtonId(touch);",
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


# A selected physical source is also an EXACT CONTENT SNAPSHOT. Inode and
# path equivalence alone cannot exclude an in-place emulator save rewrite.
# Thread the selected normalized fingerprint from the candidate through
# UIManager to the final twice-validated read-only source opener.
assigned_h = read("include/Integration/Gen5/Gen5AssignedSource.h")
assigned_cpp = read("src/Integration/Gen5/Gen5AssignedSource.cpp")
require(selector_header, "getSelectedGen5Fingerprint() const",
        "std::string selectedGen5Fingerprint;")
require(games, "selectedGen5Fingerprint.clear();",
        "selectedGen5Fingerprint=fresh.instance.contentFingerprint;")
require(ui_header, "std::string_view selectedFingerprint")
require(manager, "selectScreen.getSelectedGen5Fingerprint()",
        "if(selectedFingerprint.empty())",
        "gameId,selectedCopy,",
        "selectedFingerprint);")
require(assigned_h, "std::string_view expectedContentFingerprint = {}")
require(assigned_cpp, "expectedContentFingerprint",
        "out.instance.contentFingerprint!=expectedContentFingerprint",
        "probe.instance.contentFingerprint!=expectedContentFingerprint",
        "source contents changed after selection",
        "source contents changed during final revalidation")

# A bounded source discovery can produce a useful partial list; the UI
# must clearly report that its search is incomplete even when rows exist.
discovery_header = read("include/Integration/Gen5/Gen5SourceDiscovery.h")
require(discovery_header, "size_t maxEntries = 4096;",
        "size_t maxDirectories = 128;",
        "size_t entriesExamined = 0;",
        "size_t directoriesExamined = 0;")
require(discovery, "entryLimitReached", "s.result.entriesExamined",
        "s.result.directoriesExamined", "limits.maxEntries,32768",
        "limits.maxDirectories,512")
require(games, 'gen5Notice+=" Scan limit reached; list may be incomplete."')

# Generation V source selection now follows the already accepted Gen IV
# touch contract: a clean release inside the same clipped visible row may
# choose a save, while a vertical drag only scrolls the viewport. Touch does
# not bypass source assignment, fingerprint selection or strict revalidation.
require(selector_header, "TouchScrollState gen5CandidateTouchScroll;",
        "Gen5UnassignConfirm")
require(games, "gen5CandidateTouchScroll.updateVertical(",
        "gen5CandidateTouchScroll.offset()",
        "gen5CandidateTouchScroll.reset();",
        "gen5CandidateTouchScroll.stop();",
        "case Overlay::Gen5Candidates: gen5CandidateIndex = idx;",
        "case Overlay::Gen5Candidates: gen5CandidateIndex = r.idx;",
        "pushViewportHit(overlayRects,x+24,touchY,w-48,rowH-6,i,",
        "touch.justTapped()", "const int maxFirst=std::max(0,count-visibleRows);")
gen5_input = games.split("if (overlay == Overlay::Gen5Candidates) {", 1)[1]
gen5_input = gen5_input.split("if (overlay == Overlay::Gen4Setup)", 1)[0]
assert "gen5CandidateTouchScroll.updateVertical(" in gen5_input
assert "assignGen5Candidate(gen5Instances[" in gen5_input
assert "if(kDown & (HidNpadButton_Up|HidNpadButton_Down))" in gen5_input
gen5_draw = games.split("overlay == Overlay::Gen5Candidates && u)", 1)[1]
gen5_draw = gen5_draw.split("overlay == Overlay::Gen4Setup && u)", 1)[0]
assert "pushViewportHit(overlayRects,x+24,touchY" in gen5_draw
assert "gen5CandidateTouchScroll.offset()" in gen5_draw

# Forgetting a remembered source is a metadata mutation and cannot be
# performed by A or a single tap. Y in a separate confirmation is required;
# there are still no original SAV writes anywhere on the Gen V route.
gen5_setup = games.split("if (overlay == Overlay::Gen5Setup) {", 1)[1]
gen5_setup = gen5_setup.split("if (overlay == Overlay::Gen5Candidates)", 1)[0]
assert "gen5SetupIndex==2)overlay=Overlay::Gen5UnassignConfirm;" in gen5_setup
assert "gen5SetupIndex==2)unassignCurrentGen5Game()" not in gen5_setup
# Local NRO fallback must be a detached, exact-SHA devkitA64 build.
# A queued remote runner cannot turn a stale binary into a valid candidate.
build_sh=read("tools/build_gen5_smoke_nro.sh")
require(build_sh,
        'git clone --local --no-hardlinks "$ROOT" "$TMP/work"',
        'git -C "$WORK" checkout --detach "$EXPECTED"',
        "git -C \"$WORK\" submodule update --init --recursive",
        'git -C "$WORK" worktree add --detach',
        'python3 tools/check_device_assets.py',
        'test -s PokeBankNX.nro',
        'sha256sum "$OUT_NRO" > SHA256SUMS.txt',
        "SOURCE_SAV5_WRITE=DISABLED",
        "DEVICE_ACCEPTED=false")
require(read(".github/workflows/native-pr-build.yml"),
        "bash -n tools/build_gen5_smoke_nro.sh",
        "test -x tools/build_gen5_smoke_nro.sh")

# Both Gen V pairs use their saved gender to select the correct actual
# protagonist image; missing/unassigned metadata uses a neutral caption.
product_home = read("src/UI/SaveSelectScreen.cpp")
product_art = read("tools/fetch_product_art.py")
for exact_game in ("black_nds", "white_nds", "black2_nds", "white2_nds"):
    require(product_home, f'gameId == "{exact_game}"')
for portrait in ("hilbert", "hilda", "nate", "rosa"):
    require(product_home, f'"{portrait}"')
    require(product_art, f'"{portrait}.png": "https://play.pokemonshowdown.com/sprites/trainers/{portrait}.png"')

# Failed Gen V raw files under a mixed RetroArch directory must remain
# diagnostically visible but NEVER selectable or silently reinterpreted.
source_scan = read("src/Integration/Gen5/Gen5SourceDiscovery.cpp")
host_gen5 = read("tests/test_gen5_readonly_save.cpp")
require(source_scan,
        'row.sourceVariant=="gen5-unverified-version-hint"',
        'row.validation=statusForFailure(error)',
        'row.diagnostic+="; unverified Gen V header suggests "')
require(host_gen5,
        "rejectedRetroArchGen5DiagnosticContracts()",
        "assert(catalog.rows.empty() && catalog.invalidOrAmbiguous==1)",
        "assert(!G::reopenValidatedSource(inspected).ready())")
require(games, 'item.sourceVariant=="gen5-unverified-version-hint"')

# A successful Gen IV scan cannot prove Gen V opened any emulator root;
# retain exact hardware scan counts and the first strict read-only rejection.
discovery = read("src/Integration/Gen5/Gen5SourceDiscovery.cpp")
discovery_header = read("include/Integration/Gen5/Gen5SourceDiscovery.h")
require(discovery_header, "rootsConsidered", "rootsOpened")
require(discovery, "++state.result.rootsConsidered",
        "++state.result.rootsOpened")
require(games, "discovered.rootsOpened==0",
        "Gen V scan: game=", "Gen V strict read-only rejection:")

# When no Gen V Save Instance is presented, hardware users must learn if
# the same emulator folder was scanned but rejected by strict validation.
require(
    games,
    "discovered.filesExamined==0",
    "catalog.ownedByOtherProfile>0",
    "catalog.wrongGame>0",
    "catalog.invalidOrAmbiguous",
    "Example rejection: ",
    "gen5Notice.substr(maxPerLine,maxPerLine)",
)
assert 'No valid Gen V cartridge saves found; use a manual file' not in games

# Native NRO packaging must not erase region scenery on the integration branch.
# Gen V retail covers must resolve just like Gen I-IV, and preflight must reject
# a reintroduced missing-region/missing-cover regression before emitting an NRO.
workflow = read(".github/workflows/native-pr-build.yml")
makefile = read("Makefile")
identity = read("src/Games/GameIdentity.cpp")
asset_preflight = read("tools/check_device_assets.py")
assert "rsync -a --ignore-existing recovery-work/romfs/ romfs/" in workflow
assert "rm -rf romfs" not in workflow
for game in ("black_nds", "white_nds", "black2_nds", "white2_nds"):
    require(identity, f'if (id == "{game}") return "romfs:/game_cards/{game}.png";')
    require(makefile, f'$(GAME_CARD_ART_SOURCE)/{game}.png')
    require(asset_preflight, f'"{game}.png"')
require(asset_preflight, 'REGION_BACKDROP_DIR', 'all 10 regional backdrops are packaged',
        'TRAINER_PORTRAIT_DIR', 'from fetch_product_art import TRAINER_URLS, REQUIRED_REGIONS')
require(workflow, 'python3 tools/fetch_product_art.py')

# Four native titles, profile-isolated assignments, and the existing
# shared 6x5 Box grid are exercised by executable host fixtures.
assignment=read("tests/test_gen5_readonly_save.cpp")
require(assignment,
        "void fourTitleProfileAssignmentContracts()",
        "fourTitleProfileAssignmentContracts();",
        '"black_nds","profile-a"',
        '"white_nds","profile-b"',
        '"black2_nds","profile-a"',
        '"white2_nds","profile-b"',
        "replaceFileAssignmentAndSave(source.sourceIdentity",
        "G::forGameAndProfile(discoveries,restored",
        "G::SaveCopySelection::Primary",
        "G::SaveCopySelection::Backup")
box_model=read("include/Integration/Gen5/Gen5BoxTouchNavigation.h")
require(box_model,
        "enum class BoxTouchKind5",
        "decodeGen5BoxTouch(",
        "hitId==1000","hitId==1001","hitId==2000",
        "BoxTouchKind5::Ignore","advanceGen5BoxIndex(")
require(surface,
        "Gen5BoxTouchNavigation.h",
        "decodeGen5BoxTouch(",
        "case TouchKind::FocusSlot:",
        "case TouchKind::OpenSlot:",
        "case TouchKind::Ignore:",
        "advanceGen5BoxIndex(")
require(read("tests/test_gen5_box_touch.cpp"),
        "for(int selected=-1;selected<30;++selected)",
        "1002,1003,1100,2001,77000,80000",
        "G::advanceGen5BoxIndex(index,count,delta)")
require(read("Makefile.host.gen5"),
        "GEN5_BOX_TOUCH_TEST :=","GEN5_BOX_TOUCH_SANITIZE :=",
        "host-test: $(GEN5_BOX_TOUCH_TEST)",
        "host-sanitize: $(GEN5_BOX_TOUCH_SANITIZE)")
require(read(".github/workflows/gen5-focused-host.yml"),
        "test_gen5_create_integration test_gen5_box_touch test_gen5_create_input_rules; do",
        "test_gen5_create_integration_sanitize test_gen5_box_touch_sanitize test_gen5_create_input_rules_sanitize; do")

# Shared Gen V action-screen Undo is limited to app-owned Box Clone,
# Remove and Create transactions; never reach generic storage/save handlers.
require(read("include/Integration/Gen5/Gen5SharedScreenState.h"),
        "selectedBoxUndoAvailable(",
        "undoSelectedBoxReplacement(",
        "ws.undoBoxReplacement(",
        "ws=std::move(snapshot)",
        "surface_=Surface::Browse")
require(surface,
        "state.undoSelectedBoxReplacement(staged",
        "state.selectedBoxUndoAvailable(workspace)",
        '"Y","Undo This Slot"')
undo_tests=read("tests/test_gen5_readonly_save.cpp")
require(undo_tests,
        "actionUi.selectedBoxUndoAvailable(ops)",
        "actionUi.undoSelectedBoxReplacement(",
        "Forced presentation failure for Undo regression")
create_undo_tests=read("tests/test_gen5_create_integration.cpp")
require(create_undo_tests,
        "createUi.selectedBoxUndoAvailable(workspace)",
        "createUi.undoSelectedBoxReplacement(workspace",
        "Create Undo presentation failure")

# Gen V game card selection must not re-read/parse full SAV5 every time
# focus changes. The cached preview cannot authorize opening an editor.
selector_header=read("include/UI/SaveSelectScreen.h")
require(selector_header,
        "std::optional<PokeVault::Integration::Gen5::GameCardReadOnlyPreview> gen5Preview;")
require(games,
        "card.gen5Preview=*preview;",
        "const auto& cached=title.gen5Preview;",
        "const auto& preview=title.gen5Preview;",
        "Gen V cached party; exact source verified on Open.")
fast=games.split("void SaveSelectScreen::refreshHubSelectionFromCache()",1)[1]
fast=fast.split("void SaveSelectScreen::refreshHubPreview(bool resolveLaunchTarget)",1)[0]
assert "Gen V cached party; exact source verified on Open." in fast
assert "Gen5::openAssignedSource(" not in fast
full=games.split("void SaveSelectScreen::refreshHubPreview(bool resolveLaunchTarget)",1)[1]
full=full.split("void SaveSelectScreen::begin",1)[0] if "void SaveSelectScreen::begin" in full else full
assert "const auto& preview=title.gen5Preview;" in full
require(manager,
        "Gen5::openAssignedSource(",
        "selectScreen.getSelectedGen5Fingerprint()")

# Native held items belong to the same Gen I-IV Create and Edit model.
# The new box record encodes the native item ID, not an invented provenance.
require(read("include/Integration/Gen5/Gen5CreateValidation.h"),
        "heldItem = 0",
        "if (r.heldItem > 638)",
        "CreateError5")
require(read("include/Integration/Gen5/Gen5NativeCreate.h"),
        "Crypto::write16(plain, 0x0A, static_cast<uint16_t>(r.heldItem))")
require(surface,"Add Pokemon - Held Item ID (0=None)")
require(read("tests/test_gen5_native_create.cpp"),
        "withItem.heldItem=638",
        "r.heldItem=639",
        "p.heldItem() == r.heldItem")
require(read("tests/test_gen5_create_integration.cpp"),
        "r.heldItem=638",
        "createdFull->boxPokemon(0,1)->heldItem()==638")

# Reject native Create input mistakes at the point of entry rather than
# collecting a full wizard and then failing after ~30 on-screen prompts.
input_rules=read("include/Integration/Gen5/Gen5CreateInputRules.h")
require(input_rules,
        "createLanguageAllowed5(",
        "createLastCalendarDay5(",
        "createMoveChoiceAllowed5(",
        "createRemainingEVMax5(",
        "static_assert(!createLanguageAllowed5(6))")
require(read("include/Integration/Gen5/Gen5CreateValidation.h"),
        'include "Integration/Gen5/Gen5CreateInputRules.h"',
        "createFormNeedsUnsupportedHandling5(",
        "createLanguageAllowed5(language)",
        "createLastCalendarDay5(e.year,e.month)")
require(surface,
        "basePersonal.info->formCount>1",
        "createFormNeedsUnsupportedHandling5(request.species,request.form)",
        "G::createLanguageAllowed5(ot.language)",
        "G::isNativeMetLocation5(request.encounter.location)",
        "G::createLastCalendarDay5(",
        "G::createMoveChoiceAllowed5(chosenMoves,i,id)",
        "G::createRemainingEVMax5(request.evs,i)",
        "Current PP",
        "PP Ups")
require(read("Makefile.host.gen5"),
        "GEN5_CREATE_INPUT_TEST :=","GEN5_CREATE_INPUT_SANITIZE :=")
require(read(".github/workflows/gen5-focused-host.yml"),
        "test_gen5_box_touch test_gen5_create_input_rules; do",
        "test_gen5_box_touch_sanitize test_gen5_create_input_rules_sanitize; do")
require(read("tests/test_gen5_create_input_rules.cpp"),
        "for(uint32_t y=2000;y<=2099;++y)",
        "G::createMoveChoiceAllowed5(",
        "G::createRemainingEVMax5(")

# Prompt retries are local to the selected Trainer/box and cannot become
# implicit source-save changes or leak across a new Create/cancel session.
require(read("include/Integration/Gen5/Gen5SharedScreenState.h"),
        "pendingCreateInput()",
        "rememberCreateInput(",
        "pendingCreateInput_=request",
        "pendingCreateInput_.reset()")
require(surface,
        "state.pendingCreateInput().value_or(",
        "state.rememberCreateInput(ws,request)",
        "dest>=static_cast<uint32_t>(minimum)",
        "out.empty()?initial:out",
        "request.pid=(pidHigh<<16)|pidLow",
        "G::CreateTrainer5& ot=*request.trainer")
require(read("tests/test_gen5_create_integration.cpp"),
        "addScreen.pendingCreateInput()",
        "addScreen.rememberCreateInput(workspace,r)",
        "wrongDraft.game=")
require(read("tests/test_gen5_readonly_save.cpp"),
        "partialCreate.nickname=\"LOCALONLY\"",
        "assert(!addMenu.pendingCreateInput())")

# The existing Gen I-IV Add Pokemon command must now be reachable from
# a validated empty Gen V box cell, collect explicit native fields with the
# same on-device keyboard, and stage only after confirm + preview refresh.
# Reusable shared editor source aliases must be bound before the early
# native Create/Clone action contracts that inspect them.
native_bridge = read("include/Integration/Gen5/Gen5SharedEditorBridge.h")
native_screen = read("include/Integration/Gen5/Gen5SharedScreenState.h")
require(native_bridge,
        "workspace.emptyCloneDestination(slot.location.box,slot.location.slot)",
        "append(Shared::Action::Add)")
require(native_screen,
        "ConfirmCreateBox",
        "if(action==Action::Add)",
        "ws.createInEmptyBox(request,target_.box,target_.slot,&error)",
        "confirmCreateBox(Workspace& ws",
        "surface_=Surface::ConfirmCreateBox")
require(surface,
        "bool promptNativeBoxCreate(",
        "Utils::promptNumber(",
        "Utils::promptText(",
        "request.pid=(pidHigh<<16)|pidLow",
        "request.encounter.kind=G::CreateHistory5::CallerSuppliedNative",
        "request.gender=G::createGenderForPID5",
        "state.confirmCreateBox(ws,request",
        "case Surface::ConfirmCreateBox:",
        '"Y","Enter Create Fields"',
        "state.cancelBoxAction()")
add_suite=read("tests/test_gen5_create_integration.cpp")
require(add_suite,
        "addScreen.availableActions(workspace)",
        "Action::Add",
        "addScreen.confirmCreateBox(workspace",
        "Simulated shared editor presentation failure",
        "otherTitle.game=static_cast<G::CreateGame5>",
        "addScreen.surface()==Surface::Browse")
require(read("tests/test_gen5_readonly_save.cpp"),
        "staleActions.count==3",
        "staleActions[0]==GA::Add",
        "Surface::ConfirmCreateBox",
        "partyActions[i]!=GA::Add")

# Native Create and Clone become editable in the SAME shared View/Edit
# session, retaining independently reconstructable origin certificates.
require(read("include/Integration/Gen5/Gen5StagedPokemonWorkspace.h"),
        "replacement.replacementOriginal=encrypted",
        "replacement.replacementOriginal=result.encrypted",
        "replacement.replacementOriginal",
        "edit->stage(field,stat,value,error)",
        "edit->stageName(nickname,name,error)")
require(read("include/Integration/Gen5/Gen5SharedReview.h"),
        "fieldsOnly.kind=Kind::FieldEdit",
        "fieldsOnly.before=change.replacementOriginal",
        "verifyStagedReview(fieldsOnly,error)",
        "result.fields.insert(result.fields.end()")
require(native_bridge,
        "append(Shared::Action::Edit)",
        "workspace.originalOccupied(slot.location)")
require(read("tests/test_gen5_create_integration.cpp"),
        "editCreated.activate(workspace,Action::Edit,error)",
        "nativeEdit.keep(workspace,error)",
        "changedCreate[0].replacementOriginal==created.encrypted")
require(read("tests/test_gen5_staged_save_candidate.cpp"),
        "boxOps.stageBox(0,1,Field::Friendship,0,199,&error)",
        "boxedVerified->fields.size()==2")

# Native PK5 creation must enter the SAME app-owned workspace/Review/export
# pipeline, never get a separate per-generation save writer or editor shell.
created_workspace=read("include/Integration/Gen5/Gen5StagedPokemonWorkspace.h")
created_review=read("include/Integration/Gen5/Gen5SharedReview.h")
created_export=read("include/Integration/Gen5/Gen5StagedSaveCandidate.h")
created_factory=read("include/Integration/Gen5/Gen5NativeCreate.h")
require(created_workspace,
        "CreateBox", "createInEmptyBox(",
        "emptyCloneDestination(box,slot)",
        "createNative5(request)",
        "replacement.createRequest=request",
        "std::optional<CreateRequest5> createRequest")
require(created_review,
        "change.kind==Kind::CreateBox",
        "change.createRequest",
        "createNative5(*change.createRequest)",
        "rebuilt.encrypted!=change.replacementOriginal")
require(created_export,
        "Gen5StagedPokemonWorkspace::ChangeKind::CreateBox",
        "request->game)!=source.trainer().gameVersion",
        "created.encrypted!=change.replacementOriginal")
require(created_factory,
        "CreateResult5 createNative5(",
        "Crypto::encryptCandidate(plain)")
create_suite=read("tests/test_gen5_create_integration.cpp")
require(create_suite,
        "workspace.createInEmptyBox(r,0,1,&error)",
        "checkedCreate->fields[0].field==Field::CreateBox",
        "buildStagedSaveCandidateForObservedSource(",
        "workspace.undoBoxReplacement(0,1)",
        "CHECK(input==immutable)")

# Gen V reuses the already-shared staged editor, not a second Create/Edit UI.
# Game cards and the source picker must not claim the editor is unavailable.
require(games,
        "Gen V cached party; choose Save Instances to open.",
        "Open the shared staged editor. The original save stays unchanged.",
        '"A","Open Editor"',
        "Review or change this Gen V profile/save assignment.")
assert "Gen V preview only — editor not yet enabled." not in games
require(manager, "Gen V reuses the", "shared View/Edit/Review field model")
require(games, "if (overlay == Overlay::Gen5UnassignConfirm)",
        "else if(kDown & HidNpadButton_Y)unassignCurrentGen5Game();",
        "Original save files remain untouched.")
assert "Editor pending." not in games

# Gen IV parity tranche: native Gen V Held Item, Ball and Met Level fields
# must be wired through the SAME draft, Keep, Review and export whitelist.
# Hidden Ability, Pokerus, PID/gender and original trainer gender are
# preserved rather than silently regenerated.
read_pk5 = read("include/Pokemon/Pokemon5ReadOnly.h")
native_stage = read("include/Integration/Gen5/Gen5StagedPokemonRecord.h")
native_session = read("include/Integration/Gen5/Gen5SharedPokemonSession.h")
native_review = read("include/Integration/Gen5/Gen5SharedReview.h")

shared_contract = read("include/UI/SharedPokemonEditorContract.h")
require(read_pk5, "pokerusState() const", "ball() const",
        "metLevel() const", "originalTrainerGender() const",
        "metLocation() const")
require(native_stage, "Field : uint8_t { Nature, Friendship, IV, EV, HeldItem, Ball, MetLevel,",
        "case Field::HeldItem:", "case Field::Ball:",
        "case Field::MetLevel:", "value > 638",
        "value > 25", "value > 100",
        "(candidate[0x84] & 0x80) | value")
require(native_session, "apply(Field::HeldItem,0,after.heldItem())",
        "apply(Field::Ball,0,after.ball())",
        "apply(Field::MetLevel,0,after.metLevel())")
require(native_review, "apply(Field::HeldItem,0,before.heldItem(),after.heldItem())",
        "apply(Field::Ball,0,before.ball(),after.ball())",
        "apply(Field::MetLevel,0,before.metLevel(),after.metLevel())",
        "Gen V pending review includes unrelated PK5 byte changes")
require(native_bridge, "NativeField::HeldItem,stat,value,error",
        "NativeField::Ball,stat,value,error",
        "NativeField::MetLevel,stat,value,error")
require(native_screen, "std::array<EditableRow,43>",
        '"Held Item ID",638', '"Ball ID",25', '"Met Level",100',
        "case Field::HeldItem:return pk.heldItem();",
        "case Field::Ball:return pk.ball();",
        "case Field::MetLevel:return pk.metLevel();")
require(shared_contract, "case FieldIdentity::HeldItem:",
        "case FieldIdentity::Ball:", "case FieldIdentity::MetLevel:")
require(surface, "Gen V identity + encounter edits remain staged and reversible")

# Gen V identity/encounter parity is source-backed and independent from
# future event/encounter legality. Each field flows through exact PK5 stage,
# atomic Keep, independent Review and verified app-owned SAV export.
locations = read("include/Integration/Gen5/Gen5EncounterLocations.h")
require(locations, "Locations5.cs",
        "std::array<uint16_t,276> NativeMetLocations5",
        "isNativeMetLocation5(", "locationIndex5(",
        "30001", "40001", "60003", "!isNativeMetLocation5(138)")
require(read_pk5, "fatefulEncounter() const", "metLocation() const",
        "originalTrainerGender() const", "pokerusMode() const")
require(native_stage, "case Field::TrainerId:",
        "case Field::SecretId:", "case Field::OriginalTrainerGender:",
        "case Field::Pokerus:", "case Field::MetLocation:",
        "case Field::FatefulEncounter:",
        "Crypto::write16(candidate,0x0C",
        "Crypto::write16(candidate,0x0E",
        "(candidate[0x84]&0x7F)",
        "(candidate[0x40]&~uint8_t{1})",
        "isNativeMetLocation5(value)", "baselineMode")
require(native_session, "Field::TrainerId,0,after.tid()",
        "Field::SecretId,0,after.sid()",
        "Field::OriginalTrainerGender,0,after.originalTrainerGender()",
        "Field::Pokerus,0,after.pokerusMode()",
        "Field::MetLocation,0,after.metLocation()",
        "Field::FatefulEncounter,0,after.fatefulEncounter()?1:0")
require(native_review, "Field::TrainerId,0,before.tid(),after.tid()",
        "Field::SecretId,0,before.sid(),after.sid()",
        "Field::Pokerus,0,before.pokerusMode(),after.pokerusMode()",
        "Field::MetLocation,0,before.metLocation(),after.metLocation()")
require(native_bridge, "NativeField::TrainerId,stat,value,error",
        "NativeField::SecretId,stat,value,error",
        "NativeField::Pokerus,stat,value,error",
        "NativeField::FatefulEncounter,stat,value,error",
        "NativeField::MetLocation,stat,value,error")
require(native_screen, "std::array<EditableRow,43>",
        '"Trainer ID",65535', '"Secret ID",65535',
        '"OT Gender",1', '"Pokerus",2', '"Met Location",65535',
        '"Fateful",1',
        "LocationPicker", "openLocationPicker(", "acceptLocationPicker(",
        "cancelLocationPicker()", "stageFieldValue(")
require(shared_contract, "case FieldIdentity::FatefulEncounter:",
        "case FieldIdentity::TrainerId:",
        "case FieldIdentity::SecretId:")
require(surface, "void drawLocationPicker(", "case Surface::LocationPicker:",
        "kGen5LocationTouchBase=78000", "Names::getMetLocationName(",
        "Utils::promptNumber(", "state.stageFieldValue(",
        "state.acceptLocationPicker(error)", "state.cancelLocationPicker()",
        "Gen V value changed in local draft; source unchanged")

# Gen V form, ability-slot and gender use distinct PKHeX BW/B2W2
# personal data; never borrow Gen IX abilities or accept an invented form.
personal = read("include/Integration/Gen5/Gen5PersonalData.h")
require(personal, "std::array<Personal5,668> NativePersonalBW",
        "std::array<Personal5,709> NativePersonalB2W2",
        "personal_bw blob de1a0654e8540a5c7e5c8b5e6448db50bb6d8c55",
        "personal_b2w2 blob 9236ad139126af222e915650dbda70d6df897399",
        "personal5(", "abilityForSlot5(", "genderAllowed5(",
        "base.formIndex==0", "bool b2w2")
require(native_stage, "case Field::Form:", "case Field::AbilitySlot:",
        "case Field::Gender:", "genderAllowed5(", "normalizedGender5(",
        "Party battle stats are regenerated after the native identity",
        "Gen V ability would change shininess")
require(native_bridge, "NativeField::Form,stat,value,error",
        "NativeField::AbilitySlot,stat,value,error",
        "NativeField::Gender,stat,value,error")
require(native_session, "Field::Form,0,after.form()",
        "Field::AbilitySlot,0,slot5(after)",
        "Field::Gender,0,after.gender()")
require(native_review, "Field::Form,0,before.form(),after.form()",
        "Field::AbilitySlot,0,slot5(before),slot5(after)",
        "Field::Gender,0,before.gender(),after.gender()")
require(native_screen, "IdentityPicker", "openIdentityPicker(",
        "acceptIdentityPicker(", "cancelIdentityPicker()",
        "identityChoiceAllowed(", "identityChoiceCount()",
        '"Ability Slot",2', '"Form",31', '"Gender",2')
require(surface, "void drawIdentityPicker(",
        "case Surface::IdentityPicker:",
        "kGen5IdentityTouchBase=78500",
        "state.openIdentityPicker(error)",
        "state.acceptIdentityPicker(error)", "state.cancelIdentityPicker()",
        "state.black2White2()", "personal5(")
require(shared_contract, "case FieldIdentity::Form:",
        "case FieldIdentity::Gender:", "case FieldIdentity::Ability:")
require(read("tests/test_gen5_readonly_save.cpp"),
        "ui.openIdentityPicker(error)", "ui.acceptIdentityPicker(error)",
        "kEditableRows.size()==43")
require(read("tests/test_gen5_pk5_readonly.cpp"),
        "G5::NativePersonalBW.size()==668", "G5::NativePersonalB2W2.size()==709",
        "G5::personal5(386,1,false)", "Field::AbilitySlot")

# Native Gen V move data is pinned to PKHeX MoveInfo5 (560 PP values).
# The controller/touch picker is a sub-surface of the same guarded Edit draft,
# never a generic source write or an inferred species learnability checker.
move_pp = read("include/Integration/Gen5/Gen5MovePP.h")
require(move_pp, "std::array<uint8_t,560>",
        "MaxNativeMove5 = 559", "basePP5(", "maxPP5(",
        "MoveInfo5.cs", "66ef5983a5d349efa2dd615f2926a1b1ee97306e")
require(native_stage, "case Field::Move:", "case Field::MovePP:",
        "case Field::MovePPUps:", "candidate[0x30+stat]=basePP5(value);",
        "candidate[0x34+stat]=0;", "value>maxPP5(move,ups)")
require(native_session, "Field::Move,i,afterMoves[i]",
        "Field::MovePPUps,i,afterUps[i]",
        "Field::MovePP,i,afterPP[i]")
require(native_review, "Field::Move,i,beforeMoves[i],afterMoves[i]",
        "Field::MovePPUps,i,beforeUps[i],afterUps[i]",
        "Field::MovePP,i,beforePP[i],afterPP[i]")
require(native_bridge, "NativeField::Move,stat,value,error",
        "NativeField::MovePP,stat,value,error",
        "NativeField::MovePPUps,stat,value,error")
require(native_screen, "Surface::MovePicker", "openMovePicker(",
        "movePickerCursor(", "acceptMovePicker(", "cancelMovePicker()",
        "std::array<EditableRow,43>", "Field::Move:return pk.moves()[row.stat]",
        "Field::MovePPUps:return pk.ppUps()[row.stat]",
        "Field::MovePP:return pk.pp()[row.stat]")
require(surface, "case Surface::MovePicker:", "drawMovePicker(",
        "kGen5MovePickerTouchBase=77500",
        "Names::getMoveName(id)", "basePP5(id)",
        "state.acceptMovePicker(error)", "state.cancelMovePicker()",
        "species learnability is not yet evaluated")

# Touch-to-Edit stays on the verified native PK5 field transaction
# model. The shared Add action uses explicit prompts and a staged transaction;
# inherited source-save, Storage/Bank and generic mutation routes stay barred.
# Touch IDs and hitboxes are generated by the actual drawn modal rows.
require(surface, "kGen5ActionTouchBase=77000",
        "kGen5FieldTouchBase=77100",
        "kGen5DecreaseTouchBase=77200",
        "kGen5IncreaseTouchBase=77300",
        "kGen5PartyTouchBase=77400",
        "screen.touchedButtonId(touch)",
        "screen.touchButtons.push_back(",
        "down|=HidNpadButton_A;",
        "down|=HidNpadButton_Left;",
        "down|=HidNpadButton_Right;",
        "screen.selectedPartyIndex=tap-kGen5PartyTouchBase;",
        "kGen5ActionTouchBase+static_cast<int>(i)",
        "kGen5FieldTouchBase+static_cast<int>(index)",
        "kGen5DecreaseTouchBase+static_cast<int>(index)",
        "kGen5IncreaseTouchBase+static_cast<int>(index)")
assert "openPokemonActionSheet(" not in surface
assert "touch.justTouchedDown()" not in surface
# The hitboxes must not override controller-confirmation or app-memory
# discard guards; there is no Y mapping for touch and no implicit Keep.
assert "state.confirmDiscardAndExit(staged)" in surface
assert "state.discardAllWithPresentation(staged," in surface

# The display bridge must draw from validated staged PK5, never serialize SAV5.
require(trainer, "Gen5ReadOnlyTrainer::rebuildPresentation(")
assert "fwrite(" not in trainer and "ofstream(" not in trainer

# Working-copy output is a new app-owned, read-back verified directory,
# not a physical provider save writer. The native tests exercise every
# pre-publication failure point and prohibit stale original-image export.
working_header = read("include/Integration/Gen5/Gen5WorkingCopyExport.h")
working_cpp = read("src/Integration/Gen5/Gen5WorkingCopyExport.cpp")
working_tests = read("tests/test_gen5_staged_save_candidate.cpp")
require(working_header, "publishGen5WorkingCopy(",
        "observedSourceBytes",
        "AfterBackup, AfterEdited, AfterReadback, AfterManifest")
require(working_cpp, "buildStagedSaveCandidateForObservedSource(",
        'PokeBank::Paths::exportsRoot()+"/gen5"',
        'root!=fixedRoot',
        '"original_backup.sav"', '"edited.sav"',
        '"EXPORT_MANIFEST.txt"', "readExact(backupPath",
        "readExact(editedPath", "hash(editedReadback)",
        "Gen5ReadOnlySave::parse(",
        "cleanTemporary(temporaryPath);",
        "rename(temporaryPath.c_str(),finalPath.c_str())",
        "INJECT_SAVE=DISABLED", "EMULATOR_SAVE_WRITE=DISABLED")
for forbidden in ("std::fopen(request.sourcePath", '::rename(request.sourcePath',
                  '::remove(request.sourcePath', "injectSave("):
    assert forbidden not in working_cpp, f"Gen V writer touches original source: {forbidden}"
require(working_tests, "Field::MovePPUps,0,3", "Field::MovePP,0,4")
require(working_tests, "WorkingCopyFailurePoint::AfterBackup",
        "WorkingCopyFailurePoint::AfterEdited",
        "WorkingCopyFailurePoint::AfterReadback",
        "WorkingCopyFailurePoint::AfterManifest",
        "stale[0x70000]^=0x80",
        "EMULATOR_SAVE_WRITE=DISABLED",
        "INJECT_SAVE=DISABLED")

# Review -> ConfirmExport is the only Gen V working-copy output action.
# It performs a fresh read-only source normalization, requires explicit Y,
# preserves staged edits on failure/success, and NEVER invokes inherited save.
screen_state = read("include/Integration/Gen5/Gen5SharedScreenState.h")
require(screen_state, "ConfirmExport", "requestExport(const Workspace& workspace)",
        "surface_=Surface::ConfirmExport;",
        "if(surface_==Surface::ConfirmExport)surface_=Surface::Review;")
require(surface, "state.requestExport(staged)",
        "case Surface::ConfirmExport:",
        "if(keys&HidNpadButton_B)", "else if(keys&HidNpadButton_Y)",
        "readNormalizedSourceReadOnly(",
        "publishGen5WorkingCopy(staged,request)",
        "state.finishExport();",
        "WorkingCopyExportRequest request;",
        "accountUidHex(screen.userUid)",
        "request.observedSourceBytes=observed;",
        "Export Verified Generation V Working Copy?",
        "Export Working Copy",
        "original SAV5 unchanged")
# Prohibit accidental write routing through the unguarded Trainer surface.
assert "saveChanges(" not in surface and "saveToSource(" not in surface
assert "injectSave(" not in surface and "openItemsShortcut(" not in surface
assert "openStorageEditor(" not in surface
assert surface.index("state.requestExport(staged)") < surface.index("case Surface::ConfirmExport:")
require(read("tests/test_gen5_readonly_save.cpp"),
        "assert(ui.requestExport(localUiWorkspace));",
        "Surface::ConfirmExport",
        "ui.finishExport();",
        "assert(localUiWorkspace.hasChanges());")

# The no-account "Pokemon Saves" profile has a stable all-zero UID
# identity in the existing provider-neutral assignments. The exporter must
# use that same isolated namespace, not silently reject offline profiles.
require(surface, "if(request.profileIdentity.empty())",
        "request.profileIdentity=std::string(32,'0');")
require(working_cpp, "All-zero is PokeBank NX's deliberate offline",
        "return true;")
require(working_tests, "req.profileIdentity=std::string(31,'0');",
        "req.profileIdentity=std::string(32,'0');",
        "verifyOnDisk(offline.directory+")

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

progression=read("include/Integration/Gen5/Gen5Progression.h")
require(progression,"experienceForLevel5(","levelForExperience5(",
        "regeneratePartyStats5(","0x8E","0x90","0x9A",
        "species==292","oldMax=Crypto::read16(original,0x90)")
require(native_stage,"case Field::Level:","case Field::Experience:",
        "value>experienceForLevel5(personal.info->growthRate,100)",
        "regeneratePartyStats5(candidate,Crypto::decrypt(original_),black2White2_)")
require(native_screen,'{Field::Level,0,"Level",100}',
        '{Field::Experience,0,"Experience",1640000}')
require(native_bridge,"NativeField::Level,stat,value,error",
        "NativeField::Experience,stat,value,error")
require(native_session,"Field::Experience","experienceForLevel5(")
require(read("include/Integration/Gen5/Gen5SharedReview.h"),"Field::Experience",
        "experienceForLevel5(")
require(surface,"Generation V Level","Generation V Experience")
# Species changes must use exact native personal data and never reroll PID.
require(native_stage,"case Field::Species:","value>649",
        "target.info->growthRate!=personal.info->growthRate",
        "genderAllowed5(target.info->genderRatio,gender)",
        "candidate[0x15]=ability;",
        "field==Field::Species || field==Field::Nature")
require(native_session,"apply(Field::Species,0,after.species())")
require(native_review,"Field::Species,0,before.species(),after.species()")
require(native_bridge,"NativeField::Species,stat,value,error")
require(native_screen,'{Field::Species,0,"Species Dex ID",649}',
        "case Field::Species:return pk.species();")
require(surface,"Generation V Species Dex ID (native-compatible only)",
        "case Field::Species:name")
require(read("tests/test_gen5_pk5_readonly.cpp"),
        "Field::Species,0,26","Field::Species,0,32")
require(read("tests/test_gen5_readonly_save.cpp"),
        "assert(ui.acceptSpeciesPicker(error));",
        "localUiWorkspace.viewParty(0)->species()==26")
# Duplicate switch labels are an immediate native C++ compile failure.
gen5_access=shared_contract.split("if (generation == Generation::Gen5) {",1)[1]
gen5_access=gen5_access.split("return FieldAccess::Hidden;\n}",1)[0]
assert gen5_access.count("case FieldIdentity::Nickname:")==1
assert gen5_access.count("case FieldIdentity::Species:")==1

# Species name browsing is a modal, not a write path. Search/cancel
# cannot alter a draft; A uses the same guarded native species transaction.
require(native_screen,
        "Surface::SpeciesPicker", "openSpeciesPicker(",
        "moveSpeciesCursor(", "filterSpecies(",
        "chosenSpeciesId()", "speciesChoiceAllowed(",
        "acceptSpeciesPicker(", "cancelSpeciesPicker()",
        "kEditableRows[static_cast<size_t>(fieldRow_)].field!=Field::Species",
        "current->form()!=0",
        "source.info->growthRate!=target.info->growthRate")
require(surface,"kGen5SpeciesTouchBase=78600",
        "case Surface::SpeciesPicker:",
        "drawSpeciesPicker(screen,fb,state)",
        "Utils::promptText(", "Filter national Dex names",
        "Names::getSpeciesName(id)",
        "state.acceptSpeciesPicker(error)",
        "state.cancelSpeciesPicker()",
        "Rename first if unnicknamed",
        "Only compatible original base forms, growth groups, gender and abilities")
require(native_stage,
        "Name this Pokemon before changing species",
        "(originalDecoded[0x40]>>3)!=0")
require(native_session,"originalBaseFormForSpecies() const",
        "source.form()==0")
require(native_screen,"draft_.originalBaseFormForSpecies()")
require(read("tests/test_gen5_pk5_readonly.cpp"),
        'pk->stageName(true,"DEO",&error)',
        'Field::Species,0,150,&error')
require(read("tests/test_gen5_readonly_save.cpp"),
        "assert(ui.speciesChoiceCount()==649);",
        'ui.filterSpecies("rAi"',
        "assert(ui.speciesChoiceCount()==1);",
        "ui.cancelSpeciesPicker();",
        "assert(ui.acceptSpeciesPicker(error));")
# The 25 exact native Nature IDs have named UI choices and a reversible
# separate picker; the A action cannot commit a source save.
require(native_screen,"Surface::NaturePicker",
        "std::array<std::string_view,25> kNatureNames",
        "openNaturePicker(", "moveNatureCursor(",
        "acceptNaturePicker(", "cancelNaturePicker()")
require(surface,"kGen5NatureTouchBase=79500",
        "case Surface::NaturePicker:",
        "drawNaturePicker(screen,fb,state)",
        "state.acceptNaturePicker(error)",
        "state.cancelNaturePicker()",
        "ScreenState::natureName(current->nature())")
require(read("tests/test_gen5_readonly_save.cpp"),
        'natureName(12)=="Serious"',
        "assert(ui.openNaturePicker(error));",
        "assert(ui.natureSelection()==24);",
        "assert(ui.acceptNaturePicker(error));")
# Box-only Clone and Remove live exclusively in the app-owned staged
# workspace, require the exact source slot, and are included in independent
# SAV5 review/reparse. Y, never A, stages destructive boxed removal.
box_workspace=read("include/Integration/Gen5/Gen5StagedPokemonWorkspace.h")
require(box_workspace,
        "enum class ChangeKind : uint8_t { FieldEdit, CloneBox, RemoveBox, CreateBox }",
        "cloneToEmptyBox(Slot from,size_t box,size_t slot",
        "removeBox(size_t box,size_t slot",
        "undoBoxReplacement(size_t box,size_t slot)",
        "firstEmptyCloneDestination() const",
        "target->originalEncryptedBytes().size()!=Crypto::StoredSize",
        "decoded.resize(Crypto::StoredSize)",
        "boxReplacements_.emplace(to,std::move(replacement))",
        "pending_.clear();boxReplacements_.clear();")
require(native_review,"ChangeKind::FieldEdit",
        "Field::CloneBox","Field::RemoveBox",
        "Crypto::encryptCandidate(d)!=change.replacementOriginal",
        "Gen V removal must blank exactly one occupied box PK5")
require(candidate,
        "Gen V export clone source identity is not canonical",
        "Gen V clone source differs from immutable SAV5",
        "Gen5StagedPokemonWorkspace::ChangeKind::RemoveBox",
        "Gen V export candidate did not preserve exact staged PK5 bytes")
require(native_bridge,"append(Shared::Action::Clone)",
        "append(Shared::Action::Remove)",
        "workspace.originalOccupied(slot.location)")
require(native_screen,"Surface::CloneDestination",
        "Surface::ConfirmRemoveBox",
        "confirmClone(Workspace& workspace",
        "confirmRemoveBox(Workspace& workspace",
        "workspace.cloneToEmptyBox(target_",
        "workspace.removeBox(target_.box,target_.slot",
        "workspace=std::move(snapshot)")
require(surface,"kGen5CloneTouchBase=80000",
        "case Surface::CloneDestination:",
        "case Surface::ConfirmRemoveBox:",
        "state.confirmClone(staged",
        "state.confirmRemoveBox(staged",
        "drawCloneDestination(screen,fb,state,staged)",
        '"Y","Stage Remove"',
        "Box clone/remove staged only; no source writes")
require(read("tests/test_gen5_staged_save_candidate.cpp"),
        "boxOps.cloneToEmptyBox(",
        "boxOps.removeBox(",
        "boxOps.undoBoxReplacement(",
        "verifyStagedReview(change,&error)",
        "operations->changedRecords==2",
        "boxClone.cloneToEmptyBox({Region::Box,0,0},0,2",
        "ghostOps.emptyCloneDestination(0,1)",
        "boxClone.undoBoxReplacement(0,2)",
        "buildStagedSaveCandidateForObservedSource(boxClone,stale")
require(read("tests/test_gen5_readonly_save.cpp"),
        "actionUi.activate(ops,Action::Clone,error)",
        "actionUi.activate(ops,Action::Remove,error)",
        "actionUi.confirmClone(ops,",
        "actionUi.confirmRemoveBox(ops,")
# Native text editing stays on the strict PK5 writer whitelist.
native_text=read("include/Integration/Gen5/Gen5NativeText.h")
require(native_text,"asciiName5(","encodeAsciiName5(",
        "nameCapacity5(","0x48","0x68","c>0x7E",
        "0x80000000u")
require(native_stage,"stageName(bool nickname,std::string_view name",
        "encodeAsciiName5(decoded,nickname,name)",
        "Gen V native name failed strict PK5 validation")
require(native_session,"stageName(Field field,std::string_view name",
        "stagePartyName(","stageBoxName(","replayName(true)")
require(native_review,"candidate->stageName(nickname,*name,error)",
        "Field::NicknameText","Field::OriginalTrainerText")
require(native_screen,"std::array<EditableRow,43>",
        '{Field::Nickname,0,"Nickname",10}',
        '{Field::OriginalTrainer,0,"OT Name",7}',
        "stageNameValue(")
require(native_bridge,"stageSharedName(",
        "StagedPokemon5Record::Field::NicknameText",
        "StagedPokemon5Record::Field::OriginalTrainerText")
require(surface,"Utils::promptText(","state.stageNameValue(",
        "Native PK5 text staged; original SAV5 unchanged")
require(read("tests/test_gen5_pk5_readonly.cpp"),
        'stageName(true,"PIKA NX"', 'stageName(false,"ASH"',
        'stageName(true,"Pikaché"')
require(read("tests/test_gen5_readonly_save.cpp"),
        'ui.stageNameValue("NXPIKA",error)',
        'ui.stageNameValue("NXT",error)')
view=read("include/Pokemon/Pokemon5ReadOnlyView.h")
require(view,"const PokeVault::Integration::Gen5::Personal5* nativePersonal()",
        "sourceGroup_==Enums::GameVersion::B2W2",
        "levelForExperience5(", "experienceForLevel5(",
        "uint8_t baseHP() const noexcept override",
        "p?p->hp:0", "p?p->spd:0")
require(read("tests/test_gen5_readonly_save.cpp"),
        "presented->boxes[0][0]->level()==1",
        "presented->boxes[0][0]->baseHP()==55",
        "presented->boxes[0][0]->statHPMax()==0")
print("Gen V read-only shared UI guard contracts: PASS")
