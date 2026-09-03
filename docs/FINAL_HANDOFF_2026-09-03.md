# PokeBank NX — Final Handoff Before Next Coding Session

Date: 2026-09-03

This is the final research/status handoff before the next implementation session. It does **not** supersede `PROJECT_STATUS.md`; it exists to stop research drift and make the next coding session immediately actionable.

## Repository / branch safety

- Writable repository: `GlitchedZeus/PokeBank-NX`
- Development branch: `feature/pokebank-playable`
- `upstream` / `kiasta/PKSE` is upstream-only. Never push PokeBank NX changes there.
- Do not reset, clean, overwrite, or discard interrupted local work before inspecting status, reflog, stashes, worktrees, branches, and diffs.

## Exact source identities that must not be confused

### First physically tested application source

```text
3be4de6b0b1ce00d5fe369cff9795c3fffbfa31a
```

Exact tested artifact:

```text
PokeBank-NX-UI-Theme-3be4de6.nro
SHA-256 df7199c528c11b8792cccb483e15d5b2fa742d4d895b8df78b12f329dc90694a
DEVICE TESTED — PARTIAL PASS / KNOWN FAILURES
```

### Useful Session 2.5 application checkpoint

```text
361c6f551496470db305948d702944c6ed9889c1
ui: add visible PokeBank shell and physical stick input
```

This source contains the reported PokeBank NX shell and real libnx Left Stick handling. It is **NOT DEVICE TESTED**.

Later branch commits are mostly documentation/research/tooling. Branch HEAD must never be mistaken for the application-source SHA used to build a device artifact.

If Session 2.6 changes runtime/application source, create a **new application-source commit** and build the second device artifact from that exact clean commit.

## Current hardware truth

The old exact device build established:

```text
BOOT / relaunch / idle / browsing              PASS
D-pad + held repeat                            PASS
Left Stick                                     FAIL — no input at all
Action Sheet / B / cancel                      PASS
heavy Action Sheet repetition                  PASS
OLED Black / Dark / Light + persistence        PASS
HOME return / sleep-wake / reconnect           PASS
handheld                                       PASS
docked                                         NOT TESTED
sampled Summary data                           PASS / PARTIAL
Pokémon render in View                         MISSING IN TESTED ARTIFACT
one older Legends: Arceus save                 REPRODUCIBLE CRASH
visible PokeBank NX identity                   INCOMPLETE ON OLD TESTED BUILD
```

Mutation/storage testing clarified the physical transfer shape as:

```text
INSTALLED GAME — read / automatic backup
        ↓
MUTABLE BACKUP REPRESENTATION
        ↓
APP-OWNED LEGACY STORAGE (`PKSEBANK` / `bank.dat`)
        ↓
OTHER GAME'S LOADED BACKUP REPRESENTATION
```

The tester reports the original installed Z-A save remained unchanged in the exercised Arbok flow.

Therefore:

```text
LIVE INSTALLED SAVE WRITE OBSERVED:      NO
USER-REACHABLE MUTATION UI:              YES
BACKUP/IN-MEMORY MUTATION:               YES
APP STORAGE PERSISTENCE:                 YES
CROSS-GAME STORAGE VISIBILITY:           YES
MASTER VAULT:                            NOT IMPLEMENTED
TRUE MOVE:                               NOT IMPLEMENTED
```

Do not weaken the current live-write hard lock based on this evidence.

## Next session is implementation, not research

Research is intentionally **frozen until after the second physical device test**, unless a blocker cannot be solved from the repository/current references.

The next coding session must execute `docs/PROMPT_SESSION2_6_SAFETY_CRASH_FINISH.md` and remain focused on:

1. #23 — make installed-source mutation behavior unambiguously read-only and verify persistence paths.
2. #24 — harden old/malformed/unsupported Legends: Arceus handling so parser failures return safely instead of crashing.
3. Preserve #19 — the Session 2.5 real Left Stick implementation.
4. Preserve #13/#16 — visible PokeBank NX shell/branding work.
5. Run #37 device visual-asset generation/preflight so the existing Summary renderer is not shipped without its sprites.
6. Host tests + ASan/UBSan + `git diff --check` + native build.
7. Commit a new exact application source if application code changed.
8. Clean rebuild from the exact application SHA.
9. Package/hash/preserve the exact second `.nro` and populate the second-device checklist.
10. Stop at `READY FOR SECOND DEVICE TEST / NOT DEVICE TESTED`.

Do **not** begin Master Vault, PKSM-Core integration, DS/3DS/GameCube/Stadium adapters, true Move, full visual/3D work, or Mystery Gift/event implementation in Session 2.6.

## Research stack now sufficient for post-device-test work

