# PokeBank NX — Current Verified Engineering State

Last updated: 2026-09-10

## STOP: RSE physical-test artifact is ready

The active development branch is `feature/pokebank-playable`.

No further roadmap work is authorized until the user physically tests the exact Ruby/Sapphire/Emerald artifact recorded below.

Live installed-game and RetroArch save writing remains **HARD DISABLED**.

## FRLG milestone — physically accepted

FireRed GBA and LeafGreen GBA read-only browsing/source assignment are physically accepted on a real Nintendo Switch.

Acceptance-record checkpoint:

```text
8172ebd9c067bd69df63815dbe865207f905eac6
```

Accepted FRLG runtime application source:

```text
d78b76503f02ae26309855970fc5ce0b35c12bcb
legacy: make fsdev diagnostics link-compatible
```

Physical status:

```text
FireRed GBA: DEVICE TESTED = YES
FireRed GBA: DEVICE ACCEPTED = YES
LeafGreen GBA: DEVICE TESTED = YES
LeafGreen GBA: DEVICE ACCEPTED = YES
FRLG MILESTONE: PHYSICALLY ACCEPTED
```

The accepted FRLG test covered opening both games, source assignment, persistence across full app restart, per-PokeBank/Nintendo-profile isolation, switching back to the original profile, Refresh after normal RetroArch in-game battery saves, updated Pokémon display, healthy original saves, and additional trainer/party/box browsing.

This acceptance is **read-only browsing/source-assignment only**. It is not approval for live save writing.

## RSE application source — host verified

Ruby/Sapphire/Emerald read-only support is implemented at the exact application-source checkpoint:

```text
46e0c1617fb642f45c0cd1d4b07a9bcc89c01909
gen3: add strict read-only Ruby Sapphire Emerald path
application tree: 655ccd8fb37b6627017959213f7c3811a2f3275a
```

This source includes Ruby/Sapphire/Emerald stable identities; RetroArch `.sav`/`.srm` discovery; `.state` exclusion; strict 128 KiB Gen III rotating-sector validation; all 14 sector IDs; sector signatures and checksums; coherent save counters; newest-valid-slot selection; corrupt-newer-slot fallback; RSE-vs-FRLG family validation; R/S source identity handling; Emerald money encryption handling; trainer/party/PC-box read models; Refresh; source non-mutation; and the retained PC-sector-boundary Pokémon fixture.

Host verification is complete:

```text
GitHub Actions host run: 34440786353
Ruby source support: HOST TESTED PASS
Sapphire source support: HOST TESTED PASS
Emerald source support: HOST TESTED PASS
focused/host tests: PASS
git diff --check / whitespace: PASS
device/recovery tool syntax: PASS
GitHub recovery contract: PASS
ASan: PASS
UBSan: PASS
```

No application-source correction was required during the native artifact build. The RSE application source therefore remains exactly `46e0c1617fb642f45c0cd1d4b07a9bcc89c01909`.

## Recovery snapshot — unchanged and verified

The build used only the existing committed private GitHub RomFS recovery snapshot:

```text
a2adac94f15504b90a83a295e77ad54154da4206
```

The device pipeline explicitly verified that `recovery/assets_snapshot` had not changed relative to that checkpoint before running the existing deterministic recovery tooling.

Verified recovered/preflight assets:

```text
HD renders: 3260/3260
base species: 1025/1025
type icons: 18/18
fonts: 3/3
required FRLG game-card artwork: PASS
device asset preflight: PASS
total expected RomFS files: 3283
```

No sprites were redownloaded or regenerated.

## RSE native/device artifact — READY FOR PHYSICAL TEST

Exact application source used by the native build:

```text
46e0c1617fb642f45c0cd1d4b07a9bcc89c01909
```

Required native artifact pipeline results:

```text
clean devkitA64 full build/link: PASS
final NRO produced: PASS
embedded RomFS byte-for-byte verification: PASS
embedded RomFS files: 3283/3283
embedded application-source identity: PASS
Actions physical-test artifact upload: PASS
```

The successful packaged artifact came from device-build run `34441709355`. That workflow's required recovery, preflight, native full-link, embedded-RomFS/package, and Actions artifact-upload steps all passed. The run itself is displayed red only because the later optional private GitHub prerelease-publication step failed **after** the physical-test artifact had already been preserved; this did not alter or invalidate the NRO/ZIP and is not an RSE application-source defect.

Preserved Actions artifact:

```text
name: RSE-Retest-46e0c161
artifact id: 10138231150
Actions artifact digest: sha256:73a3a001732c1834d5ab8f8774a9ddba6a17c45cece05ca7279095188f740cf0
```

Exact physical-test files:

```text
NRO: PokeBank-NX-RSE-Retest-46e0c161.nro
NRO bytes: 156711849
NRO SHA-256: 5cbf1cdd9e5b075793b6b259a647ff0b14328589b31939a4fe1f56cccc0c7d8a

ZIP: PokeBank-NX-RSE-Retest-46e0c161.zip
ZIP bytes: 149787526
ZIP SHA-256: 17c63ab10089bcaff996d1c4a390d307b61ff386f6e6644ded380a812e6a9cb9

Packaging manifest: PokeBank-NX-RSE-Retest-46e0c161.nro.manifest.txt
Packaging manifest SHA-256: 180b2c2827df236c096c760d2b278b5893987bdd2f90d3d78d1b77f57b3ad117

Build manifest: BUILD_MANIFEST.json
Build manifest SHA-256: 0d845df83c363f94c755ef2b49c37b03f0fa412352f219182ce636f225b5f037
```

Independent extraction of the preserved Actions artifact reproduced the same NRO/ZIP sizes and SHA-256 values recorded by the build manifest.

Current device status:

```text
DEVICE TESTED FOR RUBY: NO
DEVICE TESTED FOR SAPPHIRE: NO
DEVICE TESTED FOR EMERALD: NO
DEVICE TESTED FOR RSE: NO
DEVICE ACCEPTED FOR RSE: NO
```

## STOP condition / next action

**STOP DEVELOPMENT HERE.** The next project event must be the user's physical Switch test of `PokeBank-NX-RSE-Retest-46e0c161.nro` with Ruby, Sapphire and Emerald normal RetroArch battery saves.

Do not start Gen I, Gen II, DS, 3DS, modern Switch expansion, Vault/Banks, transfers, editor/Create Pokémon, legality, events/gifts, RetroArch per-Switch-user save changes, or live writing while RSE physical acceptance is pending.

If a future session begins before physical RSE results exist, do not rebuild or reimplement RSE merely because the session changed. Preserve the exact application source and artifact identity above and wait for device evidence.

## Repository authority

```text
Repository: GlitchedZeus/PokeBank-NX
Development branch: feature/pokebank-playable
Writable remote: origin
Upstream reference: kiasta/PKSE
```

Never push custom PokeBank NX work upstream.
