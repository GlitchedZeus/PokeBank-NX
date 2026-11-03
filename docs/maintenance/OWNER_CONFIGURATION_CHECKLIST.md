# Owner configuration checklist

These repository settings need the owner's own GitHub account. Development sessions cannot change them: their
GitHub access cannot delete branches, push tags, create milestones, edit repository settings or configure
branch protection. Each item says where to make the change. Tick items off as you go.

## 1. Branch pruning

- [x] **Done by the owner on November 2, 2026:** 331 → 88 branches, with all 15 identified unique-history tips
      restored. Record: [`BRANCH_CLEANUP_STATE_2026-11-02.md`](BRANCH_CLEANUP_STATE_2026-11-02.md). The verified
      pre-prune bundle is in the `maintenance/pre-branch-prune-20261102-bbc9dd69` release.
- [ ] Do **not** run the earlier session's plan. It is archived and disabled in
      [`archive/superseded-2026-11-02-phase2-plan/`](archive/superseded-2026-11-02-phase2-plan/). Any future cleanup starts from a fresh live inventory, following
      `PROJECT_RULES.md` §1.

## 2. Branch protection

GitHub → Settings → Branches (or Rules → Rulesets) → add a rule.

**`main`**
- [ ] Require a pull request before merging. Set **required approvals to 0** for now. GitHub does not let a PR's
      author approve their own PR, so a mandatory approval would block the project's only maintainer. When a
      suitable independent reviewer joins, raise required approvals to 1 (and consider "Require review from Code
      Owners" with a `CODEOWNERS` file).
- [ ] Require status checks to pass before merging, and require branches to be up to date. Mark **only** these
      four checks required. Each one was verified on Nov 2, 2026: it runs on every pull request into `main` (no
      path or branch filter and no job-level condition in its workflow on `main`), and it reported `success` on
      the last normal PR into `main` (#330, head `67b1b6a0`).
  - `Host tests (GCC)` (`host-tests.yml`)
  - `Host tests + ASan/UBSan (Clang)` (`host-tests-clang.yml`)
  - `unfiltered devkitA64 compile/link` (`native-pr-build.yml`)
  - `Gen I-V integrated devkitA64 NRO + packaged RomFS verification` (`gen15-integrated-candidate.yml`)
- [ ] Leave every other check **optional**. They still run and show on the PR, but a required check that does not
      run blocks the merge forever:
  - Path-filtered (they run only when matching files change): `Export contracts (…)`,
    `NRO audit tooling regression`, `Deterministic mutation runner (…)`, `libFuzzer (…)`,
    `Native touch contracts`, `Exact head PK5 read-only and staged contracts`, `Packed move host regression`,
    `Packed multi-move and Release regression`, `Every workflow file parses`.
  - Conditional on a manual step: `Exact-head audit of the published candidate NRO`. It runs on every PR but
    passes only when the PR carries a freshly bound candidate (`docs/qa/CANDIDATE_UNDER_AUDIT.env`). An ordinary
    code PR fails it by design.
  - Superseded: `Gen I-IV combined devkitA64 NRO + packaged RomFS verification` (the Gen I–V build covers it; see
    `WORKFLOW_AUDIT_2026-11-02.md`).
- [ ] Re-check the list before saving: open the newest PR into `main` and confirm each required name appears there
      with exactly this spelling. If a workflow is renamed or retired, update this list first.
- [ ] Block force pushes.
- [ ] Block deletion.
- [ ] Optionally require linear history. Leave it off: this project merges with merge commits.

**`development/*`** (`development/gen6-plus`, `development/legality-engine`)
- [ ] Block force pushes and deletion. Status checks are optional: these lines are paused.

**Active repair branches** (`fix/round*-gen15-repairs`)
- [ ] Optionally block force pushes. Do not block deletion: they are retired after merging.

## 3. Milestones and labels

GitHub → Issues → Milestones → New milestone.
- [ ] `Gen I–V hardware acceptance`
- [ ] `v1.0 Vault`
- [ ] `Gen VI–IX parity`
- [ ] `Legality integration`

The issue-to-milestone assignment table is in [`MILESTONE_PLAN.md`](MILESTONE_PLAN.md).
Create the milestones by hand, or with `gh api` from your account, and assign them from the table.

- [ ] Labels: the 12 labels exist (bug, enhancement, hardware-qa, legality, vault, gen-i-v, gen-vi-ix,
      documentation, blocked, priority:p0, priority:p1, priority:p2). The new ones have GitHub's default grey colour
      and no description. Optionally set colours and descriptions under Issues → Labels.

## 4. Repository description and topics

GitHub → repository home → About (gear icon). Suggested text is in [`REPO_METADATA.md`](REPO_METADATA.md);
`--with-metadata` applies it when `gh` is logged in.

## 5. Round 1 and Round 2 NRO archives

The archive workflow (`.github/workflows/archive-candidate-nro.yml`) published Rounds 3, 5–10, 12 and 13. Rounds 1
(`7fea1797`) and 2 (`e10f307f`) failed twice with `HTTP 403: Resource not accessible by integration` when creating
the release. Their artifacts expire on **2027-01-14** and **2027-01-16**.

- [ ] Publish them from your account before then: download each original artifact (`gh run download <run> -n
      PokeBank-NX-Gen1-5-Candidate-<sha8>`), check its SHA-256 against the table below, create the pre-release
      `nro/<sha8>` on the exact commit (`gh release create … --prerelease --latest=false`), then download the asset
      again and re-check the hash. The disabled archive script in
      [`archive/superseded-2026-11-02-phase2-plan/`](archive/superseded-2026-11-02-phase2-plan/) shows the exact checks (read-only reference).

| Round | Commit | NRO SHA-256 | Run / artifact |
| --- | --- | --- | --- |
| 1 | `7fea17971906cae557a7207bfb7b0bed3548cea2` | `217f24becaaa4eb940abaef01962c80a2fb5536bc6af77934add92d900bac1d5` | 38946941097 / 12066747065 |
| 2 | `e10f307faf7254a47429babc49093e407ed65f1a` | `1dd518b756274cfe70d7a87b06a2e30f027389aeaddd9640f03ef3c5a2eb7ac0` | 39120355693 / 12133189538 |

## 6. Actions settings

GitHub → Settings → Actions → General.
- [ ] Workflow permissions: keep "Read repository contents" as the default. Workflows that publish (the archive
      workflow) request `contents: write` themselves.
- [ ] Artifact retention: candidate builds keep 90 days. Archive any candidate you test with the archive workflow
      while its artifact is still valid.
