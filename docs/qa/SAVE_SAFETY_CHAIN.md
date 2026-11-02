# Save-safety chain: Gen I–V coverage (Thread 4)

Chain: **Open → Validate → Working copy → Edit → Review → Export → Verify → Exit.** Every stage of every
Gen I–V export path is checked on synthetic saves through production entry points. Gates:
`make -f Makefile.host qa` (GCC and Clang, plain and ASan/UBSan) and the App QA Contracts workflow.

## Export paths (18)

| Gen | Path (production entry point) | Contract name |
| --- | --- | --- |
| I | `Gen1::publishVerifiedGen1Export` (Pokémon + Bag/PC, Gen I overlay) | `gen1-transaction` |
| I | the same bytes through `Common::publishVerifiedStagedExport` | `gen1-pokemon+bag` |
| I | Bag only, `ClassicInventoryOverlay` | `gen1-bag` |
| II | `Gen2::publishVerifiedStagedEditorExport` | `gen2-trainer` |
| II | Bag through `Common::publishVerifiedStagedExport` | `gen2-bag` |
| III | `Gen3::publishVerifiedGen3Export(editor, request, inventory)` | `gen3-pokemon+bag` |
| III | Bag only | `gen3-bag` |
| IV | `Gen4::publishVerifiedGen4Export` (D/P, Pt, HG/SS) | `gen4-pokemon-*` (3) |
| V | `readNormalizedSourceReadOnly` + `publishGen5WorkingCopy` (PK5 / Bag / Trainer; BW, B2W2; primary and backup copy) | `gen5-*` (8) |

## Stage coverage

| Stage | What is enforced | Where |
| --- | --- | --- |
| Open | Exact game identity, never guessed from a filename alone where the save decides; ambiguous saves held back | `qa-matrix` discovery / identity / linking cells (20 versions); open defect QA-F6 (Gen III path hint) |
| Validate | Strict parsers refuse corrupt, truncated, swapped, forged or out-of-range input without crashing | `qa-fuzz-smoke` + libFuzzer: Gen I–V saves, PK records, Gen V Bag, source containers, Gen V diagnostics |
| Working copy | Staging never touches the source; the staged image carries the edit | `qa-matrix` working-copy + immutability cells; `fuzz_gen5_edits` (outside-copy bytes unchanged) |
| Edit | Every editable field stages and survives finalize + strict reparse; refused edits change nothing; PID-linked Gen V edits keep their links | `qa-matrix` fields / creation / pickers cells; `fuzz_gen5_edits`; regression QA-F4 |
| Review | The review lists the staged changes, and only those | `qa-matrix` review cells; open defect QA-F8 (Gen IV no-op release line) |
| Export | **C1** source bytes and mtime unchanged · **C3** only whitelisted bytes change · **C4** never overwrites (second export, squatting directory/file, racing destination: QA-F3/F3b regressions) · **C5** every injected failure point leaves no partial output | `qa-contracts` (`test_qa_export_contracts`, `test_qa_regressions`) |
| Verify | **C2** the exported `edited` file reparses strictly as the same game; backup equals the source | `qa-contracts` |
| Exit | **C6** Discard All restores the working copy to the original bytes with nothing pending; a later export is refused and leaves no files (all 18 paths refuse) | `qa-contracts` C6 |

Hardware-only parts (SD-card FAT semantics, on-device dialogs, a live emulator writing the same file)
are listed in [GEN1_5_MATRIX.md](GEN1_5_MATRIX.md). Every NRO stays **NOT DEVICE TESTED** until the
owner tests that exact file.