The project has enough reference coverage to stop hunting and start engineering.

### Pokémon/save correctness and native engines

- PKHeX — primary correctness/save/legality oracle.
- PKSM-Core — primary native C++ historical save/entity candidate.
- PKSM — mature Bank/backups/migrations/events integration reference.
- Pokémon Chest — Gen III–V Nintendo-platform PKSM-Core integration reference.
- Eevee (`melsbacksfriend/Eevee`) — native Switch + PKSM-Core + app-owned bank proof-of-concept.
- `ncorgan/pksav` — independent lightweight C Gen I/II/GBA oracle/reference.
- PKMDS — independent C++ Gen V-heavy reference, also useful for Wondercards.
- `pkmn-savedata` — defensive parser design reference, especially malformed-input/no-panic philosophy.
- OpenHome — independent multi-generation HOME-style conversion/compatibility architecture reference.

### Actual game-source / format oracle

The `pret` reverse-engineering projects should be used as an additional source of truth for generations where available rather than relying on save editors alone.

`pret/pokefirered/src/save.c`, for example, documents the actual FireRed/LeafGreen two-slot save layout, 14-sector rotation, Pokémon storage sectors, counters/signatures, and checksum behavior in reconstructed game source.

Use the corresponding `pret` projects as independent format/behavior references for Gen I–V work where applicable.

### Native Switch platform / filesystem layer

- JKSV — preferred mature Switch save lifecycle/mount/backup/import reliability reference.
- switchbrew/libnx + `switchbrew/switch-examples` — low-level API/source-of-truth reference for save enumeration, mounting, unmounting, account IDs, commit behavior, and Horizon service use.
- EdiZon — secondary historical reference for data-driven per-title adapter/editor architecture.

PokeBank NX remains stricter than a general save manager: Pokémon parser validation, staging, exact readback verification, rollback/recovery, and per-adapter approval are required before live writes.

### Static game/world data

- pk3DS — Gen VI/VII static species/form/encounter/location/game-data reference.
- pkNX — modern Switch static species/form/encounter/location/game-data reference.
- BW_tool — specialized Gen V save/event subsection cross-check.

### Media / Summary

The project already has sufficient visual/audio references:

- inherited PKSE build-time PokeAPI HOME sprite pipeline;
- PokeAPI/sprites;
- PokéSprite;
- Project Pokémon Sprite Index;
- pokecss-media;
- Pokémon-3D-api as an optional/reference model source;
- PokeAPI/cries.

Do not spend the next coding session looking for more artwork.

### Events / Mystery Gifts — later

For later event work, prefer existing preserved event/wondercard datasets and mature parsers before inventing a new format database. Project Pokémon's EventsGallery and PKSM/PKMDS event tooling are natural starting points. Modern Switch gift/BCAT utilities may be useful as format references, but they do not authorize online/private Nintendo protocol work.

## Post-second-device-test engineering order

Unless new hardware evidence changes priorities:

```text
Session 2.6 safety/crash finish
        ↓
second exact .nro
        ↓
physical device test #2
        ↓
PKSM-Core Gen III spike (#4)
  compare Eevee + PKSM + Chest integration patterns
  compare PKHeX + pret + pksav/PKMDS/pkmn-savedata as independent oracles
        ↓
Gen III production reads
        ↓
Master Vault + Banks
        ↓
Colosseum/XD
        ↓
Gen I/II + RetroArch
        ↓
Stadium if cheap enough for v1 stretch
        ↓
DS Gen IV/V
        ↓
3DS Gen VI/VII
        ↓
modern Switch read validation
        ↓
Summary / Oracle / Dex / conversion / legality / events
        ↓
staged writes
        ↓
per-game approved live writes
        ↓
true Move
        ↓
release hardening / v1.0
```

## Required reading for next coding session

At minimum:

```text
PROJECT_STATUS.md
docs/FINAL_HANDOFF_2026-09-03.md
docs/DEVICE_TEST_FOLLOWUP_2026-09-03.md
docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md
docs/MUTATION_SAFETY_STATIC_AUDIT_2026-09-02.md
docs/PROMPT_SESSION2_6_SAFETY_CRASH_FINISH.md
docs/SAVE_SAFETY.md
docs/DEVICE_BUILD_ASSET_GATE.md
docs/DEVICE_ARTIFACT_PACKAGING.md
```

The detailed Session 2.6 prompt remains authoritative for execution details.

## Handoff rule

The next session should spend its reasoning budget on **recovering/preserving interrupted work and writing/verifying code**, not repeating tonight's reference research.

The desired next user-facing deliverable is the actual exact replacement `.nro`, its application-source SHA/size/SHA-256, and a second-device test checklist with hardware result fields left blank.
