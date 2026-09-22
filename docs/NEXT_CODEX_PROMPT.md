# NEXT CODEX PROMPT — PR #77 HARDWARE RETEST HOLD

Continue PokeBank NX from the current GitHub state. Do not restart, replace the branch, reset backward, or merge the active PR.

## Authoritative current state

```text
Repository:
GlitchedZeus/PokeBank-NX

Production branch:
feature/pokebank-playable

Active PR:
#77 — Generation III: extend the shared Pokemon editor

Active branch:
feature/gen3-shared-pokemon-editor-20260919

Exact candidate under physical test:
2e4780412377abab3ffbe4fc2e4757339214a90f

Tree:
cdbf0b8faae901c8d765d06b95cac3140bd69e75

NRO:
PokeBank-NX-Gen1-UX4-Retest-2e478041.nro

NRO SHA-256:
32b08c1cf589252022e68bf50fe0847fea7cbf1b86835178e7d2a2c268e3b43c

PR #77:
OPEN / DRAFT / NOT MERGED

Candidate:
CI VERIFIED
DEVICE RETEST REQUIRED
NOT DEVICE ACCEPTED
```

GitHub is authoritative. At the beginning of a coding session, re-fetch PR #77. If its head has advanced, preserve the newer head and reconcile forward. Never reset/rebase backward to the SHA above merely because it is recorded here.

## Current STOP condition

The owner is physically testing the exact Actions-built NRO above.

Until a hardware result is reported:

- do not add source-code cleanup/refactor commits to PR #77;
- do not merge PR #77;
- do not start another generation;
- do not start Vault/EventDex/DS/3DS/new-feature work;
- do not delete branches, commits, recovery assets, fixtures, or historical audit files;
- documentation/issue housekeeping belongs on `main` and must not alter the tested candidate.

## Hardware focus

The current retest includes the Gen I middle-values regression:

- HP DV visible/derived but completely non-focusable;
- Attack/Defense/Speed/Special DVs individually editable through one numeric keyboard;
- HP/Attack/Defense/Speed/Special Stat Exp individually editable;
- calculated Stat column visible but non-focusable;
- calculated Special display retained and non-focusable;
- Level and Shiny remain selectable;
- Create/Edit preserve exact Gen I storage semantics.

The owner may also continue reporting Gen II/Gen III hardware bugs. Record every report against the exact NRO/SHA.

## If the candidate fails

1. re-fetch live PR #77;
2. preserve any newer concurrent work;
3. reproduce/understand the exact hardware failure;
4. make only the narrow demonstrated fix on the same branch;
5. create a new SHA;
6. rerun the entire required exact-SHA gate;
7. never combine CI evidence from one SHA with another;
8. retrieve the exact CI artifact;
9. classify it `CI VERIFIED / DEVICE RETEST REQUIRED / NOT DEVICE ACCEPTED`;
10. stop for physical retest.

## If the candidate passes

Record the physical result against the exact NRO/hash first.

Do not automatically call all of Gen II/Gen III accepted if the owner has not explicitly accepted those exact behaviors.

Once the first three generations' editor milestone is explicitly frozen/accepted, the next engineering phase is **safety hardening**, not feature expansion.

Read:

```text
docs/FULL_PROJECT_AUDIT_2026-09-22.md
issue #69
```

Priority:

1. atomic/durable Bank persistence;
2. no-loss held-Pokémon rollback;
3. destination-first conversion/custody;
4. Bank + save transaction journaling;
5. non-destructive recovery generations;
6. malformed/truncated save guards;
7. revalidate prior conversion findings against the accepted source;
8. physical Switch/SD crash-recovery tests;
9. Master Vault immutable-original persistence/recovery;
10. only then broader feature expansion.

## References

Use `docs/REFERENCE_INDEX.md` before implementing a subsystem from scratch.

Important boundaries:

- PKHeX = primary current correctness oracle/reference;
- PKSM-Core = vendored/reference/adapter candidate;
- PKSM/pkmn-chest/OpenHomeNX = bank/save/transfer references;
- PKSM-Scripts + pret/pokediamond = future DS/event/native-game research;
- Project Pokémon Pokemon-Legality-Checker = historical legality reference only; no license found, so do not copy its source;
- no upstream write behavior overrides PokeBank NX source-write locks.

## Permanent safety

```text
ORIGINAL SOURCE SAVE: IMMUTABLE
LIVE INSTALLED-GAME WRITE: HARD DISABLED
LIVE RETROARCH WRITE: HARD DISABLED
LIVE OTHER-EMULATOR WRITE: HARD DISABLED
POKEBANK STAGED EDITING: ALLOWED
UNKNOWN SAVE VARIANTS: FAIL CLOSED
```

## PR cleanup state

- issue #71 / PR #75: COMPLETE / MERGED architecture freeze;
- PR #72: CLOSED WITHOUT MERGE as superseded; branch/history preserved;
- PR #77: ONLY ACTIVE DEVELOPMENT PR; OPEN / DRAFT / NOT MERGED.

Do not reopen PR #72 or create a replacement Gen III branch.
