# Gen I-V legality: production call-graph audit

Owner requirement (final campaign, section 15): a rule is finished only when the application can
run it. This document traces, format by format, how a Pokemon record reaches each analyzer from the
app, what each analyzer gates on, how its findings become the one unified verdict, and where the
chain stops today. Line numbers refer to the head named in `GEN1_5_CAMPAIGN_CHECKPOINT.md`.

## 1. The one entry point

Every app screen that shows legality calls `Legality::analyze(pokemon, gameGroup, exactSourceGameId)`
(`src/Legality/Legality.cpp:3260`, forwarding to `analyze(pk, Context)` at `:1084`).

| Screen | Call site | Record type |
|---|---|---|
| Gen I details modal | `src/UI/Modals/Gen1PokemonDetailsModal.cpp:72` | `Pokemon1ReadOnly` (PK1 + party/box context) |
| Gen II details modal | `src/UI/Modals/Gen2PokemonDetailsModal.cpp:235` | `Pokemon2ReadOnly` |
| Gen III shared surface / details modal / trainer view | `src/UI/Gen3SharedPokemonSurface.inc:1587`, `src/UI/Modals/PokemonDetailsModal.cpp:65`, `src/UI/TrainerViewScreenBase.inc:3241` | `Pokemon3FRLG` (decrypted 80/100-byte PK3) |
| Gen IV shared surface | `src/UI/Gen4SharedPokemonSurface.inc:1863` | `Pokemon4ReadOnlyView` (136/236-byte PK4) |
| Gen V | none on this branch | `Legality::analyzePk5` is engine API only (section 6) |

`exactSourceGameId` is the app's string for the save's exact game (e.g. `emerald_gba`,
`heartgold_ds`). `analyze` refuses to let that label contradict the verified native format
(`Legality.cpp:1090-1104`): a mismatch clears the label and adds a Warning, so a mislabelled bank
entity can never receive another generation's Invalid findings.

## 2. Verdict contract (`include/Legality/Legality.h:51-105`)

- `Verdict::Invalid` when any issue has `Severity::Invalid`.
- `Verdict::NoProblemsFound` only when every `CoverageSummary` field (structure, sourceGame,
  internal, moves, encounter, eventGift, pidRng, eggBreeding, transfer) is `Complete` AND
  `problemCount() == 0` (no Invalid, no Warning).
- `Verdict::Incomplete` otherwise.
- `Report::ok()` is `problemCount() == 0` and is NOT a verdict: it is true for an Incomplete
  report. UI code must use `verdict()`. (#307's overlay uses `ok()`; see
  `GEN1_5_APP_INTEGRATION_PREP.md`.)

Every analyzer below starts its coverage at `Partial` and raises a field to `Complete` only when
its rule family ran to a decision for this record. Nothing ever lowers an Invalid to a Warning or
promotes Incomplete to a verdict.

## 3. PK1 (Red, Blue, Yellow)

`analyze` -> `pk.nativePk1ReadOnly()` (`Legality.cpp:1127`) -> `analyzeNativeGen1`
(`Legality.cpp:162`) -> structure and format checks -> `GB::checkGen1(e)` (`Legality.cpp:229`,
`src/Legality/GBLegality.cpp`).

- Gate: the record wraps a verified native PK1 (`Pokemon1ReadOnly`); the exact Gen I source, when
  supplied, selects Red/Blue vs Yellow tables and Japanese vs international text rules.
- `GB::checkGen1` searches every origin (wild slots, statics, gifts, trades, Time Capsule return,
  recorded events) and applies the per-origin and origin-free rules documented in
  `GB_LEGALITY_VERDICTS.md`. `GB::Status::Legal` -> all coverage Complete; `Illegal` -> Invalid
  issues; `Uncertain` -> Warnings with encounter/event coverage Partial.
- Verified by `tools/pkhex_gb_diff` production-parity (`production_parity.cpp`): the app path and
  the search agree on every corpus record.
