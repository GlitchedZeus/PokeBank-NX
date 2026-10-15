# Application QA findings (Thread 4)

Audited heads: PR #307 `claude/gen5-integration-20261014` at `a17c503ad272cc55e69b8b3cbd925cec999567cc`
(lane start, contains the #306 Gen I–IV baseline), then `87584a67` (Gen V Trainer/Bag/Pokedex
surfaces), then the Gen I–V hardware candidate head `fa9cfceabfecd2afb79d0df935588a3e81e0a7fe`
(tree `5de23dc2`), which carries the QA-F1 and QA-F3 fixes. Each was forward-merged into this lane
without conflicts and the full suite re-run. All inputs synthetic.

Open defects keep a failing reproduction **outside** every default gate
(`make -f Makefile.host qa-known-failures`, `tests/test_qa_known_failures_assets.py`). When a fix
lands, its reproduction moves into the passing suite as a regression test
(`tests/test_qa_regressions.cpp` in `qa-contracts`, `tests/test_qa_regressions_assets.py` in
`qa-tools-test`).

## Summary

| ID | Severity | Area | Owner | Status |
| --- | --- | --- | --- | --- |
| QA-F2 | **Medium** (CI hygiene) | 3 invalid `one-time-*` workflow files failed on every push to every branch | Thread 2R (applied); deletion: owner | **FIXED** on #307 in `96a44685` (indentation-only repair, triggers gated to `workflow_dispatch`); regression `tools/qa/check_workflows.py` in `qa-tools-test` and `app-qa-workflow-hygiene.yml` |
| QA-F3 | Low | Common (and Gen V) export publish could replace a destination created after the uniqueness check | Thread 2 | **FIXED** in `fa9cfcea`; regression test in `qa-contracts` |
| QA-F3b | Low | The same non-exclusive `rename()` publish remained in the Gen II transaction | Thread C (`Gen2ExportTransaction.cpp`) | **FIXED** on #307 (Thread C merge `cb88043`); regression `gen2PublishIsExclusive` in `qa-contracts` |
| QA-F1 | Low | Packaged-RomFS manifest omitted the four Gen V game cards the app requests | Thread 2 | **FIXED** in `3b35b3f`; regression test in `qa-tools-test` |
| QA-F4 | Low (defence in depth) | Gen V PID replay (`Field::Personality`) protects the PID/gender link only when the ability link also holds | Thread 2R (`Gen5StagedPokemonRecord.h`) | **FIXED** in `bc83815`; regression in `test_qa_regressions.cpp` |
| QA-F5 | Low (presentation) | Gen V Home party strip drops the party member's form, so alternate forms show the base-form icon; Gen IV keeps it | Thread 2R (`Gen5GameCardPreview.h`) | **FIXED** on #307 (`PartyPreviewSlot.form`); regression in `test_qa_regressions.cpp` |
| QA-F6 | Low (identity) | Gen III discovery takes the exact release from a raw substring of the WHOLE path, so an unrelated folder or word decides a hint-less save's identity (Gen I/II accept only isolated tokens) | Thread C (`RetroArchFRLGDiscovery.cpp` `identityHint`) | **FIXED** in `82032a9`; regression in `test_qa_regressions.cpp` |
| QA-F7 | Low (editor gap) | Gen IV Unown letter (form) cannot be edited: `setForm` uses the personal table's raw form count (1), the pinned PKSM-Core normalises Unown to 28 | Thread C (`Pokemon4Mutable.cpp` `setForm`) | **FIXED** in `f17926f`; regression in `test_qa_regressions.cpp` |
| QA-F8a | Low (presentation) | Gen IV net no-op clone + release reviewed as "(empty) (edited)" | Thread C | **FIXED** in `c7b0f97`; regression in `test_qa_regressions.cpp` |
| QA-F8b | Low (export bytes) | Gen IV release zero-fills a slot that held an encrypted blank, so a net no-op clone + release still changes 126 bytes of the working copy / `edited.sav` that the review no longer lists | Thread C (`Gen4StagedPokemonEditor::stageReleaseBoxPokemon`) | OPEN; repro `tests/test_qa_known_failures.cpp` |

