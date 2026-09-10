# PokeBank NX — Current Verified Engineering State

Last updated: 2026-09-10

## STOP: corrected RSE final-retest artifact is ready

Active branch: `feature/pokebank-playable`.

Ruby/Sapphire/Emerald have now been physically tested on a real Nintendo Switch using the original RSE artifact. Core read-only RSE behavior passed, but RSE is **not yet physically accepted** because that artifact exposed two UI/data-supply blockers: empty Items inventory and missing Ruby/Sapphire/Emerald game-card artwork.

Both blockers are corrected, fully host/native verified, packaged, and ready for the user's final physical RSE retest. Do not begin Gen I/II or any other roadmap work before that retest result is recorded.

Live installed-game and RetroArch save writing remains **HARD DISABLED**.

## FRLG milestone — physically accepted

Acceptance-record checkpoint:

```text
8172ebd9c067bd69df63815dbe865207f905eac6
```

```text
FireRed GBA: DEVICE TESTED = YES
FireRed GBA: DEVICE ACCEPTED = YES
LeafGreen GBA: DEVICE TESTED = YES
LeafGreen GBA: DEVICE ACCEPTED = YES
FRLG MILESTONE: PHYSICALLY ACCEPTED
```

Accepted FRLG runtime application source:

```text
d78b76503f02ae26309855970fc5ce0b35c12bcb
```

FRLG remains read-only browsing/source-assignment only. Do not reopen it without new device evidence of a defect.

## Original RSE physical test — real Switch result

The user physically tested exactly:

```text
PokeBank-NX-RSE-Retest-46e0c161.nro
application source: 46e0c1617fb642f45c0cd1d4b07a9bcc89c01909
```

Physical PASS for Ruby, Sapphire and Emerald:

- RetroArch battery save discovery;
- save open;
- trainer/read model;
- Pokémon, party and boxes;
- general browsing;
- normal read-only RetroArch save handling.

The user also continued playing Emerald after PokeBank NX had successfully read the save, saved normally in-game, and PokeBank NX continued reading the real updated save correctly. This is further physical evidence that the read-only path does not break normal game-save use.

Physical FAIL on that artifact for all three RSE games:

- Items screen was empty;
- game-card artwork was missing.

Known real Emerald evidence from the tested save:

```text
trainer money: 2100
known inventory: Potion x3
```

Current physical state:

```text
DEVICE TESTED FOR RUBY = YES
DEVICE TESTED FOR SAPPHIRE = YES
DEVICE TESTED FOR EMERALD = YES
DEVICE TESTED FOR RSE = YES
DEVICE ACCEPTED FOR RSE = NO
```

## RSE blocker fixes — verified

Original strict RSE source checkpoint:

```text
46e0c1617fb642f45c0cd1d4b07a9bcc89c01909
gen3: add strict read-only Ruby Sapphire Emerald path
```

Corrected final-retest application checkpoint:

```text
1c96df2543cba339cec3dc88e20f8c6ca4fe82bb
tests: expect RSE game-card artwork
application tree: 49870ef3bdf92c4f8434597bc83de22dc6e76e27
```

The runtime blocker fixes are included before this checkpoint. `readRSEModel()` now populates the existing read-only `InventoryPouchRecord` model consumed by the Items UI, using the RSE-specific Gen III layouts rather than FRLG offsets.

Verified inventory semantics:

```text
Ruby / Sapphire
PC Items   0x0498, 50 entries, plaintext quantity
Items      0x0560, 20 entries, plaintext quantity
Key Items  0x05B0, 20 entries, plaintext quantity
Poké Balls 0x0600, 16 entries, plaintext quantity
TM/HM      0x0640, 64 entries, plaintext quantity
Berries    0x0740, 46 entries, plaintext quantity

Emerald
PC Items   0x0498, 50 entries, plaintext quantity
Items      0x0560, 30 entries, keyed quantity
Key Items  0x05D8, 30 entries, keyed quantity
Poké Balls 0x0650, 16 entries, keyed quantity
TM/HM      0x0690, 64 entries, keyed quantity
Berries    0x0790, 46 entries, keyed quantity
```

Gen III bag entries are 4-byte little-endian item-id/quantity pairs. Ruby/Sapphire quantities are plaintext. Emerald bag quantities use the low 16 bits of Emerald's section-0 security key; PC Items remain plaintext. Invalid item IDs/impossible counts are rejected defensively, empty slots are ignored, and source bytes are never modified.

Focused RSE fixtures cover Ruby, Sapphire and Emerald non-empty inventories, all six pouch types, keyed Emerald quantities, a Potion x3-style fixture, empty slots, invalid item IDs, impossible counts, corrupt-save rejection, rotating-slot fallback, source non-mutation and retained cross-PC-sector behavior. The Potion x3 value is fixture evidence only and is not hardcoded in production parsing.

Authoritative layout behavior was cross-checked against the project's pinned PKSM-Core Gen III implementations.

Host/safety verification for the corrected application checkpoint:

```text
RSE inventory Ruby: PASS
RSE inventory Sapphire: PASS
RSE inventory Emerald: PASS
known Emerald Potion x3-style fixture: HOST TESTED PASS
FRLG Gen III regression: PASS
source mutation/write-policy checks: PASS
git diff --check: PASS
ASan: PASS
UBSan: PASS
GitHub Actions host run 34445802093: PASS
```

