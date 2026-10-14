# PokeBank NX — Main Development checkpoint

Maintained by the Main Development coordinator. Only the coordinator edits this
file; threads report to it. GitHub is authoritative: re-fetch before relying on
any SHA below.

**Last updated:** 2026-10-14 ~22:10 UTC (18:10 America/Toronto).

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
| Integrated Gen I–V candidate | not built yet | — | — |

## Generation and game coverage

Three separate columns: software / automated verification / hardware. These are
conservative estimates.

| Gen | Games | Software | Automated verification | Hardware |
|---|---|---|---|---|
| I–III | RBY, GSC, RSE/FRLG | ~90% | CI green on #306/#307 | Earlier build accepted; newest candidate untested |
| IV | DPPt, HGSS | ~90% | CI green on #306/#307 | Earlier build accepted; newest candidate untested |
| V | BW, B2W2 | ~65% | 8/8 CI green on #307 `a17c503a` | Not tested |
| VI | X, Y, OR, AS | ~40% (format, session, editor provider and sources on its own lane; not wired into the app) | 1,896 local checks across Gen VI/VII under GCC/Clang ± ASan/UBSan; Gen VI CI green on #309 so far | — |
| VII | SM, USUM; LGPE as its own family | ~15% (strict read-only SAV7 with MemeCrypto verify; export blocked on re-signing evidence) | local only; CI running | — |
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

**New since the last update (Thread 2, `60f7cdc2`; CI running, not yet green):**
- `b9c8d8fa`: native Gen V Bag in the shared Items surface with staged edits.
- `5f450952`: Gen IV parity for box Move Pokémon, plus read-only origin fields.
- `60f7cdc2`: native Trainer Name, Money and Play Time staging in the shared panel.

**Missing:**
- Bag in Review and Bag export: whether these are complete is to be confirmed in Thread 2's report.
- Party/Box edit parity audit against Gen IV.
- Trainer fields beyond Name, Money and Play Time.
- Pokédex view.
- End-to-end exact-copy and export tests for combined PK5+Bag edits.
- The leftover `patches/gen5-create/` folder.
- The integrated Gen I–V candidate NRO.
- Automatic return to PokeBank NX after DraStic (shared issue with Gen IV).

## Active PRs and branches

| Lane | Branch | HEAD | CI | Owner |
|---|---|---|---|---|
| #306 Gen I–IV | `claude/gen14-final-20261014` | `4b9b9f65` | 9/9 green | Thread 1 |
| #307 Gen V integration | `claude/gen5-integration-20261014` | `60f7cdc2` | running (`a17c503a` was 8/8 green) | Thread 2 (exclusive; legality session confirmed handoff 2026-10-14) |
| Gen VI native | `claude/gen6-native-20261014` (draft #309 → #307, CI only) | `aee7e911` | Gen VI host + native green; full host running | Thread 3 |
| Gen VII native | `claude/gen7-native-20261014` | `4e66e94f` | running | Thread 3 |
| App QA | `audit/app-qa-20261014` | `24e730a0` (Gen I–V parser fuzzing and export-safety contracts) | no PR yet | Thread 4 |
| Coordinator | `claude/optimistic-euler-ph2z4f` | docs only, on `a17c503a` | — | Coordinator |
| #300 / #304 / #302 | preserved source lanes | `58bd564b` / `4d8c267e` / `6e27bbd5` | green | Contained in #307 |
| #305 / #280 | `claude/gen4-capture-ball` / `fix/gen14-action-tap-empty-parity-20261011` | `b8eb006e` / `dcd855a7` | — | Rule confirmed by test / tests to port (Thread 1) |

## Blockers

1. Gen I–III RetroArch exit crash. **Codex owns this**; Main Development does not edit RetroArch exit code.
2. Gen IV/V DraStic "Quit to Launcher" returns to DraStic, not PokeBank NX.
3. Hardware test of the `9bf9d492` candidate is pending with the owner.
4. ~~#307 ownership~~: resolved. The legality session confirmed it stopped at `a17c503a` and left draft handoff `claude/gen5-handoff-from-legality-20261014` @ `fc519414` (Bag in the Items panel; unbuilt) for Thread 2 to review.
5. Native builds outside CI depend on pulling the devkitA64 Docker image, which can hit the Docker Hub rate limit.

## Owner decisions pending

1. Approve integrating Gen VI (later Gen VII) into #307 after the Gen V candidate. The integration requests are in `docs/gen6/integration-requests/01-02` on the Gen VI lane.
2. Gen VII re-signing: needs a "payload tail" diagnostic from one game-written SM/USUM save. PKHeX and PKSM-Core differ here, so SAV7 export stays unsupported until then.
3. Approve bumping the `memecrypto` submodule: `sha1.c` has undefined behavior in a signed shift, and `rsa.c` leaks memory.
4. Confirm the Dekopon (`sdmc:/switch/dekopon/…`) and Checkpoint (`sdmc:/3ds/Checkpoint/saves/…`) paths on the real SD card. Azahar has no Switch build, so it is unsupported.

## Next implementation tasks

1. Thread 2: connect the Gen V Bag to the shared Items screen and Review, reach Gen V editor parity, add the Trainer and Pokédex work, then build the integrated Gen I–V NRO.
2. Thread 3: SAV6 (XY/ORAS) layout, checksums and block map from cited references; PK6 crypto; strict parse/serialize; staged PK6 edits with verified export; a Gen VI `ExactFormatEditorProvider`; Checkpoint/JKSV and manual-link sources; research into verified Azahar/Dekopon save locations.
3. Thread 4: parser fuzzing under ASan/UBSan; source-immutability tests for every export; an exact-head NRO audit tool.
