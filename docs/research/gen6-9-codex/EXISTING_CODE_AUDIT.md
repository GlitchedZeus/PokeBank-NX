# Existing code and branch audit

Research snapshot: 2026-10-18. Main base `bd6528149c41a2cf444e0a62afcaf5fc1bad2cad`.

Evidence vocabulary: **ORIGINAL** = inspected original project's source; **INDEPENDENT** = separate implementation or experiment (not game acceptance); **PKHEX** = inspected PKHeX source; **NX-TEST** = executed PokeBank test; **INFERRED** = proposed design/unverified consequence; **UNKNOWN** = evidence missing; **UNSUPPORTED** = deliberately disabled. These are distinct, not a single confidence score. Every production/hardware acceptance claim in this package is UNKNOWN unless explicitly attributed to historical evidence.

## Authoritative heads observed

| PR | Exact head | Decision |
|---|---|---|
| #309 | `5e6836902555da4431be16348ef21abec0358d13` | Reuse native Gen VI lane; repair integration gaps |
| #311 | `2438b5d0b9294193a9de9c8f14736c59ebbb90a5` | Reuse read/stage code; preserve export refusal |
| #312 | `2f065ee2b0768d01bf621f969aef01120c6231e3` | Reuse SWSH container/PK8; add staging later |
| #301 | `809a31841fc2ad43dda02d4412cb9891f29a482f` | Reuse detached evidence readers/tests with license and wiring review |
| #307 | `d104310da30d8095931ca16e097a2f6e2652ddfe` | Current shared app reference; do not modify |
| #314 | `1ceea5634d1bfc251087727aa9444ec0a1109305` | Current legality reference; do not modify |

Raw metadata and workflow conclusions are in `evidence/github-snapshot.json`. Branch inventory was fetched; PR search results are preserved. #309 is reported merge-conflicting against its **current** base. #311 and #312 are stacked; that does not mean their older application histories are safe integration bases.

## #309: ORIGINAL, NX-TEST

Files `include/Integration/Gen6/Gen6PK6.h`, `Gen6SaveLayout.h`, `Gen6Save.h`, `Gen6StagedSession.h`, `Gen6Stats.h`, `Gen6PersonalData.h`, `Gen6Bag.h`, `Gen6BagCatalog.h`, `Gen6Readouts.h`, `Gen6ExactFormatEditorProvider.h`, `Gen6SaveSources.h`, corresponding `src/Integration/Gen6/*.cpp`.

Strict PK6 size/sanity/Add16, XY/ORAS size+BEEF+block CRC+version identity, Trainer readout, Party and 31×30 boxes. Invalid boxes are quarantined. Stage field/text edits, clone to box, remove from box, party stat refresh, staged Bag add/remove/quantity. Whole-save verifier permits only authorized record masks, pouch bytes and affected CRCs. Export copies via `Integration/Common/VerifiedStagedExport` to Gen VI root. Source files open read-only; fingerprint/identity must survive assignment/reopen.

Missing: Pokémon Create/Add, full party placement/removal semantics, Trainer editing, native Makefile registration, Save Instances and shared UI registration, exact-game move/form filtering beyond range limits, actual emulator/device acceptance. Amie, ribbons and other unexposed bytes must be preserved; binary availability is not editable support. Personal AO is cited for PK6 stats; game availability still needs XY-specific filtering.

## #311: ORIGINAL, NX-TEST

`Gen7Save`, `Gen7SaveLayout`, `Gen7StagedSession`, `Gen7Stats`, `Gen7PersonalData`, `Gen7Bag`, `Gen7BagCatalog`, `Gen7Readouts`, `Gen7ExactFormatEditorProvider`, `Gen7SaveSources`. PK7 reuses `Gen6PK6.h` under a namespace alias for the accessed common layout; do not assume all unimplemented PK7 fields are identical. SM/USUM size/footer/all CRCs, signature RSA/AES/SHA1 verification and SHA256 table binding. Party and 32×30 boxes, trainer read-only. Stage edits/clone/remove and Bag changes produce **UnsignedCandidate**, with CRC repair and stale original signature preserved. No production export. Readouts include names/unlocks/current box/dex.

