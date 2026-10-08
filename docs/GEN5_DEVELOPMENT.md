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

### Work still not enabled in the application

- Provider-root discovery and physical source assignment to profile/game cards
- Emulator container adapters beyond normalized 0x80000 NDS battery images
- Gen V Product Home / Games browsing and trainer-name presentation
- Shared View/Create/Edit, app-owned staged SAV transactions, native Gen V writes
- Save-copy recency resolution without explicit copy selection
- Exact-head CI, genuine-format fixtures and owner hardware acceptance

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
- Normalized 0x80000 battery image only. Primary at 0, adjacent backup at
  0x24000 for BW or 0x26000 for B2W2 (never the Gen IV 0x40000 offset).
- Unique valid copy or identical dual valid copies can be auto-selected.
  Two different valid copies require explicit verified selection; freshness
  is NOT inferred. Conflicting exact game identities are rejected.
- Declared party, individual box PK5 reads, trainer summary and basic Dex
  caught/seen counts. Malformed declared party is refused.
- Synthetic tests independently cover all four exact game identities,
  malformed records, checksum corruption and backup-only recovery.

**Not yet a user-facing Gen V feature:** source discovery, provider assignment,
Product UI routing and a staged editor remain disabled. This reader only accepts
an already-normalized NDS battery image and cannot write any physical save.

Source-backed save geometry: Project Pokémon BW / B2W2 Save Structure
(evandixon, 2017) and PKHeX SaveUtil (0x24000 / 0x26000 core sizes).
Source preference remains API-only until a safe UI flow exists.

### Phase B tranche 1 — provider-neutral read-only adapter

- `Gen5SaveInstanceAdapter.h` builds the shared `SaveInstance` metadata
  from an already-validated, immutable in-memory Gen V source.
- Exact title, provider label, path and assignment are explicit caller inputs.
  Unsupported provenance, cross-title mismatches, and conflicting save
  copies remain non-ready; no provider or title is inferred from filenames.
- This is an integration **boundary**, not live source scanning or a UI route.
- No direct file access, injected save bytes, or mutable source privileges.

### Phase C tranche 0 — isolated PK5 field transactions

- App-owned staged record only: nature, friendship, six native IVs and EVs.
- Hard bounds: nature 0–24, friendship 0–255, IV 0–31 each,
  EV 0–255 each and 510 total.
- Each accepted change: local decrypted candidate → native checksum refresh →
  encryption → strict PK5 reparse → decoded byte-for-byte verification.
- Invalid input leaves the staged buffer unchanged. Rollback restores original
  bytes. Both boxed and party PK5 records are exercised by synthetic tests.
- No native SAV slot is modified, no source/emulator file is opened or written.
- Ability, form, shiny PID strategy, move compatibility, source writes,
  party-stat recalculation, Create UI and full Save Instance integration remain
  unsupported until their respective source-backed contracts are ready.

### Phase B tranche 2 — exact title catalog placeholders

- Black, White, Black 2 and White 2 are individually registered as Gen V
  Nintendo DS `Planned` descriptors (not `ReadOnly` or writable).
- Their Game selection can use the existing Unova region backdrop asset.
  No missing cover art is linked or substituted with a fabricated asset.
- Normal source open, assignment, shared editor and launch permissions remain
  disabled until dedicated read-only source discovery and Product UI gates pass.

### Phase A tranche 3 — nested PK5 integrity diagnostics

- Declared party entries still fail save opening if encrypted PK5 validation
  fails, even when the containing save block's CRC has been refreshed.
- Box entries are counted as occupied, empty, or quarantined-invalid.
  Invalid stored PK5 records expose no semantic fields; they are not
  silently repaired and no source bytes are changed.
- Save Instance diagnostics report the count of quarantined boxed records.
  Synthetic tests independently corrupt PK5 payloads and recompute outer
  save-block checksums to verify this nested integrity boundary.

### Phase B tranche 3 — bounded emulator source discovery (backend only)

- A dedicated Gen V reader and provider-neutral scanner inspect documented
  **RetroArch, DraStic and melonDS** roots with bounded depth and file count.
- Only raw exact `0x80000` battery images and exact DeSmuME-compatible
  `.dsv` with a verified 40-byte footer are accepted. Other wrappers,
  missing files and `.dss` savestates fail closed; no byte-offset guessing.
- No SD-card root scan, symlink traversal, file writes, or assigned-game
  inference from filenames. Physical-file deduplication uses device/inode.
- A candidate's exact title comes from the validated save. Reopening
  requires a fresh parser pass, a strict checksum and content fingerprint
  check, source identity and metadata equivalence.
- The scanner is not yet connected to the Games menu or automatic profile
  assignment; historical selected-source semantics remain unchanged.
- Host tests use disposable synthetic files and verify exact-title probes,
  safe reopening, `.dsv` validity, savestate quarantine, bounded scans,
  symlink refusal and changed-file rejection. Real emulator fixtures
  and physical hardware testing are still pending.

### Phase B tranche 3b — explicit save-copy selection survives reopen

- Ambiguous dual-valid save copies remain non-ready by default.
- An explicit verified Primary/Backup selection is tracked in the
  Save Instance source label and enforced at revalidation/reopen.
- Cross-copy substitution is rejected even if the outer file fingerprint
  and on-disk metadata are otherwise identical.
- This is an API contract, not permission to write or a visible UI toggle.
