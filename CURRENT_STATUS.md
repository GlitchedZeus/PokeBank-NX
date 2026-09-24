# PokeBank NX — Current Verified Engineering State

Last updated: **2026-09-24**

## Device-accepted editor checkpoint

The physically accepted Gen I–III editor milestone remains:

~~~text
PR #77: OPEN / DRAFT / NOT MERGED
Branch: feature/gen3-shared-pokemon-editor-20260919

Application SHA:
996e6aa40c96e4408282f3d55476dae8e64968b2

Tree:
8826147ff5dc1b498b4b8505c9212243ed2f9498

NRO:
PokeBank-NX-Gen3-SharedEditor-996e6aa4.nro

NRO SHA-256:
ac3f6bd03d2a6733aee509729b81b6636cabe836095715c7b575b5dc84c8076c

Actions:
35825830004

Artifact:
10735208869

Status:
CI VERIFIED
DEVICE ACCEPTED
GENERATION III DONE
~~~

Acceptance belongs only to that exact tested artifact.

## Active audit/hardening line

~~~text
PR #79:
OPEN / DRAFT / NOT MERGED

Branch:
audit/full-project-hardening-20260923

Current documented head:
59ced7c81db457ce4e59cd8b15268d6a2296537d

Host Tests:
35961226077 / #1124 / SUCCESS

Audit Hardening Native Validation:
35961222076 / #33 / SUCCESS
~~~

This audit head is CI verified, not device accepted.

## What the audit has completed

| ID | Status | Current disposition |
|---|---|---|
| A01 | IMPLEMENTED | Bank uses verified DurableFile replacement; physical SD power-loss validation remains |
| A02 | FIXED | Failed rollback retains held Pokémon custody |
| A03 | IMPLEMENTED | Authoritative held Pokémon remains unchanged until destination commit |
| A04 | IMPLEMENTED IN SOFTWARE / HARDWARE GATE REMAINS | Journal + production Bank ↔ PokeBank-workspace true Move are integrated for supported single-file routes; physical power-loss testing remains |
| A05 | FIXED | Unreadable/corrupt Bank generations are preserved uniquely |
| A06 | FIXED | BDSP truncated-layout parsing fails closed before unsafe fixed offsets |
| A07 | FIXED | Newer/larger unsupported Bank layouts are write-blocked instead of truncated |
| A08 | IMPLEMENTED | Mutable workspaces are profile + exact-game namespaced; legacy unscoped data is quarantined |
| A09 | PARTIAL / FAIL-CLOSED | Supported single-file workspaces use DurableFile; BDSP still needs a recoverable multi-file generation |

Other open findings include the conversion matrix, N01/F07 PID-search fallback, N06 directory-generation durability, and broader malformed-input/parser coverage.

## A04 transaction status

The A04 transaction system is now implemented in software for:

~~~text
PokeBank bank.dat
<->
PokeBank-owned profile/exact-game mutable workspace
~~~

Supported single-file workspace families currently include:

- Let's Go Pikachu/Eevee
- Sword/Shield
- Legends: Arceus
- Scarlet/Violet
- Legends: Z-A
- FireRed/LeafGreen Switch

The transaction model is destination-first:

~~~text
PREPARED
-> DESTINATION_WRITTEN
-> DESTINATION_VERIFIED
-> SOURCE_RETIRE_PENDING
-> SOURCE_RETIRED
-> COMMITTED
~~~

Recovery uses whole-store SHA-256 fingerprints and fails closed on conflict, corrupt metadata or unsupported journal versions.

Important restrictions remain:

- installed-game saves are never transaction write targets;
- RetroArch/other emulator sources remain read-only;
- BDSP is excluded from true Move;
- cross-store occupied-slot swaps are refused;
- true cross-game conversion Move is disabled until conversion-preservation testing is complete;
- physical Switch FAT32/exFAT power-loss behavior is not yet accepted.

## Permanent safety invariants

~~~text
ORIGINAL SOURCE SAVE: IMMUTABLE
LIVE INSTALLED-GAME WRITE: HARD DISABLED
LIVE RETROARCH WRITE: HARD DISABLED
LIVE OTHER-EMULATOR WRITE: HARD DISABLED
POKEBANK STAGED EDITING: ALLOWED
UNKNOWN SAVE VARIANTS: FAIL CLOSED
~~~

## Current next engineering tranche

The next main software tranche is the **F05–F13 conversion fidelity audit**, beginning with the confirmed F07/N01 Gen III PID-search fallback.

Required approach:

1. re-fetch the live audit branch and preserve newer work;
2. build golden source/destination fixtures;
3. prove source bytes remain unchanged;
4. prove destination reparses/checksums correctly;
5. verify shiny, PID-derived traits, gender, nature, ability slot, form, nickname/language and declared losses;
6. fail explicitly when required semantics cannot be preserved;
7. keep cross-game true Move disabled until a route is proven;
8. keep all live-source write locks unchanged.

Parallel remaining gates after conversion work:

- malformed/truncated parser hardening;
- physical Switch transaction/power-loss recovery tests;
- BDSP multi-file transaction support;
- N06 directory-generation durability;
- Master Vault persistence/recovery.

## Canonical project documents

- README.md — public project front page
- CURRENT_STATUS.md — exact engineering checkpoint
- PROJECT_STATUS.md — high-level project state
- docs/V1_ROADMAP.md — release roadmap
- docs/GAME_SUPPORT_MATRIX.md — game support matrix
- docs/FULL_PROJECT_AUDIT_2026-09-22.md — durable audit record
- docs/AUDIT_STATUS_2026-09-23.md — live audit-branch detail
- docs/REFERENCE_INDEX.md — references/licenses/provenance

GitHub is authoritative. Never reset/rebase backward to a recorded checkpoint if the live branch has moved forward.
