# PokeBank NX — Current Verified Engineering State

Last updated: **2026-09-25**

GitHub is authoritative. Re-fetch live heads before new work and audit forward if anything has advanced.

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

Device acceptance belongs only to that exact tested artifact.

## Active audit / hardening line

~~~text
PR #79:
OPEN / DRAFT / NOT MERGED

Branch:
audit/full-project-hardening-20260923

Current exact head:
be39c6f1eb422a3b5410ddaa9004294e1e709650

Tree:
e3ac245fafde113e472d441cb4961a9e0cc3cb5d

Host Tests:
36103080464 / #1183 / SUCCESS

Included:
normal host suite SUCCESS
focused RSE regression SUCCESS
ASan SUCCESS
UBSan SUCCESS

Audit Hardening Native Validation:
36103077129 / #84 / SUCCESS
~~~

This audit head is **CI VERIFIED**, not device accepted.

## Audit state

The project is now in the **late safety-audit / exact-route closure** phase.

### Storage / custody / transaction foundation

| Area | Status |
|---|---|
| A01 durable Bank replacement | IMPLEMENTED / CI VERIFIED |
| A02 custody-safe rollback | FIXED / CI VERIFIED |
| A03 immutable source + separate destination candidate | IMPLEMENTED / CI VERIFIED |
| A04a durable Move journal / recovery engine | IMPLEMENTED / CI VERIFIED |
| A04b Bank ↔ PokeBank-owned single-file workspace true Move | IMPLEMENTED IN SOFTWARE / CI VERIFIED |
| A05 corrupt/unreadable recovery evidence retention | FIXED |
| A06 BDSP truncated-layout guard | FIXED |
| A07 unsupported newer/larger Bank write blocking | FIXED |
| A08 profile/account + exact-game mutable workspace namespacing | IMPLEMENTED |
| A09 supported single-file workspace durability | IMPLEMENTED |
| A09-BDSP recoverable two-file generation | OPEN |
| N06 durable whole-directory generation/promotion | OPEN |
| physical FAT32/exFAT power-loss acceptance | OPEN |

Installed-game, RetroArch and other emulator-source writes remain hard disabled.

### Conversion fidelity foundation

Completed or substantially implemented:

- F05 modern/Gen III shiny threshold handling;
- F06 Unown form preservation;
- F07 PID-search exhaustion fails closed;
- F08 ability-slot/hidden-ability policy;
- F09 S/V ↔ Z-A Tera/Alpha/divergent-data fidelity;
- F10 Gen III EV boundary behavior;
- F11 nickname/language/text fail-closed handling;
- F13 historical origin / current location / provenance separation;
- route-level golden fixtures across LGPE, PLA, SWSH, BDSP, SV, Z-A and Gen III slices;
- production `preflightConvert()`;
- explicit Loss / Adaptation reporting;
- PBCE v1 persisted conversion evidence;
- explicit loss-summary + user-acknowledgement policy;
- recovery-time conversion-evidence authorization.

No cross-game true-Move route is enabled.

## Sword / Shield ↔ Scarlet / Violet exact-pair audit

All eight exact title directions now have production fixture coverage:

~~~text
Sword  -> Scarlet
Sword  -> Violet
Shield -> Scarlet
Shield -> Violet
Scarlet -> Sword
Scarlet -> Shield
Violet  -> Sword
Violet  -> Shield
~~~

Important proven/fixed areas include:

- exact species/form presence gating;
- representative regional/permanent form behavior;
- shiny/PID/EC preservation;
- normal, duplicate and hidden abilities;
- fail-closed Shiftry/Gallade slot-2 ability divergence;
- destination learnset move/relearn sanitization;
- PP/PP Ups carry + destination PP clamping;
- held-item presence/loss handling;
- HOME tracker relocation/preservation;
- representative event/fateful/ribbon/mark state;
- modern languages including Japanese/Korean/Chinese text examples;
- Tera synthesis/loss reporting;
- Scale / ObedienceLevel adaptation reporting;
- historical-origin preservation;
- generation-specific base-stat handling and destination battle-stat recalculation;
- representative round trips;
- source-byte + source-SHA immutability.

### Exact-pair decision

~~~text
Sword  -> Scarlet   ROUTE MUST REMAIN DISABLED
Sword  -> Violet    ROUTE MUST REMAIN DISABLED
Shield -> Scarlet   ROUTE MUST REMAIN DISABLED
Shield -> Violet    ROUTE MUST REMAIN DISABLED
Scarlet -> Sword    ROUTE MUST REMAIN DISABLED
Scarlet -> Shield   ROUTE MUST REMAIN DISABLED
Violet -> Sword     ROUTE MUST REMAIN DISABLED
Violet -> Shield    ROUTE MUST REMAIN DISABLED
~~~

No direction is yet a potential enablement candidate.

Remaining blockers:

- unknown/reserved PK8/PK9 bytes are not fully classified;
- special/event-distribution semantics remain incomplete;
- broad ribbon/mark edge coverage remains incomplete;
- special/form-specific edge coverage remains incomplete;
- complete invalid/special-ball policy is not fully fixture-proven;
- full text-boundary/malformed UTF-16 corpus is incomplete;
- physical Switch FAT32/exFAT recovery acceptance remains open.

## Permanent safety invariants

~~~text
ORIGINAL SOURCE SAVE: IMMUTABLE
LIVE INSTALLED-GAME WRITE: HARD DISABLED
LIVE RETROARCH WRITE: HARD DISABLED
LIVE OTHER-EMULATOR WRITE: HARD DISABLED
POKEBANK STAGED EDITING: ALLOWED
UNKNOWN SAVE VARIANTS: FAIL CLOSED
~~~

A-button remains non-destructive by itself.

## What remains before the core audit can be considered closed

For the **core pre-Master-Vault safety audit**, the remaining major gates are now concentrated into roughly four buckets:

1. **PK8/PK9 closure audit** — unknown/reserved bytes plus event/ribbon/mark/form/ball/text boundary closure for SWSH ↔ SV.
2. **Malformed/truncated parser hardening sweep** — remaining game-family parser boundaries.
3. **Physical Switch recovery matrix** — FAT32/exFAT interruption tests around destination write/verify and source retirement.
4. **Outstanding durability edge cases** — BDSP two-file generation and N06 directory-generation durability, either completed or explicitly deferred behind fail-closed policy.

After those, audit work becomes mostly **route-by-route qualification** rather than another broad foundation audit.

## Current next engineering tranche

**PK8 / PK9 unknown-reserved-byte + event / ribbon / mark closure audit**

Scope should stay narrow:

- classify all meaningful/unmodeled PK8↔PK9 regions;
- expand special event/distribution fixtures;
- close ribbon/mark semantics;
- close special-form, ball and malformed/boundary text gaps where they affect SWSH↔SV;
- preserve source immutability;
- keep every cross-game route disabled;
- stop after deciding whether any SWSH↔SV exact direction can become a **POTENTIAL ROUTE CANDIDATE**.

Do not begin route activation in that tranche.

## Canonical project documents

- README.md — public project front page
- CURRENT_STATUS.md — exact engineering checkpoint
- PROJECT_STATUS.md — high-level project state
- docs/V1_ROADMAP.md — release roadmap
- docs/GAME_SUPPORT_MATRIX.md — game support matrix
- docs/FULL_PROJECT_AUDIT_2026-09-22.md — durable audit record
- docs/AUDIT_STATUS_2026-09-23.md — detailed audit-line history
- docs/REFERENCE_INDEX.md — references/licenses/provenance

GitHub is authoritative. Never reset or rebase backward to a recorded checkpoint if the live branch has moved forward.
