# Gen I–V completion campaign: 2026-10-19 engineering checkpoint

**The owner's 100% definition is not met. This is a pushed engineering checkpoint, not a complete-engine or release claim.**

Repository/branch: GlitchedZeus/PokeBank-NX, existing PR #314, `claude/gen1-5-legality-campaign-w9mkom`. Implementation: `d2253f8a36314216010c873039314a0dbcc92bfb`, parent `b174f06a577b16a45d340741f39fc6b64a8e89bd`. App #307 was re-fetched at `9d4b8b71befea78c76802c15f1911a0ca24e775c`; no remote app/UI/main change. Existing specialist code, source histories and regression gates are preserved. The evidence follow-up also preserves concurrent coordinator source-pin fixes in `d3727653d3f7d361896a6e749af245415981ddb0`.

## Production changes

- Korean Gold/Silver: read-only save layout, 17-byte box names, native pockets, bounded Korean charset decoding and raw names preserved; per-record encoding survives a Western container. Native PK2 legality receives Korean language evidence. Korean production differential skips removed. This is parser/record support, not complete Korean link/event history or editor/export certification.
- Gen III: source-local immutable e-Reader berry context validates rotated save sectors, selects the active copy and checks native berry integrity/name policy. Explicit Context reaches the production analyzer; no global last-loaded-save state. Missing/malformed evidence stays Unknown. The app provider still needs to attach this optional context, and complete historical payload authentication is not claimed.
- Shared report presentation: Invalid / No problems found / Incomplete–Unknown follow verdict(), including warning-free incomplete reports. The real Gen V staged workspace has a tested bridge. A current-app three-file proposal preserves the app's recent modal/scroll/control work; native build/display acceptance remains pending.
- Differential tooling: retain GB corpora, isolate .NET intermediate outputs between PKHeX revisions, log caught Gen V generation exceptions, expose Fishy flags, replay fixed inputs, and generate three simultaneous mutation operations on every retained Gen V base record.

## Raw observed rates — never completeness percentages

Baseline PKHeX: `6501f0ab46e8f8ca048539dbaf8cae8cb104e722`. Development Gen I seed 39173, holdouts Gen II731921 / III892771 / IV892779 / V892783 declared before their execution. All Unknown, historical/event and Korean records remain in denominators. A raw mismatch remains a mismatch in this table even when an older predicate-aware gate has a source rationale. Synthetic repeated records are not independent historical evidence.

| Generation | Records | Invalid detection | Valid certification, including Fishy | Raw binary verdict agreement | Incomplete | PKHeX-valid → Invalid | PKHeX-invalid → clean |
|---|---:|---:|---:|---:|---:|---:|---:|
| 1 | 50,000 | 11,444/13,233 (86.4808%) | 36,145/36,767 (98.3083%) | 47,589/50,000 (95.1780%) | 2,161 | 249 | 1 |
| 2 | 50,000 | 13,518/14,755 (91.6164%) | 34,571/35,245 (98.0877%) | 48,089/50,000 (96.1780%) | 1,534 | 340 | 37 |
| 3 | 32,756 | 16,520/16,525 (99.9697%) | 15,586/16,231 (96.0261%) | 32,106/32,756 (98.0156%) | 650 | 0 | 0 |
| 4 | 50,995 | 27,770/27,770 (100.0000%) | 22,521/23,225 (96.9688%) | 50,291/50,995 (98.6195%) | 659 | 45 | 0 |
| 5 | 135,193 | 72,410/72,717 (99.5778%) | 62,020/62,476 (99.2701%) | 134,430/135,193 (99.4356%) | 763 | 0 | 0 |

Gen I/II binary rates are deliberately lower than the older exception-adjusted claims. The existing predicate-aware gates pass while raw differences remain: Day Care/EXP, stat-experience floors, trainer/encoding policies, GS Ball Celebi and unrecorded event hypotheses. Those predicates require independent review; they are not automatically successful parity. The 45 Gen IV raw Invalid disagreements are the existing stricter met-location-field rule, retained in the counts. No new claim of superiority over PKHeX is made.

Gen III and V baseline comparison gates found no false Invalid/false clean, but five and 307 PKHeX-invalid records respectively remain Incomplete. The additional Gen V seed8173921 set contains 27,053 records with three mutation operations each: 25,205/25,383 invalid detection (99.2987%), 1,649/1,670 valid certification (98.7425%), 199 Incomplete, zero raw false Invalid/false clean. It is a separate set and does not inflate baseline accuracy.

Non-Fishy certification and three-state metrics are separately recorded in OBSERVED_METRICS.json wherever oracle metadata supports them. Earlier GB output did not expose Fishy, so its non-Fishy metric is unmeasured. Gen V replay kept every record byte/ID and added Fishy metadata. The engine sometimes returns clean for PKHeX Fishy results; the three-state metric retains those differences. Matching binary validity is not complete severity parity.

Current PKHeX `6042efa241a9925ce57e645329ea7f976deabdd3`: 10,000-record Gen II seed719921 probe **FAILED** with 12 unexplained EggMoveCombination findings. UPSTREAM_BREEDING_CASES.json retains every ID, byte field and both outputs. They include Korean, Japanese, Western and unhatched/hatched records. Latest EggVerifier.VerifyEggBreedingChain invokes a single compatible father-chain proof absent from the baseline engine. No exception was added and no failure was converted into a green result. The probe is a compatibility diagnostic; any repair must be followed by a freshly declared holdout.

## Per-title evidence and semantic coverage

