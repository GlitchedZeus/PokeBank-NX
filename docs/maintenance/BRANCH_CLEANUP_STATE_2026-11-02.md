# PokeBank NX — GitHub branch cleanup state (November 2, 2026)

**Source of truth:** live GitHub branch API, GitHub Actions runs and restored exact tip SHAs. This status supersedes cleanup audit predictions; an audit recommendation is not itself an executed deletion.

## Verified outcome

| Item | Result |
|---|---|
| Remote branches before the cleanup | **331** |
| Remote branches after safe cleanup and restorations | **88** |
| Net reduction | **243 (73.4%)** |
| Previously unique-history branch tips restored | **15 of 15 identified** |
| Final strict fully-contained cleanup | **14 branch deletions, GitHub Actions SUCCESS** |
| Open PRs still retained | **22** as checked after completion |
| Fully verified historical recovery bundle | [Pre-prune release](https://github.com/GlitchedZeus/PokeBank-NX/releases/tag/maintenance/pre-branch-prune-20261102-bbc9dd69) — 207,503,872-byte Git bundle + SHA-256 + reference manifest + earlier deletion logs |
| Last safe cleanup execution | [GitHub Actions run 41533237105](https://github.com/GlitchedZeus/PokeBank-NX/actions/runs/41533237105) — SUCCESS |
| One-time cleanup workflow | **Removed from main after success**; cannot silently run again from that workflow |
| Release/feature integration | **NOT DONE** by this cleanup; no new Gen I–IX integrated NRO or legality integration is claimed |

The initial run [41528310646](https://github.com/GlitchedZeus/PokeBank-NX/actions/runs/41528310646) removed 224 contained branches, then failed for missing tag committer identity. A continuation [41531060587](https://github.com/GlitchedZeus/PokeBank-NX/actions/runs/41531060587) removed patch-equivalent and some unique-history branches before it was cancelled. **Every one of the 15 identifiable unique-history branches deleted by the continuation was then restored to its exact tip**. The historical deletion log in the release describes what happened *at deletion time*, so do not interpret every DELETE log row as a branch still absent.

## Unique-history branches restored exactly

- `audit/full-repository-line-by-line-20260928`
- `audit/pre-gen3-shared-editor-71`
- `ci/classic-inventory-corrected-validation-20260913`
- `ci/classic-inventory-correction-20260913`
- `ci/classic-inventory-final-validation-20260913`
- `ci/classic-inventory-production-final-20260913`
- `ci/classic-inventory-wiring-20260913`
- `ci/gen2-final-artifact-20260913`
- `ci/gen2-final-focused-20260912`
- `ci/gen2-final-native-20260912`
- `ci/gen2-final-prod-validate-20260913`
- `ci/gen2-prod-validate-20260913`
- `ci/inventory-dirty-final-validation-20260913`
- `ci/inventory-dirty-validation-20260913`
- `ci/inventory-footer-readability-validation-20260913`

These branches are **retained pending review/integration**. Some may only contain historical CI workflows; that does not authorize silently deleting them. Do not call code integrated merely because a tag preserves the commit.

## Final verified full-ancestry deletion batch

The final job explicitly checked each branch's recorded SHA against the live remote, verified `git merge-base --is-ancestor` against the named surviving ref, and re-read open PR heads/bases before deleting. It removed only these 14 fully-contained histories:

| Deleted branch | Full-history retained under |
|---|---|
| `audit/app-qa-20261014` | `main` |
| `claude/gen4-capture-ball` | `claude/gen1-5-legality-campaign-w9mkom` |
| `claude/gen5-home-providers-20261015` | `main` |
| `claude/gen14-campaign-20261015` | `main` |
| `claude/gen14-final-20261014` | `main` |
| `claude/launch-return-20261015` | `main` |
| `claude/pensive-cerf-pueceq` | `claude/gen1-5-legality-campaign-w9mkom` |
| `claude/pickers-search-scrollbar-20261017` | `main` |
| `claude/shared-editor-gen35-20261016` | `main` |
| `audit/legality-integration-hardening-20261012` | `claude/gen1-5-legality-campaign-w9mkom` |
| `claude/legality-gen3-negai-boshi-lifetime` | `claude/gen1-5-legality-campaign-w9mkom` |
| `feature/legality-sourcefree-pk4-gen4-move-acquisition-20261011` | `feature/legality-sourcefree-egg-move-guard-20261011` |
| `feature/legality-sourcefree-pk4-palpark-move-history-20261011` | `feature/legality-sourcefree-egg-move-guard-20261011` |
| `feature/legality-sourcefree-pk12-move-compatibility-20261011` | `feature/legality-sourcefree-egg-move-guard-20261011` |

## Mandatory next cleanup rule

See [PROJECT_RULES](../PROJECT_RULES.md). **No unique, unused or not-yet-integrated branch may be deleted because it is old, appears superseded, or has an archive tag.** An archive is recoverability, NOT integration. A branch is safe to prune only after its whole history is contained in a surviving branch (checked live), its tip is unchanged, and no active PR needs it. Otherwise retain it until the owner accepts a verified integration or explicitly approves its retirement.

The remaining 88 branches are *not* the final planned topology. The eventual long-lived target is `main`, `development/app-gen1-9` and `development/legality`, **after** controlled integration of currently open application, modern-generation, Vault, QA and legality work. Do not bulk-merge code onto `main` simply to reduce the branch count. The owner-reported P0 save/profile failures on `d160aa42` remain blockers.

## Recovery

All pre-cleanup remote tips are in the Git bundle and ref manifest at the recovery release. Archived unique tips also have `archive/branches/<original-branch>` tags. Follow [RECOVERY.md](RECOVERY.md) to restore by exact SHA or from an archive tag. **Do not treat a deleted ref as proof the feature is safely integrated** without independently verifying full ancestry.
