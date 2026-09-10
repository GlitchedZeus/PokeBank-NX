# PokeBank NX — Current Verified Engineering State

Last updated: 2026-09-10

## STOP: RSE open-fix physical-retest artifact is ready

Active branch: `feature/pokebank-playable`.

Do **not** begin Gen I/II or any other roadmap work until the user physically tests the exact RSE open-fix artifact recorded below.

Live installed-game and RetroArch save writing remains **HARD DISABLED**.

## FRLG milestone — physically accepted

```text
Acceptance-record checkpoint: 8172ebd9c067bd69df63815dbe865207f905eac6
Accepted FRLG runtime source: d78b76503f02ae26309855970fc5ce0b35c12bcb
FireRed GBA: DEVICE TESTED = YES / DEVICE ACCEPTED = YES
LeafGreen GBA: DEVICE TESTED = YES / DEVICE ACCEPTED = YES
FRLG MILESTONE: PHYSICALLY ACCEPTED
```

FRLG remains read-only browsing/source assignment only.

## RSE physical history

Original physical RSE artifact:

```text
PokeBank-NX-RSE-Retest-46e0c161.nro
application source: 46e0c1617fb642f45c0cd1d4b07a9bcc89c01909
```

That artifact physically passed Ruby/Sapphire/Emerald save discovery/open, trainer/read model, Pokémon, party, boxes and general read-only browsing. It physically failed because the Items screen was empty and R/S/E game-card artwork was missing.

The user subsequently physically verified Ruby/Sapphire/Emerald artwork as **PASS**.

The later corrected artifact based on `1c96df2543cba339cec3dc88e20f8c6ca4fe82bb` exposed an RSE open regression on hardware:

```text
Ruby open = FAIL
Sapphire open = FAIL
Emerald open = FAIL
Items = UNTESTABLE
RSE artwork = PASS
```

Root cause was the stale RSE inventory guard in `FRLGReadOnlyTrainer::populate()`: once RSE inventory became non-empty, the legacy bridge rejected the trainer and returned to the save picker. The runtime bridge fix is implemented. RSE inventory-specific validation failure is now optional/nonfatal while critical Gen III save validation remains strict.

Current physical state:

```text
RSE DEVICE TESTED = YES
RSE DEVICE ACCEPTED = NO
```

## RSE open-regression runtime source

Exact runtime application source for the new physical retest:

```text
a2df4c1acdb7a556808bd58a2bdbcd4fc0335954
application tree: 559202d16f0affc13a9e1c521beff4834585b1e7
```

Tests-only checkpoint:

```text
82364e32b1b82a4313a092c7aa54f44479bfade0
```

The corrected RSE tests prove:

```text
valid Ruby inventory = PASS
valid Sapphire inventory = PASS
valid Emerald inventory = PASS
invalid Ruby item 377: save opens; inventory unavailable; trainer/party/boxes usable
impossible Sapphire count 1000: save opens; inventory unavailable; trainer/party/boxes usable
invalid keyed Emerald inventory: save opens; inventory unavailable; trainer/party/boxes usable
source bytes unchanged for optional-inventory failures
inventory failure rejects whole RSE save = NO
```

Clean permanent host verification:

```text
checkpoint: e5df0e237ba1b7ad0dd78e37a1e1aac609071c3f
GitHub Actions run: 34453208654
full host suite: PASS
focused Ruby open: PASS
focused Sapphire open: PASS
focused Emerald open: PASS
RSE inventory tests: PASS
FRLG regression: PASS
source mutation/write policy: PASS
git diff --check: PASS
ASan: PASS
UBSan: PASS
```

## Permanent recovery snapshot — unchanged

```text
2321fa488668e32392de25afed84e38919fbd21f
RomFS files: 3286
HD renders: 3260/3260
base species: 1025/1025
type icons: 18/18
fonts: 3/3
FireRed artwork: PASS
LeafGreen artwork: PASS
Ruby artwork: PASS
Sapphire artwork: PASS
Emerald artwork: PASS
```

The snapshot was restored normally for this build. It was **not repacked or modified**.

## RSE open-fix physical-retest build — PASS

The initially launched build run `34453854069` failed only because the workflow grepped for obsolete focused-test output text. All tests in that step had passed; no runtime defect was found. The workflow-only check was corrected in commit:

```text
ef8795134ec1243dd2600c33694621b0074592d1
ci: fix RSE open-fix verification output check
```

Successful exact build run:

```text
GitHub Actions run: 34454555232
host/focused/sanitizer gate: PASS
recovery snapshot restore: PASS
exact application source checkout: PASS
exact application tree verification: PASS
device asset preflight: PASS
native devkitA64 compile: PASS
native devkitA64 final link: PASS
embedded application identity: PASS
embedded RomFS: PASS 3286/3286
packaging: PASS
artifact upload: PASS
```

Preserved Actions artifact:

```text
name: RSE-OpenFix-Retest-a2df4c1a
artifact id: 10143216443
Actions artifact digest: sha256:2daecd936c60719364f5b35b66fa41500be6b5a607146fab70535cbbe5e0fec7
```

Exact physical-retest files:

```text
NRO: PokeBank-NX-RSE-OpenFix-Retest-a2df4c1a.nro
NRO bytes: 158120837
NRO SHA-256: 34fc0893ae0f0ee1a3e244c11469a5d44de181d68318040fce386470b2e0e80e

ZIP: PokeBank-NX-RSE-OpenFix-Retest-a2df4c1a.zip
ZIP bytes: 151195235
ZIP SHA-256: 847788db890523d1ebce070d1427f6230fd89a60ca14986525fd6d2b634d0788

Packaging manifest: PokeBank-NX-RSE-OpenFix-Retest-a2df4c1a.nro.manifest.txt
Packaging manifest SHA-256: 3dfa38be0fabde6f45edfb1ee52d87d2f9b1513ec89ba3fa576b0b0ed8da55f9

BUILD_MANIFEST.json SHA-256: 335c1a62b45c2b7c3f2282c424c7c2cd089a6b1178c92fec2bccb027401070ee
SHA256SUMS.txt SHA-256: 33f461dbb17fc2ddbd1ab116d682fcb9871e49e33f0c77465cbed7cff489f664
```

The Actions artifact was independently downloaded and extracted after the successful run. The NRO SHA-256 reproduced locally and matches both `BUILD_MANIFEST.json` and `SHA256SUMS.txt`. `embedded-romfs.json` independently records application source `a2df4c1acdb7a556808bd58a2bdbcd4fc0335954`, NRO size `158120837`, NRO SHA-256 `34fc0893ae0f0ee1a3e244c11469a5d44de181d68318040fce386470b2e0e80e`, and `3286/3286` RomFS verification.

## STOP condition / next action

**STOP DEVELOPMENT HERE.** The user must physically test exactly:

```text
PokeBank-NX-RSE-OpenFix-Retest-a2df4c1a.nro
SHA-256: 34fc0893ae0f0ee1a3e244c11469a5d44de181d68318040fce386470b2e0e80e
```

Verify Ruby/Sapphire/Emerald all open normally, valid inventory renders, and the already-passing artwork remains correct. Do not mark `DEVICE ACCEPTED FOR RSE = YES` until the user reports success on this exact NRO.

Do not start Gen I, Gen II, DS/3DS, Vault/Banks, transfers, editor/legality/events, RetroArch per-user changes, or live save writing while RSE acceptance is pending.

## Repository authority

```text
Repository: GlitchedZeus/PokeBank-NX
Development branch: feature/pokebank-playable
Writable remote: origin
Upstream reference: kiasta/PKSE
```

Never push custom PokeBank NX work upstream.
