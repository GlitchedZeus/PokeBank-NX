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

### Phase C tranche 1 — party/box slot-scoped staged workspace

- Validated Gen V SAV5 is copied into an app-owned, immutable baseline.
- Only occupied, individually validated party or box PK5 records may be
  staged. Native slot bounds and declared party count are enforced.
- Multiple slot edits are tracked by exact Party/Box location; a review
  returns before/after encrypted PK5 records for each changed slot.
- Out-of-range/empty slots, malformed records, invalid field values and
  510-EV-total overflow fail without changing any prior staged edit.
- Discard All drops all staged records, revealing the unmodified baseline.
- Crucially, review records are NOT serialized to a SAV, injected into an
  emulator, or written to the source filesystem.

### Phase C tranche 2 — exact Gen V shared-editor descriptor

- The existing exact-format capability model recognizes native PK5 and
  the separately identified BW versus B2W2 game families.
- Gen V inherits the one shared editor's geometry and source safety
  contract; **no separate Gen V screen** or new navigation is introduced.
- Only nature, friendship, IV and EV are designated staged-editable
  when the validated workspace provider is present; all other fields
  are read-only/hidden until their native mutation rules are audited.
- Native moves remain `Unsupported` for compatibility selection, never
  inferred by generation or legality-engine guesses.
- No Create, direct source write or Gen V game launch/open route enabled;
  all four title descriptors remain `Planned`.

### Phase B tranche 4 — explicit profile/game assigned-source bridge

- `Gen5AssignedSource` reads the same persistent `LegacySourceBindings`
  store used by Gen I–IV. No metadata writes occur during source open.
- Only exact Black/White/Black 2/White 2 assignments can open, with
  explicit profile, physical source identity, provider and BW/B2W2
  family supplied by a validated assignment.
- Unassigned, missing, ambiguous, unreadable or wrong-family entries
  produce distinct non-ready results. No filesystem scan or trainer-name
  inference replaces a missing binding.
- Opening invokes the strict Gen V source inspector, then a fresh
  fingerprint-verified reopen. The native save remains immutable.
- Host fixture tests exercise persisted and reloaded assignments,
  profile isolation, wrong families, and changed source invalidation.
- This adapter is not yet wired to the current production Games menu,
  and it cannot grant original-source write/inject capability.

### Phase B tranche 5 — exact-game, profile-scoped Save Instances catalog

- `Gen5GameSourceCatalog` produces provider-neutral rows only from strictly
  discovered, read-only Gen V sources matching the selected exact title.
- Physical deduplication occurs **before** persisted profile claims are
  applied, preventing an unassigned alias of another profile's physical
  file from leaking into the list.
- Unassigned validated sources can appear as explicit assignable choices.
  A source assigned to another profile is hidden, not silently reassigned.
- Existing sort-newest/recency and remembered-preference semantics are
  reused from the Gen I–IV SaveInstance model.
- The catalog is an API/view-model foundation; it is not connected to
  the Games screen and cannot open an assigned source on its own.

### Phase C tranche 3 — shared View/Edit draft + explicit Keep

- `Gen5SharedPokemonSession` uses the same shared editor exit guard as
  accepted Gen IV, not a separate generation-specific UI.
- View cannot edit. Edit opens an app-owned PK5 draft for one verified
  occupied Party/Box slot; navigation and back never commit implicitly.
- Back presents the existing edit confirmation semantics. Discard cancels
  the draft without losing separately accepted staged workspace changes.
- Keep rechecks original slot identity and builds a **copy** of the staged
  workspace, replays only audited Nature/Friendship/IV/EV changes, then
  byte-compares the complete encrypted PK5 result against the draft.
- EV decreases are replayed before increases to avoid transient 510-total
  overflow. A failure or concurrent slot change leaves the workspace
  entirely unchanged.
- No Create capability, SAV writer, original-file write, installed
  application data modification, injection or automatic legality fix.

### Phase B tranche 6 — validated Gen V trainer display text

