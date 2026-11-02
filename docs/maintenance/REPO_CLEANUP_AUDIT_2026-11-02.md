# Repository cleanup audit — November 2, 2026 (Phase 1)

**Status: AUDIT + PROPOSAL ONLY.** Phase 1 changed nothing on GitHub. No branch was deleted or renamed, no tag or release was created, and no PR or issue was closed, edited or commented on. The only write is the commit that adds these documents. Every proposal below is **PENDING OWNER APPROVAL**.

| Document | Content |
|---|---|
| [BRANCH_ARCHIVE_MANIFEST.md](BRANCH_ARCHIVE_MANIFEST.md) | One row per remote branch (tip SHA, last activity, PRs, classification, unique work, action, preservation location, Phase 2 status), plus tags/releases, the NRO archive and all 252 PRs |
| [branch_audit_2026-11-02.json](branch_audit_2026-11-02.json) | The same data in machine-readable form |
| `tools/qa/maintenance/branch_audit.py` | Read-only generator; re-run it with `--compare` before Phase 2 to detect drift |
| `tools/qa/maintenance/render_branch_manifest.py` | Renders the manifest from the JSON |

This audit builds on [GITHUB_CLEANUP_20261015.md](../archive/sessions/GITHUB_CLEANUP_20261015.md), [THREE_BRANCH_CONSOLIDATION_PLAN.md](THREE_BRANCH_CONSOLIDATION_PLAN.md) and the October 31 ledger `docs/MAIN_CONSOLIDATION_20261031.md` on `main`. That ledger has 316 refs; there are 8 more (net) since then, including `audit/gen12-independent-20261013`, which appeared during this audit. NRO preservation follows `docs/hardware/NRO_ARCHIVE.md` and `.github/workflows/archive-candidate-nro.yml`.

## 1. Snapshot

