# PokeBank NX — Current Verified Engineering State

## ACTIVE OWNER GATE — OCTOBER 15, 2026

**Mandatory sequence: Gen V fixes → full Gen I–V code/action audit → exhaustive real Switch test of every reachable action on all 20 releases → owner approves the exact NRO → only then active Gen VI/3DS integration.** [Master acceptance #313](https://github.com/GlitchedZeus/PokeBank-NX/issues/313) · [106-checkpoint hardware matrix](docs/handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md).

- **Single app integration:** [#307](https://github.com/GlitchedZeus/PokeBank-NX/pull/307). Last fully green app head `35cbf1637da41e9607f2d524354c51d70e401fb2` (**9/9 exact-head checks passed**). Claude has since advanced HEAD to `888b886cd02b8fa6d6c110fb661c4e4272982ead`; some newer host/native checks were still running. Earlier fully-green build: [Gen I–V NRO artifact](https://github.com/GlitchedZeus/PokeBank-NX/actions/runs/38682361760). **Hardware NOT yet accepted.** Prior `fa9cfcea` Gen V candidate **failed** physical testing (shared hub, native encrypted blank/Create, new-game trainer presentation); fixes require retest.
- **Gen I–IV:** finalization PR #306 head `4b9b9f65` is **fully an ancestor of #307**. The older #278/#300/#304 are also fully incorporated. Their PRs were closed as **superseded tracking** (not merged to main; branches/commits preserved).
- **Gen VI–VIII:** [#309](https://github.com/GlitchedZeus/PokeBank-NX/pull/309), [#311](https://github.com/GlitchedZeus/PokeBank-NX/pull/311), [#312](https://github.com/GlitchedZeus/PokeBank-NX/pull/312) are individually CI-green research/backend drafts, **not** permission to shift active app focus beyond Gen V. No owner-device accepted VI–VIII editor.
- **QA:** [#310](https://github.com/GlitchedZeus/PokeBank-NX/pull/310) fuzz/export/NRO audit green; outstanding QA-F2 obsolete one-time workflow YAML and QA-F3b Gen II exclusive publishing should be reviewed before final sign-off.
- **Legality:** [#298](https://github.com/GlitchedZeus/PokeBank-NX/pull/298), [#303](https://github.com/GlitchedZeus/PokeBank-NX/pull/303) and [#308](https://github.com/GlitchedZeus/PokeBank-NX/pull/308) remain separate, combined gate failures need review; not complete PKHeX parity.
- **GitHub hygiene:** original board 275 branches/32 open PR/45 open issues. Four ancestor PRs closed as superseded, 18 future-feature issues archived not-planned for current milestone, and one new [acceptance issue #313](https://github.com/GlitchedZeus/PokeBank-NX/issues/313) created; **28 open PRs / 28 open issues**. Full original idea histories preserved in [future backlog](docs/FUTURE_AFTER_GEN15_BACKLOG.md). **Branch refs not deleted**; 269 unique branch tips require separate safety review before pruning.

The October 14 snapshots below are historical, not today's active work authorization.

---

**Checked October 14, 2026.** GitHub remains authoritative: re-fetch every PR HEAD and exact-run CI before any integration. [Detailed cross-lane audit](docs/ENGINEERING_SNAPSHOT_20261014.md).

## Hardware acceptance and release
- Product: **PokeBank NX**, offline native C++20 Nintendo Switch homebrew; **active alpha**, no v1.0 release.
- Last recorded **owner-accepted integrated Gen I–IV/Product Home** application: `0760334fb850ca46b5171c55f690998d7680d4e7` (October 8); NRO SHA-256 `4f28d7f7ad38c2b62458642e014ae3c05e866755c4152bf4c2db0574c381ce91`.
- **CI ≠ hardware acceptance.** New artwork, Items, touch and Gen V code is not certified just because a workflow succeeds.
- The default branch `main` does **not** contain every open application/legality PR.

## Application / Save Instances / editors
| Development lane | HEAD at audit | What is actually delivered / missing |
|---|---|---|
| [#92 Gen I–IV parent](https://github.com/GlitchedZeus/PokeBank-NX/pull/92) | `ddaff2470947ed3ca579ec22eb25c7564e215a8c` | Staged Gen I–IV source/editor/Product UI parent, unmerged draft |
| [#306 Claude Gen I–IV finalization](https://github.com/GlitchedZeus/PokeBank-NX/pull/306) | `3ac0ae29fe5d40bc8abededffd2b49326610eec4` | Exact in-NRO RomFS artwork verifier, 10 regional scenes, 20 trainer portraits, Gen I–IV covers, fair DS discovery and original ROM header launch identity. **Gen I–IV candidate, Product UI native, native, Gen IV and touch checks passed; full host still running; hardware pending** |
| [#300 Gen V application](https://github.com/GlitchedZeus/PokeBank-NX/pull/300) | `58bd564ba479745f6129f57167aeac6616297753` | BW/B2W2 exact source/profile assignment, selected primary/backup copy, native Trainer/Party/24 Boxes, shared PK5 staged Edit/Create/Review, app-owned export. **All six CI checks passed; hardware pending** |
| [#304 Gen V stacked source hardening](https://github.com/GlitchedZeus/PokeBank-NX/pull/304) | `4d8c267e8b86df15aeb0002df89207cad0516211` | Persisted exact copy, equitable scan budgets, alias/repair/focus changes, independently green; **not yet reconciled/merged** |
| [#305 PK4 capture Ball](https://github.com/GlitchedZeus/PokeBank-NX/pull/305) | `b8eb006ebb85900684ce4caac4d75a18ebb5a8be` | Capture Ball derived from `max(0x83,0x86)`, separate unmerged correction |
| [#278 / #280](https://github.com/GlitchedZeus/PokeBank-NX/pull/278) | `d0a45909` / `dcd855a7` | Narrow greyed-out native Actions and Gen III/IV empty-slot touch/A parity; newer combined NRO acceptance pending |

### Gen V verification and next Switch test
At `58bd564b`, **Gen V Focused Host, Full Host Tests, Native PR Gate, Touch, and both Gen I/II packed-move jobs are green**. [GitHub Actions 38523683607](https://github.com/GlitchedZeus/PokeBank-NX/actions/runs/38523683607) provides the `PokeBank-NX-Gen5-Save-Instances-38523683607` artifact. **No physical acceptance yet.**

The owner already confirmed Gen V emulator saves live in the **same folders as working Gen IV saves**. The previous Switch build displayed empty Black/White/B2W2 cards and sometimes missing scene, cover and trainer artwork. These remain hardware-verification targets, **not a reason to move save files**.

Workflow under test: Games → X / Source/Game File → **Assign Save to This Profile** → choose validated existing save → return to Games → A to choose exact validated copy and open shared staged editor. Check cross-profile ownership, restart, save changes by emulator, staged Keep/Discard/Review/export, and unchanged original SAV bytes. The no-inode FAT/exFAT identity no longer depends on content SHA; session-level exact content fingerprints still prevent stale-source opening.

## Legality engine and research
| Lane | Exact audited HEAD | Status |
|---|---|---|
| [#103 isolated accepted Gen I–IV legality](https://github.com/GlitchedZeus/PokeBank-NX/pull/103) | `d29eada5c087e201e00c749703f1a2de3ddfadeb` | Read-only original evidence groundwork; not merged to main |
| [#298 five-lane integration](https://github.com/GlitchedZeus/PokeBank-NX/pull/298) | `5bf98b115ee9c3ff9189cccdfb7d9fd8040577c1` | L1 encounters, L2 eggs, L3 moves, L4 events, L5 QA; combined CI red |
| [#303 Claude legality](https://github.com/GlitchedZeus/PokeBank-NX/pull/303) | `bc9d8c205b05a55bfbad3c913c0a52bedce0b71a` | Gen I original PKHeX differential green; full Host Tests failing |
| [#301 Gen V–IX foundational legality](https://github.com/GlitchedZeus/PokeBank-NX/pull/301) | `809a31841fc2ad43dda02d4412cb9891f29a482f` | Conservative native-format envelopes, not full legality |

Missing independently proven acquisition histories and authenticated event data remain **Unknown/Incomplete**, not globally Invalid. No PKHeX-total parity or legal/certified stamp is claimed.

## Current acceptance priorities
1. Finish [#306](https://github.com/GlitchedZeus/PokeBank-NX/pull/306) exact-head full host verification; test one NRO on real hardware, preserving Oct 8 launch/Quit & Return.
2. Retest [#300](https://github.com/GlitchedZeus/PokeBank-NX/pull/300) Gen V in existing emulator folders, profile assign/reopen, selected retail copy, shared editor and source-safety/asset claims.
3. Review stacked [#304](https://github.com/GlitchedZeus/PokeBank-NX/pull/304) before any integration; preserve its branch history and later contributor commits.
4. Reconcile [#298](https://github.com/GlitchedZeus/PokeBank-NX/pull/298)/[#303](https://github.com/GlitchedZeus/PokeBank-NX/pull/303) from original source and exact-head test evidence, independently of app development.
5. Track all build artifacts by exact SHA/hash. No direct installed/emulator save injection, global unsafe write toggle or persistent Master Vault approval.

**Governance:** never reset/rebase backwards, force-push, overwrite newer commits or quietly merge other agents' branches. Read-only original source saves remain immutable; staged/export copies are separate. See [Game Support](docs/GAME_SUPPORT_MATRIX.md), [Roadmap](docs/V1_ROADMAP.md), [Build Record](docs/BUILD_RECORD.md).
