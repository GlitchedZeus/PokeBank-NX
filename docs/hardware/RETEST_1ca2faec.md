# Hardware retest (round 5): PokeBank-NX-Gen1-5-Candidate-1ca2faec

**Status: NOT DEVICE TESTED.**
- This build is not hardware accepted until the owner tests this exact file on a Switch.
- Only the owner records acceptance. Host and CI results never fill in a hardware PASS.
- The identity values below come from the QA-F9 exact-binary audit.

| Identity | Value |
| --- | --- |
| NRO file | `PokeBank-NX-Gen1-5-Candidate-1ca2faec.nro` |
| Code SHA | `1ca2faec55ee64702eac6e0779daed3a1c8b3377` |
| Tree SHA | `d33207faf1c4ca52e6be99c827237d3dfa20964a` |
| Build run | https://github.com/GlitchedZeus/PokeBank-NX/actions/runs/40151550783 (run `40151550783`) |
| Artifact | `PokeBank-NX-Gen1-5-Candidate-1ca2faec` (id `12582906901`) |
| NRO SHA-256 | `b8966cdbd108530ae1c056a07e318d2f05d0cdee30d910b361ab29f381e41511` |
| NRO size | 176799409 bytes |
| QA-F9 audit run / job | https://github.com/GlitchedZeus/PokeBank-NX/actions/runs/40152597969 (job `121047076478`): `NRO AUDIT: PASS`; record artifact `app-qa-nro-audit-1ca2faec` (id `12583053077`) |

QA-F9 ran on the binding commit `ade0df37` of #307, which changes only `docs/qa/CANDIDATE_UNDER_AUDIT.env`. It checked:
- the run and artifact against GitHub's own records, and the staleness guard: the candidate is built from the
  head under test, with no non-QA file changed since `1ca2faec`;
- `BUILD_IDENTITY.txt`, `SHA256SUMS.txt` and the NRO bytes all agree: size 176799409, SHA-256 `b8966cdb…41511`;
- the embedded commit is `1ca2faec` (POKEVAULT_GIT_COMMIT), the tree is `d33207fa`;
- the NACP is `PokeBank NX 0.1.0-alpha`;
- the packaged RomFS has 4267 files (digest `bc09c152ac4232a2`);
- the cross-generation contracts in **candidate mode** on the candidate's own source (585 files) report
  `0 contract(s) failing`: all 72 pass, CG61–CG68 included, with no XFAIL left;
- hardware status: **NOT DEVICE TESTED**.

Only this artifact is the round-5 candidate. It supersedes Codex's `8804f8f8` checklist candidate. Later
docs-only commits on #307 (this file included) also trigger the candidate workflow; those artifacts are
**not** the candidate.

**Before testing**
1. Check that the downloaded file's SHA-256 matches the value above.
2. For any input problem, use the input trace from [RETEST_7fea1797.md](RETEST_7fea1797.md) §0:
   - create `sdmc:/switch/PokeBank-NX/config/input_trace.enable`;
   - reproduce the problem;
   - send `sdmc:/switch/PokeBank-NX/logs/input-trace.log`.
3. Never overwrite an installed or emulator source save. Every edit exports to a separate verified copy.

**How to record results**
- Mark each box PASS / FAIL / NOT TESTED, with the game and save used and notes for anything not PASS.
- The **State** column says what changed since your Oct 24 test of `c60d4da0`:
  - **verified Codex**: the lane audited Codex's Oct 24 work against your report, found it already
    correct on the Switch paths, and changed no code;
  - **fixed round 5**: the lane found a real gap and changed code in round 5 (commit named).
- Baseline: [OCT24_REPAIRS_AND_RETEST.md](OCT24_REPAIRS_AND_RETEST.md) (27 PASS / 6 FAIL / 4 unmarked), and the
  earlier sheets [ROUND1_RESULTS_7fea1797.md](ROUND1_RESULTS_7fea1797.md), [ROUND2_RESULTS_e10f307f.md](ROUND2_RESULTS_e10f307f.md),
  [ROUND3_RESULTS_62a2f8c3.md](ROUND3_RESULTS_62a2f8c3.md). S06 stays NOT TESTED.

Host status on `1ca2faec` (coordinator and QA runs):
- `tests/test_hw_round1_regression_gate.py`: PASS, 236 guards. It guards the 70 round-3 PASS items and the 27
  Oct 24 PASS items on every PR. Mutations (`tests/test_qa_hw_round1_gate_mutations.py`): PASS.
- `tests/test_qa_cross_gen_contract.py --candidate`: 0 failing, no XFAIL left. Round-5 contracts CG61–CG68 are
  enforced, with negative controls in `tests/test_qa_round5_contract_mutations.py`.

## Group A: your 27 Oct 24 PASS items (regression check)

