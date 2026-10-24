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
# These are used by the Create contracts before the field contracts below.
native_bridge = read("include/Integration/Gen5/Gen5SharedEditorBridge.h")
native_screen = read("include/Integration/Gen5/Gen5SharedScreenState.h")

require(
    surface,
    "Gen5EditorProvider::isGen5NdsId(screen.sourceGameId)",
    "if(!isGen5Source(screen))return false;",
    "trainer.sharedScreen()",
    "trainer.sharedScreen().openActions",
    "state.activateSelected(staged,error)",
    "trainer.sharedScreen().keepWithPresentation(",
    "state.discardDraft()",
    "state.requestExit(staged)",
    "state.confirmDiscardAndExit(staged)",
    "state.discardAllWithPresentation(staged,",
    "state.moveReviewPage(1,staged,sharedReviewRows(screen,staged).size())",
    "state.moveReviewPage(-1,staged,sharedReviewRows(screen,staged).size())",
    "verifyStagedReview(c,&error)",
    "for(const auto& field:verified->fields)",
    "reviewedFieldLabel(field)",
    "SharedEditorChrome::drawReviewPanel(",
    "SharedEditorChrome::drawReviewDiscardConfirm(",
    "screen.drawGSCOverlay(fb)",
    "screen.details.active=false;",
    "drawSharedWorkspace(screen,fb)",
    "return true;",
    "All inherited generic storage/bank/source mutation routes stay barred.",
    "screen.touchedButtonId(touch);",
)
# Passive PK5 View must not invoke the legacy all-generation legality report.
# Unknown/unsupported Gen V history is unresolved, not hard-invalid.
assert "Gen5SharedReview.h" in surface
assert "verifyStagedReview(" in read("include/Integration/Gen5/Gen5SharedReview.h")
assert "Legality::analyze(" not in surface
assert surface.index("screen.details.active=false;") < surface.index("drawSharedWorkspace(screen,fb);return true;")
# Hardware failure 35cbf163: View/Create/Edit run through the SAME shared editor
# surface code path as Gen IV (SharedPokemonEditorSurface.inc), not a Gen V copy.
shared_surface = read("src/UI/SharedPokemonEditorSurface.inc")
require(shared_surface, "SharedPokemonShell::drawChrome(", "SharedPokemonShell::drawPortrait(",
        "SharedPokemonShell::drawScrollableDetails(", "SharedPokemonShell::drawDataAndGraph(",
        '"DETAILS"', '"STATS"', '"MOVES"', "Workspace::navigate(A::generation",
        "SharedSpeciesPicker::drawContent(", "SharedHeldItemPickerPresentation::drawGrid(",
        "PokeBank::UIModel::NaturePickerLayout::forChoices(", "drawMoveEditor<A>(")
require(surface, '#include "SharedPokemonEditorSurface.inc"',
        "EditorSurface::handleWorkspace<Gen5Editor>(screen, state, keys, touch);",
        "EditorSurface::drawWorkspace<Gen5Editor>(screen, fb, state);",
        "PokeVault::Integration::Gen5::displayStats5(p, b2w2(screen));",
        "bool beginSharedCreate(TrainerViewScreen& screen,std::string& error)",
        "state.beginCreateDraft(ws,*request,", "state.discardCreateDraft(",
        "drawNarrowPokemonActions(fb,screen.touchButtons,model,state.actionRow(),title,")
require(read("src/UI/Gen4SharedPokemonSurface.inc"), '#include "SharedPokemonEditorSurface.inc"',
        "Surface::handleWorkspace<Gen4Editor>(screen, state, down, touch);")
# The retired Gen V-only editor, pickers and focus mutations are gone: moving
# focus never changes a value (no Left/Right "Adjust" on a field).
for gone in ("drawWorkspace5", "drawMovePicker(", "drawLocationPicker(", "drawNaturePicker(",
             "drawSpeciesPicker(", "drawIdentityPicker(", "adjustField(", "stepWithinPanel5",
             "switchPanel5", "Gen5WorkspaceLayout", '"Left/Right","Adjust"', "kGen5FieldTouchBase"):
    assert gone not in surface, f"retired Gen V-only editor UI still present: {gone}"
for gone in ("adjustField(", "openMovePicker(", "openLocationPicker(", "openIdentityPicker(",
             "openSpeciesPicker(", "openNaturePicker(", "kEditableRows", "fieldRow_"):
    assert gone not in native_screen, f"retired Gen V-only picker model still present: {gone}"
