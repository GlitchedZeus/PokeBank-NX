# PokeBank NX — Three-Branch Consolidation Plan

**Owner decision — October 15, 2026.** This is the intended *end state*, **not** authorization to merge active drafts, delete unique history, or claim hardware acceptance.

## End state: three durable working branches

| Branch | Authority and use | Promotion gate |
|---|---|---|
| `main` | **Owner-accepted application and release history.** Native Gen I–V editor, tested artifacts, safety contracts, finalized documentation, and any later features individually accepted for release. | Owner hardware signs exact NRO and release gates; only reviewed PRs merge. |
| `development/gen6-plus` | **One future-generation application line**, starting with the new emulator/3DS Gen VI workflow, then VII and subsequent generations in order; small temporary feature PRs may target this branch while active. | **Create or activate after the full Gen I–V owner hardware acceptance in issue #313.** Native format, emulator source discovery, shared-editor integration and hardware tests separately gated per generation. |
| `development/legality-engine` | **One ongoing source-backed legality integration line.** Gen I–V first, then historical/modern-format expansion. All verified evidence rules and QA feed this branch; release-ready, read-only subsets may later merge into `main` with explicit review. | Coordinator and independent native/PKHeX/original-source/CI evidence gates. Unsupported histories remain Unknown/Incomplete, not fabricated Invalid. |

The **three-branch target does not mean deleting the existing temporary code before it has been reviewed**. During the migration, use existing active branches rather than creating duplicate integration PRs. Retain exact owner-accepted NRO references and historical source evidence as immutable annotated Git tags/releases/documents, **not hundreds of permanent working branches**.

## Current authoritative lanes (as checked October 15)

