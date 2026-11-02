# Recovering deleted branches and archived history

This page explains how to restore anything that `tools/qa/maintenance/prune_branches.sh` (repository cleanup, Phase 2) removed from GitHub. Nothing that script deletes is lost. Every deleted branch tip stays available through at least two of these:

1. **The full backup bundle** made at the start of every run, which includes every branch and tag on the remote.
2. **An archive tag** `archive/branches/<branch name>` at the exact tip, for the groups that get tags: `equivalent-archive` and `unique-archive`.
3. **A preserved ref that contains the tip.** These are the no-tag groups: `contained`, `paused-contained`, `consolidation` and `legality-base`. The log names the ref and the SHA it had when the containment was checked.
4. **The deletion log**, [BRANCH_DELETION_LOG.md](BRANCH_DELETION_LOG.md) and the matching `.tsv`. It records each branch, its tip SHA, what preserves it, the tag, the batch and the UTC time.

You need write access to the repository for every `git push` below.

## Restore one branch from its SHA (works for every group)

Look up the tip SHA in `BRANCH_DELETION_LOG.md`. The script also prints it at the end of every run. Then:

```bash
git fetch origin                                  # make sure you have the objects
git cat-file -e <sha>^{commit} && echo present    # the object is still in your clone?
git push origin <sha>:refs/heads/<branch name>
```

Commits that a preserved branch or tag contains are always on GitHub. A commit that is only in a deleted, untagged branch is not. In that case, use the bundle (below).

## Restore from an archive tag

```bash
git fetch origin 'refs/tags/archive/branches/*:refs/tags/archive/branches/*'
git rev-parse 'archive/branches/<branch name>^{commit}'      # must equal the logged tip
git push origin 'refs/tags/archive/branches/<branch name>^{commit}:refs/heads/<branch name>'
```

Each tag is annotated. Its message holds the branch name, the tip and this restore command.

## Restore from the backup bundle

The script writes `pokebank-nx-full-<UTC date>.bundle` to the directory you chose (by default, the parent of your clone). Next to it are `<bundle>.sha256` and `<bundle>.manifest.tsv`, which list each branch, its tip, its group and what preserves it. **Keep the bundle outside the repository, permanently, and keep a second copy somewhere else.** It is the only copy of anything that has neither a tag nor a containing ref.

```bash
sha256sum -c pokebank-nx-full-YYYYMMDD.bundle.sha256
git bundle verify pokebank-nx-full-YYYYMMDD.bundle
git bundle list-heads pokebank-nx-full-YYYYMMDD.bundle | grep '<branch name>'

# from inside your clone: fetch one branch out of the bundle, then push it back
git fetch /path/to/pokebank-nx-full-YYYYMMDD.bundle 'refs/heads/<branch name>:refs/heads/restore/<branch name>'
git push origin 'refs/heads/restore/<branch name>:refs/heads/<branch name>'

# or rebuild a complete copy of the remote as it was before the run
git clone --mirror /path/to/pokebank-nx-full-YYYYMMDD.bundle pokebank-nx-restore.git
```

The bundle is made from `git clone --mirror` of GitHub. It therefore also contains GitHub's read-only `refs/pull/*` refs. Do not push those back.

## Restore a PR head from its archive tag

`--tag-pr-heads` checks that each live PR head still starts with the approved prefix: `36c0d1d6` for #122, `79679950` for #163, `dcd855a7` for #280. It then tags the head, pushes the tag and verifies `tag^{}` against `git ls-remote`. The run is logged in `BRANCH_DELETION_LOG.md` (group `pr-head-tag`). To bring a head back as a branch:

```bash
git fetch origin refs/tags/archive/pr-122:refs/tags/archive/pr-122
git push origin 'refs/tags/archive/pr-122^{commit}:refs/heads/feature/touch-controls-20261005'
```

## Fetching tags that already exist

| Tags | What they preserve | Fetch |
|---|---|---|
| `archive/branches/*` | Branch tips archived by `prune_branches.sh` (about 54 after a full run) | `git fetch origin 'refs/tags/archive/branches/*:refs/tags/archive/branches/*'` |
| `archive/pr-122`, `archive/pr-163`, `archive/pr-280` | Heads of open PRs #122, #163 and #280, made by `prune_branches.sh --tag-pr-heads` before the owner closes them. The PRs and their branches are kept. | `git fetch origin 'refs/tags/archive/pr-*:refs/tags/archive/pr-*'` |
| `archive/local-reflog-20261102/*` (61) | Commits recovered from local reflogs on 2026-11-02. These are lightweight tags. Each name is the 12-hex prefix of the commit it points to. | `git fetch origin 'refs/tags/archive/local-reflog-20261102/*:refs/tags/archive/local-reflog-20261102/*'` |
| `nro/*` (9) | Source commits of the archived candidate NRO pre-releases ([NRO_ARCHIVE.md](../hardware/NRO_ARCHIVE.md)) | `git fetch origin 'refs/tags/nro/*:refs/tags/nro/*'` |
| `evidence/*` (1) | Evidence draft pre-release `evidence/gen1-5-2144a682-20261102` | `git fetch origin 'refs/tags/evidence/*:refs/tags/evidence/*'` |

`git ls-remote --tags origin` lists every tag. The REST `GET /repos/.../tags` listing does not show the `archive/local-reflog-20261102/*` tags (see REPO_CLEANUP_AUDIT_2026-11-02.md, section 1). To turn a reflog-recovery commit back into a branch:

```bash
git push origin 'refs/tags/archive/local-reflog-20261102/<12hex>^{commit}:refs/heads/recovered/<12hex>'
```

## Notes

- The no-tag groups depend on the containing branch staying alive. That branch is usually `main`, `fix/round11-gen15-repairs`, `development/gen6-plus`, `audit/gen1-4-legality-research-20261011` or `audit/legality-integration-hardening-20261012`. Before you delete one of those containers later, re-run `tools/qa/maintenance/branch_audit.py`. Its contents then need their own tag or containment.
- GitHub never re-opens a pull request that was closed because its branch was deleted. Restoring the branch lets you re-open it by hand.
- Restoring a branch never needs a force push. If the name already exists with other content, restore it under a different name, such as `restore/<name>`.

## Candidate NRO archives (`nro/*` pre-releases)

The `nro/*` pre-releases hold the exact candidate NRO files. The table and SHA-256 values are in `docs/hardware/NRO_ARCHIVE.md`.

Round 1 (`nro/7fea1797`) and Round 2 (`nro/e10f307f`) could not be published by the archive workflow: release creation returned `HTTP 403: Resource not accessible by integration`. To publish them from the owner's account:

1. Run `tools/qa/maintenance/prune_branches.sh --archive-nro-fallback`. This needs `gh` to be logged in. It is a dry run: it downloads and verifies but publishes nothing.
2. Run the same command with `--execute` to publish.

Before publishing, the script:

- checks that the size, SHA-256, run and artifact in `NRO_ARCHIVE.md` match its own copy of those values;
- refuses if the tag or release already exists;
- downloads the original artifact and checks its bytes.

After publishing, it re-downloads the asset and checks its SHA-256 again. The artifacts expire on 2027-01-14 (Round 1) and 2027-01-16 (Round 2).
