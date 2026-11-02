# PokeBank NX — Round 12 independent hardware evidence intake

**Source:** owner-uploaded `PokeBank-NX-Gen1-5-Candidate-066e1db9-results.json` (24-item custom HTML checklist), 16 owner screenshots.
**Candidate:** `PokeBank-NX-Gen1-5-Candidate-066e1db9`
**Source CI run:** https://github.com/GlitchedZeus/PokeBank-NX/actions/runs/41273304655
**Source JSON export timestamp:** 2026-11-02T01:47:43.012Z
**Audit status:** Intake only — not a code fix, not acceptance, not integrated into GitHub. No original save has been inspected.

## Summary

- **20 PASS, 4 FAIL, 0 skipped** in the *uploaded custom 24-item checklist*.
- Explicit FAIL: **H03** Gen V move/control parity + carry popups; **H04** clone carry and Gen I/II cursor; **H08** Crystal met-location options; **H14** Gen III locations and Top Up.
- **PASS with unresolved notes:** **H12** all TM/HMs request; **H16** Gen V controls/footer; **H19** Gen III JA/KO + suspicious PID/stat changes and Gen V glyphs. **Do not count notes as verified repairs.**
- The custom checklist H01–H24 **is not identical to the repository's official Round 12 H01–H18**. In particular, custom H14 covers Gen III inventory and locations; official H14 covers Gen V names. Never merge/resolve tracker items by H-number alone. Use the item's title plus source name and stable `R12-*` ID.
- Screenshots show potential encounter provenance/level mismatches and stale export toasts; these are additional investigation leads, **not proven safety failures**.
- Priority P0 is **investigate Gen III language/stat/PID effect before any export of affected records**, not a proven source-save corruption.
- Round 12 excludes Master Vault and legality engine by design. Their v1.0 feature obligations **must remain tracked**. Do not claim these were fixed.

## Stable findings (retain in every subsequent round)

| ID | Priority | Status | Owner evidence | Required outcome |
| --- | --- | --- | --- | --- |
| R12-001 | P1 | Open defect | H03,H16 | Gen V Boxes Y/Review vs Move controls |
| R12-002 | P2 | Open UX defect | H03,H04 | Routine move/clone toast and hint clutter |
| R12-003 | P1 | Open defect | H04 | Gen I–II packed-box clone shows second cursor / wrong destination |
| R12-004 | P1 | Open design/correctness defect | H08,H14 | Met-location list mixes legitimate catches with unverified fallback |
| R12-005 | P1 | Requires investigation | H08,H14 | Shown met-level / current-level contradictions |
| R12-006 | P1 | Open defect or capacity UX | H14,H12 | Top Up/Add Missing appears limited to ~20 items |
| R12-007 | P1 | Requested enhancement with safety review | H12 | All eligible TM/HM choices in inventory management |
| R12-008 | P0 | Requires investigation | H19 | Gen III language may reset PID and computed stats |
| R12-009 | P1 | Open localization gap | H19 | Gen III and Gen V Japanese/Korean name glyph display inconsistent |
| R12-010 | P2 | Requires investigation | Screenshot-only | Stale export confirmation overlays unrelated active editor/modal |
| R12-011 | P1 | Missing release feature | Previously recorded Round 11 | Move to Vault / Deposit / Withdraw absent from release flow |
| R12-012 | P1 | Missing release feature | Previously recorded Round 11 | Vault Pokémon search missing |

## Detailed findings and reproductions

### R12-001 — Gen V Boxes Y/Review vs Move controls (P1)

**Classification:** Open defect
**Source:** H03,H16
**Details:** Gen V Y currently invokes Review rather than Move, unlike requested Gen I–IV control convention. Move, Review, Help, footer must share canonical model across generations.
**Reproduce:** Gen V Black/Black 2 → Boxes; press Y on occupied Pokémon; check Review shortcut and carry semantics.
**Evidence qualification:** Owner reported; not directly photographed
**Required acceptance:** Gen V: Y Move/pickup as supported; Review on same key as classic gens (verify +); A place; B cancel; footer/Help accurate; no collision.
**Screenshot files:** 2026110120152900_c.jpg

