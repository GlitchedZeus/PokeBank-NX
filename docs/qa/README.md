# PokeBank NX application QA (Thread 4)

Independent QA for the Main Development application: parser robustness fuzzing, export-safety
contracts, exact-head NRO auditing, and CI for all of it. This lane owns new files only
(`tests/fuzz_*`, `tests/test_qa_*`, `tests/fixtures/qa/**`, `tools/qa/**`, `Makefile.host.qa`,
`.github/workflows/app-qa-*.yml`, `docs/qa/**`) plus one `include Makefile.host.qa` line in
`Makefile.host`. It never edits production `src/` or `include/`; defects go to the owning thread with
a failing regression test (see [FINDINGS.md](FINDINGS.md)).

Every input is synthetic and built in code. No real save, ROM or key is read or committed.

## Quick start

```sh
git submodule update --init --recursive vendor/PKSM-Core   # nested memecrypto is required
make -f Makefile.host qa                       # fuzz smoke + export contracts + Gen I-V matrix + tooling (CI gate)
make -f Makefile.host qa-matrix                # Gen I-V acceptance matrix only (docs/qa/GEN1_5_MATRIX.md)
make -f Makefile.host CXX=clang++ qa           # the same under Clang
make -f Makefile.host CXX=clang++ QA_FUZZ_SECONDS=300 qa-libfuzzer-run
make -f Makefile.host qa-known-failures        # open-defect repros; exits non-zero while any is open
                                               # (fixed ones move to test_qa_regressions*, inside `qa`)
```

`qa*` targets are opt-in; the default `host-test` / `host-sanitize` graph is unchanged.

Header dependencies: host rules list only their `.cpp` prerequisites, so `Makefile.host.deps` (included
last by `Makefile.host`) wraps every `$(CXX)` call in `tools/qa/cxx_with_deps.sh`, which records the
headers each output was built from in `build-host/.hdrdeps/`. `tools/qa/host_header_sweep.py` runs when
the Makefile is parsed and deletes any output older than one of its headers, so a header-only change
rebuilds every dependent test locally the way CI's clean build does. Regression:
`tests/test_qa_host_header_deps.py` (in `qa-tools-test`).

## 1. Parser robustness fuzzing

One driver, [`tests/fuzz_support.h`](../../tests/fuzz_support.h), runs each target two ways:

- **libFuzzer** (`clang -fsanitize=fuzzer,address,undefined`, `make qa-libfuzzer`), seeded from the
  same synthetic builders (`make qa-seeds`).
- **Deterministic corpus/mutation runner** (any compiler, GCC included; plain and ASan/UBSan).
  Strategies: truncation, extension, bit flips, interesting bytes, out-of-range counts, swapped
  primary/backup regions and footers, and block splices. Every mutant also runs a second time with
  its checksums **re-forged** (block CRCs, sector sums, diff8, PK4/PK5 checksums), so mutations get
  past the first checksum gate into the structural parsers.

Contract on every input: **reject cleanly, or accept and round-trip** (accepted bytes retained
verbatim, re-parse identical, caller buffer never mutated, a no-op staged editor finalizes to the
same bytes). The runner also fails if a built-in seed is not accepted, so a broken seed can never
quietly turn a target into rejection-only fuzzing.

| Target | Entry points the app uses |
| --- | --- |
| `fuzz_gen1_sav` | `Gen1::parse` (R/B/Y) and its strict export validator, `decodeInventory`, `Pokemon1ReadOnly` (PK1), Gen I staged Bag and Pokemon editors |
| `fuzz_gen2_sav` | `Gen2::parse` (G/S/C, RTC footers) and its validator, `decodeInventory`, `Pokemon2ReadOnly` (PK2), Gen II staged editor |
| `fuzz_gen3_sav` | native `Gen3::parse` (all five games, the adapter the Switch build compiles) and its validator, PK3 party/box enumeration, Bag pouches, both staged editors |
| `fuzz_gen4_sav` | `Gen4ReadOnlySave::parse` (DP/Pt/HGSS, with and without assignment), partition selection, PK4 records, `decodeReadOnlyBag`, staged editor |
| `fuzz_gen5_sav` | `Gen5ReadOnlySave::parse` with the **copy selector** (Automatic/Primary/Backup), **Save Instance identification** (`probeNormalizedBattery`), PK5 records, `decodeBag5`, `Gen5StagedBag`, `decodeTrainer5` / `Gen5StagedTrainer`, staged export candidates after a real Trainer and Bag edit |
| `fuzz_gen5_bag` | `decodeBag5` on raw Items blocks + an operation tape driving `Gen5StagedBag` set/add/remove (exactly one 4-byte slot may change per accepted op) |
| `fuzz_pk_records` | raw PK3 (`Pokemon3FRLG`, `decodeRecord`), PK4 (`Pokemon4ReadOnly`, all groups), PK5 (`Pokemon5ReadOnly`); valid records must re-encrypt to the exact input |
| `fuzz_gen5_edits` | Gen V staged edit SEQUENCES through `Gen5StagedPokemonWorkspace` (every field incl. PID-coordinated Gender/Shiny/Ability/Personality, names, clone, remove, forged field ids) then the export candidate: refused ops change nothing, PID/gender links and earlier-origin PIDs are preserved, the export reproduces the staged slots exactly |
| `fuzz_source_containers` | on-disk identification: Gen IV `readSourcePayloadReadOnly` / `inspectSourceFile`, Gen V `readNormalizedSourceReadOnly` / `inspectSourceChoices` on raw and DeSmuME `.dsv` containers; the file's bytes and mtime must not change |