assert not (ROOT / "include/Integration/Gen5/Gen5WorkspaceLayout.h").exists()
for gone in ("promptNativeBoxCreate", "Add Pokemon - PID lower 16 bits", "drawVerifiedSummary",
             "Staged PK5 Fields", "kGen5DecreaseTouchBase"):
    assert gone not in surface, f"retired Gen V UI still present: {gone}"
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
)
# Hardware failure fa9cfcea: Gen V must open on the SAME loaded-game hub as
# Gen I-IV, never forced into a Party detail view.
gen5_open = manager.split("bool UIManager::handleGen5View(", 1)[1].split("bool UIManager::handleGen4View(", 1)[0]
assert "detailViewActive=true" not in gen5_open
assert "selectedMode=TrainerViewScreen::ViewMode::Party" not in gen5_open
hub = surface.split("if(state.surface()==Surface::Browse && !screen.detailViewActive) {", 1)[1]
hub = hub.split("if(state.surface()==Surface::Actions) {", 1)[0]
require(hub, "if(screen.helpOverlayActive)return false;",
        "G::routeHub5(screen.homeMenuIndex,",
        "case G::HubCommand5::PassToBase:return false;",
        "case G::HubCommand5::StorageRefused:",
        "trainer.trainerScreen().enter(staged,error)")
# Hub routing runs before any Gen V touch decoding of box/party ids.
assert surface.index("G::routeHub5(") < surface.index("decodeGen5BoxTouch(")
# B inside a panel returns to the hub; the separate Trainer pop-up is gone.
require(surface, "screen.detailViewActive=false; // back to the hub, tile still focused",
        "editor.leave(); // focused row is remembered for the next visit",
        "if(!screen.detailViewActive) {\n        // The same loaded-game hub as Gen I-IV, drawn by the base renderer.")
assert "Trainer - Generation V" not in surface
assert '{"X/Y","Party/Trainer"}' not in surface
require(manager_header, "bool handleGen5View(")
require(
    games,
    "if (selected.sourceKind == SelectedSourceKind::Gen5AssignedFile)",
    "openGen5Setup(selectedGameId,",
    "discoverGen5Candidates();",
    "PokeVault::Integration::Gen5::assignValidatedSource(",
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
        "selectedGen5Copy=gen5CopyForVariant(chosen.sourceVariant);",
        "selectedGen5Copy=", "gen5CandidateScroll=0;")
# The copy mapping lives in one helper shared by the picker and the direct
# assigned open (NEW-16); it must still map both exact variants.
require(selector_header, 'variant == "gen5-copy-0-primary") return SaveCopySelection::Primary;',
        'variant == "gen5-copy-1-backup") return SaveCopySelection::Backup;')
# NEW-16 / C01: Home A (and Items) open the validated assigned save directly;
# anything not ready falls back to the explicit Save Instances picker.
require(games, "if (openAssignedGen5Directly(*user, selected, selectedGameId)) return;",
        "if (openAssignedGen5Directly(*user, title, title.gameId)) return;",
        "if (!opened.ready()) {",
        "selectedGen5Fingerprint = opened.instance.contentFingerprint;")
require(selector_header, "getSelectedGen5Copy() const",
        "SaveCopySelection selectedGen5Copy")
require(ui_header, "SaveCopySelection selectedCopy")
require(manager, "selectScreen.getSelectedGen5Copy()",
        "legacySourceBindings,profileIdentity(userUid),gameId,selectedCopy")
# Assigned Gen V paths are shown as LINKED even when two independent
# checksum-valid retail copies require explicit re-selection on every Open.
require(games, '"REMEMBERED / CHOOSE COPY"',
        '"This save is already linked to another profile or game."')

# Gen V must match Gen I-IV's explicit active-profile linking:
# Save Source menu -> Assign Save to This Profile -> validated chooser A.
# Opening a game by A is distinct. A metadata-only assignment never starts
# the editor, and manual source selection uses the same shared bindings.
require(selector_header, "bool gen5AssignOnly = true;")
require(games,
        '"Assign Save to This Profile"',
        '"Browse Saves / Open Shared Editor"',
        '"Assign Save File Manually"',
        'gen5AssignOnly=true; // Source/Game File defaults to metadata-only profile link.',
        'gen5AssignOnly=false; // Explicit A on a game requests the shared editor.',
        'if(gen5SetupIndex==0) {',
        'gen5AssignOnly=true;',
        'if(gen5SetupIndex==1) {',
        'gen5AssignOnly=false;',
        'if(gen5AssignOnly) {',
        'titleSelected=false;',
        'selectedGen5Fingerprint.clear();',
        '"Gen V save assigned to "+selectedUser->name',
        'overlay=Overlay::GamesDrawer;',
        'gen5AssignOnly?"Assign to Profile":"Open Editor"',
        'PokeVault::Integration::Gen5::assignValidatedSource(')
