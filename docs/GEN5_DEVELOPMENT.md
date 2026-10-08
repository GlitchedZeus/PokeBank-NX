# Generation V (isolated development lane)

## Checkpoint and scope

Branched from the owner hardware-accepted Product UI / PR #92 application at
`0760334fb850ca46b5171c55f690998d7680d4e7`.
This branch must remain separate from touch-controls PR #122 and legality PR #103.
No merge or hardware acceptance is implied.

### Phase A tranche 1 — PK5 entity foundation

- Read-only boxed (`0x88`) and party (`0xDC`) PK5 record decryption,
  32 shuffle values, LCG crypt, checksums, sanity and corruption quarantine.
- Safe synthetic candidate encryption is **not** authorization to modify an
  emulator save; no filesystem source write path is present.
- Strict semantic accessors for core PK5 fields. Display names, forms,
  locations and source-specific legality require later source-backed catalogs.
- Synthetic record test suite; enabled in host and sanitizer test graphs.
- No Gen V game shown as editable in the UI yet. External sources immutable.

### Explicitly not yet implemented

- BW/B2W2 exact-game recognition and save-block/checksum-table validation
- Save-slot fallback and precise BW versus B2W2 per-block layout mapping
- Party/PC/Trainer/Dex full save parsing or emulator provider integration
- Profile assignment, Product Home previews, shared editor, staged SAV writes
- Switch NRO generation, CI acceptance, physical Nintendo Switch testing

### Source references

- PKHeX `PKHeX.Core/PKM/PK5.cs` (PK5 fields, stored/party lengths)
- PKHeX `PKHeX.Core/PKM/Util/PokeCrypto.cs` (45 crypt/shuffle)
- FlagBrew PKSM-Core for independent later comparison and save geometry

These reference the *semantics*, not copied source code.
Next: independently source and pin BW/B2W2 save geometry, checksum tables,
and game-id evidence before wiring any save parser/UI path.

`DEVICE_ACCEPTED=false` for all builds from this branch.

### Phase A tranche 2 — strict read-only NDS battery save reader

- Source-backed 70-block BW and 74-block B2W2 checksum/mirror tables.
- Exact title match via trainer game byte: White=20, Black=21,
  White 2=22, Black 2=23 (PKHeX GameVersion).
- Normalized 0x80000 battery image only. Strict partitions, no guessed
  provider transforms. A unique valid partition is selected; identical valid
  partitions may be selected deterministically.
- Two valid *different* partitions fail closed until freshness evidence is
  implemented. This protects against choosing the wrong save slot.
- Declared party, individual box PK5 reads, trainer summary and basic Dex
  caught/seen counts. Malformed declared party is refused.
- Synthetic tests independently cover all four exact game identities,
  malformed records, checksum corruption and backup-only recovery.

**Not yet a user-facing Gen V feature:** source discovery, provider assignment,
Product UI routing and a staged editor remain disabled. This reader only accepts
an already-normalized NDS battery image and cannot write any physical save.