## RSE game-card artwork — fixed and permanent

Runtime artwork resolution now maps:

```text
ruby_gba     -> romfs:/game_cards/ruby_gba.png
sapphire_gba -> romfs:/game_cards/sapphire_gba.png
emerald_gba  -> romfs:/game_cards/emerald_gba.png
```

Tracked assets use the same pinned Libretro GBA thumbnail source as FRLG:

```text
upstream: libretro-thumbnails/Nintendo_-_Game_Boy_Advance
revision: 23b8665408e767fd3220bfb83fd5ad8dfe1a9aa1

Ruby path: Named_Boxarts/Pokemon - Ruby Version (USA).png
SHA-256: 0c264af577ca175f0d73b6766b1100dd4b78d6728bbcfb07298679500f0316e1

Sapphire path: Named_Boxarts/Pokemon - Sapphire Version (USA).png
SHA-256: 2bcea4502d475507ed7f07f3bbaf45f36ec30446c5a02519f950bea31b3154fb

Emerald path: Named_Boxarts/Pokemon - Emerald Version (USA, Europe).png
SHA-256: f39a4d3d7044d2ad693a60f6af362619f7268cd9187c6576f3d4e3373dfa49b5
```

These images are tracked, copied by the normal game-card asset pipeline, required by device preflight, captured by the permanent recovery snapshot, and verified inside the final NRO.

## Permanent recovery snapshot — evolved for RSE artwork

The old 3283-file snapshot is superseded for current builds by:

```text
2321fa488668e32392de25afed84e38919fbd21f
recovery: snapshot RSE game-card artwork
```

Authoritative restored/preflight state:

```text
HD renders: 3260/3260
base species: 1025/1025
type icons: 18/18
fonts: 3/3
FireRed artwork: PASS
LeafGreen artwork: PASS
Ruby artwork: PASS
Sapphire artwork: PASS
Emerald artwork: PASS
RomFS files: 3286
DEVICE ASSET PREFLIGHT: PASS
```

The new snapshot is committed as two GitHub parts plus manifest. Future deterministic recovery restores all five GBA game-card images. No full sprite redownload or generic recovery redo was performed.

The embedded-RomFS verifier was also corrected to derive completeness from the intended RomFS tree instead of retaining the obsolete hardcoded 3283 count; the final build pipeline independently requires the recovery manifest's derived 3286 count.

## Corrected RSE final-retest artifact — READY

Exact application source embedded in the NRO:

```text
1c96df2543cba339cec3dc88e20f8c6ca4fe82bb
```

Successful device-build run:

```text
GitHub Actions run: 34446696858
recovery restore: PASS
exact application identity: PASS
device asset preflight: PASS
focused host/safety/sanitizers: PASS
native devkitA64 full compile + final link: PASS
embedded RomFS byte-for-byte verification: PASS 3286/3286
artifact upload: PASS
```

Preserved Actions artifact:

```text
name: RSE-Final-Retest-1c96df25
artifact id: 10140106102
Actions artifact digest: sha256:9e5e4c4d5ce1e65ab5148c66a4ee83db7cfffb9244ba930e0c1ce94bfc2318fd
```

Exact final-retest files:

```text
NRO: PokeBank-NX-RSE-Final-Retest-1c96df25.nro
NRO bytes: 158120837
NRO SHA-256: d525a8bfac881e313d2fce1c154e26f0a893a907dc2a0be44f8f604c91416d07

ZIP: PokeBank-NX-RSE-Final-Retest-1c96df25.zip
ZIP bytes: 151193359
ZIP SHA-256: 7bda62d924d1a0e094fceaaaeb35c88e697029b323ede17b4306f2a4dbf6ab74

Packaging manifest: PokeBank-NX-RSE-Final-Retest-1c96df25.nro.manifest.txt
Packaging manifest SHA-256: 655dfd477bd8d2344ff7b929afcfa05fef6a7a091881f30a78d562cacbaf27b4

Build manifest: BUILD_MANIFEST.json
Build manifest SHA-256: 8c6705072803126ad447fae411de75144edaf764f7caf7731d228e84b71c403b
```

The preserved Actions artifact was independently extracted and the exact NRO/ZIP/manifest hashes above were reproduced.

## STOP condition / next action

**STOP DEVELOPMENT HERE.** The next project event is the user's final physical Switch retest of:

```text
PokeBank-NX-RSE-Final-Retest-1c96df25.nro
```

Retest the previously failing RSE Items screens and Ruby/Sapphire/Emerald card artwork while also confirming normal opening/browsing still behaves correctly.

Do **not** mark `DEVICE ACCEPTED FOR RSE = YES` until that corrected artifact passes on hardware.

Do not start Gen I, Gen II, DS, 3DS, modern Switch expansion, Vault/Banks, transfers, legality/editor/Create Pokémon, events/gifts, RetroArch per-Switch-user save changes, or live save writing while final RSE acceptance is pending.

## Repository authority

```text
Repository: GlitchedZeus/PokeBank-NX
Development branch: feature/pokebank-playable
Writable remote: origin
Upstream reference: kiasta/PKSE
```

Never push custom PokeBank NX work upstream.
