# PokeBank NX — Save Safety Contract

Status: DESIGN / ENFORCED READ-ONLY ALPHA  
Last updated: 2026-09-01

PokeBank NX currently hard-disables direct live-game save writing. This document defines the conditions that must be met before that lock can ever be relaxed.

Safety takes priority over convenience.

---

## Current state

At the verified recovery safety milestone (`c618bd5`):

- installed game saves are treated as read-only sources
- visible destinations are backup/session destinations rather than live installed titles
- the generic save API no longer accepts an injection flag
- the low-level restore path rejects live-title writes before mounting save data
- a legacy `injectToGame=1` setting is ignored and rewritten to disabled

This state is **HOST TESTED / NRO BUILDS**. It is not yet physically device-tested.

---

## Non-negotiable rule

No code path may write to a live title simply because a UI caller passes a boolean, title ID, path, or unchecked destination object.

Live writing must require a deliberately separate, validated pipeline with explicit capability boundaries.

---

## Future write pipeline

Any future live write must follow this sequence:

```text
1. Read source/destination save
        ↓
2. Validate readable structure
        ↓
3. Create independent backup
        ↓
4. Hash/fingerprint original
        ↓
5. Create staged working copy
        ↓
6. Apply intended mutation to staged copy only
        ↓
7. Repair checksums/blocks/structures
        ↓
8. Serialize staged save
        ↓
9. Reparse serialized staged save
        ↓
10. Run game-specific validation
        ↓
11. Compare intended vs unintended changes
        ↓
12. User confirms final destination/action
        ↓
13. Write live destination
        ↓
14. Read destination back
        ↓
15. Verify fingerprint/structure/intended mutation
        ↓
16. Commit success journal

Any failure:
        ↓
ROLLBACK / RESTORE BACKUP
```

A transfer is not complete until the destination has been read back and verified.

---

## Backup requirements

Before any future live write:

- backup must be created before mutation
- backup must be stored outside the live save mount
- backup file must be closed/flushed before proceeding
- backup should have SHA-256 fingerprint
- backup metadata should record game identity, title ID where relevant, save filename, timestamp, and app commit/version
- if backup creation or verification fails, abort the write

Never overwrite the only backup during the same transaction.

---

## Staging requirements

All modifications happen against a staged copy.

The live save remains untouched while PokeBank NX:

- edits Pokémon/boxes
- updates Pokédex flags
- updates Handling Trainer fields
- recalculates checksums
- rewrites block structures
- converts destination-compatible Pokémon

The staged save must be reparsed by the same adapter after serialization.

Where practical, compare it with an independent implementation/oracle as part of development tests.

---

## Change-budget validation

Before live write, the adapter should know which regions/blocks/fields are expected to change.

Examples:

```text
expected:
- destination box slot
- box occupancy/count metadata
- Pokédex registration fields if enabled
- save checksums/block hashes required by the game

unexpected:
- trainer identity
- unrelated boxes
- unrelated inventory
- story flags
- settings
```

For formats where exact byte-range prediction is practical, fail if unrelated regions differ.

For hashed/block-based saves, validate at the semantic/block level where raw byte differences are inherently broader.

---

## Round-trip invariant

For every supported save adapter:

```text
load untouched save
      ↓
serialize without mutation
      ↓
reparse
```

Expected result:

- structural validity remains true
- all parsed gameplay data is equivalent
- checksum/block validation passes
- byte-identical output where the format and adapter permit exact round-trip

If untouched serialization intentionally normalizes/reorders data, document the reason and create regression tests proving equivalence.

---

## Adapter capability model

Each game/source adapter should declare capabilities explicitly.

Example:

```text
READ_SAVE
READ_PARTY
READ_BOXES
EXPORT_POKEMON
STAGE_MODIFICATION
VALIDATE_STAGED_SAVE
WRITE_LIVE_SAVE
VERIFY_LIVE_WRITE
```

During the current alpha, native title adapters must not advertise `WRITE_LIVE_SAVE`.

UI actions must be generated from capability checks rather than assumptions based only on game name.

---

## Read-only sources

Sources can be safely useful long before they are writable.

