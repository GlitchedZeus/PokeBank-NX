# Gen VII native saves (Sun / Moon / Ultra Sun / Ultra Moon)

Thread 3 lane, branch `claude/gen7-native-20261014`, built on the Gen VI lane
(`claude/gen6-native-20261014` @ `aee7e91`). New files only, plus one
`include Makefile.host.gen7` line in `Makefile.host`. Let's Go Pikachu/Eevee is a separate
save family and is not handled here.

## What exists

| Layer | Files | Status |
|---|---|---|
| PK7 record | reuses `Gen6PK6.h` (identical Decrypt67/Encrypt67 + checksum; every accessor offset used matches PKHeX `PK7.cs`) | done |
| SAV7 identification: size 0x6BE00 / 0x6CC00, BEEF footer, all block CRC16Invert (block 36 computed with the signature region cleared), MyStatus version (SN 30, MN 31, US 32, UM 33) | `Gen7SaveLayout.h`, `Gen7Save.h/.cpp` | done; tables and CRC differential vs PKSM-Core |
| MemeCrypto signature verification: RSA verify + payload must start with SHA256(checksum table) | `Gen7Save::verifySignature` (vendored FlagBrew/memecrypto `bd3b16ee`) | done |
| Trainer (read-only), Party, 32×30 Boxes, quarantined invalid boxed records | `Gen7Save` | done |
| Shared-editor provider | `Gen7ExactFormatEditorProvider.h`: per Box/Party field states, as in Gen VI. IV/EV/Nature/Level are editable on party records too, with hyper-training-aware stat recomputation (`Gen7Stats.h`, base stats generated from PKHeX `personal_uu` by `docs/gen7/tools/gen_gen7_personal.py`, checked against PKSM-Core `personal7`). No field is a UI editor target while `ExportSupported` is false | done |
| Sources | `Gen7SaveSources.h/.cpp`: same verified Checkpoint/Dekopon layouts as Gen VI (title-ID generic); Azahar unsupported | done |
| Staged Edit/Clone/Remove | `Gen7StagedSession.h/.cpp`: app-owned copy, unsigned candidate (records written, owning block CRCs refreshed, signature untouched), independent whitelist verifier | done up to the signing step |
| Export | **explicitly unsupported** (see below) | blocked on owner evidence |

## Why SAV7 export is unsupported

Writing a working copy means re-signing block 36 with MemeCrypto. The two pinned
references build the 0x80-byte signed payload differently:

- **PKHeX** `6501f0ab` `MemeCrypto.SignInPlace`: SHA256 of the checksum table
  followed by a **zero** tail. The region is cleared on load by `ClearMemeCrypto`.
- **PKSM-Core** `aa22d7a4` `SavSUMO/SavUSUM::resign`: SHA256 followed by
  `reverseCrypt()` of the cleared region, which gives a **non-zero** tail.

Both verify with the public key, so the parser accepts either. The project rule
forbids guessing which one the game expects, so no SAV7 write path exists yet.
`Gen7Save` reports `diagnostics().signatureTail` (Zero / NonZero) and the Save
Instance diagnostic says "payload tail zero/nonzero".

**Owner check:** load one untouched SM or USUM save made by the game itself. Through
Checkpoint or Dekopon is fine. Report the probe diagnostic's payload tail. A
game-written signature settles which construction to implement. The staged session
can then reuse the Gen VI design (record mask whitelist + affected block CRCs +
re-signed block 36).

## Vendor defects found (FlagBrew/memecrypto @ bd3b16ee, PKSM-Core nested submodule)

1. `sha1.c:203` left-shifts `0x80` into the sign bit of an `int` (UBSan `shift-base`).
   Only that one check is exempted, and only for the vendored memecrypto objects, in
   `Makefile.host.gen7`.
2. `rsa.c` never `mpz_clear()`s its temporaries, so each RSA call leaks about 1.5 KB
   (it also happens on device for every Gen VII parse). The LSan suppression in
   `tests/test_gen7_save.cpp` is scoped to `rsa_encrypt`/`rsa_decrypt` only.

Fixing either needs a submodule bump, which is owner-approved vendor work.

## Percentages

| Area | Software | Automated verification | Hardware acceptance |
|---|---|---|---|
| PK7 format (read) | 100% | 100% | 0%: NOT DEVICE TESTED |
| SAV7 strict read (identity, CRC, signature, Trainer/Party/Boxes) | 90% | 90% | 0% |
| Staged edit up to signing | 90% | 90% | 0% |
| Export | 0% (blocked as above) | n/a | 0% |
| Editor provider / sources | 70% (read-only; registration pending Thread 2) | 70% | 0% |

## Integration requests (Thread 2)

As in `docs/gen6/integration-requests/`:

- `Platform::Nintendo3DS`.
- A `SaveFormat::PK7` and SM/USUM families.
- The ids `sun_3ds`, `moon_3ds`, `ultrasun_3ds` and `ultramoon_3ds`.
- A Gen 7 branch of `fieldAccessForGeneration` returning `Gen7EditorProvider::fieldState`.
- The native `Makefile` adds `src/Integration/Gen7` plus the memecrypto C sources.
  The base `Makefile.host.base` already compiles them for PKSM-Core.