No crash, sanitizer report, out-of-bounds read or round-trip violation was found in any Gen I–V parser,
record decoder, Bag decoder, copy selector or Save Instance identification path (see
[Fuzzing evidence](#fuzzing-evidence)). All 18 export paths (including the Gen V Trainer Name/Money path and the production Gen I transaction `publishVerifiedGen1Export`) satisfy the six export-safety contracts C1–C6 (C6: Discard All restores the working copy and a later export is refused); see [SAVE_SAFETY_CHAIN.md](SAVE_SAFETY_CHAIN.md).

## QA-F4 (Low, defence in depth; FIXED in bc83815): Gen V PID replay guard conjoins the gender and ability links

- **Found by:** `fuzz_gen5_edits` (libFuzzer, 2026-10-15), the new edit-sequence fuzzer over
  `Gen5StagedPokemonWorkspace`.
- **Code:** `include/Integration/Gen5/Gen5StagedPokemonRecord.h`, `case Field::Personality`. The
  `linked` lambda returns `genderOk && abilityOk`; the replay is refused only when the PREVIOUS
  record was linked on both. A Gen V origin record whose stored gender matches its PID but whose
  ability byte does not match its PID ability slot (bit 16) is therefore "unlinked", and any
  replayed PID is accepted, including one that desynchronises stored gender from the PID
  (PKHeX `EntityGender.GetFromPID`).
- **Impact:** limited. The shared editor only replays PIDs built by the coordinated
  Gender/Shiny/Ability procedures, which keep the links, and retail Gen V records carry a consistent
  ability. It matters for already-inconsistent (imported or hand-edited) records and as a
  defence-in-depth gap in the record API.
- **Repro:** `tests/test_qa_known_failures.cpp` case QA-F4 (direct `StagedPokemon5Record` on a
  synthetic Eevee: stored gender male = PID-derived, ability byte 9, replay PID `0x0009FE00` ->
  female-derived; accepted today).
- **Suggested fix:** check each link independently: refuse when the previous record's gender link
  held and the new PID breaks it, and separately when the ability link held and the new PID breaks
  it.

## QA-F7 (Low, editor gap; FIXED in f17926f): Gen IV Unown letter cannot be edited

- **Found by:** the Gen I–V acceptance matrix (Gen IV fields cell), 2026-10-15; verified against the
  pinned reference.
- **Code:** `src/Pokemon/Pokemon4Mutable.cpp` `setForm`: `value >= base.formCount` with `base` read
  from the Gen IV personal tables (`PersonalInfo4DP/PT/HGSS.cpp`), where Unown (201) has form count 1
  because its letters share one stat entry.
- **Reference:** `vendor/PKSM-Core/source/personal/personal.cpp` `PersonalDPPtHGSS::formCount`
  returns 28 for species 201 ("Normalized to fit with other formCounts' return values").
- **Effect:** in D/P, Pt and HG/SS the shared editor refuses every Unown letter except A, while other
  multi-form species edit normally.
- **Repro:** `tests/test_qa_known_failures.cpp` QA-F7. `createStored` Unown then `setForm(1)` is
  refused in layout groups 77/78/79; Burmy (control) is accepted.
- **Suggested fix:** use the same normalisation as the pinned reference for Unown's form bound. Any
  extra Unown rules (e.g. letter/PID relation) need a citation before they are enforced.

## QA-F8 (Low; review half QA-F8a FIXED in c7b0f97, export-bytes half QA-F8b OPEN): Gen IV no-op release

- **Found by:** the Gen I–V acceptance matrix (Gen IV actions cell), 2026-10-15.
- **Code:** `Gen4StagedPokemonEditor::stageReleaseBoxPokemon` writes an all-zero record; empty box
  slots in a save hold an encrypted blank record (both mean empty). `describeStagedSlotChanges`
  (`Gen4ExportTransaction.cpp`) compares records, so the slot is listed as "(empty) (edited)".
- **Effect:** clone into an empty slot, then release that clone: the review and the export manifest
  show "Box 2 slot 1: (empty) (edited)", and `edited.sav` differs from the source in that slot,
  although the player changed nothing.
- **Repro:** `tests/test_qa_known_failures.cpp` QA-F8b, regression QA-F8a in `test_qa_regressions.cpp` (synthetic D/P save from `qasynth::gen4`).
- **Status:** the review now skips empty→empty slots (c7b0f97, regression QA-F8a). Still open as
  **QA-F8b**: the working copy and `edited.sav` keep the re-encoded empty slot (126 bytes on the
  synthetic D/P save). Suggested fix: when releasing a slot that was empty in the original, restore
  the original slot bytes.

## QA-F6 (Low, identity; owner Thread C; FIXED in 82032a9): Gen III release hint is a substring of the whole path

- **Found by:** the Gen I–V acceptance matrix (Phase 5, `tests/test_qa_matrix_gen3.cpp` identity
  cell), 2026-10-15. Confirmed with a dedicated repro.
- **Code:** `src/Legacy/RetroArchFRLGDiscovery.cpp` `identityHint(path)`. It lower-cases the whole
  path, keeps only alphanumerics, and returns the release whose name occurs ANYWHERE in it
  (`compact.find(needle)`). Ruby/Sapphire and FireRed/LeafGreen saves are byte-identical families, so
  the strict family check cannot catch a wrong hint.
- **Effect:** a valid save with no release in its own name is identified from any directory or word
  that merely contains a release name, e.g. `.../myleafgreenerybackups/game.sav` → Ready
  `leafgreen_gba`. The same hint-less save in a neutral folder is correctly held as
  `AmbiguousIdentity`. This contradicts the project rule that discovery never guesses save
  identity. It is also inconsistent with Gen I (`RetroArchRBYDiscovery.cpp` `identityHint`), which
  accepts only title forms or an *isolated* path token ("/savefiles/red/game.srm" works,
  "redesign.srm" does not).
- **Repro:** `tests/test_qa_known_failures.cpp` case QA-F6 (`make -f Makefile.host qa-known-failures`).
  Observed: status Ready, gameId `leafgreen_gba`. Expected: `AmbiguousIdentity`, as for the control file.
- **Suggested fix:** match Gen III hints like Gen I: title forms ("pokemonruby", "pokeruby", ...) or
  isolated path tokens only.

## QA-F5 (Low, presentation; FIXED on #307): Gen V Home party strip drops form

- **Found by:** Phase 6 cross-generation review of the Home snapshot path, confirmed on the real
  production path (2026-10-15).
- **Code:** `include/Integration/Gen5/Gen5GameCardPreview.h`. `PartyPreviewSlot` has no form field,
  and `homeCardSnapshot` writes `s.party[i]={slot.species,slot.level,0,slot.shiny}`. The Home strip
  renders `SpriteManager::getIconSprite(species, form, shiny)` (`src/UI/SaveSelectScreen.cpp`), so a
  Gen V party member in an alternate form (Basculin Blue-Striped, Deerling/Sawsbuck seasons, Unown,
  Shellos/Gastrodon East, Burmy/Wormadam cloaks) shows the base-form icon. The Gen IV snapshot passes
  `party[i].form()` (`Gen4GameCardPreview.h`), so Gen IV and Gen V disagree.
- **Repro:** `tests/test_qa_known_failures.cpp` case QA-F5. A synthetic Pokémon Black save whose
  party holds Basculin with form 1 (PK5 `0x40 >> 3`, PKHeX `PK5.Form`, `Pokemon5ReadOnly::form()`)
  goes through `inspectSourceFile` -> `LegacySourceBindings` assignment -> `HomeSnapshotCache::refresh`
  -> `openAssignedSource` -> `homeCardSnapshot` (the `SaveSelectScreen` wiring). The snapshot is
  Current with the right species, but `party[0].form == 0`. Reproduces under GCC and Clang with
  ASan/UBSan.
- **Suggested fix:** carry `pk->form()` in `PartyPreviewSlot` and pass it in `homeCardSnapshot`, as
  Gen IV does.
- **Related cross-generation notes (not filed, no repro yet):** Gen I/II and Gen III Home party
  previews in `SaveSelectScreen.cpp` also pass form 0 (`addParty(i, species, level, 0, false)`), so
  a Gen II/III Unown shows the base letter; Gen III also passes level 0. Shiny is shown only for
  Gen V: Gen IV hard-codes `false` with the comment "Gen IV Home shiny badges are not part of the
  accepted card output", and Gen I–III also pass `false`. These are recorded for the Phase 5 matrix
  cross-generation comparison (owner C for Gen I–IV) rather than filed as defects.

## QA-F2 (Medium; FIXED on #307 in 96a44685): invalid workflow files produced a failed run on every push

- **Files:** `.github/workflows/one-time-product-direct-launch-fix.yml` (YAML error line 70),
  `one-time-product-direct-launch-fix-v2.yml` (line 67), `one-time-quick-games-freeze-fix-20261005.yml`
  (line 42). Inline Python heredocs at column 1 break the YAML block scalar.
- **Effect:** GitHub cannot read their `on:` filters, so every push to every branch that carries them
  records a failed run: 1,128 / ~1,123 / 1,049 failed runs so far, including this lane (run
  38643570357), Gen VI (38644385291) and the coordinator branch (38643805374). Every lane shows three
  unrelated red entries, which hides real failures.
- **Repro:** `python3 tests/test_qa_known_failures_assets.py` (QA-F2 line).
- **Proposed fix:** delete the three files (they are spent one-time patch jobs with `contents: write`);
  see [WORKFLOW_CLEANUP_PROPOSAL.md](WORKFLOW_CLEANUP_PROPOSAL.md) §1. Needs owner approval.

## QA-F3 (Low, FIXED in fa9cfcea for Common and Gen V; Gen II open as QA-F3b): export publication is not exclusive

- **Code:** `src/Integration/Common/VerifiedStagedExport.cpp` (`publishVerifiedStagedExport`: unique
  name chosen with `stat()` at line ~167, published with `rename()` at line ~256). Same pattern in
  `src/Integration/Gen2/Gen2ExportTransaction.cpp` (`rename` at line ~232). Gen V
  (`Gen5WorkingCopyExport.cpp`) re-checks `exists()` immediately before `rename()`, so its window is
  much narrower but not closed.
- **Effect:** POSIX `rename()` of a directory silently **replaces an existing empty directory**. If the
  destination appears between the uniqueness check and publication (a concurrent export, another
  tool), the export reports success into that path instead of failing closed or taking the next free
  name. A non-empty destination makes `rename()` fail, so no export data is lost; the defect is a
  non-exclusive publish, not data loss. On the Switch the app is single-process, hence Low.
- **Status:** `fa9cfcea` claims the destination with an exclusive `mkdir` (Common and Gen V). The
  repro is now a passing regression test, `tests/test_qa_regressions.cpp`, extended to a racing
  non-empty directory as well. Gen II's own transaction (Thread 1) still publishes with `rename()`
  (QA-F3b).
- **Original repro (deterministic):** case QA-F3 creates the destination
  from inside the strict validator callback, which the exporter calls after read-back and before
  `rename()`; the export then reports success into the pre-existing directory.
- **Suggested fix (owner decides):** reserve the final name exclusively up front, as Gen V already
  does with its `.pending` reservation: e.g. `mkdir(final)` as the reservation and write the files
  inside a temp directory under it, or re-check plus fail closed if `rename()` would target an
  existing path. `renameat2(RENAME_NOREPLACE)` is not available on Horizon/newlib.

## QA-F1 (Low, FIXED in 3b35b3f): packaged-RomFS manifest did not cover Gen V game cards

- **Code:** `tools/verify_nro_romfs.py` `GAME_CARDS` stops at Gen IV, while
  `src/Games/GameIdentity.cpp` `gameCardArtworkPath()` requests `game_cards/black_nds.png`,
  `white_nds.png`, `black2_nds.png` and `white2_nds.png`.
- **Effect:** today `tools/prepare_release_romfs.sh` copies the Gen V cards into `romfs/`, and the
  verifier's "every local file is packaged" check catches a dropped file, so current NROs are fine.
  But if the prepare step stops copying them, the native gate still passes an NRO without Gen V
  artwork: the defence in depth that exists for Gen I–IV is missing for Gen V.
- **Status:** `3b35b3f` adds the four covers to `GAME_CARDS`. `tests/test_qa_regressions_assets.py`
  now asserts every `romfs:/` literal in the source is in the manifest.
- **Mitigation shipped here:** `tools/qa/nro_audit.py` checks every `romfs:/` literal in the
  application source in addition to the manifest, so it flags this class of gap automatically.
- **Suggested fix:** add the four Gen V ids to `GAME_CARDS` (or derive the list from the source as the
  audit tool does).

## Observations (no defect)

- **Gen II export rewrites the whole secondary copy.** `Gen2StagedEditor` re-synchronizes the
  international Gold/Silver secondary copy (mirror table in `synchronizeInternationalMirrors`) and
  the second checksum (0x7E6D) from the primary on every finalize. With a consistent source only the
  edited field's mirror changes; with a source whose secondary copy had diverged, the whole secondary
  image is replaced by the primary. That is the intended repair-for-strict-reload behaviour, and the
  exported backup preserves the original.
- **Gen V export touches the footer block checksum.** Changing any block's checksum also changes the
  checksum mirror table in the final footer block, whose own checksum then changes. Expected; the
  contract whitelist follows this chain.
- **All-zero PK4/PK5 records** are accepted as blank slots and are never encrypted (documented
  PKHeX/PKSM convention). The fuzz harness exempts them from the re-encryption round trip; the inputs
  are kept as a regression corpus in `tests/fixtures/qa/corpus/fuzz_pk_records/`.
- **No manifest line injection.** The Common exporter does not escape `\n` in manifest fields (the
  Gen V exporter rejects them), but no generation's text decoder produces a newline and the Gen IV
  change lines carry species names only, so no reachable path exists today.

## Fuzzing evidence

Local runs on the audited head (Ubuntu 24.04, GCC 13.3, Clang 18.1):

| Run | Result |
| --- | --- |
| `make qa` under GCC and under Clang (deterministic runner plain + ASan/UBSan, all 8 targets; 17 export contracts; NRO audit regression) | pass |
| libFuzzer 30 s/target (smoke) | clean: 59k / 37k / 1.4k / 0.9k / 0.2k / 607k / 3.8k / 69 execs |
| libFuzzer soak 240 s/target | clean: gen1 545k, gen2 327k, gen3 13k, gen4 11k, gen5-bag 35k, gen5-sav 2.6k, source-containers 733 execs |
| `fuzz_launch_plan` (Thread L launch/return plan, 2026-10-15): deterministic 140k execs ASan/UBSan + libFuzzer 1.75M runs / 180 s | clean: plan argv == libnx tokenisation, loader caps held, hosted argv accepted unchanged by the return host, return host never used unless enabled, return-selection round trip |
| `fuzz_home_cache` (Home snapshot cache, Gen IV/V cards, 2026-10-15): deterministic 280k execs ASan/UBSan + libFuzzer 597k runs / 120 s | clean: unassigned/missing never parse or show an old party; reuse only for same path+binding+stamp; failed re-validation never Current, Stale only with the same assignment's data; a write during the parse forces the next re-parse |
| libFuzzer soak 240 s, `fuzz_pk_records` | first run stopped on an all-zero PK4 record: harness gap (blank-slot convention), fixed, input kept as regression corpus; re-run clean, 4.7M execs |

CI evidence for this branch is linked from the draft PR.

## Candidate NRO audit: Gen I–V hardware candidate `fa9cfcea`

Audited by `app-qa-nro-audit.yml` (job 116229262837, run 38675885758) against the artifact the
candidate run published (run 38673482742, artifact 11945232986), downloaded from that run, not rebuilt.
(The QA container's network policy blocks the Actions artifact storage host, so the audit ran in CI.)

| Field | Result |
| --- | --- |
| NRO | `PokeBank-NX-Gen1-5-Candidate-fa9cfcea.nro` |
| Size | 175,435,441 bytes (matches posted) |
| SHA-256 | `57b4ec2bebf6f15cac0ee59218672fd6acc029c1e025abbe159a322aa1bccd23` (matches posted) |
| Code SHA / tree SHA | `fa9cfceabfecd2afb79d0df935588a3e81e0a7fe` / `5de23dc22b7b2ea60fbada9e45245386e20ae871` |
| Embedded commit | `fa9cfcea` (POKEVAULT_GIT_COMMIT) |
| NACP | PokeBank NX 0.1.0-alpha |
| Packaged RomFS | 4,267 files, content digest `5c636404250499cb`; required manifest (incl. Gen V covers) and every source-referenced `romfs:/` asset present |
| Audit | PASS; record uploaded as artifact 11946126752 |
| Hardware status | **NOT DEVICE TESTED** |
