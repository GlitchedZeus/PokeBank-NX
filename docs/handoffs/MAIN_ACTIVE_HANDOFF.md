# Main application: active handoff (Claude → Codex)

> **Rules:** read [`docs/ENGINEERING_PROTOCOL.md`](../ENGINEERING_PROTOCOL.md) first.
> **Source of truth:** GitHub. Resume from the SHA below, not from any chat summary.
> **Updating:** update this file at every significant checkpoint, and push it.

Last updated: 2026-10-22 UTC, by Codex (Round 4 takeover; owner-authorized application work).

## October 24 full-gate repair checkpoint

- Remote WIP `ba7e9df3cacfae67125d0dee31a9bc5a2d15764a` PASSED native
  compile/link/packaged RomFS run40073455301. Production is frozen since that SHA.
- GCC and Clang QA contracts +489-cell matrix passed (18 C1-C6 export paths,
  20 exact archived sessions and explicit differing internal Gen V copy choices).
  GCC final leak checks1 passed; Clang direct confirmation reruns both variants
  with the required POKEBANK_QA_GEN3_FIXTURE environment to record both summaries.
- Full normal gates found stale source assertions: old Trainer-card touch count
  rejected the new Review button; Gen II/III old local Review painter counts no
  longer followed shared Review/Discard. Updated to require the real shared paths,
  retain boxed Clone cells, locked-row no-touch and Y/B confirmation protections.
- Fresh sanitizer builds exposed a missing 3-argument getIconSprite overload in
  the ISOLATED host renderer mock. Added its no-assets backend stub; production
  SpriteManager is unchanged. Both full sanitizer gates resumed with leaks1.
- Safari Create cross-product now explicitly includes native Hyper Potion21,
  asserting its native label as well as Safari5. All5 held choices x12 balls x5
  Gen III titles are covered. This is a test extension, not another production change.
- Shared Gen IV hardware surface guard now follows both normal and docked search
  footers; targeted run passes. Full normal suites rerun; sanitizer suites compile.
- Do not integrate until ALL full gates finish green. Then preserve remote updates,
  push #307, build ONE official exact-head NRO, verify/bind/audit it, finish all CI.
  Report remains NOT DEVICE TESTED; historical27/6/4 and S06 NOT TESTED unchanged.

## October 24 validation checkpoint — production freeze

- Remote WIP verified at `1716f7f7f0441b9e0f875046828ea88308b04ac1`.
  Native smoke run 40073165857 is compiling it. #307 remains at92f95612.
- Trainer informational fields now use the EXISTING shared ReadOnly gold token,
  with an explicit card hint; no fake gender/time writer or selectable locked row.
- Gen V UI source guard now follows classify/formAvailable/formRow/priorForm;
  the actual new chooser covers these paths in behavioral tests. Touch viewport
  guards now follow two-column docked search width, retaining clipped hitbox checks.
- Source candidate guards are at0 failures. Owner regression70/196 passes;
  native PNG and assets/workflow regressions pass. Gen II DV reroll still preserves
  shiny/gender/Unown. Compact620 menu real-font geometry and inline Gen V Language pass.
- Full GCC/Clang normal + fresh-directory sanitizer suites and both QA matrices
  are running. Logs and resume state: project outputs/oct24-repairs. No hardware
  result changed. Do not integrate until all required gates pass, then build,
  verify, docs-bind and independently audit ONE official replacement candidate.

## October 24 form-aware compatibility and validation checkpoint

- Last remote WIP `18b76be5c7393c1f963d16ef1979f4d0a2892106` PASSED native
  build 40072465759 (compile/link/complete RomFS). Not the final hardware NRO.
- New Gen V form data covers 24 native form rows and all 23 multi-form species;
  BW versus B2W2 availability follows pinned PKHeX personal tables. Wormadam
  cloak, Rotom appliance and White/Black Kyurem are genuinely distinct. Added
  prior-form labels for retained Deoxys/Shaymin/Giratina moves, correct BW Snarl
  TM95 unavailability, and Smeargle's real Sketch exception. Existing unknown
  moves remain Preserved, never certified legal. Reproducible generator + exact
  hashes in docs/data/gen5-move-compatibility-sources.json.
- Focused form, docked controller/touch/search/sort/no-match, Help/event-year
  tests PASS normal AND ASan/UBSan with detect_leaks=1. Pikachu remains 55 BW /
  64 B2W2 direct moves. Shared candidate contracts report 0 failing; 26 real
  broken-path mutation controls PASS. Owner gate PASS all 70 Round 3 /196 guards.
  Old 900px pin now follows the OWNER's October24 compact620 request; no old
  hardware status is rewritten or inferred for these new dimensions.
- GCC QA contracts + all489 matrix cells passed after badge/backup changes.
  Rerun with final form data and leak checks enabled is in progress. Expanded
  backup tests explicitly distinguish the two differing native Gen V copies;
  UI uses existing glyph buttons to choose Primary / Backup / Cancel, with no
  live-source fallback or assignment changes. Exact file is rechecked afterward.
- Full GCC and Clang host builds are running from new campaign build directories.
  Full sanitizer variants, final native build, #307 integration, exact-head ONE
  official NRO + docs binding + independent audit are still required.
- Earlier generations' unsupported Gender/Play Time writers remain read-only.
  Native Switch historical previews remain immutable/read-only; Gen I-V emulator
  archives have full existing staged Review/Discard/Export. S06 NOT TESTED.

## October 24 Trainer / picker / touch checkpoint — WIP

- Pushed checkpoint before these edits: `80377153dc2e5019c727e2bde441f3a6a8fae466`.
  Its native build 40070164451 PASSED compile/link/assets/RomFS. #307 still starts
  at 92f95612; no application integration or replacement hardware artifact yet.
- Implemented native badge reads/presentation across Gen I-V, including 16 Johto+
  Kanto badges for Gen II/HGSS; source-pinned PKHeX offsets. Name/Money stay staged.
  Unsupported older Gender/Time writers remain clearly read-only; no invented Gen I Gender.
- Implemented 620px Location/Ability geometry, shared docked Held Item search using
  Species keyboard + two-column results and artwork, descriptive classic Help,
  IV/V detail-tap viewport activation, and honest native event-year labels/evidence.
- WORK IN PROGRESS: docked search needs behavioral tests + native compilation;
  CG40 must follow the new per-generation shared helper and retain negative controls.
  Current candidate guard also flags one old-style read-only Gender label; fix it
  through shared field-state presentation. Full QA matrix (489 cells) currently building;
  full normal/sanitized host suites must rerun on frozen source with leak checks outside ptrace.
- Next: finish Gen V form-specific move pools from pinned PokeAPI CSVs downloaded
  at /tmp/oct24-pokeapi; don't ignore form. Add targeted behavior regressions, run
  full gates, integrate forward to #307, then ONE official NRO + docs binding + audit.
- Historical hardware remains exactly 27 PASS / 6 FAIL / 4 unmarked; S06 NOT TESTED.

## October 24 backup-browser repair checkpoint — WIP, not yet integrated

- Starting #307 remains `92f956120019673733bd8d65cd65ab33c46cc237`. Review WIP
  f0150dd6 passed unfiltered native build 40068229858 / job 120795867730,
  including packaged RomFS. Held-item milestone 7130bff3 is pushed/verified.
- Gen III Safari Create tests now PASS normal AND ASan/UBSan/leak detection on all
  five titles. LeakSanitizer must run outside sandbox tracing (ptrace prevents it);
  no sanitizer check was disabled. This is host evidence, not a hardware result.
- New backup work: fullscreen emulator history with the same compact boxed rows as
  native history; timestamp-first native/legacy labels, newest-first mixed copies;
  exact selected-file opens through existing Gen I-V read-only Trainer bridges.
  No profile/live-source binding changes and no fallback to the current original.
  Recoverable archives remain visible when their live source/assignment is absent;
  unassigned recovery history cannot Create/Browse/Delete or claim profile ownership.
