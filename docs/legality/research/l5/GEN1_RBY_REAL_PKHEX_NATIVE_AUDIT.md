# L5 Gen I Red/Blue/Yellow — Original PKHeX.Core Native Parity Audit

Date: 2026-10-12. Independent L5 QA only; PR #294 draft and unmerged.

## Source SHA checkpoints re-fetched before work

- Accepted legality: `d29eada5c087e201e00c749703f1a2de3ddfadeb`, `src/Legality/Legality.cpp` Git blob `b1d02d6620e8fffca2cf71874b786d44108e193f`.
- Current coordinator #298: `ad9c1c859f94c0321a5d17489c72b0e7dbb9f440`, production analyzer Git blob `4ac8a0dc0badb8211eb105811eb3fa9f928e36e4`.
- L5 initial head `574388921920694c27f8b7ed27d65b3f723ea430`.
- Source projects during this investigation: L1 `3fdb0d87da3399869e7e6d0720d609460b05bbbd`, L2 `6fdca6ef613f85e1bb01cfa245a94c1398205d29`, L3 `74bb7d89fe265dc18ecc0aea509cc6bb3d1f505f`, L4 `d9e92ced72133ffddfe33ad49ae718ba69f21d42`.
- Genuine original PKHeX source: `kwsch/PKHeX@6501f0ab46e8f8ca048539dbaf8cae8cb104e722`; Git blobs `PKHeX.Core/PKM/PK1.cs=a4b692ae7045afb9d11ea3a3b6e25b94c0c4de8b` and `PKHeX.Core/Legality/LegalityAnalysis.cs=b7f5ca8534e426ad3bf1d30de6f7f7ca57544f3c`.

## Source-audited F3 and F4 discrepancies

**L5-F3**: On the *accepted/L5* analyzer, absent sourceProfile activates modern ability validation for native PK1/PK2; the source-free PK1 wrapper correctly returns neutral 0. This is a false global Invalid irrespective of valid original move/encounter provenance.

**L5-F4**: On the *accepted/L5* analyzer, absent sourceProfile activates modern species gender-ratio validation for native PK1; its neutral gender 2 represents an absent field, not actual Gen I genderless classification. This produces a false Invalid for ordinary Bulbasaur. Magnemite provides a genuine native genderless species control.

**Current coordinator has already made the needed source-level changes**: ability gate requires native generation != 1 and != 2; gender gate requires native generation != 1. Do not repeat F3/F4 as *unfixed coordinator defects* merely because accepted baseline has them. Separate full production analyzer executions are the acceptance proof, not source text alone.

Original PKHeX `LegalityAnalysis.ParsePK1` runs GB-specific nickname, level, trainer, miscellaneous, move PP and effort checks; it does **not** run modern ability or gender verifiers. PKHeX `PK1` exposes `Ability=-1` as an absent value; NX uses neutral `ability=0`. PKHeX convenience `Gender` is DV-derived from its shared GB adapter; Gen I does not natively store gender, and NX intentionally leaves `gender=2` neutral. **Neither numerical representation should be turned into a Gen I legality finding.**

## Genuine executable differential work added

