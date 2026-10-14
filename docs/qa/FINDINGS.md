# Application QA findings (Thread 4)

Audited head: PR #307 `claude/gen5-integration-20261014` at `a17c503ad272cc55e69b8b3cbd925cec999567cc`
(contains the #306 Gen I–IV baseline). All inputs synthetic.

Each open defect has a failing reproduction that is kept **out** of every default gate:

```sh
make -f Makefile.host qa-known-failures   # exits non-zero while any defect below is open
```

The C++ cases live in `tests/test_qa_known_failures.cpp`, the tooling/CI cases in
`tests/test_qa_known_failures_assets.py`. Each can be ported verbatim into the owning thread's suite
as the regression test for its fix.

## Summary

| ID | Severity | Area | Owner | Status |
| --- | --- | --- | --- | --- |
| QA-F2 | **Medium** (CI hygiene) | 3 invalid `one-time-*` workflow files fail on every push to every branch | Owner approval; coordinator | OPEN |
| QA-F3 | Low | Common and Gen II export publish can replace a destination created after the uniqueness check | Thread 2 (`Integration/Common`), Thread 1 (Gen II publisher) | OPEN |
| QA-F1 | Low | Packaged-RomFS manifest omits the four Gen V game cards the app requests | Thread 2 (`native-pr-build.yml` gate) / coordinator (`tools/verify_nro_romfs.py`) | OPEN |

No crash, sanitizer report, out-of-bounds read or round-trip violation was found in any Gen I–V parser,
record decoder, Bag decoder, copy selector or Save Instance identification path (see
[Fuzzing evidence](#fuzzing-evidence)). All 15 export paths satisfy the five export-safety contracts.

## QA-F2 (Medium): invalid workflow files produce a failed run on every push

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

## QA-F3 (Low): export publication is not exclusive

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
- **Repro (deterministic):** `tests/test_qa_known_failures.cpp` case QA-F3 creates the destination
  from inside the strict validator callback, which the exporter calls after read-back and before
  `rename()`; the export then reports success into the pre-existing directory.
- **Suggested fix (owner decides):** reserve the final name exclusively up front, as Gen V already
  does with its `.pending` reservation: e.g. `mkdir(final)` as the reservation and write the files
  inside a temp directory under it, or re-check plus fail closed if `rename()` would target an
  existing path. `renameat2(RENAME_NOREPLACE)` is not available on Horizon/newlib.

## QA-F1 (Low): packaged-RomFS manifest does not cover Gen V game cards

- **Code:** `tools/verify_nro_romfs.py` `GAME_CARDS` stops at Gen IV, while
  `src/Games/GameIdentity.cpp` `gameCardArtworkPath()` requests `game_cards/black_nds.png`,
  `white_nds.png`, `black2_nds.png` and `white2_nds.png`.
- **Effect:** today `tools/prepare_release_romfs.sh` copies the Gen V cards into `romfs/`, and the
  verifier's "every local file is packaged" check catches a dropped file, so current NROs are fine.
  But if the prepare step stops copying them, the native gate still passes an NRO without Gen V
  artwork: the defence in depth that exists for Gen I–IV is missing for Gen V.
- **Repro:** `python3 tests/test_qa_known_failures_assets.py` (QA-F1 line).
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
| `make qa` under GCC and under Clang (deterministic runner plain + ASan/UBSan, all 8 targets; 15 export contracts; NRO audit regression) | pass |
| libFuzzer 30 s/target (smoke) | clean: 59k / 37k / 1.4k / 0.9k / 0.2k / 607k / 3.8k / 69 execs |
| libFuzzer soak 240 s/target | clean: gen1 545k, gen2 327k, gen3 13k, gen4 11k, gen5-bag 35k, gen5-sav 2.6k, source-containers 733 execs |
| libFuzzer soak 240 s, `fuzz_pk_records` | first run stopped on an all-zero PK4 record: harness gap (blank-slot convention), fixed, input kept as regression corpus; re-run clean, 4.7M execs |

CI evidence for this branch is linked from the draft PR.