- Physical snapshots reject stale/replaced files, traversal, symlinks, oversized or
  partial reads. Manifest/game/source/path are rechecked at A. Native legacy folders
  remain ownership-unknown and non-deletable; native historical opens are validated,
  immutable READ-ONLY previews (native Switch staged editing/export from those previews
  is not added). Gen I-V emulator backups retain full staged Review/Discard/Export Copy.
- Pure snapshot tests PASS normal + ASan/UBSan/leak detection: duplicate chronological
  dates, source disappearance, stale files/manifests, directory changes, traversal/links.
  Native backup deletion retains Y confirmation, protects legacy/unknown ownership,
  and now keeps a final owned snapshot. Original source trees are never deletion targets.
- Expanded QA matrix adds one real backup-session cell for EACH of all 20 game titles:
  open exact archive -> stage native Money -> verified separate export -> strict
  reparse -> Discard -> unchanged archive bytes/mtime. GCC build/run is in progress
  (project output gcc-qa-matrix.log, exec session 16855); not yet claimed PASS.
- Focused shared-editor, backup layout and namespace tests pass after updating their
  obsolete Review/layout assertions to the newly approved shared paths. Full host
  sweep earlier stopped on another retired Discard painter assertion; it now guards
  the common confirmation. Full suites must rerun on the frozen final source.
- Still pending: older Trainer badges/gender/play-time parity, compact Location/Ability
  menus, docked Held Item search, Help audit, IV Pokérus touch, event/year context,
  existing V compatibility/inline-Language and II Unown exhaustive verification;
  full GCC/Clang host/sanitizers and both matrices; native build; #307 integration;
  ONE freshly built, bound and independently audited replacement NRO. S06 NOT TESTED.

## October 24 Gen III Create repair checkpoint

- Review WIP `f0150dd621086e0317de0c8de42eb64cccaecd14` is pushed/remote-verified;
  native compile run 40068229858 is in progress (not a hardware candidate).
- R2-REVIEWMOVE Safari reproduction: caught-Ball IDs 1–12 were supported; the
  Held Item chooser also offered Safari Ball but native validation incorrectly used
  Bag availability. Use PKHeX's separate GetAllHeld set (244 IDs), pinned to
  6042efa241a9925ce57e645329ea7f976deabdd3 / ItemStorage3RS.cs
  SHA256 3c6d50c6a6775a9d61d6dced2e1b60366789bddefb7c6f60e8cb1ba045438646.
  Picker and serializer now share native held-item support. Bag policy is unchanged;
  unknown encounter/obtainability evidence is not a clean legality verdict.
- GCC native staged-Pokémon regression PASS: all 12 caught Balls crossed with None,
  Safari Ball, Hyper Potion and Berry Juice as Held Item, across all 5 Gen III games;
  encrypted native Create/reparse/checksums/Discard/source immutability verified.
  Key Items, HMs, out-of-range held IDs and unsupported caught Balls still refuse
  without staging bytes. Full sanitizer and integration gates remain pending.
- Existing touch source guards now follow the owner-requested shared Review pages,
  including touchable shared navigation, instead of the retired Gen III scroll list.
- Backup-session source changes remain local WIP under active development; not yet
  integrated or claimed tested. No #307/main/#314 movement from this checkpoint.

## October 24 repair campaign — active Codex writer

- Fetched and resumed actual application #307 head `92f956120019673733bd8d65cd65ab33c46cc237`.
  All ten newer commits preserved by fast-forward. Codex is the active application writer.
- The owner supplied a new 27 PASS / 6 FAIL / 4 unmarked checklist, including requested
  changes under PASS entries. Historical results are not rewritten by host tests.
  Main and legality #314 remain separate; original sources/backups remain immutable.
- First repair package routes Gen I–III Review through the accepted shared IV/V panel,
  pages, X Export / Y Discard / B Back and shared two-step Discard. Gen V now uses
  the same four-choice exit presenter/decision as I–IV. Trainer V exposes Y Review
  and a visible touch control, retaining its existing + shortcut.
- Focused GCC Review/discard/page/exit model tests, all-title Gen V Trainer footer
  (196 checks), and classic stat-state/derived HP-DV test PASS. Candidate source
  contracts report zero failures. Full normal/sanitizer/native gates still required.
- Starting-head GCC/Clang CI both failed an obsolete assertion expecting the old
  Special-stat drawing expression. It now guards both native calculated-column
  values and their read-only styling, matching the approved #324 layout.
- Work is temporarily on `wip/codex-oct24-repairs` until all integration gates pass;
  then merge/push forward to the SAME application #307 branch. No repairs are lost.
- Remaining packages: validated fullscreen backup open (including protected legacy
  native backups), older Trainer parity, compact pickers/search/help, Gen III Safari
  Create reproduction and validated held-item rules, existing V move/language and II
  Unown verification, IV Pokérus touch, documented event labels. Build and audit ONE
  replacement NRO after final application integration. The previous c60 binding is
  historical and must not be reused for this changed application code. S06 NOT TESTED.

## Official Round 4 candidate binding — 2026-10-22

- Application source `c60d4da0abc467aedac54aa886179c89fc89bc19`, tree
  `b646118a299bbe531f87d948d7a1d55aff4e22de`, built successfully by gen15 integrated run
  39728794477 / job 119658788437. The original app58 and backup44 histories are preserved.
- Sole bound device candidate: `PokeBank-NX-Gen1-5-Candidate-c60d4da0.nro`,
  176610993 bytes, SHA-256 `6a150386e48f9a8ea075032de453bde036550dca7adfdf40ee3388f83092c456`.
  Artifact 12404690063: https://github.com/GlitchedZeus/PokeBank-NX/actions/runs/39728794477/artifacts/12404690063. Packaged RomFS PASS:
  4267 files / 156.6 MiB / digest bc09c152ac4232a2. NOT DEVICE TESTED.
- GCC/Clang full normal/sanitizer gates passed; both normal suites ran all 197 tests.
  Both 469-cell QA matrices and all 18 real export paths passed with leak detection.
  Candidate source contracts zero; all 70 owner-PASS items guarded (196 guards).
- QA portability follow-up `0de3420f76c489baf25611a54dd5bfb2b9febeaf` changes only
  three test loop bindings plus handoff. Its affected normal/sanitized tests passed.
  Application files still exactly match the official candidate. This binding is a
  separate DOCS-ONLY commit; candidate ancestry and the non-QA staleness guard remain intact.
- Exact-file QA-F9 and final exact-head CI are the delivery gates: consult #307 checks
  and its audit workflow record, not the superseded stale-62a2 run. The audit downloads
  this original artifact and does not rebuild. No later automatically produced candidate
  supersedes this explicit binding. Full 37-ID statuses/retest steps are in
  docs/hardware/ROUND4_REPAIRS_AND_RETEST.md; PR #307 remains draft.
- Remaining owner task is physical Switch retesting of this exact NRO. Hardware baseline
  remains 70 PASS / 37 FAIL / 3 NOT TESTED, S06 NOT TESTED. Never write a live emulator
  source, merge legality #314 or change main. Any new application code requires another
  freshly built/bound/audited candidate before making hardware claims.

## CI portability follow-up — 2026-10-22

- Both app #307 and Codex WIP were pushed and remote-verified at
  `c60d4da0abc467aedac54aa886179c89fc89bc19` (tree
  `b646118a299bbe531f87d948d7a1d55aff4e22de`). PR #307 remains draft.
- Official gen15 integrated candidate run 39728794477 / job 119658788437 PASSED all
  build/asset/package/upload steps. Published artifact 12404690063 contains
  PokeBank-NX-Gen1-5-Candidate-c60d4da0.nro, 176610993 bytes, SHA-256
  `6a150386e48f9a8ea075032de453bde036550dca7adfdf40ee3388f83092c456`.
  Packaged RomFS: 4267 files, 156.6 MiB, digest bc09c152ac4232a2. NOT DEVICE TESTED.
