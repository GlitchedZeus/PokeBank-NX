# October 28 engine campaign — HGSS encounter repair

**Current continuation:** [joint breeding coverage and remaining rules](JOINT_BREEDING_GAPS.md),
[new primary replay](JOINT_PRIMARY_METRICS.json) and [independent Gen I–V sample](JOINT_INDEPENDENT_METRICS.json).
These supersede the pre-breeding counts below for the changed production path.
The seven original Gen V egg false cleans are now Incomplete; their joint rules
remain open. The new primary replay also retains raw stricter Happiny disagreements
and additional Incomplete reports. See the continuation inventory before interpreting the older
paragraphs saying joint ancestry has not changed production. Native III/IV and
hatched V integration remain open. Acceptance is still blocked.

Acceptance remains blocked. This continues PR314 from30696ba7, without changing PR307, building an application/NRO, merging, or replacing earlier evidence. Official PKHeX26.10.18 remains pinned to `bfe89c74a3f5265805e364b79bc04ad38ebbc5ea`. Newer master `8e3eb3d47bb496746c3eaa9d160329d295a5e2e5` is identified separately and is not the oracle for these measurements.

## Actual production changes

Five primary false-clean records used Magikarp rod-swarm placeholders on maps where the native map/type swarm gate cannot enable fishing outbreaks. `hgss_swarm_repair.py` removes590 physical rows (572 distinct projected tuples,52 locations), retaining24,380 slots and all1,766 areas. Only Magikarp Good/Super Rod variants absent even from the broad native ordinary/night/swarm projection are removed. Every other typed row remains. The deterministic generator runs the same pinned correction, including in `--check` mode.

Two other primary false cleans used ordinary Safari bank entries to bypass the real ten-slot Safari level/frame checks. `slotMatchExact` now excludes ordinary slot types at HGSS Safari location202, including when considering transferred origins. The native surf/fishing dispatch uses `FieldSystem_GenerateSafariEncounter` when the Safari system flag is set; ordinary battles cannot provide the required Safari Ball. The real Safari tables and RNG checks remain active.

[HGSS_VERDICT_CHANGES.json](HGSS_VERDICT_CHANGES.json) preserves every changed primary record, exact hex hash, old/new production lines and released-reference result. Exactly seven primary records changed: all from NoProblemsFound to Invalid. Their seven original positive controls still pass. The regression also guards40 real fishing-outbreak slot alternatives across both games. IDs occur only in fixtures/evidence, not production rules.

The broader existing regression exposed an obsolete positive: `slot-type0-pid1-lead12`, RattataLv20 in the ordinary Safari grass placeholder. Its bytes remain permanently in `test_legality_gen4.cpp` as an Invalid regression. The current official release also rejects the unchanged bytes. A separate genuine retained HGSS grass positive supplies the independent per-rule mutations; their exact diagnostic assertions remain enforced. This is a source-backed correction of the fixture's premise, not removal of a difficult case.

## Evidence and limits