assign_only_body=games.split("if(gen5AssignOnly) {",1)[1].split("selectedUserUid=selectedUser->uid;",1)[0]
assert "titleSelected=true;" not in assign_only_body
assert "selectedGen5Fingerprint=fresh.instance.contentFingerprint;" not in assign_only_body

# A file with two different checksum-valid copies cannot open through an
# automatic persisted assignment that does not record which copy was chosen.
require(games, '"REMEMBERED / CHOOSE COPY"', "SaveCopySelection::Automatic",
        "const bool separateGen5Copy =",
        "instance.generation==5 && !instance.sourceVariant.empty()",
        "if (!separateGen5Copy &&")


# Profile bindings must remain stable across normal emulator-save writes on
# FAT/exFAT devices (st_ino==0), matching Gen IV. The selected content SHA
# remains separately mandatory before opening the editor.
require(read("include/Integration/Gen5/Gen5SourceDiscovery.h"),
        "persistentGen5SourceIdentity(",
        "the changing byte fingerprint")
require(discovery, "persistentGen5SourceIdentity(",
        "if(inode!=0)", "gen5-physical-v2:")
require(read("tests/test_gen5_readonly_save.cpp"),
        "persistedGen5FATIdentityContracts()",
        "assert(oldSave==emulatorSavedAgain)")
assert "pathIdentity(path,&st,row.contentFingerprint)" not in discovery
# No-inode FAT aliases must preserve one source/profile claim, and manual
# parent-path tricks must not escape that identity rule.
require(discovery, "hasDotPathComponent(path)",
        "unsupported source path with dot or parent segments",
        "std::tolower(static_cast<unsigned char>(out[0]))")
require(read("tests/test_gen5_readonly_save.cpp"),
        '"SDMC:/Switch/Drastic/User/Backup/POKEMON BLACK.DSV"',
        "root+\"/junk/../black_rejected.srm\"")

# A selected physical source is also an EXACT CONTENT SNAPSHOT. Inode and
# path equivalence alone cannot exclude an in-place emulator save rewrite.
# Thread the selected normalized fingerprint from the candidate through
# UIManager to the final twice-validated read-only source opener.
assigned_h = read("include/Integration/Gen5/Gen5AssignedSource.h")
assigned_cpp = read("src/Integration/Gen5/Gen5AssignedSource.cpp")
# Profile visibility and the persisted binding are enforced once, in the
# shared assignment helper that the Games UI calls.
require(assigned_cpp, "Source::visibleToProfile(checked,profile)",
        "bindings.replaceFileAssignmentAndSave(")
require(selector_header, "getSelectedGen5Fingerprint() const",
        "std::string selectedGen5Fingerprint;")
require(games, "selectedGen5Fingerprint.clear();",
        "selectedGen5Fingerprint=assigned.instance.contentFingerprint;")
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
assert "gen5SetupIndex==3)overlay=Overlay::Gen5UnassignConfirm;" in gen5_setup
assert "gen5SetupIndex==3)unassignCurrentGen5Game()" not in gen5_setup
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
    "drawGen5SourceNotice(fb,gen5Notice",
)
assert 'No valid Gen V cartridge saves found; use a manual file' not in games

# Native NRO packaging must not erase region scenery on the integration branch.
# Gen V retail covers must resolve just like Gen I-IV, and preflight must reject
# a reintroduced missing-region/missing-cover regression before emitting an NRO.
workflow = read(".github/workflows/native-pr-build.yml")
makefile = read("Makefile")
identity = read("src/Games/GameIdentity.cpp")
asset_preflight = read("tools/check_device_assets.py")
romfs_stage = read("tools/prepare_release_romfs.sh")
# The snapshot only fills gaps; branch-owned artwork is never replaced.
require(workflow, "tools/prepare_release_romfs.sh recovery-work/romfs",
        "python3 tools/verify_nro_romfs.py PokeBankNX.nro")
require(romfs_stage, '[ -e "romfs/$f" ] || cp -p')
assert "rm -rf romfs" not in workflow
for game in ("black_nds", "white_nds", "black2_nds", "white2_nds"):
    require(identity, f'if (id == "{game}") return "romfs:/game_cards/{game}.png";')
    require(makefile, f'$(GAME_CARD_ART_SOURCE)/{game}.png')
    require(asset_preflight, f'"{game}.png"')
require(asset_preflight, 'REGION_BACKDROP_DIR', 'all 10 regional backdrops are packaged',
        'TRAINER_PORTRAIT_DIR', 'from fetch_product_art import TRAINER_URLS, REQUIRED_REGIONS')
