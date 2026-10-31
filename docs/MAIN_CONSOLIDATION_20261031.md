# Main consolidation and repository cleanup ledger — October 31, 2026

**Status: PROPOSAL + RECORD.** Nothing in this ledger deletes a branch. Branch deletion needs an exact preservation audit (below) **and the owner's explicit approval**. PRs are closed only when their exact head commit is already contained in a successor line; issues close only with owner-documented evidence.

## 1. What was consolidated

| Item | Value |
|---|---|
| Old `main` | `6fb44150` (137 owner documentation commits since the `1ffec508` split, no code changes) |
| Application merged | `fix/gen4-bag-remove-compaction` `0842c6f3` (Round 10; 3244 commits since the split) |
| Merge commit | `6a909055` on `integration/main-gen15-consolidation` (a real two-parent merge; no rebase, reset or force-push) |
| Hardware candidate under owner test | `3634d1e6` — NRO SHA-256 `b6cd0b89e25c83627558b4d3d70949431c40947c3c8cd26fe46b67bb2c9e8cd9`; an ancestor of the merge, unchanged |
| Code / tests / assets / workflows | byte-identical to `0842c6f3` (verified: every non-Markdown path) |
| Documentation | 12 conflicting Markdown files resolved forward (see the merge commit message); all 10 main-only documents kept |
| Removed files | the application line's audited Sept 29 removals stay removed: `.github/workflows/import-pkse.yml` (destructive import), `PKSE.sln`, `PKSE.vcxproj`, `PKSE.vcxproj.filters`, `CppProperties.json` (stale VS metadata); still in history |
| Not merged | legality #314 / #327 / #328; Gen VI–IX lanes; Japanese Gen I/II editing stays locked |

## 2. Long-term branch structure

| Branch | Purpose | Status |
|---|---|---|
| `main` | Gen I–V application and ongoing bug fixes | becomes the consolidated application when the consolidation PR merges |
| `development/gen6-plus` | future Gen VI–IX application | created from the verified consolidation commit; #309→#311→#312 and #321 to be retargeted after owner approval |
| `development/legality-engine` | combined, reviewed Gen I–IX legality | created from the same commit with **no legality merged**; #314, #327, #328 stay isolated and are retargeted only after review and owner approval |

## 3. Open pull requests (30)

Method: `git merge-base --is-ancestor <PR head> <line>` and `git rev-list --no-merges <head> --not <app> <#314> <main>`; patch equivalence with `git cherry`.

