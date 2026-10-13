# L2 Generation I — native DV, Stat Experience and five-stat source evidence

**L2 lane:** PR #295, branch `feature/legality-l2-breeding-20261012`, unmerged. This new Gen I-only tranche does not rewrite or remove any prior Gen II–IX work and never modifies `src/Legality/Legality.cpp`, MAIN, original saves, or the integrated coordinator.

## Scope and original source pins

**There is no breeding, native gender, nature, modern IV/EV, held item, ability or native shiny flag in Pokémon Red/Blue/Yellow.** None of those absent attributes is a legality rejection. Gen I records store four independent four-bit DVs (Attack / Defense / Speed / Special); the HP DV is recomputed from the respective least-significant bits: `((Atk&1)<<3)|((Def&1)<<2)|((Spe&1)<<1)|(Spc&1)`.

Read-only fields are 33 bytes boxed and 44 bytes party. DV16: big-endian offset `0x1B` (Atk, Def, Spe, Spc nibbles); Stat Experience: five big-endian 16-bit values at `0x11,0x13,0x15,0x17,0x19`; boxed level at `0x03`; party level at `0x21`, cached five party stats at `0x22..0x2B`. Gen I has *one* Special stat, not separate Special Attack and Defense.

| Authoritative existing project source | Verified Git blob before this tranche | Reuse |
|---|---|---|
| `include/Integration/Gen1/Gen1ReadOnlySave.h` | `d52f60cd861b48cab4d577dd702607c72e970a15` | strict native record model |
| `src/Integration/Gen1/Gen1ReadOnlySave.cpp` | `a2445a20a2ede222a2b14e56f87305ef3263484e` | native species mapping, packed field semantics, Japanese/international parser |
| `src/Integration/Gen1/Gen1BattleStats.cpp` | `63b61c8c3c3e53d0d1fd5ffd9b45901c579dc55b` | actual existing stat formula: ceil(sqrt(Stat Exp)) capped at 255, divided by four, five stats |
| `src/Integration/Gen1/Gen1PersonalData.inc` | `399462b48b08cdd9c6bb3f7d398304147fb19cb1` | existing Gen I base stats, PKSM-Core aa22d7a4f87c0351baf7da5962ba5acd01039a7c |
| `src/Integration/Gen1/Gen1StagedPokemonEditor.cpp` | `c18ffe817003bc1ae44526febdae7ddc45137bb8` | real derived HP DV helper (the test links, never writes) |
| `PKHeX.Core/PKM/PK1.cs` `kwsch/PKHeX@66ef5983a5d349efa2dd615f2926a1b1ee97306e` | `a4b692ae7045afb9d11ea3a3b6e25b94c0c4de8b` | independent Gen I PK1 byte offsets/one Special stat reference; GPL-3.0-or-later |

Existing Gen I adapter also cites PKSM-Core `aa22d7a4f87c0351baf7da5962ba5acd01039a7c` and PKHeX `e15d2467b32da7bc26ce7cc8e5c4ede32740e20a`. Preserve their notices and source provenance.

## Newly implemented L2-only helper

`include/Legality/Gen1NativeDVStatEvidence.h`, `src/Legality/Gen1NativeDVStatEvidence.cpp`

- Validates independently attested strict PK1 decoded fields against the immutable raw bytes: internal/national species identity, body size, trainer ID, three-byte experience, current HP, five Stat Experience words, DV packed nibbles and derived HP DV, moves/PP byte field mirrors, level offsets and cached party stat byte mirrors. A malformed decoded wrapper is **InvalidNativeRecord** for that source candidate only; never global Pokémon Invalid.
- Reuses production `StagedPokemonEditor::calculateBattleStats` for level 1–100, all 151 species, five unified Gen I stats. Does **not** install an independent stat calculator.
- A level-zero/glitch source is `UnknownLevelContext`; >100 is outside the project's strict parser contract. Native R/B/Y acquisition game cannot be proven solely by a format wrapper, nor can time-capsule tradeback history.
- Boxed records return conditional native stat fields, **not** a claim that boxed current HP equals current calculated HP.
- Party cache is **UnresolvedPartyCache** by default. A mismatch only becomes `ContradictedAttestedPartyStats` when independently verified to be a standard unmodified recalculation from the exact level and exact Stat Experience at that calculation time. Otherwise changing party stats via game history/battle/stale state is Unknown.
- No repairs, setters, original-save writes, new held items, imaginary breeding rules, or modern attribute checks.

## Regression and honest execution gate

Two production-linked tests: `tests/test_legality_l2_gen1_native_dv_stat.cpp` (all 65,536 DV words, all 151 species at four levels, five Stat Exp boundary sets, boxed/party) and `tests/test_legality_l2_gen1_native_record_adversarial.cpp` (published Bulbasaur/Pikachu vectors, byte corruption, stale cache, level 0/101, absent modern attributes, original buffer immutability).

Added both to `tests/test_legality_l2_run_all.sh`, now **19** native programs. The stand-alone Gen I exact-head CI workflow compiles the **real** `Gen1ReadOnlySave.cpp`, `Gen1BattleStats.cpp` and `Gen1StagedPokemonEditor.cpp` using GCC/Clang normal + ASan/UBSan. Dead-code elimination keeps the dependency graph focused without replacing the production derived HP DV method.