require(romfs_stage, 'python3 tools/fetch_product_art.py')
for game in ("black_nds", "white_nds", "black2_nds", "white2_nds"):
    require(romfs_stage, game)

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
        "test_gen5_create_integration test_gen5_box_touch test_gen5_create_input_rules ",
        "test_gen5_create_integration_sanitize test_gen5_box_touch_sanitize test_gen5_create_input_rules_sanitize ",
        "test_gen5_bag_screen test_gen5_party_box_parity test_gen5_trainer test_gen5_source_e2e test_gen5_export_validation test_gen5_save_diagnostics test_gen5_pid_coordination; do",
        "test_gen5_trainer_sanitize test_gen5_source_e2e_sanitize test_gen5_export_validation_sanitize test_gen5_save_diagnostics_sanitize test_gen5_pid_coordination_sanitize; do")
# One integrated Gen I-V candidate with packaged RomFS verification.
require(read(".github/workflows/gen15-integrated-candidate.yml"),
        "python3 tools/verify_nro_romfs.py PokeBankNX.nro --list", "DEVICE_ACCEPTED=false",
        "STATUS=NOT DEVICE TESTED", "devkitpro/devkita64@sha256:1fc388c3a0d34bd2045a6dadcb1020e069d5f876a187fd705de14b4440c00282")

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
        'footer.yLabel="Undo This Slot"')
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
# focus changes. The Home card shows the CURRENT validated snapshot (same
# wording as Gen I-IV), refreshed only when the save's size/mtime or the
# assignment changes. The snapshot cannot authorize opening an editor.
selector_header=read("include/UI/SaveSelectScreen.h")
require(selector_header,
        "PokeVault::Source::HomeSnapshotCache homeSnapshots;",
        "bool linkedSave = false;")
assert "gen5Preview" not in selector_header
require(games,
        "PokeVault::Integration::Gen5::homeCardSnapshot(",
        "homeSnapshots.refresh(request, statSaveFile,",
        "applySnapshotToHub(*snapshot);")
assert "cached party" not in games
fast=games.split("void SaveSelectScreen::refreshHubSelectionFromCache()",1)[1]
fast=fast.split("void SaveSelectScreen::refreshHubPreview(bool resolveLaunchTarget)",1)[0]
assert "homeSnapshots.find(" in fast
assert "Gen5::openAssignedSource(" not in fast
full=games.split("void SaveSelectScreen::refreshHubPreview(bool resolveLaunchTarget)",1)[1]
full=full.split("void SaveSelectScreen::begin",1)[0] if "void SaveSelectScreen::begin" in full else full
assert "homeSnapshots.find(homeSnapshotKey(currentProfileIdentity(), title.gameId))" in full
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
# Shared Create builds its native defaults through the same validators.
require(read("include/Integration/Gen5/Gen5SharedScreenState.h"),
        "createLanguageAllowed5(trainer.language)",
        "if(const auto checked=validateCreate5(r);!checked)",
        "r.gender=createGenderForPID5(personal.info->genderRatio,pid);")
require(read("Makefile.host.gen5"),
        "GEN5_CREATE_INPUT_TEST :=","GEN5_CREATE_INPUT_SANITIZE :=")
require(read(".github/workflows/gen5-focused-host.yml"),
        "test_gen5_box_touch test_gen5_create_input_rules ",
        "test_gen5_box_touch_sanitize test_gen5_create_input_rules_sanitize ")
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
        "Utils::promptNumber(",
        "Utils::promptText(",
        "else if(state.surface()==Surface::ConfirmCreateBox &&",
        "!beginSharedCreate(screen,error)) {",
        "case Surface::ConfirmCreateBox:",
        "state.cancelBoxAction()",
        "ScreenState::sharedCreateDefaults(")
require(native_screen, "beginCreateDraft(Workspace& ws", "discardCreateDraft(Workspace& ws",
        "if(createPending_)return; // use discardCreateDraft",
        "if(surface_==Surface::Edit && createPending_) {")
require(read("tests/test_gen5_party_box_parity.cpp"), "void sharedCreateRoute(",
        "ui.beginCreateDraft(ws, *defaults, ok, error)", "ui.discardCreateDraft(ws, ok, error)")
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
        "Choose a validated Gen V Save Instance; source stays read-only.",
        '"Choose a validated Gen V Save Instance."',
        "Open shared staged editor. Original save stays unchanged.",
        '"A",gen5AssignOnly?"Assign to Profile":"Open Editor"',
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
require(shared_contract, "case FieldIdentity::HeldItem:",
        "case FieldIdentity::Ball:", "case FieldIdentity::MetLevel:")


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
require(shared_contract, "case FieldIdentity::FatefulEncounter:",
        "case FieldIdentity::TrainerId:",
        "case FieldIdentity::SecretId:")