Examples:

```text
installed Switch save
RetroArch/GB/GBC/GBA save
manual imported save
backup file
```

An adapter may ship as `READ ONLY` indefinitely if write validation is incomplete.

Do not delay Vault import/browsing support merely because safe live writing is not ready.

---

## Per-game write enablement

Live writes are enabled **per game family/adapter**, never globally for all games at once.

Example progression:

```text
FireRed GBA backup file adapter
READ + STAGE + VALIDATE

then after extensive testing:
WRITE BACKUP FILE

later, separately:
Switch Scarlet adapter
READ + STAGE + VALIDATE

only after its own tests:
WRITE LIVE SAVE
```

One safe adapter does not validate another.

---

## Required automated tests before enabling a live-write capability

For the specific adapter:

- [ ] malformed/truncated save rejected
- [ ] unsupported version rejected read-only
- [ ] untouched round-trip tests
- [ ] party read tests
- [ ] box read tests
- [ ] single-slot staged write tests
- [ ] first/last slot boundary tests
- [ ] empty ↔ occupied slot tests
- [ ] checksum/block regeneration tests
- [ ] staged reparse validation
- [ ] unintended-change detection
- [ ] simulated interruption before publish
- [ ] simulated interruption during/after publish where testable
- [ ] backup restore tests
- [ ] host sanitizer pass
- [ ] native `.nro` build

---

## Required physical-device tests

Before a live-write capability is considered generally usable:

1. read-only device test on real hardware
2. write to a disposable/test save with independent pre-test backup
3. reboot/reopen game and verify save loads
4. verify intended Pokémon/content
5. verify unrelated content remains intact
6. repeat across multiple slots/boxes
7. repeat app restart/reboot cycles
8. test rollback from intentionally failed staged operation without touching live save
9. test insufficient-space behavior where practical
10. record exact app commit, game version, firmware/CFW environment

Do not perform early write testing on irreplaceable saves.

---

## Low-space behavior

Before persistent writes:

- estimate required temporary + backup + final space
- require safety margin
- fail before mutation if insufficient
- never delete the previous backup automatically just to make room for a risky write

Vault insertion and save-writing space checks should be separate because they have different recovery consequences.

---

## Crash recovery journal

Future save-write transactions should record states such as:

```text
BACKUP_CREATED
STAGE_CREATED
STAGE_VALIDATED
WRITE_STARTED
WRITE_COMPLETED
READBACK_VERIFIED
COMMITTED
ROLLED_BACK
```

On next launch, any incomplete transaction should be surfaced and handled conservatively.

Never silently claim success from the presence of a partially written file.

---

## User-facing confirmations

Live-write UI, when it eventually exists, should clearly distinguish:

```text
Export to backup file
Copy to staged save
Write to installed game
```

A normal A-button selection on a Pokémon must never perform a live write immediately.

The user should see destination game/source and operation type before confirmation.

---

## HOME bridge safety

The intended Pokémon HOME workflow is:

```text
PokeBank NX
  ↓
supported game save
  ↓
official Pokémon HOME app
```

PokeBank NX should not impersonate HOME/Nintendo services, extract credentials, spoof private server protocols, forge HOME trackers/history, or claim a generated Pokémon is guaranteed safe from enforcement.

A preflight may say:

```text
HOME Bridge: No known compatibility issues
```

It must not say:

```text
Ban Safe
Guaranteed HOME Safe
```

---

## Gate to remove the hard lock

The current live-write hard lock remains in place until **all** of the following are true for at least one explicitly named adapter:

- [ ] immutable Vault/backup foundation works
- [ ] staged save engine works
- [ ] adapter round-trip corpus passes
- [ ] checksums/block validation passes
- [ ] backup + rollback tested
- [ ] readback verification implemented
- [ ] unintended-change validation implemented
- [ ] host tests/sanitizers pass
- [ ] native `.nro` builds
- [ ] read-only device testing passes
- [ ] disposable-save live-write device testing passes repeatedly
- [ ] `PROJECT_STATUS.md` explicitly records which adapter is enabled

Even then, all other adapters remain read-only until individually validated.
