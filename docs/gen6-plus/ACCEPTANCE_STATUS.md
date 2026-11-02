# Gen VI–IX acceptance status (truthful matrix)

Owner hardware intake of experimental `5895e6b9` (2026-11-01, evidence preserved unmodified in
[`docs/hardware/gen6-plus-5895e6b9-owner-intake/`](../hardware/gen6-plus-5895e6b9-owner-intake/)):
**NOT HARDWARE ACCEPTED. NO-GO for Gen VI–IX as a v1.0-ready editor.** `5895e6b9` is frozen as a failed
hardware candidate.

`CAPABILITY_MATRIX.md` (generated from the registry) says what the **code path** does and that **host tests**
pass. It is not acceptance. Acceptance needs four separate states, and a host test never implies 2–4:

1. **Host tested**: synthetic fixtures, adapter + real-screen host UI suites, sanitizers, CI.
2. **Owner hardware opened an authentic save**: on a real Switch, on the exact bound NRO.
3. **Feature parity**: the shared editor workflows the Gen I–V application offers, natively for that title.
4. **Exported copy validated in game**: a verified Export Copy restored by the owner and loaded by the game.

| Title | 1 Host tested | 2 Owner hardware (5895e6b9) | 3 Feature parity | 4 Export validated in game |
| --- | --- | --- | --- | --- |
| Pokémon X | Yes | NOT TESTED | Partial (no Create; party release blocked) | NOT TESTED |
| Pokémon Y | Yes | **FAIL**: the one discovered file was rejected (G69-HW-001); no save opened | Partial | NOT TESTED |
| Pokémon Omega Ruby | Yes | NOT TESTED | Partial | NOT TESTED |
| Pokémon Alpha Sapphire | Yes | **DISPUTED**: card shows trainer "Billy" (Dekopon), no party, dex 0/721; owner reports broken (G69-HW-002) | Partial | NOT TESTED |
| Pokémon Sun / Moon / Ultra Sun / Ultra Moon | Yes | NOT TESTED | Partial; **Export blocked** (SAV7 signing) | BLOCKED |
| Let's Go Pikachu | Yes | **LIMITED**: trainer facts and 72/1000 storage read; Bag and Pokédex unavailable (G69-HW-005/006) | Partial (Move blocked; Bag/Dex missing) | NOT TESTED |
| Let's Go Eevee | Yes | NOT TESTED | Partial | NOT TESTED |
| Sword / Shield | Yes | NOT TESTED | Partial (IV/EV/level/species/moves read-only) | NOT TESTED |
| Brilliant Diamond / Shining Pearl | Yes | NOT TESTED | Partial (same) | NOT TESTED |
| Legends: Arceus | Yes | NOT TESTED | Partial (same; Alpha read-only) | NOT TESTED |
| Scarlet / Violet | Yes | NOT TESTED | Partial (same) | NOT TESTED |
| Legends: Z-A | Yes | **LIMITED**: installed save opened read-only (boxes, editor views, trainer); battle teams unreadable so box changes refused (G69-HW-007); species/moves/trainer/dex read-only (G69-HW-008/009/012) | Partial | NOT TESTED |

No title is at state 2, 3 or 4. "Partial" means: the shared Party/Boxes/editor surface opens and stages the
edits listed in `CAPABILITY_MATRIX.md`; title-native species, form, move/PP, level/IV/EV (Gen VIII+),
trainer, bag and Pokédex editing are missing or read-only, and Create is blocked everywhere. The Gen I–V
common Actions menu and Vault transfer are v1.0 requirements that Gen VI–IX does not inherit yet.

## Owner issue ledger status (G69-HW-*)

| Id | Finding | Status after this change | Owner retest |
| --- | --- | --- | --- |
| 001 | Y: candidate rejected, "no BEEF" | Parser verified against pinned PKHeX (sizes, BEEF at size−0x1F0, all 55/58 block offsets identical): no layout defect found. The rejection now says what the file is (all-zero = never written by the game; empty footer; not a decrypted `main`) and names the provider and file. Root cause unconfirmed without the file. | R1 |
| 002 | Alpha Sapphire: no party, dex 0/721 | ORAS party/box/Zukan offsets verified against PKHeX (`Zukan6` caught 0x08, seen 4×0x60). No defect found; the save may genuinely be early (no party). Unconfirmed. | R2 |
| 003 | Poké Ball art on 3DS cards | Open: no licensed 3DS cover art in the build; fallback stays. | — |
| 004 | Installed 3DS title ≠ valid save | Documented: 3DS games are never installed on a Switch; the card needs an emulator/Checkpoint save file. | R1 |
| 005/006 | Let's Go Bag / Pokédex / trainer editing | Open (feature parity). | — |
| 007 | Z-A battle teams unreadable | **Fixed (likely cause)**: team blocks are read by raw bytes like PKHeX (an Int16-array / byte-value block was refused before). Still fail-closed; the Trainer screen now states the exact reason if unreadable. | R3 |
| 008/009/012 | Z-A species/moves/trainer/dex read-only | Open (needs PKHeX-pinned per-title tables). | — |
| 010 | Z-A Origin / Met location "Unknown" | Open: no Z-A location/origin name table; values are not invented. | — |
| 011 | "Unknown provider" + LIVE LOCKED + STAGED | Provider now reads "Installed game (read-only copy)". Staged changes stay in PokeBank NX; the installed save is never written. | R3 |
| 013 | Matrix overstates readiness | This file. | — |
| 014 | Titles not individually tested | Recorded as NOT TESTED above. | — |

## Specific owner retest items (next candidate only)

- **R1 (Pokémon Y):** HOME → Pokémon Y → A. Read the Save Instances message: it now names the file and says
  whether it is all-zero (the game never saved: save once in the emulator, retry), has an empty footer, or is not
  a decrypted `main`. Report that sentence.
- **R2 (Alpha Sapphire):** open the card with A, look at Trainer → play time and the party in the editor.
  Report whether the in-game save at that moment has a party / Pokédex entries.
- **R3 (Legends: Z-A):** open the installed card with A → Trainer. Report the "Battle teams" row: either
  "N box references" (fixed) or the exact "Unreadable: …" reason. The header should say "Installed game
  (read-only copy)".