- Exact-head CI caught GCC's -Werror=range-loop-construct in a new test's structured
  binding. Three test loops now bind const references (matrix, Bag flow, encounter
  contexts); no warning setting or assertion changes. Affected GCC normal and sanitizer
  Bag/context tests passed; matrix passed 469 cells, zero problems, both variants.
- Application files are unchanged from the official c60 candidate. The next commit
  binds that ONE artifact via docs-only changes, retaining candidate ancestry and the
  non-QA-file staleness guard. Original 62a2 binding correctly failed its stale-binary
  audit; do not rerun or silence that old identity. Require a new QA-F9 PASS and final
  exact-head CI before delivery. Source saves/main/#314 remain untouched; S06 NOT TESTED.

## All local integration gates closed — 2026-10-22

- Validated and remote-verified source checkpoint:
  `2ba52983bbe663966be25f2d74c2425657e7331d` on
  `wip/codex-round4-takeover-20261021`. Code is frozen. Codex is the sole application
  integration writer for this owner-authorized takeover; keep #307 draft.
- GCC AND Clang complete `host-test host-sanitize` PASSED, leak detection enabled.
  Each normal suite ran all 197 binaries. Final sanitizer dependency sweeps rebuilt
  every source/test changed during the long build. Logs: project outputs
  `full-final-gcc-frozen.log` / `full-final-clang-frozen.log`.
- Both compilers' frozen `qa-contracts qa-matrix` PASSED normal + ASan/UBSan/leaks=1:
  469 cells, zero problems; all 18 real export paths satisfy C1–C6. Candidate source
  contracts report zero failures; 70-owner-PASS gate (196 guards), repository safety
  and diff checks passed after the test updates.
- Application files exactly match native-tested code checkpoint
  `2753e17ba677694bde809e56ddbf0b11d9fabe03`: run 39726730148, job 119652230798,
  all compile/link/asset/package steps PASS. Later changes are tests/CI/docs only;
  `git diff --quiet 2753e17b HEAD -- src include Makefile romfs` passed.
- Refreshed remote integration baseline remains app58 and backup44; BOTH are ancestors
  of this checkpoint. Backup merge fe5c3ad7 preserves both original parents. This docs
  checkpoint is being pushed forward to #307 and the WIP together, without a merge to
  main or legality #314. Exact-head GitHub CI still must complete before calling the
  repair campaign Integrated. The full 37-ID record is docs/hardware/ROUND4_REPAIRS_AND_RETEST.md.
- Next: let gen15-integrated-candidate.yml build the actual #307 head; obtain exact
  filename/source/tree/hash/size/run/artifact identity. Bind that file in a docs-only
  commit, keep the staleness guard, and require QA-F9 NRO AUDIT: PASS. Wait for final
  CI conclusions, update the PR description and deliver only the bound official NRO.
  No smoke NRO is the hardware candidate. Hardware remains 70/37/3; S06 NOT TESTED.

## Full normal suite contract follow-up — 2026-10-22

- Latest remote-verified application-code checkpoint is
  `2753e17ba677694bde809e56ddbf0b11d9fabe03`; native run 39726730148, job
  119652230798, passed EVERY compile/link, asset preflight, packaged RomFS and smoke
  upload step. #307 remains app58; no official candidate or binding is changed.
- The complete Clang `host-test` run passed all 197 binaries after the fixes below.
  Log: project `outputs/round4-takeover/full-final-clang-normal-contract-rerun.log`.
  The equivalent GCC run also passed all 197 binaries. Both complete sanitizer builds
  remain in progress; run a final dependency sweep after their parsed graph completes.
- Ten old assertions expected retired implementations: duplicated literal Origin labels,
  a supposedly constrained full legality claim, inline rather than shared Discard rows,
  the old narrow scrollbar gutter, a Touch Back label, fake Box context for Party,
  combined Special values and raw item IDs. They now follow the real shared helpers and
  require honest NotChecked/Incomplete status, unevaluated history, Y/B boxed Discard,
  the full scrollbar touch gap, Party-specific context, split Special numbers, and named
  Potion Review. Independent Gen II arithmetic stays unchanged; the Gen I manifest test
  additionally checks native Potion x25 after strict export reparse.
- The dedicated Gen V Bag Add flow still expected Key Items to be view-only and could
  loop forever when the newly supported picker opened. It now tests Key/TM native x1
  staging, required story warning, cancellation, native cap refusal, strict export reparse
  and Discard for all four titles. Picker/pocket loops are bounded and still report missing
  choices as failures. Existing stackable x1/quantity/focus/source-immutability checks remain.
- Full qa-tools-test PASSED in 635.34 seconds under simultaneous compiler load: all 20
  broken controls, NRO audit unit checks, the 70-PASS gate/mutations, workflow parsing,
  manual-staleness inventory and header-dependency regressions. Its CI tools job budget
  is increased from ten to twenty minutes so the expanded checks can finish; none skipped.
- Candidate source contracts were rerun after these test updates: ZERO failures; the
  70-owner-PASS gate (196 guards), repository safety and diff checks passed again.
- The normal sweep also executed the last 23 tests independently: all passed, including
  Party Actions, native Gen II Party editing, nature effects and encounter context.
- Keep this as WIP until complete GCC/Clang sanitizer + normal gates pass, then refresh
  app/backup remote heads and integrate forward into #307, retaining draft. Build ONE
  official integrated candidate, bind it in a docs-only commit and require QA-F9 PASS.
  S06 remains NOT TESTED. No live saves, main or legality #314 changes.

## Gate follow-up — preserve unchanged Gen II Party presentation (2026-10-22)

- Resume remote-verified WIP checkpoint `8658ef96d1fc1df70d6121f1eaac1449f4e4b72f`.
  Native run 39725096656 on application-code head 629d6365 passed EVERY compile/link,
  preflight, packaged RomFS and smoke artifact step. No official candidate is built yet.
- The complete normal suite surfaced a real bridge regression: the new Party refresh
  recreated untouched Gen II Party objects after a Box edit. Keep existing objects when
  their complete native body, stored names and egg marker are unchanged. Changed Party
  records still get a strictly reparsed presentation. The original pointer-preservation
  assertion is retained; new tests require Party nickname/friendship edits to appear,
  repeated unchanged refreshes to preserve identity, damage/status to remain intact, and
  Discard to restore the original Party view and immutable source.
- Focused Clang bridge tests passed normal + ASan/UBSan/leaks=1. Complete normal GCC and
  Clang are rerunning; full sanitizer builds continue in the same clean directories.
- Two pre-existing FR/LG tests hardcoded build-host/frlg_fixture.sav. They now honor the
  already exported POKEBANK_QA_GEN3_FIXTURE, retaining the same strict content checks, so
  relocated clean build directories are tested correctly. The complete normal rerun got
  past both fixture consumers and then found the Gen II regression above.
- Parser-cache equivalence and all twenty negative controls passed; candidate contracts
  remained zero. Complete qa-tools-test is rerunning with the optimization (session 25089).
- Still WIP: no integration to #307 until full normal/sanitizer gates and a native build
  of this new production refresh pass. Refresh source dependencies after the long builds
  finish; their parsed graph predates these latest source/test changes. S06 NOT TESTED;
  original emulator saves untouched; legality #314 and main untouched.

## Final gate checkpoint — 2026-10-22

- Latest application code is pushed and remote-verified at
  `629d636510995a15eb6ab66181b357f25ed7efec` on the Codex WIP. The source contains
  the complete Gen III Review follow-up and the missing shared Language bridge dispatch.
