# GitHub Housekeeping — October 15, 2026

**Requested by owner: finish Gen V; audit/accept all Gen I–V; reduce branch/PR/issue clutter without losing any work.** This is a historical bookkeeping record, not a promise that later branches have stopped moving.

## Result so far

| Metric | Before | After | Notes |
|---|---:|---:|---|
| Open pull requests | 32 | **28** | Four fully incorporated PRs closed as **superseded**, not merged |
| Open issues | 45 | **28** | 18 deferred future-feature issues closed with retained discussions; one new Gen I–V acceptance gate #313 opened |
| Branches | 275 | **275** | **No Git refs were deleted**: connected GitHub actions available to this session cannot delete remote branches. A safe pruning mechanism and contributor approval are required. |
| Distinct branch tip SHAs | 269 | 269 | Only **6 duplicate refs** at exactly identical SHAs; most branch heads are unique and cannot be discarded blindly |
| Historical `ci/` branches | 20 | 20 | High-priority review candidates, but first prove ancestry/contents and archive reproducible artifacts |

### Closed *superseded tracking* PRs

- [#278](https://github.com/GlitchedZeus/PokeBank-NX/pull/278) — Gen I–IV narrow Actions
- [#300](https://github.com/GlitchedZeus/PokeBank-NX/pull/300) — original Gen V integration
- [#304](https://github.com/GlitchedZeus/PokeBank-NX/pull/304) — Gen V profile/source hardening
- [#306](https://github.com/GlitchedZeus/PokeBank-NX/pull/306) — Gen I–IV finalization

All four exact PR heads were independently checked with GitHub commit comparison: each was an **ancestor** of active [Gen I–V application PR #307](https://github.com/GlitchedZeus/PokeBank-NX/pull/307), with `behind_by=0`. Closure comments preserve each old head SHA and the successor. **Branches were retained.** Nothing was merged into default `main`.

### Archived **future**, not completed or abandoned issues

18 deferred original issues: [#3](https://github.com/GlitchedZeus/PokeBank-NX/issues/3), [#7](https://github.com/GlitchedZeus/PokeBank-NX/issues/7), [#27](https://github.com/GlitchedZeus/PokeBank-NX/issues/27), [#10](https://github.com/GlitchedZeus/PokeBank-NX/issues/10), [#20](https://github.com/GlitchedZeus/PokeBank-NX/issues/20), [#40](https://github.com/GlitchedZeus/PokeBank-NX/issues/40), [#47](https://github.com/GlitchedZeus/PokeBank-NX/issues/47), [#32](https://github.com/GlitchedZeus/PokeBank-NX/issues/32), [#33](https://github.com/GlitchedZeus/PokeBank-NX/issues/33), [#34](https://github.com/GlitchedZeus/PokeBank-NX/issues/34), [#11](https://github.com/GlitchedZeus/PokeBank-NX/issues/11), [#25](https://github.com/GlitchedZeus/PokeBank-NX/issues/25), [#35](https://github.com/GlitchedZeus/PokeBank-NX/issues/35), [#46](https://github.com/GlitchedZeus/PokeBank-NX/issues/46), [#78](https://github.com/GlitchedZeus/PokeBank-NX/issues/78), [#39](https://github.com/GlitchedZeus/PokeBank-NX/issues/39), [#41](https://github.com/GlitchedZeus/PokeBank-NX/issues/41), [#60](https://github.com/GlitchedZeus/PokeBank-NX/issues/60).

The [full deferred roadmap with issue links](FUTURE_AFTER_GEN15_BACKLOG.md) keeps these discoverable and reopenable. None is falsely marked feature-complete.

### Active owner-acceptance work remains OPEN

- [#307](https://github.com/GlitchedZeus/PokeBank-NX/pull/307) — one Gen I–V application/Claude's Gen V finalization.
- [#313](https://github.com/GlitchedZeus/PokeBank-NX/issues/313) — **every-action, 20-release Gen I–V source audit and exhaustive physical Switch acceptance**.
- [#310](https://github.com/GlitchedZeus/PokeBank-NX/pull/310) — independent application QA, including outstanding workflow hygiene and Gen II export race.
- Legality coordinator [#298](https://github.com/GlitchedZeus/PokeBank-NX/pull/298) / [#303](https://github.com/GlitchedZeus/PokeBank-NX/pull/303) / [#308](https://github.com/GlitchedZeus/PokeBank-NX/pull/308) stay isolated for source/evidence review.
- Gen VI–VIII [#309](https://github.com/GlitchedZeus/PokeBank-NX/pull/309), [#311](https://github.com/GlitchedZeus/PokeBank-NX/pull/311), [#312](https://github.com/GlitchedZeus/PokeBank-NX/pull/312) remain draft **research**, on hold for app integration until #313 owner sign-off.

## Safe future branch pruning — must be explicit and reversible

The repository contains **275 refs with 269 unique tips**, including 152 `feature/`, 40 `codex/`, 20 `ci/`, 11 `docs/`, 10 `claude/` and more. It is **unsafe** to delete by age or prefix because many retain unique research and historical CI work.

Before deleting ANY branch:
1. Re-fetch its latest exact head SHA; record the head and all associated open or closed PRs/issues.
2. Determine whether its **entire** history is reachable from an actively preserved integration branch (e.g., #307 for app, #298/#303 for legality), not merely whether a file looks duplicated.
3. Preserve a durable archive ref/tag or reproducible artifact for every **unique, unmerged** historical line before considering deletion. Never discard legality/source evidence or old owner-accepted NRO references.
4. Never delete an active open-PR head, default `main`, the ongoing Claude lanes, Gen VI–VIII experimental branches, or source-history/legal-audit branches until owners have deliberately transferred them.
5. Only then remove *confirmed redundant* refs with the appropriate repository permission; audit the remaining branch count and ensure the protected keep-set is unchanged.

**Status:** Cleanup of PRs/issues is completed for this round. **Remote branch deletion was not performed** because no safe delete-ref action is exposed by the connected GitHub tool. This document is not authorization to mass-delete branch history.

## Release priority

[Gen I–V every-action checklist](handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md) → owner exact-NRO device acceptance → only then **Gen VI / 3DS new emulator**. All original save sources stay immutable.
