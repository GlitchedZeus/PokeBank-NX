# NEXT CODEX PROMPT — GEN I RBY READ-ONLY

Continue PokeBank NX on `feature/pokebank-playable`.

Use MEDIUM reasoning. Use HIGH if Gen I checksum/layout/version behavior becomes ambiguous.

## THIS IS NOT RECOVERY

Do not redo Gen III, redownload sprites, regenerate the RomFS, or re-open accepted FRLG/RSE work without new device evidence.

Live installed-game and RetroArch save writing remains **HARD DISABLED**.

## VERIFIED BASELINE

FRLG and RSE are physically accepted on a real Nintendo Switch for the current read-only browsing/source-assignment milestone.

```text
FireRed GBA: DEVICE TESTED = YES / DEVICE ACCEPTED = YES
LeafGreen GBA: DEVICE TESTED = YES / DEVICE ACCEPTED = YES
Ruby GBA: DEVICE TESTED = YES / DEVICE ACCEPTED = YES
Sapphire GBA: DEVICE TESTED = YES / DEVICE ACCEPTED = YES
Emerald GBA: DEVICE TESTED = YES / DEVICE ACCEPTED = YES
```

Accepted RSE application source:

```text
a2df4c1acdb7a556808bd58a2bdbcd4fc0335954
application tree: 559202d16f0affc13a9e1c521beff4834585b1e7
```

Accepted RSE artifact:

```text
PokeBank-NX-RSE-OpenFix-Retest-a2df4c1a.nro
bytes: 158120837
SHA-256: 34fc0893ae0f0ee1a3e244c11469a5d44de181d68318040fce386470b2e0e80e
```

Host verification run `34453208654`: PASS.
Native/device build run `34454555232`: PASS.
Embedded RomFS: 3286/3286 PASS.

Permanent recovery snapshot:

```text
2321fa488668e32392de25afed84e38919fbd21f
```

Do not repack that snapshot unless the required asset set genuinely changes.

## REQUIRED READING ORDER

Read only what is needed:

1. `CURRENT_STATUS.md`
2. `docs/CODEX_SESSION.md`
3. this file
4. the **GB / GBC** chunk in `docs/PROJECT_RESOURCE_INDEX.md`
5. if external/reference research is needed, `docs/RESEARCH_CURRENT_INDEX.md` and only the relevant Gen I note/reference

Do not automatically crawl every roadmap/history/research document.

# SINGLE DEVELOPMENT MISSION

Implement the first production-quality **read-only Generation I RetroArch source path** for:

```text
Pokémon Red
Pokémon Blue
Pokémon Yellow
```