- BOTH GCC and Clang `qa-contracts qa-matrix` passed normal + ASan/UBSan with
  `ASAN_OPTIONS=detect_leaks=1`: 469 matrix cells, zero problems; all 18 real export
  paths satisfy C1–C6. The three reported cross-generation fact differences are retained.
- All twenty shared broken-path controls passed before parser optimization; the 70-owner-
  PASS gate (196 guards) and repository safety passed. Fourteen other current PR source/UI/
  touch/art guards passed; generated-species contract also passed with pinned network fetch.
- The exact RSE save-open workflow compilation and regression passed normal + sanitizer
  with leak detection after adding its LocationNames link dependency. The first local
  wrapper's temporary output-path mapping was incorrect; its corrected complete rerun passed.
- QA guard runtime approached the CI tools job's ten-minute budget: repeated checks parsed
  unchanged C++ source again. Cached immutable definition tuples by EXACT source string,
  with name filtering afterward. No assertion, source scope or mutation is removed. The
  old/new parser outputs compared exactly on all 569 real source files, plus changed input
  and name-filter examples. All twenty broken paths passed again with the cache, and candidate mode still reports
  zero failures. The complete QA tools job is rerunning to verify its time budget.
- Complete host GCC/Clang + sanitizers still running (sessions 47246 / 32982; resumed
  with three workers each after an intentional interrupt to increase parallelism); the native
  compile for 629d6365 is run 39725096656, job 119647073707. Do not integrate to #307 until
  those gates finish green. App #307 remains app58; no official candidate or new binding.

## Codex milestone 8 — complete Gen III Review and exercise repaired matrix behavior (2026-10-22)

- Resume baseline is remote-verified `be3d965a42846a81b5feb3b55ec10b4e46e38574`
  on `wip/codex-round4-takeover-20261021`. Its native run 39690466629 passed
  compile/link, asset verification and smoke packaging. App #307 and backup were refreshed
  and remain `58a81207e3eaef6d5070a17f4db5ab4b1ed286ea` and
  `44fb4b7eaa58f3e627ff981d84f2caeaed293667`. No other writer has advanced them.
- Gen III Review now names every editable scalar/native field, including Nature, Language,
  Ball, Met Location, Friendship, PID, ability, gender, Unown form and Pokérus. Redundant
  generic rows are removed; an unclassified raw change still gets an explicit stored-record
  row. Duplicate/unknown location labels retain their native IDs when needed to distinguish
  before/after. Added direct Party Review tests for all five exact Gen III games.
- Both rebuilt GCC and Clang Gen III native editor suites passed. The focused Clang normal
  and ASan/UBSan Review suites passed with leak detection enabled before this checkpoint.
  Added LocationNames to every relevant host graph; the full runs had exposed this missing
  link dependency, rather than a production Switch linking failure.
- The 20-title matrix now exercises Language via the shared field bridge and native
  Keep/replay/export, byte-exact OT/nickname preservation, all five Bag catalogues, unique
  Key Item/TM/HM x1 limits, warning cancellation and strict candidate reparsing. Gen II's
  independent six-stat formula assertions remain; the retired combined Special cell must
  be empty, with split-stat rendering covered by its real renderer suite.
- Expanded Language coverage exposed one missed shared bridge dispatch; it now forwards to
  native Language. The descriptor and navigation assertions now require editable Language
  and still refuse edits without a validated workspace. CG54 additionally guards that actual
  dispatch and has a broken-path control. Stored Met Location acceptance remains a no-op;
  the shared picker source test now explicitly checks that guard and the native setter path.
- Full GCC/Clang normal + sanitizer gates and both expanded matrices are rebuilding in
  `/tmp/round4-final-{gcc,clang}` and `/tmp/round4-final-qa-{gcc,clang}`. Logs are in the
  project `outputs/round4-takeover/`. Do not claim final full-suite success yet.
- Candidate source mode passed with 0 violations. The 20 broken-path controls, 70-PASS gate
  and repository safety are rerunning. Earlier qa-tools-test finished successfully; its
  optional manual-workflow inventory lists stale legacy checks without changing those workflows.
- Still required: finish/fix full gates, push and remote-verify the next checkpoint, native
  build of the new code, integrate finished work forward into #307, wait for actual-head CI,
  build ONE official integrated candidate, docs-only exact binding and QA-F9 audit. No
  hardware status changed. S06 remains NOT TESTED; no live emulator writes; #314/main untouched.

## Codex milestone 7 — native Gen I–III Party Actions and editing (2026-10-21)

- Source baseline `c9a457a3c3df246695fc47cde18e68c43e79d236` is pushed and remote-verified.
  Its Switch native run 39687900533 passed every build/asset/package step. #307 remains
  `58a81207`; the backup merge and application repairs are still isolated on the Codex WIP.
- Added ONE shared Party Actions coordinator/presenter before older frame handlers. A and
  completed taps open the same boxed-row View / Edit / Origin & Legality / Cancel sheet.
  Its selected native Party target is explicit in each generation's existing editor state.
  Generic modern mutators remain barred. Unsupported source layouts retain their native
  read-only View fallback. Party Clone/Release/PC Move are not offered by this new sheet.
- Gen I/II share their existing boxed edit codec with explicit native Party offsets; Gen III
  shares its PK3 edit codec with 100-byte Party records in the selected rotating save slot.
  Stat inputs refresh native cached level/stats; damage and fainted state, status, unused/mail
  bytes, other Party slots, PC records, inactive slots and RTC remain protected. No live save writes.
- Keep passes the Party target into the native staged writer; Discard/Continue stay draft-only.
  Party presentation refreshes after Keep/Discard. Gen II draft stat previews calculate the
  new values while passive View continues to display the stored native cache. Origin uses
  the Party record, and the context line identifies Party instead of a fake PC slot.
- Gen I/II Party Review has named moves and native fields. Gen III Review now adds named
  moves, PP/PP Ups, changed IV/EV cells, species/nickname/OT/IDs/experience/item fields;
  export summaries identify Party rather than mislabelling the target as Box 1.
- Tested so far: GCC Gen I new Party preservation/cache/Review/finalized-reparse cases and
  existing five-stat/boxed semantics passed; Gen II Party cases passed for G/S/C with RTC;
  Gen III Party cache/preview/rejection/immutability cases passed for all five exact games
  alongside existing boxed/native-export tests. Expanded semantic Review checks are rerunning.
  The actual shared Party coordinator passed all four actions by both controller and touch
  for Gen I/II/III, unsupported/empty targets and simultaneous A+B cancellation priority.
- Party checkpoint `c81853da370c335c77a532a1f9bc77cf5d50ff10` is pushed and remote-verified.
  Four focused suites × GCC/Clang × normal/ASan+UBSan = 16 PASS, leak detection enabled.
  Native run 39689447670 is compiling this Party checkpoint.
- Follow-up review found a preservation edge: PK3 metadata/PID setters may refresh cached
  Party stats even when their inputs are unchanged. The new follow-up restores all 20 cached
  bytes exactly in that case. A friendship-only edit with unusual cache bytes now has a
  regression case; the clean full-suite runs must cover this follow-up.
- Candidate source contracts now have ZERO violations; CG33's XFAIL is removed. The 70-owner-
  PASS regression gate (196 guards) and repository safety passed during this milestone.
  Nineteen broken-path shared controls are running; full focused GCC/Clang + sanitizers,
  full host suites, QA matrix, final native build and exact NRO audit are still required.
- This is a WIP checkpoint, not app integration or a hardware-verified repair. Do not bind
  a candidate or deliver any smoke NRO. S06 remains NOT TESTED; legality #314 stays separate.

## Codex milestone 6 — honest Gen IV/V Met Location context (2026-10-21)

- Baseline `e061f03c1c2b1bd4b5077eda2a2d2b2eb8234ac8` is pushed and remote-verified.
  #307 is still `58a81207`; work stays on `wip/codex-round4-takeover-20261021`.