require(surface,
         "Names::getMetLocationDisplayName(",
        "Utils::promptNumber(",

        )

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
require(surface,




         "personal5(")
require(shared_contract, "case FieldIdentity::Form:",
        "case FieldIdentity::Gender:", "case FieldIdentity::Ability:")
require(read("tests/test_gen5_pk5_readonly.cpp"),
        "G5::NativePersonalBW.size()==668", "G5::NativePersonalB2W2.size()==709",
        "G5::personal5(386,1,false)", "Field::AbilitySlot")

# Native Gen V move data is pinned to PKHeX MoveInfo5 (560 PP values).
# The controller/touch picker is a sub-surface of the same guarded Edit draft,
# using pinned species/game learnsets. It is NOT a full legality verdict and
# never performs a generic source-save write.
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

# Touch-to-Edit stays on the verified native PK5 field transaction
# model. The shared Add action uses explicit prompts and a staged transaction;
# inherited source-save, Storage/Bank and generic mutation routes stay barred.
# Touch IDs and hitboxes are generated by the actual drawn modal rows.
require(surface, "kGen5ActionTouchBase=77000",

        "kGen5PartyTouchBase=77400",
        "screen.touchedButtonId(touch)",
        "screen.touchButtons.push_back(",
        "down|=HidNpadButton_A;",
        "screen.selectedPartyIndex=tap-kGen5PartyTouchBase;",



        )
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
        "::mkdir(finalPath.c_str(),0700)!=0", "unpublish(",
        "INJECT_SAVE=DISABLED", "EMULATOR_SAVE_WRITE=DISABLED")
# QA-F3: publication never renames a directory onto the destination.
assert "::rename(temporaryPath.c_str(),finalPath.c_str())" not in working_cpp
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
        "readSourceFileReadOnly(",
        "request.observedSourceFile=observedFile;",
        "request.sourceContainer=container;",
        "publishGen5WorkingCopy(staged,request)",
        "state.finishExport();",
        "WorkingCopyExportRequest request;",
        "accountUidHex(screen.userUid)",
        "request.observedSourceBytes=observed;",
        "Export Verified Generation V Working Copy?",
        "SharedEditorChrome::drawReviewPanel(",
        "original SAV5 unchanged")
require(read("include/UI/SharedReviewPresentation.h"),'out.push("X","Export Copy")')
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
require(native_bridge,"NativeField::Level,stat,value,error",
        "NativeField::Experience,stat,value,error")
require(native_session,"Field::Experience","experienceForLevel5(")
require(read("include/Integration/Gen5/Gen5SharedReview.h"),"Field::Experience",
        "experienceForLevel5(")
require(surface,
        "experienceForLevel5(")
# Species changes must use exact native personal data and never reroll PID.
require(native_stage,"case Field::Species:","value>649",
        "target.info->growthRate!=personal.info->growthRate",
        "genderAllowed5(target.info->genderRatio,gender)",
        "candidate[0x15]=ability;",
        "field==Field::Species || field==Field::Nature")
require(native_session,"apply(Field::Species,0,after.species())")
require(native_review,"Field::Species,0,before.species(),after.species()")
require(native_bridge,"NativeField::Species,stat,value,error")
require(surface,
        "case Field::Species:name")
require(read("tests/test_gen5_pk5_readonly.cpp"),
        "Field::Species,0,26","Field::Species,0,32")
require(read("tests/test_gen5_readonly_save.cpp"),

        "localUiWorkspace.viewParty(0)->species()==26")
# Duplicate switch labels are an immediate native C++ compile failure.
gen5_access=shared_contract.split("if (generation == Generation::Gen5) {",1)[1]
gen5_access=gen5_access.split("return FieldAccess::Hidden;\n}",1)[0]
assert gen5_access.count("case FieldIdentity::Nickname:")==1
assert gen5_access.count("case FieldIdentity::Species:")==1

# Species name browsing is a modal, not a write path. Search/cancel
# cannot alter a draft; A uses the same guarded native species transaction.
require(native_screen,





        "current->form()!=0",
        "source.info->growthRate!=target.info->growthRate")
require(surface,


        "Utils::promptText(",




        )
require(native_stage,
        "Name this Pokemon before changing species",
        "(originalDecoded[0x40]>>3)!=0")
require(native_session,"originalBaseFormForSpecies() const",
        "source.form()==0")
require(native_screen,"draft_.originalBaseFormForSpecies()")
require(read("tests/test_gen5_pk5_readonly.cpp"),
        'pk->stageName(true,"DEO",&error)',
        'Field::Species,0,150,&error')