- Raw name bytes: `Gen1::PokemonRecord::originalTrainerRaw` / `nicknameRaw` are passed as `e.otRaw` /
  `e.nickRaw` (`Legality.cpp:201-214`), so the glyph and terminator rules (D-3 in
  `PKHEX_VERIFIED_IMPROVEMENTS.md`) run on the stored bytes, not the decoded string. An OT with
  bytes but no decodable glyph is a Warning (Incomplete), not "OT is empty".

## 4. PK2 (Gold, Silver, Crystal)

`analyze` -> `pk.nativePk2ReadOnly()` (`Legality.cpp:1129`) -> `analyzeNativeGen2`
(`Legality.cpp:962`) -> `GB::checkGen2(e)` (`Legality.cpp:1031`).

- Gate: verified native PK2; Crystal-only met data, Korean rules and the exact source profile
  are passed through `e`. Korean records whose cross-language history is unproven stay Incomplete.
- Same verdict mapping as PK1, with the same raw name bytes (`Legality.cpp:1012-1026`). Parity
  measured by `pkhex_gb_diff` (Korean skipped, counted).

## 5. PK3 (Ruby, Sapphire, Emerald, FireRed, LeafGreen; GameCube origins held in them)

`analyze` -> `Gen3Pk3::verify(pk, exactSourceGameId)` (`Legality.cpp:1118`,
`src/Legality/Gen3Legality.cpp:3548`) runs BEFORE the species-0 early return, so an empty slot or a
record with no National Dex index is still reported (`Legality.cpp:1119-1125`). Then the older
Gen III checks in `analyze` run, and `applyGen3Analyzer(r, gen3Verified, ...)` (`Legality.cpp:3255`)
merges the analyzer's report.

- Gate (`Gen3Legality.cpp:3550-3552`): game group FRLG (the shared GBA group) and a data size of
  80 (box) or 100 (party) bytes. Anything else leaves `Verification::ran == false` and the older
  checks alone decide (coverage stays Partial, never NoProblemsFound).
- `analyzeDetailed` (`Gen3Legality.cpp:3571`) follows PKHeX's `LegalityAnalysis.UpdateChecks`
  order; `isCertified` raises all coverage fields when the encounter, PID/IV, moves, egg and
  misc families reached a decision.
- Merge rule (`applyGen3Analyzer`): Invalid and Warning issues are appended (deduplicated by text);
  older-check Warnings that the analyzer verified for this exact record are downgraded to Info;
  coverage becomes Complete only when the analyzer certified AND the older checks raised nothing.
- GameCube (version byte 15) origins: the same encounter search considers the Colosseum and XD
  templates (`Kind::XdTrade` ... `Kind::XdSlot`, `Gen3Legality.cpp:1471`) and `verifyCxd`
  (`Gen3Legality.cpp:4152`) applies the shadow, team-lock, name-screen and starter PID rules. A GBA
  record relabelled with version 15 matches nothing and is Invalid, as in PKHeX. The legacy evidence
  blocks in `Legality.cpp:360-530` still add Info/Warning only (one older Info about GameCube RNG
  evidence can appear next to a matched record; it never changes the verdict).
- `Gen3Cxd::judge` (`Gen3Legality.cpp:4379-4510`, `include/Legality/Gen3Cxd.h`) exposes the same
  rules for a record that has left PK3 format; PK4 and PK5 call it (sections 6 and 7).

## 6. PK4 (Diamond, Pearl, Platinum, HeartGold, SoulSilver)

`analyze` -> older Gen IV checks -> `applyGen4Analyzer(r, pk, context.exactSourceGameId)`
(`Legality.cpp:3256`, `src/Legality/Gen4LegalityHook.cpp:55`) -> `analyzePk4(bytes,
exactSourceGameId, &evidence)` (`Gen4Legality.cpp`).

- Gate (`Gen4LegalityHook.cpp:56-59`): `pk.nativePk4ReadOnly()` non-null and decrypted size 136 or
  236. The hook receives the ORIGINAL exact source id (not the sanitized one) because the Gen IV
  analyzer itself decides whether the label fits the record's version byte.
