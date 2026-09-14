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