These passed on `c60d4da0`. Confirm each still holds. Items marked **fixed round 5** were touched again on
your requests, so check the new behaviour as well.

| Result | ID | Accepted behaviour (and round-5 change) | State | Host guard |
| --- | --- | --- | --- | --- |
| [ ] | E03 | Origin & Legality wording, one truthful panel | verified Codex | CG03, CG48, CG49 |
| [ ] | L06 | Party / summary tap opens Actions; Gen IV Pokérus Details tap acts like A (6A) | verified Codex (S, 6A) | `test_party_summary_tap_routing`, CG33 |
| [ ] | R2-TRAINER | Gen I Trainer Name / Money reach Review. Round 5: Gen II–IV Play Time and Gender are staged edits (P3), see Group C | fixed round 5 (S `f775db01`) | CG30, CG64 |
| [ ] | C07 | Backup deletion stays protected (Y confirm). Round 5: Switch backups open as a staged copy, see Group B | fixed round 5 (L `1eec6f18`) | CG59, CG60, CG61 |
| [ ] | M04 | Gen IV move Category spacing | verified Codex | CG52 |
| [ ] | B08 | Export result is visible. Round 5: Review from the Bag / exit menu is the shared paged Review (P1) | fixed round 5 (C `4e1532d3`, `9f034431`) | CG58, CG65–CG68 |
| [ ] | E02 | Pokémon Actions present and in order; Party Actions keep View / Edit / Origin & Legality / Cancel | verified Codex | CG33, `test_classic_party_review_path` |
| [ ] | E05 | Footer glyphs, Help closes on B. Round 5: Help names only controls that act there (4D) | fixed round 5 (P `edff856e`) | CG36, CG29, CG63 |
| [ ] | R2-BUG-002 | Gen III/IV Trainer Review and staged exit. Round 5: Review closes back to the screen it came from (P1) | fixed round 5 (C `a7244a64`) | CG30, CG31, CG67 |
| [ ] | R2-CHECK-011 | Black 2 location choices context-backed; event years named, exact dates unverified (6B) | verified Codex (P, 6B) | CG53, `test_gen45_encounter_context`, `test_event_year_source` |
| [ ] | NEW-02 | No Touch:Back footer; touch Back kept | verified Codex | CG37 |
| [ ] | NEW-03 | Bag Remove / Key Item warning glyphs; A commits, B cancels | verified Codex | CG38 |
| [ ] | NEW-04 | Gen II PC Move completes | verified Codex | CG56, `test_gen2_box_move_presentation` |
| [ ] | NEW-05 | Species keyboard beside the list | verified Codex | CG39 |
| [ ] | NEW-06 | Held Item search with Sort; docked keyboard (4C) | verified Codex (P, 4C) | CG40 |
| [ ] | NEW-07 | Crystal OT Gender cycles inline | verified Codex | CG41 |
| [ ] | NEW-08 | Ball picker scrollbar gutter | verified Codex | CG42 |
| [ ] | NEW-09 | Nature colours: raised stat green, lowered red (stat rows and Nature picker) | verified Codex | CG43 (every theme), `test_stats_label_neutral_contract`, source pin |
| [ ] | NEW-11 | Selected species reads name first | verified Codex | CG45 |
| [ ] | NEW-12 | Gen IV/V location choices context-backed | verified Codex | CG53, `test_gen45_encounter_context` |
| [ ] | NEW-13 | Ability picker names and full effects (4B) | verified Codex (P, 4B) | CG46 |
| [ ] | NEW-15 | Create defaults IVs / DVs 0 | verified Codex | CG55 |
| [ ] | NEW-16 | Gen V Home A opens the assigned save | verified Codex | CG57 |
| [ ] | NEW-17 | Gen V Key Items / TMs editable | verified Codex | CG34 |
| [ ] | NEW-18 | Classic Trainer Play Time and Pokédex shown. Round 5: Gen II–IV Play Time editable (P3) | fixed round 5 (S `f775db01`) | CG47, CG64 |
| [ ] | NEW-19 | Gen I–IV staged exit: A Review, B Continue, Y Discard & Exit, X Export Copy. Round 5: Review from it returns to the screen underneath | fixed round 5 (C `a7244a64`) | CG31, CG67 |
| [ ] | NEW-20 | Gen I–III Party Actions popup | verified Codex | CG33 |

## Group B: your 6 Oct 24 FAILs and 4 unmarked items (retest on device)

