# Independent Gen I/II legality audit

Audit target: `49240a36dd5dad2608b2571145652a39c0aeefd5` (PR #303).
Coordinator reference: `5bf98b115ee9c3ff9189cccdfb7d9fd8040577c1` (PR #298).
Exact parent used for inherited-failure reproduction: `c2544ac6d278468f6d30826b9a0115b098e5913b`.
Pinned PKHeX.Core: `6501f0ab46e8f8ca048539dbaf8cae8cb104e722`.

Recommendation: **BLOCK — UNSAFE** until the lifetime defect is fixed separately,
the standalone checker stops blessing contradictory records, and required test
failures are resolved. This audit changes QA tools, fixtures and reporting only.
It does not modify production legality behavior, enable Gen II application
verdicts, edit source saves, modify Claude's branch, or merge any branch.

## Verified findings

### F1 — High: Negai Boshi candidate borrows a destroyed string

`src/Legality/Legality.cpp:603–614` constructs a named
`Gen3NegaiBoshiEvent::Candidate` using `pk.otName()`. That method returns an owning
`std::u16string` temporary; `Candidate::otName` is `std::u16string_view`.
The temporary dies at the initializer's semicolon. `commonFields()` later
compares the view with the event name. Confidence: **high**.

`tests/audit/gen3_negai_lifetime_repro.cpp` reproduces that exact lifetime pattern
with the actual production candidate and matcher. Clang ASan reports
`stack-use-after-scope`, reading two bytes. Strict Clang also rejects the real
production translation unit at line 608 with `-Wdangling-gsl`; the same error was
reproduced on the exact parent. GCC passing other tests does not establish safety.

Expected: live storage through every matcher call. Observed: use after lifetime.
Safe recommendation: bind `const std::u16string negaiOtName = pk.otName();` before
constructing the candidate, and reference that owner, or make the candidate own
its name. Put the production repair in a separate focused Gen III change.
The audit branch includes only the reproducer.

### F2 — High: adult duplicate moves receive standalone Legal

`src/Legality/GBLegality.cpp::movesWellFormed` checks IDs, a leading move and packed
slots, but does not reject duplicate nonzero moves. The separate duplicate check
in `eggMovesProblem` applies only to unhatched eggs. `checkGen1` and `checkGen2`
therefore accept ordinary records with the same move in two different slots.
Confidence: **high**.

Reproduction: the six `g1-duplicate-*` and six `g2-duplicate-*` cases in
`tests/audit/gen12_adversarial.corpus`. All six slot pairs are covered. For a
level-50 Bulbasaur at 117,360 EXP, duplicate Tackle is accepted by both NX
standalone formats. Pinned PKHeX rejects it with `Move1:Duplicate`.
Original TM acquisition checks scan existing slots before teaching a move:
[pokered CheckIfMoveIsKnown](https://github.com/pret/pokered/blob/af519899719f0754965776faac0e836a3b906e6d/engine/items/tmhm.asm),
[pokecrystal KnowsMove](https://github.com/pret/pokecrystal/blob/3bc8daa4173e96a7f4011dad3922eb6fa5dad5c6/engine/pokemon/knows_move.asm).

Expected: Invalid for these ordinary persistent native records. Observed: Legal
with no problems. This is **not** a claim that the Gen I application displays
false legal: `analyzeNativeGen1` already has a separate duplicate guard. The GB
comparison executable bypasses it. Gen II application origin verdicts remain
disabled. Recommendation: add the invariant to the shared standalone boundary
and retain the existing application guard.

### F3 — High: Gen II stored level contradicts EXP but is accepted

`src/Legality/GBLegality.cpp::checkGen2` checks EXP for eggs and level 100, but uses
`Entity2::level` for origin and move history without comparing EXP-derived level
at other levels. Confidence: **high for the ordinary party records tested**.

Reproduction: `g2-level-50-exp-0`: species 1, party level 50, zero EXP and attainable
moves. Expected: Invalid; observed: Legal as a hatched Bulbasaur. Pinned PKHeX
rejects zero-EXP Bulbasaur as having no valid encounter. The additional
`g2-level-50-exp-109923`, `g2-level-50-exp-125126`, and
`g2-level-99-exp-1059860` cases are accepted by **both** checkers even though the
stored party level is inconsistent. PKHeX's `PKM.CurrentLevel` is derived from
EXP, so that agreement does not verify stored-level integrity.

Source: [pokecrystal CalcLevel](https://github.com/pret/pokecrystal/blob/3bc8daa4173e96a7f4011dad3922eb6fa5dad5c6/engine/pokemon/experience.asm),
[PKHeX PKM.CurrentLevel](https://github.com/kwsch/PKHeX/blob/6501f0ab46e8f8ca048539dbaf8cae8cb104e722/PKHeX.Core/PKM/PKM.cs).
Bulbasaur's Medium Slow formula gives 109,923 / 117,360 / 125,126 EXP for
levels 49 / 50 / 51.

Recommendation: validate normal native party level/EXP before origin search.
Do not apply that rule indiscriminately to internal Day Care records: the games
can retain the deposit level while accumulating EXP. Preserve the special Odd
Egg EXP rule. `GB::Entity1` does not carry EXP; its harness similarly bypasses the
Gen I application's existing level/EXP guard.

### F4 — High, QA runtime: malformed corpus reads beyond its buffer

Original `tools/pkhex_gb_diff/gb_diff.cpp::main` treats every body length except 48
as PK1, then reads byte 0x21 and other fixed offsets without checking length.
Input `1 truncated 0 01 50 50` produces ASan `heap-buffer-overflow`.
Confidence: **high**. This is the differential harness, not the native save parser.

Expected: diagnostic and nonzero exit without a sanitizer error. Observed before
repair: invalid read and exit 1. Repaired C++ and C# harnesses reject bad sizes,
name sizes, hex, language/egg flags, extra fields and duplicate IDs with exit 2.
Neither silently skips malformed lines. The native protocol runner exercises a
valid control and 12 malformed inputs against each real executable; the C++
runner is also tested with ASan/UBSan.

### F5 — Medium: comparison accounting and exception logic can hide defects

Original `compare.py::load` overwrites duplicate IDs; corpus IDs are also silently
overwritable. Matched unknown verdicts such as ERROR/ERROR and an empty corpus
can exit successfully. Uncertainty is admitted when **any** known reason occurs,
so an unrelated definitive complaint can be masked. Deliberate differences are
accepted by explanation substrings without inspecting the record.
Confidence: **high**, reproduced by the CLI protocol tests.

The audit rejects duplicates, malformed rows, unknown verdicts, empty evidence,
missing/extra records and explanations inconsistent with VALID. Every uncertainty
complaint must have a known basis. GS Ball exceptions require an actual Japanese
Celebi with the relevant met fields and only the expected PKHeX complaints;
level-100 exceptions require the stored level, EXP direction and sole complaint
to match. Word-filter result codes are exact nickname codes, not substrings.

The stricter checker accepts all six original 50,000-record runs with unchanged
counts. Thus this audit does **not** allege that those actual 300,000 results
contained duplicate IDs or masked unexplained differences. The confirmed defect
is that the original comparison protocol could let such evidence pass.

### F6 — Medium: stale L5 wiring assertion fails the integration gate

`tools/audit_legality_prod_wiring_contract.py::audit` requires the literal
`--expected-invalid 38`, while the committed workflow and independently executed
invalid-site scanner require and observe 44. The command exits 1 at the audited
SHA and at the exact parent. Confidence: **high**.

The audit updates only the QA wiring contract to 44. The scanner still requires
exactly 44 sites and F3/F4 native guards; no production check is removed. The
repaired static contract passes. This proves a static wiring match, not runtime
parity or historical authenticity.

### F7 — High: Gen I application accepts EXP above the level-100 cap

`src/Legality/Legality.cpp::analyzeNativeGen1` compares stored level with
`getLevelFromExp`, whose lookup clamps every larger EXP value to 100. It does
not independently reject EXP above the species' level-100 total. The standalone
Gen I subject also omits EXP. Confidence: **high for ordinary native party data**.

The new `Pokemon1ReadOnly` / `Legality::analyze` regression, including a real
serialized-save -> `Gen1ReadOnlySave::parse` -> wrapper -> analyzer path, exposes this:
Bulbasaur, level 100, 1,059,861 EXP (one above its 1,059,860 cap), four attainable
moves, ordinary names, zero Stat Experience and a consistent Gen I catch rate.
Expected: Invalid. Observed: **NoProblemsFound**. Pinned PKHeX rejects the raw
counterpart with `Level:LevelEXPTooHigh`. This finding affects the application
analyzer, unlike the standalone-only duplicate/under-100 EXP findings.

Original R/B battle EXP is capped, and Day Care retrieval also clamps level and
writes maximum-level EXP before returning the Pokémon:
[pokered Daycare script](https://github.com/pret/pokered/blob/af519899719f0754965776faac0e836a3b906e6d/scripts/Daycare.asm),
[pokered battle EXP](https://github.com/pret/pokered/blob/af519899719f0754965776faac0e836a3b906e6d/engine/battle/experience.asm).
The documented transient Day Care EXP accumulation is therefore not a normal
party-record witness for this specimen. Unsupported glitch histories must be
considered separately rather than silently declared proven.

Recommendation: add a species-specific maximum-EXP check to the native PK1
application's structural checks, in Claude's production change. This audit
retains the failing acceptance regression and does not change production.

## Independent re-examination of the seven newest fixes

1. **Gen I evolution order / Stadium 2.** The Exeggutor case is possible in the
   pinned Stadium reminder model: Exeggcute learns PoisonPowder at 37, evolves
   later, and Exeggutor's earlier Stomp is reminded. Original R/B lists Stomp at
   28 and PoisonPowder at 37. The implementation now limits reminder sources
   when Gen II is unreachable and reproduces the pinned verdict. Specific
   Transfer Pak/menu execution and cartridge-specific reminder quirks have not
   been independently run here; PKHeX's reminder table ultimately cites a
   reverse-engineering report. That remaining evidence gap must not be described
   as original-hardware verification.
2. **Kingler Protect / Take Down.** Original Crystal Krabby has Protect by level
   up and TM compatibility; original Gen I supplies Take Down. Relearning
   Protect after the Gen I portion is a valid move-source explanation in pinned
   `LearnGroup2`. The native regression passes. Stadium 2's retained met data
   route is explicitly documented in pinned `GBRestrictions`; this session does
   not claim a hardware trace of it.
3. **Unhatched egg nicknames.** The standard creation routines assign EGG and
   translations, and the pinned nickname verifier rejects a renamed unhatched
   egg. The current regressions distinguish translations from arbitrary names;
   all pass. Cross-language trade/name-field preservation is still bounded by
   the language scenarios covered, not an exhaustive international trade proof.
4. **Level-100 EXP.** Battle and Rare Candy behavior supports the level-100 cap
   for ordinary party records. Day Care EXP can exceed a level threshold while
   its stored deposit level remains unchanged; retrieval recalculates the level
   and writes that level's threshold EXP. These are different save contexts.
   PKHeX does not simply waive lower EXP at stored level 100: it derives the
   current level from EXP, explaining the intentional differential. The more
   general stored-level omission is F3.
5. **Japanese GS Ball Celebi.** A first-party source explicitly confirms that
   original GBC Crystal used Mobile Adapter special news to obtain Celebi and
   distinguishes the VC route, which needs no adapter:
   [official Pokémon Crystal page](https://www.pokemon.co.jp/ex/VCCR/171214_02/).
   The pinned Ilex Forest script loads Celebi at 30. The Western source's save
   routine explicitly documents the VC hook enabling the GS Ball flag. These
   support separating original Japanese Mobile distribution, inaccessible
   unmodified Western retail script, and VC activation. The precise historical
   news payload and original Japanese caught-data implementation have not been
   rebuilt here; no distribution-date claim beyond verified evidence is made.
6. **Korean Gold/Silver.** The assertion that no Korean disassembly exists is
   outdated. [Narishma-gb/pokegold-kr](https://github.com/Narishma-gb/pokegold-kr/tree/c90f31dadef17f033a8f1bdada701b26cef18241)
   includes Time Capsule exchange, trade validation and record initialization
   code. Its `move_mon.asm` initializes the unused bytes to zero; `link.asm`
   contains Gen I conversion and party exchange. It is WIP and requires base
   ROMs to establish matching builds. No ROM or hardware validation was done,
   and source presence alone is not proof of end-to-end Korean/Western trade
   compatibility. Retain **Incomplete** for disputed Western-trade, met-data
   and naming-screen cases; do not upgrade them using this discovery alone.
7. **Verdict certainty.** Current Mew/Celebi fallback avoids definitive rejection
   merely because a known event list has no match, and tentative-only candidates
   stay Uncertain. New boundary tests check status/problem consistency. However,
   the general search still treats all other exhausted candidate lists as
   definitive; complete original-game coverage must be established before that
   becomes an authenticity claim.

## Corpus and coverage conclusions

The production tables and stochastic corpus share PKHeX resources and the table
parser. This is useful transcription coverage, not independent historical proof.
Both random generators deduplicate move choices and normally synthesize EXP from
level, systematically reducing the chance of F2/F3. More corpus size would not
repair those normalizations. The runtime compares standalone GB routines, not
`Legality::analyze`; PK1 application guards therefore need a separate test.

The new deterministic corpus has 28 raw records with independent structural
expectations: two legal controls and 26 invalid records. NX disagrees with 24
expectations; PKHeX with eight. The ordinary differential reports 17 unexplained
differences, one existing deliberate level-100 difference, and seven contradictory
records accepted by both. The structural runner fails on those records even when
both engines agree. It is intentionally an acceptance failure until fixed, with
no added allowlist. These synthetic records are not authenticated retail specimens.

The original 300,000-case results reproduced exactly, with every ID accounted for:

| Format | Matching legal | Matching invalid | Uncertain | Deliberate | Unexplained |
|---|---:|---:|---:|---:|---:|
| PK1 | 110,644 | 39,289 | 67 | 0 | 0 |
| PK2 | 105,257 | 42,375 | 1,705 | 663 | 0 |

`gen12-independent-20261013/differential-summary.json` records SHA-256 hashes of
each original corpus and both runtime outputs. Full corpora remain local; no
personal save, ROM or distributed event payload is included.

## Actual validation

Final test ledger is maintained in `gen12-independent-20261013/test-ledger.md`.
It includes failing and blocked commands, exact-parent reproductions, sanitizer
findings, and the limits of each passing suite. Test construction/link mistakes
were repaired before reporting final QA results; a failed attempt is not a pass.

Independent QA additions: strict protocol tests, validation in both runtime
harnesses, 28 deterministic adversarial records and source-based expectations,
structural comparison without exemptions, a native PK1 analyzer guard matrix,
2,048 GB boundary/verdict/immutability probes, a real Negai lifetime reproducer,
CI execution/artifact retention, the corrected L5 wiring assertion, and a
pre-existing L3 test compilation repair that splits incompatible auto deductions
without changing either expression or assertion. Production
source files remain byte-identical to the audited commit; QA files were modified
while later tests ran, and their final contents are committed separately.

## Remaining gaps and exact work before merge

- Repair F1 in a separate Gen III production change; retain strict Clang diagnostics
  and prove the owner remains alive through both Negai matchers under ASan.
- Have Claude repair F2/F3/F7 or explicitly tighten the standalone API's structural
  contract. Keep normal party, box and transient Day Care interpretation distinct.
- Make the new structural acceptance gate pass against original source invariants;
  do not add differences or uncertainty allowlists to hide contradictory records.
- Resolve inherited host/Clang/L2 failures in their appropriate scopes. The Gen IV
  ball issue belongs to the existing separate PR #305, not this audit branch.
- Obtain cartridge-specific Stadium 2 reminder/menu evidence for the asserted
  historical routes, and matching Korean source/hardware evidence for disputed
  trade and naming behavior. Keep unsupported cases Incomplete.
- Independently verify full Gen II encounter/learnset/trade data against original
  game code. The existing pret verifier runs 2,673 **Gen I** comparisons only;
  rebuilding Gen II tables from PKHeX is not independent Gen II validation.
- Establish completeness for event/gift distributions and alternative origins,
  joint four-move breeding parents, region-specific text and transfer histories.
  The current individual egg-source union does not itself produce one compatible
  historical father carrying all required inherited moves.
- Require exact-current-head green integration checks after those repairs. No
  original save writes, Gen II application verdict enablement, Gen V/Master Vault
  expansion, editor replacement, force push or merge is authorized by this audit.
