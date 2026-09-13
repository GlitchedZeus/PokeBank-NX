# PokeBank NX — Current Verified Engineering State

Last updated: 2026-09-13

## CURRENT STATE — GEN I + GEN II + GEN III LEGACY READ-ONLY PHYSICALLY ACCEPTED

Active development branch:

```text
feature/pokebank-playable
```

Latest accepted GSC code checkpoint before documentation commits:

```text
50e32d4456f84ea8ac9680c820526770c2b2cf3e
gen2: show fixed male trainer gender in gold and silver
```

The project owner physically tested the corresponding latest GSC NRO and reported Gold, Silver and Crystal working as intended, including the final Gold/Silver Trainer gender correction.

Current accepted legacy state:

```text
Generation I
Red       READ ONLY / DEVICE ACCEPTED
Blue      READ ONLY / DEVICE ACCEPTED
Yellow    READ ONLY / DEVICE ACCEPTED

Generation II
Gold      READ ONLY / DEVICE ACCEPTED
Silver    READ ONLY / DEVICE ACCEPTED
Crystal   READ ONLY / DEVICE ACCEPTED

Generation III
FireRed   READ ONLY / DEVICE ACCEPTED
LeafGreen READ ONLY / DEVICE ACCEPTED
Ruby      READ ONLY / DEVICE ACCEPTED
Sapphire  READ ONLY / DEVICE ACCEPTED
Emerald   READ ONLY / DEVICE ACCEPTED
```

Live installed-game writing and live emulator-source writing remain **HARD DISABLED** by default.

---

## ACCEPTED GENERATION I BASELINE

Accepted application source:

```text
50dac31f53907143f48884681056f8d582813b76
```

Accepted exact device artifact:

```text
PokeBank-NX-RBY-ItemsFix-Retest-50dac31f.nro
bytes: 159754197
SHA-256: b2a8c68a80b27ca647777e7286da976b25d66ff7460555f9a404a1a783a1c16b
```

Accepted RBY capabilities include strict read-only parsing, Trainer, Party, Boxes, PK1 details, Bag, PC Items, generation-appropriate Trainer fields, bounded RetroArch discovery, GB identity and source immutability.

Do not fabricate SID/modern fields for Generation I.

---

## ACCEPTED GENERATION II BASELINE

Current accepted GSC code checkpoint:

```text
50e32d4456f84ea8ac9680c820526770c2b2cf3e
```

Accepted GSC behavior includes:

- Gold read-only opening/browsing;
- Silver read-only opening/browsing;
- Crystal read-only opening/browsing;
- Trainer;
- Party;
- PC Boxes;
- Gen II items/inventory;
- PK2 details;
- permanent Gold/Silver/Crystal artwork/resources;
- Gold trainer gender displays `Male`;
- Silver trainer gender displays `Male`;
- Crystal gender remains save-derived;
- Gen II SID remains unsupported/N/A;
- source immutability;
- Gen I + Gen III regressions preserved.

Physical owner retest result:

```text
Gold:    PASS
Silver:  PASS
Crystal: PASS
Gold fixed Male Trainer field:   PASS
Silver fixed Male Trainer field: PASS
Crystal existing gender behavior: PASS
```

Issue #6 is complete/closed for the original Gen I–III RetroArch read-only scope.

---

## ACCEPTED GENERATION III BASELINE

Accepted final RSE application source:

```text
a2df4c1acdb7a556808bd58a2bdbcd4fc0335954
```

Accepted RSE artifact SHA-256:

```text
34fc0893ae0f0ee1a3e244c11469a5d44de181d68318040fce386470b2e0e80e
```

Accepted Gen III behavior includes strict rotating-sector/signature/counter/checksum validation, Trainer, Party, Boxes, PK3 browsing, FRLG/RSE inventory, Refresh/current-save behavior, bounded RetroArch discovery and source immutability.

Do not reopen accepted Gen I/II/III read behavior without new physical evidence of a regression.

---

# NEXT ACTIVE PHASE — SAFE UNIVERSAL SAVE EDITOR

The next major engineering phase is to build a shared, generation-aware editor over the now-proven classic save readers.

Initial targets:

