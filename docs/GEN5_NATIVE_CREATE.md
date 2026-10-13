# Isolated native Gen V Create foundation

Base: PR #300, `5745f200bbc3e442bcee92e0b3945e4eb401c260`.
Lane: `codex/gen5-native-create-engine`. No existing production file is modified.

## API and scope

Include `Integration/Gen5/Gen5NativeCreate.h`; populate `CreateRequest5` and call
`createNative5(const CreateRequest5&)`. Inspect the result's boolean conversion,
`validation.error`, and `validation.message`. Success owns exactly 136 encrypted
bytes; every failure owns zero bytes. Inputs are never changed. There is no I/O,
save pointer, source handle, overwrite permission, global RNG or clock dependency.

Required inputs are exact title, species, level, nature, gender, native ability
slot, friendship, PID, explicit trainer identity/language/name, custom nickname,
at least one move, and explicit ordinary native encounter metadata. Invalid
defaults deliberately fail; zero TID/SID/PID remain valid when explicitly supplied.
Stat order is HP/Attack/Defense/Speed/Sp.Attack/Sp.Defense. IVs default to zero;
EVs default to zero and support Gen V's 255 per stat / 510 total limits.
Moves have optional current PP: omitted initializes full native PP, explicit zero
means depleted. Empty slots have zero PP/Ups. Holes, duplicates, out-of-range IDs,
PP overflow and PP Ups for one-PP moves are rejected.

All 649 base species are supported structurally in both native personal tables.
Nonzero forms require title-specific native data. BW rejects the additional
Therian, Kyurem and Keldeo forms that B2W2 supports. Nonzero forms for species
351/421/487/492/493/555/648/649 are deliberately gated pending item/battle/reversion
handling; personal-table presence alone is not enough for this Create API.
Ability slots 0/1 must match PID bit 16; slot 2 sets the native hidden flag.
Gender must agree with the native ratio and PID low byte. Nature is independent
of PID in PK5. Supplied PID and shiny status are never rerolled.

Only printable ASCII names in language IDs 2/3/4/5/7 are currently supported.
Nickname length is 1..10, OT length 1..7; neither is truncated or transliterated.
All nicknames are explicitly custom, with the native nickname flag set. There is
no automatic localized species-name fallback. Japanese/Korean and other glyphs
fail explicitly, even where a future codec could support them.

CallerSuppliedNative history encodes the supplied exact origin title, ordinary
location ID, met level, ball and calendar date literally. Unknown, egg, event,
transfer and special-location-bank histories fail. No trainer/date/location is
inferred. The engine cannot authenticate what the caller supplies. These are
**structural checks, not encounter validation or legality claims**; moves are not
checked against learnsets and hidden abilities are not certified obtainable.
No event ribbons, egg flags, N sparkle, Pokérus or held item is invented.

Party output is explicitly gated. The reference distinguishes PK5's 220-byte
party record from PK4's 236-byte record, with status/level/stats at 0x88..0x9B,
mail at 0x9C..0xD3 and trailing data at 0xD4..0xDB. Full native initialization and
hardware behavior of that extension have not been certified by this lane.

## Regression commands

Run from the repository root, using distinct build folders for each compiler or
configuration. Clang runs against the untouched integration baseline:

```sh
make -f Makefile.host.gen5-create CXX=clang++ CREATE_BUILD=build-host/gen5-create-clang test debug sanitize
```

`test_gen5_native_create.cpp` has a test-only decoder with its own byte reads,
checksum accumulation, stream implementation and pinned block positions; it does
not use production crypto to establish the expected decoded bytes. It also uses
literal EXP oracle points, including every Erratic threshold from 69 through 98.
Assertions execute even with NDEBUG defined.

`test_gen5_create_integration.cpp` embeds generated PK5s into synthetic SAV5
fixtures, then exercises the existing session, Review, candidate export, clone
and remove paths. This is **not new-slot Create integration**. It checks exact
allowed-byte differences, untouched source/other partition, tampered review
rejection and rejection of a new PK5 falsely submitted as a field edit.

The existing integration baseline has compile/test defects; see the three
coordinator patches. Test them in a disposable checkout, not in this lane's
production tree. After applying them there, one may run the new tests from this
checkout with the patched dependency headers first on the include path:

```sh
make -f Makefile.host.gen5-create CPPFLAGS=-I/absolute/test-checkout/include CREATE_BUILD=build-host/gen5-create-patched test debug sanitize integration integration-sanitize
```

Without the patches GCC 16 debug/sanitizer compilation fails in existing
personal-data pointer assertions. The integration test also intentionally
requires correct shared Erratic level interpretation. Passing standalone Create
does not mean the unpatched shared integration is ready.

## Coordinator integration contract

1. Review/apply the baseline fixes separately. The EXP patch is required before
   enabling Create in the editor: otherwise an Erratic level-70 record appears
   as level 69. The local `createExperience5` correction prevents bad creation
   now; after coordinator adoption it can be consolidated in a later change.
2. Collect explicit request inputs and compare `request.game` with the exact
   selected source title. Do not silently synthesize identity or history.
3. Add a distinct CreateBox transaction/certificate to workspace and Review.
   Require a byte-exact canonical empty destination, immutable source snapshot,
   target/title binding and independently reproducible Create request. Do not
   reuse FieldEdit or forge CloneBox source evidence. Both are rejected today.
4. Add shared-session Create/Keep/Discard semantics and explicit Review UI.
   Cancel/failure must leave no pending replacement. Preserve the new draft's
   identity/history and show structural validity separately from legality.
5. Only after those tests pass should the coordinator expose PokemonCreation
   capability and extend the existing candidate-export whitelist for CreateBox.
   Keep backup, confirmation, stale-source and app-owned destination checks.
   Never enable Inject Save or emulator/installed-game writes.
6. Register this standalone runner in shared host/CI gates after acceptance.
   This lane deliberately does not edit the shared makefile/UI/session/export.

## Source oracle

The original source was inspected at PKHeX tree
`66ef5983a5d349efa2dd615f2926a1b1ee97306e`:

- [PK5 fields and record geometry](https://raw.githubusercontent.com/kwsch/PKHeX/66ef5983a5d349efa2dd615f2926a1b1ee97306e/PKHeX.Core/PKM/PK5.cs)
- [PokeCrypto block order and stream](https://raw.githubusercontent.com/kwsch/PKHeX/66ef5983a5d349efa2dd615f2926a1b1ee97306e/PKHeX.Core/PKM/Util/PokeCrypto.cs)
- [StringConverter5](https://raw.githubusercontent.com/kwsch/PKHeX/66ef5983a5d349efa2dd615f2926a1b1ee97306e/PKHeX.Core/PKM/Strings/StringConverter5.cs)
- [EntityGender](https://raw.githubusercontent.com/kwsch/PKHeX/66ef5983a5d349efa2dd615f2926a1b1ee97306e/PKHeX.Core/PKM/Util/EntityGender.cs)
- [Experience thresholds](https://raw.githubusercontent.com/kwsch/PKHeX/66ef5983a5d349efa2dd615f2926a1b1ee97306e/PKHeX.Core/PKM/Util/Experience.cs)

The existing pinned BW/B2W2 personal and MoveInfo5 PP tables are reused directly;
no modern-generation table or PK4 record factory substitutes for PK5. PKHeX's
.NET implementation was **not executed**; no executable PKHeX parity is claimed.
