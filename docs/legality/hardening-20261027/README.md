# October27: Gen I–V release audit and production repairs

**Release acceptance: not achieved.** PR314 stays draft/unmerged on its original base. Engine checkpoint: `0a9ad2073b2ab678e5213b63b6fab60925e23172`. Reference: official [PKHeX26.10.18](https://github.com/kwsch/PKHeX/releases/tag/26.10.18), source `bfe89c74a3f5265805e364b79bc04ad38ebbc5ea`. PR307/app/main were not modified. No Pokémon or original save was changed.

## 1. Exact or evidence-backed equivalent coverage

[RULE_GAPS.md](RULE_GAPS.md) inventories rule families and their title/region/context limits. [RELEASE_CHANGE_TRIAGE.json](RELEASE_CHANGE_TRIAGE.json) classifies all128 paths in the retained legality/resource change list:83 reviewed changes with explicit limits,39 modern-only changes outside nativeI–V,6 new chain files exposing missing integration. Additional changed nativePK2/PK3/PKM/version interfaces were inspected. A changed path is not a semantic rule; unchanged applicable source branches are not automatically equivalent. The larger result-site inventory in the evidence bundle leaves uncertified applicability/proof fields unknown.

New scoped equivalents/repairs:

- Berry Fix released model accepts seeds0..213, preserving dual-OT/gender constraints in nativePK3 and III→IV→V histories.72 JP/English constructions retain correct/incorrect gender, old endpoints, midnight0..2 and214 negatives. Original distributor executable/receipt is not authenticated.
- GenIV traded hatch-game ball reconstruction follows the hatch writer rather than origin version. Native HGSS/Pt routines rebuild the Pokémon while preserving MET_GAME. Public Pt→HG/HG→Pt records and contradictory-ball negatives are permanent regressions. The current hatch-map flag matches the released table; full native map/region proof remains partial.
- GenV evolved normal fixed-slot encounters require the current PID-selected ability; unevolved fixed slots and hidden permission remain checked. Public Munchlax→Snorlax and both wrong-slot directions are permanent regressions. This is reference-equivalent; no GenV executable proof is claimed.
- Global Link export retains774 typed source rows, pairs all762 old Entree rows by represented identity and adds3 represented variants. Unsupported recipient/move alternatives stay Incomplete. Korean language8 is distinct from Spanish7 and promotion-language flags. Existing709 PGF payload/recipient pairing is preserved. These are catalog conditions, not authenticated receipts.
- Ranch IV reversal exhausts the complete low16 state space for the released first/third-output model, independent of PID. No-state remains Incomplete pending original Wii evidence. NativePK4, transferredPK5 and constructive state controls are retained; complete Ranch verdict equivalence is not claimed.
- The GenV app boundary requires exact title, native party/box geometry, valid current trainer and explicit original/staged provenance for a nonempty clean report. It reads current staged trainer/record bytes and returns checked identity. Nonblank species-zero payload and missing/contradictory context cannot silently become clean or invent a box-only Invalid.

The broader structural, encounter, move, RNG, ribbon, item, name and transfer ports are **represented**, with gaps listed individually. These scoped repairs do not certify complete families.

## 2. Rules still missing or incomplete

- General recursive joint father ancestry is integrated for GenII, **not GenIII–V**. The released chain dispatch covers hatched and unhatched origins; local per-move/egg-slot checks do not establish that whole recursive rule.
- GenIII/IV joint EV/EXP acquisition budget is incomplete. The released per-stat sum is unsound; the native counterexample below requires a joint solver for recipients, items, Pokerus, vitamins/berries, caps, levels and era-specific acquisition.
- New HGSS encounter-type/ball/frame conditions, typed HG/SS encounter and GS eggmove resource drift, full reversal seed-set equivalence, word-filter applicability/regex equivalence, optional settings, active-trainer Korean GTS restrictions and rare alternative histories remain open.
- Historical events, original distributor records, PCNY/region/link cases and save-owned Enigma evidence remain partial. Catalog absence cannot prove impossibility. Unsupported receipt histories remain Incomplete.
- Current active classic app panels are placeholders. The engine's older call graph and twenty-title API dispatch do not establish current PR307 UI wiring.
- An independently checked source-branch denominator, production-per-rule trace and external review are unfinished. No completeness percentage is defensible.

## 3. Verified rules better supported than PKHeX

The new native HGSS witness compiles the unchanged original EXP split and EV-award body with pinned Hoothoot/item/encounter data. Six participants/five ExpShares, with the target holding Power Bracer, yield **HP+1 and Attack+4 for1EXP** in one battle. Starting at vitamin-reachable100/100 EVs produces101/104. Released PKHeX requires4 additionalEXP and rejects the exact candidate; NX accepts it. The same EVs with0EXP remain Invalid. This proves a specific original-game counterexample, not a complete EV budget solver or hardware run. [NATIVE_EV_SOURCE_PINS.json](NATIVE_EV_SOURCE_PINS.json) and `ev_exp_native_witness.py` bind the proof.

Existing improvements are preserved: ordinary GenIV coupled met-location setters, bounded terrain bytes that cannot alias low mask bits, and native GB stored-level/EXP invariants. All52 latest raw GenIV Valid→Invalid records independently satisfy the coupled-setter contradiction; they remain in raw counts. The unchanged independent native-chain cross-check constructs6 GenII parent routes rejected by the released reference. Japanese GSBall availability and regional-name differences retain their historical/keyboard evidence limits; none authenticates an individual Pokémon. Ranch uncertainty and unsupported PGL receipt warnings are **not** claimed as verified superiority.

## 4. Fresh raw comparison results

Source-built release oracles run separately from production drivers. Final affected-generation seeds2026102723/24/25 were declared before generation, after the last native repair was committed. GB uses fresh2026102711/12; its production code was unchanged by later repairs. Retained bytes/hashes, not seeds alone, are authoritative: some upstream conversion routines use their own RNG/date state. Generated encounters bias the sample and do not authenticate original-game specimens.

Raw agreement = `(referenceValid / NXNoProblemsFound + referenceInvalid / NXInvalid) / all emitted rows`. ReferenceValid includes Fishy. Incomplete is never clean or binary agreement. Fishy overlaps Valid/Invalid. Oracle Misc:Error stays in the denominator and is not proof of illegality. No difficult record or deliberate disagreement is removed.

| Gen | Rows | Raw agreement | Raw false Invalid | Raw false clean | Incomplete | Reference Fishy, all |
|---|---:|---:|---:|---:|---:|---:|
| I | 30000 | 28363 /30000 (94.5433%) | 175 | 3 | 1459 | 26633 |
| II | 30000 | 28747 /30000 (95.8233%) | 192 | 45 | 1016 | 20353 |
| III | 40945 | 40169 /40945 (98.1048%) | 0 | 0 | 776 | 4424 |
| IV | 63209 | 62290 /63209 (98.5461%) | 52 | 7 | 860 | 4616 |
| V | 134598 | 133697 /134598 (99.3306%) | 0 | 60 | 841 | 32833 |

Additional three-field correlated GenV set:26934 rows,26738 agreement (99.2723%),0 falseInvalid,3 falseclean,193 Incomplete,9158 Fishy,89 referenceMisc:Error. Main GenV set has559 referenceMisc:Error. Allfive generations have0 load failures. Full cross-tables, valid-only Fishy and per-cart/origin splits: [OBSERVED_METRICS.json](OBSERVED_METRICS.json).

Adjudication is **separate**: [ADJUDICATION_SUMMARY.json](ADJUDICATION_SUMMARY.json). GenI154 and GenII192 stricter level/EXP records match the existing native invariant. The remaining21 GenI stricter cases have a scoped Korean-origin policy label with interoperability evidence limits, not a blanket verified-improvement claim. GenII raw clean differences include12 GSBall routes,6 native parent routes,1 regional-name policy and26 unadjudicated records; GenI has3 unadjudicated raw clean differences. GenIV52 stricter met-field records are source-supported; its7 raw clean differences are6 encounter-type and1 RNG-frame complaints. GenV60 raw clean differences are53 word-filter complaints and7 egg combinations; correlated3 are word-filter complaints. These categories do not produce a corrected “accuracy” score.

Every earlier dataset remains measured on the final engine: first299447 rows plus27070 correlated; second299362 plus27054 correlated. [ALL_COMPARISONS.json](ALL_COMPARISONS.json) preserves counts/hashes alongside the latest set. Final GB reuses the second GB set explicitly: do not sum these totals as unique Pokémon. Original first/second streams and final unchanged-byte replays are retained, including prior GenIII egg discrepancies. Earlier frozen campaign denominators were not relabelled as fresh.

The complete public native-format census now includes **all105 files**, selected by `.pk1`–`.pk5`/`.ck3`/`.xk3` suffix regardless of folder, including Illegal and General folders. All105 processed,0 errors, **0 newly authenticated acquired specimens**. The earlier directory selector missed30 files; its75-file result remains preserved in [PUBLIC_FIXTURES_OLD_DIRECTORY_SELECTOR.json](PUBLIC_FIXTURES_OLD_DIRECTORY_SELECTOR.json). [PUBLIC_CENSUS_CORRECTION.json](PUBLIC_CENSUS_CORRECTION.json) maps all75 unchanged original hashes/verdicts to the new census and lists every additional path. No former record was dropped. A permanent regression guards the105-path census and misleading folder labels.

[PUBLIC_FIXTURES.json](PUBLIC_FIXTURES.json) preserves original path/blob/SHA, decoded data, original and converted reference verdicts, native result and assumed host context. It includes4 CK3/3 XK3 derivatives and23 unhatched eggs, including additional regional eggs. Original and converted reference validity agree for every file in this run. These compare decoded canonical stored bodies; original party tails and source-save/slot custody are not established by this corpus. Native party/box contexts are tested separately by the boundary suites. Current cross-tables: G1 sixValid/clean, threeValid/Invalid; G2 threeInvalid/Invalid, twoInvalid/Incomplete, fifteenValid/clean, oneValid/Incomplete; G3 sixInvalid/Invalid, **oneInvalid/clean**, fifteenValid/clean, threeValid/Incomplete; G4 twoInvalid/Invalid, twentyValid/clean, oneValid/Invalid, tenValid/Incomplete; G5 oneInvalid/Invalid, tenValid/clean, sixValid/Incomplete. The new raw GenIII clean disagreement is the published edited Nidoran male PID/inherited-level-up case. Both engines have a gender-bit check, but move-source classification/alternative ancestry needs original-game adjudication; do not automatically change it to Invalid. Three PK1 files carry zero party level; the remaining PK4 form case has an assumed host context. No file is silently normalized, and a public “Legal” filename is not acquisition proof.

## 5. Evidence and integration preventing release acceptance

[APP_HANDOFF.md](APP_HANDOFF.md) supplies the exact workspace call, checked identity/current-trainer contract, per-file conflict ownership and alltwenty active title surfaces. App snapshotd6cc878a is unchanged. The two-file presentation patch passes apply-check; it is a prerequisite, not full GenV wiring. Actual current workspace bridge passes84 cases on allfour titles under GCC/Clang normal and leak-enabled ASan/UBSan. The shared renderer still needs evaluated-report mode and complete wrapped/paged issues. A true merge-tree rehearsal reports13 conflicts; no branch merge or merge commit was made. Combined native build, NRO/device rendering and external review remain outstanding.

Final local affected matrix passes GCC/Clang normal and leak-enabled ASan/UBSan: GenIII506+12000 fuzz, GenIV249+40000 fuzz, GenV207+40000 fuzz, source context247, release rules140 plus256 constructive RNG states, twenty-title API matrix183, verdict/coverage59049 combinations. Four native EV source replays and four actual-workspace runs pass. Counts describe executions, not completeness. [VALIDATION.json](VALIDATION.json) binds the code checkpoint; exact published-head CI must be checked from live run/artifact metadata. Earlier green checks cannot be attributed to a different head.

Release is blocked by the named missing rules, unadjudicated differences, absent authentic historical evidence, incomplete app wiring, merge conflicts and missing combined device/independent validation. This session does **not** declare100% completion or PKHeX parity.