- `tools/audit_legality_l5_gen1_pkhex_oracle/Program.cs` plus `.csproj`: link and **execute the genuine original pinned** `PKHeX.Core` .NET 10 project. Direct calls to `new PK1` and `new LegalityAnalysis`, preserving `CheckIdentifier.Ability/Gender` with actual Severity.Invalid results. No translated or synthetic PKHeX parser is used.
- `tests/fixtures/legality_l5_gen1_rby_oracle_cases.json` and matching `.inc`: exactly **72** deterministic source-agnostic native PK1 comparisons (Bulbasaur/Magnemite; DV16 0, 0x5555, 0xffff; Tackle 33 and non-native move 200; missing, Red, Blue, Yellow, wrong-generation and unknown source labels). The artificial origin labels must not be treated as actual attached original-game source proof.
- `tests/fixtures/legality_l5_gen1_rby_actual_native.cpp`: link **real accepted or coordinator PK1 read-only wrapper and production `Legality::analyze`**. Check F3/F4 source-free baseline, corrected coordinator behavior, incompatible foreign source warnings, real native Gen I move ceiling negative control, HP DV and four base DVs, and no mutation of original source saves.
- `tools/audit_legality_l5_gen1_rby_native.mk`: L5-only build sidecar used in separate, pinned accepted and coordinator checkouts. Tests normal, ASan/UBSan, and separately retained Gen II native gender/ability cases plus Gen III–IV native ball-domain analyzer regressions.
- `tools/audit_legality_l5_gen1_rby_live_diff.py`: fail-closed **three-real-executable** differential. It refuses a source model, empty/canned/duplicated observations, mismatched input IDs, different source fields, DV disagreements or unsupported full parity promotion. Reports exact accepted-versus-original-PKHeX false-Invalid emissions and insists the coordinator no longer emits them. Never counts synthetic move learnability or a PKHeX global Valid result as a verified original event history.
- `.github/workflows/l5-independent-gen1-rby-real-pkhex.yml`: exact commit source pin verification, .NET 10 PKHeX.Core build and 72 live oracle queries, actual accepted baseline NX and corrected coordinator binaries, normal/ASan+UBSan, 72-vector JSONL differential plus Gen II–IV regression gates. **A committed CI workflow is not proof its jobs ran**.

```sh
# In a checkout with all four exact pinned trees and an SDK for .NET 10:
dotnet build tools/audit_legality_l5_gen1_pkhex_oracle/L5Gen1PKHeXOracle.csproj -c Release
dotnet tools/audit_legality_l5_gen1_pkhex_oracle/bin/Release/net10.0/L5Gen1PKHeXOracle.dll \
  tests/fixtures/legality_l5_gen1_rby_oracle_cases.json > /tmp/original-pkhex.jsonl
# Native actual PK1 builds use the independent Makefile sidecar in separate
# accepted and coordinator checkouts, with normal and sanitizer modes.
python3 tools/audit_legality_l5_gen1_rby_live_diff.py --self-test
```

**Acceptance remains BLOCKED until the exact-head Actions logs show original .NET PKHeX, actual accepted NX and coordinator binaries, ASan/UBSan and neighboring Gen II–IV suites all executed successfully.** The project currently has no locally clonable full checkout (GitHub DNS failure), no installed .NET 10 host SDK, and no authenticated retail original save witness. Never claim a passed live oracle comparison without actual runtime logs.

## Original-history 21-cell Red/Blue/Yellow matrix

See `Q23_GEN1_RBY_21_CELL_COVERAGE.json` for **3 retail versions × 7 legality categories**: encounters, four-move history, DVs/stats, gift/events, NPC trades, metadata, and verdict correctness.

Source-researched versus independently executed native verdicts are separate. All 21 rows are **Blocked/Unknown** for complete original game-history parity. Even an exact output match on 72 synthetic PK1 cases does not distinguish a real Red capture from Blue or Yellow: Gen I PK1 has no original-game version field (`PK1.Version == GameVersion.RBY`). Version-specific slots, TM/HM chronology, original gifts, NPC trades and 3-way Time Capsule histories need independently authenticated original game/source, encounter time, training choices, and candidate exhaustiveness.

**No production `Legality.cpp` edits, writes to original saves, coordinator/main merges, or third-party PKHeX binaries vendored into the project.** GPLv3 PKHeX reference is downloaded as separate exact-version test-only checkout.

## Major L5 continuation — independent Stat Experience and all-151 native regression

Following the original 72-vector baseline, L5 added actual native-format
field-boundary testing and comprehensive *source-independent* Gen I species
coverage, without changing coordinator production legality code.

- **Verified directly via local GCC 14.2 and Clang 17 compilers:** four C++20
  variants with `-Wall -Wextra -Wpedantic -Werror` (optimized and
  ASan/UBSan for both), each **PASS 1,377,425 assertions**. Overall
  **5,509,700 executed native-format assertions**, zero observed sanitizer
  diagnostics. The `-DNDEBUG` compilation was intentionally rejected.