Original source: [pret/pokeheartgold9d8b759](https://github.com/pret/pokeheartgold/tree/9d8b7591f09b65804da2fb2dfd56f320633e0d36). [HGSS_SWARM_SOURCE_PINS.json](HGSS_SWARM_SOURCE_PINS.json) records SHA256 and Git blob hashes. Unchanged native swarm gate/slot bodies execute3,932,160 map/type combinations, all four fishing outbreaks and each rod's actual replacement slots. Unchanged Safari dispatch bodies separately verify the land/surf/rod handler distinction with isolated callee probes. Both run under GCC/Clang normal and leak-enabled ASan/UBSan. This is original-function replay, not hardware capture or acquired-specimen custody.

The released HGSS Super Rod table omits native outbreak slots1 and4 while duplicating slot3. NX retains all five native slots; this is a verified table/mechanism improvement, not yet a demonstrated individual PID/IV false-invalid case against PKHeX. Do not replace the whole table with the release export.

The broader raw encounter-data projection reports122 missing projected tuples versus the released table. These include the16 real outbreak alternatives, Safari placeholders, inaccessible map geometry and other unadjudicated entries. They are not122 proven missing catchable encounters. Headbutt coverage was not changed or certified by this repair. Safari rates/other lead-frame branches remain separate obligations.

## Raw measurements

The original298,752-record primary corpus is unchanged. GB streams are explicitly reused because this change does not touch their production path; GenIII/IV/V drivers were rebuilt and replayed. All Fishy and errors stay in their original denominators. These are reference agreement measurements, not rule completeness or historical accuracy.

| Gen | Records | Agreement | Raw false Invalid | Raw false clean | Incomplete | Reference Fishy |
|---|---:|---:|---:|---:|---:|---:|
| I |30000|28363|175|3|1459|26633|
| II |30000|28747|192|45|1016|20353|
| III |40945|40179|2|0|764|4529|
| IV |63209|62297|52|0|860|4616|
| V |134598|133697|0|60|841|32833|

Primary GenIV agreement improves by7; other primary counts are unchanged. The separate correlated GenV26,934 retains3 false cleans and193 Incomplete. All earlier retained comparison sets were replayed separately; they are overlapping corpora, not additional unique specimens. See [PRIMARY_METRICS.json](PRIMARY_METRICS.json) and [RETAINED_REPLAY_METRICS.json](RETAINED_REPLAY_METRICS.json).

Fresh seeds were recorded in [HOLDOUT_PLAN.json](HOLDOUT_PLAN.json) before generation. All emitted base records/mutations are retained; no valid-only grown selection is used. Internal pre-emission generator exceptions and template deduplication still limit coverage. PKHeX conversion has independent randomness, so output bytes/hashes, rather than seed alone, identify the actual corpus. There is no new GB holdout in this cycle.

| Gen | Fresh records | Agreement | Raw false Invalid | Raw false clean | Incomplete | Reference Fishy |
|---|---:|---:|---:|---:|---:|---:|
| III |40945|40187|0|0|758|4450|
| IV |63244|62289|45|0|910|4667|
| V |134588|133656|0|36|896|32116|

Fresh GenV555 `Misc:Error` results remain counted. GenIV45 raw stricter results carry the existing coupled-met-field complaint; their source-supported classification must remain separate from raw accuracy. [FRESH_METRICS.json](FRESH_METRICS.json) records cross-tables and raw-file hashes. The raw streams remain in the separate local `outputs/legality-campaign-20261028` archive; the October27 archive is untouched.

## Required acceptance answers

1. **Equivalent coverage:** this change closes the scoped HGSS outbreak gate and Safari ordinary-slot fallback discrepancies. The previous [rule-family inventory](../hardening-20261027/RULE_GAPS.md) remains the broader map; it is not a completed applicable-branch denominator.
2. **Missing/incomplete:** full joint recursive breedingIII–V, general joint EV/EXP histories, remaining regional/name/event/RNG branches and context/evidence gaps remain. The118-case original primary/correlated false-clean ledger retains the seven resolved records and all111 still-open records in [RAW_FALSE_CLEAN_QUEUE.json](RAW_FALSE_CLEAN_QUEUE.json). New holdout cases are additional evidence, not replacements.
3. **Better-supported rules:** the original HGSS one-battle EV/EXP counterexample and coupled native met setters remain scoped native evidence. This cycle adds the original five-slot fishing-outbreak table mechanism; individual capture superiority has not been demonstrated.
4. **Accuracy:** raw counts are above and machine-readable. Synthetic agreement does not establish real-world accuracy. No newly authenticated historical Pokémon was obtained.
5. **Release blockers:** unresolved raw disagreements, avoidable Incomplete, missing joint solvers and branch proof, historical acquisition/receipt gaps, independent review and separately authorized app integration/device validation. Existing app handoff limits remain; no application integration was performed.

## Validation and continuation

Local affected suites pass GCC/Clang normal and ASan/UBSan: GenIV250 cases +40,000 fuzz records, GenV207 +40,000, release144 +256 constructive RNG states, terrain2310 corrupt-byte checks, and14 unchanged targeted fixtures. Counts describe executions, not completeness. Generation check passes. CI must be verified against the actual pushed head.

The frozen6501 reference predates these native corrections. A separate50,999-record run retains128 raw stricter differences:91 unsupported encounter rows and37 pre-existing coupled-met-field disagreements. The unchanged91 records are re-evaluated against the official release; native-level adjudication is in progress. Do not suppress these by ID or replace old raw counts.

The frozen-6501 CI oracles retain their original streams and raw cross-tables. Claude's initial placeholder-membership allowance was too broad: it would excuse a simulated false Invalid on six of seven genuine positive controls. `test_hgss_reference_adjudication.py` permanently prevents that. The tightened gate requires an independent necessary-level absence from pinned original data, including all possible original-game evolution predecessors. Safari union includes every area/time/base/bonus condition, so it is intentionally generous; a possible level does not establish a valid RNG frame. A transfer uses any acquisition level at or below its overwritten transfer level, rather than inventing a lost met location.

[HGSS_NATIVE_LEVEL_INDEX.json](HGSS_NATIVE_LEVEL_INDEX.json) is independently regenerated from native data and hash-checked in CI. [HGSS_FROZEN_REFERENCE_ADJUDICATION.json](HGSS_FROZEN_REFERENCE_ADJUDICATION.json) records the91 native level proofs with unchanged bytes and full original diagnostics. All91 also remain Invalid in official26.10.18. Every one of the128 older-reference stricter rows is independently replayed unchanged in [FROZEN_SECOND_REFERENCE_GEN34.json](FROZEN_SECOND_REFERENCE_GEN34.json):93 released Invalid and35 released Valid. Native coupled setters explain the remaining scoped stricter cases; neither count is removed from raw agreement.

Beyond source-proven absence, a reference-version change requires the actual unchanged record to be Invalid under the unmodified pinned official release. Analysis errors cannot justify an exception. This is a second-reference measurement, not original-game proof. The recovered seven original GenV CI failures are all Invalid under the release, preserved in [FROZEN_SECOND_REFERENCE_GEN5.json](FROZEN_SECOND_REFERENCE_GEN5.json); all seven also fail the generous native Safari minimum-level bound. Dynamic reference replay is keyed by exact byte hashes and verifies matching input/output bytes. No specimen ID list affects production or the comparison predicate. CI preserves both raw references, production streams, joined inputs, assembly/source hashes and engine/reference heads as artifacts, including failures.

Both full older-reference CLI smoke tests pass with the tightened gate and explicit second release: native/current-release disagreements remain visible. Local production code and primary/fresh metrics are unchanged by this verification repair. [REFERENCE_GATE_VALIDATION.json](REFERENCE_GATE_VALIDATION.json) records the tests and source-witness hashes. Existing eleven PR-triggered workflows passed atf5f50e13; that older green state does not certify this new change or full release acceptance.

[CONTINUATION.json](CONTINUATION.json) contains the next concrete work and source paths. Full recursive breeding is the next implementation family; three GenV movesets account for seven primary egg-combination discrepancies. Initial tutor/donor inspection is research only and has not changed production verdicts.
