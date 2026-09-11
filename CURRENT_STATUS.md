# PokeBank NX — Current Verified Engineering State

Last updated: 2026-09-11

## CURRENT STATE — GENERATION I + GENERATION III LEGACY READ-ONLY PHYSICALLY ACCEPTED

Active development branch: `feature/pokebank-playable`.

Generation I Red/Blue/Yellow and Generation III FireRed/LeafGreen/Ruby/Sapphire/Emerald are physically accepted for the current **read-only legacy source** milestone on a real Nintendo Switch.

Live installed-game writing and live RetroArch writing remain **HARD DISABLED**.

## ACCEPTED GENERATION I RBY BASELINE

```text
Red GB: DEVICE TESTED = YES / DEVICE ACCEPTED = YES
Blue GB: DEVICE TESTED = YES / DEVICE ACCEPTED = YES
Yellow GB: DEVICE TESTED = YES / DEVICE ACCEPTED = YES
GEN I RBY LEGACY READ-ONLY: PHYSICALLY ACCEPTED
```

Accepted application identity:

```text
Application source: 50dac31f53907143f48884681056f8d582813b76
Application tree: 1cf73ea12833e8a94a06dfaf2b9036e9059344ec
Commit: gen1: add read-only RBY inventory support
Verification run: 34576027301
Recovery snapshot: 5bfc27a10de1eeaf52cb92316c1453df3d4fb613
```

Accepted exact device artifact:

```text
PokeBank-NX-RBY-ItemsFix-Retest-50dac31f.nro
bytes: 159754197
SHA-256: b2a8c68a80b27ca647777e7286da976b25d66ff7460555f9a404a1a783a1c16b
Native/device build run: 34576781488
Embedded RomFS: 3289/3289 PASS
```

Physical retest result on the exact artifact above:

```text
RBY opens: PASS
Trainer: PASS
Party: PASS
Boxes: PASS
Pokemon details: PASS
General browsing: PASS
Items: PASS
Bag: PASS
PC Items: PASS
RBY inventory navigation: PASS
No Invalid category: PASS
No modern Medicines pouch fallback: PASS
Yellow GB platform identity: PASS
Generation-appropriate Trainer fields: PASS
No fabricated SID/modern Trainer fields: PASS
```

Gen I inventory remains modeled as the two real saved stores, **Bag** and **PC Items**. TM/HM, key items, Poké Balls, medicines and ordinary items remain entries in those Gen I stores rather than fabricated modern pouch categories. Malformed optional inventory remains nonfatal to otherwise-valid Trainer/Party/Boxes access, and source bytes remain immutable.

### RBY physical-test history

The first device-test runtime was:

```text
Application source: d9077e2da3909b6fbe9d8db9ce7384a71b8b98e7
Application tree: 0ea6fbe365afe8122f252754554fef1fae73e183
Artifact: PokeBank-NX-RBY-Retest-d9077e2d.nro
SHA-256: b56bbce9f8d6155f318cbac44819967378df468f1355dfa4538691d1e536a664
```

That first physical test passed Trainer, Party, Boxes, Pokémon details and general browsing, but exposed the Items/category defect and the incorrect GBA label for Yellow. The corrected `50dac31f...` ItemsFix runtime was then fully host/sanitizer/native verified and physically retested successfully. Preserve both records.

## ACCEPTED GENERATION III BASELINE

```text
FireRed GBA: DEVICE TESTED = YES / DEVICE ACCEPTED = YES
LeafGreen GBA: DEVICE TESTED = YES / DEVICE ACCEPTED = YES
Ruby GBA: DEVICE TESTED = YES / DEVICE ACCEPTED = YES
Sapphire GBA: DEVICE TESTED = YES / DEVICE ACCEPTED = YES
Emerald GBA: DEVICE TESTED = YES / DEVICE ACCEPTED = YES
GEN III LEGACY READ-ONLY: PHYSICALLY ACCEPTED
```

Accepted RSE runtime remains `a2df4c1acdb7a556808bd58a2bdbcd4fc0335954`; accepted RSE NRO SHA-256 remains `34fc0893ae0f0ee1a3e244c11469a5d44de181d68318040fce386470b2e0e80e`.

Do not reopen any accepted Gen I or Gen III path without new physical evidence of a defect.

## VERIFIED RBY ITEMSFIX GATES

```text
Focused RBY inventory: PASS
RBY parser/oracle: PASS
RBY discovery: PASS
RBY source browser: PASS
RBY read-only bridge: PASS
Malformed inventory nonfatal: PASS
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

## NEXT MILESTONE — GENERATION II GSC READ-ONLY

The next legacy milestone is:

```text
gold_gbc
silver_gbc
crystal_gbc
Game Boy Color / RetroArch
READ ONLY
```

The next coding session should recover the accepted branch state first, then audit pinned PKSM-Core `Sav2` / `PK2` and independently check PKHeX before implementing strict Generation II parsing/discovery. Expected scope includes international/Japanese layouts, save sizes/checksums, Gold/Silver vs Crystal differences, Trainer, Party, PC Boxes, inventory, truthful Gen II Pokémon summaries, bounded normal battery-save `.sav`/`.srm` discovery, source immutability, malformed/truncated rejection, Gen I + Gen III regressions, sanitizers, native build and eventual exact device-test artifact.

**GSC implementation has not started in this acceptance/merge session.**

## SAFETY / REPOSITORY AUTHORITY

```text
Repository: GlitchedZeus/PokeBank-NX
Development branch: feature/pokebank-playable
Writable remote: origin
Upstream reference: kiasta/PKSE
Live installed-game writing: HARD DISABLED
Live RetroArch writing: HARD DISABLED
```

Physical read acceptance does not authorize live writes. Never push custom PokeBank NX work upstream.