### R12-002 — Routine move/clone toast and hint clutter (P2)

**Classification:** Open UX defect
**Source:** H03,H04
**Details:** Owner reports popup on pickup, placement, multi-move and clone completion across Gen I–V. Remove redundant success/hint overlays when footer already communicates controls. Do not remove capacity errors, source-change warnings, destructive confirmation, export results or failure diagnostics.
**Reproduce:** Move and clone in Boxes using Gen I–V; capture transient overlays.
**Evidence qualification:** Owner reported; not pictured
**Required acceptance:** No redundant success/toast on ordinary carry/placement. Preserve meaningful errors and destructive confirmations.
**Screenshot files:** None (owner written result only)

### R12-003 — Gen I–II packed-box clone shows second cursor / wrong destination (P1)

**Classification:** Open defect
**Source:** H04
**Details:** Owner reports clone auto-targeting next box while a second cursor remains independently navigable. Validate selected destination and packed append semantics; no extra cursor overlay.
**Reproduce:** Gen I or II Boxes → occupied slot → Clone; navigate boxes; inspect selected destination and focus; place, cancel.
**Evidence qualification:** Owner reported; not pictured
**Required acceptance:** One coherent focus/carry indicator; clone appends to selected box if capacity, never silently shifts to another box; cancel restores state.
**Screenshot files:** None (owner written result only)

### R12-004 — Met-location list mixes legitimate catches with unverified fallback (P1)

**Classification:** Open design/correctness defect
**Source:** H08,H14
**Details:** Crystal and Gen III Ruby show many locations marked "Not a listed catch; unverified" (Ruby Pikachu 87 rows), including locations unrelated to the species; user wants appropriate choices for each species/game and event, egg, transfer, evolved histories. The prior fallback was intentional; revise policy, do not silently equate unknown with impossible.
**Reproduce:** Crystal new Arbok → Met Location; Ruby new Pikachu → Met Location; confirm default list and Other histories.
**Evidence qualification:** Owner report + screenshots
**Required acceptance:** Default lists verified, game/form/time/level-relevant routes; separate advanced unverified/manual provenance clearly. Unsupported evidence remains Incomplete, never claimed legal.
**Screenshot files:** 2026110120052600_c.jpg;2026110114150600_c.jpg;2026110114151100_c.jpg;2026110114151500_c.jpg;2026110114151700_c.jpg

### R12-005 — Shown met-level / current-level contradictions (P1)

**Classification:** Requires investigation
**Source:** H08,H14
**Details:** Screenshots show Black 2 Nidorino Lv5 with offered Hidden Grotto Lv55–60 ancestry and Diamond Pidgeotto Lv5 with Route229 wild Lv51 via Pidgey; these are incompatible if the record claims that wild encounter path. Verify source choice, met-level semantics, evolution, trade and transfers before asserting legality.
**Reproduce:** Gen V Black 2 Lv5 Nidorino → choose met location; Gen IV Diamond Lv5 Pidgeotto; stage and Review selected encounter.
**Evidence qualification:** Visual observation; not proven accepted/exported
**Required acceptance:** Candidate cannot be represented as verified wild encounter if current level below the encounter level; flag or refuse with reason; valid trade/event paths handled separately.
**Screenshot files:** 2026110114261500_c.jpg;2026110114223000_c.jpg;2026110114225300_c.jpg

### R12-006 — Top Up/Add Missing appears limited to ~20 items (P1)