| Result | ID | What should happen now | State | Host evidence |
| --- | --- | --- | --- | --- |
| [ ] | R2-SPECIAL | Gen II: one Special DV and one Stat Exp; separate calculated SpA and SpD in the Calculated column | verified Codex (R, P5) | CG51, `test_classic_stat_field_state` |
| [ ] | R2-REVIEWMOVE | Gen III Create: Pikachu with a Safari caught Ball and Keep; Safari / Hyper Potion as Held Item; named values in Review; export and reopen (FRLG and one RSE title) | Codex `7130bff3`, coordinator 5B. **Owner decision open:** this build allows Safari Ball and Berry Juice as Gen III *held* items (PKHeX `GetAllHeld`), which PKHeX legality flags as unreleased held items. Report whether to keep that | `test_gen3_staged_pokemon_editor` (5 titles × 12 balls × 5 held items) |
| [ ] | M05 | Gen V Moves: Pikachu gets its own pool, not 559 moves. Learnable / Transfer / Prior form / Preserved labels; Wormadam, Rotom, B2W2 Kyurem forms | verified Codex (R, P5) | `test_gen5_move_compatibility` |
| [ ] | S05 | Original saves never change: open, edit, export and discard, then reopen the original | fixed round 5 (L `1eec6f18`: Switch backups are opened through a copy) | CG61, matrix `immutability` cell (20 games) |
| [ ] | NEW-10 | Met Location window (620 px) shows every row in full, with no "…". Row 1 is the stored location (accent colour) | fixed round 5 (P `d361d86f`) | CG62, `test_met_location_fit` (2,025,016 Gen IV/V rows + 5,339 Gen III rows measured) |
| [ ] | NEW-14 | Gen V Language: A and a completed tap cycle it inline; it stays staged and shows as a named Review change | verified Codex (R, P5) | CG54 |
| [ ] | R2-EMUBACKUP (unmarked) | Backups list is full screen, newest first; A opens the selected copy. Switch backups now open as an isolated staged copy ("Opened from backup – date"), never the backup folder itself; the backup stays byte-identical | emulator backups verified Codex; Switch backups fixed round 5 (L `1eec6f18`) | CG59, CG61, `test_emulator_backup_access` |
| [ ] | R2-REROLL (unmarked) | Gen II Unown: chooser portrait, kept Pokémon and DV-derived letter agree; Y rerolls keep shiny / gender / form | verified Codex (R, P5; docs fix to REROLL_MECHANICS) | CG50, CG06, `test_reroll_mechanics_gen15` |
| [ ] | C01 (unmarked) | A selects and advances normally; assigned Gen V save opens directly | verified Codex | round-1 C01 guards, CG57 |
| [ ] | AUD-01 (unmarked) | One shared Review in Gen I–V: same pages, names, before/after values. In Gen I/II the Bag's Review is now the shared paged Review; a vertical swipe turns one page and a tap selects nothing | Gen V verified Codex (S, P1); Gen I–IV fixed round 5 (C `4e1532d3`, `9f034431`) | CG35, CG65–CG68, `test_gen12_review_parity`, `test_classic_party_review_path` |

## Group C: new owner requests in round 5 (check on device)

| Result | Request | What should happen now | State | Host evidence |
| --- | --- | --- | --- | --- |
| [ ] | P3 Trainer | Gen II–IV Trainer page: Play Time is an editable row (0:00:00–999:59:59). Gender is editable in Crystal and Gen III–IV only when the save stores Male / Female. No Gender row in Gen I or Gold / Silver. Each change is staged, appears in Review, is restored by Discard and is written only to the exported copy | fixed round 5 (S `f775db01`) | CG64 (+ 6 negative controls), byte-exact tests in `test_gen4_staged_pokemon_editor`, `test_gen3_trainer_edit`, `test_gsc_readonly_bridge`, `test_trainer_playtime_gender_rows` |
| [ ] | P1 Review return | Review opened from the staged-exit menu, a Trainer card or the Bag closes back to that screen, never to a Box Actions sheet for an old slot. Box Actions → Review Changes still returns to Actions | fixed round 5 (C `a7244a64`) | CG67 |
| [ ] | P1 Gen I Review | Gen I Review shows only the shared panel and its Discard All confirmation (X Export Copy, Y Discard All, B Back), with no old panel or footer on top | fixed round 5 (C `8f0e1210`) | CG65 |
| [ ] | 4D Help | Gen I–IV Help no longer claims "+ Review" (+ opens Product Home there). It names the real Review route (Trainer page Y, or B for the staged exit menu); "Moving a Pokémon: Y places it, B cancels" | fixed round 5 (P `edff856e`) | CG63, `test_classic_help_accuracy` |

## Group D: still NOT TESTED

| Result | ID | Item | Host evidence |
| --- | --- | --- | --- |
| [ ] | S06 | An exported edited save loads in an emulator as a separate save; the original is unchanged | Matrix `export` and `export.checksum=detects-corruption` cells in all 20 games; export contracts C1–C6. This is internal consistency, not emulator acceptance |

Report PASS / FAIL / NOT TESTED by ID with the exact NRO SHA-256 above and any reproduction steps.
Every NRO is **NOT DEVICE TESTED** until the owner tests that exact file.
