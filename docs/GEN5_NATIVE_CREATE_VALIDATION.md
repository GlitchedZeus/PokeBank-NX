# Native Create execution report — 2026-10-13

Integration base and final pre-push GitHub refresh:
`5745f200bbc3e442bcee92e0b3945e4eb401c260` (PR #300 still open).
Compilers: GCC 16.2.1 (20260810), Clang 22.1.8; C++20, warnings as errors.

## Executed new-code tests

| Configuration | Dependencies | Actual result per execution |
| --- | --- | --- |
| GCC `-O2` | untouched PR #300 | PASS: 10,135 creation cases / 795,840 checks |
| Clang `-O2` | untouched PR #300 | PASS: 10,135 / 795,840 |
| Clang `-O0 -g3 -D_GLIBCXX_ASSERTIONS` | untouched PR #300 | PASS: 10,135 / 795,840 |
| Clang ASan+UBSan `-O1 -g` | untouched PR #300 | PASS: 10,135 / 795,840 |
| GCC optimized / debug / ASan+UBSan | coordinator-patched headers | PASS in all three: 10,135 / 795,840 |
| New synthetic SAV5 integration, GCC optimized / ASan+UBSan | coordinator-patched headers | PASS in both: 16 save scenarios / 8,386,744 checks |
| New synthetic SAV5 integration, Clang optimized / ASan+UBSan | coordinator-patched headers | PASS in both: 16 / 8,386,744 |

Commands are in `GEN5_NATIVE_CREATE.md` and `Makefile.host.gen5-create`.
Sanitizers used `ASAN_OPTIONS=detect_leaks=0` and
`UBSAN_OPTIONS=halt_on_error=1`; LeakSanitizer was not claimed or tested.
Separate output directories prevent accidental compiler/configuration reuse.

The 10,135 accepted cases cover every species 1..649 in each of four titles at
levels 1/50/100, all 25 natures, all 32 shuffle selectors, every move ID 1..559 and
all supported PP Ups, all six growth curves, all 30 Erratic thresholds 69..98,
normal/hidden abilities, native forms/title differences, fixed/dual/genderless
species, ASCII language/name boundaries and deterministic ciphertext. Additional
rejection cases cover wide integer inputs, invalid IV/EV/PP/form/PID/gender,
missing trainer/history, unsupported histories/party/text, and corruption. Every
request is compared with its snapshot on success and failure.

## Existing tests actually executed

- Untouched `test_gen5_pk5_readonly`: optimized GCC and Clang ASan+UBSan PASS.
- Coordinator-patched `test_gen5_pk5_readonly`, `test_gen5_readonly_save`, and
  `test_gen5_staged_save_candidate`: all three PASS under optimized GCC and
  GCC ASan+UBSan, then all three PASS under Clang ASan+UBSan.
- `test_source_mutation_policy`: optimized and ASan+UBSan PASS.
- Python contracts `test_gen5_readonly_ui_contract.py`,
  `test_ci_repository_safety_contract.py`,
  `test_engineering_authority_contract.py`, and
  `test_save_instance_architecture_contract.py`: PASS.
- Broad `make -f Makefile.host host-test`: first attempt blocked by uninitialized
  pinned PKSM-Core dependencies; after initializing the recorded submodules,
  the canonical run reached the confirmed Gen V compile defects and failed.
- A subsequent **diagnostic** `host-test` completed successfully: 115 suite
  executables plus prerequisite contracts. It used the untouched checkout for
  other tests, substituting the already-built coordinator-patched save-reader
  and save-candidate executables with the `GEN5_SAVE_TEST` and
  `GEN5_EXPORT_TEST` make-variable overrides. This is not an unpatched full-gate
  pass. The entire repository sanitizer gate was not run; affected native tests
  and source-protection tests were run under sanitizers as listed above.

Diagnostic full-host command (run from the Create checkout):

```sh
make -f Makefile.host -j3 \
  GEN5_SAVE_TEST="$(realpath --relative-to=. /tmp/pokebank-gen5-create-coordinator-check/build-host/test_gen5_readonly_save)" \
  GEN5_EXPORT_TEST="$(realpath --relative-to=. /tmp/pokebank-gen5-create-coordinator-check/build-host/test_gen5_staged_save_candidate)" \
  host-test
```

The disposable checkout was based on the exact integration SHA above; the only
changes there are the three supplied coordinator patches. No warnings were
suppressed in successful patched runs. An early diagnostic with
`-Wno-error=missing-field-initializers` still failed on redeclarations and is
not counted as a pass.

## Confirmed upstream defects and remaining gates

1. Duplicate `copySize` and `stale` declarations, an unused local `Slot` alias,
   and omitted pending-certificate aggregate fields block warnings-as-errors
   builds. Two pointer-truth static assertions additionally fail GCC debug or
   ASan+UBSan compilation. Patch 0001 fixes these without disabling warnings.
2. Erratic EXP has a misplaced integer floor: 20 of levels 69..98 differ from
   the pinned original reference table. This also makes native level-70 EXP
   display as level 69. Patch 0002 corrects shared progression; the new Create
   API already uses an isolated correction.
3. Save regression assertions still expected Species/Ability/Held Item to be
   non-editable. The current shared contract explicitly makes them editable.
   Patch 0003 corrects only those expectations; source-write and Create gates
   stay closed. The patched complete save regression passes.

The isolated branch does not apply those shared-file patches. Until the
coordinator accepts them, canonical GCC debug/sanitizer and full host gates
remain blocked upstream. The Create foundation itself is usable/tested with
Clang on the unchanged baseline.

No devkitA64 compiler was available on PATH or at `/opt/devkitpro`; `DEVKITPRO`
was unset and the Switch makefile preflight stopped with its missing-toolchain
error. No Switch compilation, NRO, hardware validation, executable PKHeX parity,
or GitHub Actions pass is claimed.

Party Create, egg/event/transfer provenance, non-ASCII text, automatic native
species naming, held-item/dependent forms and shared Create/Keep/Review/export
wiring remain gated. See the explicit coordinator integration checklist in
`GEN5_NATIVE_CREATE.md`. This is not legally-obtainable Pokémon generation.

Only new files are delivered. Original emulator/installed saves were never
opened for writing. New native tests operate solely on app-owned byte vectors;
existing export tests use temporary synthetic fixtures. MAIN, PR #300, accepted
legality and the other development lanes were not modified or merged.
