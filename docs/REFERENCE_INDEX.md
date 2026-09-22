# PokeBank NX — Technical Reference Index

Last reviewed: 2026-09-22

This is the quick lookup map for external projects, research repositories, fixtures, and implementation oracles used while designing PokeBank NX.

It supplements the deeper audits already in this repository:

- `docs/UPSTREAM_AUDIT.md`
- `docs/BANK_PROJECT_REFERENCE_AUDIT_2026-09-02.md`
- `docs/ISSUE71_UNIVERSAL_EDITOR_REUSE_FREEZE.md`
- `docs/FUTURE_LEGAL_POKEMON_GENERATOR.md`

## Reuse classifications

- **INHERITED / DIRECT REUSE** — code is already part of PokeBank NX or may be integrated under compatible licensing after focused review.
- **ADAPTER / SELECTIVE PORT** — bounded functionality may be translated or wrapped behind PokeBank NX interfaces after provenance, license, and correctness review.
- **REFERENCE / ORACLE** — study algorithms, tests, data organization, format behavior, UX, and edge cases; independently implement the PokeBank NX version.
- **REFERENCE ONLY — LICENSE UNCLEAR** — do not copy source until permission/license is explicitly compatible.

No upstream project's write behavior overrides PokeBank NX's permanent safety policy:

```text
ORIGINAL SOURCE SAVE: IMMUTABLE
LIVE INSTALLED-GAME WRITE: HARD DISABLED
LIVE RETROARCH WRITE: HARD DISABLED
LIVE OTHER-EMULATOR WRITE: HARD DISABLED
POKEBANK STAGED EDITING: ALLOWED
UNKNOWN SAVE VARIANTS: FAIL CLOSED
```

---

## Core implementation / correctness references

### kiasta/PKSE

Classification: **INHERITED / DIRECT REUSE**

Role:

- original native Nintendo Switch foundation;
- save browsing and game-specific structures;
- inherited editor/conversion/legality code;
- modern Switch format behavior.

Rules:

- PokeBank NX changes belong on `GlitchedZeus/PokeBank-NX`;
- never push custom PokeBank NX work upstream to PKSE;
- inherited mutation paths do not bypass PokeBank NX write locks.

### FlagBrew/PKSM-Core

Classification: **INHERITED VENDOR REFERENCE / ADAPTER CANDIDATE**

Repository is already recorded as the `vendor/PKSM-Core` git submodule.

High-value areas:

- PK1/PK2/PK3/PK4/PK5 representations;
- Sav1/Sav2/Sav3/Sav4/Sav5 families;
- generation conversion;
- historical save layouts;
- strings/personal data;
- Gen III through DS-era format behavior.

Important safety note: PokeBank NX must keep strict outer validation around third-party parsers. A parser accepting a buffer is not enough proof that the save is structurally trustworthy.

### kwsch/PKHeX

Classification: **PRIMARY CORRECTNESS ORACLE / REFERENCE**

Use for:

- save-layout and Pokémon-format comparison;
- legality/encounter behavior;
- conversion semantics;
- item/move/location datasets;
- generation/version edge cases;
- test-fixture comparison.

Do not equate “PKHeX accepts it” with PokeBank NX provenance or safe-write guarantees. PokeBank NX has stricter custody, immutability, and transaction requirements.

### pkHouse

Classification: **REFERENCE / SELECTIVE ALGORITHM COMPARISON**

Use for modern Switch format behavior and cross-generation handling where relevant. Do not inherit direct-write assumptions without independent safety proof.

---

## Bank / storage / transfer references

### FlagBrew/PKSM

Classification: **REFERENCE / SELECTIVE ADAPTER SOURCE**

High value:

- BankFile format/versioning;
- migration;
- typed parse failures;
- backup rotation;
- hash-based change detection;
- search/sort/editor UX;
- Wondercard/event workflows;
- real constrained-Nintendo integration of PKSM-Core.