**Classification:** Open defect or capacity UX
**Source:** H14,H12
**Details:** Owner reports only ~20 entries selected in Gen III scope and asks entire pocket/category including PC. Gen I bag has shared fixed physical capacity; distinguish native 20-slot limit from truncation, test every generation.
**Reproduce:** On disposable save for Gen I–V choose This Pocket/All Bag/PC/Bag+PC and Top Up/Add Missing; compare available vs selected counts, review capacity/overflow.
**Evidence qualification:** Owner reported; review screenshot shows Gen II 83 staged TM edits but not exact All Bag coverage
**Required acceptance:** No silent subset selection; exact eligible count, capacity and excluded reasons shown; enforce native slot/item restrictions and do not create impossible inventories.
**Screenshot files:** 2026110120215300_c.jpg *(corrected from 2026110120154000_c.jpg; see SCREENSHOT_INDEX.md)*

### R12-007 — All eligible TM/HM choices in inventory management (P1)

**Classification:** Requested enhancement with safety review
**Source:** H12
**Details:** Owner requests all TM/HMs rather than omitting some. Must distinguish game-valid obtainable/released item IDs and protected HMs, and separate Bag/PC capacity semantics.
**Reproduce:** Gen I–V Bag/PC → TM/HM → Add Missing; enumerate candidates and explain disabled/omitted IDs.
**Evidence qualification:** Owner request; Gen II Review screenshot supplied
**Required acceptance:** Expose every supported native-format TM/HM allowed under safe policy or give explicit refusal reason; no unsafe key item/flag mutation.
**Screenshot files:** 2026110120215300_c.jpg *(corrected from 2026110120154000_c.jpg; see SCREENSHOT_INDEX.md)*

### R12-008 — Gen III language may reset PID and computed stats (P0)

**Classification:** Requires investigation
**Source:** H19
**Details:** Two LeafGreen Box1/Slot1 Pikachu Lv5 drafts: English shows nonzero PID and calculated stats, Japanese shows PID=0, blank stats, radar zero. Owner says switching to Japanese changes stats. Screenshots alone do not prove the change was performed on the same in-memory draft without other edits.
**Reproduce:** Create one Gen III LeafGreen Pikachu; note before PID/stats/IV/EV/level; A-cycle language English→Japanese→English with no other input; inspect and byte-compare staged record; Discard; repeat on existing Pokémon.
**Evidence qualification:** Owner report + strong screenshot correlation, causal path not proven
**Required acceptance:** Changing language must only alter intended language/name data; PID and stats remain stable; never serialize damaged record; no write to source save.
**Screenshot files:** 2026110120462700_c.jpg;2026110120463000_c.jpg

### R12-009 — Gen III and Gen V Japanese/Korean name glyph display inconsistent (P1)

**Classification:** Open localization gap
**Source:** H19
**Details:** Owner reports Gen IV displays language glyphs properly; Gen III lacks JA/KO glyphs, Gen V shows no comparable language glyph change. Check font coverage, decoding, native-name sources and game-specific language support.
**Reproduce:** Open same JP/KO named specimens/drafts in Gen III, IV and V; compare text, keyboard, named species and display values.
**Evidence qualification:** Owner reported, limited screenshot evidence
**Required acceptance:** Correct glyph/font display for supported languages with no stat/data mutation; unsupported must be explicitly identified, not silent fallbacks.
**Screenshot files:** 2026110120462700_c.jpg;2026110120463000_c.jpg

### R12-010 — Stale export confirmation overlays unrelated active editor/modal (P2)

**Classification:** Requires investigation
**Source:** Screenshot-only
**Details:** In Diamond Arbok met-location modal the toast reports exported Pidgeotto; in Black 2 Arbok modal it reports exported Raichu. May simply be a prior success message lingering; verify lifetime and z-order, not assert wrong export without file evidence.
**Reproduce:** Export Pidgeotto/Raichu; immediately switch Pokémon and open Met Location, observe toast duration and placement. Inspect output files before concluding incorrect export target.
**Evidence qualification:** Screenshots
**Required acceptance:** Confirmation identifies actual export and expires before unrelated modal or stays unobtrusive and clearly scoped; export itself correct.
**Screenshot files:** 2026110114225300_c.jpg;2026110114253600_c.jpg

