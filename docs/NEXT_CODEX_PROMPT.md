# PokeBank NX — Authoritative Next Codex Prompt

Use MEDIUM reasoning.

## CURRENT STOP STATE — WAIT FOR PHYSICAL RSE TEST

Do **not** continue roadmap development until the user supplies physical Ruby/Sapphire/Emerald Switch-test results.

This is not a recovery task and not an RSE implementation task. Do not regenerate assets, redownload sprites, redo FRLG, redo RSE, or start another generation merely because a new session began.

Live save writing remains **HARD DISABLED**.

## FRLG accepted

FRLG read-only browsing/source assignment is physically accepted.

```text
FRLG acceptance-record checkpoint:
8172ebd9c067bd69df63815dbe865207f905eac6

FireRed GBA: DEVICE TESTED = YES
FireRed GBA: DEVICE ACCEPTED = YES
LeafGreen GBA: DEVICE TESTED = YES
LeafGreen GBA: DEVICE ACCEPTED = YES
```

Do not reopen FRLG without new device evidence of a defect.

## RSE application source — frozen for current device test

Exact Ruby/Sapphire/Emerald application source:

```text
46e0c1617fb642f45c0cd1d4b07a9bcc89c01909
gen3: add strict read-only Ruby Sapphire Emerald path
```

Host CI is complete:

```text
CI run 34440786353: PASS
Ruby: HOST TESTED PASS
Sapphire: HOST TESTED PASS
Emerald: HOST TESTED PASS
ASan: PASS
UBSan: PASS
```

Do not reimplement or rerun host CI unless later source changes require it.

## Recovery snapshot — complete and unchanged

```text
a2adac94f15504b90a83a295e77ad54154da4206
HD renders: 3260/3260
base species: 1025/1025
types: 18/18
fonts: 3/3
required game artwork: PASS
total RomFS: 3283/3283
```

Do not redownload or regenerate sprites. Use only the committed deterministic recovery tooling if restoration is genuinely needed later.

## Exact RSE physical-test artifact

Native build/package verification is complete for exact application source `46e0c1617fb642f45c0cd1d4b07a9bcc89c01909`.

```text
native devkitA64 full build/link: PASS
embedded RomFS: PASS 3283/3283

PokeBank-NX-RSE-Retest-46e0c161.nro
bytes: 156711849
SHA-256: 5cbf1cdd9e5b075793b6b259a647ff0b14328589b31939a4fe1f56cccc0c7d8a

PokeBank-NX-RSE-Retest-46e0c161.zip
bytes: 149787526
SHA-256: 17c63ab10089bcaff996d1c4a390d307b61ff386f6e6644ded380a812e6a9cb9

PokeBank-NX-RSE-Retest-46e0c161.nro.manifest.txt
SHA-256: 180b2c2827df236c096c760d2b278b5893987bdd2f90d3d78d1b77f57b3ad117

BUILD_MANIFEST.json
SHA-256: 0d845df83c363f94c755ef2b49c37b03f0fa412352f219182ce636f225b5f037
```

The physical-test bundle was preserved as GitHub Actions artifact `RSE-Retest-46e0c161` (artifact id `10138231150`). Required recovery, preflight, native-link, embedded-RomFS/package and artifact-upload steps passed. The Actions run is red only because an optional private prerelease-publication step failed after artifact preservation; do not treat that as an RSE application failure.

Current RSE hardware state:

```text
DEVICE TESTED FOR RUBY: NO
DEVICE TESTED FOR SAPPHIRE: NO
DEVICE TESTED FOR EMERALD: NO
DEVICE TESTED FOR RSE: NO
DEVICE ACCEPTED FOR RSE: NO
```

## What to do when the user returns with RSE test results

First verify that the user tested the exact filename/hash above. Record their physical observations in `CURRENT_STATUS.md` and the relevant GitHub issue. If all requested Ruby/Sapphire/Emerald read-only behaviors pass, record RSE physical acceptance. If something fails, diagnose only the observed RSE defect from this exact checkpoint and preserve the source/save safety rules.

Do not infer device acceptance from host/native tests alone.

## Out of scope while RSE physical testing is pending

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

Stop and wait for the user's physical Ruby/Sapphire/Emerald test. Do not create new feature work before those results.
