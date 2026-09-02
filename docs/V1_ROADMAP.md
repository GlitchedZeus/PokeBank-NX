# PokeBank NX — v1.0 Roadmap

Last updated: 2026-09-02

This is the canonical end-to-end roadmap from the current alpha to a serious PokeBank NX v1.0 release.

Master tracking issue: **#29 — Track PokeBank NX v1.0 roadmap and release gates**.

`PROJECT_STATUS.md` remains authoritative for what is actually implemented/tested today. This roadmap describes dependency order and release gates; it does not turn planned work into implemented work.

## Status rules

```text
[x] COMPLETE      = milestone evidence is recorded
[ ] IN PROGRESS   = active work exists but acceptance gate is not complete
[ ] PLANNED       = not complete
```

Physical evidence uses the stricter project vocabulary:

```text
IMPLEMENTED
HOST TESTED
NRO BUILDS
DEVICE TESTED — PASS / PARTIAL PASS / FAIL
```

A source commit, CI pass, or buildable `.nro` is never automatically `DEVICE TESTED`.

---

# v1.0 product target

A serious v1.0 should provide a stable controller-first Switch application that can:

- detect and safely browse advertised Pokémon save sources;
- keep installed-game sources read-only unless that exact adapter has separately passed the live-write safety gate;
- import Pokémon into a durable Master Vault with immutable raw bytes, hashes, origin and provenance;
- organize Vault entities into named Banks without accidental raw duplication;
- distinguish `COPY`, real `MOVE`, and deliberate `CLONE`;
- convert compatible Pokémon between supported generations/game formats with validation;
- provide professional Summary, provenance and collection views;
- derive Pokédex/Living Dex state from the Vault;
- support legality-aware editing/generation only where rules are understood and validated;
- support historical event/Mystery Gift workflows where data/license policy allows;
- recover cleanly from malformed saves, interrupted Vault transactions and missing optional resources;
- preserve exact release artifacts and verification metadata;
- remain stable/readable in handheld and docked use.

The roadmap intentionally separates **read support**, **Vault support**, **conversion/staging**, and **approved live writing**. One working write adapter never authorizes all games.

---

# Phase checklist

## Phase 0 — Repository recovery / project continuity — COMPLETE

- [x] Recover stable repository history after interrupted work.
- [x] Establish `origin = GlitchedZeus/PokeBank-NX` as writable remote.
- [x] Keep `kiasta/PKSE` upstream-only.
- [x] Record application-source vs docs/status commit distinction.
- [x] Establish repeatable session/runbook/status discipline.

Key docs: `PROJECT_STATUS.md`, `docs/SESSION_RUNBOOK.md`.

## Phase 1 — Save-safety hard lock / stable game identities — COMPLETE FOUNDATION