# The 25 exact native Nature IDs have named UI choices and a reversible
# separate picker; the A action cannot commit a source save.
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
require(surface,"void stageCloneOrMoveByKeypad(TrainerViewScreen& screen)",
        '"Clone to Gen V Box"', '"Move to Gen V Box"',
        "case Surface::ConfirmRemoveBox:",
        "!state.confirmClone(staged,",
        "state.confirmRemoveBox(staged",
        '"Release this Pokemon?"', '"Y","Confirm Release"',
        "SharedEditorChrome::drawReviewPanel(")
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
require(native_bridge,"stageSharedName(",
        "StagedPokemon5Record::Field::NicknameText",
        "StagedPokemon5Record::Field::OriginalTrainerText")
require(surface,"Utils::promptText(",
        )
require(read("tests/test_gen5_pk5_readonly.cpp"),
        'stageName(true,"PIKA NX"', 'stageName(false,"ASH"',
        'stageName(true,"Pikaché"')
view=read("include/Pokemon/Pokemon5ReadOnlyView.h")
require(view,"const PokeVault::Integration::Gen5::Personal5* nativePersonal()",
        "sourceGroup_==Enums::GameVersion::B2W2",
        "levelForExperience5(", "experienceForLevel5(",
        "uint8_t baseHP() const noexcept override",
        "p?p->hp:0", "p?p->spd:0")
require(read("tests/test_gen5_readonly_save.cpp"),
        "presented->boxes[0][0]->level()==1",
        "presented->boxes[0][0]->baseHP()==55",
        "presented->boxes[0][0]->statHPMax()>0")

# Assignment is an executable backend transaction shared with production UI.
require(assigned_cpp, "SourceAssignmentResult assignValidatedSource(",
        "reopenValidatedSource(selected)", "binding.sourceVariant=checked.sourceVariant;",
        "binding.physicalIdentity=checked.physicalIdentity;", "copy=persisted;")
require(games, "captureSourceFocus(", "restoreSourceFocus(",
        "remembered.binding.sourcePath,240", "gen5Notice=assigned.diagnostic;")
require(read("tests/test_gen5_readonly_save.cpp"),
        "persistentCopyAssignmentContracts();", "sourceSelectionFocusContracts();")

require(games, "void SaveSelectScreen::closeGen5Setup()", "gen5SetupReturnCandidates=sameTarget;")
require(games, "instance.generation==5 && instance.rememberedSource", '"REMEMBERED / "')
require(games, "focusedGen5Game=focused.gameId;", "user->titles[i].gameId==focusedGen5Game")
# Shared one-read copy enumeration backs discovery and the manual chooser.
require(discovery, "inspectSourceChoices(const std::string& path", "state.stopAt=", "remainingRoots",
        "state.result.rootsExamined", "s.result.unreadableDirectories")
require(games,"PokeVault::Integration::Gen5::inspectSourceChoices(",
        "gen5ProfileFocus.remember(","gen5ProfileFocus.gameFor(","drawGen5SourceNotice(fb,gen5Notice",
        "rows.push_back(choice);", "Some emulator folders could not be read.")
require(read("tests/test_gen5_readonly_save.cpp"), "boundedProviderDiscoveryContracts();",
        "snapshotChoiceAndRememberedAliasContracts();", "gen5ProfileFocusAndSdOwnershipContracts();")
# Gen V Bag uses the SAME shared Items panel and inventory contract as Gen
# III/IV: A quantity, X Add, Y Remove (confirmed), L/R pockets, + Review.
items_panel = read("src/UI/Panels/ItemsPanel.cpp")
bag_model = read("include/Integration/Gen5/Gen5BagScreen.h")
require(surface, "return handleBag(screen,keys,tap,touch);", "bag.stepRow(-1,ws)", "bag.stepPage(1,", "bag.beginQuantity(ws,error)",
        "bag.openAdd(ws,error)", "bag.requestRemove(ws,error)", "bag.confirmRemove(ws,refresh,error)",
        "bag.pickAdd(ws,gen5PolicyRule(screen,bag.pocket(),id),", "state.openReviewFromBrowse(ws)",
        "CategoryTouch::PreviousId", "bag.tapRow(static_cast<size_t>(tap),ws)",
        "bag.tapAddChoice(", "drawBag(screen,fb);", "ws.bagReviewChanges()",
        "case G::HubTile5::Items:")
assert '{"Y","Bag"}' not in surface
assert surface.index("return handleBag(screen,keys,tap,touch);") < surface.index("switch(state.surface()) {\n        case Surface::Actions:")
require(items_panel, "Gen5::bagPocketName5(", "Native Generation V bag data failed validation.")
require(bag_model, "PokeBank::UIModel::inventoryActionAvailability(", "Workspace snapshot=ws;",
        "return pocket<BagPocket5Count;", "maxCount>1", "KeyItemAddWarning",
        "requestAddWarning", "needsKeyItemWarning(rule)",
        '"Confirm the Key Item warning before staging this item"')
