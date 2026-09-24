# NEXT ENGINEERING PROMPT — CONVERSION FIDELITY / GOLDEN FIXTURE AUDIT

Continue PokeBank NX from the exact CURRENT LIVE GitHub state.

Repository:
GlitchedZeus/PokeBank-NX

Active audit PR:
#79

Active audit branch:
audit/full-project-hardening-20260923

Known documentation checkpoint:
59ced7c81db457ce4e59cd8b15268d6a2296537d

Before any write:

- re-fetch PR #79 and its live head;
- preserve newer work;
- never reset/rebase backward;
- keep PR #79 draft/open;
- do not merge PR #77 or #79;
- keep all live-source write locks hard disabled.

## Completed foundation to preserve

A01 durable Bank persistence
A02 custody-safe rollback
A03 immutable original + destination candidate
A04a durable Move journal/recovery
A04b Bank ↔ PokeBank-owned workspace transaction integration for supported single-file routes
A05 recovery evidence preservation
A06 BDSP malformed-layout guard
A07 newer/larger Bank write block
A08 profile/exact-game namespaces
A09 supported single-file workspace durability

Do not redo those systems unless a new regression proves a defect.

## Primary task

Perform the exact-current F05–F13 conversion fidelity audit using golden fixtures.

Start with N01/F07 because it is already confirmed:

Modern/PK8-layout -> Gen III PID search can exhaust and silently fall back to an incompatible PID while still reporting successful conversion.

Required behavior:
if required correlated traits cannot be preserved, conversion must fail explicitly and must not produce a source-retiring true-Move candidate.

## Audit matrix

Reverify:

F05 shiny preservation
F06 Unown/PID-derived form
F07 PID-search exhaustion
F08 ability slot/number mapping
F09 S/V ↔ Z-A Tera/divergent fields
F10 Gen III EV 252/253/255 policy
F11 nickname/language/loss semantics
F13 profile/account provenance implications

Classify each exact-current item:

CONFIRMED DEFECT
ALREADY FIXED
PROVEN CORRECT BY FIXTURE
NEEDS POLICY
NEEDS MORE FIXTURES
UNSUPPORTED / FAIL CLOSED

## Golden fixture contract

For every supported conversion pair under test:

- capture exact source bytes/hash;
- conversion must not mutate source;
- serialize destination;
- reparse destination;
- verify destination checksum/container;
- verify species/form;
- verify shiny;
- verify PID/EC correlations where applicable;
- verify gender;
- verify nature;
- verify ability slot/number;
- verify IV/EV/DV/Stat Exp policy;
- verify moves/PP;
- verify nickname/language;
- verify origin/version/met data;
- verify held item/ball where applicable;
- verify all declared losses.

No silent fallback.

No silent trait drift.

No claim of legality without evidence.

## F07 fix policy

If PID search cannot satisfy all required Gen III correlated traits within the supported algorithm:

return explicit conversion failure.

Do not:
- leave the original incompatible PID;
- silently change nature/gender/shiny/ability slot;
- allow A04b to retire the source behind that candidate.

Add a permanent regression for forced/exhausted search failure.

## A04b integration gate

Cross-game true Move remains disabled until a conversion route is fixture-proven.

When a route becomes proven, document whether it is eligible for later A04b enablement.

Do not broadly enable all cross-game true Move during this tranche.

Do not change same-game/native-compatible true-Move behavior unless required by a proven bug.

## Source safety

ORIGINAL SOURCE SAVE: IMMUTABLE
LIVE INSTALLED-GAME WRITE: HARD DISABLED
LIVE RETROARCH WRITE: HARD DISABLED
LIVE OTHER-EMULATOR WRITE: HARD DISABLED
UNKNOWN SAVE VARIANTS: FAIL CLOSED

## Validation

Run:

- new golden conversion tests;
- existing A01–A09 regressions;
- A04a transaction tests;
- A04b production transaction tests;
- full Host Tests;
- ASan;
- UBSan;
- Audit Hardening Native Validation.

Do not claim DEVICE ACCEPTED from CI.

## Documentation

Update docs/AUDIT_STATUS_2026-09-23.md with exact-current F05–F13 disposition, fixtures, policies, route gating and exact CI evidence.

Update the conversion section of CURRENT_STATUS/roadmap only when the tranche is complete.

## Stop condition

When the F05–F13 conversion fidelity tranche is complete, STOP.

Do not automatically start:

- BDSP multi-file journal;
- N06 directory transaction;
- Master Vault;
- Gen IV;
- DS/3DS;
- live source writes.

Final report must include:

- exact final SHA/tree;
- every conversion pair/fixture tested;
- F05–F13 disposition table;
- exact F07 failure behavior;
- any newly confirmed P0/P1/P2 findings;
- route-level cross-game true-Move eligibility;
- Host/ASan/UBSan/native results;
- remaining conversion limitations;
- updated A01–A09 status;
- recommended next tranche.