Five Gen VII binaries exist, not one. Stats test applies Hyper Training. Existing documentation's numerical completion percentages are not adopted here. LGPE research is already in `docs/gen7/LGPE_RESEARCH.md`; it is a distinct PB7 family.

## #312: ORIGINAL, NX-TEST

`Gen8SwishCrypto.h/.cpp`, `Gen8PK8.h`, `Gen8SaveLayout.h`, `Gen8Save.h/.cpp`; test `test_gen8_swsh`. Salted SHA256 checked before parsing; repeating 0x7F xorpad, key-seeded XorShift32, strict type/subtype/length validation, duplicate key rejection and exact payload consumption. PK8 0x148/0x158, party format stored in boxes. Eight SWSH size histories; status game ID, Party and Box blocks. Party entries below count must decode; bad boxed records quarantined. Battle team lock availability is tracked so later clone/remove can fail closed. No Gen8 staged session/export/editor provider yet.

## #301: newer work hidden by an old PR body

[include/Legality/Gen6NativePK6Evidence.h](https://github.com/GlitchedZeus/PokeBank-NX/blob/809a31841fc2ad43dda02d4412cb9891f29a482f/include/Legality/Gen6NativePK6Evidence.h) performs actual Decrypt67, sanity/checksum/species/nature/EV inspection without I/O or heap allocation. [include/Legality/Gen7NativePK7Evidence.h](https://github.com/GlitchedZeus/PokeBank-NX/blob/809a31841fc2ad43dda02d4412cb9891f29a482f/include/Legality/Gen7NativePK7Evidence.h) reuses it with the USUM ceiling. `Gen6RecordAnalysis.cpp` and `Gen7RecordAnalysis.cpp` exist, alongside detached native tests and pinned PKHeX oracle tooling. `Gen56NativeEnvelopeEvidence` deliberately does not claim decryption. `Gen59FormatDomainEvidence` is conservative domain evidence, not complete encounters or transfer history.

Local execution: domain, PK6 and PK7 standalone regressions pass; `audit_legality_gen59_wiring.py` FAILS with `mandatory production guard missing: Kind::LetsGo`. Exact-head CI also fails. Do not report #301 as accepted, and do not add another decoder. The detached readers explicitly mark GPL-3.0-or-later adaptation; keep provenance when using them.

## Other existing implementations and conflicts

Current #307 has `Pokemon7LGPE`, `Pokemon8SWSH`, `Pokemon8BDSP`, `Pokemon8LA`, `Pokemon9SV`, `Pokemon9LZA`; family encryption and Trainer/Inventory classes; `LGPEReadValidation`, `SCReadValidation`, `BDSPReadValidation`, `PLAReadValidation`. These mostly consume mutable legacy buffers. Reuse verified field semantics or wrap readers; **replace legacy persistence routing with staged verified export** in a future lane. Do not enable legacy injection.

Duplications: shared `Encryption.cpp` SC container versus Gen8SwishCrypto; modern record crypt versus Gen8PK8; detached legality PK6 reader versus Gen6PK6. Decision: differential first, one owner per write-capable codec; expose immutable normalized evidence to legality. Do not delete older readers during integration without proving equivalent malformed-input behavior.

Unique work: Gen6/7 lane files are not in #307 or main; Gen8 adds its own new files on top. #301 readers are separate from #314's current integration. Later shared editor/picker/quantity/launch changes on #307 supersede assumptions in the stacked lane. File-by-file migration is safer than merging its entire old base. This is an engineering recommendation, not a merge performed here.

Focused missing test: SM/USUM Z-Ring count2 is supported by both pinned PKHeX PlayerBag implementations, but `Gen7Bag::decodeBag7` rejects it using the pocket cap1. This research branch includes a reproducible defect test, not a production fix. Existing 102-check Gen7 Bag suite passes without covering this exception. Reuse the Bag implementation with that focused repair; do not replace it.
