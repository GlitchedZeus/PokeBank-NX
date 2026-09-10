# PokeBank NX — Authoritative Next Codex Prompt

Use MEDIUM reasoning.

## CURRENT STOP STATE — WAIT FOR FINAL PHYSICAL RSE RETEST

Do **not** continue roadmap development until the user supplies physical Ruby/Sapphire/Emerald results for the exact corrected artifact below.

This is not a recovery task and not an RSE implementation task. Do not regenerate assets, redownload sprites, redo FRLG, redo RSE, or start another generation merely because a new session began.

Live installed-game and RetroArch save writing remains **HARD DISABLED**.

## FRLG accepted

FRLG read-only browsing/source assignment is physically accepted.

```text
FRLG acceptance-record checkpoint:
8172ebd9c067bd69df63815dbe865207f905eac6

Accepted FRLG runtime application source:
d78b76503f02ae26309855970fc5ce0b35c12bcb

FireRed GBA: DEVICE TESTED = YES
FireRed GBA: DEVICE ACCEPTED = YES
LeafGreen GBA: DEVICE TESTED = YES
LeafGreen GBA: DEVICE ACCEPTED = YES
```

Do not reopen FRLG without new device evidence of a defect.

## Original RSE physical test — completed, not accepted

The user physically tested exactly:

```text
PokeBank-NX-RSE-Retest-46e0c161.nro
application source: 46e0c1617fb642f45c0cd1d4b07a9bcc89c01909
```

Physical PASS for Ruby, Sapphire and Emerald:

- save discovery and open;
- trainer/read model;
- Pokémon, party and boxes;
- general browsing;
- normal read-only RetroArch battery-save handling.

The user also continued playing Emerald, saved normally in-game, and PokeBank NX continued reading the updated real save correctly.

Physical FAIL on that artifact for all three games:

- Items screen empty;
- Ruby/Sapphire/Emerald game-card artwork missing.

Known real Emerald evidence:

```text
trainer money: 2100
inventory: Potion x3
```

Therefore:

```text
DEVICE TESTED FOR RUBY = YES
DEVICE TESTED FOR SAPPHIRE = YES
DEVICE TESTED FOR EMERALD = YES
DEVICE TESTED FOR RSE = YES
DEVICE ACCEPTED FOR RSE = NO
```

## Corrected RSE application source — frozen for final retest

Exact corrected application source:

```text
1c96df2543cba339cec3dc88e20f8c6ca4fe82bb
tests: expect RSE game-card artwork
application tree: 49870ef3bdf92c4f8434597bc83de22dc6e76e27
```

This checkpoint contains the strict read-only RSE inventory fix and Ruby/Sapphire/Emerald artwork integration.

Verified inventory behavior:

```text
Ruby inventory: PASS
Sapphire inventory: PASS
Emerald inventory: PASS
Emerald keyed bag quantities: PASS
PC item plaintext quantities: PASS
Potion x3-style host fixture: PASS
invalid item/count rejection: PASS
source non-mutation: PASS
FRLG regression: PASS
ASan/UBSan: PASS
```

Do not hardcode the user's Potion x3 evidence; production parsing already decodes it generically.

## Permanent recovery snapshot — current

Use the evolved permanent snapshot:

```text
2321fa488668e32392de25afed84e38919fbd21f
recovery: snapshot RSE game-card artwork
```

Verified contents:

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
embedded/expected RomFS: 3286/3286
```

Do not use the superseded 3283-file snapshot for the current RSE final retest. Do not redownload or regenerate the sprite library.

## Exact corrected RSE physical-retest artifact

Successful build/verification run:

```text
GitHub Actions run: 34446696858
focused host/safety tests: PASS
ASan/UBSan: PASS
native devkitA64 full compile + final link: PASS
embedded application identity: PASS
embedded RomFS: PASS 3286/3286
artifact upload: PASS
```

Preserved Actions artifact:

```text
name: RSE-Final-Retest-1c96df25
artifact id: 10140106102
Actions artifact digest: sha256:9e5e4c4d5ce1e65ab5148c66a4ee83db7cfffb9244ba930e0c1ce94bfc2318fd
```

Exact files:

```text
PokeBank-NX-RSE-Final-Retest-1c96df25.nro
bytes: 158120837
SHA-256: d525a8bfac881e313d2fce1c154e26f0a893a907dc2a0be44f8f604c91416d07

PokeBank-NX-RSE-Final-Retest-1c96df25.zip
bytes: 151193359
SHA-256: 7bda62d924d1a0e094fceaaaeb35c88e697029b323ede17b4306f2a4dbf6ab74

PokeBank-NX-RSE-Final-Retest-1c96df25.nro.manifest.txt
SHA-256: 655dfd477bd8d2344ff7b929afcfa05fef6a7a091881f30a78d562cacbaf27b4

BUILD_MANIFEST.json
SHA-256: 8c6705072803126ad447fae411de75144edaf764f7caf7731d228e84b71c403b
```

## What to do when the user returns with final RSE retest results

First verify they tested the exact NRO filename/hash above. Record observations for Ruby, Sapphire and Emerald, especially:

- Items screen now shows real inventory correctly, including the user's Emerald Potion x3 where still present;
- Ruby/Sapphire/Emerald game-card artwork renders;
- save opening/trainer/Pokémon/party/boxes/general browsing remain healthy;
- normal in-game RetroArch saving remains unaffected by this read-only app.

If all requested behavior passes, record `DEVICE ACCEPTED FOR RSE = YES`. If something fails, diagnose only that observed defect from exact application source `1c96df2543cba339cec3dc88e20f8c6ca4fe82bb`.

Do not infer device acceptance from host/native tests alone.

## Out of scope while final RSE acceptance is pending

Do not start:

- Gen I or Gen II;
- DS or 3DS;
- modern Switch expansion;
- Vault/Banks;
- transfers;
- legality/editor/Create Pokémon;
- events/gifts;
- RetroArch per-Switch-user save patch;
- live save writing.

## STOP

Stop and wait for the user's final physical test of `PokeBank-NX-RSE-Final-Retest-1c96df25.nro`.
