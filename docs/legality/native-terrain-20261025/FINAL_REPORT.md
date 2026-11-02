# Native terrain repair — October 25, 2026

Published code: `892b0622695563d478b73c3d4425030aad1208a9`; tree `afcc593b4483dfd052053d00eb48ef9f58c3e80e`. [Draft PR #314](https://github.com/GlitchedZeus/PokeBank-NX/pull/314) retains its original integration base. All 49 checks across 24 workflows PASS on this exact code head. Application PR #307 and main are unchanged.

## Production correction

The Gen IV encounter selector and final ground-tile verifier used `groundTile & 31` before indexing encounter flags. Adding 32 to a stored Grass=2 therefore made 34 look like the same tile. The defect also affected PK5 records whose Gen IV origin is checked by the shared analyzer. A valid checksum did not prevent the invalid field from being accepted.

Both paths now use a shared predicate that checks the full byte against the 32-bit flag width before shifting. No high bits are discarded. Existing valid flag indices retain the same interpretation, including the native maximum/sentinel value 24. No encounter resources, oracle code, original fixture expectations or differential exceptions were changed.

## Source evidence

The pinned `pret/pokeheartgold` reconstruction is `9d8b7591f09b65804da2fb2dfd56f320633e0d36`:

- [`BattleSystem_GetTerrainId`](https://github.com/pret/pokeheartgold/blob/9d8b7591f09b65804da2fb2dfd56f320633e0d36/src/battle/battle_system.c#L285) returns a terrain bounded by `TERRAIN_MAX`.
- [`Terrain`](https://github.com/pret/pokeheartgold/blob/9d8b7591f09b65804da2fb2dfd56f320633e0d36/include/constants/battle.h#L76) defines that maximum as 24.
- [`ov12_022567D4`](https://github.com/pret/pokeheartgold/blob/9d8b7591f09b65804da2fb2dfd56f320633e0d36/src/battle/overlay_12_0224E4FC.c#L5224) passes the getter's terrain to the capture metadata setter.
- [`sub_0207213C`](https://github.com/pret/pokeheartgold/blob/9d8b7591f09b65804da2fb2dfd56f320633e0d36/src/pokemon.c#L3721) writes it to `MON_DATA_MET_TERRAIN`. The property is a terrain index; it is not an index modulo 32.

Source files were checked against the pinned Git tree/blob identities and SHA256. [SOURCE_PINS.json](SOURCE_PINS.json) retains all source URLs and identities, including both PKHeX reference versions. No ROM build, hardware capture or historical receipt was authenticated by this analysis.

The reference predicate uses `1 << (int)value` without an index bound. The [C# language specification](https://learn.microsoft.com/en-us/dotnet/csharp/language-reference/language-specification/expressions) defines the shift count for a 32-bit operand using its lowest five bits. Thus 34 selects the same bit as 2. This explains the shared behavior observed in the raw reference runs.

## Regression evidence

The permanent fixture has ten unchanged synthetic positives: one PK4 for each of Diamond, Pearl, Platinum, HeartGold and SoulSilver, and five retained PK5s with those origin games. All ten were clean in production and both pinned references before constructing corrupt copies. One positive preserves terrain 24. Initial development also inspected a legacy HGSS fixture with a pre-existing newer-reference disagreement; it was excluded from this new positive set and its original expectation and unresolved status remain unchanged.

For a directed 80-record set, the ten positives and seven high-bit copies of each were run through the prior engine and freshly built references `6501f0ab46e8f8ca048539dbaf8cae8cb104e722` and `6042efa241a9925ce57e645329ea7f976deabdd3`. All three accepted 63 of the 70 corruptions. The other seven were already rejected by a separate roamer check. The repair rejects all 70 corruptions and preserves all ten positives. Raw oracle results are retained; no parity exception was added to turn their acceptance into a passing differential claim.

`tests/test_gen4_ground_tile.cpp` then checks every byte 25..255 on every fixture through the actual PK4/PK5 analyzers: 2,310 corrupt copies per configuration. It refreshes checksums, requires the specific terrain failure, verifies immutable inputs and repeats each original after corrupt-copy analysis. The CI focused matrix runs this with GCC/Clang, normal and ASan/UBSan, with leak detection and halt-on-error enabled, and preserves the logs, unchanged fixtures and source pins in separate evidence artifacts.

Local validation: all 20 affected configurations PASS (five suites × GCC/Clang × normal/ASan+UBSan, leak detection and halt-on-error enabled). All 212,187 retained results are byte-identical: 50,995 retained PK4s, a prior 25,999-record PK4 holdout now treated as regression data, and 135,193 retained PK5s. The read-only current-app bridge passes all four Gen V titles. [LOCAL_VALIDATION.json](LOCAL_VALIDATION.json) records the command results and hashes. All 49 checks across 24 workflows PASS on published code `892b0622695563d478b73c3d4425030aad1208a9`. [FINAL_VALIDATION.json](FINAL_VALIDATION.json) retains every check/run identity and verified artifact digest. This subsequent WIP-only documentation checkpoint records the completed gates; the PR stays at its passing code head. These constructed corruptions are regression controls, not independent historical specimens or a newly claimed holdout.

## Remaining scope

This addresses a field-validation defect beyond what the two pinned references detect. It does not resolve the earlier 92 Gen IV newer-reference disagreements, nine Gen V ancestry records or missing historical acquisition evidence. Frozen corpora and percentages remain preserved. Five Enigma records still require their own original saves. Full PKHeX parity and historical completeness remain unfinished.

Both successful compiler artifacts include the normal/sanitizer terrain logs, ten unchanged fixtures and exact source pins. All four original CI ZIP SHA256 digests match GitHub, and every ZIP member matches its separately downloaded file. The two full-card artifacts also retain all 81 original files, 63 released rows and zero authenticated receipts.

The 212,187 retained evaluations are regression evidence. The 80 directed records and exhaustive byte mutations are declared construction controls; they do not add independent historical specimens or alter frozen parity denominators. Earlier 92 Gen IV and nine Gen V ancestry disagreements remain unresolved.

Reproduction: `run-focused.sh gcc` and `run-focused.sh clang` execute the five affected suites in both modes. REPLAY_INPUT_MANIFEST.json, REPLAY_RESULT.json, DIRECTED_RESULT.json, REFERENCE_BUILDS.json and the original CI ZIPs retain the exact inputs, identities, outputs and hashes. FINAL_VALIDATION.json consolidates the final checks and artifact verification. CHANGED_FILES.txt and ENGINE_CHANGE.patch describe the change from the prior documentation checkpoint.
