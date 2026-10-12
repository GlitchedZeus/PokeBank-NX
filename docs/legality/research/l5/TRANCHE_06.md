# L5 major tranche 6 — 251,904 single-bit corruption trials and production native codec harness

**Issue:** [L5 #290](https://github.com/GlitchedZeus/PokeBank-NX/issues/290) / [master #285](https://github.com/GlitchedZeus/PokeBank-NX/issues/285).
**Draft PR:** [#294](https://github.com/GlitchedZeus/PokeBank-NX/pull/294), `audit/legality-l5-research-qa-20261012`.
**Authoritative accepted analyzer and crypto pin:** `feature/legality-engine-gen1-4-20260929@d29eada5c087e201e00c749703f1a2de3ddfadeb`. Never reset/rebase backward, force-push, self-merge, or overwrite a live advancing lane.

## Work performed and actually executed

Previous tranche produced **336 checksum-valid, independently generated synthetic encrypted Pokémon-format records**: 144 PK3 across all 24 Gen III shuffle permutations, 192 PK4 across all 32 Gen IV shuffle permutations, each with EV byte 0/1/252/253/254/255, balanced egg/non-egg tags. An additional 12 ciphertext records have deliberately damaged checksums.

In this tranche, an **independent JS decoder read the exact fixture JSON from the GitHub L5 branch** and actually performed:
- 336 valid original synthetic ciphertext decoding and checksum/EV/egg verification;
- 12 separate damaged ciphertext checksum rejections;
- for every valid PK3, bit-flipping **every one of the 48 ciphertext data bytes × 8 bits = 384 mutations**: 144 ×384 = **55,296 checks**;
- for every valid PK4, bit-flipping **every one of the 128 ciphertext data bytes ×8 bits = 1,024 mutations**: 192 ×1,024 = **196,608 checks**;
- **251,904 single-bit ciphertext corruptions in total, zero unexpectedly passed checksum**, all in memory, restoring the exact immutable source ciphertext bytes after each test. Elapsed research JavaScript execution approximately **5.65 seconds**.

Every separate perturbed input was actually decoded and checked, not just counted algebraically. `Q16_ENCRYPTED_TAMPER_SWEEP.json` preserves exact corpus Git blob `2252161521120c1db34b2700bdceeaf2d0859ffa`, fault model, breakdown, execution-scope semantics and blockers.

**Proven scope:** Source-faithful independent JavaScript implementation of Gen III native 32-bit XOR/unshuffle/checksum and Gen IV checksum-seeded 16-bit LCG XOR/unshuffle/checksum, run against *exact GitHub-committed synthetic ciphertext*. This is **not** an executed build of NX's accepted `Encryption3FRLG.cpp` / `Encryption4.cpp`, **not** a real save ingested by NX's Pokémon models, and **not** a `Legality::analyze` or PKHeX behavioral parity test.

**Scope exclusions:** The modeled faults flip one ciphertext **data-region bit** without updating the header or stored checksum. This does not test multiple-bit changes, deliberate checksums recomputed after malicious editing, arbitrary header/size corruption, other source providers, actual encounter/move legality, or saved-game provenance.

## New C++ production cryptography regression

`tests/fixtures/legality_l5_native_codec_batch.cpp` is a read-only C++ program that directly links the **accepted C++ native encryption implementations**:
- `src/Encryption/Encryption3FRLG.cpp`, accepted original Git blob `eec27b73a15d6652371ed2e066690ef0fa9fb7ef`;
- `src/Encryption/Encryption4.cpp`, accepted original Git blob `406935761713db95ae738398b65ba0a37d4cf97b`;
- `include/Encryption/Encryption.h` pins Gen III/IV shuffled block table.

It streams the exact 348 fixture rows from Python, decrypts the raw native records using the real project functions, verifies checksum/sanity, original PID, permutation, EV, egg flag, species, and **encrypts back byte-for-byte** to the exact input ciphertext for every checksum-valid record. It requires the 12 damaged records to fail checksum. It then repeats **all 251,904 individual ciphertext bit faults** through those real native C++ decryption routines, expecting every fault to fail checksum; original input is never mutated. It also strictly validates native lengths and metadata.

`tools/audit_legality_l5_native_codec.py` is the independent no-write host runner: it checks 336/12 corpus and all IDs, compiles actual production C++ crypto with `g++`, and invokes normal or ASan/UBSan builds. `--source-pin` also hashes **five actual accepted source files** using Git blob SHA-1, failing closed if someone changes the assumed implementation. Eight additional malformed input tests require *the explicit native input-rejection message*, rather than treating a 348-record count failure as sufficient rejection evidence. `ASAN_OPTIONS=detect_leaks=0` is used when appropriate for containerized sanitizers; ASan/UBSan still execute.

From a coordinator-approved, full, **forward reconciled** repository checkout:

```bash
python3 tools/audit_legality_l5_native_ev_vectors.py --source-root .
python3 tools/audit_legality_l5_native_codec.py --source-pin
python3 tools/audit_legality_l5_native_codec.py --source-pin --sanitize
```

Expected native crypt results: **348** input fixture status checks, **336** identical native encrypt/decrypt round trips, **12** damaged-checksum rejects, **251,904** single-bit native decrypt/checksum rejects, and **8** malformed CLI input rejects in each normal/sanitized run. Expected status is **NOT CLAIMED until that C++ build has actually executed**.

## Full actual production legality analyzer is a separate step

Prior `tests/fixtures/legality_l5_ev_native_analyzer_probe.cpp` and `tools/audit_legality_l5_ev_probe.mk` add **36 distinct synthetic native PK3/PK4 `Legality::analyze` assertions** (2 generations ×2 source contexts ×6 EV cap boundaries; 2×2×3 unhatched egg EV boundaries). These use actual accepted Pokémon parsers and, for PK3, `clone()` roundtrip. Baseline and post-fix modes distinguish:
1. **False-Invalid EV hard-cap rule:** native Gen III/IV permit EV 253–255 per stat (510 total) while current NX incorrectly uses 252;
2. **Missing unhatched egg EV check:** pinned PKHeX rejects nonzero EV on unhatched egg, but accepted NX's inspected EV section does not have equivalent direct logic.

The crypto regression and actual analyzer regression are **different executable steps** and must not be conflated. The 36-case real analyzer has not been run on the latest branch, nor have the coordinator-owned production F1/F2 patches landed through L5.

## Research and release acceptance

- Authoritative original saves remain immutable; no save writes or licensed game/event payloads are committed.
- The current synthetic corpus proves format/single-bit checksum detection, not original Pokémon capture/breeding/event history.
- Run exact-head full host, focused RSE and ASan/UBSan under GitHub Actions with non-skipped commit identity, plus native codec and analyzer probes; file changes must remain on current draft L5 lane.
- Coordinator owns forward-only reconciliation to the newer accepted engine, any production EV/egg checks and final decision, without losing newer commits or merging umbrella PR #103/Main.
- Missing source histories and incomplete candidate universes remain **Incomplete/Unknown**; a failed one-route hypothesis never proves an impossible Pokémon globally.
