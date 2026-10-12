# Conditional Platinum/HGSS ordinary egg IV stage

`include/Legality/Gen4EggIVInheritanceStage.h` implements ordinary daycare IV-stage replay and verification from an independently known final RNG state. It is a reusable conditional evidence helper, registered in host and sanitizer tests. It does not add an analyzer verdict or reconstruct parent history from a detached PK4.

## API and scope

Populate `Input` with exact game, two original parent IV arrays **in physical daycare deposit order**, pre-inheritance child IVs, source-attested RNG boundary and availability flags. For HGSS, both collection-time held items must also be known. Use `replay` for an entry state or `verifyFinalState` for a final state plus observed child IVs. The latter rewinds only the known branch's number of calls before replaying.

Supported games are `platinum_nds`, `heartgold_nds`, `soulsilver_nds`. Diamond/Pearl remain Unsupported pending direct confirmation. Unknown parent slots, initial values, HGSS items or RNG state produce explicit Unknown results. Malformed IV values are distinguished from candidate mismatch. A mismatch excludes this supplied construction only; it is not global Invalid. Neither a matching stage nor a supplied availability flag authenticates the external history.

Held item IDs are used solely to choose the six recognized Power-item branches; other numeric IDs follow the retail non-Power switch branch. This helper does not certify item legality, parent eligibility, species/form, gender, PID, event origin or offspring authenticity. Those are separate candidate checks. Platinum does not inspect held items in this stage, so missing item snapshots do not prevent its replay.

## Version behavior

- Platinum consumes six calls and preserves the retail indexed-removal/stale-tail behavior, including repeated stat selection and later overwrites.
- HGSS selects three distinct stats. No Power items consumes six calls; one consumes four; two consume five. Two identical Power items still consume the holder-selection call. Odd selects physical slot0; even selects slot1. Only one item is forced.
- Stats use HP/Atk/Def/Spe/SpA/SpD order. Power Weight=294, Bracer=289, Belt=290, Anklet=293, Lens=291, Band=292.
- LCRNG uses multiplier `0x41C64E6D`, increment `0x6073`; its inverse multiplier is `0xEEB9EB65`. Unknown items mean an unknown HGSS branch length, never an assumed six-draw history.

Do not use move-father/mother order as physical parent order. This implementation is separate from the active `feature/legality-breed002-daycare-order-20261011` fix and does not modify the Gen III joint bridge.

## Provenance

Algorithm independently implemented from the verified research packet [RNG-003](https://github.com/GlitchedZeus/PokeBank-NX/blob/fc0225e6351af62365f29200300323e38f52c8fe/docs/legality/research/packets/RNG-003.md).

- HGSS: [pret/pokeheartgold src/get_egg.c](https://github.com/pret/pokeheartgold/blob/9d8b7591f09b65804da2fb2dfd56f320633e0d36/src/get_egg.c), `InheritIVs`, `_IVList_Remove`, `Daycare_TryGetForcedInheritedIV`, `SetStatIdToPowerItemStat`.
- Platinum: [pret/pokeplatinum daycare.c](https://github.com/pret/pokeplatinum/blob/637bb186e11b5ce1ee578981311c057498e57126/src/overlay005/daycare.c), `Egg_InheritIVs`, `RemoveIVIndexFromList`.
- [Research reproduction script](https://github.com/GlitchedZeus/PokeBank-NX/blob/fc0225e6351af62365f29200300323e38f52c8fe/docs/legality/research/scripts/reproduce_gen4_egg_ivs.py) hosts unchanged functions temporarily against an independent model.

`tests/gen4_egg_iv_stage_vectors.inc` contains synthetic numeric stage inputs/outputs from that research, not commercial game resources or original Pokémon records. No retail source bodies, binary assets or PKHeX modules are imported. The 138 fixtures cover all item pairs at seed 0 plus selected cases at seeds 1, 2, 0x12345678 and 0xFFFFFFFF. Explicit parent and initial-child IVs are stored in each fixture.

## Validation

- All 138 committed fixtures agree on IVs, final state, draw count, individual draws, selected stats and donors. Reverse checks, contradictory observed values, malformed IVs, Unknown inputs, Platinum repeated stats and SoulSilver aliases pass.
- An independent full matrix comparison reran the original retail/model research and compared this C++ implementation across **25,382 cases** (259 seeds × 49 item pairs × two games), with no differences in IVs, final RNG state or draw count.
- New focused host test and existing Gen III IV-stage/joint-egg tests pass.
- ASan/UBSan pass with `ASAN_OPTIONS=detect_leaks=0`. The environment's ptrace prevents LeakSanitizer from running; leak checking was not completed. The helper itself allocates no dynamic memory.
- Full host/RSE and platform builds are not claimed locally; this isolated checkout lacks the optional vendor submodule files. Normal PR CI remains required before acceptance. No accepted branch or main movement is part of this change.

Build the focused normal and sanitizer targets with `make -f Makefile.host HOST_BUILD=/tmp/nx-gen4-egg-test /tmp/nx-gen4-egg-test/test_gen4_egg_iv_inheritance_stage /tmp/nx-gen4-egg-test/test_gen4_egg_iv_inheritance_stage_sanitize`, then run each executable. Full host and sanitizer suites include these targets automatically.
