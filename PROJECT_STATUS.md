# PokeBank NX Project Status

Last updated: 2026-09-11

For the shortest engineering handoff, read `CURRENT_STATUS.md` and `docs/NEXT_CODEX_PROMPT.md`.

## Project identity and safety

```text
Product: PokeBank NX
Version: 0.1.0-alpha
Repository: GlitchedZeus/PokeBank-NX
Development branch: feature/pokebank-playable
Writable remote: origin
Upstream reference: kiasta/PKSE
Live installed-game writing: HARD DISABLED
Live RetroArch writing: HARD DISABLED
```

`DEVICE ACCEPTED` is used only after the user physically tests the exact relevant artifact and reports the milestone working.

## Current headline

Generation I Red/Blue/Yellow and Generation III FireRed/LeafGreen/Ruby/Sapphire/Emerald are now **physically accepted for the current legacy read-only path**. Generation II Gold/Silver/Crystal is the next legacy milestone, but no GSC runtime implementation was started during this closure session.

| Game | State |
|---|---|
| Red GB | IMPLEMENTED / HOST TESTED / NRO BUILDS / DEVICE TESTED / DEVICE ACCEPTED |
| Blue GB | IMPLEMENTED / HOST TESTED / NRO BUILDS / DEVICE TESTED / DEVICE ACCEPTED |
| Yellow GB | IMPLEMENTED / HOST TESTED / NRO BUILDS / DEVICE TESTED / DEVICE ACCEPTED |
| FireRed GBA | DEVICE TESTED / DEVICE ACCEPTED |
| LeafGreen GBA | DEVICE TESTED / DEVICE ACCEPTED |
| Ruby GBA | DEVICE TESTED / DEVICE ACCEPTED |
| Sapphire GBA | DEVICE TESTED / DEVICE ACCEPTED |
| Emerald GBA | DEVICE TESTED / DEVICE ACCEPTED |
| Gold GBC | NEXT LEGACY READ-ONLY MILESTONE / NOT IMPLEMENTED |
| Silver GBC | NEXT LEGACY READ-ONLY MILESTONE / NOT IMPLEMENTED |
| Crystal GBC | NEXT LEGACY READ-ONLY MILESTONE / NOT IMPLEMENTED |

## Generation I RBY physical-test history

### 1. First physical test — defect discovered

```text
Application source: d9077e2da3909b6fbe9d8db9ce7384a71b8b98e7
Application tree: 0ea6fbe365afe8122f252754554fef1fae73e183
Artifact: PokeBank-NX-RBY-Retest-d9077e2d.nro
NRO bytes: 159741909
NRO SHA-256: b56bbce9f8d6155f318cbac44819967378df468f1355dfa4538691d1e536a664
```

Physical result:

```text
Trainer: PASS
Party: PASS
Boxes: PASS
Pokemon details: PASS
General RBY browsing: PASS
Items: FAIL — device-discovered category/inventory defect
Platform label: FAIL — Yellow was shown as GBA instead of GB
```

This test is historical evidence and remains preserved.

### 2. Corrected ItemsFix runtime — physically accepted

```text
Pre-fix feature/docs head: 0e92cee6ae743d5156dd8f7aaf55750b70dde5e5
Application source: 50dac31f53907143f48884681056f8d582813b76
Application tree: 1cf73ea12833e8a94a06dfaf2b9036e9059344ec
Commit: gen1: add read-only RBY inventory support
Verification run: 34576027301
Recovery snapshot: 5bfc27a10de1eeaf52cb92316c1453df3d4fb613
Recovery archive SHA-256: 43ca238b1c8402f7cdc17ba9a0ca58ec87bc68895faf354403db2673a9f419fa
Native/device build run: 34576781488
Embedded RomFS: 3289/3289 PASS
```

Exact accepted package:

```text
NRO: PokeBank-NX-RBY-ItemsFix-Retest-50dac31f.nro
NRO bytes: 159754197
NRO SHA-256: b2a8c68a80b27ca647777e7286da976b25d66ff7460555f9a404a1a783a1c16b

ZIP: PokeBank-NX-RBY-ItemsFix-Retest-50dac31f.zip
ZIP bytes: 152773612
ZIP SHA-256: f6dbdf2a76df1598e37ea0d1a771ac7a1eaa1864a29294c9101a6c6c09e2ecb6

NRO manifest SHA-256: 4b43aee6f5e68db8a3733449df4627af1c5e2a4c82f19bb038ceb5917014b126
BUILD_MANIFEST.json SHA-256: d4c93a7c24240ec444f9a105039b7920e2ac82545a8866caba97a963fa2e8f6d
SHA256SUMS.txt SHA-256: cb0930d99fe60339eca103bf82c80d61dba0ba4ac6153ffde536675d16ef2894

Actions artifact: RBY-ItemsFix-Retest-50dac31f
Artifact id: 10190135145
Actions archive bytes: 305759538
Actions archive SHA-256: 1a9fa5a4e43ac4c00d06e89c96b5effbe4450958fc41f4ad3f71e39fc8fe4901
```

Physical retest on the exact NRO/hash above passed RBY opening, Trainer, Party, Boxes, Pokémon details, general browsing, Items, Bag, PC Items, inventory navigation and the GB platform label. No `Invalid category` or modern `Medicines` fallback remained. Generation-appropriate Trainer fields were shown without fabricated SID/modern fields.

Authoritative RBY status:

```text
Red: DEVICE TESTED = YES / DEVICE ACCEPTED = YES
Blue: DEVICE TESTED = YES / DEVICE ACCEPTED = YES
Yellow: DEVICE TESTED = YES / DEVICE ACCEPTED = YES
RBY Items: DEVICE TESTED = YES / DEVICE ACCEPTED = YES
GENERATION I RBY LEGACY READ-ONLY: PHYSICALLY ACCEPTED
```

## RBY verification gates — preserved

```text
Focused RBY inventory: PASS
RBY parser/oracle: PASS
RBY discovery: PASS
RBY source browser: PASS
RBY read-only bridge: PASS
Malformed inventory nonfatal test: PASS
Full host suite: PASS
FRLG regression: PASS
RSE regression: PASS
Source mutation/write policy: PASS
ASan: PASS
UBSan: PASS
git diff --check: PASS
Gen III protected paths: PASS
Device asset preflight: PASS
Native devkitA64 compile: PASS
Native devkitA64 FINAL LINK: PASS
Embedded application identity: PASS
Embedded RomFS: 3289/3289 PASS
```

The accepted implementation uses strict read-only Generation I inventory with the two real stores, Bag and PC Items, dedicated Gen I item naming and GB platform identity. Malformed optional inventory does not invalidate otherwise-readable Trainer/Party/Boxes data. Source bytes remain immutable.

## Accepted Generation III baseline

```text
FireRed GBA: DEVICE TESTED YES / DEVICE ACCEPTED YES
LeafGreen GBA: DEVICE TESTED YES / DEVICE ACCEPTED YES
Ruby GBA: DEVICE TESTED YES / DEVICE ACCEPTED YES
Sapphire GBA: DEVICE TESTED YES / DEVICE ACCEPTED YES
Emerald GBA: DEVICE TESTED YES / DEVICE ACCEPTED YES
GEN III LEGACY READ-ONLY: PHYSICALLY ACCEPTED
```

Accepted RSE application source remains `a2df4c1acdb7a556808bd58a2bdbcd4fc0335954`; exact accepted RSE NRO SHA-256 remains `34fc0893ae0f0ee1a3e244c11469a5d44de181d68318040fce386470b2e0e80e`.

Do not reopen accepted Gen I or Gen III behavior without new physical evidence of a defect.

## Current architecture / safety rules

- Legacy source identity remains provider/path/save-container based.
- Savestates are not canonical battery-save sources.
- Original source bytes remain untouched during read-only milestones.
- Unknown save variants fail safely instead of being guessed writable.
- Live installed-game writing is HARD DISABLED.
- Live RetroArch writing is HARD DISABLED.
- Physical acceptance of reads does not authorize live writes.

## Next milestone

Generation II Gold/Silver/Crystal is next:

```text
gold_gbc
silver_gbc
crystal_gbc
Game Boy Color / RetroArch
READ ONLY
```

The next coding session should recover first, inspect pinned PKSM-Core `Sav2` / `PK2`, independently oracle with PKHeX, then implement only the focused GSC read-only milestone with strict layouts/checksums, international/Japanese handling, Gold/Silver vs Crystal differences, Trainer, Party, Boxes, inventory, truthful Gen II summary fields, bounded normal battery-save discovery, source immutability, malformed/truncated rejection, Gen I + Gen III regressions, ASan/UBSan and native/device build gates.

No GSC implementation was started in this acceptance/merge session.
