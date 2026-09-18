# PokeBank NX — Current Verified Engineering State

Last updated: 2026-09-18

## Current repository state

```text
Repository: GlitchedZeus/PokeBank-NX
Production branch: feature/pokebank-playable
Accepted milestone merge commit: e0815da5c1a167827a07ed1dfd50d49db2136aa3
Accepted source checkpoint: c24859ce17d33040685ea19b9aff068ba378d8ae
Accepted source tree: 88d2ad46d57346df346453698ee11792f7df4f1f
PR #74: MERGED
Writable remote: origin
Upstream/reference: kiasta/PKSE
```

Always re-fetch production before new work. Preserve any newer GitHub head; never reset backward to one of the checkpoints recorded here.

## Status vocabulary

```text
IMPLEMENTED          code exists but may not yet be fully validated
CI VERIFIED          exact source SHA passed the stated automated gate
DEVICE TEST PENDING  exact CI artifact is ready for owner hardware testing
DEVICE ACCEPTED      owner physically tested and accepted that exact artifact/hash
```

Never promote a future milestone to DEVICE ACCEPTED without an owner hardware result.

## Latest device-accepted milestone

### Gen I/II packed move + multi-select — DEVICE ACCEPTED

```text
Physical result: PASS / NO BUGS FOUND
Accepted source: c24859ce17d33040685ea19b9aff068ba378d8ae
Accepted tree: 88d2ad46d57346df346453698ee11792f7df4f1f
Accepted parent: 99aacea1d9c5bd392b1eb779d175e49d3f99179c
Accepted NRO: PokeBank-NX-Gen1-UX4-Retest-c24859ce.nro
Accepted NRO size: 161681137 bytes
Accepted NRO SHA-256: 0eeef3c4752fc52240332567bf3919099271be4985fda346da26d66018fff82f
Accepted Actions run: 35316216883
Accepted artifact: Gen1-UX4-Retest-Candidate
Accepted artifact ID: 10535653625
Artifact digest: sha256:15c9a98c37c05f7ce4427791b33968faa43e0a9cb7442e57eb0a8b537ef0f6c1
PR #74 merge commit: e0815da5c1a167827a07ed1dfd50d49db2136aa3
```

Accepted behavior:

- Red/Blue/Yellow source boxes use their native 20 valid slots.
- Gold/Silver/Crystal source boxes use their native 20 valid slots.
- PokeBank-owned Legacy Storage remains 30 slots per box.
- Y tap performs single packed movement.
- Y hold enters rectangular multi-select.
- D-pad expands/contracts selection while held.
- selected/group ordering is deterministic.
- same-box and cross-box packed group movement are accepted.
- insufficient destination capacity rejects atomically without partial insertion.
- B restores the exact staged pre-pickup state.
- Release exists only through A Actions with explicit confirmation; no bulk Release shortcut was added.
- Gen I/II fullscreen View/Edit/Create are accepted.
- Gen II action-sheet ownership is corrected: Add immediately shows Create, Review opens Review, and View/Edit/Create own the frame without the old action sheet remaining above them.
- controller behavior tested cleanly on hardware.

## Physically accepted baselines

```text
Red GB read-only                         DEVICE ACCEPTED
Blue GB read-only                        DEVICE ACCEPTED
Yellow GB read-only                      DEVICE ACCEPTED
Gold GBC read-only                       DEVICE ACCEPTED
Silver GBC read-only                     DEVICE ACCEPTED
Crystal GBC read-only                    DEVICE ACCEPTED
FireRed GBA read-only                    DEVICE ACCEPTED
LeafGreen GBA read-only                  DEVICE ACCEPTED
Ruby GBA read-only                       DEVICE ACCEPTED
Sapphire GBA read-only                   DEVICE ACCEPTED
Emerald GBA read-only                    DEVICE ACCEPTED
Classic staged Inventory                 DEVICE ACCEPTED
Gen I boxed staged Pokemon editor        DEVICE ACCEPTED
Gen I passive View unification           DEVICE ACCEPTED
Gen I/II packed move + multi-select      DEVICE ACCEPTED
```

Gold/Silver Trainer gender is fixed Male. Crystal Trainer gender is save-derived. Gen II SID does not exist.

Historical Gen I editor acceptance remains:

```text
Accepted source: 69668bc81629228ef25c1bdada7c7ce1aed9b666
Accepted NRO: PokeBank-NX-Gen1-UX4-Retest-69668bc8.nro
Accepted NRO SHA-256: 3ab11f7ba6938bbab5f7cbbf192d819532ce94f09bc7788a3bb0d8f6217f3763
```

## Safety invariants

```text
ORIGINAL SOURCE SAVE: IMMUTABLE
LIVE INSTALLED-GAME WRITE: HARD DISABLED
LIVE RETROARCH WRITE: HARD DISABLED
LIVE OTHER-EMULATOR WRITE: HARD DISABLED
POKEBANK STAGED EDITING: ALLOWED
UNKNOWN SAVE VARIANTS: FAIL CLOSED
```

No accepted milestone authorizes live source writeback.

## Continuation boundary

PR #74 is complete and merged. Do not reopen or amend its exact accepted source checkpoint.

At the start of the next session:

1. re-fetch `feature/pokebank-playable`;
2. preserve any newer production head;
3. treat the exact `c24859ce...` hardware result as frozen accepted evidence;
4. preserve Gen I/II packed movement, native source capacities, Legacy Storage 30-slot layout, fullscreen surfaces, Release semantics, and all source-write locks;
5. wait for explicit owner direction before selecting the next major milestone.

**Do not start Gen III automatically.**