Do not replace PokeBank NX Master Vault with a flat PKSM Bank. Use it to pressure-test our parser, migration, backup, and legacy-import design.

### Universal-Team/pkmn-chest

Historical link may appear under SNBeast; the active project is under Universal-Team.

Classification: **HIGH-VALUE REFERENCE / SELECTIVE PORT AFTER REVIEW**

High value:

- Generation III–V offline Pokémon bank;
- PKSM-Core integration outside PKSM itself;
- game-save side vs Bank side architecture;
- bank persistence;
- sorting and summary flows;
- transfer UX;
- constrained DS memory/storage failure modes.

Useful future comparison targets include its bank implementation, save adapters, transfer flow, and recent bounds-hardening history.

### gocario/PHBank

Classification: **REFERENCE / UX REFERENCE**

High value:

- direct Game ↔ Bank interaction;
- single/multiple/whole-box movement;
- summary and IV/EV presentation;
- explicit save/backup flow.

Borrow the source/destination mental model, not its direct-write safety assumptions.

### 0xb01u/PHBankGBC / PHBankGB lineage

Classification: **REFERENCE ONLY — LICENSE/PROVENANCE NEEDS CARE**

Useful as a secondary historical Gen I/II layout cross-check. Some old title IDs and format details are incomplete or uncertain. Never prefer it over PKSM-Core, PKHeX, primary research, or golden fixtures.

### JostenSyon/OpenHomeNX

Classification: **HIGH-VALUE REFERENCE / SELECTIVE PORT AFTER REVIEW**

High value:

- intermediate Pokémon representation;
- original-byte backup concepts;
- DS/3DS parsers;
- namespaced backup layout;
- transfer-loss UX;
- import scanning;
- Wondercards;
- updater manifest + SHA + temporary-file replacement;
- box/search/multi-select workflows.

Do **not** adopt its live installed-save writes. Its storage/write paths remain things to audit, not a safety authority.

### uNSS transaction model

Classification: **REFERENCE**

Use as a transaction/custody design reference when implementing destination-first commit, rollback, and crash recovery.

### SafeReplaceFile / atomic replacement research

Classification: **DESIGN REQUIREMENT / REFERENCE**

Target properties for mutable PokeBank-owned persistence:

```text
write temporary image
flush data
close and verify
preserve previous generation
atomic rename/replace where filesystem semantics permit
recover deterministically from target/tmp/backup combinations
never discard the only recoverable copy
```

Must be tested on the actual SD-card filesystems we support, especially FAT32/exFAT behavior.

---

## Save discovery / emulator references

### DrasticDS_nx

Classification: **REFERENCE**

Future value: DS emulator save discovery, path conventions, Switch integration patterns.

### RetroArch

Classification: **RUNTIME SOURCE / REFERENCE**

Current Gen I–III discovery source. PokeBank NX reads these source saves but keeps live RetroArch writes hard-disabled.

### Additional future emulator adapters

Track exact adapters rather than one generic “emulator save” path. Planned research includes mGBA, melonDS, DraStic, Azahar and user-selected folders.

---

## Legality / encounter / event references

### projectpokemon/Pokemon-Legality-Checker

Classification: **REFERENCE ONLY — NO LICENSE FOUND**

Important repository facts as of this review:

- old C#/.NET legality checker;
- repository source is a WIP;
- original Beta54 source was reported lost;
- maintainer stated fixes/redos submitted as PRs would be considered;
- repository currently has an open issue specifically noting that no license file exists.

Therefore:

**Do not copy or port its source into PokeBank NX unless the rights holder supplies explicit compatible permission/license.**

Historical research value:

- Gen III R/S/E/FR/LG/XD checks;
- Gen III → Gen IV Pal Park behavior;
- PID/IV/RNG relationships;
- chained shiny checks;
- gender/PID checks;
- event/egg/wild/roaming templates;
- trash-byte validation;
- Pokéwalker logic;
- Mystery Gift/event logic;
- Gen IV and early BW legality ideas.

