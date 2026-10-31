# G4-01 — shared PK4 and strict read-only Gen IV foundation

Implementation issue: [#82](https://github.com/GlitchedZeus/PokeBank-NX/issues/82).
Frozen contract: [closed audit #80](https://github.com/GlitchedZeus/PokeBank-NX/issues/80).
The closed Gen I–III audit (#81) is not reopened by this tranche.

Original tranche start: `5b62e2baf6521b305a38fb5b2113793a9d2e43d4`, tree
`3900cc96935effe124d61d76b86e6d6a8de5b344`.
This continuation fetched and preserved the existing implementation at
`3fe75ba770efcd56978807973040501a74a8bc26`, tree
`e242cbf20f816c578b2bb3e7079714ab22bac5f3`.
Final application/tree identities and exact-head CI evidence belong in #82 after the final push.

## Existing game-first architecture and integration

| Responsibility | Existing implementation | G4-01 integration |
|---|---|---|
| Stable game/tile identity | `Games/GameIdentity.{h,cpp}`, `GameDescriptor` registry | Five DS IDs: `diamond_nds`, `pearl_nds`, `platinum_nds`, `heartgold_nds`, `soulsilver_nds`. All remain **Planned**, without visible menu support. |
| Covers | `Games::gameCardArtworkPath`, `UI::SystemIcons::gameCardIcon` cache, `SaveSelectScreen::draw` | Existing RomFS cover loading preserved. No DS cover assets or browser-first replacement introduced. |
| Profile/game selection | `UI::SaveSelectScreen`; installed-game enumeration is separate from read-only legacy catalog entries | Existing cards remain the entry point. Gen IV has a backend open-by-profile/game function, not a new screen. |
| Gen I–III discovery | `RetroArchFRLGDiscovery` aggregates the separate strict RBY/GSC/Gen III adapters, configured/conventional roots and bounded discovery | Unchanged. No whole-SD DS scan or assumption of one emulator folder. |
| Source ownership | `LegacySourceBindings`, at `Paths::legacySourceBindingsFile()` under PokeBank's config directory | Same database, same explicit profile ownership, same safe temp/backup persistence. Existing two-column and three-column rows still load. |
| Existing path identity | Legacy discovery hashes `retroarch:` + normalized path; catalog instance contains actual path and source identity | Old rows cannot reconstruct a path without discovery; this limitation is documented rather than claiming the old database already stored it. |
| Legacy opening | `SaveSelectScreen` game card → source instance; refresh at selection, exact instance resolution → `UIManager` read-only trainer adapter | Preserved. No Gen I–III navigation or write-policy change. |
| Missing legacy sources | Strict re-resolution fails; rescanning can remove a vanished catalog entry | Preserved. Gen IV backend adds an explicit Missing result suitable for a future missing/reassign tile state. |
| Persistence across restart | `UIManager` loads bindings; catalog discovery rebuilds visible assigned legacy cards | New optional file assignment records persist path, source type, expected family, profile and exact external game ID in this same database. |

`BindingRecord` optional fields are serialized as six hex-encoded columns:
source identity, profile, game ID, path, source type, expected raw family.
Source type is descriptive metadata (`manual`, `DraStic`, `melonDS`, etc.), not
an emulator-specific path rule. Validation/existence status is refreshed by the
read-only resolver, not trusted from stale disk metadata. Old ownership-only
rows remain valid. Assignment metadata never goes inside a Pokémon save.

`LegacySourceBindings::resolveFileForGame(profile, game)` looks only at explicit
bindings. Zero matches is Unassigned; multiple matches is Ambiguous, even when
one path is missing. A missing file remains Missing and never selects a nearby
save. `Gen4::openAssignedSource` opens the exact assigned file with `rb`, reads a
bounded 0x80000-byte payload, detects its actual validated layout, then compares
assignment provenance. Raw DP identity stays DP; external Diamond or Pearl is
retained separately. HG/SS ROMCode and Platinum layout mismatches are explicit.
No mismatch silently renames the game or edits the binding. A replacement file
at the same path is revalidated; this tranche does not promise content-hash
identity binding to a particular trainer/playthrough.

## Format implementation

One `Pokemon4ReadOnly` and one `Encryption4` serve all three policies.
Stored records are 0x88 bytes; party records are 0xEC. Body XOR uses the checksum,
tail XOR uses PID, and all 32 `(PID >> 13) & 31` selectors use four 0x20 blocks.
Encryption refreshes Add16 over decrypted 0x08..0x87 on a **copy**. Parsing never
calls encryption or repairs a checksum. Invalid size/checksum/sanity records
retain diagnostic bytes but return neutral semantic fields and are never
reported as valid empty Pokémon. Callers must check `valid()`.

The save object owns an unchanged source copy and exposes const spans. Its occupied
party view and count are separate from all six native party slots. All 540 boxed
slots remain positional; invalid entities are inspectable/quarantined rather than
compacted. Native HP, status, cached stats, all remaining tail bytes, ribbon fields,
text trash and reserved bytes survive parsing. The current-box index is range
checked; bad box indices return an invalid immutable record (native builds disable
exceptions).

| Policy | General | Storage start / size | CRC exclusion | Box geometry |
|---|---:|---:|---:|---|
| DP | 0xC100 | 0xC100 / 0x121E0 | 0x14 | +4, 18 × 30 × 0x88 packed |
| Pt | 0xCF2C | 0xCF2C / 0x121E4 | 0x14 | +4, packed |
| HGSS | 0xF628 | 0xF700 / 0x12310 | 0x10 | 0x1000 stride; each 0x10 padding preserved |

All saves must be exactly 0x80000 bytes. General and Storage candidates are
independent across the two 0x40000 partitions. Every candidate checks bounds,
declared size, SDK magic (0x20060623 or Korean 0x20070903), and CRC16-CCITT.
Counter comparison matches pinned PKHeX, including FFFFFFFF and FFFFFFFE edge
behavior; equal ordinary majors use minor as tiebreak, full ordinary ties prefer
partition zero. Counter offset is always -0x14, including HGSS.

A valid older block is exposed with `recoveredOlderCopy` when the nominal newest
is invalid. This is **read-only recovery metadata**, not a claim about retail game
fallback. Neither required block having a valid candidate fails the parse.
Trainer language comes from +0x19; HG/SS exact identity comes from +0x1C. DP bytes
never claim Diamond versus Pearl. Extra blocks, unused gaps, wallpaper and padding
remain untouched in the source image. Extra-block checksums are **not evaluated**
in this tranche, and are not prerequisites for trainer/party/box parsing.

## Text, personal data and references

- PKSE v1.2.0: `55039848bbeeda114484614a9ed2e1d29804dc47` (AGPL-3.0).
- PKSM-Core: submodule `aa22d7a4f87c0351baf7da5962ba5acd01039a7c`.
- Generated text/personal resource pin: existing `tools/pkhex_source.py` default
  `6501f0ab46e8f8ca048539dbaf8cae8cb104e722`.
- Frozen audit behavior cross-check: PKHeX
  `09e7f18fbb33635e35cf9ffcbfd3322403780f8e`, `SAV4BlockDetection`, `SAV4`,
  `SAV4Sinnoh`, `SAV4HGSS`, `StringConverter4`, `StringConverter4Util`, `PokeCrypto`.

`tools/gen_gen4text.py` is adapted from pinned PKSE; `tools/gen_gen4_personal.py`
is restricted to DP/Pt/HGSS. Regeneration reproduced all imported numeric data.
The three personal resources have 501/508/508 rows. Container group selects the
personal/form table, independently of an entity's origin version. Existing
location-name infrastructure is reused; no duplicate location table was added.
Move/item domains, learnsets, evolution, legality and encounters are deferred.

Gen IV is a 16-bit code page, not UTF-16. Japanese/international and Korean tables
are generated. The language-aware codec applies PKHeX gender-width and apostrophe
encoding rules; unmapped decoded codes stop safely. Zero and FFFF terminate.
Encoding is a utility for fixtures/future work, not a save editor. Its returned
buffer uses clear-zero semantics; a future PK4 editor must implement the frozen
no-preclear entity-string policy before exposing text mutation.

## Permanent evidence

`tests/test_gen4_readonly_foundation.cpp`, registered for normal and ASan/UBSan
runs in `Makefile.host.gen4`, covers:

- direct differential encryption against pinned PKSM-Core templates for stored
  and party records, all 32 selectors, asymmetric body/tail data;
- canonical blank records pinned in `tests/fixtures/gen4/*.hex`, independent of
  the PokeBank encryptor; stale-checksum refresh and invalid-checksum/sanity quarantine;
- party HP 0/1/7, status, cached stats, trash/reserved bytes, packed fields, ribbons,
  locations/balls, form table differences and exact round trips;
- every General/Storage partition combination for each layout, all candidate
  corruption/fallback cases, major/minor/tie/uninitialized/rollover cases;
- CRC fixture stamping through **PKSM-Core**, independently of the validator;
- every layout's language, trainer, six native party slots, last box/slot geometry,
  Korean magic, HG/SS identity, raw DP ambiguity and assignment provenance;
- source SHA unchanged for successful and failed parser paths; extra/padding bytes
  are included in the full-image digest; invalid total sizes and wrong layouts;
- binding restart, immutable source files, missing path with another save present,
  ambiguous explicit bindings, profile isolation and exact-game mismatches.

The fixtures are deterministic synthetic format fixtures plus pinned upstream
oracles, **not user/retail save captures**. The full accepted legacy and durability
suite also runs. Local and final exact-head CI results are recorded in #82.

## Safety and next tranche

No Gen IV staged/source write API, UI exposure, conversion, inventory mutation,
Pokédex/extra-block mutation, True Move, N06 promotion, Master Vault or BDSP
transaction changes. `routeEnabledForTrueMove()` stays false. README is unchanged.
PR #77 stays untouched at `996e6aa40c96e4408282f3d55476dae8e64968b2`;
PR #79 stays open/draft/unmerged. FAT32 8/8 physical acceptance belongs only to
`cf1e390f9f9cba13c64c6f500df0bf3ff5060048`.

Recommended next tranche: **G4-02 — game-first read-only DS source/menu integration**.
Use the existing cover renderer and assignment database; add five separate game
entries, setup/reassignment/missing-source UI, bounded known-emulator discovery
and manual fallback, then route the validated read model into the shared View.
Qualify emulator wrappers (notably DeSmuME/other .dsv formats) separately before
accepting them; G4-01 accepts exact raw payloads only, regardless of filename.
No further standalone parser tranche is required before that integration, but
its setup and device-routing work must pass before visible Gen IV support is
advertised. Inventory/legality and staged editing remain later, separate gates.

A normal native CI NRO may be generated, but the backend is not device-routed.
No new physical test is requested for unreachable functionality and no artifact
from this tranche is DEVICE ACCEPTED.