```text
Gen I  Red / Blue / Yellow
Gen II Gold / Silver / Crystal
Gen III Ruby / Sapphire / Emerald / FireRed / LeafGreen
```

First editor slice should prioritize:

- Trainer name;
- money;
- inventory quantities;
- Poké Balls / Master Balls;
- medicine;
- TMs/HMs where appropriate;
- Berries where appropriate;
- advanced Key Items with warnings;
- later player/map position;
- later full party/box Pokémon editing.

Audit/reuse inherited PKSE editor infrastructure first.

Editor architecture must remain staged:

```text
read original source
→ preserve original bytes
→ edit in-memory/staged copy
→ Review Changes
→ validate/checksum repair
→ export/test copy
→ future source-specific approved write-back
```

Adding edit controls does not authorize live source mutation.

---

# V1.0 REQUIRED ADDITION — FULL TOUCH-ONLY HANDHELD OPERATION

Tracked by issue #55.

PokeBank NX v1 must be completely usable on the physical Switch touchscreen if Joy-Cons/controllers are unavailable.

Required touch coverage includes Home, save/source selection, Trainer, Inventory, Party, Boxes, Vault/Banks, Pokémon details/actions, editor fields, software keyboard, numeric input, dialogs, scrolling and Back/Cancel navigation.

Controller and docked operation remain required and must coexist with touch.

---

# V1.0 REQUIRED ADDITION — UNIVERSAL EMULATOR SAVE DISCOVERY

Tracked by issue #56.

PokeBank NX must move beyond RetroArch-only source discovery using a reusable `SaveSourceAdapter`-style architecture.

Initial source targets:

```text
RetroArch
Tico
mGBA standalone
melonDS standalone
melonDS/libretro
DraStic
Azahar
Custom folders
Unknown-but-valid Pokémon saves
```

System discovery scope:

```text
GB / GBC / GBA / DS / 3DS
```

Required design includes config-driven paths, source/container normalization, fast scan, optional cancellable deep SD scan, multiple saves per game, deduplication, discovery cache, custom folders, provenance, and source-change detection before future writes.

---

# OTHER V1 MAJOR WORK STILL OPEN