- Gen IV/V now share a contextual picker. Default rows contain the stored location plus
  species / ancestor wild-source examples, with actual method and source level ranges.
  They never repeat the record's own Met Level as an encounter claim. X switches to the
  full native history catalogue, explicitly unverified. Every original native option and
  unusual stored id remains available; selecting the stored value bypasses mutation.
- Cached source context uses 16,837 deduplicated wild rows for all nine exact titles and
  566 generation-specific evolution edges. A reproducible bounds-checked generator reads
  unmodified PKHeX primary resources pinned to `6042efa241a9925ce57e645329ea7f976deabdd3`.
  Input SHA-256 values and counts are committed in `docs/data/`. No dependency on a ROM,
  save, .NET runtime, network, or the separate legality branch is added to normal builds.
- Egg/hatch and fateful flags are reported as stored evidence; trade, egg, special/event,
  and transfer markers have distinct unverified context. Gifts/static/other histories are
  not rejected when absent from wild tables. Forms, conditional encounter requirements,
  evolution eligibility and distribution validity are explicitly unchecked. Transferred
  non-native origins keep their existing read-only field restriction. No other field changes.
- Tests: all nine titles × Pikachu/Walrein, exact native-id retention, unusual stored ids,
  real Spheal ancestry in Pearl, unknown eggs/gifts/static/events/trades/transfers;
  GCC + Clang normal and ASan/UBSan with leak detection enabled all passed (4 runs).
  Generator corruption/bounds/count/hash tests and exact-source reproduction passed.
  Gen IV hardware surface and shared editor unit passed. An obsolete Y-only Bag Remove
  assertion now follows the owner's explicit A/B exception and retains B-first cancellation.
- CG53 is enforced; its real production calls/data and X access have four new broken-path
  controls. All 15 broken-path controls, the 70-PASS gate (196 guards), and Gen V UI guard passed. Candidate mode currently
  has one remaining actionable contract: CG33 Party Actions / native Party edit I–III.
- Corrected the native-only Review namespace ambiguity found in run 39686906403.
  Native run 39687900533 passed compile/link, asset preflight, packaged RomFS and smoke artifact upload on c9a457a3. No official NRO,
  app integration, hardware re-verification or candidate audit is claimed.

## Codex milestone 5 — one paged Review with every semantic field (2026-10-21)

- Source baseline for this milestone: `7beb91e7242046a02c53f05b65dcf8431d3af2b8`
  on the Codex WIP branch. #307 remains `58a81207`; no legality-lane code merged.
- Gen IV and V now use the same `SharedEditorChrome::drawReviewPanel` and the same
  Y-confirmed Discard All question, heading, X Export Copy / Y Discard All / B Back controls
  and L/R paging. The same controller handlers still invoke the native export transactions.
- Gen V Review expands every independently verified field instead of hiding fields after
  the first three. Full text-name differences, named ability / location values, Bag and
  Trainer changes are visible. A native PC Move remains one grouped movement action.
- Gen IV now produces a separate structured semantic-field Review from strict original /
  staged reparses. Move names, IV/EV fields, nature, names, OT/IDs and other native changes
  are paged. Grouped manifest summaries remain stable. Bag summaries use item names and
  game-aware TM/HM identifiers (HM01 Cut is not modern TM93).
- Added direct tests that Recover and every changed stat remain visible, all 25 semantic
  rows can be reached on seven pages without changing staged bytes, and a PC Move still
  yields one grouped row. Actual shared renderer tests cover all pages for 0–20 changes,
  bounds/non-overlap and only the visible footer's touch targets.
- Tested: four suites × GCC/Clang × normal/ASan+UBSan = 16 PASS, leak detection enabled:
  Gen IV staged editor / real verified exports; Gen V Review/Discard/Export trace (400
  checks across W/B/W2/B2); actual shared renderer; shared editor unit/source contracts.
  Gen V read-only route, touch-row and repository-safety guards passed. The 70-PASS gate
  (196 guards) and all eleven broken-path source mutation controls passed.
- CG35 is now enforced. Candidate mode has exactly two unresolved contracts: CG33
  (native Party Actions/edit I–III) and CG53 (source-backed IV/V encounter context).
- Native build of `20955b20518708c06f4c139ada894567b063f6c7` (run 39684964093)
  exposed a missing shared-label include in the standalone GSC overlay. `7beb91e7`
  fixes it; native run 39685646748 passed compile/link and packaged RomFS verification
  (4267 files, 156.6 MiB, digest bc09c152ac4232a2). Review head e061f03c native run
  39686906403 then exposed an ambiguous forward declaration of sharedReviewRows across
  an anonymous namespace boundary. Milestone 6 moves that declaration to its actual
  namespace. No final NRO or candidate binding exists yet.
- The earlier full host invocations read code while this Review was being developed and
  failed linking the new name/view dependencies against the old parsed build graph.
  Dependencies are now explicit in the Gen IV and QA source lists. Final exact-head full
  GCC/Clang, sanitizers and 20-title QA matrix remain required after the code freeze.

## Codex milestone 4 — native Trainer summaries and one Origin panel (2026-10-21)

- WIP remains `wip/codex-round4-takeover-20261021`; refreshed #307 and backup heads
  are unchanged at `58a81207` and `44fb4b7e`. No legality #314 integration.
- Implemented native read-only Play Time and Pokédex Seen/Caught for Gen I–IV. Timers
  come from primary PKHeX `6042efa241a9925ce57e645329ea7f976deabdd3`: SAV1/2 offsets
  and SaveBlock3Small{RS,E,FRLG}. Gen IV already parsed its timer. Invalid minute/second
  bytes report Unavailable, not a normalized guess. Gen I's saturation flag is retained.
- One `SharedEditorChrome::drawOriginLegalityPanel` now draws all seven active/legacy
  Origin modals. Generation adapters supply actual stored source, identity, checksum,
  OT, met facts/dates where present, moves, coverage and an honest final verdict.
  Missing history is not Invalid. No app adapter produces a clean verdict. Gen I–III
  remain Not checked; Gen IV/V remain Incomplete. Crystal level comparison is a separate
  Level context fact. Reserved Invalid / No problems found / Unknown states remain distinct.
- Corrected missed Details species fields to name first; list numbering remains intact.
  Ball row/clip/touch widths now reserve the scrollbar's full 18 px touch slop plus 4 px.
  Met Location bottom padding matches Move's 14 px. Unavailable ability has no fake slot name.
- Preserved the already-working held-item live keyboard callback and inline Crystal OT
  gender implementation. Source guards now follow these actual implementations, and follow
  shared footer/modal delegation; eight broken-path mutation controls pass.
- Gen V Language is now editable in the shared field-access policy as well as its native
  adapter. Its host native/Bag/inventory suites passed 12 runs: 3 × GCC/Clang × normal/sanitizers,
  with leak detection enabled. All native-language/default/nature tests had passed 32 runs earlier.
- Tested: four native timer/parser/bridge suites with GCC; actual Origin modal renderer
  (1–20 rows, bounds/non-overlap, only visible B footer touch, old Gen II graph assertions);
  shared editor, live item search and ability-effects units. Menu geometry rerun pending.
  The 70-owner-PASS regression gate passed again (196 guards); repository safety and Gen V
  shared UI / touch-row contracts passed.
- Native run 39534739735 on `88f974dae2b6677eeabddd689969f8cb77966fe5` failed compilation:
  the shared nature display called `nature()` on raw PK4. This checkpoint corrects that by
  an adapter method using Pokemon4ReadOnlyView. A new native build is still required.
- Full GCC/Clang runs compiled many suites but exposed the host runner's absolute-path
  bug (`.//tmp/...`). Recipes now execute their already slash-containing paths directly,
  retaining configured ASAN_OPTIONS. Clang also encountered the old missing host Switch stub
  in the new renderer. That isolated renderer now has a host-only controller-mask shim;
  native builds continue to use libnx. Full exact-head host/sanitizer reruns are required.
