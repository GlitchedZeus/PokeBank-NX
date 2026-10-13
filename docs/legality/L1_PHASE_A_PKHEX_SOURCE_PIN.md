# L1 Phase A — Source-pin registry for PKHeX-native Gen V/VI static/trade templates

Generated native source: `include/Legality/Gen56PkhexSourceCatalog.generated.h`

**Legal/provenance:** PKHeX.Core © PKHeX contributors, GPL-3.0-or-later. This generated extract derives copyrightable table definitions; retain original licensing and attribution. PokeBank NX is AGPLv3. This is NOT the whole PKHeX encounter engine.

- Immutable upstream Git revision: `66ef5983a5d349efa2dd615f2926a1b1ee97306e`
- Supplied owner's archive SHA-256: `b90420f2dc1bceeadde318601f194a819b049af893e4318243786f04047abd9b`
- The **exact** `EncounterGenerator5.cs` and `EncounterGenerator6.cs` bytes at this revision match the owner source archive's published SHA-256 hashes `cae4866450951cfaa78c1f75e31ef99d7d2dabfa6aac544b9695d5daa7b59bf9` and `ce04375f3b148553b1900d0996ef6a79fbbe31ce0e6d31a0b3a8f0688d49f03e` respectively.
- This does **not** assert the four table files' SHA-256 matches the supplied archive; they are Git-blob pinned to the immutable upstream revision until independently crosschecked against the original ZIP.
- All array names, original row ordinals, game-version masks and static source fields are source-verified. In ORAS the six Cosplay Pikachu templates have dynamically variable encounter locations; those locations are deliberately **not** certified.

| PKHeX table file | Exact Git blob | Extracted rows |
|---|---|---:|
| `Gen5/Encounters5BW.cs` | `9ab0abd941e189261cab2ee302c3a7fd98da5a8b` | 44 |
| `Gen5/Encounters5B2W2.cs` | `9876a9cd700f2fcc337be05741029d1fc88c30f2` | 98 |
| `Gen6/Encounters6XY.cs` | `147d623de132e7210bf8449c40825cd70cb75da0` | 51 |
| `Gen6/Encounters6AO.cs` | `b59214398c756be172107dc09cdff9bf9684ff9c` | 85 |

**Coverage:** 278 static/gift/NPC-trade source templates across 21 arrays. Encounter5/6 **wild encounter area files**, Dream World, Dream Radar, event redemption databases, breeding and exact RNG PID/IV are NOT generated here. They must remain **Unknown / Incomplete**, never invalidated by an empty partial catalog.

**Pipeline:** PKHeX.Core Encounters5*/Encounters6* C# table initializers → named arrays and original index → conservative source-row extract → C++20 constexpr data. Generic `new(...){...}` rows and six original `BaseCosplay with` rows preserved; locations for the cosplay rows are dynamic. Trade positional metadata (OT/TID, language, gender), event payload conditions, flawless IV requirements and source-context RNG must be evaluated separately, not silently assumed.

This lane does not modify the Product UI, native PK5 decoder, accepted Legality::analyze, coordinator PR #298/#301, or original saves.


## Phase A verifiable implementation checkpoint

- `include/Legality/Gen56PkhexOriginalRoute.h` implements the PKHeX `EncounterPossible5`/`EncounterPossible6` **static/NPC-trade table selection order only** for White, Black, White 2, Black 2, X, Y, Alpha Sapphire, and Omega Ruby. Candidate encounters are bounded at 64, original-save/specimen ID and original title must be independently witnessed, with source-only positive and candidate-local negative verdicts. It does not implement all of `EncounterEnumerator5/6` or the binary wild tables.
- `include/Legality/Gen5PkhexLcrngStage.h` adapts exact `LCRNG64.cs` multiplication, forward/reverse step and `MonochromeRNG.cs` Gen V trainer/PID wild high-bit XOR. Source blobs: `e7069e285264dc18e3eaf8942803156b8f101547` and `12ad6a53add1509e8a642b8c501fd42c1390da11`. Matching an **independently recorded original seed** establishes a bounded *raw PID frame only*. Shiny-forcing, gender, special abilities, and encounter-condition mutations are separate, unverified.
- Actual PKHeX `EncounterStatic5.IsMatchLocation` expressly admits **roamer met locations outside the placeholder `Location` table value**. The two pinned BW Tornadus/Thundurus source rows have flag bit 4; those locations cannot become local contradictions merely by differing from literal `25`.
- Actual PKHeX Gen V/VI static egg-source location checks allow later hatch/trade changes. Source rows with `EggLocation` do **not** claim their later met location must remain the original table value. ORAS six Cosplay Pikachu forms also admit variable met locations.
- `tools/legality/l1_verify_pkhex_gen56_sources.py` independently verifies the owner-archive SHA-256 of both generators, four immutable source Git blob hashes, exactly 21 arrays and 278 ordered data tuples; `--source-root` must be supplied with the actual original PKHeX extracted archive, otherwise the full source-parity acceptance gate is **BLOCKED**. `--self-test` is only a parser unit test, not a host PKHeX C# oracle.
- `tests/test_legality_l1_phasea_catalog.cpp`, `test_legality_l1_phasea_original_routes.cpp`, and `test_legality_l1_phasea_rng.cpp` are registered in the 48-suite L1-only host/sanitizer matrix. Full unmodified .NET PKHeX differential oracle, complete original retail save provenance, wild binary table generation, event archives, and original PID/IV conditions remain incomplete.

**Verification limitation:** exact source row parity was checked against all four C# files at the immutable upstream commit; this is source data verification, **not** retail legality parity or successful full repository compilation. No new PK5 decoder was written. Main, coordinator and accepted branches are untouched.
