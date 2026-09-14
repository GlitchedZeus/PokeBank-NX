# Gen II audit: exact native domains

## Held items

Primary reference: [PKHeX ItemStorage2 at 77dcd3a7895bceaafbbff12d25bdf77c1acd8ca5](https://github.com/kwsch/PKHeX/blob/77dcd3a7895bceaafbbff12d25bdf77c1acd8ca5/PKHeX.Core/Items/ItemStorage2.cs).
`GetAllHeld()` explicitly combines General, Balls, and the first 50 Machine entries (TMs).
The committed numeric domain in `Gen2HeldItems.h` contains exactly those 192 IDs plus a separate None choice.
This domain is common to G/S/C; Crystal-only key items remain excluded, as do HMs, key/story items, placeholders and unused IDs.
The project item-name table supplies labels only; a name no longer grants selectability.
Backend Create and explicit held-item changes use the same domain as the picker.
An unrelated edit preserves an existing unusual byte; View identifies it as Raw item N.
This is a held-item selection domain, not proof of complete encounter legality.

## Exact capabilities

The new resolver checks game ID, platform, generation and current record/save format together and retains region/family/revision. Unknown or mismatched identities return no capabilities. Generation defaults remain for established callers; exact identities win in the shared box summary and Gen II caught-data presentation. This model does not enable modern editors or writes.

- RBY uses single Special, DVs 0–15 and Stat Exp. GSC uses split battle Special with one stored Special DV/Stat Exp; only Crystal exposes caught/met fields.
- [PB7 at the same pin](https://github.com/kwsch/PKHeX/blob/77dcd3a7895bceaafbbff12d25bdf77c1acd8ca5/PKHeX.Core/PKM/PB7.cs) stores six AV bytes and adds them in its stat calculation. LGPE is represented as IV + AV (0–200), with no generic EV presentation.
- [PA8 at the same pin](https://github.com/kwsch/PKHeX/blob/77dcd3a7895bceaafbbff12d25bdf77c1acd8ca5/PKHeX.Core/PKM/PA8.cs) uses Ganbaru values and its own stat calculation. PLA is represented by an explicit effort-level model (0–10), not generic Gen VIII EV controls. These are presentation capabilities; legacy/unused bytes in the formats are not claims of game-supported controls.
- SV has its distinct PK9 identity with modern IV/EV presentation. Future format/revision-specific exceptions belong in the exact resolver, not species or broad-generation inference.