- CG34/40/41/42/44/45/46/47/48/49 are now enforced, in addition to the earlier enforced repairs.
  Remaining actionable candidate contracts: CG33 Party Actions/native Party edit I–III,
  CG35 shared Review parity, CG53 encounter-backed IV/V location context.
- Still no official Round 4 NRO or updated binding. #307 integration, full gates, one integrated
  candidate and exact-file QA-F9 audit remain. Hardware checks have not changed; S06 stays NOT TESTED.

## Codex milestone 3 — staged native fields and shared presentation (2026-10-21)

- Work remains on `wip/codex-round4-takeover-20261021`; #307 is still
  `58a81207e3eaef6d5070a17f4db5ab4b1ed286ea`. No legality #314 merge.
- Implemented PK5 Language at native byte 0x17 (IDs 1–5, 7, 8), independent Keep/Review
  replay, checksum/encryption and full SAV export. OT and nickname bytes stay unchanged.
- New Gen I/II DVs and Gen V IVs default to zero; existing records are unchanged.
  Gen II Special shows its two calculated stats separately; the actual shared portrait
  now receives DV-derived Unown form. Gen III–V show all 25 nature effects, five neutral,
  green raised/red lowered stats and +10%/−10%; HP never changes with nature.
- Gen IV/V move Category has a separate detail line and the badge has a 12 px gutter.
- Bag Remove uses A/B glyph choices, B cancels first, no duplicated footer. Release and
  Backup Delete remain Y-only. Redundant Close Help / Touch Back footer labels removed,
  with touch Back preserved. The owner-directed Bag exception is explicit in the protocol.
- Gen III Review now includes staged inventory rows and Discard restores that inventory;
  Gen III/IV Review returns to Trainer when entered from the Trainer card.
- Gen V Key Item and TM/HM native staged Add/Remove are implemented, cap one, duplicate
  refusal, Key Item warning before Add/Remove, no event flags changed. An interior empty
  native bag slot can be reused without shifting unrelated stacks.
- Tested: eight field/default/nature/portrait suites × GCC/Clang × normal/ASan+UBSan =
  32 successful runs (leaks enabled). The updated actual renderer also passed GCC.
  Classic inventory UI and Gen V bag UI passed GCC. Expanded Gen V bag/native-export tests
  passed after enabling unique pockets, across four titles and both selected save copies.
- The two full host compiler/sanitizer runs are still in progress; changing headers makes
  another dependency sweep necessary before claiming a final exact-head result.
- CG36/37/38/43/50/51/52/54/55/56 now pass and are enforced. The CG08 checker follows
  the actual shared bag presenter instead of requiring a duplicated footer at its call site.
  The first 70-PASS gate rerun exposed that checker limitation; the corrected rerun is pending.
- Published Review head `c2a2e5f4027ac441b53663a515c986359de4567e` passed native
  Switch compilation, link and packaged RomFS verification: Actions run 39532522658,
  job 119001525925. 4267 assets, 156.6 MiB, digest bc09c152ac4232a2.
  This is a development smoke build, not the official Round 4 audited candidate.
- Remaining: Party Actions/native Party editing I–III; shared truthful Origin & Legality;
  encounter-backed IV/V location context; older Trainer summary; Review parity; missed
  older picker paths reported by CG40/41/42/44/45/46; complete gates then #307 integration,
  one final candidate and exact-binary QA-F9. S06 stays NOT TESTED.

## 1. Where to start

