# Owner configuration checklist

These repository settings need the owner's own GitHub account. Development sessions cannot change them: their
GitHub access cannot delete branches, push tags, create milestones, edit repository settings or configure
branch protection. Each item says where to make the change. Tick items off as you go.

## 1. Branch pruning

- [ ] Run `tools/maintenance/prune_branches.sh` in dry-run mode, then with `--execute`. The script, its checks and
      the exact commands are described in [`RECOVERY.md`](RECOVERY.md) and at the top of the script.
- [ ] Keep the bundle the script creates (`pokebank-nx-full-<date>.bundle` plus its manifest and SHA-256) outside
      the repository, for example on a second drive.

## 2. Branch protection

GitHub → Settings → Branches (or Rules → Rulesets) → add a rule.

**`main`**
- [ ] Require a pull request before merging, with at least one approval. You can approve your own PRs as the
      repository owner if you merge them yourself; keep the PR requirement anyway so every change has a CI
      record.
- [ ] Require status checks to pass, and require branches to be up to date before merging. Mark these checks
      required (they run on every PR into `main` today):
  - `Host tests (GCC)`
  - `Host tests + ASan/UBSan (Clang)`
  - `Export contracts (g++, plain + ASan/UBSan)`
  - `Export contracts (clang++, plain + ASan/UBSan)`
  - `Deterministic mutation runner (g++, plain + ASan/UBSan)`
  - `Deterministic mutation runner (clang++, plain + ASan/UBSan)`
  - `libFuzzer (clang -fsanitize=fuzzer,address,undefined)`
  - `unfiltered devkitA64 compile/link`
  - `Gen I-V integrated devkitA64 NRO + packaged RomFS verification`
  - `Native touch contracts`
  - `NRO audit tooling regression`
  - `Exact-head audit of the published candidate NRO`. This check only runs on PRs that bind a candidate. Make
    it required only if every PR into `main` carries a candidate binding; otherwise leave it optional.
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
`tools/maintenance/prune_branches.sh --with-metadata` creates the milestones and applies the table when `gh` is
logged in.

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

- [ ] Publish them from your account before then. `tools/maintenance/prune_branches.sh --archive-nro-fallback`
      does this with `gh`. It downloads each original artifact (`gh run download`), checks the SHA-256 against
      `docs/hardware/NRO_ARCHIVE.md`, creates the pre-release `nro/<sha8>` on the exact commit, re-downloads the
      asset and checks the hash again.

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
