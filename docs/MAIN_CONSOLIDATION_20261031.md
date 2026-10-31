# Main consolidation and repository cleanup ledger — October 31, 2026

**Status: RECORD + PROPOSAL.** Nothing in this ledger deletes a branch, closes a PR or closes an issue by itself. Every deletion, closure and retarget listed here waits for the owner's explicit approval; unique or unmerged history additionally needs an archival ref first.

## 1. What was consolidated

| Item | Value |
|---|---|
| Old `main` | `6fb44150` (137 owner documentation commits since the `1ffec508` split, no code changes) |
| Application merged | `fix/gen4-bag-remove-compaction` `0842c6f3` (Round 10; 3244 commits since the split) |
| Merge commit | `6a909055` on `integration/main-gen15-consolidation`: a real two-parent merge (`6fb44150`, `0842c6f3`); no rebase, reset or force-push |
| Owner hardware candidate | **`3634d1e6`**, NRO SHA-256 `b6cd0b89e25c83627558b4d3d70949431c40947c3c8cd26fe46b67bb2c9e8cd9`, 177,159,857 bytes. It is an ancestor of the merge and its testing identity is unchanged: the merge commit is **not** the hardware build |
| Code / tests / assets / workflows | byte-identical to `0842c6f3` (verified on every non-Markdown path) |
| Documentation | 12 conflicting Markdown files resolved forward (see the merge commit); all 10 main-only documents kept |
| Removed files | the application line's audited Sept 29 removals stay removed: `.github/workflows/import-pkse.yml` (destructive import), `PKSE.sln`, `PKSE.vcxproj`, `PKSE.vcxproj.filters`, `CppProperties.json` (stale VS metadata). They remain in history |
| Not merged | legality #314 / #327 / #328 and their lineage; Gen VI–IX lanes. Japanese Gen I/II editing stays locked |

## 2. Long-term branch structure (planned)

