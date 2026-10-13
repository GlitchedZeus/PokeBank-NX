# L2 Generation I — Original Stat Experience Battle Replay and Arithmetic

**Branch / lane:** L2 draft PR #295, feature/legality-l2-breeding-20261012. No production analyzer, MAIN, Gen II–IX edits, merges, or original save writes.

## Source-pinned original retail mechanics

- Original Gen I engine: pret/pokered@af519899719f0754965776faac0e836a3b906e6d, engine/battle/experience.asm, Git blob f9f47f82573771b108df6db4dacecb634a156a24.
- GainExperience/.gainStatExpLoop adds defeated species' five original base stats to the recipient's five 16-bit Stat Experience values, saturating each at 65535. DivideExpDataByNumMonsGainingExp splits awards among multiple recipients, and Exp. All changes the share. Do not replay a full award when these conditions are not independently proven.
- Project personal table: src/Integration/Gen1/Gen1PersonalData.inc, Git blob 399462b48b08cdd9c6bb3f7d398304147fb19cb1, pinned PKSM-Core aa22d7a4f87c0351baf7da5962ba5acd01039a7c.
- Existing original species map: Gen1ReadOnlySave.cpp Git blob a2445a20a2ede222a2b14e56f87305ef3263484e. Original production five-stat calculation: Gen1BattleStats.cpp Git blob 63b61c8c3c3e53d0d1fd5ffd9b45901c579dc55b.
- Stat Exp contributes floor(min(255,ceil(sqrt(exp)))/4). First contribution at exp=10; maximal contribution of 63 starts at 63002. R/B/Y stores FOUR DVs and ONE Special. HP DV derives from four low bits. No breeding, native abilities, gender, nature, held item, or modern EVs.

## Implemented source and regression

- include/Legality/Gen1OriginalStatExpBattleHistory.h and src/Legality/Gen1OriginalStatExpBattleHistory.cpp: bounded 16-event original battle history; independently attested origin, monotonic event ordering, source Red/Blue/Yellow game, original 5-stat initial and per-event snapshots, one recipient and no Exp. All; saturating base-stat awards using the existing Gen I table. Unobserved/multi-recipient/Exp. All histories remain Unknown/Incomplete. Contradictions are historical candidate-local, never invented global Invalid.
- tests/test_legality_l2_gen1_original_stat_exp_battles.cpp: 151 original species x three Gen I games x boxed/party x eight starting Stat Exp configurations. Valid one- and two-battle transcripts, saturation, duplicated identity, absent evidence, incomplete coverage, raw-record corruption.
- tests/test_legality_l2_gen1_stat_exp_math_boundaries.cpp: 151 species x four levels x four stored DV patterns x ten Stat Exp root boundaries; plus the complete 0..65535 Stat Exp domain for Pikachu HP and Special. Uses original existing battle-stat code; no alternate production calculator.
- Both new programs are registered in tests/test_legality_l2_run_all.sh and the existing L2 Gen I GCC/Clang sanitizer workflow. There are 22 native programs, and the expanded exact-head Stage7 acceptance requires 164 four-mode executions including 19 additional fixture programs.

## Executed tests and limitations

| Native source-exact fixture | GCC normal | GCC ASan/UBSan | Clang normal | Clang ASan/UBSan |
|---|---:|---:|---:|---:|
| Attested original battle history | 137,715 pass | 137,715 pass | 137,715 pass | 137,715 pass |
| Five-stat integer-root mathematics | 203,569 pass | 203,569 pass | 203,569 pass | 203,569 pass |

Total: **1,365,136 executed assertions; 8/8 locally executed binaries passed** under C++20 with -Wall -Wextra -Wpedantic -Werror; ASan/UBSan for sanitizer builds.

The new source and test files exactly match their committed Git blob hashes. The local original-species map (256 bytes) matches GitHub source byte-for-byte (FNV1a32 1355681799); all 760 Gen I original base-stat bytes also match (FNV1a32 59214578). The production Gen1BattleStats.cpp is byte-exact. Other locally required headers/reader identifiers and the staged-editor derived-HP-DV linker entrypoint were restricted test shims, NOT a full exact-head repository checkout.

**BLOCKED:** Entire repository checkout (direct GitHub clone fails DNS), bash tests/test_legality_l2_run_all.sh full-source execution, existing full PK1 parser linkage, 164-execution Stage7 final CI, production legality analyzer integration and original retail device evidence. Never count queued or cancelled GitHub jobs as passes.

## Remaining original Gen I historical coverage

Multi-party and Exp. All share calculation; original Vitamin Stat Exp histories; Time Capsule transfers; authenticated full historical defeat logs and stat-recalculation timestamps; PKHeX.Core host oracle differential on authentic PK1; complete CI acceptance; coordinator-owned integration. Absent modern-only fields never invalidate native PK1.
