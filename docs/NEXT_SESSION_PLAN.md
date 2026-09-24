# PokeBank NX — Next Session Plan

Last updated: **2026-09-24**

Status: **GEN I–III DEVICE ACCEPTED / A01–A09 HARDENING MOSTLY IMPLEMENTED / CONVERSION AUDIT NEXT**

## Recover current live state

Before any write, re-fetch GitHub.

Known checkpoint at this documentation update:

~~~text
Repository:
GlitchedZeus/PokeBank-NX

Accepted editor PR:
#77 — OPEN / DRAFT / NOT MERGED

Accepted Gen III application:
996e6aa40c96e4408282f3d55476dae8e64968b2

Audit PR:
#79 — OPEN / DRAFT / NOT MERGED

Audit branch:
audit/full-project-hardening-20260923

Documented audit head:
59ced7c81db457ce4e59cd8b15268d6a2296537d

Host:
35961226077 / #1124 / SUCCESS

Native:
35961222076 / #33 / SUCCESS
~~~

If the live audit branch has advanced, audit forward. Never reset/rebase backward to this checkpoint.

## Preserve completed hardening

Do not redo or weaken:

- A01 durable Bank replacement;
- A02 custody-safe rollback;
- A03 immutable original + separate destination candidate;
- A04 transaction journal/recovery core;
- A04 production Bank ↔ PokeBank-workspace integration for supported single-file routes;
- A05 recovery evidence preservation;
- A06 BDSP truncation guard;
- A07 unsupported Bank write blocking;
- A08 profile/exact-game workspace namespace;
- A09 single-file workspace DurableFile persistence.

Keep installed-game, RetroArch and other emulator-source writes hard disabled.

Keep BDSP true Move disabled.

Keep cross-game true Move disabled until conversion routes are proven.

## Primary next task — F05–F13 conversion fidelity audit

Reverify exact-current conversion behavior rather than carrying old findings forward by assumption.

Start with the already confirmed high-risk item:

**N01 / F07 — Gen III downgrade PID-search exhaustion can silently fall back instead of failing explicitly.**

Then cover:

- F05 shiny preservation across Gen III threshold differences;
- F06 PID-derived Unown form;
- F08 ability slot/ability-number mapping;
- F09 S/V ↔ Z-A divergent/Tera data;
- F10 Gen III EV 252/253/255 policy;
- F11 nickname/language/loss behavior;
- F13 account/profile provenance where relevant.

## Test-first rule

For each conversion path:

1. build a minimal golden source fixture;
2. hash/capture original bytes;
3. run conversion;
4. prove original bytes are unchanged;
5. serialize destination;
6. reparse destination;
7. verify checksum/container integrity;
8. verify required identity traits;
9. record every intentional loss;
10. if required traits cannot be preserved, fail explicitly.

Do not silently generate a different Pokémon.

## Required traits to verify where applicable

- species/form;
- shiny;
- PID/EC;
- gender;
- nature;
- ability slot/number;
- IVs/EVs/DVs/Stat Exp;
- moves/PP;
- nickname flags/text;
- language;
- origin/version;
- held item;
- met data;
- ball;
- Tera/divergent modern fields;
- Unown/PID-derived form.

## Transaction safety interaction

A04b cross-game true Move must remain fail-closed while a route is unproven.

A warning dialog is not enough to authorize source retirement for a conversion whose correctness is unknown.

Same-game/native-compatible transaction routes may remain enabled according to the current A04b gate.

## Validation

After the conversion tranche:

- focused golden conversion tests;
- permanent host suite;
- A01–A09 hardening regressions;
- A04a/A04b transaction tests;
- ASan;
- UBSan;
- native devkitA64 compile/link/NRO.

Do not claim device acceptance from CI.

## Still deferred

Do not start these automatically:

- BDSP multi-file journal;
- N06 directory-generation transaction;
- Master Vault;
- Gen IV / DS / 3DS;
- live source writing.

After conversion fixtures are green, reassess parser hardening and the physical Switch power-loss/recovery matrix before Master Vault becomes authoritative storage.