| Item | Value |
| --- | --- |
| Repository | `GlitchedZeus/PokeBank-NX` |
| Integration branch | `claude/gen5-integration-20261014` (PR #307, **draft**: keep it draft) |
| Last code commit (the round-3 NRO was built from this) | `62a2f8c34476062ece344ef2188ab6b385369a7d` |
| Verified application integration HEAD at takeover | `58a81207e3eaef6d5070a17f4db5ab4b1ed286ea` |
| Codex working checkpoint | `wip/codex-round4-takeover-20261021`; see section 6c; not yet integrated into #307 |
| Legality engine | PR #314: a **separate lane**. Do not merge it with #307 or `main` without the owner's authorization. |
| `main` | Not touched by this campaign. Nothing merges to `main` without the owner. |

To start:

```sh
git fetch origin
git checkout -B work origin/claude/gen5-integration-20261014
git log --oneline -1 62a2f8c34476062ece344ef2188ab6b385369a7d   # must exist: the round-3 code head
```

## 2. Hardware status (owner-recorded only)

| Round | NRO tested | Owner result | Recorded in |
| --- | --- | --- | --- |
| 1 | `7fea1797` | 42 PASS | `docs/hardware/ROUND1_RESULTS_7fea1797.md` |
| 2 | `e10f307f` | 6 more PASS (B01, B02, B03, B06, S01, S03) | `docs/hardware/ROUND2_RESULTS_e10f307f.md` |
| 3 | `62a2f8c3` (sha256 `0c7a6fb0…2078`) | **110 checks: 70 PASS, 37 FAIL, 3 NOT TESTED** | [`docs/hardware/ROUND3_RESULTS_62a2f8c3.md`](../hardware/ROUND3_RESULTS_62a2f8c3.md) (owner, `c2287f50`) |

**Round-3 results are on GitHub.** Read `ROUND3_RESULTS_62a2f8c3.md` §9 for the owner's priority order (P0–P3).
Protect all 70 PASS items. The hardware gate guards the 48 round-1/2 passes today; QA was asked (2026-10-20) to
add the remaining round-3 passes to `tests/test_hw_round1_regression_gate.py`.

## 3. Round-3 state of each repair

Each repair is at "Implemented → Tested → Pushed → Integrated". None is hardware verified until the
round-3 results file says so.

| ID | Repair | Integrated at |
| --- | --- | --- |
| R2-BUG-004 | Species change resets moves to the game's level-up moves (`SpeciesMoveDefaults`, `tools/gen_levelup_learnsets15.py`, pinned pret + PKHeX 6501f0ab) | 8a84128e |
| R2-BUG-002 | Gen III/IV staged Trainer Name/Money editors and export (`Gen3StagedTrainerEditor`, `Gen4StagedTrainerEditor`) | 7ed1cc16 |
| R2-BUG-001 | Trainer "A: Edit" footer, including Gen I (`ClassicTrainerSession::readOnlyForFooterOverride`) | 11940524 |
| R2-BUG-003 | Gen V Trainer layout matches Gen I–IV (`Gen5TrainerScreen::footer`) | 7ed1cc16 |
| R2-BUG-012 | Boxed-row modals (`include/UI/SharedBoxedRows.h`) | 7ed1cc16, 9d4b8b71 |
| R2-BUG-013 | Coloured controller glyphs (`GlyphInstruction.h`, `ScreenChrome::drawGlyphHelpLine`) | aeccd13, 678627f5 |
| R2-BUG-006 | 4-column item grid (`SharedItemGridPicker.h`, `MenuType::ItemPicker`) | 9d4b8b71 |
| R2-BUG-005 | TM/HM move details (`MachineMoves.h`), Gen V HM labels and feathers | 7ed1cc16, 9d4b8b71 |
| R2-BUG-007 | Move chooser type badges | 9d4b8b71 |
| R2-CHECK-009 | Gen I Box Actions → Move (`beginGen1FromAction`) | 9d4b8b71 |
| R2-BUG-008 | Gen III Met Location "keep stored" (`Gen3EncounterPickerModel.h`) | 5949b15 |
| Part 9 #1, 3, 4, 6, 8, 9 | Gen V staged-slot actions, provider labels, RetroArch core from save evidence, Gen V item art, Gen IV Storage carry, Home/Profile focus | see `RETEST_62a2f8c3.md` Group B |

Investigated, not defects:
- **R2-CHECK-010:** the Super Rod in the R/S PC is native behaviour. The R/S Devon Goods / Letter / Orb and
  Emerald HM PC offers were removed.
- **R2-CHECK-011:** the names match PKHeX, but the **owner FAILED it in round 3** (location lists too broad); QA CG53 covers it and lane P owns it with NEW-12.

Never device-verified: B04, E03, E11, L06, S04, S06, C09, M08, and the real RetroArch/DraStic launch and return.

## 4. Tests actually run on `62a2f8c3` (all PASS)

**GitHub Actions on `62a2f8c3`, all success:**
- Native PR Gate 39227970367
- Host Tests GCC 39227970682 (41.6 min)
- Host Tests Clang 39227970563
- Gen I–V Integrated Candidate 39227970775
- Gen I–IV HW Candidate 39227970763
- Gen V Focused 39227970525
- App QA Contracts 39227970615
- Fuzz 39227970787
- Workflow Hygiene 39227970810
- Touch 39227970818
- Packed Move 39227970721 / 39227970952

**QA-F9 exact-binary audit:** run 39233872565, job 117998117020, on binding commit `887552e7`. Result
`NRO AUDIT: PASS`; CG `--candidate` reports 32 contracts, 0 failing.

**The integrator's local gate on the same app code** (`9d4b8b71`, clean build dirs; it differs from `62a2f8c3` only by one CI timeout line):
- GCC and Clang `host-test`, `host-sanitize` and `qa`: 429 PASS lines each;
- py contracts 29/29;
- hardware gate PASS;
- devkitA64 OK.

**Failing tests:** none on `62a2f8c3`.

## 5. NRO

| | |
| --- | --- |
| Built natively in CI | yes |
| File | `PokeBank-NX-Gen1-5-Candidate-62a2f8c3.nro`, 176029361 bytes |
| Artifact | https://github.com/GlitchedZeus/PokeBank-NX/actions/runs/39227970775/artifacts/12175187129 (expires 2027-01-17) |
| SHA-256 | `0c7a6fb01af2a1356676beedf3aec9da79842a499c7c63833fb683f42f332078` |

Later docs-only commits on #307 also trigger a candidate build. Those artifacts are **not** the audited candidate.

## 6. Branches and parallel work

**Lane branches.** All are integrated into #307 by SHA. Their tips are history; do not resume from them.

| Lane | Branch | Tip |
| --- | --- | --- |
| C | `claude/gen14-campaign-20261015` | `114a22f3` |
| S | `claude/shared-editor-gen35-20261016` | `3ff061bf` |
| P | `claude/pickers-search-scrollbar-20261017` | `8d533b52` |
| I | `claude/item-quantity-policy-20261016` | `18ffcc76` |
| R | `claude/reroll-mechanics-20261017` | `a1c96ed6` |
| L | `claude/launch-return-20261015` | `4f57bde4` (draft PR #319, CI only) |
| QA | `audit/app-qa-20261014` | `5c965cb3` |
| Coordinator | `claude/optimistic-euler-ph2z4f` | `docs/handoffs/MAIN_DEV_CHECKPOINT.md`, `ROUND3_LANE_PLAN.md` |

**WIP checkpoint branches: none.** On 2026-10-20 every lane session was asked to push any unpushed work. Six lanes replied
"clean, nothing unpushed", each with an `ls-remote`-verified tip matching the table above: 2R `c3b0ac48`, QA `5c965cb3`,
C `114a22f3`, P `8d533b52`, R `a1c96ed6`, L `4f57bde4`. The messages to **S** and **I** were blocked by a permission
check, so their local state is unconfirmed; their remote tips are S `3ff061bf` and I `18ffcc76`.
`git ls-remote origin 'refs/heads/wip/*'` returned nothing.

**Other lanes, separate and not to be mixed in:**
- `claude/gen1-5-legality-campaign-w9mkom` (legality, active 2026-10-20);
- `research/gen6-9-codex-20261018`;
- `claude/gen6/7/8-native-*`.

**Git conflicts:** none open on #307.

## 6b. Round 4 (2026-10-20): live repair status

States: Implemented → Tested (host) → Pushed → Integrated (#307) → Hardware verified (owner only).

| Owner IDs | Repair | State | Branch @ SHA |
| --- | --- | --- | --- |
| NEW-16, C01 | Gen V Home A / Items open the validated assigned save directly; anything not ready falls back to Save Instances with the reason (`SaveSelectScreen::openAssignedGen5Directly`) | **Integrated** (Native PR Gate 39511458784 green) | #307 `c59ac178` (from `wip/coord-gen5-direct-open-20261020` @ `61491b53`) |
| B08 | Export result / status messages painted on every surface (`TrainerViewScreen::drawTransientStatus`, scope guards in the composite `draw`/`update`); root cause: Gen II–V surfaces return before the only status painter | **Integrated**; `tests/test_status_toast_contract.py` | #307 `c59ac178` |
| R2-BUG-006 | Short item grids use balanced rows and a narrower frame (lane P) | **Integrated**; host test `test_item_grid_picker` | #307 `5887c56c` (P `e3c81745`) |
| R2-REROLL | Gen II Unown letter recomputed from DVs and the editor sprite follows it; DV and shiny rules audited correct against pret (lane R) | **Integrated**; host test `test_gen2_unown_letter` | #307 `aea3730e` (R `fd30e0fc`) |
| NEW-11, NEW-05, NEW-08, NEW-10 | "Pikachu #25" label; search keyboard no longer covers the species list; scrollbar gutter in value lists; compact Met Location popups (lane P) | **Integrated** | #307 `1d803f01` (P `9573e8e8`) |
| QA gate | All 70 round-3 PASS items guarded (196 guards); CG29; CG30–CG35 XFAIL contracts reproduce R2-TRAINER/R2-BUG-002, NEW-19, R2-REVIEWMOVE, L06/E02/NEW-20, NEW-17, AUD-01 | **Integrated** | #307 `847fe390`, `0d810f7a` |
| NEW-04 | Gen II PC box Move: the grid refresh used `finalizedBytes()`, which refuses mid-carry, so every pickup cancelled. Added display-only `StagedEditor::presentationBytes()`; finalize still refuses mid-carry (lane C) | **Integrated**; host test `test_gen2_box_move_presentation` (G/S/C pickup, same/other-box drop, cancel, finalize + export) | #307 `131fb23f` (C `c3807c63`) |
| NEW-07, NEW-06, NEW-13 | Crystal OT Gender cycles inline; Held Item search; Ability names + short effects (lane P). **Caveat:** the ability effect lines in `include/UI/AbilityEffects.h` are P's own summaries; no source in the repo. Owner or I to spot-check | **Integrated**; `test_gen2_field_selector`, `test_item_picker_search`, `test_ability_effects` | #307 `75546e44` (P `77576217`) |
| QA contracts | CG30–CG60: all 37 owner FAILs as XFAIL, with owners; CG39/57/58 enforced | **Integrated** | #307 `c80a86c5` |
| R2-EMUBACKUP, C07, S05 | PKSE-style emulator backup browser: create, browse, list, delete with Y; no automatic backup (lane L) | Pushed (WIP), not integrated yet | `wip/l-backup-browser-20261020` @ `6cb161c4` |

**XFAIL contracts:** CG30–CG35 fail in `--candidate` mode until their lanes land. Normal mode reports 0 failing. A round-4 candidate needs all of them fixed (or the owner's decision).

**Lane assignments sent 2026-10-20.** Each lane works from #307 `c2287f50` and pushes to its own branch or `wip/<lane>-<topic>-20261020`:

| Lane | Owner IDs |
| --- | --- |
| L | R2-EMUBACKUP, C07, S05: PKSE-style backup browser |
| C | NEW-04: Gen II PC Move; then NEW-19, R2-TRAINER, R2-BUG-002: Gen I–IV staged exit menu; then R2-REVIEWMOVE and NEW-18 |
| S | L06, E02, NEW-20: Party Actions; E03: legality layout; M04/M05; AUD-01; NEW-01/NEW-02 |
| I | NEW-17: Gen V Key Items / TM / HM editing; NEW-03; R2-CHECK-010 glyphs |
| R | R2-REROLL / Unown audit; NEW-15; R2-SPECIAL; NEW-09; NEW-14 |
| P | NEW-12 / R2-CHECK-011; NEW-10; NEW-05; NEW-06; NEW-07; NEW-08; NEW-11; NEW-13; R2-BUG-006 |
| QA | Gate the 70 PASS items; contracts for the 37 FAIL items |

The 2R integrator lane was not reached (permission check), so the coordinator is integrating. Before resuming, check each lane
branch and every `wip/*` branch with `git ls-remote origin`.

## 6c. Codex takeover checkpoint (2026-10-21 UTC)

Codex owns `wip/codex-round4-takeover-20261021`; the owner explicitly authorized taking over #307,
reviewing/integrating the backup work and finishing the remaining Round 4 repairs. Do not write this
branch concurrently. No messages were sent to other contributors. Legality #314 remains separate.

- Integration base: `58a81207e3eaef6d5070a17f4db5ab4b1ed286ea` (all existing 12 repair entries retained).
- Reviewed backup tip: `44fb4b7eaa58f3e627ff981d84f2caeaed293667`.
- Working code checkpoint: `fe5c3ad76fd5b1e54d5f8e4d773986b7757405ef`, a forward merge preserving both parents.
- Implemented: explicit emulator/installed backup creation; newest-first combined manual/export history;
  Y-confirmed eligible deletion; original-source refusal; no automatic backup on unchanged open;
  removal of obsolete Auto-Backup setting; exact selected legacy source routing; live last-copy
  recheck at deletion; recovery of earlier Working bytes after a failed partial refresh, without
  touching an unexpected callback destination.
- Tested: nine focused suites under GCC and Clang normal builds (18 executions): emulator backup
  access/layout, classic verified export, verbatim export, Gen I export transaction, Gen V export
  validation, Gen V Review/Discard/Export trace, source-mutation policy and write policy. Game Hub
  source contract and repository safety passed. All 70 owner PASS items remain guarded (196 guards).
- ASan/UBSan focused runs and complete GCC export contracts/20-game matrix are in progress. Initial
  sanitizer execution in the tracing sandbox hit the documented LeakSanitizer/ptrace limitation;
  reruns preserve leak detection outside that sandbox. No sanitizer checks were disabled.
- NOT yet integrated into #307 and NOT hardware verified. No new official candidate is ready.
- Remaining: finish/record phase-1 tests, enforce CG59/60 after their genuine passing behavior, then
  Trainer Review/exit/named rows, Party Actions, Gen V inventory/language/defaults, remaining UI and
  source-backed location work, all candidate contracts, full gates, exact NRO build/binding/audit.

The owner's Round 3 report is already committed. Do not request it again. S06 remains NOT TESTED;
exports are tested only on disposable host sources. The pre-takeover `wip/coord-gen5-direct-open-20261020`
tip `61491b533ffba38a03bd46639a8ac3ea2ee11d0e` is already represented in #307. The backup WIP branch
remains preserved. Do not delete WIP refs or claim this checkpoint completes all 37 owner FAILs.

### Codex milestone 2: classic Review and readable changes

Code checkpoint: `29084707239d5c7bc8db9f70c2056d9e5dd27fe2`.
Implemented: Y / touch Review on Gen I-IV Trainer cards; shared staged exit choices Review / Continue /
Y Discard & Exit / Export Copy; verified export dispatch to each existing native publisher; returning
from Review preserves Trainer navigation; named Gen I/II moves, Gen I Bag items and Gen V named
moves/items/species/natures/balls. Native values are unchanged. Name-table linkage is retained in
standalone focused graphs and workflows.

Tested: five focused GCC suites passed (classic exit controls and B priority, Gen I Trainer staged
edits, Gen I/II Pokemon native edits plus explicit named-move regressions, Gen I/II Review discard
parity). CG30/31/32 and CG59/60 source checks pass and their XFAIL markers are removed. The CG59
reachability check now follows the real update -> create -> copy -> write path; negative controls
check broken create, automatic-on-open and carry-finalize paths. CG56's matcher recognizes the
already implemented guarded presentation/finalize ternary; an inverted or unguarded finalize still
fails. Complete GCC/Clang host + ASan/UBSan builds are running from clean directories.

Phase 1 final validation: 36/36 focused executions passed (GCC/Clang normal + ASan/UBSan, leak
checking enabled). Complete GCC export contracts and 469-cell / 20-title matrix passed in normal
and sanitizer configurations (0 problems). These are host results, not owner hardware acceptance.
The backup WIP and this milestone are still on the Codex WIP lane, not yet #307. Native compilation,
remaining actionable contracts, Clang QA matrix and final source-bound NRO audit are pending.

## 7. Next development tasks (in order)

1. **Record the round-3 results** (§2). Then add the round-3 PASS items to the hardware gate.
2. **Reproduce each round-3 FAIL from source before changing code.** Use the owner's input trace when a failure
   is input-related (`sdmc:/switch/PokeBank-NX/config/input_trace.enable`).
3. **Owner priority areas.** Which items actually failed comes from the results file. Files to inspect first:
   - **Save / backup management:**
     - `include/UI/BackupSelectionScreen.h`
     - `include/UI/EmulatorBackupLayout.h`
     - `include/UI/SourceBackupClassification.h`
   - **Gen II Move Pokémon:**
     - `src/Integration/Gen2/Gen2PackedMove.cpp`
     - `include/UI/ClassicPackedMovePresentation.h`
     - `tests/test_gen12_packed_move.cpp`
   - **Gen I–IV Review / exit parity:**
     - `include/UI/Gen12ReviewDiscardConfirm.h`
     - `include/UI/PokemonEditorExitGuard.h`
     - `include/UI/LeaveDecisionLayout.h`
     - reference: `include/Integration/Gen5/Gen5SharedReview.h`
   - **Gen V direct save opening:**
     - `include/Integration/Gen5/Gen5SourceSelection.h`
     - `Gen5SourceDiscovery.h`
     - `Gen5AssignedSource.h`
   - **Export Copy:**
     - `include/Integration/Common/VerifiedStagedExport.h`
     - `Gen{1..4}ExportTransaction.h`
     - `Gen5WorkingCopyExport.h`
   - **Gen V Bag (Key Item / TM / HM editing):**
     - `include/Integration/Gen5/Gen5Bag.h`
     - `Gen5BagScreen.h`
     - `ItemQuantityPolicy`
     - `MachineMoves.h`
4. **For each working repair:**
   - run the gates in the protocol;
   - commit and push (to a `wip/` branch if anything is unfinished);
   - update this file.
5. **Build one round-4 candidate.** When the round is complete, build the single round-4 candidate on the #307 head,
   bind it in `docs/qa/CANDIDATE_UNDER_AUDIT.env` (docs-only), and run QA-F9.

**Partially implemented files:** none known on #307. Check any `wip/*` branches (§6).

## 8. Source-save protection risks

- **Export writes a separate copy.** `BUILD_IDENTITY.txt` must keep `SOURCE_SAVE_WRITES=DISABLED`.
- **Never write a live emulator save.** For S06, the owner copies the export into a separate slot by hand.
- **Repairs to Export Copy, Gen V direct save opening, and backup management touch the immutability path.** Keep
  these green:
  - `tests/test_qa_export_contracts`;
  - the QA matrix `export`, `export.checksum` and `immutability` cells (20/20 games);
  - `test_ci_repository_safety_contract.py`.
- **Never add saves, ROMs or keys to the repository.**