Layout constants used by seed builders and forgers come from the production headers or from the
accepted fixtures that already pin them (cited in [`tests/fuzz_synthetic_saves.h`](../../tests/fuzz_synthetic_saves.h)).

## 2. Export-safety contracts

[`tests/test_qa_export_contracts.cpp`](../../tests/test_qa_export_contracts.cpp) drives every export
path exactly as its UI caller does: Gen I Pokemon+Bag and Bag, Gen II Trainer and Bag, Gen III
Pokemon+Bag and Bag, Gen IV (DP/Pt/HGSS), Gen V PK5, Bag and Trainer Name/Money (BW/B2W2, primary
and backup copy).
For each path:

- **C1** source file bytes, size and mtime unchanged, including across failed exports and when the
  export root is the source's own directory;
- **C2** the published edited save strictly re-parses as the same exact game (same Gen V copy) and
  the published backup equals the source;
- **C3** only whitelisted bytes change (the edited field/record/block, its checksum words, and
  format-defined mirrors: Gen II secondary copy, Gen V checksum mirror table);
- **C4** a second export never overwrites the first; a directory or file squatting on the exact
  destination is left untouched;
- **C5** every injected failure point, and a plain-file export root, leave no file and no
  temporary/pending directory behind.

## 3. Exact-head NRO audit

```sh
python3 tools/qa/nro_audit.py PokeBankNX.nro --expect-sha <40-hex code SHA> \
    --artifact pokebank-nro-artifact.zip --ci-run <run URL> --record HARDWARE_TEST_RECORD.md
```

Prints NRO size, SHA-256, code SHA, tree SHA, whether the build embedded the expected commit
(`POKEVAULT_GIT_COMMIT`), NACP title/version and the packaged RomFS digest. It reuses
`tools/verify_nro_romfs.py` for the ASET/RomFS walk and manifest check, adds a check that every
`romfs:/` asset the application source requests is packaged, verifies a CI artifact copy is
byte-identical, and writes a hardware-test record whose status is always **NOT DEVICE TESTED**.
`--expect-nro-sha256` / `--expect-size` check the file against the identity posted for it.
Regression: `tests/test_qa_nro_audit.py`.

[`app-qa-nro-audit.yml`](../../.github/workflows/app-qa-nro-audit.yml) runs the audit in CI against
the exact artifact a candidate run published (downloaded from that run, never rebuilt). It has NO
defaults (QA-F9): a pull_request run reads the candidate identity only from
[`CANDIDATE_UNDER_AUDIT.env`](CANDIDATE_UNDER_AUDIT.env), and a dispatch run only from its required
inputs. Every value is bound to GitHub's run and artifact records, and on pull_request the candidate
must be built from the head under test, with no application file changed since. When a new candidate
is published, update every line of that file together.

## 4. CI

- [`app-qa-fuzz.yml`](../../.github/workflows/app-qa-fuzz.yml): per PR, the deterministic smoke run
  (GCC and Clang, plain + ASan/UBSan) and a 30 s/target libFuzzer run. Nightly / dispatch: 10 min
  per target with libFuzzer, plus a longer deterministic soak with a per-run RNG stream. Crash
  inputs are uploaded as artifacts.
- [`app-qa-contracts.yml`](../../.github/workflows/app-qa-contracts.yml): export contracts under
  GCC and Clang with ASan/UBSan, the Gen I–V acceptance matrix ([GEN1_5_MATRIX.md](GEN1_5_MATRIX.md),
  tables in the step summary), plus the NRO audit regression.

## 5. CI hygiene

[WORKFLOW_CLEANUP_PROPOSAL.md](WORKFLOW_CLEANUP_PROPOSAL.md): proposal only. Nothing is deleted
without owner approval.