- **Application:** [PR #307](https://github.com/GlitchedZeus/PokeBank-NX/pull/307), `claude/gen5-integration-20261014`. Gen I–V combined native candidate is in active work; the earlier `35cbf163` HEAD passed 9/9 CI, but newer heads continue to move. The owner-tested `fa9cfcea` build **failed Gen V hardware**, so no acceptance is claimed.
- **Legality:** [PR #314](https://github.com/GlitchedZeus/PokeBank-NX/pull/314), `claude/gen1-5-legality-campaign-w9mkom`, is **already consolidating L1–L5, #303/#305/#308 and Gen V legality next**. Its current full host/L3/cross-lane gates include failures. **Finish #314 rather than opening another broad legality branch.** Existing sub-lanes remain referenced until their exact heads are proven incorporated.
- **Future-gen staging:** [#309](https://github.com/GlitchedZeus/PokeBank-NX/pull/309) Gen VI, [#311](https://github.com/GlitchedZeus/PokeBank-NX/pull/311) Gen VII and [#312](https://github.com/GlitchedZeus/PokeBank-NX/pull/312) Gen VIII: valuable separate experimental drafts but **not approved for active application integration until Gen I–V passes on Switch**.
- **Application QA:** [#310](https://github.com/GlitchedZeus/PokeBank-NX/pull/310) independent fuzz/export/NRO audit. Keep the concrete unfinished QA-F2 workflow hygiene and QA-F3b Gen II export publication safeguards visible until fixed.

## Phase A — finish the Gen I–V application before simplifying `main`

- [ ] Claude finishes Gen V inside **the same shared editor** as Gen I–IV; no new Gen V-specific application shell.
- [ ] Correct and test source/Save Instance assignment, retail primary/backup selection, live Product Home card data and actual emulator provider labels; no save relocation demand.
- [ ] Finish species/modal sprite/shiny previews and Move ACC / PWR / PP parity; match generation-native editable fields, Create, Trainer, Bag, Pokédex and staged review/export.
- [ ] Resolve the owner's prior Gen V device failures and Gen I–III RetroArch / Gen IV–V DraStic launch-return bugs.
- [ ] Native, GCC/Clang, ASan/UBSan, real NRO embedded-art and export-safety tests green at **one frozen exact HEAD**.
- [ ] Audit **every reachable UI operation** in real code and execute [the full Switch acceptance checklist](handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md) across **all 20 Gen I–V games** and inputs. [Issue #313](https://github.com/GlitchedZeus/PokeBank-NX/issues/313) is the one authoritative tracker.
- [ ] Owner explicitly accepts an NRO SHA-256; then review the accepted application history for integration into `main`. **Do not merge a CI-only PR into `main` ahead of this sign-off.**

## Phase B — finish an independently verified Gen I–V legality engine

- [ ] Reconcile every owned L1–L5 Gen I–IV lane and the approved fixes from #303/#305/#308 into [#314](https://github.com/GlitchedZeus/PokeBank-NX/pull/314) *without losing unique rule/source evidence*.
- [ ] Expand real Gen V format checks and only supported encounter/move/egg/event/origin rules with authentic, pinned original-game and distribution evidence. Disallow invented legality confidence.
- [ ] Clear #314's failed full host, L3 and independent cross-lane audit gates; normal, ASan/UBSan, differential PKHeX and original-source positive/candidate-negative/Unknown tests must all pass together.
- [ ] Verify on-device **read-only legality output** is understandable and has correct supported/unsupported states, without any Pokémon/save mutation and without unrelated UI regressions.
- [ ] Explicitly document the verified Gen I–V rule coverage, which cases remain Unknown and which later-generation/PKHeX comparisons are still out of scope. **"Ready" means the stated Gen I–V coverage contract is met; it is not a fabricated claim of complete worldwide historical PKHeX parity.**
- [ ] Once accepted, promote the verified legality integration history to `development/legality-engine`; merge an approved, safe read-only subset into `main` only after cross-app integration tests and the owner agrees.

**Sequencing note:** Phases A and B may be worked on *in parallel on separate branches*. Gen VI application integration remains gated on Phase A owner acceptance; the project's three-branch cleanup and a release-level legality claim should wait until the relevant Phase B rules/tests are accepted.

## Phase C — repository compression without losing history

1. **Archive inventory**: enumerate all branches, exact SHA, open/closed PR and unique commits. Git tags/releases retain milestones, source evidence and owner-approved binaries. Preserve legal/license provenance and golden fixtures.
2. **Land acceptance**: merge only approved, current-head integration PRs, never force-push/reset/rebase backwards. Do not assume a green CI run proves physical hardware behavior.
3. **Close superseded PRs** only when their entire HEAD is an ancestor of an authoritative retained branch **or** their unique changes have been explicitly dispositioned; link to successor, original SHA and evidence. Keep active fixes and QA issues open until truly completed.
4. **Consolidate issue tracking** around [#29 release roadmap](https://github.com/GlitchedZeus/PokeBank-NX/issues/29), [#313 full Gen I–V acceptance](https://github.com/GlitchedZeus/PokeBank-NX/issues/313), the legality coordinator and specific unresolved bugs. Future features remain indexed at [the preserved backlog](FUTURE_AFTER_GEN15_BACKLOG.md), not marked complete.
5. **Prune obsolete refs after proof**: for each branch, verify ancestry, remaining PR references, archive tags and no independent changes. Never delete working-Claude branches, historical owner-accepted source/artifact refs or independently valuable legality research simply because they are old.
6. **End with one active PR per long-lived lane whenever practical**, plus short-lived scoped PRs for actual changes. Work down the issue and PR count by implementing, testing and genuinely closing tasks—not by hiding missing work.

**Current housekeeping baseline:** 275 branch refs, 269 unique tips; after earlier cleanup 28 open PRs / 28 open issues. During this check, two new active PRs #314 and #315 appeared, raising open PRs to 30. Earlier four fully incorporated PRs #278/#300/#304/#306 and 18 deferred ideas were closed with history preserved; **zero branch refs were deleted**. See [the cleanup ledger](GITHUB_CLEANUP_20261015.md).

## Non-negotiable safeguards

Original installed/emulator saves stay **immutable**. Editable state lives only in verified app-owned staged workspaces; output is a separately confirmed copy. A is never destructive. No unlocked cross-game True Move, blanket Inject Save, premature Master Vault, or unsupported source/legality assertion. Each archived reference/cleanup action must be reversible until unique data is safely retained. Default `main` is currently not the unmerged app/legality integration history.

**Result we want:** one dependable `main`, one future-generation development branch, one legality development branch, small reviewable PRs, a shrinking **truthful** issue backlog, and permanently accessible research/artifact evidence.
