# CLAUDE-READY ENGINEERING HANDOFF

Research snapshot: 2026-10-18. Main base `bd6528149c41a2cf444e0a62afcaf5fc1bad2cad`.

Evidence vocabulary: **ORIGINAL** = inspected original project's source; **INDEPENDENT** = separate implementation or experiment (not game acceptance); **PKHEX** = inspected PKHeX source; **NX-TEST** = executed PokeBank test; **INFERRED** = proposed design/unverified consequence; **UNKNOWN** = evidence missing; **UNSUPPORTED** = deliberately disabled. These are distinct, not a single confidence score. Every production/hardware acceptance claim in this package is UNKNOWN unless explicitly attributed to historical evidence.

Start here. The research branch contains **no runtime integration**, no source saves, and no vendor modifications. Research is usable now; full retail-format and legality certification is not complete. Tuesday, October 20 is an owner decision after Gen I–V acceptance, not an automatic start date.

## Decisions that save implementation time

1. **Reuse #309** native Gen6PK6, SAV6 layout/parser, staged session, Bag, stats, providers and independent export verifier. Create/Add Pokémon and Trainer staging are missing. Do not recreate its decoder.
2. **Reuse #311** PK7 alias of the Gen VI codec, strict SAV7 parser, stats, Bag, readouts and **existing staged session**. The PR body underreports this work. Export remains UNSUPPORTED.
3. **New signing defect evidence:** `reverseCrypt(zero)` leaves output untouched in both GCC and Clang experiments; MemorySanitizer independently reports uninitialized input at vendor `memecrypto.c:218`. The caller in PKSM-Core uses an uninitialized output array. Thus “PKSM's nonzero tail” is not a trustworthy format requirement. See [the full investigation](GEN7_SIGNATURE_INVESTIGATION.md). A single retail tail observation alone does not prove export acceptance.
4. **Reuse #312** SwishCrypto/SCBlock and PK8 strict parser. It has no Gen VIII staged session. Preserve complete unknown blocks; do not treat `ChecksumsValid => true` in PKHeX as permission to bypass the container SHA-256.
5. **#301 contains actual detached PK6/PK7 readers**, not just transport headers: `Gen6NativePK6Evidence.h`, `Gen7NativePK7Evidence.h`, record analysis sources, oracle tests. They duplicate some format-lane crypto. Its production wiring audit currently FAILS. Reuse as differentials first, not as a wholesale merge.
6. Existing shared/native classes already cover LGPE, SWSH, BDSP, PLA, SV and Z-A. They are useful readers, not export safety proofs. Z-A is **PA9/SAV9ZA**, not PK9. PA9 box slot is `0x158 + 0x40`; party stride adds `0x88` in current PKHeX.
7. Current exact-head CI is **not all green**: #307 NRO audit fails at candidate identity; #314 Gen V differential fails; #301 foundation fails. Do not declare Gen I–V accepted from passing build jobs.

## First implementation actions after authorization

- Fetch current main/#307/#314/#309/#311/#312/#301 again. Use [EXISTING_CODE_AUDIT](EXISTING_CODE_AUDIT.md) SHAs only as this audit's evidence. Refresh workflow conclusions; never reset to these SHAs.
- On the authorized app integration lane, forward-integrate reviewed Gen VI files with current shared editor contracts. See [file map](SHARED_EDITOR_INTEGRATION.md). Do not merge the old stacked app history from #312 merely to obtain its new files.
- Complete exact-game catalogs, Create and Trainer staging with verified byte masks; strict reparse, full diff containment, original hash unchanged. Reuse common Bag/Move/Species presentations and Review/Discard.
- Treat SAV7 signing and vendor hygiene as separate gates; do not enable export because a self-signed synthetic file verifies.
- Follow stages A–I in [CLAUDE_IMPLEMENTATION_ORDER](CLAUDE_IMPLEMENTATION_ORDER.md), with distinct LGPE/BDSP/PLA/ZA models.

## Working research assets

- `metadata/pkhex-index.json`: exact source hashes/URLs, all five Gen VI/VII block tables, current save-size constants, selected SC keys, record property offset navigation index. A property offset is not a width/mask specification.
- `tools/generate_metadata.py`: deterministic pinned-source extractor; changed revision is refused.
- `tools/format_probe.py`: bounded read-only BDSP revision+MD5 diagnostic and record-range checks. Integrity is not game validity.
- `tools/signature_probe.c`, `tools/run_signature_probe.py`: reproducible reverseCrypt sentinel experiment and two payload signing/verification tests against unchanged vendor code.
- `metadata/transfer-policy.json`: conditional HOME graph, explicitly not production transfer permission.
- `tests/test_research.py`: structural synthetic BDSP integrity/tamper/revision, bounded ranges, metadata invariants, transfer guard tests.
- [TEST_RESULTS](TEST_RESULTS.md): actual commands, compilers, outcomes and limitations. No native build or device acceptance is claimed.

## What another engineer must still obtain

Private/local device diagnostics, not committed saves: family/patch, source manager/version, file lengths, all-block CRC result, signature verification, payload-tail category excluding the trailing SHA1 digest, and before/after source hashes. For BDSP capture both filenames through repeated manual/autosave cycles and controlled private restore tests; timestamps or filenames alone are not authority. For all families obtain game-load and save/reopen evidence on expendable copies after native build gates.

Read [RISKS_AND_BLOCKERS](RISKS_AND_BLOCKERS.md) before enabling any write-capable adapter. This package narrows unknowns; it does not authorize the Gen VI–IX transition.

Additional high-value defect: the Gen VII Bag reader rejects Z-Ring x2 despite a per-item exception in both pinned PKHeX revisions. Use the isolated `reproduce_gen7_zring.cpp` and repair per-item quantity policy without enabling story-item Add. Quantity metadata extraction now covers nine family schemas, with default caps explicitly distinguished from per-item exceptions.