- [x] Low-level live installed-save restore/write hard lock.
- [x] Disable/ignore legacy `injectToGame=1` path.
- [x] Stable 23 release/platform identities.
- [x] Separate GBA FireRed/LeafGreen from Switch FireRed/LeafGreen identities.
- [ ] IN PROGRESS — finish auditing inherited mutation UI above the lower-level lock (#23).

## Phase 2 — Controller-first Pokémon Action Sheet — COMPLETE

- [x] A on occupied Pokémon opens a deliberate menu rather than immediate mutation.
- [x] B/Cancel navigation is non-mutating.
- [x] Shared Party / Boxes / Storage Action Sheet foundation.
- [x] Host tests, sanitizer pass and native build.
- [x] Same behavior later physically exercised successfully in the combined device build.

Issue: #2.

## Phase 3 — HOME-style controller/theme foundation — COMPLETE FOUNDATION / DEVICE PARTIAL

- [x] D-pad controller navigation.
- [x] Contextual `+` Options / `-` Help foundation.
- [x] OLED Black / Dark / Light themes.
- [x] Theme persistence.
- [x] Reusable cards/panels/modals/focus primitives.
- [x] Physical D-pad/theme/Action Sheet validation on first device build.
- [ ] IN PROGRESS — normalize final controller semantics (#26).

## Phase 4 — First exact physical Switch milestone — COMPLETE

- [x] Exact `.nro` physically launched.
- [x] Source SHA / filename / size / SHA-256 recorded.
- [x] Short hardware matrix recorded.
- [x] Extended torture test recorded.
- [x] Failures converted into issues rather than hidden.

Exact device-tested source: `3be4de6b0b1ce00d5fe369cff9795c3fffbfa31a`.

Issues/report: #8, `docs/DEVICE_TEST_REPORT_2026-09-01.md`, `docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md`.

## Phase 5 — Visible PokeBank NX shell + real analog input — IN PROGRESS

- [x] Source checkpoint for visible PokeBank NX shell/chrome/cards.
- [x] Source checkpoint for real libnx Left Stick position handling.
- [x] Deadzone/hysteresis + single-tap/held-repeat host coverage reported.
- [x] PokeBank NX NRO/window identity source change.
- [ ] Physical visual acceptance (#13/#16).
- [ ] Physical Left Stick single-tap/held/diagonal acceptance (#19).

Current useful source checkpoint: `361c6f551496470db305948d702944c6ed9889c1`.

## Phase 6 — Safety/crash finish + second exact device build — CURRENT BLOCKER

- [ ] Trace Release/Create/Move/Multi/Edit/apply/save/Storage persistence targets (#23).
- [ ] Block unsafe mutation UI on installed-game sources.
- [ ] Preserve low-level write lock.
- [ ] Harden old/malformed/unsupported Legends Arceus handling (#24).
- [ ] Preserve Session 2.5 visible-shell + analog work.
- [ ] Create new application-source checkpoint if source changes.
- [ ] Host tests PASS.
- [ ] ASan/UBSan PASS.
- [ ] `git diff --check` PASS.
- [ ] Native `.nro` builds.
- [ ] Exact replacement artifact filename/size/SHA-256 recorded and preserved.
- [ ] Second physical Switch test completed.
- [ ] Close #13/#19 only if the exact replacement binary passes their physical gates.

Current prompt: `docs/PROMPT_SESSION2_6_SAFETY_CRASH_FINISH.md`.

## Phase 7 — PKSM-Core Gen III integration spike — PLANNED NEXT MAX

- [ ] Audit `PK3` / `Sav3` build/dependency cost on Switch/devkitPro.
- [ ] Map PKSM-Core interfaces behind PokeBank NX adapters.
- [ ] Read FireRed/LeafGreen GBA saves without live writes.
- [ ] Extract Party / Boxes.
- [ ] Validate active slot/sector selection.
- [ ] Validate PK3 encryption/decryption/checksum behavior.
- [ ] Compare with PKHeX/reference vectors.
- [ ] Decide DIRECT REUSE / ADAPTER-WRAPPER / SELECTIVE PORT / REFERENCE ONLY.

Issue: #4.

Reference implementations to compare: PKSM, Pokémon Chest and PKSM-Core. See `docs/BANK_PROJECT_REFERENCE_AUDIT_2026-09-02.md`.

## Phase 8 — Gen III read-only adapter family — PLANNED

- [ ] FireRed / LeafGreen GBA production read adapter.
- [ ] Ruby / Sapphire / Emerald read adapters.
- [ ] malformed/truncated/checksum rejection.
- [ ] safe Party / Box browsing through common source interface.
- [ ] golden-fixture regressions.
- [ ] physical read-only testing on Switch.

Issues: #4, #6, #17.

## Phase 9 — Master Vault v1 — PLANNED

- [ ] Immutable raw Pokémon entity storage.
- [ ] Stable Vault IDs.
- [ ] SHA-256 entity integrity.
- [ ] Origin/source provenance.
- [ ] Active/current location separate from origin/history.
- [ ] Parent/derived relationships.
- [ ] Transaction journal.
- [ ] Interrupted-transaction recovery.
- [ ] Persistent reopen after restart.
- [ ] Rebuildable metadata/search index.
- [ ] corruption/migration tests informed by PKSM BankFile failure classes.

Issue: #3.

## Phase 10 — Named Banks + legacy Storage migration — PLANNED

- [ ] Named Banks referencing Vault IDs rather than blind raw duplication.
- [ ] Same Vault entity may appear in multiple organizational Banks.
- [ ] Bank deletion/reference removal cannot silently destroy Vault entity.
- [ ] Determine exact current legacy Storage format/path/persistence semantics (#27).
- [ ] Provide safe migration/import path for legacy Storage if feasible.
- [ ] Consider optional PKSMBANK import after Vault v1.

Issues: #3, #27.

## Phase 11 — Transfer Workspace / high-volume organization UX — PLANNED

- [ ] Dedicated Game ↔ Vault/Bank workspace inspired by PHBank/Pokémon Chest.
- [ ] One-Pokémon selection.
- [ ] Multi-select.
- [ ] Whole-box operation UI where safe/supported.
- [ ] Clear `COPY` / `MOVE` / `CLONE` wording.
- [ ] Preview before any destructive future transaction.
- [ ] Current installed-game sources stay read-only until write approval.

Related: #20, #26.

## Phase 12 — Professional Summary + provenance — PLANNED

- [ ] Generation-aware Summary fields.
- [ ] Origin vs current location.
- [ ] lineage/provenance history.
- [ ] legality state with `UNKNOWN != LEGAL`.
- [ ] Pokémon sprite/artwork/render with bounded memory (#25).
- [ ] intentional missing-art fallback.
- [ ] optional Right Stick model rotation only if technically/legal-resource practical.

Issues: #9, #25.

## Phase 13 — Golden corpus + PKHeX Oracle — PLANNED

- [ ] Reproducible legal test fixtures (#17).
- [ ] malformed/corrupt variants.
- [ ] stable SHA-256 fixture manifest.
- [ ] host PKHeX `inspect`.
- [ ] host PKHeX `legality`.
- [ ] host PKHeX `convert`.
- [ ] machine-readable comparison output.
- [ ] engine/revision metadata included in comparisons.

Issues: #5, #17.

## Phase 14 — Gen I / II + RetroArch discovery — PLANNED

- [ ] Discover supported RetroArch/legacy save sources.
- [ ] Red / Blue / Yellow read adapters.
- [ ] Gold / Silver / Crystal read adapters.
- [ ] language/region differences handled.
- [ ] manual import fallback.
- [ ] compare PHBankGBC offsets/behavior only as secondary reference.
- [ ] validate against PKSM-Core/PKHeX/fixtures before trusting historical constants.

Issue: #6.

## Phase 15 — Modern Switch read-adapter validation — PLANNED

Validate family-specific read behavior before staging/writes:

- [ ] Switch FireRed / LeafGreen.
- [ ] Let's Go Pikachu / Eevee.
- [ ] Sword / Shield.
- [ ] Brilliant Diamond / Shining Pearl.
- [ ] Legends: Arceus.
- [ ] Scarlet / Violet.
- [ ] Legends: Z-A.
- [ ] malformed/truncated/unsupported-version rejection.
- [ ] title ID/save filename/container facts cross-checked.
- [ ] source-specific capability matrix updated.

Issue: #11.

## Phase 16 — Conversion + compatibility engine v1 — PLANNED

- [ ] Destination compatibility: `SUPPORTED`, `SUPPORTED_WITH_CHANGES`, `UNSUPPORTED`, `UNKNOWN`.
- [ ] Convert between supported Pokémon formats.
- [ ] Preserve origin separately from derived/current representation.
- [ ] Parent/derived provenance.
- [ ] PKHeX Oracle comparison where available.
- [ ] Export `.pk*` representation.
- [ ] Stage destination-save representation without writing installed game.
- [ ] Transaction/journal state for staged transfer.

Issue: #10.

## Phase 17 — Vault-driven Pokédex / Living Dex — PLANNED

- [ ] National Dex.
- [ ] Vault-derived normal ownership.
- [ ] Vault-derived shiny ownership.
- [ ] missing/owned counts and filters.
- [ ] select entry → owned Vault specimens.
- [ ] regional Dex data.
- [ ] index rebuild from Vault.
- [ ] meaningful forms/gender distinctions.
- [ ] Alpha/Shiny Alpha where applicable.
- [ ] Living Dex views.
- [ ] Shiny Living Dex views.

Issue: #7.

## Phase 18 — Legality-aware editing / shiny operations — PLANNED

- [ ] Edit only safe app-owned/Vault representations until live write approval.
- [ ] legality validation before/after edit where available.
- [ ] `Make Shiny` obeys shiny locks and generation rules.
- [ ] conservative `UNKNOWN` where rules are not proven.
- [ ] derived entity/provenance preferred over silent original overwrite.

## Phase 19 — Encounter/legal generation — PLANNED

- [ ] Encounter-driven generation path.
- [ ] legal trainer/encounter consistency.
- [ ] form constraints.
- [ ] shiny constraints.
- [ ] generation-specific move/ability/item constraints.
- [ ] host-oracle comparison before native claims.

PKHeX/Auto Legality tooling remains host/reference infrastructure, not a promise of guaranteed online acceptance.

## Phase 20 — Bulk Vault population helpers — PLANNED

- [ ] Safe collection-generation tools for supported legal specimens.
- [ ] Fill/assist Living Dex workflows.
- [ ] shiny collection helper where legal.
- [ ] dedupe/idempotency.
- [ ] provenance labels generated entities clearly.

## Phase 21 — Mystery Gifts / event archive workflows — PLANNED

- [ ] Audit PKSM event/Wondercard behavior before reinventing.
- [ ] event database/data licensing/provenance plan.
- [ ] browse historical distributions offline where permitted.
- [ ] import/export supported Wondercard formats.
- [ ] event specimen provenance.
- [ ] legality/availability constraints.

## Phase 22 — Advanced transfer compatibility — PLANNED

- [ ] generation-hop rules.
- [ ] moves/abilities/items/forms lost or changed across destinations.
- [ ] handling trainer/origin data rules where applicable.
- [ ] clear preview of required changes.
- [ ] refuse unsupported/unknown destructive conversions.

## Phase 23 — Generic staged-save write architecture — PLANNED

- [ ] snapshot/fingerprint destination save.
- [ ] verified backup.
- [ ] mutate isolated staged copy only.
- [ ] repair checksums/containers.
- [ ] reparse staged output.
- [ ] compare intended vs unintended changes.
- [ ] durable transaction state.
- [ ] rollback/recovery behavior.
- [ ] UI displays staged/not-written state accurately.

No generic live-write switch.

## Phase 24 — First approved live-write adapter — PLANNED

- [ ] Choose one narrow, well-understood adapter.
- [ ] pass host fixtures/round-trip validation.
- [ ] pass staged mutation validation.
- [ ] verified backup.
- [ ] write.
- [ ] readback.
- [ ] exact intended-change verification.
- [ ] rollback behavior.
- [ ] physical Switch test.
- [ ] record capability only for that adapter.

## Phase 25 — Per-game live-write expansion — PLANNED

- [ ] Approve additional game families individually.
- [ ] Track per-version/container constraints.
- [ ] no inherited/global authorization.
- [ ] unsupported/unknown versions remain read-only.

## Phase 26 — True Move semantics — PLANNED LATE

- [ ] `Game → Vault/Bank MOVE`: destination durable first, then verified source removal.
- [ ] `Vault/Bank → Game MOVE`: destination game verified first, then active Vault placement retired.
- [ ] archival/provenance history remains behind the scenes without appearing as another active playable copy.
- [ ] Copy remains explicit and separate.
- [ ] Clone remains explicit and separate.
- [ ] one/multi/whole-box Move uses same safety transaction model.

Issue: #20.

## Phase 27 — Official HOME bridge workflow — PLANNED

Use supported game saves as the bridge rather than impersonating private HOME/Nintendo protocols.

- [ ] PokeBank NX → supported game → official HOME workflow documented/tested where applicable.
- [ ] Preserve genuine HOME tracker/history; never forge it.
- [ ] No credential extraction/private-protocol impersonation.
- [ ] No guaranteed-ban-safety claims.

## Phase 28 — Native `.nro` reliability / diagnostics / accessibility — PLANNED THROUGHOUT

Implement incrementally (#21):

- [ ] Diagnostics screen.
- [ ] privacy-safe diagnostic export.
- [ ] Applet/constrained-memory detection.
- [ ] privacy-safe crash/error logs.
- [ ] clear READ ONLY / VAULT / STAGED / active-location badges.
- [ ] search/filter.
- [ ] Box Quick Jump.
- [ ] Continue / Recently Viewed / Recently Added.
- [ ] Favorites.
- [ ] text-size option.
- [ ] Reduced Motion.
- [ ] focus not dependent on color alone.
- [ ] optional original sounds / restrained rumble.
- [ ] storage-health / Verify Vault / rebuild index.

Issue: #21.

## Phase 29 — Final branding/startup/icon/metadata — PLANNED

- [ ] final PokeBank NX splash/startup flow.
- [ ] real startup stages tied to real work.
- [ ] final title/icon/NACP metadata.
- [ ] visible version + source SHA.
- [ ] graceful missing optional resources.
- [ ] no normal-path PKSE product identity.

Issue: #16.

## Phase 30 — Performance / scale / release-candidate torture testing — PLANNED

- [ ] virtualized huge Vault/Dex grids.
- [ ] bounded LRU-style artwork/sprite caches.
- [ ] lazy raw-entity loading.
- [ ] metadata-first browsing.
- [ ] large synthetic Vault memory/performance soak.
- [ ] SD-space/error handling.
- [ ] handheld + docked readability pass.
- [ ] controller disconnect/sleep/resume pass.
- [ ] malformed-save corpus pass.
- [ ] repeated transaction interruption/recovery testing.
- [ ] release candidate physical torture test.

## Phase 31 — v1.0 release — PLANNED

- [ ] advertised support matrix matches actual verified capability.
- [ ] exact release source SHA frozen.
- [ ] host tests PASS.
- [ ] ASan/UBSan PASS.
- [ ] native release `.nro` builds.
- [ ] exact filename / byte size / SHA-256 recorded.
- [ ] persistent artifact/release available.
- [ ] release notes clearly separate read-only vs live-write capabilities.
- [ ] README/status/docs refreshed from actual release state.
- [ ] v1.0 tag/release.

---

# Current critical path

As of 2026-09-02:

```text
#23 mutation safety audit
#24 PLA crash hardening
+ preserve #19 analog source fix
+ preserve #13/#16 PokeBank NX shell
        ↓
freeze new application source if changed
        ↓
host + sanitizer + native build
        ↓
replacement exact .nro
        ↓
DEVICE TEST #2
        ↓
#4 PKSM-Core PK3/Sav3
        ↓
Gen III read adapter
        ↓
Master Vault v1
        ↓
Banks / Summary / Oracle / retro + modern reads
        ↓
conversion / Dex / legality / events
        ↓
staged writes
        ↓
per-game approved live writes
        ↓
true Move
        ↓
release hardening
        ↓
v1.0
```

Do not skip the second hardware gate to start deep save architecture merely because later roadmap items are more exciting.

---

# Supporting references

Core project docs:

- `PROJECT_STATUS.md`
- `docs/PROJECT_MAP.md`
- `docs/NEXT_SESSION_PLAN.md`
- `docs/ARCHITECTURE.md`
- `docs/SAVE_SAFETY.md`
- `docs/MASTER_VAULT_SPEC.md`
- `docs/TRANSFER_MODEL.md`
- `docs/NRO_QUALITY_ROADMAP.md`
- `docs/GAME_SUPPORT_MATRIX.md`
- `docs/UPSTREAM_AUDIT.md`
- `docs/BANK_PROJECT_REFERENCE_AUDIT_2026-09-02.md`

Primary tracking issue: **#29**.