- Merge rule (`Gen4LegalityHook.cpp:62-75`): verified moves downgrade the older "audited pool"
  Warnings to Info; analyzer issues are appended; coverage becomes Complete only when the older
  checks raised nothing and the analyzer's own verdict is NoProblemsFound.
- Pal Park (`Gen4LegalityPalPark.cpp`): Gen III-origin PK4s are judged through the Gen III
  templates retained after transfer. Colosseum/XD origins (version 15): `findVerifiedEncounter`
  calls `findCxdEncounter` (`Gen4Legality.cpp:516, 628`), which asks the judge passed to
  `analyzePk4`. Production passes `&Gen4::judgeCxdOrigin` (`Gen4Legality.cpp:1785-1790`,
  `Gen4LegalityPalPark.cpp:1033`), which runs `Gen3Cxd::judge` with the original level bounded by
  the met level and the ability bit only when the Gen III species has two abilities. NoMatch ->
  Invalid; Undecided -> Warning (Incomplete); Matched -> the template's moves, ball and level are
  checked like any other encounter. A null judge (tests only) leaves the record Incomplete.
- Known reader limitation (Main Development, not engine): `Pokemon4ReadOnly.cpp:58` hides a Bad
  Egg, so the analyzer sees a zeroed record and returns Incomplete without a reason.

## 7. PK5 (Black, White, Black 2, White 2): engine API only

`Legality::analyzePk5(record, Pk5Input)` (`src/Legality/Gen5Legality.cpp:2159`) is complete as an
engine but has NO caller in this branch's app.

- Gate: 136 or 220 bytes; all-zero returns an empty report; decrypt when `input.encrypted`;
  `Gen5::read` failure -> structure None; checksum mismatch -> Invalid; sanity flag -> Invalid.
- Gen III/IV-origin records are routed through `Gen4::analyzeOrigin5` (a PK4 view of the record,
  `Gen5Legality.cpp:1655` `analyzeOrigin`). Colosseum/XD origins go the same way: `analyzeOrigin5`
  runs `judgeCxdOrigin` (`Gen4Legality.cpp:1771`) and analyzes only a Matched or NoMatch answer, so
  an undecided judge leaves the Gen V record Incomplete. The male-OT Colosseum/XD rule applies only
  when the origin was not analyzed (`Gen5Legality.cpp:1716`).
- Wiring plan: #307's Gen V read-only panel (`src/UI/Gen5SharedReadOnlySurface.inc`, Main
  Development) must call the adapter Lane C adds (`include/Legality/Gen5AppAdapter.h`) with the raw
  136/220-byte record and the exact game id; the adapter renders `verdict()`, never `ok()`.

## 8. Summary table

| Format | App call | Analyzer entry | Format gate | Can reach NoProblemsFound | Blocked families today |
|---|---|---|---|---|---|
| PK1 | yes | `GB::checkGen1` | native PK1 | yes | unrecorded GB events, glitches, Korean edge cases (Incomplete) |
| PK2 | yes | `GB::checkGen2` | native PK2 | yes | same, plus Day Care slot context |
| PK3 | yes | `Gen3Pk3::verify` + `applyGen3Analyzer` | FRLG group, 80/100 bytes | yes (Colosseum/XD included) | e-Reader all-zero IVs (Incomplete), English XD TID/SID with no seed (Warning), Enigma Berry without save context, PKHeX "Fishy" RNG cases |
| PK4 | yes | `applyGen4Analyzer` -> `analyzePk4` | native PK4, 136/236 bytes | yes (Pal Park Colosseum/XD included) | undecided Colosseum/XD judge answers, reader-hidden Bad Egg |
| PK5 | **no** | `analyzePk5` (+ `Gen5App::analyze` adapter) | 136/220 bytes | yes (engine) | no app caller until #307 integration |

The invalid-site inventory gate (`tools/audit_legality_l5_invalid_site_gate.py`) lists every place
`Severity::Invalid` is raised in `Legality.cpp`, so a new Invalid cannot appear outside an audited
site.
