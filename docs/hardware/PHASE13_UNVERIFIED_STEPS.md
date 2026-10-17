# Phase 13 — device steps for the six NOT VERIFIED items

These six items were **not verified** in round 1. They are not failures. Each step below is
**host-verified / NOT device-verified**: the host check named under it passes in CI, but only the owner's
run on the round-2 candidate NRO counts as device evidence. Use the input trace from
[RETEST_7fea1797.md](RETEST_7fea1797.md) §0 for anything that misbehaves.

Report each as PASS / FAIL / NOT VERIFIED, with the game and save used.

## B04 — Gen IV/V Add Item

1. Open a Diamond/Pearl/Platinum or HeartGold/SoulSilver save, then a Black/White or Black 2/White 2 save.
2. Bag → pick a pocket with free space → Add Item → choose a stackable item (e.g. Potion).
3. Expect: the item appears in the pocket with quantity 1 *before* any quantity prompt; A on that row
   edits the quantity, B leaves it at 1.
4. Repeat with a unique item (a Key Item): it is added with quantity 1 and offers no quantity editor.
5. Review Pending Changes lists the addition; Discard removes it; the source save is unchanged.

Host check: Lane I `tests/test_bag_add_flow_gen15.cpp` (pending) and QA CG12 (pending).

## E03 — Origin & Legality wording

1. In each generation (one save per gen I–V), open a Pokémon → Actions.
2. Expect the row label **Origin & Legality**, the same heading on the modal and on the right-hand
   provenance pane, and no "Encounter legality", "Legality & Provenance" or "Deep encounter legality"
   anywhere.

Host check: CG03 (enforced; PASS on 7fea1797, audit run 38947904299).

## E11 — Shiny vs non-shiny stat rerolls

1. Gen I/II: edit a non-shiny Pokémon, press **Y** several times: DVs change immediately, no confirm,
   the Pokémon stays non-shiny. Repeat on a shiny one: it stays shiny (Gen II), and Gen II gender and the
   Unown letter follow the DVs.
2. Gen III–V: same on a non-shiny and a shiny Pokémon: IVs change immediately; shininess, gender, nature
   and (Gen III) Unown letter are preserved.
3. Discard; the source save is unchanged.

Host check: CG06 (enforced) and Lane R `tests/test_reroll_mechanics_gen15.cpp` (pending).

## L06 — Party / summary-panel taps

1. In a Gen I–III save, tap a party slot; then tap the summary panel of a selected box Pokémon.
2. Expect the generation's own sheet/View to open directly, with no flash of a generic View.
3. In Gen IV/V do the same; the shared editor's View opens.

Host check: CG11 (enforced: no generic action sheet / details modal opens in a Gen I–V session) and
2R's phase 13 test (pending).

## S04 — Gen V Review / Discard / Export

1. Black/White save: edit a Pokémon (e.g. nickname), add a Bag item, change Trainer money.
2. Review Pending Changes: all three edits are listed.
3. Discard All → confirm with **Y**: the list is empty and the values are back.
4. Make the edits again → Export Copy: a new file appears under `sdmc:/switch/PokeBank-NX/exports/`;
   the source save's size and timestamp are unchanged.

Host check: export contracts C1–C5 for Gen V PK5/Bag/Trainer (`tests/test_qa_export_contracts.cpp`) and
Lane S `tests/test_gen5_review_discard_export_trace.cpp` (pending).

## S06 — Exported copy round-trip in an emulator

1. Take the exported copy from S04 (and one Gen I–IV export).
2. Load it in the emulator as a separate save (never overwrite the original): the edits are present and
   the game reports no corrupted save.
3. Load the original save: unchanged.

Host check: export contract C2 (the published edited save strictly re-parses as the same exact game).
Device only otherwise: there is no emulator in CI.