| Branch | Purpose | When |
|---|---|---|
| `main` | Gen I–V application and ongoing bug fixes | when the owner approves and merges the consolidation PR |
| `development/gen6-plus` | future Gen VI–IX application | created from the integrated `main` **after** the owner accepts it; then #309→#311→#312 and #321 retarget there |
| `development/legality-engine` | combined, reviewed Gen I–IX legality | created from the integrated `main` after acceptance, then **receives the reviewed engine** (#314 or successor, then #327, then #328). An empty branch is not the completed consolidation |

## 3. Open pull requests (31)

Method: `git merge-base --is-ancestor <PR head> <line>` and `git rev-list --no-merges <head> --not <application> <#314> <old main>`; patch equivalence with `git cherry`. **No PR is closed in this pass.**

| PR | Head | Classification | Proposed action (owner approval) |
|---|---|---|---|
| [#328](https://github.com/GlitchedZeus/PokeBank-NX/pull/328) | `work/claude-gen69-legality-foundation-20261030` `63556298` | ACTIVE — 3 unique commits, stacked on #314 | Keep open, isolated; tracked in #329 |
| [#327](https://github.com/GlitchedZeus/PokeBank-NX/pull/327) | `work/claude-gen15-legality-independent-20261030` `6cc05036` | ACTIVE — 2 unique commits, stacked on #314 | Keep open, isolated; tracked in #329 |
| [#321](https://github.com/GlitchedZeus/PokeBank-NX/pull/321) | `research/gen6-9-codex-20261018` `9b46c779` | UNIQUE — 1 research commit (Gen VI–IX audit, SAV7 evidence) | Keep; retarget to `development/gen6-plus` once it exists |
| [#314](https://github.com/GlitchedZeus/PokeBank-NX/pull/314) | `claude/gen1-5-legality-campaign-w9mkom` `eabe41f3` | ACTIVE — Gen I–V legality campaign | Keep open, isolated; tracked in #329 |
| [#312](https://github.com/GlitchedZeus/PokeBank-NX/pull/312) | `claude/gen8-native-20261015` `2f065ee2` | UNIQUE — stacked on #311 (14 commits incl. stack) | Keep (stacked) |
| [#311](https://github.com/GlitchedZeus/PokeBank-NX/pull/311) | `claude/gen7-native-20261014` `2438b5d0` | UNIQUE — stacked on #309 (13 commits incl. #309) | Keep (stacked) |
| [#309](https://github.com/GlitchedZeus/PokeBank-NX/pull/309) | `claude/gen6-native-20261014` `5e683690` | UNIQUE — 6 commits (Gen VI native lane, bottom of stack) | Keep; retarget to `development/gen6-plus` once it exists |
| [#308](https://github.com/GlitchedZeus/PokeBank-NX/pull/308) | `claude/legality-gen3-negai-boshi-lifetime` `2761a75d` | CONTAINED in #314 (`eabe41f3`) | Keep open while #314/#327/#328 develop; close as superseded after the legality consolidation is accepted |
| [#307](https://github.com/GlitchedZeus/PokeBank-NX/pull/307) | `claude/gen5-integration-20261014` `d6cc878a` | INCORPORATED — head `d6cc878a` is in the application line | Close only after the consolidation is merged into `main` |
| [#305](https://github.com/GlitchedZeus/PokeBank-NX/pull/305) | `claude/gen4-capture-ball` `b8eb006e` | CONTAINED in #314 (`eabe41f3`) | Keep open while #314/#327/#328 develop; close as superseded after the legality consolidation is accepted |
| [#303](https://github.com/GlitchedZeus/PokeBank-NX/pull/303) | `claude/pensive-cerf-pueceq` `bc9d8c20` | CONTAINED in #314 (`eabe41f3`) | Keep open while #314/#327/#328 develop; close as superseded after the legality consolidation is accepted |
| [#301](https://github.com/GlitchedZeus/PokeBank-NX/pull/301) | `feature/legality-gen5-9-foundation-20261012` `809a3184` | UNIQUE — 24 commits (Gen V–IX legality foundation) | Keep; legality review |
| [#299](https://github.com/GlitchedZeus/PokeBank-NX/pull/299) | `audit/legality-integration-hardening-20261012` `67f56f38` | CONTAINED in #314 (`eabe41f3`) | Keep open while #314/#327/#328 develop; close as superseded after the legality consolidation is accepted |
| [#298](https://github.com/GlitchedZeus/PokeBank-NX/pull/298) | `integration/legality-gen14-major-20261012` `5bf98b11` | CONTAINED in #314 (`eabe41f3`) | Keep open while #314/#327/#328 develop; close as superseded after the legality consolidation is accepted |
| [#296](https://github.com/GlitchedZeus/PokeBank-NX/pull/296) | `feature/legality-l3-moves-transfer-20261012` `aead1faf` | CONTAINED in #314 (`eabe41f3`) | Keep open while #314/#327/#328 develop; close as superseded after the legality consolidation is accepted |
| [#295](https://github.com/GlitchedZeus/PokeBank-NX/pull/295) | `feature/legality-l2-breeding-20261012` `a9da17a7` | CONTAINED in #314 (`eabe41f3`) | Keep open while #314/#327/#328 develop; close as superseded after the legality consolidation is accepted |
| [#294](https://github.com/GlitchedZeus/PokeBank-NX/pull/294) | `audit/legality-l5-research-qa-20261012` `59a95176` | CONTAINED in #314 (`eabe41f3`) | Keep open while #314/#327/#328 develop; close as superseded after the legality consolidation is accepted |
| [#293](https://github.com/GlitchedZeus/PokeBank-NX/pull/293) | `feature/legality-l1-encounters-rng-20261012` `573cbc17` | CONTAINED in #314 (`eabe41f3`) | Keep open while #314/#327/#328 develop; close as superseded after the legality consolidation is accepted |
| [#292](https://github.com/GlitchedZeus/PokeBank-NX/pull/292) | `feature/legality-l4-events-meta-20261012` `03ae8c09` | CONTAINED in #314 (`eabe41f3`) | Keep open while #314/#327/#328 develop; close as superseded after the legality consolidation is accepted |
| [#280](https://github.com/GlitchedZeus/PokeBank-NX/pull/280) | `fix/gen14-action-tap-empty-parity-20261011` `dcd855a7` | UNIQUE — 9 commits (Gen III–IV empty-slot touch parity, Oct 11) | Owner/engineering review: port or retire |
| [#273](https://github.com/GlitchedZeus/PokeBank-NX/pull/273) | `feature/legality-gen4-egg-iv-stage-20261011` `dfa2fb53` | UNIQUE — 2 legality commits not in #314 | Keep; legality review |
| [#252](https://github.com/GlitchedZeus/PokeBank-NX/pull/252) | `audit/gen1-4-legality-research-20261011` `fc0225e6` | UNIQUE — 5 research commits not in #314 | Keep; legality evidence |
| [#230](https://github.com/GlitchedZeus/PokeBank-NX/pull/230) | `feature/legality-sourcefree-egg-move-guard-20261011` `95416a8f` | UNIQUE — top of the #227→#230 stack (6 commits total) | Keep; legality review |
| [#229](https://github.com/GlitchedZeus/PokeBank-NX/pull/229) | `feature/legality-sourcefree-pk12-move-compatibility-20261011` `14baf42c` | UNIQUE — inside the #230 stack | Keep (stacked) |
| [#228](https://github.com/GlitchedZeus/PokeBank-NX/pull/228) | `feature/legality-sourcefree-pk4-palpark-move-history-20261011` `3df1de83` | UNIQUE — inside the #230 stack | Keep (stacked) |
| [#227](https://github.com/GlitchedZeus/PokeBank-NX/pull/227) | `feature/legality-sourcefree-pk4-gen4-move-acquisition-20261011` `a1eea80b` | UNIQUE — inside the #230 stack | Keep (stacked) |
| [#163](https://github.com/GlitchedZeus/PokeBank-NX/pull/163) | `feature/gen5-readonly-foundation-20261008` `79679950` | UNIQUE — 85 commits (early Gen V read-only lane; Gen V since rebuilt on the shared editor) | Owner review: archive tag, then close as obsolete |
| [#144](https://github.com/GlitchedZeus/PokeBank-NX/pull/144) | `audit/gen1-4-pkhex-parity-ledger-20261006` `b450bf25` | UNIQUE — 1 commit (PKHeX parity ledger) | Keep; legality evidence |
| [#122](https://github.com/GlitchedZeus/PokeBank-NX/pull/122) | `feature/touch-controls-20261005` `36c0d1d6` | UNIQUE — 370 commits (touch-control parity lane, Oct 9) | Owner review; large unique history |
| [#103](https://github.com/GlitchedZeus/PokeBank-NX/pull/103) | `feature/legality-engine-gen1-4-20260929` `d29eada5` | CONTAINED in #314 (`eabe41f3`) | Keep open while #314/#327/#328 develop; close as superseded after the legality consolidation is accepted |
| [#92](https://github.com/GlitchedZeus/PokeBank-NX/pull/92) | `feature/gen4-full-editor-20260928` `ddaff247` | INCORPORATED — head `ddaff247` is in the application line | Close only after the consolidation is merged into `main` |

## 4. Open issues (34, plus new tracker #329)

**No issue is closed automatically.** Fixes observed on older builds stay open until the owner verifies them on the current hardware candidate.

| Issue | Topic | Status | Proposed action |
|---|---|---|---|
| [#326](https://github.com/GlitchedZeus/PokeBank-NX/issues/326) | Gen V Move picker learnset | FIXED — pending current hardware verification (passed in Round 5) | Owner verifies on `3634d1e6`, then close |
| [#325](https://github.com/GlitchedZeus/PokeBank-NX/issues/325) | Gen V Language inline | FIXED — pending current hardware verification (NEW-14 passed in Round 5) | Owner verifies on `3634d1e6`, then close |
| [#322](https://github.com/GlitchedZeus/PokeBank-NX/issues/322) | Unify Save Backups / open validated copies | FIXED — pending current hardware verification (R2-EMUBACKUP passed in Round 5) | Owner verifies on `3634d1e6`, then close |
| [#324](https://github.com/GlitchedZeus/PokeBank-NX/issues/324) | Gen II Special column + neutral Gen IV stats | PARTLY FIXED — R2-SPECIAL passed in Round 5; Gen IV part unconfirmed | Owner verifies |
| [#323](https://github.com/GlitchedZeus/PokeBank-NX/issues/323) | Shared Review + Trainer parity | IN PROGRESS — Round 5/6 Review findings; Round 7–10 repairs untested | Keep |
| [#320](https://github.com/GlitchedZeus/PokeBank-NX/issues/320) | P0 A/B/R input regression | FIXED? — later rounds ran 42/59 checks; no explicit record | Owner verifies |
| [#95](https://github.com/GlitchedZeus/PokeBank-NX/issues/95) | G4-04 Gen IV Create + fields | IMPLEMENTED — #92 head is in the application | Close after the consolidation merges and the owner agrees |
| [#313](https://github.com/GlitchedZeus/PokeBank-NX/issues/313) | Gen I–V final acceptance gate | ACTIVE | Keep |
| [#329](https://github.com/GlitchedZeus/PokeBank-NX/issues/329) | Gen I–IX legality master tracker | ACTIVE (new) | Keep |
| [#285](https://github.com/GlitchedZeus/PokeBank-NX/issues/285) | Legality master inventory | ACTIVE — #314 lineage | Keep |
| [#286](https://github.com/GlitchedZeus/PokeBank-NX/issues/286) | L1 encounters lane | Lane PR #293 contained in #314 | Keep while legality develops; revisit after consolidation |
| [#287](https://github.com/GlitchedZeus/PokeBank-NX/issues/287) | L2 breeding lane | Lane PR #295 contained in #314 | Keep while legality develops |
| [#288](https://github.com/GlitchedZeus/PokeBank-NX/issues/288) | L3 moves lane | Lane PR #296 contained in #314 | Keep while legality develops |
| [#289](https://github.com/GlitchedZeus/PokeBank-NX/issues/289) | L4 events lane | Lane PR #292 contained in #314 | Keep while legality develops |
| [#290](https://github.com/GlitchedZeus/PokeBank-NX/issues/290) | L5 research/QA lane | Lane PR #294 contained in #314 | Keep while legality develops |
| [#89](https://github.com/GlitchedZeus/PokeBank-NX/issues/89) | Backups + explicit Inject workflow | ACTIVE / future (injection stays disabled) | Keep |
| [#31](https://github.com/GlitchedZeus/PokeBank-NX/issues/31) | Read-only DS Gen IV/V adapters | IMPLEMENTED? — DraStic/melonDS Gen IV/V sources exist | Owner decides |
| [#69](https://github.com/GlitchedZeus/PokeBank-NX/issues/69) | Full project audit | MOSTLY DONE — remediation in PR #101 (one deferred item) | Owner decides |
| #29, #55, #56, #57, #58, #59, #70 | v1.0 roadmap / release-gate trackers | ACTIVE roadmap | Keep |
| #5, #9, #15, #16, #17, #21, #26, #30, #49, #50 | backlog / research / preservation | ACTIVE backlog | Keep; candidates for the project board's Backlog |

## 5. Branch refs (314)

| Class | Count | Meaning | Archive requirement before any deletion |
|---|---:|---|---|
| `IN_MAIN` | 69 | every commit reachable from the consolidated application | none beyond this ledger's SHA record once `main` holds the merge |
| `IN_LEGALITY_314` | 171 | every commit reachable from #314 `eabe41f3` (unmerged) | archive tag on the #314 head (or its successor) first |
| `MERGES_ONLY` | 1 | no unique non-merge commit | none beyond the SHA record |
| `UNIQUE` | 73 | at least one commit in neither line nor old `main` | `archive/<branch>` tag **and** owner review |

Columns: branch, tip SHA, tip date, class, unique non-merge commits, PRs whose head equals this tip, and the proposed handling. **Protected** = `main`, every open-PR head and base, the Round 9 / Round 10 candidate sources and the branches named in section 1. Deleting a branch never deletes Actions artifacts or merged history, but `blob/<branch>/...` links break, so each deletion needs approval.

<details><summary><strong>Protected (21)</strong></summary>

| Branch | Tip | Date | Class | Unique | PR(s) at tip | Handling |
|---|---|---|---|---:|---|---|
| `audit/legality-integration-hardening-20261012` | `67f56f38` | 2026-10-12 | IN_LEGALITY_314 | 0 | #299 | PROTECTED: open PR head/base |
| `audit/legality-l5-research-qa-20261012` | `59a95176` | 2026-10-13 | IN_LEGALITY_314 | 0 | #294 | PROTECTED: open PR head/base |
| `claude/gen1-5-legality-campaign-w9mkom` | `eabe41f3` | 2026-10-29 | IN_LEGALITY_314 | 0 | #314 | PROTECTED: #314 head (active legality) |
| `claude/gen14-final-20261014` | `4b9b9f65` | 2026-10-14 | IN_MAIN | 0 | #306 | PROTECTED: open PR head/base |
| `claude/gen4-capture-ball` | `b8eb006e` | 2026-10-13 | IN_LEGALITY_314 | 0 | #305 | PROTECTED: open PR head/base |
| `claude/gen5-integration-20261014` | `d6cc878a` | 2026-10-27 | IN_MAIN | 0 | #307 | PROTECTED: #307 head; Round 5/6 hardware documents historically linked here |
| `claude/legality-gen3-negai-boshi-lifetime` | `2761a75d` | 2026-10-14 | IN_LEGALITY_314 | 0 | #308 | PROTECTED: open PR head/base |
| `claude/pensive-cerf-pueceq` | `bc9d8c20` | 2026-10-14 | IN_LEGALITY_314 | 0 | #303 | PROTECTED: open PR head/base |
| `feature/gen4-full-editor-20260928` | `ddaff247` | 2026-10-11 | IN_MAIN | 0 | #92 | PROTECTED: open PR head/base |
| `feature/legality-engine-gen1-4-20260929` | `d29eada5` | 2026-10-12 | IN_LEGALITY_314 | 0 | #103,#283 | PROTECTED: open PR head/base |
| `feature/legality-l1-encounters-rng-20261012` | `573cbc17` | 2026-10-13 | IN_LEGALITY_314 | 0 | #293 | PROTECTED: open PR head/base |
| `feature/legality-l2-breeding-20261012` | `a9da17a7` | 2026-10-13 | IN_LEGALITY_314 | 0 | #295 | PROTECTED: open PR head/base |
| `feature/legality-l3-moves-transfer-20261012` | `aead1faf` | 2026-10-13 | IN_LEGALITY_314 | 0 | #296 | PROTECTED: open PR head/base |
| `feature/legality-l4-events-meta-20261012` | `03ae8c09` | 2026-10-13 | IN_LEGALITY_314 | 0 | #292 | PROTECTED: open PR head/base |
| `feature/legality-sourcefree-pk3-move-acquisition-20261011` | `6fb8c9ba` | 2026-10-11 | IN_LEGALITY_314 | 0 | #226 | PROTECTED: open PR head/base |
| `fix/gen14-narrow-native-actions-20261011` | `d0a45909` | 2026-10-12 | IN_MAIN | 0 | #278 | PROTECTED: open PR head/base |
| `fix/gen4-bag-remove-compaction` | `0842c6f3` | 2026-10-28 | IN_MAIN | 0 |  | PROTECTED: Round 10 candidate 3634d1e6 source, keep unchanged |
| `integration/gen15-combined` | `c4f414a3` | 2026-10-28 | IN_MAIN | 0 |  | PROTECTED: Round 9 candidate d6a20a2a source and audit record |
| `integration/gen4-polish-hardware-20260928` | `8b3bcc16` | 2026-09-28 | IN_MAIN | 0 | #90 | PROTECTED: open PR head/base |
| `integration/legality-gen14-major-20261012` | `5bf98b11` | 2026-10-13 | IN_LEGALITY_314 | 0 | #298 | PROTECTED: open PR head/base |
| `main` | `6fb44150` | 2026-10-25 | IN_MAIN | 0 |  | PROTECTED: default branch |

</details>

<details><summary><strong>Deletion candidates: contained, owner approval required (220)</strong></summary>

| Branch | Tip | Date | Class | Unique | PR(s) at tip | Handling |
|---|---|---|---|---:|---|---|
| `audit/app-qa-20261014` | `aecc9ea6` | 2026-10-27 | IN_MAIN | 0 |  | deletion candidate (approval): none (history in the application) |
| `audit/full-project-hardening-20260923` | `00ee7a6e` | 2026-09-28 | IN_MAIN | 0 | #79 | deletion candidate (approval): none (history in the application) |
| `audit/legality-master-parallel-plan-20261012` | `062f5728` | 2026-10-12 | IN_LEGALITY_314 | 0 | #291 | deletion candidate (approval): archive tag on the #314 head first |
| `checkpoint/pre-session2-recovery-2026-09-01` | `17bad5e7` | 2026-09-01 | IN_MAIN | 0 |  | deletion candidate (approval): none (history in the application) |
| `claude/gen14-campaign-20261015` | `73248a15` | 2026-10-26 | IN_MAIN | 0 |  | deletion candidate (approval): none (history in the application) |
| `claude/gen5-home-providers-20261015` | `ece88e27` | 2026-10-15 | IN_MAIN | 0 | #315 | deletion candidate (approval): none (history in the application) |
| `claude/launch-return-20261015` | `e7bce8cb` | 2026-10-26 | IN_MAIN | 0 |  | deletion candidate (approval): none (history in the application) |
| `claude/pickers-search-scrollbar-20261017` | `e52b7d97` | 2026-10-26 | IN_MAIN | 0 |  | deletion candidate (approval): none (history in the application) |
| `claude/reroll-mechanics-20261017` | `967b75bd` | 2026-10-26 | MERGES_ONLY | 0 |  | deletion candidate (approval): none (history in the application) |
| `claude/shared-editor-gen35-20261016` | `42a92780` | 2026-10-26 | IN_MAIN | 0 |  | deletion candidate (approval): none (history in the application) |
| `codex/gen5-native-create-engine` | `6e27bbd5` | 2026-10-13 | IN_MAIN | 0 | #302 | deletion candidate (approval): none (history in the application) |
| `codex/gen5-profile-save-instances-hardware` | `4d8c267e` | 2026-10-13 | IN_MAIN | 0 | #304 | deletion candidate (approval): none (history in the application) |
| `codex/legality-event3-ribbons-and-alm-audit-20261004` | `65ba9657` | 2026-10-04 | IN_LEGALITY_314 | 0 | #104 | deletion candidate (approval): archive tag on the #314 head first |
| `codex/legality-gen4-bcc-reroll-failed-leads-20261005` | `917c40eb` | 2026-10-05 | IN_LEGALITY_314 | 0 | #133 | deletion candidate (approval): archive tag on the #314 head first |
| `codex/legality-gen4-bcc-reroll-nolead-20261005` | `5714b3e0` | 2026-10-05 | IN_LEGALITY_314 | 0 | #134 | deletion candidate (approval): archive tag on the #314 head first |
| `codex/legality-gen4-bcc-reroll-nolead-central-20261005` | `5a0e3fc2` | 2026-10-05 | IN_LEGALITY_314 | 0 | #138 | deletion candidate (approval): archive tag on the #314 head first |
| `codex/legality-gen4-bcc-reroll-pressure-20261005` | `2bb13109` | 2026-10-05 | IN_LEGALITY_314 | 0 | #131 | deletion candidate (approval): archive tag on the #314 head first |
| `codex/legality-gen4-bcc-reroll-static-magnet-20261005` | `ab61d24c` | 2026-10-05 | IN_LEGALITY_314 | 0 | #132 | deletion candidate (approval): archive tag on the #314 head first |
| `codex/legality-gen4-bcc-reroll-sync-fail-20261005` | `2b776c69` | 2026-10-05 | IN_LEGALITY_314 | 0 | #142 | deletion candidate (approval): archive tag on the #314 head first |
| `codex/legality-gen4-bcc-reroll-sync-success-v2-20261005` | `cf4bdc86` | 2026-10-05 | IN_LEGALITY_314 | 0 | #136 | deletion candidate (approval): archive tag on the #314 head first |
| `codex/legality-gen4-bcc-sync-fail-central-20261005` | `90816ca5` | 2026-10-05 | IN_LEGALITY_314 | 0 | #143 | deletion candidate (approval): archive tag on the #314 head first |
| `codex/legality-gen4-bug-contest-direct-20261005` | `dde366b4` | 2026-10-05 | IN_LEGALITY_314 | 0 | #129 | deletion candidate (approval): archive tag on the #314 head first |
| `codex/legality-gen4-central-reporting-20261005` | `2227bd90` | 2026-10-05 | IN_LEGALITY_314 | 0 | #126 | deletion candidate (approval): archive tag on the #314 head first |
| `codex/legality-gen4-central-reporting-adapter-20261005-copy` | `2227bd90` | 2026-10-05 | IN_LEGALITY_314 | 0 | #126 | deletion candidate (approval): archive tag on the #314 head first |
| `codex/legality-gen4-central-reporting-v2-20261005` | `66f38499` | 2026-10-05 | IN_LEGALITY_314 | 0 | #140 | deletion candidate (approval): archive tag on the #314 head first |
| `codex/legality-gen4-lead-central-20261005` | `978bc764` | 2026-10-05 | IN_LEGALITY_314 | 0 | #127 | deletion candidate (approval): archive tag on the #314 head first |
| `codex/legality-gen4-lead-central-validation-20261005` | `978bc764` | 2026-10-05 | IN_LEGALITY_314 | 0 | #127 | deletion candidate (approval): archive tag on the #314 head first |
| `codex/legality-gen4-lead-failures-20261005` | `762cf1a0` | 2026-10-05 | IN_LEGALITY_314 | 0 | #121 | deletion candidate (approval): archive tag on the #314 head first |
| `codex/legality-gen4-lead-predicates-20261005` | `72e518f7` | 2026-10-05 | IN_LEGALITY_314 | 0 | #120 | deletion candidate (approval): archive tag on the #314 head first |
| `codex/legality-gen4-lead-reporting-20261005` | `2227bd90` | 2026-10-05 | IN_LEGALITY_314 | 0 | #126 | deletion candidate (approval): archive tag on the #314 head first |
| `codex/legality-gen4-min31-reroll-frames-20261005` | `c8599aef` | 2026-10-05 | IN_LEGALITY_314 | 0 | #130 | deletion candidate (approval): archive tag on the #314 head first |
| `codex/legality-gen4-pressure-grass-source-20261005` | `1410886b` | 2026-10-05 | IN_LEGALITY_314 | 0 | #125 | deletion candidate (approval): archive tag on the #314 head first |
| `codex/legality-gen4-pressure-success-20261005` | `3bcca932` | 2026-10-05 | IN_LEGALITY_314 | 0 | #124 | deletion candidate (approval): archive tag on the #314 head first |
| `codex/legality-gen4-reporting-adapter-v2-20261005` | `82d55f86` | 2026-10-05 | IN_LEGALITY_314 | 0 | #139 | deletion candidate (approval): archive tag on the #314 head first |
| `codex/legality-gen4-reporting-production-hook-20261005` | `e4684c7a` | 2026-10-05 | IN_LEGALITY_314 | 0 | #141 | deletion candidate (approval): archive tag on the #314 head first |
| `codex/legality-gen4-safari-reroll-nolead-20261005` | `fa557ae4` | 2026-10-05 | IN_LEGALITY_314 | 0 | #137 | deletion candidate (approval): archive tag on the #314 head first |
| `codex/legality-gen4-static-central-20261005` | `fbe447ef` | 2026-10-05 | IN_LEGALITY_314 | 0 | #110 | deletion candidate (approval): archive tag on the #314 head first |
| `codex/legality-gen4-static-central-form-combined-20261005` | `b8d2c1c1` | 2026-10-05 | IN_LEGALITY_314 | 0 | #111 | deletion candidate (approval): archive tag on the #314 head first |
| `codex/legality-gen4-static-egg-evolution-20261005` | `a634460a` | 2026-10-05 | IN_LEGALITY_314 | 0 | #108 | deletion candidate (approval): archive tag on the #314 head first |
| `codex/legality-gen4-static-egg-state-20261005` | `fa401310` | 2026-10-05 | IN_LEGALITY_314 | 0 | #115 | deletion candidate (approval): archive tag on the #314 head first |
| `codex/legality-gen4-static-evolution-20261005` | `324c99f6` | 2026-10-05 | IN_LEGALITY_314 | 0 | #107 | deletion candidate (approval): archive tag on the #314 head first |
| `codex/legality-gen4-static-magnet-success-20261005` | `a54243a9` | 2026-10-05 | IN_LEGALITY_314 | 0 | #123 | deletion candidate (approval): archive tag on the #314 head first |
| `codex/legality-wurmple-evidence-20261005` | `c12fc374` | 2026-10-05 | IN_LEGALITY_314 | 0 | #105 | deletion candidate (approval): archive tag on the #314 head first |
| `codex/legality-wurmple-source-proof-20261005` | `21a48650` | 2026-10-05 | IN_LEGALITY_314 | 0 | #106 | deletion candidate (approval): archive tag on the #314 head first |
| `dev/classic-inventory-20260912` | `f603e3e4` | 2026-09-13 | IN_MAIN | 0 |  | deletion candidate (approval): none (history in the application) |
| `dev/inventory-footer-readability-candidate-20260913` | `fc1e6de2` | 2026-09-13 | IN_MAIN | 0 |  | deletion candidate (approval): none (history in the application) |
| `dev/standalone-runtime-final-20260912` | `74698c3f` | 2026-09-12 | IN_MAIN | 0 |  | deletion candidate (approval): none (history in the application) |
| `docs/issue71-production-status-20260919` | `96da3c1d` | 2026-09-19 | IN_MAIN | 0 | #76 | deletion candidate (approval): none (history in the application) |
| `docs/legality-20261011-accepted-through-242` | `06eda48b` | 2026-10-11 | IN_LEGALITY_314 | 0 | #248 | deletion candidate (approval): archive tag on the #314 head first |
| `docs/legality-gen1-4-parity-through-186-20261009` | `f458bc4b` | 2026-10-09 | IN_LEGALITY_314 | 0 | #189 | deletion candidate (approval): archive tag on the #314 head first |
| `docs/legality-gen4-methodk-reroll-parity-20261009` | `6263aff7` | 2026-10-09 | IN_LEGALITY_314 | 0 | #181 | deletion candidate (approval): archive tag on the #314 head first |
| `docs/legality-move-history-checkpoint-20261011` | `8bd51041` | 2026-10-11 | IN_LEGALITY_314 | 0 | #239 | deletion candidate (approval): archive tag on the #314 head first |
| `docs/legality-parity-checkpoint-20261010` | `96c5eaaf` | 2026-10-10 | IN_LEGALITY_314 | 0 | #206 | deletion candidate (approval): archive tag on the #314 head first |
| `docs/legality-pkhex-parity-ledger-20261006` | `350f7018` | 2026-10-06 | IN_LEGALITY_314 | 0 | #145 | deletion candidate (approval): archive tag on the #314 head first |
| `docs/legality-sourcefree-parity-checkpoint-20261011` | `87b2fa73` | 2026-10-11 | IN_LEGALITY_314 | 0 | #224 | deletion candidate (approval): archive tag on the #314 head first |
| `docs/refresh-frontpage-20261012` | `ff450262` | 2026-10-12 | IN_MAIN | 0 | #297 | deletion candidate (approval): none (history in the application) |
| `feature/gen1-editor-ux-cleanup2-20260913-copy` | `2107ac40` | 2026-09-13 | IN_MAIN | 0 |  | deletion candidate (approval): none (history in the application) |
| `feature/gen1-editor-ux-cleanup2-20260913` | `6ecd9c28` | 2026-09-13 | IN_MAIN | 0 | #64 | deletion candidate (approval): none (history in the application) |
| `feature/gen1-editor-ux-cleanup3-20260913` | `0fd59ae9` | 2026-09-13 | IN_MAIN | 0 | #65 | deletion candidate (approval): none (history in the application) |
| `feature/gen1-editor-ux-polish-20260913` | `c3ad2be6` | 2026-09-13 | IN_MAIN | 0 | #63 | deletion candidate (approval): none (history in the application) |
| `feature/gen1-party-storage-view-unification-20260914` | `5a2642ac` | 2026-09-14 | IN_MAIN | 0 | #67 | deletion candidate (approval): none (history in the application) |
| `feature/gen1-staged-editor-recovery-20260913` | `574d604b` | 2026-09-13 | IN_MAIN | 0 | #61 | deletion candidate (approval): none (history in the application) |
| `feature/gen12-packed-y-multimove-20260917-copyprobe2` | `16a4852b` | 2026-09-17 | IN_MAIN | 0 |  | deletion candidate (approval): none (history in the application) |
| `feature/gen12-packed-y-multimove-20260917-copyprobe` | `b9b3de57` | 2026-09-17 | IN_MAIN | 0 |  | deletion candidate (approval): none (history in the application) |
| `feature/gen12-packed-y-multimove-20260917` | `c24859ce` | 2026-09-18 | IN_MAIN | 0 | #74 | deletion candidate (approval): none (history in the application) |
| `feature/gen2-shared-pokemon-editor-20260914` | `cfb70357` | 2026-09-15 | IN_MAIN | 0 | #68 | deletion candidate (approval): none (history in the application) |
| `feature/gen3-shared-pokemon-editor-20260919` | `996e6aa4` | 2026-09-23 | IN_MAIN | 0 | #77 | deletion candidate (approval): none (history in the application) |
| `feature/gen4-shared-pokemon-editor-20260928` | `6e9f54f2` | 2026-09-28 | IN_MAIN | 0 | #87 | deletion candidate (approval): none (history in the application) |
| `feature/legality-breed002-daycare-order-20261011` | `ae54aef4` | 2026-10-11 | IN_LEGALITY_314 | 0 | #272 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-candidate-palpark-history-20261011` | `b46f766c` | 2026-10-11 | IN_LEGALITY_314 | 0 | #260 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-candidate-scoped-move-witnesses-20261011` | `f51f3357` | 2026-10-11 | IN_LEGALITY_314 | 0 | #253 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-coherent-history-milestone-20261011` | `c87fd598` | 2026-10-11 | IN_LEGALITY_314 | 0 | #268 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-dppt-marsh-evolved-safari-native-20261009` | `df91a69e` | 2026-10-09 | IN_LEGALITY_314 | 0 | #195 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-emerald-levelup-pinned-evidence-20261011` | `b12f4225` | 2026-10-11 | IN_LEGALITY_314 | 0 | #247 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-emerald-shedinja-move-history-20261011` | `c4e1a850` | 2026-10-11 | IN_LEGALITY_314 | 0 | #254 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gamecube-palpark-move-stage-20261011` | `d7c72389` | 2026-10-11 | IN_LEGALITY_314 | 0 | #242 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen1-branched-catch-rate-ancestry-20261010` | `b9e93f8f` | 2026-10-10 | IN_LEGALITY_314 | 0 | #207 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen12-joint-sourcefree-moves-20261011` | `c03e7d9f` | 2026-10-11 | IN_LEGALITY_314 | 0 | #234 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen12-sourcefree-format-ceilings-20261010` | `1090b7c6` | 2026-10-10 | IN_LEGALITY_314 | 0 | #216 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen12-time-capsule-directional-reporting-20261010` | `d3e17d28` | 2026-10-10 | IN_LEGALITY_314 | 0 | #209 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen2-crystal-boxed-pk2-report-20261010` | `697d5731` | 2026-10-10 | IN_LEGALITY_314 | 0 | #205 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen2-crystal-dratini-gift-evolution-20261010` | `85075ef8` | 2026-10-10 | IN_LEGALITY_314 | 0 | #202 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen2-crystal-eevee-evolution-gift-20261009` | `82c7e56e` | 2026-10-09 | IN_LEGALITY_314 | 0 | #200 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen2-crystal-evolved-wild-20261009` | `2d7d439d` | 2026-10-09 | IN_LEGALITY_314 | 0 | #196 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen2-crystal-native-pk2-caught-origin-20261009` | `8cf2fb78` | 2026-10-09 | IN_LEGALITY_314 | 0 | #197 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen2-crystal-raw-save-pipeline-20261009` | `e7bb7c71` | 2026-10-09 | IN_LEGALITY_314 | 0 | #199 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen2-crystal-red-gyarados-shiny-native-20261010` | `bf9d2e41` | 2026-10-10 | IN_LEGALITY_314 | 0 | #203 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen2-crystal-six-evolution-families-20261009` | `7c9fe9dd` | 2026-10-09 | IN_LEGALITY_314 | 0 | #198 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen2-crystal-starter-evolution-gifts-20261009` | `a99a9b50` | 2026-10-09 | IN_LEGALITY_314 | 0 | #201 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen2-crystal-tyrogue-gift-evolution-20261010` | `3219c531` | 2026-10-10 | IN_LEGALITY_314 | 0 | #204 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen2-sourcefree-timecapsule-report-20261010` | `55242544` | 2026-10-10 | IN_LEGALITY_314 | 0 | #217 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen3-detached-wild-evidence-20261011` | `98a7b627` | 2026-10-11 | IN_LEGALITY_314 | 0 | #225 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen3-egg-iv-retail-stage-20261011` | `5c0a117a` | 2026-10-11 | IN_LEGALITY_314 | 0 | #259 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen3-emerald-safari-block-cutecharm-fail-20261008` | `b97ff76f` | 2026-10-08 | IN_LEGALITY_314 | 0 | #169 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen3-emerald-safari-block-grass-pressure-20261008` | `c6ec0d7e` | 2026-10-08 | IN_LEGALITY_314 | 0 | #168 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen3-emerald-safari-block-intimidate-20261008` | `1f5f2027` | 2026-10-08 | IN_LEGALITY_314 | 0 | #171 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen3-emerald-safari-block-static-success-20261008` | `7a3d895b` | 2026-10-08 | IN_LEGALITY_314 | 0 | #167 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen3-emerald-safari-block-sync-fail-20261008` | `4ecb89b7` | 2026-10-08 | IN_LEGALITY_314 | 0 | #170 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen3-emerald-safari-cute-fail-20261008` | `d285c6cd` | 2026-10-08 | IN_LEGALITY_314 | 0 | #160 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen3-emerald-safari-grass-pressure-20261008` | `7908f59d` | 2026-10-08 | IN_LEGALITY_314 | 0 | #164 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen3-emerald-safari-intimidate-20261008` | `17c70890` | 2026-10-08 | IN_LEGALITY_314 | 0 | #166 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen3-emerald-safari-nolead-20261007` | `a4910509` | 2026-10-07 | IN_LEGALITY_314 | 0 | #157 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen3-emerald-safari-pressure-fail-20261008` | `e4c996b4` | 2026-10-08 | IN_LEGALITY_314 | 0 | #161 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen3-emerald-safari-pressure-success-20261008` | `7373617d` | 2026-10-08 | IN_LEGALITY_314 | 0 | #162 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen3-emerald-safari-static-success-20261008` | `f115f7f6` | 2026-10-08 | IN_LEGALITY_314 | 0 | #165 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen3-emerald-safari-sync-20261007` | `44050980` | 2026-10-07 | IN_LEGALITY_314 | 0 | #158 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen3-emerald-safari-sync-fail-20261008` | `a872b911` | 2026-10-08 | IN_LEGALITY_314 | 0 | #159 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen3-joint-move-pool-20261011` | `e0a1b3fe` | 2026-10-11 | IN_LEGALITY_314 | 0 | #231 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen3-safari-ball-production-info-20261009` | `03ea5153` | 2026-10-09 | IN_LEGALITY_314 | 0 | #186 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen3-safari-emerald-nolead-20261007` | `51d35e57` | 2026-10-07 | IN_LEGALITY_314 | 0 | #156 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen3-safari-evidence-20261007` | `f8e859ec` | 2026-10-07 | IN_LEGALITY_314 | 0 | #152 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen3-safari-method3-20261007` | `51d35e57` | 2026-10-07 | IN_LEGALITY_314 | 0 | #156 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen3-safari-methodh-regular-20261007` | `eee4fd41` | 2026-10-07 | IN_LEGALITY_314 | 0 | #154 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen3-safari-origin-resolution-20261007` | `76b6449c` | 2026-10-07 | IN_LEGALITY_314 | 0 | #153 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen34-ball-domain-20261006` | `0b16a0bd` | 2026-10-06 | IN_LEGALITY_314 | 0 | #148 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen34-ball-domain-production-20261006` | `ff1b15ca` | 2026-10-06 | IN_LEGALITY_314 | 0 | #149 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen34-full-palpark-chain-20261011` | `f6a17e15` | 2026-10-11 | IN_LEGALITY_314 | 0 | #281 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen34-language-domain-20261006` | `22af43f1` | 2026-10-06 | IN_LEGALITY_314 | 0 | #146 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen34-language-production-20261006` | `255d4f42` | 2026-10-06 | IN_LEGALITY_314 | 0 | #147 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen34-native-egg-pokeball-info-20261009` | `7bb678fe` | 2026-10-09 | IN_LEGALITY_314 | 0 | #190 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen34-ordinary-egg-move-construction-20261011` | `32b863d6` | 2026-10-11 | IN_LEGALITY_314 | 0 | #257 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen34-parent-eligibility-20261011` | `07774772` | 2026-10-11 | IN_LEGALITY_314 | 0 | #265 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen34-same-parent-moveset-20261011` | `5655e1f9` | 2026-10-11 | IN_LEGALITY_314 | 0 | #262 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen34-sourcefree-egg-move-origins-20261010` | `f2e637fb` | 2026-10-10 | IN_LEGALITY_314 | 0 | #215 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen34-sourcefree-egg-state-20261010` | `a9ea1a49` | 2026-10-10 | IN_LEGALITY_314 | 0 | #212 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen34-sourcefree-format-domains-20261010` | `d5ab1f57` | 2026-10-10 | IN_LEGALITY_314 | 0 | #210 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen34-sourcefree-species-moves-20261010` | `2dbb8dd6` | 2026-10-10 | IN_LEGALITY_314 | 0 | #213 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen4-bcc-mixed-sync-20261009` | `d767b12f` | 2026-10-09 | IN_LEGALITY_314 | 0 | #173 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen4-bcc-mixed-sync-depth23-20261009` | `650a9f0a` | 2026-10-09 | IN_LEGALITY_314 | 0 | #178 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen4-bcc-mixed-sync-depth23-reporting-20261009` | `d3a9053d` | 2026-10-09 | IN_LEGALITY_314 | 0 | #179 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen4-bcc-mixed-sync-real-source-report-20261009` | `66c0b004` | 2026-10-09 | IN_LEGALITY_314 | 0 | #177 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen4-bcc-mixed-sync-reporting-20261009` | `dc1dd291` | 2026-10-09 | IN_LEGALITY_314 | 0 | #174 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen4-bcc-mixed-sync-reverse-20261009` | `8f402d54` | 2026-10-09 | IN_LEGALITY_314 | 0 | #175 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen4-bcc-mixed-sync-reverse-reporting-20261009` | `34f420b8` | 2026-10-09 | IN_LEGALITY_314 | 0 | #176 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen4-bcc-multidepth-central-report-20261009` | `b9bf7226` | 2026-10-09 | IN_LEGALITY_314 | 0 | #180 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen4-bcc-pid-nature-rejections-20261009` | `98f411d9` | 2026-10-09 | IN_LEGALITY_314 | 0 | #182 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen4-breeding-candidate-universe-20261011` | `2fe63bce` | 2026-10-11 | IN_LEGALITY_314 | 0 | #279 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen4-coherent-egg-parent-history-20261011` | `41721756` | 2026-10-11 | IN_LEGALITY_314 | 0 | #276 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen4-contest-ribbon-prefix-20261011` | `49f3c426` | 2026-10-11 | IN_LEGALITY_314 | 0 | #258 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen4-detached-joint-moves-20261011` | `1b5667c1` | 2026-10-11 | IN_LEGALITY_314 | 0 | #232 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen4-egg-iv-power-history-20261011` | `806027d1` | 2026-10-11 | IN_LEGALITY_314 | 0 | #275 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen4-egg-move-source-family-20261011` | `73c4068e` | 2026-10-11 | IN_LEGALITY_314 | 0 | #249 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen4-egg-vs-wild-ancestry-20261009` | `2909edc2` | 2026-10-09 | IN_LEGALITY_314 | 0 | #194 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen4-event-joint-moves-20261011` | `780007ae` | 2026-10-11 | IN_LEGALITY_314 | 0 | #263 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen4-event-persisted-ribbons-20261011` | `ffc5ea78` | 2026-10-11 | IN_LEGALITY_314 | 0 | #264 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen4-evolved-special-ball-20261009` | `9569cc6f` | 2026-10-09 | IN_LEGALITY_314 | 0 | #192 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen4-fixed-ball-origin-resolution-20261007` | `57217337` | 2026-10-07 | IN_LEGALITY_314 | 0 | #151 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen4-fixed-ball-provenance-20261006` | `75f99942` | 2026-10-06 | IN_LEGALITY_314 | 0 | #150 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen4-great-marsh-ball-provenance-20261009` | `d0b3dfe5` | 2026-10-09 | IN_LEGALITY_314 | 0 | #185 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen4-hgss-apricorn-wild-ball-20261009` | `78bd91cd` | 2026-10-09 | IN_LEGALITY_314 | 0 | #191 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen4-native-daycare-parent-snapshots-20261011` | `2be50d68` | 2026-10-11 | IN_LEGALITY_314 | 0 | #274 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen4-native-pk4-language-regression-20261009` | `17342ec2` | 2026-10-09 | IN_LEGALITY_314 | 0 | #172 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen4-native-pk4-multiretry-report-20261009` | `f33d27f1` | 2026-10-09 | IN_LEGALITY_314 | 0 | #187 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen4-nincada-shedinja-bcc-ball-20261009` | `5d2973a4` | 2026-10-09 | IN_LEGALITY_314 | 0 | #188 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen4-platinum-egg-pid-rng-20261012` | `d29eada5` | 2026-10-12 | IN_LEGALITY_314 | 0 | #103,#283 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen4-sourcefree-npc-trades-20261011` | `fecfbca9` | 2026-10-11 | IN_LEGALITY_314 | 0 | #221 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen4-sourcefree-palpark-fields-20261010` | `16c74353` | 2026-10-10 | IN_LEGALITY_314 | 0 | #214 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen4-sourcefree-ranger-manaphy-20261011` | `cc086802` | 2026-10-11 | IN_LEGALITY_314 | 0 | #222 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen4-sourcefree-retail-wild-20261011` | `d2e84eb6` | 2026-10-11 | IN_LEGALITY_314 | 0 | #219 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen4-sourcefree-static-gifts-20261011` | `47f3ec31` | 2026-10-11 | IN_LEGALITY_314 | 0 | #220 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen4-sourcefree-wc4-events-20261011` | `7b65baaa` | 2026-10-11 | IN_LEGALITY_314 | 0 | #223 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen4-special-ball-native-pk4-20261009` | `4cdf5381` | 2026-10-09 | IN_LEGALITY_314 | 0 | #184 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen4-special-ball-source-info-20261009` | `8e583094` | 2026-10-09 | IN_LEGALITY_314 | 0 | #183 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen4-unresolved-hatch-origin-guard-20261010` | `886574fd` | 2026-10-10 | IN_LEGALITY_314 | 0 | #211 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen4-wild-source-linked-rng-20261011` | `0dccf2e1` | 2026-10-12 | IN_LEGALITY_314 | 0 | #282 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-gen4-wurmple-special-ball-pid-20261009` | `6bcce570` | 2026-10-09 | IN_LEGALITY_314 | 0 | #193 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-hgss-egg-pid-stage-20261012` | `dd395c3c` | 2026-10-12 | IN_LEGALITY_314 | 0 | #284 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-history-sequence-milestone-20261011` | `fe3e3d05` | 2026-10-11 | IN_LEGALITY_314 | 0 | #269 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-integrate-rng-palpark-with-breeding-20261011` | `d8a2f2eb` | 2026-10-11 | IN_LEGALITY_314 | 0 | #277 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-native-source-format-mismatch-20261010` | `bad141a8` | 2026-10-11 | IN_LEGALITY_314 | 0 | #218 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-palpark-exact-source-move-stage-20261011` | `1b011823` | 2026-10-11 | IN_LEGALITY_314 | 0 | #236 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-palpark-gen3-ancestor-move-retention-20261011` | `7b122ed8` | 2026-10-11 | IN_LEGALITY_314 | 0 | #241 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-palpark-gen4-evolution-lineage-20261011` | `37071451` | 2026-10-11 | IN_LEGALITY_314 | 0 | #240 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-palpark-hm-boundary-20261011` | `bcd65775` | 2026-10-11 | IN_LEGALITY_314 | 0 | #256 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-palpark-move-id-bound-20261011` | `5b324590` | 2026-10-11 | IN_LEGALITY_314 | 0 | #237 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-palpark-move-stage-20261011` | `60ef8796` | 2026-10-11 | IN_LEGALITY_314 | 0 | #233 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-palpark-stage-structural-guard-20261011` | `95fd6e10` | 2026-10-11 | IN_LEGALITY_314 | 0 | #235 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-palpark-two-era-joint-moves-20261011` | `145002e9` | 2026-10-11 | IN_LEGALITY_314 | 0 | #238 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-pk2-conditional-timecapsule-stages-20261011` | `12a6db65` | 2026-10-11 | IN_LEGALITY_314 | 0 | #244 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-pk2-timecapsule-invalid-move-reporting-20261011` | `916e9931` | 2026-10-11 | IN_LEGALITY_314 | 0 | #245 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-pk3-gamecube-unanchored-moves-20261011` | `6c0259bf` | 2026-10-11 | IN_LEGALITY_314 | 0 | #246 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-pk4-raw-text-capabilities-20261011` | `a0285476` | 2026-10-11 | IN_LEGALITY_314 | 0 | #261 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-sourcefree-pk2-timecapsule-ancestry-20261011` | `4d3026e0` | 2026-10-11 | IN_LEGALITY_314 | 0 | #243 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/legality-timecapsule-directional-20261011` | `54afda53` | 2026-10-11 | IN_LEGALITY_314 | 0 | #266 | deletion candidate (approval): archive tag on the #314 head first |
| `feature/multilingual-gen15` | `e6bdbebb` | 2026-10-28 | IN_MAIN | 0 |  | deletion candidate (approval): none (history in the application) |
| `feature/pokebank-playable` | `3aeecb80` | 2026-09-19 | IN_MAIN | 0 |  | deletion candidate (approval): none (history in the application) |
| `feature/pokemon-editor-ui-foundation-final-20260913` | `1cca8f6a` | 2026-09-14 | IN_MAIN | 0 | #66 | deletion candidate (approval): none (history in the application) |
| `feature/touch-modal-isolation-20261011` | `bbe7e97f` | 2026-10-11 | IN_MAIN | 0 | #267 | deletion candidate (approval): none (history in the application) |
| `feature/universal-editor-contract-audit-20260918` | `fb28323c` | 2026-09-18 | IN_MAIN | 0 | #75 | deletion candidate (approval): none (history in the application) |
| `fix/full-audit-remediation-20260929` | `408a5644` | 2026-10-01 | IN_MAIN | 0 | #101 | deletion candidate (approval): none (history in the application) |
| `fix/gen14-touch-ux-parity-20261011` | `9f4002a8` | 2026-10-11 | IN_MAIN | 0 | #271 | deletion candidate (approval): none (history in the application) |
| `fix/shared-editor-neutral-headings-20260915` | `b695980b` | 2026-09-17 | IN_MAIN | 0 | #73 | deletion candidate (approval): none (history in the application) |
| `integration/gen14-touch-hardware-20261010` | `b1fcbd3e` | 2026-10-11 | IN_MAIN | 0 | #208 | deletion candidate (approval): none (history in the application) |
| `integration/gen5-ui-hardware-candidate-20261012` | `58bd564b` | 2026-10-14 | IN_MAIN | 0 | #300 | deletion candidate (approval): none (history in the application) |
| `integration/legality-gen1-4-method-witnesses-20261011` | `2dd8f7a6` | 2026-10-11 | IN_LEGALITY_314 | 0 | #255 | deletion candidate (approval): archive tag on the #314 head first |
| `recovery/interrupted-2026-09-01` | `3101fc09` | 2026-09-01 | IN_MAIN | 0 |  | deletion candidate (approval): none (history in the application) |
| `scratch/do-not-use` | `cf42d853` | 2026-10-05 | IN_MAIN | 0 |  | deletion candidate (approval): none (history in the application) |
| `staging/product-home-perf-audit-v2-20261004` | `564bb3ed` | 2026-10-04 | IN_MAIN | 0 |  | deletion candidate (approval): none (history in the application) |
| `tmp-do-not-use` | `2107ac40` | 2026-09-13 | IN_MAIN | 0 |  | deletion candidate (approval): none (history in the application) |
| `tmp-rby-adapter` | `0b592145` | 2026-09-10 | IN_MAIN | 0 | #51 | deletion candidate (approval): none (history in the application) |
| `ui/pokemon-product-polish-20260929` | `5b346dd3` | 2026-09-29 | IN_MAIN | 0 | #100 | deletion candidate (approval): none (history in the application) |
| `ui/product-shell-round2-20260928` | `5e4cf038` | 2026-09-28 | IN_MAIN | 0 | #97 | deletion candidate (approval): none (history in the application) |
| `wip/c-r5-review-20261024` | `dca9880c` | 2026-10-24 | IN_MAIN | 0 |  | deletion candidate (approval): none (history in the application) |
| `wip/c-r7-special-20261026` | `cbbdcb12` | 2026-10-26 | IN_MAIN | 0 |  | deletion candidate (approval): none (history in the application) |
| `wip/codex-round4-takeover-20261021` | `6f4cac5c` | 2026-10-22 | IN_MAIN | 0 |  | deletion candidate (approval): none (history in the application) |
| `wip/coord-gen5-direct-open-20261020` | `61491b53` | 2026-10-20 | IN_MAIN | 0 |  | deletion candidate (approval): none (history in the application) |
| `wip/coord-r5-gen3-held-legal-20261025` | `15c77226` | 2026-10-25 | IN_MAIN | 0 |  | deletion candidate (approval): none (history in the application) |
| `wip/l-backup-browser-20261020` | `44fb4b7e` | 2026-10-20 | IN_MAIN | 0 |  | deletion candidate (approval): none (history in the application) |
| `wip/l-r5-backup-copy-20261024` | `1eec6f18` | 2026-10-25 | IN_MAIN | 0 |  | deletion candidate (approval): none (history in the application) |
| `wip/l-r6-bag-unavailable-review-20261026` | `76966b43` | 2026-10-26 | IN_MAIN | 0 |  | deletion candidate (approval): none (history in the application) |
| `wip/l-r6-review-entry-20261025` | `7152178b` | 2026-10-26 | IN_MAIN | 0 |  | deletion candidate (approval): none (history in the application) |
| `wip/l-r7-review-keys-gen5-backups-20261026` | `42bf612d` | 2026-10-26 | IN_MAIN | 0 |  | deletion candidate (approval): none (history in the application) |
| `wip/lane-r-round7-b04` | `bfff58d0` | 2026-10-26 | IN_MAIN | 0 |  | deletion candidate (approval): none (history in the application) |
| `wip/lane-r-round7-unown` | `47b8097e` | 2026-10-26 | IN_MAIN | 0 |  | deletion candidate (approval): none (history in the application) |
| `wip/legality-evidence-20261025` | `eabe41f3` | 2026-10-29 | IN_LEGALITY_314 | 0 | #314 | deletion candidate (approval): archive tag on the #314 head first |
| `wip/p-classic-picker-dismiss-20261026` | `efb687eb` | 2026-10-26 | IN_MAIN | 0 |  | deletion candidate (approval): none (history in the application) |
| `wip/p-round6-20261025` | `0d68e8b7` | 2026-10-26 | IN_MAIN | 0 |  | deletion candidate (approval): none (history in the application) |
| `wip/p-round7-20261026` | `727fad5d` | 2026-10-26 | IN_MAIN | 0 |  | deletion candidate (approval): none (history in the application) |
| `wip/pkse-1.2.x-gen15-audit` | `44f24400` | 2026-10-28 | IN_MAIN | 0 |  | deletion candidate (approval): none (history in the application) |
| `wip/s-gen3-create-review-shape` | `3c929189` | 2026-10-26 | IN_MAIN | 0 |  | deletion candidate (approval): none (history in the application) |

</details>

<details><summary><strong>Unique work: keep (73)</strong></summary>

| Branch | Tip | Date | Class | Unique | PR(s) at tip | Handling |
|---|---|---|---|---:|---|---|
| `archive/qa-campaigns-2026-10-28` | `c0f1c967` | 2026-10-29 | UNIQUE | 3 |  | KEEP: 3 unique patch(es); archive tag + owner review before any deletion |
| `audit/full-repository-line-by-line-20260928` | `143c5e5c` | 2026-09-29 | UNIQUE | 182 |  | KEEP: 182 unique patch(es); archive tag + owner review before any deletion |
| `audit/gen1-4-legality-research-20261011` | `fc0225e6` | 2026-10-11 | UNIQUE | 5 | #252 | KEEP: 5 unique patch(es); archive tag + owner review before any deletion |
| `audit/gen1-4-pkhex-parity-ledger-20261006` | `b450bf25` | 2026-10-06 | UNIQUE | 1 | #144 | KEEP: 1 unique patch(es); archive tag + owner review before any deletion |
| `audit/pre-gen3-shared-editor-71` | `c363505e` | 2026-09-15 | UNIQUE | 9 | #72 | KEEP: 9 unique patch(es); archive tag + owner review before any deletion |
| `ci/classic-inventory-corrected-validation-20260913` | `a0280e3e` | 2026-09-13 | UNIQUE | 1 |  | KEEP: 1 unique patch(es); archive tag + owner review before any deletion |
| `ci/classic-inventory-correction-20260913` | `4d11237b` | 2026-09-13 | UNIQUE | 5 |  | KEEP: 5 unique patch(es); archive tag + owner review before any deletion |
| `ci/classic-inventory-final-validation-20260913` | `338e6873` | 2026-09-13 | UNIQUE | 1 |  | KEEP: 1 unique patch(es); archive tag + owner review before any deletion |
| `ci/classic-inventory-production-final-20260913` | `55cdd5c2` | 2026-09-13 | UNIQUE | 1 |  | KEEP: 1 unique patch(es); archive tag + owner review before any deletion |
| `ci/classic-inventory-wiring-20260913` | `87bf28de` | 2026-09-13 | UNIQUE | 5 |  | KEEP: 5 unique patch(es); archive tag + owner review before any deletion |
| `ci/gen2-final-artifact-20260913` | `ccb14a3b` | 2026-09-12 | UNIQUE | 1 |  | KEEP: 1 unique patch(es); archive tag + owner review before any deletion |
| `ci/gen2-final-focused-20260912` | `0656096d` | 2026-09-12 | UNIQUE | 54 |  | KEEP: 54 unique patch(es); archive tag + owner review before any deletion |
| `ci/gen2-final-native-20260912` | `66034438` | 2026-09-12 | UNIQUE | 54 |  | KEEP: 54 unique patch(es); archive tag + owner review before any deletion |
| `ci/gen2-final-prod-validate-20260913` | `fd176de0` | 2026-09-12 | UNIQUE | 1 |  | KEEP: 1 unique patch(es); archive tag + owner review before any deletion |
| `ci/gen2-prod-validate-20260913` | `d57a503b` | 2026-09-12 | UNIQUE | 1 |  | KEEP: 1 unique patch(es); archive tag + owner review before any deletion |
| `ci/gsc-gender-validation-20260912` | `92d5d3bf` | 2026-09-12 | UNIQUE | 3 |  | KEEP: 3 unique patch(es); archive tag + owner review before any deletion |
| `ci/inventory-dirty-final-validation-20260913` | `19343ab8` | 2026-09-13 | UNIQUE | 12 |  | KEEP: 12 unique patch(es); archive tag + owner review before any deletion |
| `ci/inventory-dirty-validation-20260913` | `f2b61c9d` | 2026-09-13 | UNIQUE | 10 |  | KEEP: 10 unique patch(es); archive tag + owner review before any deletion |
| `ci/inventory-footer-readability-validation-20260913` | `3e588c02` | 2026-09-13 | UNIQUE | 1 |  | KEEP: 1 unique patch(es); archive tag + owner review before any deletion |
| `ci/inventory-hardware-polish-validation-20260913` | `1b3c9b84` | 2026-09-13 | UNIQUE | 6 |  | KEEP: 6 unique patch(es); archive tag + owner review before any deletion |
| `ci/inventory-ui-retest-production-artifact-20260913` | `0040e9ba` | 2026-09-13 | UNIQUE | 1 |  | KEEP: 1 unique patch(es); archive tag + owner review before any deletion |
| `ci/standalone-final-host-validation-20260912` | `6ab062e2` | 2026-09-12 | UNIQUE | 3 |  | KEEP: 3 unique patch(es); archive tag + owner review before any deletion |
| `ci/standalone-final-validation-20260912` | `aaa7a7fb` | 2026-09-12 | UNIQUE | 4 |  | KEEP: 4 unique patch(es); archive tag + owner review before any deletion |
| `ci/standalone-production-artifact-20260912` | `ed19625f` | 2026-09-12 | UNIQUE | 1 |  | KEEP: 1 unique patch(es); archive tag + owner review before any deletion |
| `ci/standalone-validation-20260912` | `5bce5864` | 2026-09-12 | UNIQUE | 3 |  | KEEP: 3 unique patch(es); archive tag + owner review before any deletion |
| `claude/gen5-handoff-from-legality-20261014` | `fc519414` | 2026-10-14 | UNIQUE | 1 |  | KEEP: 1 unique patch(es); archive tag + owner review before any deletion |
| `claude/gen6-native-20261014` | `5e683690` | 2026-10-15 | UNIQUE | 6 | #309 | KEEP: earlier point of `claude/gen7-native-20261014`; archive tag + owner review before any deletion |
| `claude/gen7-native-20261014` | `2438b5d0` | 2026-10-15 | UNIQUE | 13 | #311 | KEEP: earlier point of `claude/gen8-native-20261015`; archive tag + owner review before any deletion |
| `claude/gen8-native-20261015` | `2f065ee2` | 2026-10-15 | UNIQUE | 14 | #312 | KEEP: 14 unique patch(es); archive tag + owner review before any deletion |
| `claude/item-quantity-policy-20261016` | `f6acf510` | 2026-10-28 | UNIQUE | 1 |  | KEEP: 1 unique patch(es); archive tag + owner review before any deletion |
| `claude/optimistic-euler-ph2z4f` | `fb5e79e8` | 2026-10-20 | UNIQUE | 77 |  | KEEP: 77 unique patch(es); archive tag + owner review before any deletion |
| `claude/tender-gates-bkl5jq` | `d78690ea` | 2026-10-28 | UNIQUE | 1 |  | KEEP: 1 unique patch(es); archive tag + owner review before any deletion |
| `codex/gsc-readonly` | `fa081f67` | 2026-09-11 | UNIQUE | 37 | #53 | KEEP: 37 unique patch(es); archive tag + owner review before any deletion |
| `codex/legality-gen4-bcc-reroll-sync-success-20261005` | `31fd10f9` | 2026-10-05 | UNIQUE | 2 | #135 | KEEP: 1 unique patch(es); archive tag + owner review before any deletion |
| `codex/legality-gen4-central-reporting-adapter-20261005` | `66322e07` | 2026-10-05 | UNIQUE | 3 | #128 | KEEP: 2 unique patch(es); archive tag + owner review before any deletion |
| `codex/legality-gen4-reroll-foundation-20261005` | `6b5a942c` | 2026-10-05 | UNIQUE | 3 |  | KEEP: 3 unique patch(es); archive tag + owner review before any deletion |
| `codex/legality-gen4-static-form-boundary-20261005` | `ccbe0bcc` | 2026-10-05 | UNIQUE | 1 | #109 | KEEP: 0 unique patch(es); archive tag + owner review before any deletion |
| `codex/rby-itemsfix-patch` | `3d51730a` | 2026-09-11 | UNIQUE | 13 |  | KEEP: 13 unique patch(es); archive tag + owner review before any deletion |
| `dev/gen2-pokemon-editor-20260912` | `4866de7f` | 2026-09-12 | UNIQUE | 53 |  | KEEP: earlier point of `ci/gen2-final-focused-20260912`; archive tag + owner review before any deletion |
| `dev/gen2-staged-editor-20260912` | `87a32b21` | 2026-09-12 | UNIQUE | 16 |  | KEEP: 16 unique patch(es); archive tag + owner review before any deletion |
| `dev/inventory-footer-readability-20260913` | `586433f6` | 2026-09-13 | UNIQUE | 3 |  | KEEP: 2 unique patch(es); archive tag + owner review before any deletion |
| `dev/inventory-hardware-polish-20260913` | `08fb25f7` | 2026-09-13 | UNIQUE | 11 |  | KEEP: earlier point of `ci/inventory-dirty-final-validation-20260913`; archive tag + owner review before any deletion |
| `dev/standalone-runtime-audit-20260912` | `f278c6d5` | 2026-09-12 | UNIQUE | 9 |  | KEEP: 9 unique patch(es); archive tag + owner review before any deletion |
| `dev/standalone-runtime-candidate-20260912` | `c8a08c1d` | 2026-09-12 | UNIQUE | 5 |  | KEEP: 5 unique patch(es); archive tag + owner review before any deletion |
| `docs/legality-checkpoint-palpark-egg-20261011` | `0f4d05a6` | 2026-10-11 | UNIQUE | 2 | #251 | KEEP: 2 unique patch(es); archive tag + owner review before any deletion |
| `docs/project-status-refresh-20260929` | `6927f06c` | 2026-09-29 | UNIQUE | 3 | #102 | KEEP: 3 unique patch(es); archive tag + owner review before any deletion |
| `feature/gen1-staged-editor-20260913` | `a45a1b63` | 2026-09-13 | UNIQUE | 1 |  | KEEP: 0 unique patch(es); archive tag + owner review before any deletion |
| `feature/gen4-complete-editor-20260928` | `4a3e2e19` | 2026-09-28 | UNIQUE | 31 | #94 | KEEP: 31 unique patch(es); archive tag + owner review before any deletion |
| `feature/gen5-readonly-foundation-20261008` | `79679950` | 2026-10-12 | UNIQUE | 85 | #163 | KEEP: 85 unique patch(es); archive tag + owner review before any deletion |
| `feature/legality-gen3-hoenn-safari-rng-20261007` | `cf60f1fa` | 2026-10-07 | UNIQUE | 4 | #155 | KEEP: 4 unique patch(es); archive tag + owner review before any deletion |
| `feature/legality-gen4-egg-iv-stage-20261011` | `dfa2fb53` | 2026-10-11 | UNIQUE | 2 | #273 | KEEP: 2 unique patch(es); archive tag + owner review before any deletion |
| `feature/legality-gen5-9-foundation-20261012` | `809a3184` | 2026-10-12 | UNIQUE | 24 | #301 | KEEP: 24 unique patch(es); archive tag + owner review before any deletion |
| `feature/legality-pk2-timecapsule-move-stage-20261011` | `6e4bd7ec` | 2026-10-11 | UNIQUE | 1 | #250 | KEEP: earlier point of `docs/legality-checkpoint-palpark-egg-20261011`; archive tag + owner review before any deletion |
| `feature/legality-rng-move-input-hardening-20261011` | `82d32ab6` | 2026-10-11 | UNIQUE | 16 | #270 | KEEP: 16 unique patch(es); archive tag + owner review before any deletion |
| `feature/legality-sourcefree-egg-move-guard-20261011` | `95416a8f` | 2026-10-11 | UNIQUE | 6 | #230 | KEEP: 6 unique patch(es); archive tag + owner review before any deletion |
| `feature/legality-sourcefree-pk12-move-compatibility-20261011` | `14baf42c` | 2026-10-11 | UNIQUE | 3 | #229 | KEEP: earlier point of `feature/legality-sourcefree-egg-move-guard-20261011`; archive tag + owner review before any deletion |
| `feature/legality-sourcefree-pk4-gen4-move-acquisition-20261011` | `a1eea80b` | 2026-10-11 | UNIQUE | 1 | #227 | KEEP: earlier point of `feature/legality-sourcefree-egg-move-guard-20261011`; archive tag + owner review before any deletion |
| `feature/legality-sourcefree-pk4-palpark-move-history-20261011` | `3df1de83` | 2026-10-11 | UNIQUE | 2 | #228 | KEEP: earlier point of `feature/legality-sourcefree-egg-move-guard-20261011`; archive tag + owner review before any deletion |
| `feature/touch-controls-20261005` | `36c0d1d6` | 2026-10-09 | UNIQUE | 370 | #122 | KEEP: 370 unique patch(es); archive tag + owner review before any deletion |
| `fix/gen14-action-tap-empty-parity-20261011` | `dcd855a7` | 2026-10-11 | UNIQUE | 9 | #280 | KEEP: 9 unique patch(es); archive tag + owner review before any deletion |
| `hardening/audit-bank-migration-20260928` | `342a149a` | 2026-09-28 | UNIQUE | 1 | #98 | KEEP: 1 unique patch(es); archive tag + owner review before any deletion |
| `hardening/audit-settings-durability-20260928` | `d928533c` | 2026-09-28 | UNIQUE | 1 | #99 | KEEP: 1 unique patch(es); archive tag + owner review before any deletion |
| `integration/gen2-staged-editor-20260912` | `d3da18fd` | 2026-09-12 | UNIQUE | 2 |  | KEEP: 2 unique patch(es); archive tag + owner review before any deletion |
| `polish/v1-ui-qol-20260928` | `c8c98d5d` | 2026-09-28 | UNIQUE | 13 | #88 | KEEP: 11 unique patch(es); archive tag + owner review before any deletion |
| `research/gen6-9-codex-20261018` | `9b46c779` | 2026-10-18 | UNIQUE | 1 | #321 | KEEP: 1 unique patch(es); archive tag + owner review before any deletion |
| `staging/product-home-perf-audit-20261004` | `defa2c99` | 2026-10-04 | UNIQUE | 8 |  | KEEP: 8 unique patch(es); archive tag + owner review before any deletion |
| `wip/codex-oct24-repairs` | `11b44e09` | 2026-10-24 | UNIQUE | 1 |  | KEEP: 1 unique patch(es); archive tag + owner review before any deletion |
| `wip/coord-r5-safari-ball-20261024` | `31a75206` | 2026-10-24 | UNIQUE | 1 |  | KEEP: 1 unique patch(es); archive tag + owner review before any deletion |
| `wip/l-backup-picker-20261024` | `0fe9e5f7` | 2026-10-24 | UNIQUE | 1 |  | KEEP: earlier point of `wip/l-r5-backup-browser-20261024`; archive tag + owner review before any deletion |
| `wip/l-r5-backup-browser-20261024` | `8a0f9eb4` | 2026-10-24 | UNIQUE | 2 |  | KEEP: 2 unique patch(es); archive tag + owner review before any deletion |
| `wip/p-heldsearch-20261024` | `65bd8d65` | 2026-10-24 | UNIQUE | 1 |  | KEEP: 1 unique patch(es); archive tag + owner review before any deletion |
| `work/claude-gen15-legality-independent-20261030` | `6cc05036` | 2026-10-31 | UNIQUE | 2 | #327 | KEEP: 2 unique patch(es); archive tag + owner review before any deletion |
| `work/claude-gen69-legality-foundation-20261030` | `e33e1115` | 2026-10-31 | UNIQUE | 2 |  | KEEP: 2 unique patch(es); archive tag + owner review before any deletion |

</details>

## 6. NRO preservation

CI artifacts expire, after 90 days for candidate builds. Important NROs are copied, with their SHA-256 and build identity, into GitHub **pre-releases** by `.github/workflows/archive-candidate-nro.yml`. That workflow downloads the original artifact from its own run (no rebuild), verifies the SHA-256 and refuses to overwrite an existing release. Release tags are `nro/<sha8>` on the exact application commit. See `docs/hardware/NRO_ARCHIVE.md` for the list.

## 7. Project board

Issues and PRs are organised on the **PokeBank NX** GitHub Projects board (Backlog / To Do / In Progress / Review / Done, with a Resolution field separating Completed, Superseded and Deferred work). See `docs/PROJECT_BOARD.md`.

## 8. Rules applied

- No force-push, reset, rebase or deletion. The consolidation is a merge on a temporary branch, merged into `main` only after owner approval.
- The Round 10 NRO `3634d1e6`, its branch, artifact, audit, checklist and test reports are unchanged.
- No PR or issue is closed in this pass. Closures happen only after their successor is accepted (`main` for #92 / #307; the legality consolidation for the #314 lineage) and with the owner's agreement.
- Legality #314 / #327 / #328 stay isolated; nothing from them is in `main`.
