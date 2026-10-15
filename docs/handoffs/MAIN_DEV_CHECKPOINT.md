# PokeBank NX — Main Development checkpoint

Maintained by the Main Development coordinator. Only the coordinator edits this
file; threads report to it. GitHub is authoritative: re-fetch before relying on
any SHA below.

**Last updated:** 2026-10-15 ~03:30 UTC (23:30 America/Toronto, Oct 14).

## Active milestone

**Stage B — complete Gen V (Black, White, Black 2, White 2)** on PR #307.
Stage A (Gen I–IV) is software-complete on #306 and waiting on hardware testing.
Stage C (Gen VI) is starting in parallel.

## Hardware acceptance

| Build | Code SHA | NRO SHA-256 | Status |
|---|---|---|---|
| Integrated Gen I–IV / Product Home | `0760334fb850ca46b5171c55f690998d7680d4e7` | `4f28d7f7ad38c2b62458642e014ae3c05e866755c4152bf4c2db0574c381ce91` | **Owner accepted** (Oct 8) |
| Earlier #306 candidate | (earlier #306 head) | — | Owner confirmed: artwork restored, Gen IV launches directly into DraStic |
| #306 Gen I–IV candidate | `9bf9d492` (PR head `4b9b9f65` adds docs only) | recorded in a #306 PR comment | **NOT DEVICE TESTED** |
| **Integrated Gen I–V candidate** (#307) | `fa9cfceabfecd2afb79d0df935588a3e81e0a7fe` (tree `5de23dc2`) | `57b4ec2bebf6f15cac0ee59218672fd6acc029c1e025abbe159a322aa1bccd23` (175,435,441 B; run 38673482742, artifact 11945232986) | **NOT DEVICE TESTED**. Owner checklist in the #307 comment. #307 is frozen during the test. |

## Generation and game coverage

Three separate columns: software / automated verification / hardware. These are
conservative estimates.

| Gen | Games | Software | Automated verification | Hardware |
|---|---|---|---|---|
| I–III | RBY, GSC, RSE/FRLG | ~90% | CI green on #306/#307 | Earlier build accepted; newest candidate untested |
| IV | DPPt, HGSS | ~90% | CI green on #306/#307 | Earlier build accepted; newest candidate untested |
| V | BW, B2W2 | ~90% | 9/9 CI green on `fa9cfcea`; full GCC/Clang host + sanitizers pass | Candidate `fa9cfcea` awaiting owner test |
| VI | X, Y, OR, AS | ~60% (PK6/SAV6, staged Edit/Clone/Remove, party stat recompute, staged Bag, read-only box names/Pokédex, editor provider, sources; not wired into the app) | 7 Gen VI test binaries under GCC/Clang ± ASan/UBSan; differential vs PKSM-Core | — |
| VII | SM, USUM; LGPE as its own family | ~35% (read-only SAV7 + MemeCrypto verify; PK7 staged Edit/Clone/Remove up to signing; hyper-training-aware party stats; read-only box names/Pokédex; export blocked on re-signing evidence; LGPE research note only) | Gen VII focused CI green on #311 | — |
| VIII | SwSh, BDSP, PLA | ~5% (PK8/PB8/PA8 classes, BDSP/LA crypto headers) | none | — |
| IX | SV, Legends: Z-A | ~2% (`BaseStatsGen89`, MyStatus validation) | none | — |

## Gen V: implemented vs missing (PR #307 `a17c503a`)

**Implemented in software:**
- Exact BW/B2W2 source/profile assignment.
- Pinned Primary/Backup copy selection, with fair per-provider scan budgets (#304).
- Native Trainer, Party and 24 Boxes read.
- Shared staged PK5 Edit, Review and app-owned export.
- Native empty-slot Create (#302; Erratic EXP fix live), plus box Clone and Remove.
- DraStic/melonDS launch with Unova ROM header identity (IRB/IRA/IRE/IRD).
- Gen V Bag backend: strict decode, staged quantity/Add/Remove, bag-aware export (`BAG_CHANGED`).

**Added by Thread 2 (`a17c503a..60c48c1b`, 10 commits):**
- Bag in the shared Items screen, with Review rows and Bag export.
- Product Home Items opens the Gen V Bag through the Save Instance chooser.
- Moving Pokémon between boxes at Gen IV parity, plus read-only origin fields.
- Trainer Name, Money and Play Time staging, with the export whitelist and a `TRAINER_CHANGED` manifest entry.
- Read-only Pokédex.
- End-to-end assign → reopen → copy → stage → export test.
- 572 randomized export validations.
- `patches/gen5-create` removed.
- `gen15-integrated-candidate.yml`: one Gen I–V NRO.

**Missing:**
- QA-F1 fixed (`3b35b3f`) and QA-F3 fixed (`fa9cfce`; Gen II's equivalent is Thread 1's).
- Hardware acceptance of `fa9cfcea`.
- Trainer fields beyond Name, Money and Play Time.
- The integrated Gen I–V candidate NRO.
- Automatic return to PokeBank NX after DraStic (shared issue with Gen IV).

## Active PRs and branches

| Lane | Branch | HEAD | CI | Owner |
|---|---|---|---|---|
| #306 Gen I–IV | `claude/gen14-final-20261014` | `4b9b9f65` | 9/9 green | Thread 1 |
| #307 Gen V integration | `claude/gen5-integration-20261014` | `fa9cfcea` (**frozen for hardware test**) | **9/9 green** | Thread 2 (standing by) |
| Gen VI native | `claude/gen6-native-20261014` (draft #309 → #307, CI only) | `5e683690` | see PR #309 | Thread 3 |
| Gen VII native | `claude/gen7-native-20261014` (draft #311 → Gen VI lane, CI only) | `08a4359b` | see PR #311 | Thread 3 |
| App QA | `audit/app-qa-20261014` (draft #310 → #307, CI only) | `e43f234` | running; no Gen I–V parser crash/UB found so far | Thread 4 |
| Coordinator | `claude/optimistic-euler-ph2z4f` | docs only, on `a17c503a` | — | Coordinator |
| #300 / #304 / #302 | preserved source lanes | `58bd564b` / `4d8c267e` / `6e27bbd5` | green | Contained in #307 |
| #305 / #280 | `claude/gen4-capture-ball` / `fix/gen14-action-tap-empty-parity-20261011` | `b8eb006e` / `dcd855a7` | — | Rule confirmed by test / #280 tests change Gen III/IV surfaces → Thread 1 |

## Safety audit (2026-10-15)

The PKSE-derived installed-save inject path (`restoreBackupToTitle`, used by `saveTrainerInfo*` with `injectToTitle`) refuses before mounting, via `Safety::canWriteTo(LiveGame)`. That checks the compile-time `LIVE_SAVE_WRITES_ENABLED = false`, and `test_write_policy`, `test_source_mutation_policy` and `test_gen4_safety_contract` guard it. `writeBlocksToSaveData7LGPE` only patches workspace images.

## Blockers

0. Usage limit: three threads plus the coordinator use up the 5-hour quota in about 1 hour. Thread 4 resumed to audit the candidate.

1. Gen I–III RetroArch exit crash. **Codex owns this**; Main Development does not edit RetroArch exit code.
2. Gen IV/V DraStic "Quit to Launcher" returns to DraStic, not PokeBank NX.
3. Hardware test of the `9bf9d492` candidate is pending with the owner.
4. ~~#307 ownership~~: resolved. The legality session confirmed it stopped at `a17c503a` and left draft handoff `claude/gen5-handoff-from-legality-20261014` @ `fc519414` (Bag in the Items panel; unbuilt) for Thread 2 to review.
5. Native builds outside CI depend on pulling the devkitA64 Docker image, which can hit the Docker Hub rate limit.

## Owner decisions pending

1. Approve integrating Gen VI (later Gen VII) into #307 after the Gen V candidate. The integration requests are in `docs/gen6/integration-requests/01-02` on the Gen VI lane.
2. Gen VII re-signing: needs a "payload tail" diagnostic from one game-written SM/USUM save. PKHeX and PKSM-Core differ here, so SAV7 export stays unsupported until then.
3. Approve bumping the `memecrypto` submodule: `sha1.c` has undefined behavior in a signed shift, and `rsa.c` leaks memory.
6. Approve a new `Integration/Gen7b` lane for Let's Go Pikachu/Eevee (research: `docs/gen7/LGPE_RESEARCH.md` on the Gen VII lane).
5. QA-F2 (Medium): 3 invalid `one-time-*` workflow YAMLs fail on every push to every branch (~1,100 failed runs each). Approve removing or fixing them.
4. Confirm the Dekopon (`sdmc:/switch/dekopon/…`) and Checkpoint (`sdmc:/3ds/Checkpoint/saves/…`) paths on the real SD card. Azahar has no Switch build, so it is unsupported.

## Next implementation tasks

1. Thread 2: connect the Gen V Bag to the shared Items screen and Review, reach Gen V editor parity, add the Trainer and Pokédex work, then build the integrated Gen I–V NRO.
2. Thread 3: SAV6 (XY/ORAS) layout, checksums and block map from cited references; PK6 crypto; strict parse/serialize; staged PK6 edits with verified export; a Gen VI `ExactFormatEditorProvider`; Checkpoint/JKSV and manual-link sources; research into verified Azahar/Dekopon save locations.
3. Thread 4: parser fuzzing under ASan/UBSan; source-immutability tests for every export; an exact-head NRO audit tool.