require(surface,"case BagSurface::KeyItemAddWarning:","bag.requestAddWarning(ws,rule)",
        "SharedBagAddPicker::drawKeyItemWarning(")
assert "Key Items/TMs view only" not in surface
require(trainer, "staged_.bagPresentation()")
assert "writeFile" not in bag_model and "fopen" not in bag_model
require(read("tests/test_gen5_bag_screen.cpp"), "unavailableBag();", "fullPocketRefusesAdd();")

# Gen IV parity: Clone and Move Pokemon use the Gen IV box / slot keypads
# over the verified empty-destination check; origin dates are View-only (no dynamic_cast under -fno-rtti).
require(surface, "state.moveDestination()", '"Move to Slot"',
        )
assert "dynamic_cast" not in surface
require(native_bridge, "append(Shared::Action::Move);")
require(native_screen, "workspace.moveToEmptyBox(target_,")
require(read("tests/test_gen5_party_box_parity.cpp"), "Action::Move", "originSummary5(")

# Gen V Trainer: only cited Name/Money/Play Time stage; other rows read-only.
trainer_model = read("include/Integration/Gen5/Gen5Trainer.h")
require(trainer_model, "6501f0ab46e8f8ca048539dbaf8cae8cb104e722", "PlayerBlockIndex = 27",
        "MiscBlockIndex = 52", "MaxMoney = 9999999", "kPlayerRanges", "kMiscRanges")
require(surface, "return handleTrainerEditor(screen,keys,tap);", "editor.applyName(ws,r.text,refresh,error)",
        "editor.applyMoney(", "editor.applyPlayTime(", "drawTrainerEditor(screen,fb);",
        "ws.trainerReviewChanges()", "for(const TRow row:{TRow::Name,TRow::Money,TRow::PlayTime,TRow::Pokedex})",
        "for(const TRow row:{TRow::Gender,TRow::TrainerId,TRow::SecretId,TRow::Badges})")
require(read("include/Integration/Gen5/Gen5StagedSaveCandidate.h"), "Gen5StagedTrainer::kPlayerRanges",
        "did not preserve the exact staged trainer")
require(read("src/Integration/Gen5/Gen5WorkingCopyExport.cpp"), "TRAINER_CHANGED=")

# Product Home Items reaches the Gen V Bag through the explicit Save Instance
# chooser (no path guessing), and opens the same shared session on Items.
items_route = games[games.index("void SaveSelectScreen::selectCurrentTitleForItems()"):]
items_route = items_route[:items_route.index("SelectedSourceKind::Gen4AssignedFile")]
assert "not yet supported" not in items_route
require(items_route, "openGen5Setup(title.gameId,", "discoverGen5Candidates();", "gen5AssignOnly=false;")
require(manager, "intent!=SaveSelectScreen::OpenIntent::Items", "trainerScreen.openItemsShortcut();")
require(surface, "if(screen.itemsShortcutActive) {", "if(state.requestExit(ws))screen.goBack=true;")

# Gen V Pokedex is a read-only view of the cited Zukan5 block.
dex_model = read("include/Integration/Gen5/Gen5Pokedex.h")
require(dex_model, "Zukan5.cs", "CaughtOffset = 0x08", "SeenOffset = 0x5C", "No flag is ever written")
require(surface, "editor.openPokedex(ws,error)", "Pokedex - Generation V (read-only)")
assert "write" not in dex_model.split("namespace PokeVault")[1].lower().replace("never written", "")


# Hardware failure 35cbf163 repair: every Gen V field lives in the ONE shared
# editor field model (Gen5EditorFields.h + the Gen5Editor adapter), reached
# through the shared pickers, keypads and move editor; nothing is Gen V-only UI.
fields = read("include/Integration/Gen5/Gen5EditorFields.h")
require(fields, '"Species", "Nickname", "Level", "Experience", "OT", "Trainer ID", "SID",',
        '"Held Item", "Friendship", "Language", "Origin", "Ball",',
        '"Met Level", "Met Location", "Pokerus", "Form", "OT Gender", "Fateful"',
        "ItemStorage5.GetAllHeld()", "BagCatalog5::kUnreleased", "Master (1) .. Dream (25)",
        "NativeMetLocations5.begin()", "Gen5MoveCompatibility::selectableMoves(", "kMaxSpecies = 649",
        "probe.stage(Field::Form, 0, form, error)", "info.ability2 != info.ability1",
        "keepShinyForIds5(", "Field::Shiny", "earlierGenerationOrigin5(")