- **Exactly executed source:** `tests/fixtures/legality_l5_gen1_rby_vector_integrity.cpp`
  original committed Git blob `925bf4b680c403b07d5da98bfe19dbb23f57882a`.
  Locally reconstructed original vector files were verified byte-identical
  to the GitHub blobs, not retyped approximations:
  - `legality_l5_gen1_rby_oracle_cases.json`:
    `3f65ffe0d00219db94cc65804112bc615d4efbb0`.
  - `legality_l5_gen1_rby_oracle_cases.inc`:
    `7b449e587d2ace8a6ca319d1ca35f20a7a71241e`.
- The 72 native vectors now exercise **all five independent PK1 stat experience
  fields** with deliberately rotated distinct values `0,1,32768,65534,65535`,
  not five aliased copies. Each is checked through the actual original
  `PKHeX.Core/PKM/PK1.cs` `EV_HP/ATK/DEF/SPE/SPC` properties **when that
  executable runs**, and actual `Pokemon1ReadOnly` native
  `statExpHP/ATK/DEF/SPE/Special` getters **when NX CI executes**. Gen I
  Stat Experience has 16-bit bounds; modern 252/510 EV restrictions must
  never be applied. No native abilities, gender field, nature, held item
  or breeding information is inferred.
- Species growth-sensitive synthetic level-5 EXP fields are `135` for
  Bulbasaur and `125` for Magnemite, reducing unrelated level-verifier
  noise in the real oracle.
- **New full-native coordinator regression not locally runnable:**
  `tests/fixtures/legality_l5_gen1_all_151_sourcefree_native.cpp`.
  Source-map inspection on original coordinator
  `src/Integration/Gen1/Gen1ReadOnlySave.cpp`, Git blob
  `a2445a20a2ede222a2b14e56f87305ef3263484e`, confirmed a
  **complete unique inverse mapping of all 151 species** from 256 native
  internal IDs. The L5-only production-linked QA binary requires
  **1,812 cases** (151 species × 6 exact/unknown/mismatched source labels
  × 2 native move states) for correct Gen I no-ability/gender treatment,
  native Gen I move ID 200 Invalid, no modern-EV false Invalid, and explicit
  source mismatch warning. Normal and ASan/UBSan are mandated.
  This was **committed to CI**, not executed or passed in the local container.

### Execution/acceptance accounting

| Evidence | Actually executed? | Scope |
|---|---|---|
| GCC optimized, GCC ASan/UBSan, Clang optimized, Clang ASan/UBSan | **YES, PASS** | 5,509,700 standalone native DV+Stat Experience assertions |
| Original game internal species map, 151 unique IDs | **YES, source table inspected** | Complete static 256-entry mapping, not linked analyzer |
| Fixed `uint8_t` native move narrowing under GCC/Clang strict compilation | **YES** | Reproduced compile failure and passing corrected cast |
| Original .NET 10 PKHeX.Core host, 72 paired vectors | **NOT YET VERIFIED** | Must finish exact-head Actions job |
| Actual accepted NX + coordinator PK1 `Legality::analyze` normal/ASan | **NOT YET VERIFIED** | Must finish exact-head Actions job |
| Coordinator all-151 species × six source contexts × two move classes | **NOT YET VERIFIED** | Must finish exact-head Actions job |
| Gen II-IV linked side-effect controls | **NOT YET VERIFIED** | Must finish exact-head Actions job |

The prior source-reviewed F3/F4 findings do **not** become newly confirmed
runtime results because this local machine lacks a full GitHub checkout
and .NET 10. Any pending, queued, skipped, cancelled or failing CI gate
remains **BLOCKED**, not Passed. Full original PKHeX R/B/Y method parity
also requires independently authenticated original game/source context,
valid multi-move and trade histories and full event/archive witnesses.