**Local tested subset (not full checkout):** new helper header/CPP and original `Gen1BattleStats.cpp` were byte-for-byte verified against Git blobs; an isolated 262,756-check boundary/DV fixture passed GCC normal, GCC ASan/UBSan, Clang normal and Clang ASan/UBSan (**4/4**, total 1,051,024 executed checks). The isolated harness used a **test-only** PK1 field-header subset, minimal Bulbasaur/Pikachu personal-stat values and internal-species adapter. It therefore does **not** establish full reader integration, all 151 species, or full repository production acceptance. The committed two full tests and expanded L2 runner are **not locally executed**; full GitHub checkout/CI remains required, and no queued/pending workflows count as pass.

**Full checkout commands:**

```bash
bash tests/test_legality_l2_run_all.sh
CXX=clang++ bash tests/test_legality_l2_run_all.sh
```

These run each of 19 L2 suites under normal and sanitizer modes per compiler; final extended L2 acceptance now requires **152** exact-head executions, not the previous 144. Host, native-link, and hardware coverage remain separate gates.

## Known missing evidence

Actual R/B/Y archived party states showing precisely when stat recalculation occurred; independently pinned retail CPU/stat-overflow edge cases; original save/region/tradeback identity proof; authenticated retail specimen parity; cross-check of the full linked native model under all compilers; linkage into production legality analysis (coordinator-owned). Unsupported source histories and absent modern properties must remain Incomplete/Unknown, never globally Invalid.


## Follow-up major tranche: original Gen I EXP thresholds and level provenance

This is **Generation I native legality only**; it does not add or reinterpret breeding, modern EV/IV, abilities, natures, gender, held items, or native shininess.

### Additional source-backed code

- `include/Legality/Gen1NativeExperienceEvidence.h` and `src/Legality/Gen1NativeExperienceEvidence.cpp`: read-only proof for original 24-bit PK1 EXP `0x0E..0x10` and boxed/party level `0x03`/`0x21`. Compares the exact read-only source bytes and decoded fields, consults the EXISTING `StagedPokemonEditor::growthRate(species)` (original Gen I personal table, valid values 0/3/4/5) and `Pokemon::getExpForLevel` / `Pokemon::getLevelFromExp` (shared PKHeX-derived growth tables). There is no new production growth formula and no PK1 parser duplication.
- `Gen1NativeDVStatEvidence.h/.cpp`: exposes the separate experience result and only gates candidate status when `originalExperienceProgressionIndependentlyVerified` has been explicitly supplied. A detached record, unknown species growth identity, glitch provenance or Time Capsule history is **Unknown**, not globally invalid. A level-100 EXP value exceeding the minimum level-100 threshold is **Unknown**: the modern shared calculator's clamp to 100 does not prove ordinary retail origin.
- `tests/test_legality_l2_gen1_native_exp_growth.cpp`: checks all 151 original species at levels 1 through 100, inclusive/exclusive threshold boundaries, 24-bit EXP encoding, malformed wrappers, level-100 saturation, Missing/Unknown proof and opt-in joint DV result behavior.
- `tests/test_legality_l2_run_all.sh` now lists **20** L2 programs; `L2 Stage 7 Final Breeding Legality Acceptance` requires **156** executions (20 original native programs × four compiler/modes plus 19 additional regression programs × four compiler/modes), assuming all required files compile.

Additional original authoritative sources, separately pinned:

| Source | Git blob | Reused field/mechanic |
|---|---|---|
| `src/Pokemon/Experience.cpp`, PokeBank NX at pre-tranche head | `3eb891a7467c28b697e41a7fbe4788b71dce6a22` | exact shared growth thresholds and level lookup |
| `include/Pokemon/Experience.h` | `9002bc6b11a949ba593ed30780c2ca48dfd95c22` | native level/experience API |
| `PKHeX.Core/PKM/Util/Experience.cs` at `kwsch/PKHeX@66ef5983a5d349efa2dd615f2926a1b1ee97306e` | `2e695c078143ae102bfa7fe27c8d2bcdbeea3b99` | independent original source for the level-100 clamp and growth tables |
| `PKHeX.Core/PKM/PK1.cs` at same pin | `a4b692ae7045afb9d11ea3a3b6e25b94c0c4de8b` | 24-bit EXP source byte offsets |

### Verified versus blocked

The new header/implementation and integrated DV header/implementation were exactly copied into an isolated container, and the four resulting Git blob hashes matched the committed PR files. Local strict C++20 compilation (GCC and Clang) with **normal plus ASan/UBSan** executed the previous 262,756-check DV regression and a new 2,196-check EXP threshold/unknown test each of four ways: **264,952 checks per configuration, 1,059,808 checks total**.

**Important:** the isolated test used the exact original `Gen1BattleStats.cpp`, but a reduced two-species read-only PK1 map/personal-data fixture and a test-only implementation of the four original Gen I EXP growth formulae, **not** the committed full `Experience.cpp` object or the full reader. The original full `Pokemon::Experience` and Gen I parser are linked by the CI jobs and full checkout runner, neither of which executed locally. No full 151-species runtime pass or integration pass is claimed until the exact-head GitHub workflow succeeds.

The full-project command remains `bash tests/test_legality_l2_run_all.sh` (and `CXX=clang++ bash tests/test_legality_l2_run_all.sh`). The acceptance gate remains blocked while full-checkout/CI runs are unavailable or pending. This tranche does not touch the production legality analyzer, main UI or original save bytes.