### R12-011 — Move to Vault / Deposit / Withdraw absent from release flow (P1)

**Classification:** Missing release feature
**Source:** Previously recorded Round 11
**Details:** Round 11 H41–H43 + Extra 7 explicitly requested Move to Vault in common Actions. Current Round 12 candidate intentionally excludes Vault, so feature remains a visible v1.0 blocker, not a new Round 12 regression.
**Reproduce:** All Gen I–V Actions; Vault backend and deposit/withdraw separately; test with real copies.
**Evidence qualification:** Prior repository engineering intake, not current screenshot evidence
**Required acceptance:** Move to Vault workflow works end-to-end for promised formats with stable persistent IDs, failure rollback, unchanged source save; acceptance on Switch.
**Screenshot files:** None (owner written result only)

### R12-012 — Vault Pokémon search missing (P1)

**Classification:** Missing release feature
**Source:** Previously recorded Round 11
**Details:** Round 11 Extra 8 requests Bank/Vault search with highlighted matches; tracked as v1.0 requested feature, outside current NRO.
**Reproduce:** Open Vault search when supported; compare exact names, forms and keyboard characters.
**Evidence qualification:** Prior repository worksheet
**Required acceptance:** Search supported and hardware-tested or explicitly owner-rescoped before release.
**Screenshot files:** None (owner written result only)

## Raw recorded test statuses

| Custom checklist ID | Result | Owner supplied note? |
|---|---|---|
| H01 | PASS | — |
| H02 | PASS | — |
| H03 | FAIL | Contains notes |
| H04 | FAIL | Contains notes |
| H05 | PASS | — |
| H06 | PASS | — |
| H07 | PASS | — |
| H08 | FAIL | Contains notes |
| H09 | PASS | — |
| H10 | PASS | — |
| H11 | PASS | — |
| H12 | PASS | Contains notes |
| H13 | PASS | — |
| H14 | FAIL | Contains notes |
| H15 | PASS | — |
| H16 | PASS | Contains notes |
| H17 | PASS | — |
| H18 | PASS | — |
| H19 | PASS | Contains notes |
| H20 | PASS | — |
| H21 | PASS | — |
| H22 | PASS | — |
| H23 | PASS | — |
| H24 | PASS | — |

All raw owner notes are preserved **verbatim** in the uploaded JSON (`source-results.json`) and must be read before implementing each issue; the descriptions above are independently paraphrased. Do not modify the raw original.

## Safety / QA requirements

1. Freeze and identify the exact NRO under test. Do not mutate the candidate while interpreting feedback.
2. Run targeted failing test *before* any fix and passing test *after*, alongside full GCC, Clang, sanitizers and native NRO build; retain exact SHAs.
3. Exercise relevant Gen I–V paths through shared editor, boxes, item management, review, staging, Discard, Export Copy and byte-identical source verification.
4. Never conflate a general "unverified" candidate with a confirmed legal wild encounter. Unknown history remains Incomplete; don't invent legality verdicts.
5. Do not loosen source save immutability, damage refusal, native capacity limits, or safety confirmation to satisfy UI requests.
6. Link each item to historical Round 1–11 requirements and the permanent feature matrix; NEVER silently omit old defects or Vault requirements in future rounds.
7. Keep the distinct v1.0 missing features visible on the release blocker list: Vault deposit, withdraw, transfer workflow, Move to Vault, and search. No unsupported placeholder should be enabled.

## Hand-off request to main Claude

Audit the current Round 12 repair head against the exact owner feedback here, fix rooted issues on a follow-up branch, attach focused regressions and explicit acceptance mapping for every ID. Report what remains unresolved and which claims still lack hardware evidence. Do not push/merge changes to the frozen Round 12 candidate or overwrite the owner JSON.