- Master Vault v1 + Named Banks (#3)
- DS/3DS stable identities (#30)
- Nintendo DS read adapters (#31)
- Nintendo 3DS read adapters (#32)
- modern Switch source validation (#11)
- professional Summary/provenance (#9)
- Pokémon visual Summary support (#25)
- Vault Pokédex/Living Dex (#7)
- host PKHeX Oracle (#5)
- golden test corpus (#17)
- conversion/compatibility engine (#10)
- staged transaction/write framework
- individually approved live-write adapters
- true Move semantics (#20)
- reliability/diagnostics/performance polish (#21)
- final startup/branding/NRO metadata (#16)
- release-candidate hardware torture pass

Canonical full roadmap: issue #29.

---

# SAFETY / REPOSITORY AUTHORITY

```text
Repository: GlitchedZeus/PokeBank-NX
Development branch: feature/pokebank-playable
Writable remote: origin
Upstream reference: kiasta/PKSE
Live installed-game writing: HARD DISABLED by default
Live emulator-source writing: HARD DISABLED by default
```

PKSE is an upstream/reference source only. Never push custom PokeBank NX work upstream.

Physical read acceptance does not authorize live writes. Any future write path must be source-specific, staged, backed up, validated, read back and recoverable.


## Standalone runtime ownership

PokeBank NX is a standalone all-in-one Nintendo Switch Pokémon management application. External Pokémon homebrew and PC tools may be development references or optional migration tools, but are not prerequisites for normal advertised operation.

Canonical runtime root: `sdmc:/switch/PokeBank-NX/`. See `docs/STANDALONE_RUNTIME.md` and `docs/UI_OWNERSHIP_STATUS.md`. Issue #58 remains open for the full visible v1 UI conversion.

---

## Classic Inventory Editor — final development validation handoff (2026-09-13)

Status:

```text
Classic Inventory Editor: HOST VERIFIED / NATIVE VERIFIED / DEVICE TEST PENDING
Gen I RBY inventory:       HOST/NATIVE VERIFIED / DEVICE TEST PENDING
Gen II GSC inventory:      HOST/NATIVE VERIFIED / DEVICE TEST PENDING
Gen III RSE/FRLG inventory: HOST/NATIVE VERIFIED / DEVICE TEST PENDING
```

Validated code candidate:

```text
HEAD 3d972829e7b78ff1694bb499ceb20880f09474f0
TREE b5252e3a7e17d8b1ee350f66cc77c4c0a1b1a745
Corrected validation run 34739635304
Host job 103677024711
Native job 103677024816
```

The completed corrected validation passed exact identity, `git diff --check`, focused classic inventory, the previous FRLG link-failure regression, RBY staged-sidecar linkage, the permanent host suite, ASan, UBSan, authoritative control/safety invariants, device asset preflight, full devkitA64 compilation/final link, embedded application identity, and embedded RomFS verification.

Final inventory controls are authoritative:

```text
A             Edit Amount
X             Add Item
Y             Remove Item
D-pad/Stick   Navigate
L / R         Previous / Next valid category
+             Options
-             Help
B             Back / Cancel
```

The obsolete `Y Add`, `X Remove`, and `+` direct-Pending mappings are not part of the final control contract.

Validated classic inventory behavior includes exact-game and exact-pocket add catalogs; Yellow rejecting Dusk Ball/future items; `(None)` for valid empty categories; unsupported categories skipped; exact-game TM/HM move names; real Gen I Bag/PC structures; real Gen II pockets; the fixed Gen II Key Item capacity regression and Key Item warning flow; and completed Gen III staged inventory serialization.

The Gen II boxed Pokémon staged editor remains present and regression-tested. Party mutation remains deferred.

Classic edits remain source → staged memory → validate → export copy. Live installed-game writes, live RetroArch writes, and live emulator-source writes remain **HARD DISABLED**. Physical hardware acceptance of this editor milestone is still pending. Historical device-accepted read-only milestones above are unchanged.


---

## Inventory UI hardware follow-up — physical retest required (2026-09-13)

The newest physical Switch inventory test did **not** device-accept the inventory milestone. Classic staged inventories used the intended `A Edit / X Add / Y Remove` mapping, but editable modern Switch backup/workspace inventories still used the inherited opposite `Y Add / X Remove` mapping. The same hardware pass also found the classic Add Item picker too wide, picker controls insufficiently visible, and normal item-name color inconsistent because `item.isNew` recolored the whole item name.

The isolated hardware-polish candidate corrects presentation/input only; save formats, exact-game catalogs, staged serializers, and live-write policy are unchanged.

```text
Candidate SHA: 8cf4c7bf7a9213afcb4ef84a1a892b99017a7ed4
Candidate tree: e58f3e6058d48377983399bc3b198d52f20a9083
Validation run: 34742137283
Host job: 103683513164 — PASS
Native job: 103683513554 — PASS
Full host: PASS
ASan: PASS
UBSan: PASS
Device asset preflight: PASS
devkitA64: PASS
Native final link: PASS
Embedded identity/RomFS: PASS
Physical device status: RETEST REQUIRED
```

Authoritative editable Inventory controls for both classic staged saves and modern editable backup/workspaces are now:

```text
D-pad / Left Stick  Navigate
A                   Edit Amount
X                   Add Item
Y                   Remove Item
L / R               Previous / Next category
+                   Options
-                   Help
B                   Back / Cancel
```

Read-only installed/live sources remain read-only and must not advertise unavailable A/X/Y edit actions. The classic Add Item picker now uses the same 560px-centered geometry family as the inherited picker, 40px rows, a live position counter, visible controls, and functional L/R paging while retaining exact-game + exact-pocket filtering. Modern Add Item still uses the existing game/pouch provider.

Normal unselected item names now render with the normal text role regardless of `item.isNew`; selected names retain their selected readable color. The underlying `isNew` value is preserved. No NEW badge was added in this pass.

Yellow future-item rejection, machine move labels, Gen I real Bag/PC backing, GSC real pockets, Gen III staged serialization, Key Item warnings, `(None)` empty categories, unsupported-category skipping, Gen II Key Item capacity, Coin Case/Balls behavior, and source immutability remain regression-covered.

Live installed-game writes, live RetroArch writes, and live emulator-source writes remain **HARD DISABLED**.