- Trainer names come only from the selected and fully validated Gen V
  Trainer block. Maximum seven native UTF-16 code units.
- Strict display conversion rejects malformed surrogate pairs, private-use
  placeholders, control characters and noncharacters; unsupported text
  leaves the trainer label empty without changing save acceptance.
- Valid Japanese/Unicode names are converted into UTF-8 and surfaced
  in source details and provider-neutral Save Instances rows.
- No source text writing, nicknames, trainer editing or game assignment
  inference has been enabled.

### Phase B tranche 7 — assigned game card preview contract

- A read-only `Gen5GameCardPreview` consumes **only** an explicitly assigned,
  currently validated source from `Gen5AssignedSource`; an unassigned,
  missing or inconsistent result yields no preview.
- Exposes selected exact game, provider identity, validated trainer name,
  gender (only when the native value is known), Pokédex seen/caught totals,
  and the six party slots as bounded immutable species/item/nature/shiny data.
- Data is native Gen V, with no invented species names, ROM cover art,
  level fields, source mutations, save injection or implicit profile claims.
- The production Games screen is unchanged; this is a source-backed bridge
  for integrating the existing UI without creating another one.

### Phase B tranche 8 — native Gen V party level in preview

- The shared Game Card preview now includes level directly from the native PK5
  party extension at offset `0x8C` (`PKHeX.Core/PKM/PK5.cs`, `Stat_Level`).
- Boxed PK5 records do **not** expose a synthetic level. Out-of-range party
  level data is shown as unavailable (`0`) rather than repaired or guessed.
- No native save mutation, source write, or level-edit capability is enabled.

### Phase B tranche 9 — physical replacement guard for assigned Gen V saves

- New Gen V-only source IDs bind the normalized path to the underlying
  device/inode when provided by the filesystem. If inode data is unavailable,
  the already-verified normalized battery SHA-256 fingerprint is used instead.
- A normal in-place update retaining the inode preserves an existing exact
  game/profile binding, while swapping in a different physical file at the
  same path forces an explicit reassignment.
- Source identity is distinct from displayed provider, title and user labels;
  the Gen I–IV binding database format and source identity algorithms are
  unchanged. This is a Gen V development-only identity-format change, and
  existing exploratory Gen V assignments may need to be selected again.
- An actual Switch SD/filesystem compatibility check remains required before
  Game Sources is enabled for Gen V.

### Phase B tranche 10 — prevent broad source-root traversal

- Reject SD-card and filesystem roots (sdmc:/, /), dot and parent-path
  traversal from configured or explicitly supplied Gen V source roots.
- A hostile or misconfigured RetroArch savefile_directory never causes the
  Gen V scanner to enumerate the SD root. Valid bounded emulator directories
  continue to use the existing depth and file-count limits.
- Focused tests reject /, sdmc:/, .. traversal and config pointing at
  / without scanning any candidate files. No Gen I–IV scanner changed.


### Phase D tranche 1 — Product Games / Save Instances preview-only wiring

- Gen V Black/White/Black 2/White 2 now have dedicated Game cards in the
  existing Product Games browser. Their immutable trainer, Dex and party
  preview is populated only by an explicitly assigned, strictly revalidated
  source using Gen5AssignedSource and Gen5GameCardPreview.
- A on a Gen V card opens the existing-style Save Instances chooser and
  source-setup modal; sources are bounded to Gen V's known emulator roots or
  explicitly supplied manual paths. Exact title/provenance/profile identity
  is checked again before accepting a new binding.
- The selected save is associated with the current profile through the
  already established metadata-only LegacySourceBindings transaction.
  Assign/forget changes metadata, never original emulator SAV bytes.
- Gen V remains SourceSupport::Planned in GameIdentity. There is no
  fallback into the Gen IV editor, native save write/Inject path, Items
  editor, or unvalidated game launcher. The UI explicitly says preview only.
- This early Product UI integration needs an exact-head native build and
  owner hardware confirmation before being considered accepted; the full
  Gen V editor/view implementation is a separate follow-up milestone.
