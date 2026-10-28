# Round 8 automated compatibility and safety matrix

**Status: NOT DEVICE TESTED. Evidence class for every row: SYNTHETIC.** No real save, ROM or key is used or
committed. Synthetic fixtures are built in code (provenance: `tests/fixtures/qa/README.md`,
`tests/fixtures/gen4/README.md`, `tests/fuzz_synthetic_saves.h`, `tests/host-ui/HostUiFixtures.h`). Synthetic
evidence shows that the app's logic accepts, refuses and preserves these inputs as intended. **It does not prove
compatibility with real Switch titles, real emulator files or real cartridges.** That needs real-save evidence,
which is recorded separately below and is currently none.

This matrix is the development team's half of Round 8. The owner's half is Group H of
`docs/hardware/RETEST_<sha>.md` (ordinary saves only). Missing fixtures on the owner's side are
NOT TESTED / NO FIXTURE, never FAIL.

## How it runs

| Gate | Command | Notes |
|---|---|---|
| GCC | `make -f Makefile.host host-test` | every test below |
| GCC AddressSanitizer + UBSan | `make -f Makefile.host host-sanitize` | every test below (`<name>_sanitize`) |
| Clang | `make -f Makefile.host CXX=clang++ HOST_BUILD=build-host-clang host-test` | every test below except `test_ui_switch_title_discovery_host` (the host-UI harness links the app, which is GCC-only) |
| Clang ASan/UBSan | CI `host-tests-clang.yml` | not available in this container (no Clang sanitizer runtime) |
| Matrix only | `make -f Makefile.host round8-compat-matrix` | builds and runs the tests below, plain and sanitized, plus the guard |
| Guard | `python3 tests/test_round8_compat_matrix.py` | fails if any test below leaves HOST_TESTS / HOST_SANITIZE_TESTS |
| Fuzz | `make -f Makefile.host qa-fuzz-smoke` (CI `app-qa-fuzz.yml`, GCC and Clang, libFuzzer on Clang) | malformed-input fuzzing, separate from host-test |

`CXXFLAGS` has no `-DNDEBUG`, so every `assert` is live.

## Matrix

S = success cases, F = deliberately failing/refused cases, B = original/source bytes unchanged,
C = checksum (explicit equality, or strict reparse), E = Export Copy (published copy + `original_backup`).