RULE_INVENTORY.json contains both source snapshots, hashes, immutable source URLs, changed-file inventory, all 20 title ×34 area rows, and regional variants. It is an audit scaffold, **not the requested completed semantic-rule inventory**. All applicability, per-rule positives/negatives, original-source adjudications and production traces must be completed before the applicable-rule denominator is certified. It does not call a source occurrence Complete. Rule coverage, production-wired rule coverage, historical-provenance coverage and original-source percentages remain **unmeasured for each of Gen I, II, III, IV and V**; a fabricated 0% or 100% would be misleading.

PER_GAME_METRICS.md lists all 20 titles. Gen III/IV corpora carry the exact source title through the production analyzer. GB corpora carry native format/language but not the exact source save title, so per-title accuracy is unmeasured. Gen V corpus game labels are origin versions (including earlier-game transfers); they are reported as origin-group observations, not actual save-title wiring proof. The separate workspace bridge verifies all four actual Gen V source identities.

## Research and remaining engineering

See SOURCES.md and SOURCE_PINS.json for exact primary-source pins/paths and license attribution. Existing Mew/Celebi, GS Ball, GameCube, Ranch, PCD/PGT/PGF and transfer work is preserved. No authenticated original personal save or event payload was imported in this session. A PKHeX-created event-shaped record proves neither historical authenticity nor completeness of the event catalogue.

Highest-impact remaining work, with exact locations:

1. **Father-chain legality across Gen II–V.** GB/Gen3/Gen4/Gen5 analyzers and shared chain proof/data; begin with the 12 current-upstream cases and original `pokecrystal/engine/pokemon/breeding.asm` inheritance/compatibility. Include alternate histories, compatible egg groups, male/female-only species and event/transfer move sources. Do not turn an unproven bounded search into Invalid.
2. **Actual source context reachability.** Gen3BerryContext is implemented/tested; app provider construction and caller ownership remain. Native Day Care slot parsing and level/EXP history remain in Gen1/2ReadOnlySave and GBLegality. Gen IV Korean GTS/Ranch/storage-path evidence needs a source-local provider and Gen4 analysis, not record-origin guesses.
3. **Historical provenance.** Inventory and authenticate original Mew distributions, Japanese Mobile/GS Ball/Celebi receipts, original e-Reader/GameCube/PCD/PGT/PGF artifacts with verifiable custody. Current core GB event fallback and Gen3/4/5 event candidate matching must retain Unknown for unauthenticated cases. Locate primary material; expand production predicates/positive and negative fixtures rather than equating PKHeX templates with authentic receipts.
4. **Semantic audit and latest upstream.** Adjudicate each source site's Gen I–V applicability and full helper dispatch. Important current changes include encounter generators3/4, Gen V encounter data, Ranch/trade/egg templates and chain breeding; the 102-file drift is not fully audited.
5. **App/independent acceptance.** APP_HANDOFF.md describes dependencies and the exact-head patch. Native NRO/rendered UI and physical Switch testing, full final-head CI, independent QA and source-disagreement review remain required. No 100% or release-ready claim is supported.

## Reproduction and datasets

The complete retained text corpus/verdict streams are in the local archive named/hash-bound by DATASETS.json. OBSERVED_METRICS.mismatches.csv lists **all** raw mismatches/Incomplete records; full exact bytes and oracle/engine explanations remain in the archive. No source ROMs, personal saves or SDK binaries are bundled. This archive is a local deliverable, not an implicit GitHub release upload.

Use .NET10, C++20 and Python3; pin/check the source revisions before generating:

```sh
WORK=/tmp/gb1 tools/pkhex_gb_diff/run.sh /path/to/pinned-PKHeX /path/to/pokered 50000 39173 1
WORK=/tmp/gb2 tools/pkhex_gb_diff/run.sh /path/to/pinned-PKHeX /path/to/pokered 50000 731921 2
WORK=/tmp/g3 tools/pkhex_gen34_diff/run.sh /path/to/pinned-PKHeX 892771 3 3
WORK=/tmp/g4 tools/pkhex_gen34_diff/run.sh /path/to/pinned-PKHeX 892779 3 4
GEN5_DIFF_KEEP=/tmp/g5 tools/pkhex_gen5_diff/run.sh /path/to/pinned-PKHeX 892783 4
# Build PKHeXGen5Oracle.csproj as run.sh does. Reuse the exact retained input:
dotnet /path/to/oracle/PKHeXGen5Oracle.dll replay /tmp/g5/pkhex.txt > /tmp/replayed.txt
dotnet /path/to/oracle/PKHeXGen5Oracle.dll adversarial /tmp/g5/pkhex.txt 8173921 > /tmp/adversarial.txt
GEN5_DIFF_CORPUS=/tmp/adversarial.txt GEN5_DIFF_KEEP=/tmp/adversarial-results tools/pkhex_gen5_diff/run.sh /path/to/pinned-PKHeX
# Latest-source compatibility, expected to expose the retained breeding gap:
WORK=/tmp/latest-gb2 tools/pkhex_gb_diff/run.sh /path/to/6042efa-PKHeX /path/to/pokered 10000 719921 2
```

Raw metrics reproduction: `python3 tools/legality_completion/metrics.py OUTPUT.json GEN:LABEL:DIRECTORY ...`. It rejects missing, duplicate or mismatched record identities/verdict counts; no allow-list removes raw differences. Gen V METADATA.json marks streams exposing Fishy. Generation exceptions are now logged, but the old generator's sampled/omitted attempts remain a coverage limitation; a fully exhaustive framework is still required.

VALIDATION.md records 24/24 focused compiler/sanitizer executions, original-source/fixture limits, harness failures and repairs, additional app bridge checks, and the outstanding full-suite/independent/native acceptance. The focused GitHub workflow adds these checks without replacing existing lane/host/differential gates. ACTIONS.json binds fetched workflow links/status to an exact implementation commit; pending CI is not reported as passed.