Because the code/research is old, treat it as an independent historical oracle and compare every useful rule with modern PKHeX/PKSM-Core and game evidence.

### FlagBrew/PKSM-Scripts

Classification: **REFERENCE / DATA RESEARCH**

High value later for:

- Gen IV–VII save-layout documentation;
- event constants/flags;
- Wondercard/Mystery Gift storage;
- scripts showing concrete offsets and state transitions;
- save/event diff tooling ideas.

The project itself warns that some scripts deliberately create illegal states. Never treat a script merely existing as proof that a value is legal.

### EventsGallery

Classification: **REFERENCE / DATA SOURCE SUBJECT TO PERMISSION**

Potential future Event Vault/EventDex source. Redistribution/embedding permissions are tracked separately; do not silently import event data into releases before that is resolved.

### PoGoEncTool

Classification: **REFERENCE**

Useful for encounter/generation research and future legal-generation work; integrate only after comparing with current PKHeX legality behavior and PokeBank provenance requirements.

---

## Native game / decompilation references

### pret/pokediamond

Historical links may appear under SNBeast; the current project is under pret.

Classification: **REFERENCE / NATIVE GAME ORACLE**

Future Gen IV value:

- Diamond/Pearl save logic;
- checksums;
- party/storage;
- Pokédex;
- item/event behavior;
- actual game-side semantics when editor implementations disagree.

It is a WIP decompilation, so confirm the relevant function is actually matched/understood before treating it as authoritative.

### Other pret decomps / disassemblies

Classification: **REFERENCE**

When available, prefer native game behavior for questions about what the game actually accepts, stores, repairs, or recalculates.

---

## Fixture / interoperability references

### Official Pokémon Bank → HOME fixtures

Classification: **GOLDEN FIXTURE / PRESERVATION TARGET**

Keep official transfer outputs as non-destructive fixtures for conversion, provenance, and metadata comparison. Never rewrite the only copy of a fixture during tests.

### PokéBridge

Classification: **REFERENCE**

Useful for interoperability/transfer ideas where applicable; compare behavior rather than assuming format equivalence.

### PKHeX test / blank-save machinery

Classification: **TEST REFERENCE**

Useful for malformed-save corpus design, valid minimum fixtures, parser comparison, and regression tests.

---

# When to use which reference

## Gen I–III editor correctness

```text
PKHeX
PKSM-Core
PKSE inherited implementation
golden save/Pokémon fixtures
native-game research where available
```

## Gen III–V bank / DS work

```text
PKSM-Core
pkmn-chest
PKSM
PKHeX
pret/pokediamond for DP-specific native behavior
```

## Legality & Provenance engine

```text
PKHeX (primary modern oracle)
PKSM-Core
Pokemon-Legality-Checker (historical, reference only)
PKSM-Scripts (event/save research)
native game/decomp evidence
known official event/transfer fixtures
```

## Master Vault / durable storage

```text
PokeBank NX's own immutable-entity/provenance model
PKSM BankFile parser/migration tests
pkmn-chest bank integration
OpenHomeNX original-backup/provenance ideas
SafeReplaceFile/transaction research
uNSS transaction concepts
```

## Transfer workspace

```text
PHBank UX
pkmn-chest two-side model
OpenHomeNX transfer UX
PokeBank NX destination-first durable transaction rules
```

---

# Research intake rule

When a new repository is discovered, record:

1. canonical repository/owner;
2. exact commit or tag inspected;
3. license and attribution requirements;
4. whether source reuse is allowed;
5. what subsystem it can help;
6. what it must **not** be trusted for;
7. exact files/functions/data worth revisiting;
8. tests or fixtures we can derive independently;
9. whether the finding changes the roadmap.

Do not dump unclassified links into implementation prompts. The purpose of this index is to make future research findable without turning every reference into a dependency.