| # | Scenario (owner does not need these saves) | Tests | S | F | B | C | E | Evidence |
|---|---|---|---|---|---|---|---|---|
| 1 | Bank missing, unreadable/corrupt, newer layout, truncated, damaged records, interrupted save, damaged previous generation, `bank.dat` is a directory | `test_bank_fault_injection` (production `Bank` with injected read faults), `test_bank_load_policy`, `test_bank_format_policy`, `test_bank_recovery_contract`, `test_bank_record_validation` | ✓ first run, healthy reload, interrupted-save restore | ✓ every unusable case blocks `save()` with a reason | ✓ quarantined file byte-identical, folder unchanged | record checksum | n/a | synthetic, isolated `mkdtemp` directories, removed afterwards; never a real SD path |
| 2 | Gen II battery saves padded to 128 KiB: 32/64 KiB core, 0x00/0xFF fill | `test_gsc_gen2_adapter`, `test_gsc_discovery`, `test_gsc_staged_editor`, `test_gsc_export_transaction` (`paddedExport`) | ✓ | ✓ padding+RTC, stray byte, 96 KiB, disturbed padding at export | ✓ | ✓ explicit stored == recomputed, strict reparse | ✓ size, padding and 64 KiB upper half verbatim; `original_backup` == source | synthetic |
| 3 | Japanese regional saves | Gen II: `test_gsc_gen2_adapter` (32 KiB Crystal without gender; Gold, Silver, 64 KiB Crystal editing refused), `test_gsc_inventory`; Gen I: `test_rby_gen1_adapter`, `test_gen1_trainer_edit` (name refused, money with checksum + reparse), `test_rby_pokemon_editor` (editor refused); Gen III: `test_gen3_trainer_edit`, `test_gen3_staged_pokemon_editor` (5-character limit); Gen IV: `test_gen4_staged_pokemon_editor` | ✓ | ✓ | ✓ | ✓ (Gen I) | — | synthetic |
| 4 | Non-English FireRed/LeafGreen Switch releases | `test_frlg_switch_releases` (12 ids: game, language, family, backup namespace, true Move only for English), `test_game_identity`, `test_ui_switch_title_discovery_host` (real `SaveSelectScreen` discovery on a scripted console: each release listed once; other profile, system save, duplicate, non-Pokémon and near-miss ids rejected; fallback spaces) | ✓ | ✓ | n/a (nothing mounted) | n/a | n/a | synthetic title ids; mutation-checked (removing one id fails 4 checks) |
| 5 | Gen III emulator saves with an RTC footer (mGBA / VBA-M) | `test_gen3_trainer_edit` (`rtcFooterExport`: Ruby, Sapphire, Emerald, FireRed, LeafGreen), `test_gen3_staged_pokemon_editor`, `test_pksm_gen3_adapter`, `test_retroarch_frlg_discovery`; Gen II RTC: `test_gsc_export_transaction` | ✓ | ✓ odd footer, tampered footer | ✓ | strict reparse | ✓ footer verbatim, `original_backup` == source | synthetic |
| 6 | FireRed/LeafGreen Meteorite (280); unreleased held items kept | `test_gen3_inventory_byte_fidelity` (Meteorite), `test_gen3_staged_pokemon_editor` (Safari Ball / Berry Juice kept, not selectable) | ✓ | ✓ quantity 2, wrong pocket, new unreleased held item | ✓ | sector checksum bytes only | — | synthetic |
| 7 | Deliberately corrupted saves | `test_gen3_sector_signature_policy` (torn slot), `test_rse_gen3_adapter`, `test_pksm_gen3_adapter`, `test_rby_gen1_adapter`, `test_gen1_export_transaction`, `test_gsc_gen2_adapter`, `test_gen4_readonly_foundation`, `test_gen4_staged_pokemon_editor`, `test_gen5_readonly_save`, `test_gen5_export_validation`; fuzz targets in `qa-fuzz-*` | ✓ fallback to the good slot/copy | ✓ | ✓ | ✓ | Gen I/II/V | synthetic |
| 8 | Gen II/III Bag byte fidelity | `test_gen2_inventory_byte_fidelity`, `test_gen3_inventory_byte_fidelity` (refusals: `test_gsc_staged_editor`, Meteorite block) | ✓ | ✓ | ✓ only the slot + checksum change | strict reparse | — | synthetic |
| 9 | Gen III Egg names; Gen V species change keeps PID gender | `test_gen3_staged_pokemon_editor`, `test_gen5_species_pid_gender` | ✓ | ✓ | ✓ staged bytes unchanged on refusal | — | — | synthetic, record level |
| 10 | Gen II OT name limit | `test_gsc_pokemon_editor` (7 accepted, 8 refused, Edit and Create), `tests/test_gen2_ot_prompt_limit.py` (prompts offer 7; run by the guard) | ✓ | ✓ | — | — | — | synthetic |

<!-- round8-matrix-tests:begin -->
test_bank_fault_injection
test_bank_load_policy
test_bank_format_policy
test_bank_recovery_contract
test_bank_record_validation
test_gsc_gen2_adapter
test_gsc_discovery
test_gsc_staged_editor
test_gsc_export_transaction
test_gsc_inventory
test_rby_gen1_adapter
test_gen1_trainer_edit
test_rby_pokemon_editor
test_gen3_trainer_edit
test_gen3_staged_pokemon_editor
test_gen4_staged_pokemon_editor
test_frlg_switch_releases
test_game_identity
test_ui_switch_title_discovery_host
test_pksm_gen3_adapter
test_retroarch_frlg_discovery
test_gen3_inventory_byte_fidelity
test_gen3_sector_signature_policy
test_rse_gen3_adapter
test_gen4_readonly_foundation
test_gen5_readonly_save
test_gen5_export_validation
test_gen1_export_transaction
test_gen2_inventory_byte_fidelity
test_gen5_species_pid_gender
test_gsc_pokemon_editor
<!-- round8-matrix-tests:end -->

## Real-save evidence

| Scenario | Real-save evidence |
|---|---|
| All rows above | **None.** No real Japanese, padded, localized-Switch, RTC, corrupted or event-item save has been tested. |

If a specific real sample is ever needed for final validation, the team will ask for it by name. Private or
licensed saves are never committed to GitHub.

## Known gaps (recorded, not hidden)

- Export Copy is not yet exercised for: Meteorite edits, unreleased held items, the accepted Gen III Egg edit,
  and the Japanese Gen I/III/IV money edits (they are checked through `finalizedBytes` and strict reparse).
- No Japanese Gen V save test. Gen V species/PID gender is checked at record level, not through a save.
- Localized FireRed/LeafGreen: discovery and identity are covered; parsing a save opened under a localized
  title id is not separately tested (the save format is the shared FireRed/LeafGreen format).
- `test_rby_staged_inventory`, `test_gen3_staged_inventory`, `test_gsc_universal_inventory` and
  `test_classic_inventory_catalog` run only in `Makefile.host.classic_inventory` (no sanitizer build).
- Gen II fuzz seeds include no padded or Japanese image.