| Item | Value |
|---|---|
| Snapshot time | 2026-11-02, after a plain `git fetch origin` (no prune) and a fetch of every remote tag |
| `origin/main` | `9ce7f737` (consolidation PR #330 merged October 31) |
| Remote branches | **324** (318 distinct tips) |
| Local-only `r13/*` branches | 6 (`r13/breeding`, `encdata`, `guards`, `levelexp`, `recheck`, `touchdrag`). These are **not on the remote**. They are protected by rule and must be re-checked once pushed |
| Pull requests | **252** total: **34 open** (33 draft), **218 closed**, of which **109 merged** and **109 closed unmerged** |
| Issues (excluding PRs) | 81 total, **35 open** |
| Remote tags | **69**: 7 `nro/*`, 1 `evidence/*`, 61 `archive/local-reflog-20261102/*` |
| Releases | **8**, all pre-releases: 7 `nro/*` candidate archives and 1 draft `evidence/gen1-5-2144a682-20261102` |

`GET /repos/.../tags` returned only 8 tags, while `git ls-remote --tags` shows 69. The 61 `archive/local-reflog-20261102/*` tags are on the remote but did not appear in that listing. The script reads `git ls-remote`, which shows every tag.

## 2. Method

For each remote branch, the script records the tip SHA, the last commit date and author, ahead/behind `origin/main`, and the PRs whose head or base is the branch. It then runs these tests:

1. **Retained set:** every KEEP / KEEP-PAUSED branch tip plus every tag that exists on the remote. Local-only tags do not count.
2. **Unique commits:** `git rev-list <tip> --not <retained set>`. A retained branch is compared against the *other* retained refs.
3. **Patch equivalence:** `git patch-id --stable` for each unique non-merge commit, checked against all retained history. **Tree equivalence:** the tip's tree is identical to the tree of a retained commit.
4. **Evidence check:** every hardware/QA evidence blob at the tip must exist in retained history (`docs/hardware/**`, `docs/qa/**`, `docs/qa-archives/**`, `docs/handoffs/**`, `RETEST_*`, `ROUND*_RESULTS*`, owner intake, screenshots, `*DEVICE_TEST*` / `*HARDWARE*` documents). If any blob is missing there, the branch cannot be DELETE-SAFE.
5. **Protection rules:** a branch is retained if any of these apply:
   - it is a named active branch;
   - it matches `r13/*`;
   - it is the head **or base** of an open PR;
   - it is named in the candidate/NRO history;
   - its tip is a candidate commit;
   - it is the only holder of a candidate commit.

   A branch whose name matches legality, `pkhex-parity`, `gen[6-9]-native`, `gen6-9`, `gen69`, `gen5-9`, `gen1-9` or `gen6-plus` is **PAUSED**. A paused branch is retained (KEEP-PAUSED) or archive-tagged, and is never plain-deleted.
6. **Recent work:** a non-protected branch with unique commits from the last 7 days is kept for owner review rather than archived.

## 3. Branch counts

### By recommended action

| Action | Branches | Meaning |
|---|---:|---|
| KEEP | **23** | Active lanes, open-PR heads and bases, candidate-history holders, recent unique work |
| KEEP-PAUSED | **23** | Paused Gen VI–IX / legality branches used by an open PR |
| DELETE-SAFE | **70** | Fully contained (64) or patch- and tree-equivalent (6). No open PR, and no evidence missing from retained history |
| ARCHIVE-TAG-THEN-DELETE | **208** | Annotated `archive/<branch>` tag at the exact tip first, then delete |
| **Total** | **324** | |

### By classification

| Classification | Branches | Action split |
|---|---:|---|
| ACTIVE | 13 | 13 KEEP |
| PAUSED (Gen VI–IX / legality) | 192 | 23 KEEP-PAUSED; 169 ARCHIVE-TAG-THEN-DELETE (160 fully contained in #314 / #299 / #252 / #230 lineage, 8 with unique commits, 1 patch-equivalent) |
| MERGED | 71 | 64 DELETE-SAFE; 7 KEEP (still the head or base of an open PR) |
| SUPERSEDED | 6 | 6 DELETE-SAFE |
| DUPLICATE | 0 as a separate class | 11 branches share tips in 5 groups. All are fully contained, so they are counted as MERGED / PAUSED |
| ABANDONED-UNIQUE | 42 | 39 ARCHIVE-TAG-THEN-DELETE; 3 KEEP (open PRs #122, #163, #280) |

DELETE-SAFE by prefix: `feature` 18, `wip` 17, `claude` 7, `dev` 5, `fix` 4, `integration` 3, `audit` 2, `codex` 2, `docs` 2, `ui` 2, and one each of `checkpoint`, `ci`, `r11`, `recovery`, `scratch`, `staging`, `tmp-do-not-use`, `tmp-rby-adapter`.

Non-paused ARCHIVE-TAG-THEN-DELETE by prefix (39): `ci` 19 (September 12–13 CI validation lanes, mostly workflow-only commits), `wip` 5, `dev` 4, `audit` 2, `codex` 2, `hardening` 2, and one each of `docs`, `feature`, `integration`, `polish`, `staging`.

**No branch with an open PR, as head or as base, is a deletion candidate.** The hardware/QA evidence check found **zero** DELETE-SAFE branches with evidence missing from retained history. One archive candidate holds evidence that exists nowhere else: `wip/codex-oct24-repairs` (`docs/hardware/OCT24_REPAIRS_AND_RETEST.md`, `docs/handoffs/MAIN_ACTIVE_HANDOFF.md`). It is ARCHIVE-TAG-THEN-DELETE, and its tag preserves both files.

## 4. Proposed active branch set

### Target set after Phase 2 and the PR consolidations below (9 branches plus `r13/*`)

| Branch | Role |
|---|---|
| `main` | Gen I–V application and owner-accepted history |
| `development/gen6-plus` | Gen VI+ application line (paused behind Gen I–V acceptance; base of #331) |
| `fix/round11-gen15-repairs` | PR #332, Round 11 repairs, Round 12 candidate `066e1db9` |
| `fix/round12-gen15-repairs` | PR #333, Round 13 candidate `9c301c48`, under owner hardware test |
| `claude/optimistic-euler-ph2z4f` | Active session branch (this audit) |
| `claude/gen1-5-legality-campaign-w9mkom` | Legality PR #314 (head `2144a682`; evidence pre-release `evidence/gen1-5-2144a682-20261102`) |
| `work/claude-gen15-legality-independent-20261030` | Legality PR #327 |
| `work/claude-gen69-legality-foundation-20261030` | Legality PR #328 |
| `feature/unified-link-emulator-gen1-9-20261101` | PR #331 into `development/gen6-plus` |
| `r13/*` | Round 13 lanes, once pushed |

### Retained only until the owner decides on a PR or lane

- **Paused PRs with unique work:**

  | PR | Branch |
  |---|---|
  | #301 | `feature/legality-gen5-9-foundation-20261012` |
  | #273 | `feature/legality-gen4-egg-iv-stage-20261011` |
  | #252 | `audit/gen1-4-legality-research-20261011` |
  | #144 | `audit/gen1-4-pkhex-parity-ledger-20261006` |
  | #230 | stack top, plus its stacked bases for #227–#229 |
  | #309 / #311 / #312 | Gen VI–VIII stack: 14 commits not patch-equivalent to `development/gen6-plus`, so the stack is not superseded by the import |
  | #321 | Gen VI–IX research |

- **Recent unique work:**
  - `archive/qa-campaigns-2026-10-28`: QA archive zip and Round 11 checklists.
  - `audit/gen12-independent-20261013`: created today; 1 commit with Gen I/II audit docs and tools.
  - `claude/tender-gates-bkl5jq`: 1 commit.
  - `feature/ds-melonds-link-emulator-20261101`: 6 commits not patch-equivalent; probably a precursor of #331.
- **Base branches that only an open PR needs:**

  | Branch | Needed by |
  |---|---|
  | `integration/legality-gen14-major-20261012` | base of #314, #308, #305, #303, #301, #299 |
  | `feature/legality-engine-gen1-4-20260929` | base of #298 and the lane PRs |
  | `feature/gen4-full-editor-20260928` | base of #163, #122, #103 |
  | `integration/gen4-polish-hardware-20260928` | base of #92 |
  | `claude/gen14-final-20261014` | base of #307 |
  | `fix/gen14-narrow-native-actions-20261011` | base of #280 |

  To free one of these, the owner must first close or retarget every PR that uses it. For example, #314 can be retargeted to `main` or to a new `development/legality-engine` branch.

## 5. Pull requests

All 252 are listed in the manifest. **Open (34):** #333, #332, #331, #328, #327, #321, #314, #312, #311, #309, #308, #307, #305, #303, #301, #299, #298, #296, #295, #294, #293, #292, #280, #273, #252, #230, #229, #228, #227, #163, #144, #122, #103, #92.

### Open PRs proposed as obsolete (propose only; each closure comment should record the old head SHA and the successor)

| PR | Head @ SHA | Evidence | Proposed successor | When |
|---|---|---|---|---|
| [#307](https://github.com/GlitchedZeus/PokeBank-NX/pull/307) | `claude/gen5-integration-20261014` @ `d6cc878a` | Head is an ancestor of `main` | `main` via merged [#330](https://github.com/GlitchedZeus/PokeBank-NX/pull/330) | Now |
| [#92](https://github.com/GlitchedZeus/PokeBank-NX/pull/92) | `feature/gen4-full-editor-20260928` @ `ddaff247` | Head is an ancestor of `main` | `main` via #330 | Now. The branch stays until #163, #122 and #103 are closed or retargeted |
| [#163](https://github.com/GlitchedZeus/PokeBank-NX/pull/163) | `feature/gen5-readonly-foundation-20261008` @ `79679950` | 85 unique commits from the early read-only Gen V lane; Gen V was rebuilt on the shared editor | `main` (Gen V shared editor from #307/#330) | After the `archive/feature/gen5-readonly-foundation-20261008` tag |
| [#122](https://github.com/GlitchedZeus/PokeBank-NX/pull/122) | `feature/touch-controls-20261005` @ `36c0d1d6` | 391 unique commits (370 not patch-equivalent), last activity October 9; touch parity landed through merged #208 / #267 / #271 | `main` (touch parity) | **Owner decision.** Archive tag first |
| [#303](https://github.com/GlitchedZeus/PokeBank-NX/pull/303), [#305](https://github.com/GlitchedZeus/PokeBank-NX/pull/305), [#308](https://github.com/GlitchedZeus/PokeBank-NX/pull/308) | `bc9d8c20`, `b8eb006e`, `2761a75d` | Heads contained in #314 `2144a682` | [#314](https://github.com/GlitchedZeus/PokeBank-NX/pull/314) | After the owner accepts the legality consolidation, as in the October 31 ledger |
| [#292](https://github.com/GlitchedZeus/PokeBank-NX/pull/292)–[#296](https://github.com/GlitchedZeus/PokeBank-NX/pull/296), [#298](https://github.com/GlitchedZeus/PokeBank-NX/pull/298), [#299](https://github.com/GlitchedZeus/PokeBank-NX/pull/299), [#103](https://github.com/GlitchedZeus/PokeBank-NX/pull/103) | L1–L5 lanes, legality integration, hardening runner, engine foundation | Heads contained in #314 | #314 | Same as above. Branches become ARCHIVE-TAG-THEN-DELETE (paused, never plain-deleted) |
| [#227](https://github.com/GlitchedZeus/PokeBank-NX/pull/227), [#228](https://github.com/GlitchedZeus/PokeBank-NX/pull/228), [#229](https://github.com/GlitchedZeus/PokeBank-NX/pull/229) | Stacked source-free legality PRs | Heads contained in #230 `95416a8f` | [#230](https://github.com/GlitchedZeus/PokeBank-NX/pull/230) (stack top; 3 unique commits) | Optional, low priority. #230 must first be retargeted off the #229 branch |

The count is 18 PRs. Of these, #307, #92 and #163 have clear successors on `main` today; #122 needs an owner decision; and 14 are legality PRs, deferred until the legality consolidation is accepted.

**Not proposed as obsolete:**
- Active: #333, #332, #331, #328, #327, #314.
- Unique paused work: #301, #273, #252, #230, #144, #321, and the #309 → #311 → #312 stack.
- **#280** has 9 unique commits (Gen III–IV empty-slot touch/A parity). The owner should decide whether to port or retire it. If it is retired, it is superseded by merged #271 after an archive tag.

**Retargets to consider:**
- #321 → `development/gen6-plus` (as the October 31 ledger proposed).
- #314 → `main` or `development/legality-engine` when the legality line is accepted. This frees `integration/legality-gen14-major-20261012`.

## 6. Issues

There are 35 open issues: #5, #9, #15, #16, #17, #21, #26, #29, #30, #31, #49, #50, #55, #56, #57, #58, #59, #69, #70, #89, #95, #285–#290, #313, #320, #322–#326 and #329. **No issue is proposed for closure without an explicit successor.** Each unresolved requirement stays mapped to a surviving issue or document.

### Proposed duplicates and consolidations (propose only)

| Issue | Proposal | Successor |
|---|---|---|
| [#285](https://github.com/GlitchedZeus/PokeBank-NX/issues/285) Gen I–IV legality master inventory | Duplicate scope of the newer Gen I–IX master tracker | [#329](https://github.com/GlitchedZeus/PokeBank-NX/issues/329) (copy any unchecked items first) |
| [#286](https://github.com/GlitchedZeus/PokeBank-NX/issues/286)–[#290](https://github.com/GlitchedZeus/PokeBank-NX/issues/290) L1–L5 lane trackers | Lane PRs #293 / #295 / #296 / #292 / #294 are contained in #314 | #329, once the legality consolidation is accepted. Move any open checklist lines into #329 |
| [#95](https://github.com/GlitchedZeus/PokeBank-NX/issues/95) G4-04 Gen IV Create + fields | Implemented: the #92 head is in `main` | Close as completed after owner confirmation. Remaining hardware checks are tracked in #313 |
| [#31](https://github.com/GlitchedZeus/PokeBank-NX/issues/31) Read-only DS Gen IV/V adapters | Overlaps [#56](https://github.com/GlitchedZeus/PokeBank-NX/issues/56) (universal emulator save discovery) and [#30](https://github.com/GlitchedZeus/PokeBank-NX/issues/30); DraStic/melonDS Gen IV/V sources exist | Fold into #56 after the owner confirms the adapters |
| [#15](https://github.com/GlitchedZeus/PokeBank-NX/issues/15) Automate preservation of device-test NROs | Implemented by `archive-candidate-nro.yml` + `docs/hardware/NRO_ARCHIVE.md`, but archives are still pending (see section 7) | Keep open until those archives are published, then close as completed |
| [#21](https://github.com/GlitchedZeus/PokeBank-NX/issues/21) / [#70](https://github.com/GlitchedZeus/PokeBank-NX/issues/70) | Two overlapping polish/QoL backlogs | Optional: merge #21's open items into #70 (owner decision) |
| [#69](https://github.com/GlitchedZeus/PokeBank-NX/issues/69) Full project audit | Mostly done (remediation in merged #101); one item deferred | Move the deferred item to a surviving issue, then close (owner decision) |

### Retained as-is

- **v1.0 requirements:** #29 roadmap, #55, #56, #57, #58, #59, #70.
- **Gen I–V acceptance gate:** #313.
- **Owner hardware bugs:** #320, #322, #323, #324, #325, #326. Close each only after the owner verifies it on the current candidate.
- **Save sessions / Inject:** #89.
- **Backlog and research:** #5, #9, #16, #17, #26, #30, #49, #50.
- **Deferred ideas** closed on October 15 stay indexed in `docs/roadmap/FUTURE_AFTER_GEN15_BACKLOG.md`.

## 7. Tags, releases and the NRO archive

**All 69 remote tags and 8 releases are KEEP; none is proposed for deletion.**

| Tag | Status |
|---|---|
| `nro/1ca2faec`, `nro/26186464`, `nro/3634d1e6`, `nro/62a2f8c3`, `nro/6ec5ccb9`, `nro/d6a20a2a`, `nro/e88957f2` | Candidate NRO pre-releases. All seven commits are in `main` |
| `evidence/gen1-5-2144a682-20261102` | Draft pre-release on the #314 head |
| 61 × `archive/local-reflog-20261102/*` | Recovered local-reflog commits |

Candidate commits named in `CANDIDATE_UNDER_AUDIT.env` history that **have no `nro/*` pre-release yet**:

| Commit | Round | Kept reachable by |
|---|---|---|
| `7fea1797` | Round 1 | `main` |
| `e10f307f` | Round 2 | `main` |
| `8804f8f8`, `a0633b6d`, `c60d4da0` | Earlier candidate identities | `main` |
| `066e1db9` | Round 12 | **only** `fix/round11-gen15-repairs` / `fix/round12-gen15-repairs` |
| `9c301c48` | Round 13 | **only** `fix/round12-gen15-repairs` |

Their Actions artifacts expire (the earliest listed expiry is 2027-01-14). Recommendation: run *Archive candidate NRO* for Rounds 1, 2, 11/12 and 13 before any Phase 2 deletion, then update `NRO_ARCHIVE.md`.

Phase 2 would add **208 annotated `archive/<old-branch-name>` tags**, one per ARCHIVE-TAG-THEN-DELETE row. A tag is a git ref only, with no release attached.

## 8. Risks

1. **Drift.** Branches are moving: #327, #328, #331, #333 and `development/gen6-plus` all changed in the last 48 hours, and a new branch appeared during this audit. Phase 2 must re-run the script with `--compare`. Any drift requires a regenerated manifest and re-approval for the changed rows.
2. **Deleting a base branch closes its PRs.** GitHub closes every open PR whose base branch is deleted. The script treats bases as protected, and Phase 2 must re-query open PRs immediately before each deletion batch.
3. **Patch equivalence is not identity.** The 6 SUPERSEDED rows also have tip trees identical to retained commits. Owners who prefer zero judgement can move them to ARCHIVE-TAG-THEN-DELETE at no cost.
4. **Tag volume.** 208 new tags is many, but tags are cheap and reversible. An alternative for the 160 fully-contained legality branches is one summary tag per lineage. That needs explicit owner approval, because the requirement says paused branches are never plain-deleted.
5. **Broken links.** Docs that link to `blob/<branch>/...` break once the branch is deleted. Hardware-round documents on `main` mostly link to files on `main`, but historical handoffs may reference old branches.
6. **Local-only state.** The `r13/*` branches and the local tags `archive/legality-314-eabe41f3` and `archive/main-pre-consolidation-6fb44150` are not on the remote. Their commits are reachable from `#314` and `main` respectively, but the tag names are not published.
7. **Tooling.** The GitHub MCP tools available here cannot delete remote refs (also noted on October 15). Phase 2 needs `git push` with a credential that is allowed to push tags and delete branches.
8. **Unexpired candidate artifacts.** `066e1db9` and `9c301c48` are reachable only through the Round 11/12 fix branches, which are KEEP. They must not lose their branch before they are merged or archived.

## 9. Phase 2 procedure (after owner approval only)

Run from a clean worktree. **Never** use force-push, rebase, reset or `git fetch --prune`. Work in batches of at most 25 branches.

1. **Refresh and detect drift.**
   ```sh
   git fetch origin
   git fetch origin 'refs/tags/*:refs/tags/*'
   python3 tools/qa/maintenance/branch_audit.py --repo . --fetch-prs \
     --out /tmp/branch_audit_phase2.json \
     --compare docs/maintenance/branch_audit_2026-11-02.json
   ```
   Continue only if the drift is 0, or if the owner has re-approved every drifted row.
2. **Archive NROs first.** Run *Archive candidate NRO* for the pending candidates in section 7.
3. **Create annotated archive tags first.** For each approved ARCHIVE-TAG-THEN-DELETE row (`branch`, `tip`):
   ```sh
   test "$(git rev-parse refs/remotes/origin/$BRANCH)" = "$TIP" || { echo "DRIFT $BRANCH"; exit 1; }
   git tag -a "archive/$BRANCH" "$TIP" -m "Archive of branch $BRANCH at $TIP
   Related PR(s): <from manifest>
   Audit: docs/maintenance/BRANCH_ARCHIVE_MANIFEST.md (2026-11-02)"
   git push origin "refs/tags/archive/$BRANCH"
   ```
   Push each tag by its explicit name. Never use `--tags` or a wildcard push.
4. **Verify each tag.** `git ls-remote origin "refs/tags/archive/$BRANCH^{}"` must print exactly `$TIP`. Stop the batch on any mismatch.
5. **Re-check PRs.** Immediately before deleting, query open PRs (MCP `list_pull_requests state=open` or REST). Skip any branch that is now the head or base of an open PR.
6. **Delete only if unchanged.** This applies to verified-archive rows and to DELETE-SAFE rows:
   ```sh
   git push --force-with-lease="refs/heads/$BRANCH:$TIP" origin ":refs/heads/$BRANCH"
   ```
   The lease refuses the delete if the branch moved after the audit.
7. **Post-check.**
   - Every KEEP / KEEP-PAUSED branch still exists at a tip equal to, or a fast-forward of, its audited tip.
   - Every open PR still has its head and base branches and is still open.
   - Every `archive/*` tag resolves to its recorded tip.
   - Every `nro/*` and `evidence/*` tag and release is unchanged.
   - Re-run the script. Remaining branches should equal the KEEP + KEEP-PAUSED set plus any rows the owner deferred.
8. **Record.** Update the manifest's Phase 2 status column (DONE, with the tag name and date) in a docs-only commit.

PR closures and issue consolidations (sections 5 and 6) are separate owner actions. Close a PR first, then re-run the audit, so that its branch moves from KEEP to DELETE-SAFE or ARCHIVE-TAG-THEN-DELETE.

## 10. Items needing owner decision

1. Approve the **70 DELETE-SAFE** and **208 ARCHIVE-TAG-THEN-DELETE** rows, in whole or in part. Optionally move the 6 SUPERSEDED rows to archive-tag.
2. Choose the archival form for the **160 fully-contained paused legality branches**: one tag each (proposed), or KEEP-PAUSED.
3. **PR closures:**
   - #307 and #92 → `main`.
   - #163 → `main` (after its archive tag).
   - #122 → archive tag, then close or keep.
   - #280 → port or retire.
   - The 14 legality PRs → #314: when?
4. **Retargets:** #321 → `development/gen6-plus`; #314 → `main` or a new `development/legality-engine`. Retargeting #314 frees `integration/legality-gen14-major-20261012`.
5. `integration/main-gen15-consolidation`: merged via #330 and only *textually* named in `NRO_ARCHIVE.md`. Keep it, or reclassify it as DELETE-SAFE after updating that reference.
6. **Recent unique branches** (kept for now):
   - `archive/qa-campaigns-2026-10-28`: convert to tag `archive/qa-campaigns-2026-10-28` and delete the branch?
   - `audit/gen12-independent-20261013`
   - `claude/tender-gates-bkl5jq`
   - `feature/ds-melonds-link-emulator-20261101`: superseded by #331?
7. **Issue consolidations** in section 6: #285 and #286–#290 → #329; #95 completed; #31 → #56; #21 → #70; #69 closure.
8. Push the `r13/*` lanes, or confirm that they stay local. Publish the local tags `archive/legality-314-eabe41f3` and `archive/main-pre-consolidation-6fb44150` if the names matter.
9. Run *Archive candidate NRO* for `7fea1797`, `e10f307f`, `066e1db9` and `9c301c48` (and the older `8804f8f8` / `a0633b6d` / `c60d4da0` identities if their artifacts still exist).