Target normal in-game battery saves (`.srm`, `.sav`, equivalent normalized raw save where already supported`).

Do NOT parse emulator savestates as canonical game saves.

Do NOT begin Gold/Silver/Crystal in this session.

## REUSE THE EXISTING ARCHITECTURE

Keep the same separation that now works for Gen III:

```text
RetroArch/provider discovery
        -> exact source identity
        -> strict format validation
        -> PokeBank-owned read model
        -> Trainer / Party / Boxes / Pokémon UI
```

Legacy source identity is provider/path/save-container based. Do not pretend a RetroArch save is inherently owned by a Nintendo account; profile assignment remains an organizational binding unless the provider itself supplies ownership.

## REFERENCE PRIORITY

Before inventing Gen I parsing behavior, inspect existing pinned/reference implementations.

Priority:

1. pinned `vendor/PKSM-Core` Gen I (`Sav1`, `PK1`, related helpers)
2. PKHeX Gen I save/PK1 code as an independent correctness oracle when useful
3. project reference audits/resources already indexed
4. other mature open-source Gen I bank/save tools only when they add concrete evidence

Record exact upstream project/revision/path for any newly relied-on behavior.

Do not copy large external implementations wholesale if a small adapter/selective native implementation is sufficient.

## REQUIRED GEN I BEHAVIOR

Implement/verify for Red, Blue and Yellow:

### Discovery / identity

- bounded RetroArch battery-save discovery;
- stable IDs `red_gb`, `blue_gb`, `yellow_gb`;
- no false R/B/Y classification when evidence is insufficient;
- preserve source filename/path/provider diagnostics;
- same game in different physical save files remains distinct source instances.

### Structural validation

Determine and enforce the real Gen I save contract before exposing Pokémon.

At minimum investigate/test:

- supported save sizes;
- game/release distinction where structurally possible;
- language/region differences that affect offsets/text/validation;
- checksum location/range and exact algorithm;
- party count bounds;
- box/storage structure and active/current-box semantics;
- malformed/truncated data;
- duplicate/impossible party/storage records;
- any Yellow-specific layout differences.

Important project rule:

`GameVersion != StructuralRevision`.

If multiple Gen I structural layouts exist, model them explicitly rather than assuming one English-US layout represents every save.

Unknown structural variants must be read-rejected or marked unsupported, never guessed writable.

### Trainer

Expose truthful fields available in Gen I, such as:

- trainer name;
- visible trainer ID;
- money if safely supported;
- badges/playtime only if already cleanly represented and verified.

Do not fabricate SID or later-generation fields that do not exist.

### Party

- correct party count;
- all party Pokémon;
- species;
- nickname;
- OT;
- level/EXP;
- moves/PP;
- stats/DVs/EV-style stat experience where available;
- current HP/status where represented;
- no fake modern fields.

### PC / Boxes

- enumerate all Gen I PC boxes correctly;
- handle current box and stored box structures;
- preserve slot order;
- empty slots handled safely;
- box names should remain truthful to what the game actually stores (do not invent custom names as save data).

### Pokémon view

Map Gen I records into the existing PokeBank Pokémon view without pretending unavailable metadata exists.

Preserve raw/original encoded bytes where practical so untouched source representation remains exact.

### Refresh

Changing/saving the game normally in RetroArch and using PokeBank Refresh should reread the current battery save without requiring app restart.

### Safety

- source file remains byte-for-byte unchanged;
- no repair/writeback API;
- no live write route;
- malformed input fails cleanly;
- accepted FRLG/RSE behavior remains unchanged.

## HOST FIXTURES / TESTS

Add focused Gen I fixtures/tests based on independently verified layout behavior.

At minimum cover:

- valid Red;
- valid Blue;
- valid Yellow;
- empty/minimal party where legal;
- six-Pokémon party;
- boxed Pokémon;
- Pokémon at storage boundary/last slot;
- nickname and OT text;
- checksum valid/invalid;
- wrong/truncated size;
- invalid party count;
- game/layout mismatch if distinguishable;
- source-byte non-mutation;
- Refresh/source rediscovery behavior.

Do not make production code and fixtures share the same unverified invented assumptions. Cross-check fixture layout against pinned PKSM-Core/PKHeX or another independent reference.

Retain and run existing Gen III regressions.

## CHECKPOINT EARLY

Once a coherent RBY read-only source milestone passes host verification:

```text
commit
push origin/feature/pokebank-playable
remote-verify SHA
```

Do not leave meaningful project-authored work only in the temporary workspace.

## VERIFICATION

Before a device build run:

- focused RBY tests PASS;
- full host suite PASS;
- FRLG regression PASS;
- RSE regression PASS;
- ASan/UBSan PASS;
- source mutation/write policy PASS;
- `git diff --check` PASS.

Then restore the committed recovery snapshot and run the full native devkitA64 compile/final link.

If R/B/Y game-card artwork is added as a required UI asset, use the established tracked/pinned asset pipeline, update device preflight and evolve the permanent recovery snapshot exactly once. Do not manually patch only a build workspace.

## DEVICE ARTIFACT

When RBY is coherent, build one physical-test artifact from the exact application source SHA, for example:

```text
PokeBank-NX-RBY-Retest-<shortsha>.nro
PokeBank-NX-RBY-Retest-<shortsha>.zip
manifest
SHA256SUMS.txt
```

Record exact source SHA, NRO size/hash, ZIP size/hash, native final-link result, embedded identity and embedded RomFS verification.

Update `CURRENT_STATUS.md`, this file, `PROJECT_STATUS.md`, `docs/GAME_SUPPORT_MATRIX.md`, `docs/BUILD_RECORD.md` as appropriate, and issue #6.

Push/remote-verify all source/tests/docs before stopping.

# STOP CONDITION

STOP after the RBY physical-test NRO is ready.

Do NOT begin Gen II, DS, 3DS, Vault/Banks, transfers, editor/legality/events, RetroArch per-Switch-user save routing, or live writing.

Expected final report:

```text
GEN III ACCEPTED BASELINE: PRESERVED
Red read-only source: IMPLEMENTED / HOST TESTED
Blue read-only source: IMPLEMENTED / HOST TESTED
Yellow read-only source: IMPLEMENTED / HOST TESTED
Focused RBY tests: PASS
FRLG/RSE regressions: PASS
ASan/UBSan: PASS
Source mutation/write policy: PASS
Native devkitA64 final link: PASS
Application source: <SHA>
Recovery snapshot: <SHA>
Embedded RomFS: PASS
NRO: <filename>
NRO bytes: <bytes>
NRO SHA-256: <hash>
SOURCE/TESTS/DOCS PUSHED + REMOTE VERIFIED: YES
DEVICE TESTED FOR RBY: NO
DEVICE ACCEPTED FOR RBY: NO
STOP FOR PHYSICAL SWITCH TEST
```