| PR | Head | Classification | Action |
|---|---|---|---|
| [#328](https://github.com/GlitchedZeus/PokeBank-NX/pull/328) | `work/claude-gen69-legality-foundation-20261030` `e33e1115` | ACTIVE — 2 unique commits, stacked on #314 | Keep open, isolated |
| [#327](https://github.com/GlitchedZeus/PokeBank-NX/pull/327) | `work/claude-gen15-legality-independent-20261030` `6cc05036` | ACTIVE — 2 unique commits, stacked on #314 | Keep open, isolated |
| [#321](https://github.com/GlitchedZeus/PokeBank-NX/pull/321) | `research/gen6-9-codex-20261018` `9b46c779` | UNIQUE — 1 research commit (Gen VI–IX audit, SAV7 evidence) | Keep; retarget to `development/gen6-plus` after approval |
| [#314](https://github.com/GlitchedZeus/PokeBank-NX/pull/314) | `claude/gen1-5-legality-campaign-w9mkom` `eabe41f3` | ACTIVE — Gen I–V legality campaign | Keep open, isolated |
| [#312](https://github.com/GlitchedZeus/PokeBank-NX/pull/312) | `claude/gen8-native-20261015` `2f065ee2` | UNIQUE — stacked on #311 (14 commits incl. stack) | Keep (stacked) |
| [#311](https://github.com/GlitchedZeus/PokeBank-NX/pull/311) | `claude/gen7-native-20261014` `2438b5d0` | UNIQUE — stacked on #309 (13 commits incl. #309) | Keep (stacked) |
| [#309](https://github.com/GlitchedZeus/PokeBank-NX/pull/309) | `claude/gen6-native-20261014` `5e683690` | UNIQUE — 6 commits (Gen VI native lane, bottom of stack) | Keep; retarget to `development/gen6-plus` after approval |
| [#308](https://github.com/GlitchedZeus/PokeBank-NX/pull/308) | `claude/legality-gen3-negai-boshi-lifetime` `2761a75d` | SUPERSEDED — head contained in #314 (`eabe41f3`) | Closed with a reference to #314; branch kept as legality evidence |
| [#307](https://github.com/GlitchedZeus/PokeBank-NX/pull/307) | `claude/gen5-integration-20261014` `d6cc878a` | INCORPORATED — head `d6cc878a` contained in the application line | Closed as superseded by the consolidation PR |
| [#305](https://github.com/GlitchedZeus/PokeBank-NX/pull/305) | `claude/gen4-capture-ball` `b8eb006e` | SUPERSEDED — head contained in #314 (`eabe41f3`) | Closed with a reference to #314; branch kept as legality evidence |
| [#303](https://github.com/GlitchedZeus/PokeBank-NX/pull/303) | `claude/pensive-cerf-pueceq` `bc9d8c20` | SUPERSEDED — head contained in #314 (`eabe41f3`) | Closed with a reference to #314; branch kept as legality evidence |
| [#301](https://github.com/GlitchedZeus/PokeBank-NX/pull/301) | `feature/legality-gen5-9-foundation-20261012` `809a3184` | UNIQUE — 24 commits (Gen V–IX legality foundation) | Keep; future `development/legality-engine` review |
| [#299](https://github.com/GlitchedZeus/PokeBank-NX/pull/299) | `audit/legality-integration-hardening-20261012` `67f56f38` | SUPERSEDED — head contained in #314 (`eabe41f3`) | Closed with a reference to #314; branch kept as legality evidence |
| [#298](https://github.com/GlitchedZeus/PokeBank-NX/pull/298) | `integration/legality-gen14-major-20261012` `5bf98b11` | SUPERSEDED — head contained in #314 (`eabe41f3`) | Closed with a reference to #314; branch kept as legality evidence |
| [#296](https://github.com/GlitchedZeus/PokeBank-NX/pull/296) | `feature/legality-l3-moves-transfer-20261012` `aead1faf` | SUPERSEDED — head contained in #314 (`eabe41f3`) | Closed with a reference to #314; branch kept as legality evidence |
| [#295](https://github.com/GlitchedZeus/PokeBank-NX/pull/295) | `feature/legality-l2-breeding-20261012` `a9da17a7` | SUPERSEDED — head contained in #314 (`eabe41f3`) | Closed with a reference to #314; branch kept as legality evidence |
| [#294](https://github.com/GlitchedZeus/PokeBank-NX/pull/294) | `audit/legality-l5-research-qa-20261012` `59a95176` | SUPERSEDED — head contained in #314 (`eabe41f3`) | Closed with a reference to #314; branch kept as legality evidence |
| [#293](https://github.com/GlitchedZeus/PokeBank-NX/pull/293) | `feature/legality-l1-encounters-rng-20261012` `573cbc17` | SUPERSEDED — head contained in #314 (`eabe41f3`) | Closed with a reference to #314; branch kept as legality evidence |
| [#292](https://github.com/GlitchedZeus/PokeBank-NX/pull/292) | `feature/legality-l4-events-meta-20261012` `03ae8c09` | SUPERSEDED — head contained in #314 (`eabe41f3`) | Closed with a reference to #314; branch kept as legality evidence |
| [#280](https://github.com/GlitchedZeus/PokeBank-NX/pull/280) | `fix/gen14-action-tap-empty-parity-20261011` `dcd855a7` | UNIQUE — 9 commits (Gen III–IV empty-slot touch parity, Oct 11) | Owner/engineering review: port or close as obsolete |
| [#273](https://github.com/GlitchedZeus/PokeBank-NX/pull/273) | `feature/legality-gen4-egg-iv-stage-20261011` `dfa2fb53` | UNIQUE — 2 legality commits not in #314 | Keep; legality review |
| [#252](https://github.com/GlitchedZeus/PokeBank-NX/pull/252) | `audit/gen1-4-legality-research-20261011` `fc0225e6` | UNIQUE — 5 research commits not in #314 | Keep; legality evidence |
| [#230](https://github.com/GlitchedZeus/PokeBank-NX/pull/230) | `feature/legality-sourcefree-egg-move-guard-20261011` `95416a8f` | UNIQUE — 3 own commits (stack #227→#228→#229→#230, 6 total) | Keep; legality review |
| [#229](https://github.com/GlitchedZeus/PokeBank-NX/pull/229) | `feature/legality-sourcefree-pk12-move-compatibility-20261011` `14baf42c` | UNIQUE — inside the #230 stack | Keep (stacked) |
| [#228](https://github.com/GlitchedZeus/PokeBank-NX/pull/228) | `feature/legality-sourcefree-pk4-palpark-move-history-20261011` `3df1de83` | UNIQUE — inside the #230 stack | Keep (stacked) |
| [#227](https://github.com/GlitchedZeus/PokeBank-NX/pull/227) | `feature/legality-sourcefree-pk4-gen4-move-acquisition-20261011` `a1eea80b` | UNIQUE — inside the #230 stack | Keep (stacked) |
| [#163](https://github.com/GlitchedZeus/PokeBank-NX/pull/163) | `feature/gen5-readonly-foundation-20261008` `79679950` | UNIQUE — 85 commits (early Gen V read-only lane; Gen V since rebuilt on the shared editor) | Owner review: archive or close as obsolete (keep branch) |
| [#144](https://github.com/GlitchedZeus/PokeBank-NX/pull/144) | `audit/gen1-4-pkhex-parity-ledger-20261006` `b450bf25` | UNIQUE — 1 commit (PKHeX parity ledger) | Keep; legality evidence |
| [#122](https://github.com/GlitchedZeus/PokeBank-NX/pull/122) | `feature/touch-controls-20261005` `36c0d1d6` | UNIQUE — 370 commits (touch-control parity lane, Oct 9) | Owner review: large unique history; keep branch |
| [#103](https://github.com/GlitchedZeus/PokeBank-NX/pull/103) | `feature/legality-engine-gen1-4-20260929` `d29eada5` | SUPERSEDED — head contained in #314 (`eabe41f3`) | Closed with a reference to #314; branch kept as legality evidence |
| [#92](https://github.com/GlitchedZeus/PokeBank-NX/pull/92) | `feature/gen4-full-editor-20260928` `ddaff247` | INCORPORATED — head `ddaff247` contained in the application line | Closed as superseded by the consolidation PR |

## 4. Open issues (34)

| Issue | Topic | Classification | Action |
|---|---|---|---|
| [#326](https://github.com/GlitchedZeus/PokeBank-NX/issues/326) | Gen V Move picker learnset | RESOLVED — owner README records direct move pools passing Round 5 | Closes on consolidation merge |
| [#325](https://github.com/GlitchedZeus/PokeBank-NX/issues/325) | Gen V Language inline | RESOLVED — inline Language passed Round 5 (NEW-14) | Closes on consolidation merge |
| [#322](https://github.com/GlitchedZeus/PokeBank-NX/issues/322) | Unify Save Backups / open validated copies | RESOLVED — backup history and selected-copy staging passed Round 5 (R2-EMUBACKUP) | Closes on consolidation merge |
| [#95](https://github.com/GlitchedZeus/PokeBank-NX/issues/95) | G4-04 Gen IV Create + fields | RESOLVED — #92 head contained; Gen IV Create/Edit in the application | Closes on consolidation merge |
| [#324](https://github.com/GlitchedZeus/PokeBank-NX/issues/324) | Gen II Special column + neutral Gen IV stats | LIKELY RESOLVED — R2-SPECIAL passed Round 5; Gen IV part unconfirmed | Owner confirm |
| [#323](https://github.com/GlitchedZeus/PokeBank-NX/issues/323) | Shared Review + Trainer parity | IN PROGRESS — Round 5/6 Review findings; Rounds 7–10 untested | Keep |
| [#320](https://github.com/GlitchedZeus/PokeBank-NX/issues/320) | P0 A/B/R input regression | LIKELY RESOLVED — Rounds 5–6 ran 42/59 checks on later builds | Owner confirm |
| [#313](https://github.com/GlitchedZeus/PokeBank-NX/issues/313) | Gen I–V final acceptance gate | ACTIVE | Keep |
| [#285](https://github.com/GlitchedZeus/PokeBank-NX/issues/285) | Legality master inventory | ACTIVE — #314 lineage | Keep |
| [#286](https://github.com/GlitchedZeus/PokeBank-NX/issues/286) | L1 encounters lane | SUPERSEDED by #314 (lane PR #293 contained) | Propose close after owner OK |
| [#287](https://github.com/GlitchedZeus/PokeBank-NX/issues/287) | L2 breeding lane | SUPERSEDED by #314 (lane PR #295 contained) | Propose close after owner OK |
| [#288](https://github.com/GlitchedZeus/PokeBank-NX/issues/288) | L3 moves lane | SUPERSEDED by #314 (lane PR #296 contained) | Propose close after owner OK |
| [#289](https://github.com/GlitchedZeus/PokeBank-NX/issues/289) | L4 events lane | SUPERSEDED by #314 (lane PR #292 contained) | Propose close after owner OK |
| [#290](https://github.com/GlitchedZeus/PokeBank-NX/issues/290) | L5 research/QA lane | SUPERSEDED by #314 (lane PR #294 contained) | Propose close after owner OK |
| [#89](https://github.com/GlitchedZeus/PokeBank-NX/issues/89) | Backups + explicit Inject workflow | ACTIVE / future (injection stays disabled) | Keep |
| [#31](https://github.com/GlitchedZeus/PokeBank-NX/issues/31) | Read-only DS Gen IV/V adapters | LIKELY RESOLVED — DraStic/melonDS Gen IV/V sources exist | Owner confirm |
| [#69](https://github.com/GlitchedZeus/PokeBank-NX/issues/69) | Full project audit | LIKELY RESOLVED — audit remediation in PR #101 (one deferred item) | Owner confirm |
| #29, #55, #56, #57, #58, #59, #70 | v1.0 roadmap / release-gate trackers | ACTIVE roadmap | Keep |
| #5, #9, #15, #16, #17, #21, #26, #30, #49, #50 | backlog / research / preservation | ACTIVE backlog | Keep (candidates for one consolidated backlog issue, owner choice) |

## 5. Branch refs (315)

| Class | Count | Meaning |
|---|---:|---|
| Contained in the consolidated application (`main` after merge) | 69 | every commit reachable from the merge |
| Contained in the #314 legality line | 171 | every commit reachable from `eabe41f3` |
| Merge commits only | 1 | no unique non-merge commit |
| Unique work | 73 | at least one commit in neither line nor old `main` |

**Deletion candidates (owner approval required):** contained branches that are not protected. Protected = `main`, open-PR heads and bases, the Round 9 / Round 10 candidate sources, and branches named below. Deleting a branch never deletes Actions artifacts or merged history, but links to `blob/<branch>/...` would break, so each deletion must be approved.

<details><summary><strong>Protected contained branches (21)</strong></summary>

| Branch | Head | Date | Note |
|---|---|---|---|
| `audit/legality-integration-hardening-20261012` | `67f56f38` | 2026-10-12 | PROTECTED — open PR head/base; in #314 line |
| `audit/legality-l5-research-qa-20261012` | `59a95176` | 2026-10-13 | PROTECTED — open PR head/base; in #314 line |
| `claude/gen1-5-legality-campaign-w9mkom` | `eabe41f3` | 2026-10-29 | PROTECTED — open PR head/base; in #314 line |
| `claude/gen14-final-20261014` | `4b9b9f65` | 2026-10-14 | PROTECTED — open PR head/base; in application |
| `claude/gen4-capture-ball` | `b8eb006e` | 2026-10-13 | PROTECTED — open PR head/base; in #314 line |
| `claude/gen5-integration-20261014` | `d6cc878a` | 2026-10-27 | PROTECTED — #307 head; Round 5/6 hardware documents historically linked here; in application |
| `claude/legality-gen3-negai-boshi-lifetime` | `2761a75d` | 2026-10-14 | PROTECTED — open PR head/base; in #314 line |
| `claude/pensive-cerf-pueceq` | `bc9d8c20` | 2026-10-14 | PROTECTED — open PR head/base; in #314 line |
| `feature/gen4-full-editor-20260928` | `ddaff247` | 2026-10-11 | PROTECTED — open PR head/base; in application |
| `feature/legality-engine-gen1-4-20260929` | `d29eada5` | 2026-10-12 | PROTECTED — open PR head/base; in #314 line |
| `feature/legality-l1-encounters-rng-20261012` | `573cbc17` | 2026-10-13 | PROTECTED — open PR head/base; in #314 line |
| `feature/legality-l2-breeding-20261012` | `a9da17a7` | 2026-10-13 | PROTECTED — open PR head/base; in #314 line |
| `feature/legality-l3-moves-transfer-20261012` | `aead1faf` | 2026-10-13 | PROTECTED — open PR head/base; in #314 line |
| `feature/legality-l4-events-meta-20261012` | `03ae8c09` | 2026-10-13 | PROTECTED — open PR head/base; in #314 line |
| `feature/legality-sourcefree-pk3-move-acquisition-20261011` | `6fb8c9ba` | 2026-10-11 | PROTECTED — open PR head/base; in #314 line |
| `fix/gen14-narrow-native-actions-20261011` | `d0a45909` | 2026-10-12 | PROTECTED — open PR head/base; in application |
| `fix/gen4-bag-remove-compaction` | `0842c6f3` | 2026-10-28 | PROTECTED — Round 10 candidate source (owner testing 3634d1e6) — keep unchanged; in application |
| `integration/gen15-combined` | `c4f414a3` | 2026-10-28 | PROTECTED — Round 9 candidate d6a20a2a source and audit record; in application |
| `integration/gen4-polish-hardware-20260928` | `8b3bcc16` | 2026-09-28 | PROTECTED — open PR head/base; in application |
| `integration/legality-gen14-major-20261012` | `5bf98b11` | 2026-10-13 | PROTECTED — open PR head/base; in #314 line |
| `main` | `6fb44150` | 2026-10-25 | PROTECTED — default branch; in application |

</details>

<details><summary><strong>Deletion candidates — contained, owner approval required (220)</strong></summary>

| Branch | Head | Date | Note |
|---|---|---|---|
| `audit/app-qa-20261014` | `aecc9ea6` | 2026-10-27 | in application |
| `audit/full-project-hardening-20260923` | `00ee7a6e` | 2026-09-28 | in application |
| `audit/legality-master-parallel-plan-20261012` | `062f5728` | 2026-10-12 | in #314 line |
| `checkpoint/pre-session2-recovery-2026-09-01` | `17bad5e7` | 2026-09-01 | in application |
| `claude/gen14-campaign-20261015` | `73248a15` | 2026-10-26 | in application |
| `claude/gen5-home-providers-20261015` | `ece88e27` | 2026-10-15 | in application |
| `claude/launch-return-20261015` | `e7bce8cb` | 2026-10-26 | in application |
| `claude/pickers-search-scrollbar-20261017` | `e52b7d97` | 2026-10-26 | in application |
| `claude/reroll-mechanics-20261017` | `967b75bd` | 2026-10-26 | merges only |
| `claude/shared-editor-gen35-20261016` | `42a92780` | 2026-10-26 | in application |
| `codex/gen5-native-create-engine` | `6e27bbd5` | 2026-10-13 | in application |
| `codex/gen5-profile-save-instances-hardware` | `4d8c267e` | 2026-10-13 | in application |
| `codex/legality-event3-ribbons-and-alm-audit-20261004` | `65ba9657` | 2026-10-04 | in #314 line |
| `codex/legality-gen4-bcc-reroll-failed-leads-20261005` | `917c40eb` | 2026-10-05 | in #314 line |
| `codex/legality-gen4-bcc-reroll-nolead-20261005` | `5714b3e0` | 2026-10-05 | in #314 line |
| `codex/legality-gen4-bcc-reroll-nolead-central-20261005` | `5a0e3fc2` | 2026-10-05 | in #314 line |
| `codex/legality-gen4-bcc-reroll-pressure-20261005` | `2bb13109` | 2026-10-05 | in #314 line |
| `codex/legality-gen4-bcc-reroll-static-magnet-20261005` | `ab61d24c` | 2026-10-05 | in #314 line |
| `codex/legality-gen4-bcc-reroll-sync-fail-20261005` | `2b776c69` | 2026-10-05 | in #314 line |
| `codex/legality-gen4-bcc-reroll-sync-success-v2-20261005` | `cf4bdc86` | 2026-10-05 | in #314 line |
| `codex/legality-gen4-bcc-sync-fail-central-20261005` | `90816ca5` | 2026-10-05 | in #314 line |
| `codex/legality-gen4-bug-contest-direct-20261005` | `dde366b4` | 2026-10-05 | in #314 line |
| `codex/legality-gen4-central-reporting-20261005` | `2227bd90` | 2026-10-05 | in #314 line |
| `codex/legality-gen4-central-reporting-adapter-20261005-copy` | `2227bd90` | 2026-10-05 | in #314 line |
| `codex/legality-gen4-central-reporting-v2-20261005` | `66f38499` | 2026-10-05 | in #314 line |
| `codex/legality-gen4-lead-central-20261005` | `978bc764` | 2026-10-05 | in #314 line |
| `codex/legality-gen4-lead-central-validation-20261005` | `978bc764` | 2026-10-05 | in #314 line |
| `codex/legality-gen4-lead-failures-20261005` | `762cf1a0` | 2026-10-05 | in #314 line |
| `codex/legality-gen4-lead-predicates-20261005` | `72e518f7` | 2026-10-05 | in #314 line |
| `codex/legality-gen4-lead-reporting-20261005` | `2227bd90` | 2026-10-05 | in #314 line |
| `codex/legality-gen4-min31-reroll-frames-20261005` | `c8599aef` | 2026-10-05 | in #314 line |
| `codex/legality-gen4-pressure-grass-source-20261005` | `1410886b` | 2026-10-05 | in #314 line |
| `codex/legality-gen4-pressure-success-20261005` | `3bcca932` | 2026-10-05 | in #314 line |
| `codex/legality-gen4-reporting-adapter-v2-20261005` | `82d55f86` | 2026-10-05 | in #314 line |
| `codex/legality-gen4-reporting-production-hook-20261005` | `e4684c7a` | 2026-10-05 | in #314 line |
| `codex/legality-gen4-safari-reroll-nolead-20261005` | `fa557ae4` | 2026-10-05 | in #314 line |
| `codex/legality-gen4-static-central-20261005` | `fbe447ef` | 2026-10-05 | in #314 line |
| `codex/legality-gen4-static-central-form-combined-20261005` | `b8d2c1c1` | 2026-10-05 | in #314 line |
| `codex/legality-gen4-static-egg-evolution-20261005` | `a634460a` | 2026-10-05 | in #314 line |
| `codex/legality-gen4-static-egg-state-20261005` | `fa401310` | 2026-10-05 | in #314 line |
| `codex/legality-gen4-static-evolution-20261005` | `324c99f6` | 2026-10-05 | in #314 line |
| `codex/legality-gen4-static-magnet-success-20261005` | `a54243a9` | 2026-10-05 | in #314 line |
| `codex/legality-wurmple-evidence-20261005` | `c12fc374` | 2026-10-05 | in #314 line |
| `codex/legality-wurmple-source-proof-20261005` | `21a48650` | 2026-10-05 | in #314 line |
| `dev/classic-inventory-20260912` | `f603e3e4` | 2026-09-13 | in application |
| `dev/inventory-footer-readability-candidate-20260913` | `fc1e6de2` | 2026-09-13 | in application |
| `dev/standalone-runtime-final-20260912` | `74698c3f` | 2026-09-12 | in application |
| `docs/issue71-production-status-20260919` | `96da3c1d` | 2026-09-19 | in application |
| `docs/legality-20261011-accepted-through-242` | `06eda48b` | 2026-10-11 | in #314 line |
| `docs/legality-gen1-4-parity-through-186-20261009` | `f458bc4b` | 2026-10-09 | in #314 line |
| `docs/legality-gen4-methodk-reroll-parity-20261009` | `6263aff7` | 2026-10-09 | in #314 line |
| `docs/legality-move-history-checkpoint-20261011` | `8bd51041` | 2026-10-11 | in #314 line |
| `docs/legality-parity-checkpoint-20261010` | `96c5eaaf` | 2026-10-10 | in #314 line |
| `docs/legality-pkhex-parity-ledger-20261006` | `350f7018` | 2026-10-06 | in #314 line |
| `docs/legality-sourcefree-parity-checkpoint-20261011` | `87b2fa73` | 2026-10-11 | in #314 line |
| `docs/refresh-frontpage-20261012` | `ff450262` | 2026-10-12 | in application |
| `feature/gen1-editor-ux-cleanup2-20260913-copy` | `2107ac40` | 2026-09-13 | in application |
| `feature/gen1-editor-ux-cleanup2-20260913` | `6ecd9c28` | 2026-09-13 | in application |
| `feature/gen1-editor-ux-cleanup3-20260913` | `0fd59ae9` | 2026-09-13 | in application |
| `feature/gen1-editor-ux-polish-20260913` | `c3ad2be6` | 2026-09-13 | in application |
| `feature/gen1-party-storage-view-unification-20260914` | `5a2642ac` | 2026-09-14 | in application |
| `feature/gen1-staged-editor-recovery-20260913` | `574d604b` | 2026-09-13 | in application |
| `feature/gen12-packed-y-multimove-20260917-copyprobe2` | `16a4852b` | 2026-09-17 | in application |
| `feature/gen12-packed-y-multimove-20260917-copyprobe` | `b9b3de57` | 2026-09-17 | in application |
| `feature/gen12-packed-y-multimove-20260917` | `c24859ce` | 2026-09-18 | in application |
| `feature/gen2-shared-pokemon-editor-20260914` | `cfb70357` | 2026-09-15 | in application |
| `feature/gen3-shared-pokemon-editor-20260919` | `996e6aa4` | 2026-09-23 | in application |
| `feature/gen4-shared-pokemon-editor-20260928` | `6e9f54f2` | 2026-09-28 | in application |
| `feature/legality-breed002-daycare-order-20261011` | `ae54aef4` | 2026-10-11 | in #314 line |
| `feature/legality-candidate-palpark-history-20261011` | `b46f766c` | 2026-10-11 | in #314 line |
| `feature/legality-candidate-scoped-move-witnesses-20261011` | `f51f3357` | 2026-10-11 | in #314 line |
| `feature/legality-coherent-history-milestone-20261011` | `c87fd598` | 2026-10-11 | in #314 line |
| `feature/legality-dppt-marsh-evolved-safari-native-20261009` | `df91a69e` | 2026-10-09 | in #314 line |
| `feature/legality-emerald-levelup-pinned-evidence-20261011` | `b12f4225` | 2026-10-11 | in #314 line |
| `feature/legality-emerald-shedinja-move-history-20261011` | `c4e1a850` | 2026-10-11 | in #314 line |
| `feature/legality-gamecube-palpark-move-stage-20261011` | `d7c72389` | 2026-10-11 | in #314 line |
| `feature/legality-gen1-branched-catch-rate-ancestry-20261010` | `b9e93f8f` | 2026-10-10 | in #314 line |
| `feature/legality-gen12-joint-sourcefree-moves-20261011` | `c03e7d9f` | 2026-10-11 | in #314 line |
| `feature/legality-gen12-sourcefree-format-ceilings-20261010` | `1090b7c6` | 2026-10-10 | in #314 line |
| `feature/legality-gen12-time-capsule-directional-reporting-20261010` | `d3e17d28` | 2026-10-10 | in #314 line |
| `feature/legality-gen2-crystal-boxed-pk2-report-20261010` | `697d5731` | 2026-10-10 | in #314 line |
| `feature/legality-gen2-crystal-dratini-gift-evolution-20261010` | `85075ef8` | 2026-10-10 | in #314 line |
| `feature/legality-gen2-crystal-eevee-evolution-gift-20261009` | `82c7e56e` | 2026-10-09 | in #314 line |
| `feature/legality-gen2-crystal-evolved-wild-20261009` | `2d7d439d` | 2026-10-09 | in #314 line |
| `feature/legality-gen2-crystal-native-pk2-caught-origin-20261009` | `8cf2fb78` | 2026-10-09 | in #314 line |
| `feature/legality-gen2-crystal-raw-save-pipeline-20261009` | `e7bb7c71` | 2026-10-09 | in #314 line |
| `feature/legality-gen2-crystal-red-gyarados-shiny-native-20261010` | `bf9d2e41` | 2026-10-10 | in #314 line |
| `feature/legality-gen2-crystal-six-evolution-families-20261009` | `7c9fe9dd` | 2026-10-09 | in #314 line |
| `feature/legality-gen2-crystal-starter-evolution-gifts-20261009` | `a99a9b50` | 2026-10-09 | in #314 line |
| `feature/legality-gen2-crystal-tyrogue-gift-evolution-20261010` | `3219c531` | 2026-10-10 | in #314 line |
| `feature/legality-gen2-sourcefree-timecapsule-report-20261010` | `55242544` | 2026-10-10 | in #314 line |
| `feature/legality-gen3-detached-wild-evidence-20261011` | `98a7b627` | 2026-10-11 | in #314 line |
| `feature/legality-gen3-egg-iv-retail-stage-20261011` | `5c0a117a` | 2026-10-11 | in #314 line |
| `feature/legality-gen3-emerald-safari-block-cutecharm-fail-20261008` | `b97ff76f` | 2026-10-08 | in #314 line |
| `feature/legality-gen3-emerald-safari-block-grass-pressure-20261008` | `c6ec0d7e` | 2026-10-08 | in #314 line |
| `feature/legality-gen3-emerald-safari-block-intimidate-20261008` | `1f5f2027` | 2026-10-08 | in #314 line |
| `feature/legality-gen3-emerald-safari-block-static-success-20261008` | `7a3d895b` | 2026-10-08 | in #314 line |
| `feature/legality-gen3-emerald-safari-block-sync-fail-20261008` | `4ecb89b7` | 2026-10-08 | in #314 line |
| `feature/legality-gen3-emerald-safari-cute-fail-20261008` | `d285c6cd` | 2026-10-08 | in #314 line |
| `feature/legality-gen3-emerald-safari-grass-pressure-20261008` | `7908f59d` | 2026-10-08 | in #314 line |
| `feature/legality-gen3-emerald-safari-intimidate-20261008` | `17c70890` | 2026-10-08 | in #314 line |
| `feature/legality-gen3-emerald-safari-nolead-20261007` | `a4910509` | 2026-10-07 | in #314 line |
| `feature/legality-gen3-emerald-safari-pressure-fail-20261008` | `e4c996b4` | 2026-10-08 | in #314 line |
| `feature/legality-gen3-emerald-safari-pressure-success-20261008` | `7373617d` | 2026-10-08 | in #314 line |
| `feature/legality-gen3-emerald-safari-static-success-20261008` | `f115f7f6` | 2026-10-08 | in #314 line |
| `feature/legality-gen3-emerald-safari-sync-20261007` | `44050980` | 2026-10-07 | in #314 line |
| `feature/legality-gen3-emerald-safari-sync-fail-20261008` | `a872b911` | 2026-10-08 | in #314 line |
| `feature/legality-gen3-joint-move-pool-20261011` | `e0a1b3fe` | 2026-10-11 | in #314 line |
| `feature/legality-gen3-safari-ball-production-info-20261009` | `03ea5153` | 2026-10-09 | in #314 line |
| `feature/legality-gen3-safari-emerald-nolead-20261007` | `51d35e57` | 2026-10-07 | in #314 line |
| `feature/legality-gen3-safari-evidence-20261007` | `f8e859ec` | 2026-10-07 | in #314 line |
| `feature/legality-gen3-safari-method3-20261007` | `51d35e57` | 2026-10-07 | in #314 line |
| `feature/legality-gen3-safari-methodh-regular-20261007` | `eee4fd41` | 2026-10-07 | in #314 line |
| `feature/legality-gen3-safari-origin-resolution-20261007` | `76b6449c` | 2026-10-07 | in #314 line |
| `feature/legality-gen34-ball-domain-20261006` | `0b16a0bd` | 2026-10-06 | in #314 line |
| `feature/legality-gen34-ball-domain-production-20261006` | `ff1b15ca` | 2026-10-06 | in #314 line |
| `feature/legality-gen34-full-palpark-chain-20261011` | `f6a17e15` | 2026-10-11 | in #314 line |
| `feature/legality-gen34-language-domain-20261006` | `22af43f1` | 2026-10-06 | in #314 line |
| `feature/legality-gen34-language-production-20261006` | `255d4f42` | 2026-10-06 | in #314 line |
| `feature/legality-gen34-native-egg-pokeball-info-20261009` | `7bb678fe` | 2026-10-09 | in #314 line |
| `feature/legality-gen34-ordinary-egg-move-construction-20261011` | `32b863d6` | 2026-10-11 | in #314 line |
| `feature/legality-gen34-parent-eligibility-20261011` | `07774772` | 2026-10-11 | in #314 line |
| `feature/legality-gen34-same-parent-moveset-20261011` | `5655e1f9` | 2026-10-11 | in #314 line |
| `feature/legality-gen34-sourcefree-egg-move-origins-20261010` | `f2e637fb` | 2026-10-10 | in #314 line |
| `feature/legality-gen34-sourcefree-egg-state-20261010` | `a9ea1a49` | 2026-10-10 | in #314 line |
| `feature/legality-gen34-sourcefree-format-domains-20261010` | `d5ab1f57` | 2026-10-10 | in #314 line |
| `feature/legality-gen34-sourcefree-species-moves-20261010` | `2dbb8dd6` | 2026-10-10 | in #314 line |
| `feature/legality-gen4-bcc-mixed-sync-20261009` | `d767b12f` | 2026-10-09 | in #314 line |
| `feature/legality-gen4-bcc-mixed-sync-depth23-20261009` | `650a9f0a` | 2026-10-09 | in #314 line |
| `feature/legality-gen4-bcc-mixed-sync-depth23-reporting-20261009` | `d3a9053d` | 2026-10-09 | in #314 line |
| `feature/legality-gen4-bcc-mixed-sync-real-source-report-20261009` | `66c0b004` | 2026-10-09 | in #314 line |
| `feature/legality-gen4-bcc-mixed-sync-reporting-20261009` | `dc1dd291` | 2026-10-09 | in #314 line |
| `feature/legality-gen4-bcc-mixed-sync-reverse-20261009` | `8f402d54` | 2026-10-09 | in #314 line |
| `feature/legality-gen4-bcc-mixed-sync-reverse-reporting-20261009` | `34f420b8` | 2026-10-09 | in #314 line |
| `feature/legality-gen4-bcc-multidepth-central-report-20261009` | `b9bf7226` | 2026-10-09 | in #314 line |
| `feature/legality-gen4-bcc-pid-nature-rejections-20261009` | `98f411d9` | 2026-10-09 | in #314 line |
| `feature/legality-gen4-breeding-candidate-universe-20261011` | `2fe63bce` | 2026-10-11 | in #314 line |
| `feature/legality-gen4-coherent-egg-parent-history-20261011` | `41721756` | 2026-10-11 | in #314 line |
| `feature/legality-gen4-contest-ribbon-prefix-20261011` | `49f3c426` | 2026-10-11 | in #314 line |
| `feature/legality-gen4-detached-joint-moves-20261011` | `1b5667c1` | 2026-10-11 | in #314 line |
| `feature/legality-gen4-egg-iv-power-history-20261011` | `806027d1` | 2026-10-11 | in #314 line |
| `feature/legality-gen4-egg-move-source-family-20261011` | `73c4068e` | 2026-10-11 | in #314 line |
| `feature/legality-gen4-egg-vs-wild-ancestry-20261009` | `2909edc2` | 2026-10-09 | in #314 line |
| `feature/legality-gen4-event-joint-moves-20261011` | `780007ae` | 2026-10-11 | in #314 line |
| `feature/legality-gen4-event-persisted-ribbons-20261011` | `ffc5ea78` | 2026-10-11 | in #314 line |
| `feature/legality-gen4-evolved-special-ball-20261009` | `9569cc6f` | 2026-10-09 | in #314 line |
| `feature/legality-gen4-fixed-ball-origin-resolution-20261007` | `57217337` | 2026-10-07 | in #314 line |
| `feature/legality-gen4-fixed-ball-provenance-20261006` | `75f99942` | 2026-10-06 | in #314 line |
| `feature/legality-gen4-great-marsh-ball-provenance-20261009` | `d0b3dfe5` | 2026-10-09 | in #314 line |
| `feature/legality-gen4-hgss-apricorn-wild-ball-20261009` | `78bd91cd` | 2026-10-09 | in #314 line |
| `feature/legality-gen4-native-daycare-parent-snapshots-20261011` | `2be50d68` | 2026-10-11 | in #314 line |
| `feature/legality-gen4-native-pk4-language-regression-20261009` | `17342ec2` | 2026-10-09 | in #314 line |
| `feature/legality-gen4-native-pk4-multiretry-report-20261009` | `f33d27f1` | 2026-10-09 | in #314 line |
| `feature/legality-gen4-nincada-shedinja-bcc-ball-20261009` | `5d2973a4` | 2026-10-09 | in #314 line |
| `feature/legality-gen4-platinum-egg-pid-rng-20261012` | `d29eada5` | 2026-10-12 | in #314 line |
| `feature/legality-gen4-sourcefree-npc-trades-20261011` | `fecfbca9` | 2026-10-11 | in #314 line |
| `feature/legality-gen4-sourcefree-palpark-fields-20261010` | `16c74353` | 2026-10-10 | in #314 line |
| `feature/legality-gen4-sourcefree-ranger-manaphy-20261011` | `cc086802` | 2026-10-11 | in #314 line |
| `feature/legality-gen4-sourcefree-retail-wild-20261011` | `d2e84eb6` | 2026-10-11 | in #314 line |
| `feature/legality-gen4-sourcefree-static-gifts-20261011` | `47f3ec31` | 2026-10-11 | in #314 line |
| `feature/legality-gen4-sourcefree-wc4-events-20261011` | `7b65baaa` | 2026-10-11 | in #314 line |
| `feature/legality-gen4-special-ball-native-pk4-20261009` | `4cdf5381` | 2026-10-09 | in #314 line |
| `feature/legality-gen4-special-ball-source-info-20261009` | `8e583094` | 2026-10-09 | in #314 line |
| `feature/legality-gen4-unresolved-hatch-origin-guard-20261010` | `886574fd` | 2026-10-10 | in #314 line |
| `feature/legality-gen4-wild-source-linked-rng-20261011` | `0dccf2e1` | 2026-10-12 | in #314 line |
| `feature/legality-gen4-wurmple-special-ball-pid-20261009` | `6bcce570` | 2026-10-09 | in #314 line |
| `feature/legality-hgss-egg-pid-stage-20261012` | `dd395c3c` | 2026-10-12 | in #314 line |
| `feature/legality-history-sequence-milestone-20261011` | `fe3e3d05` | 2026-10-11 | in #314 line |
| `feature/legality-integrate-rng-palpark-with-breeding-20261011` | `d8a2f2eb` | 2026-10-11 | in #314 line |
| `feature/legality-native-source-format-mismatch-20261010` | `bad141a8` | 2026-10-11 | in #314 line |
| `feature/legality-palpark-exact-source-move-stage-20261011` | `1b011823` | 2026-10-11 | in #314 line |
| `feature/legality-palpark-gen3-ancestor-move-retention-20261011` | `7b122ed8` | 2026-10-11 | in #314 line |
| `feature/legality-palpark-gen4-evolution-lineage-20261011` | `37071451` | 2026-10-11 | in #314 line |
| `feature/legality-palpark-hm-boundary-20261011` | `bcd65775` | 2026-10-11 | in #314 line |
| `feature/legality-palpark-move-id-bound-20261011` | `5b324590` | 2026-10-11 | in #314 line |
| `feature/legality-palpark-move-stage-20261011` | `60ef8796` | 2026-10-11 | in #314 line |
| `feature/legality-palpark-stage-structural-guard-20261011` | `95fd6e10` | 2026-10-11 | in #314 line |
| `feature/legality-palpark-two-era-joint-moves-20261011` | `145002e9` | 2026-10-11 | in #314 line |
| `feature/legality-pk2-conditional-timecapsule-stages-20261011` | `12a6db65` | 2026-10-11 | in #314 line |
| `feature/legality-pk2-timecapsule-invalid-move-reporting-20261011` | `916e9931` | 2026-10-11 | in #314 line |
| `feature/legality-pk3-gamecube-unanchored-moves-20261011` | `6c0259bf` | 2026-10-11 | in #314 line |
| `feature/legality-pk4-raw-text-capabilities-20261011` | `a0285476` | 2026-10-11 | in #314 line |
| `feature/legality-sourcefree-pk2-timecapsule-ancestry-20261011` | `4d3026e0` | 2026-10-11 | in #314 line |
| `feature/legality-timecapsule-directional-20261011` | `54afda53` | 2026-10-11 | in #314 line |
| `feature/multilingual-gen15` | `e6bdbebb` | 2026-10-28 | in application |
| `feature/pokebank-playable` | `3aeecb80` | 2026-09-19 | in application |
| `feature/pokemon-editor-ui-foundation-final-20260913` | `1cca8f6a` | 2026-09-14 | in application |
| `feature/touch-modal-isolation-20261011` | `bbe7e97f` | 2026-10-11 | in application |
| `feature/universal-editor-contract-audit-20260918` | `fb28323c` | 2026-09-18 | in application |
| `fix/full-audit-remediation-20260929` | `408a5644` | 2026-10-01 | in application |
| `fix/gen14-touch-ux-parity-20261011` | `9f4002a8` | 2026-10-11 | in application |
| `fix/shared-editor-neutral-headings-20260915` | `b695980b` | 2026-09-17 | in application |
| `integration/gen14-touch-hardware-20261010` | `b1fcbd3e` | 2026-10-11 | in application |
| `integration/gen5-ui-hardware-candidate-20261012` | `58bd564b` | 2026-10-14 | in application |
| `integration/legality-gen1-4-method-witnesses-20261011` | `2dd8f7a6` | 2026-10-11 | in #314 line |
| `recovery/interrupted-2026-09-01` | `3101fc09` | 2026-09-01 | in application |
| `scratch/do-not-use` | `cf42d853` | 2026-10-05 | in application |
| `staging/product-home-perf-audit-v2-20261004` | `564bb3ed` | 2026-10-04 | in application |
| `tmp-do-not-use` | `2107ac40` | 2026-09-13 | in application |
| `tmp-rby-adapter` | `0b592145` | 2026-09-10 | in application |
| `ui/pokemon-product-polish-20260929` | `5b346dd3` | 2026-09-29 | in application |
| `ui/product-shell-round2-20260928` | `5e4cf038` | 2026-09-28 | in application |
| `wip/c-r5-review-20261024` | `dca9880c` | 2026-10-24 | in application |
| `wip/c-r7-special-20261026` | `cbbdcb12` | 2026-10-26 | in application |
| `wip/codex-round4-takeover-20261021` | `6f4cac5c` | 2026-10-22 | in application |
| `wip/coord-gen5-direct-open-20261020` | `61491b53` | 2026-10-20 | in application |
| `wip/coord-r5-gen3-held-legal-20261025` | `15c77226` | 2026-10-25 | in application |
| `wip/l-backup-browser-20261020` | `44fb4b7e` | 2026-10-20 | in application |
| `wip/l-r5-backup-copy-20261024` | `1eec6f18` | 2026-10-25 | in application |
| `wip/l-r6-bag-unavailable-review-20261026` | `76966b43` | 2026-10-26 | in application |
| `wip/l-r6-review-entry-20261025` | `7152178b` | 2026-10-26 | in application |
| `wip/l-r7-review-keys-gen5-backups-20261026` | `42bf612d` | 2026-10-26 | in application |
| `wip/lane-r-round7-b04` | `bfff58d0` | 2026-10-26 | in application |
| `wip/lane-r-round7-unown` | `47b8097e` | 2026-10-26 | in application |
| `wip/legality-evidence-20261025` | `eabe41f3` | 2026-10-29 | in #314 line |
| `wip/p-classic-picker-dismiss-20261026` | `efb687eb` | 2026-10-26 | in application |
| `wip/p-round6-20261025` | `0d68e8b7` | 2026-10-26 | in application |
| `wip/p-round7-20261026` | `727fad5d` | 2026-10-26 | in application |
| `wip/pkse-1.2.x-gen15-audit` | `44f24400` | 2026-10-28 | in application |
| `wip/s-gen3-create-review-shape` | `3c929189` | 2026-10-26 | in application |

</details>

<details><summary><strong>Unique work — keep (never delete without owner review) (73)</strong></summary>

| Branch | Head | Date | Note |
|---|---|---|---|
| `archive/qa-campaigns-2026-10-28` | `c0f1c967` | 2026-10-29 | KEEP — 3 unique commit(s); 3 unique patch(es) |
| `audit/full-repository-line-by-line-20260928` | `143c5e5c` | 2026-09-29 | KEEP — 182 unique commit(s); 182 unique patch(es) |
| `audit/gen1-4-legality-research-20261011` | `fc0225e6` | 2026-10-11 | KEEP — 5 unique commit(s); 5 unique patch(es) |
| `audit/gen1-4-pkhex-parity-ledger-20261006` | `b450bf25` | 2026-10-06 | KEEP — 1 unique commit(s); 1 unique patch(es) |
| `audit/pre-gen3-shared-editor-71` | `c363505e` | 2026-09-15 | KEEP — 9 unique commit(s); 9 unique patch(es) |
| `ci/classic-inventory-corrected-validation-20260913` | `a0280e3e` | 2026-09-13 | KEEP — 1 unique commit(s); 1 unique patch(es) |
| `ci/classic-inventory-correction-20260913` | `4d11237b` | 2026-09-13 | KEEP — 5 unique commit(s); 5 unique patch(es) |
| `ci/classic-inventory-final-validation-20260913` | `338e6873` | 2026-09-13 | KEEP — 1 unique commit(s); 1 unique patch(es) |
| `ci/classic-inventory-production-final-20260913` | `55cdd5c2` | 2026-09-13 | KEEP — 1 unique commit(s); 1 unique patch(es) |
| `ci/classic-inventory-wiring-20260913` | `87bf28de` | 2026-09-13 | KEEP — 5 unique commit(s); 5 unique patch(es) |
| `ci/gen2-final-artifact-20260913` | `ccb14a3b` | 2026-09-12 | KEEP — 1 unique commit(s); 1 unique patch(es) |
| `ci/gen2-final-focused-20260912` | `0656096d` | 2026-09-12 | KEEP — 54 unique commit(s); 54 unique patch(es) |
| `ci/gen2-final-native-20260912` | `66034438` | 2026-09-12 | KEEP — 54 unique commit(s); 54 unique patch(es) |
| `ci/gen2-final-prod-validate-20260913` | `fd176de0` | 2026-09-12 | KEEP — 1 unique commit(s); 1 unique patch(es) |
| `ci/gen2-prod-validate-20260913` | `d57a503b` | 2026-09-12 | KEEP — 1 unique commit(s); 1 unique patch(es) |
| `ci/gsc-gender-validation-20260912` | `92d5d3bf` | 2026-09-12 | KEEP — 3 unique commit(s); 3 unique patch(es) |
| `ci/inventory-dirty-final-validation-20260913` | `19343ab8` | 2026-09-13 | KEEP — 12 unique commit(s); 12 unique patch(es) |
| `ci/inventory-dirty-validation-20260913` | `f2b61c9d` | 2026-09-13 | KEEP — 10 unique commit(s); 10 unique patch(es) |
| `ci/inventory-footer-readability-validation-20260913` | `3e588c02` | 2026-09-13 | KEEP — 1 unique commit(s); 1 unique patch(es) |
| `ci/inventory-hardware-polish-validation-20260913` | `1b3c9b84` | 2026-09-13 | KEEP — 6 unique commit(s); 6 unique patch(es) |
| `ci/inventory-ui-retest-production-artifact-20260913` | `0040e9ba` | 2026-09-13 | KEEP — 1 unique commit(s); 1 unique patch(es) |
| `ci/standalone-final-host-validation-20260912` | `6ab062e2` | 2026-09-12 | KEEP — 3 unique commit(s); 3 unique patch(es) |
| `ci/standalone-final-validation-20260912` | `aaa7a7fb` | 2026-09-12 | KEEP — 4 unique commit(s); 4 unique patch(es) |
| `ci/standalone-production-artifact-20260912` | `ed19625f` | 2026-09-12 | KEEP — 1 unique commit(s); 1 unique patch(es) |
| `ci/standalone-validation-20260912` | `5bce5864` | 2026-09-12 | KEEP — 3 unique commit(s); 3 unique patch(es) |
| `claude/gen5-handoff-from-legality-20261014` | `fc519414` | 2026-10-14 | KEEP — 1 unique commit(s); 1 unique patch(es) |
| `claude/gen6-native-20261014` | `5e683690` | 2026-10-15 | KEEP — 6 unique commit(s); earlier point of `claude/gen7-native-20261014` |
| `claude/gen7-native-20261014` | `2438b5d0` | 2026-10-15 | KEEP — 13 unique commit(s); earlier point of `claude/gen8-native-20261015` |
| `claude/gen8-native-20261015` | `2f065ee2` | 2026-10-15 | KEEP — 14 unique commit(s); 14 unique patch(es) |
| `claude/item-quantity-policy-20261016` | `f6acf510` | 2026-10-28 | KEEP — 1 unique commit(s); 1 unique patch(es) |
| `claude/optimistic-euler-ph2z4f` | `fb5e79e8` | 2026-10-20 | KEEP — 77 unique commit(s); 77 unique patch(es) |
| `claude/tender-gates-bkl5jq` | `d78690ea` | 2026-10-28 | KEEP — 1 unique commit(s); 1 unique patch(es) |
| `codex/gsc-readonly` | `fa081f67` | 2026-09-11 | KEEP — 37 unique commit(s); 37 unique patch(es) |
| `codex/legality-gen4-bcc-reroll-sync-success-20261005` | `31fd10f9` | 2026-10-05 | KEEP — 2 unique commit(s); 1 unique patch(es) |
| `codex/legality-gen4-central-reporting-adapter-20261005` | `66322e07` | 2026-10-05 | KEEP — 3 unique commit(s); 2 unique patch(es) |
| `codex/legality-gen4-reroll-foundation-20261005` | `6b5a942c` | 2026-10-05 | KEEP — 3 unique commit(s); 3 unique patch(es) |
| `codex/legality-gen4-static-form-boundary-20261005` | `ccbe0bcc` | 2026-10-05 | KEEP — 1 unique commit(s); 0 unique patch(es) |
| `codex/rby-itemsfix-patch` | `3d51730a` | 2026-09-11 | KEEP — 13 unique commit(s); 13 unique patch(es) |
| `dev/gen2-pokemon-editor-20260912` | `4866de7f` | 2026-09-12 | KEEP — 53 unique commit(s); earlier point of `ci/gen2-final-focused-20260912` |
| `dev/gen2-staged-editor-20260912` | `87a32b21` | 2026-09-12 | KEEP — 16 unique commit(s); 16 unique patch(es) |
| `dev/inventory-footer-readability-20260913` | `586433f6` | 2026-09-13 | KEEP — 3 unique commit(s); 2 unique patch(es) |
| `dev/inventory-hardware-polish-20260913` | `08fb25f7` | 2026-09-13 | KEEP — 11 unique commit(s); earlier point of `ci/inventory-dirty-final-validation-20260913` |
| `dev/standalone-runtime-audit-20260912` | `f278c6d5` | 2026-09-12 | KEEP — 9 unique commit(s); 9 unique patch(es) |
| `dev/standalone-runtime-candidate-20260912` | `c8a08c1d` | 2026-09-12 | KEEP — 5 unique commit(s); 5 unique patch(es) |
| `docs/legality-checkpoint-palpark-egg-20261011` | `0f4d05a6` | 2026-10-11 | KEEP — 2 unique commit(s); 2 unique patch(es) |
| `docs/project-status-refresh-20260929` | `6927f06c` | 2026-09-29 | KEEP — 3 unique commit(s); 3 unique patch(es) |
| `feature/gen1-staged-editor-20260913` | `a45a1b63` | 2026-09-13 | KEEP — 1 unique commit(s); 0 unique patch(es) |
| `feature/gen4-complete-editor-20260928` | `4a3e2e19` | 2026-09-28 | KEEP — 31 unique commit(s); 31 unique patch(es) |
| `feature/gen5-readonly-foundation-20261008` | `79679950` | 2026-10-12 | KEEP — 85 unique commit(s); 85 unique patch(es) |
| `feature/legality-gen3-hoenn-safari-rng-20261007` | `cf60f1fa` | 2026-10-07 | KEEP — 4 unique commit(s); 4 unique patch(es) |
| `feature/legality-gen4-egg-iv-stage-20261011` | `dfa2fb53` | 2026-10-11 | KEEP — 2 unique commit(s); 2 unique patch(es) |
| `feature/legality-gen5-9-foundation-20261012` | `809a3184` | 2026-10-12 | KEEP — 24 unique commit(s); 24 unique patch(es) |
| `feature/legality-pk2-timecapsule-move-stage-20261011` | `6e4bd7ec` | 2026-10-11 | KEEP — 1 unique commit(s); earlier point of `docs/legality-checkpoint-palpark-egg-20261011` |
| `feature/legality-rng-move-input-hardening-20261011` | `82d32ab6` | 2026-10-11 | KEEP — 16 unique commit(s); 16 unique patch(es) |
| `feature/legality-sourcefree-egg-move-guard-20261011` | `95416a8f` | 2026-10-11 | KEEP — 6 unique commit(s); 6 unique patch(es) |
| `feature/legality-sourcefree-pk12-move-compatibility-20261011` | `14baf42c` | 2026-10-11 | KEEP — 3 unique commit(s); earlier point of `feature/legality-sourcefree-egg-move-guard-20261011` |
| `feature/legality-sourcefree-pk4-gen4-move-acquisition-20261011` | `a1eea80b` | 2026-10-11 | KEEP — 1 unique commit(s); earlier point of `feature/legality-sourcefree-egg-move-guard-20261011` |
| `feature/legality-sourcefree-pk4-palpark-move-history-20261011` | `3df1de83` | 2026-10-11 | KEEP — 2 unique commit(s); earlier point of `feature/legality-sourcefree-egg-move-guard-20261011` |
| `feature/touch-controls-20261005` | `36c0d1d6` | 2026-10-09 | KEEP — 370 unique commit(s); 370 unique patch(es) |
| `fix/gen14-action-tap-empty-parity-20261011` | `dcd855a7` | 2026-10-11 | KEEP — 9 unique commit(s); 9 unique patch(es) |
| `hardening/audit-bank-migration-20260928` | `342a149a` | 2026-09-28 | KEEP — 1 unique commit(s); 1 unique patch(es) |
| `hardening/audit-settings-durability-20260928` | `d928533c` | 2026-09-28 | KEEP — 1 unique commit(s); 1 unique patch(es) |
| `integration/gen2-staged-editor-20260912` | `d3da18fd` | 2026-09-12 | KEEP — 2 unique commit(s); 2 unique patch(es) |
| `polish/v1-ui-qol-20260928` | `c8c98d5d` | 2026-09-28 | KEEP — 13 unique commit(s); 11 unique patch(es) |
| `research/gen6-9-codex-20261018` | `9b46c779` | 2026-10-18 | KEEP — 1 unique commit(s); 1 unique patch(es) |
| `staging/product-home-perf-audit-20261004` | `defa2c99` | 2026-10-04 | KEEP — 8 unique commit(s); 8 unique patch(es) |
| `wip/codex-oct24-repairs` | `11b44e09` | 2026-10-24 | KEEP — 1 unique commit(s); 1 unique patch(es) |
| `wip/coord-r5-safari-ball-20261024` | `31a75206` | 2026-10-24 | KEEP — 1 unique commit(s); 1 unique patch(es) |
| `wip/l-backup-picker-20261024` | `0fe9e5f7` | 2026-10-24 | KEEP — 1 unique commit(s); earlier point of `wip/l-r5-backup-browser-20261024` |
| `wip/l-r5-backup-browser-20261024` | `8a0f9eb4` | 2026-10-24 | KEEP — 2 unique commit(s); 2 unique patch(es) |
| `wip/p-heldsearch-20261024` | `65bd8d65` | 2026-10-24 | KEEP — 1 unique commit(s); 1 unique patch(es) |
| `work/claude-gen15-legality-independent-20261030` | `6cc05036` | 2026-10-31 | KEEP — 2 unique commit(s); 2 unique patch(es) |
| `work/claude-gen69-legality-foundation-20261030` | `e33e1115` | 2026-10-31 | KEEP — 2 unique commit(s); 2 unique patch(es) |

</details>

## 6. Rules applied

- No force-push, reset, rebase or deletion; the consolidation is a merge on a temporary branch, merged only after owner approval.
- The Round 10 NRO `3634d1e6`, its branch, artifacts, audit and checklist are unchanged; `main` is newer and is **not** that hardware build.
- PRs closed in this pass have heads proven contained in a successor; each closing comment names the successor.
- Legality #314 / #327 / #328 stay isolated; nothing from them is in `main`.
