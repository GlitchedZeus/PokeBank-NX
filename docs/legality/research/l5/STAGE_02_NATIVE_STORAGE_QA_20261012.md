# L5 Stage 2 — MAJOR native storage and source-identity adversarial QA

**Date:** 2026-10-12. **Scope:** existing PR #294, branch `audit/legality-l5-research-qa-20261012`. No production or shared CI changes, no original saves accessed or written.

## New executable implementation

1. `tests/fixtures/legality_l5_native_storage_adversarial.cpp`: standalone **independent** Gen I–IV native record and source-identity verifier/test model. Git blob `6804d18a3331c57cd9e1b618b8c292bcc5949cb4`, exactly matched against the locally executed file. Runs **1,057,328 assertions** covering:
   - Exact PK1 33/44, PK2 32/48, PK3 80/100, PK4 136/236 body-length domains; every wrong length in 0..260.
   - Gen I internal list markers; strict Gen II species 1..251, marker/body parity, 0xFD egg marker; Gen I/II PokéList terminators and stale post-sentinel slots.
   - Gen I/II packed DVs, HP DV derivation, PP Ups over all 65,536 16-bit values per native gen, both box and party.
   - Native source known, missing, unknown or contradictory; no invented current save/game identity and no automatic global Invalid from absent provenance.
   - PK3/PK4 **decrypted canonical** core checksums, single-bit tampering, zero species and checksum ordering, PK4 sanity, EV byte 0/1/252/253/254/255, and input immutability.
   - **Model limitation:** two known Gen I internal species IDs, not the complete national mapping; this is deliberately independent source-model testing, not the production save parser.

2. `tests/fixtures/legality_l5_native_codec_adversarial.cpp`: exercises **ACTUAL committed production** `Encryption::encryptArray3FRLG` / `decryptArray3FRLG` / `checksum3FRLG` and `Encryption::encryptArray4` / `decryptArray4` / `checksum4` and PK4 size/blank APIs:
   - All 24 Gen III and 32 Gen IV shuffle classes, box/party formats, native EV edge bytes, egg flags, multiple trainer/PID values, exact encrypted roundtrip, checksum and source input immutability.
   - Exhaustive selected encrypted core single-bit corruption: **83,968** mutations rejected by existing production checksum logic.
   - **56 party-only checksum blind-spot controls:** party-only bytes are outside PK3/PK4 core checksum and thus cannot be declared corrupt solely from that checksum; expected native format, not a production flaw.
   - Entire fixture yielded **203,292 actual C++ assertions PASS**. Committed fixture Git blob **`dd7fb8ad1303e92a942b4b159ae261c43775892d`** matched local test source.

3. `tools/audit_legality_l5_stage2_native.py`: opt-in fail-closed reproducible runner. Default `--full` compiles/executes both fixtures under GCC optimized and GCC ASan+UBSan; `--model-only` is labeled **BLOCKED_NOT_EXECUTED** for the production gate. `--source-pin` requires the precise source Git blobs below. All binaries are built in a temporary directory; no source modification.

## Source and executable verification — ACTUAL LOCAL TESTS

From the isolated local compile directory (not a full repository checkout), the **production C++ source bytes** were checked with `git hash-object` and matched the live GitHub branch:

| Git blob | Inspected source |
|---|---|
| `eec27b73a15d6652371ed2e066690ef0fa9fb7ef` | `src/Encryption/Encryption3FRLG.cpp` |
| `406935761713db95ae738398b65ba0a37d4cf97b` | `src/Encryption/Encryption4.cpp` |
| `dd7fb8ad1303e92a942b4b159ae261c43775892d` | committed native production test |
| `6804d18a3331c57cd9e1b618b8c292bcc5949cb4` | committed independent native model |

The isolated compile used minimal **compatible interface headers** and the exact-byte production `.cpp` implementations, because the full checkout was not locally available. It is a source-pinned production-crypto functional test, **not a complete application build** or original-retail save parser execution.

Actual local executions:

| Target | GCC 14.2 normal, C++20 O2 Werror | GCC 14.2 ASan+UBSan O1 | Clang 17 O2 |
|---|---|---|---|
| Independent Gen I–IV model | **PASS: 1,057,328** | **PASS: 1,057,328** | **PASS: 1,057,328** |
| Exact production PK3/PK4 encryption C++ source | **PASS: 203,292** | **PASS: 203,292** | **PASS: 203,292** with `-Wno-unknown-warning-option` |

Initial Clang `-Werror` attempt rejected two GCC-specific diagnostic pragmas in existing `Encryption3FRLG.cpp` (`-Wstringop-overflow` and `-Wrestrict`); rerun with compiler-specific warning-compatibility flag passed without modifying production code. GCC source and sanitizer tests passed without the exception.

## Stage 2 QA finding and release boundaries

- **Test-model correctness fixed before commit:** an intentionally corrupted PK3/PK4 native species byte turning 1 → 0 originally bypassed model checksum verification as an empty slot. The final committed model verifies checksum/sanity before distinguishing empty species. Production `Legality.cpp:604` separately early-returns on species 0; any concern requires verifying the upstream native save-parser validation boundary, **not** declaring a proven production defect.
- **Native party-stat corruption:** PK3 and PK4 party-only data is not covered by stored core checksum. The 56 controls establish the intended limitation of core checksums. A future higher-layer structural validation may detect other contradictions; do not convert the checksum blind spot into a historical Invalid assertion.
- **Source identity:** an unrecognized/missing/wrong-generation container label cannot overwrite native format capability or prove original encounter chronology. PK1/PK2 missing abilities/genders remain covered by Stage 1; fresh full analyzer tests still require coordinator integration.
- **No confirmed new production false-Invalid from Stage 2.** No original retail history, paired before/after source, authenticated GameCube/Mystery Gift, full PKHeX differential parity, or proof for the 336 game/family cells.

## Coordinator replay on full reconciled checkout

```sh
python3 tools/audit_legality_l5_stage2_native.py --full --source-pin
python3 tools/audit_legality_l5_stage2_native.py --model-only --clang
# Existing Stage 1 production PK1/PK2 source identity (coordinator F3/F4 guards)
make -f Makefile.host -f tools/audit_legality_l5_pk12_probe.mk l5-pk12-fixed
make -f Makefile.host -f tools/audit_legality_l5_pk12_probe.mk l5-pk12-fixed-sanitize
```

The **new Python runner's exact full-checkout invocation remains UNEXECUTED here** due inaccessible complete repository checkout; do not conflate it with the separately executed C++ source-pinned tests. The coordinator owns production `Legality.cpp` changes and CI registration; L5 did not edit them.

## Prioritized next acceptance

**P0:** Coordinator replay exact checkout full Stage 2 native runner normal/sanitized and Stage 1 200-context native analyzer; verify both full-link headers and current PR #298 exact head. **P1:** strict PK1/PK2 battery-save parser checksum/region/RTC, empty list and noncanonical text native specimens with original provenance; strict PK3/PK4 serialization across save/provider adapters. **P1:** do not call checksum outside covered ranges proof of tamper. **P2:** independent actual original-retail source pairs and legally usable pinned PKHeX comparison fixtures, then Stage 3 differential program.

All new code stays under `tests/fixtures/legality_l5_*`, `tools/audit_legality_l5_*` and `docs/legality/research/l5/**`. Existing PR #294 remains OPEN/DRAFT, no merge, no rebase/reset/force push, no original-save writes.