# Pin compatibility to actual Black/White and Black 2/White 2 tables and the
# guarded existing-source preservation path. An arbitrary move cannot be added
# merely because its ID is within the native 1..559 range.
compatibility = read("include/Integration/Gen5/Gen5MoveCompatibility.h")
require(compatibility, 'game=="black_nds"||game=="white_nds"',
        'game=="black2_nds"||game=="white2_nds"',
        "if(inSource)return Availability::Preserved;",
        "if(classify(game,species,form,move,false)!=Availability::Unavailable)",
        "if(formAvailable(game,species,form))",
        "if(const auto* row=formRow(species,form))",
        "if(priorForm(game,species,move))return Availability::FormHistory;",
        "std::sort(moves.begin()+1,moves.end());")
require(read("tests/test_gen5_move_compatibility.cpp"),
        "assert(!C::direct(game,25,0,459))",
        "assert(std::find(list.begin(),list.end(),459)==list.end())",
        "assert(C::classify(game,25,0,459,false)==C::Availability::Unavailable)")
assert "for (uint16_t move = 0; move <= MaxNativeMove5; ++move)" not in fields
require(surface, "status = \"Transfer\";", "status = \"Preserved\";")

require(surface, "Fields::detailEditable(draft(screen), row, b2w2(screen))",
        "Fields::pidRowEditable(draft(screen), row, b2w2(screen))",
        "PickerTarget::Species", "PickerTarget::HeldItem", "PickerTarget::Ball",
        "PickerTarget::MetLocation", "PickerTarget::Pokerus", "PickerTarget::Form",
        "PickerTarget::Nature", "PickerTarget::Ability", "Fields::heldItemChoices()",
        "Fields::metLocationChoices()", "Fields::ballChoices()",
        "Fields::moveChoices(screen.sourceGameId,p->species(),p->form(),p->moves())",
        'Utils::promptNumber("Level"', 'Utils::promptNumber("Experience"',
        'Utils::promptNumber("Friendship"', 'Utils::promptNumber("Met Level"',
        "d.stageName(nickname ? Field::NicknameText : Field::OriginalTrainerText",
        "Fields::stageTrainerId(probe, secret,", "Fields::toggleGender(probe, error)",
        "Fields::toggleShiny(probe, error)", "Fields::toggleFlag(d, row == FatefulEncounter",
        "backendState.speciesRefusal(species)", "backendState.recreateCreateSpecies(",
        "return b2w2(screen) ? Enums::GameVersion::B2W2 : Enums::GameVersion::BW;",
        "learnability not checked", 'return Names::getMetLocationDisplayName(origin, location);',
        "static constexpr bool kCompactForm = true;")  # species search: shared chooser, QA CG15
require(native_screen, "Gen5SharedPokemonSession& editDraft() noexcept",
        "std::string speciesRefusal(uint32_t species) const",
        "bool recreateCreateSpecies(Workspace& ws,uint32_t species,bool shiny,")
require(native_stage, "case Field::Shiny:", "case Field::Personality:", "withGender5(",
        "withShiny5(", "withAbilityBit5(")
require(native_session, "apply(Field::Personality,0,after.pid())")
require(native_review, "apply(Field::Personality,0,before.pid(),after.pid())")
require(read("tests/test_gen5_readonly_save.cpp"), "ui.editDraft().stage(GField::AbilitySlot,0,2,error)",
        'ui.editDraft().stageName(GField::NicknameText,"NXPIKA",error)', "ui.speciesRefusal(1)")
require(read("tests/test_gen5_party_box_parity.cpp"),
        'ui.recreateCreateSpecies(ws, 1, false, "Bulbasaur", "Pikachu", ok, error)')
# Glyph/overflow guard: every Gen V adapter caption drawn into a shared fixed-width
# panel is no longer (in characters) than the hardware-accepted Gen IV string in
# the same slot, and uses only glyphs Gen IV already draws (ASCII, — • é).
import re as _re
_gen4_surface = read("src/UI/Gen4SharedPokemonSurface.inc")
def _adapter_string(text, key):
    match = _re.search(key + r'\s*=\s*"([^"]*)"', text)
    assert match, f"missing adapter string {key}"
    return match.group(1)
for _key in ("kMovePickerSubtitle", "kMoveEditorSubtitle",
             "kHeldItemTitle", "kMovePickerTitle", "kDataTitle"):
    _gen5_text = _adapter_string(surface, _key)
    _gen4_text = _adapter_string(_gen4_surface, _key)
    assert len(_gen5_text) <= len(_gen4_text), f"{_key} longer than Gen IV: {_gen5_text!r}"
    assert all(ord(c) < 128 or c in "—•é" for c in _gen5_text), f"{_key} glyph: {_gen5_text!r}"
print("Gen V read-only shared UI guard contracts: PASS")
