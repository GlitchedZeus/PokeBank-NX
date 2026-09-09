# PokeBank NX — Current Research Router

Last updated: 2026-09-09

> **Research/navigation only.** This file does not expand the active coding milestone. `CURRENT_STATUS.md`, `docs/CODEX_SESSION.md`, and `docs/NEXT_CODEX_PROMPT.md` remain authoritative.

This is the short entry point for the large research sweep completed on September 9, 2026. The goal is to preserve the findings without forcing Codex/Astra to reread thousands of lines of research every session.

## AI rule

For a normal coding session:

```text
CURRENT_STATUS.md
    -> docs/CODEX_SESSION.md
    -> docs/NEXT_CODEX_PROMPT.md
    -> docs/PROJECT_RESOURCE_INDEX.md
    -> ONLY the relevant research note below, if needed
```

Do not read every research note by default.

---

## Immediate / P0

### Switch SD file replacement / current LeafGreen bug

Open:

```text
docs/SWITCH_FILESYSTEM_SAFE_REPLACE_RESEARCH_2026-09-09.md
```

Key findings preserved there:

- libnx `fsdev` delegates rename to `fsFsRenameFile()` and does not supply desktop POSIX replace-existing behavior;
- `DNS-MITM_Manager` demonstrates a real Switch `tmp -> bak -> target` update pattern;
- `badpiggies_nx` independently documents both destination-exists behavior and the additional open-handle rename/delete hazard;
- ordinary `sdmc:` file replacement must remain separate from mounted Horizon savedata commit semantics;
- the current LeafGreen assignment failure should log stage + errno/native result rather than only `save failed`.

### Time-sensitive Bank -> HOME preservation

Open:

```text
docs/research/2026-09-09/BANK_HOME_PRESERVATION_CAPTURE_PLAN.md
```

The official Bank shutdown is February 25, 2027 at 7:00 p.m. PST / February 26, 2027 at 03:00 UTC. New Bank -> HOME captures disappear after that deadline, so this is a preservation task to schedule during 2026 even though Gen VII/VIII implementation is future scope.

The note records the HOME-native capture model and the HOME Live Plugin dumper/reference distinction.

---

## Save formats, structural revisions, profiles, wrappers and localization

Open:

```text
docs/research/2026-09-09/SAVE_FORMATS_PROFILES_CONTAINERS.md
```

Contains the September findings on:

- SwSh/SV/BDSP/PLA/Z-A structural save revisions and migration-path-dependent layouts;
- `StructuralRevision` vs installed `GameVersion`;
- unknown revision => read-only/unsupported;
- Switch save identity vs current human-visible profile identity;
- orphan/deleted profile handling;
- save container metadata such as journal/data sizes;
- Gen I/II RTC sidecars/trailers and wrapper preservation;
- DeSmuME `.dsv` normalization;
- No$GBA compressed-save normalization;
- DraStic and Azahar NX discovery paths;
- Japanese/Korean/Western string encoding, terminators and trash-byte preservation.

---

## Transfers, HOME semantics, provenance and golden fixtures

Open:

```text
docs/research/2026-09-09/TRANSFER_FIXTURES_PROVENANCE.md
```

For the shutdown/deadline capture campaign also open:

```text
docs/research/2026-09-09/BANK_HOME_PRESERVATION_CAPTURE_PLAN.md
```

Contains:

- sequential generation-boundary conversion architecture;
- PKHeX + PKSM-Core as independent behavioral oracles;
- `Poke_Transporter_GB` + PCCS Gen I/II -> III policies;
- real Pal Park / Poké Transfer / Bank / HOME research fixtures;
- multilingual NPC Gift/Trade PKM corpus;
- Project Pokémon public save corpuses;
- RoC's PC as a bulk pairable official-path capture corpus for IV->V, V->VI and VI->VII;
- fixture manifest / hashes-only policy when redistribution rights are unclear;
- HOME Tracker preservation rule: preserve observed trackers, never fabricate official HOME provenance;
- Handling Trainer / HOME sidecar / modern representation concerns;
- time-sensitive Bank -> HOME native-state capture planning.

---

## Vault, data packs, legality, events, fuzzing and performance

Open:

```text
docs/research/2026-09-09/VAULT_DATA_EVENTS_PERFORMANCE.md
```

For exact future durability testing open:

```text
docs/research/2026-09-09/POWERLOSS_DURABILITY_TEST_PLAN.md
```

Contains:

- immutable/content-addressed Vault objects;
- versioned manifests and crash-safe migrations;
- SQLite-on-Switch findings and why desktop WAL defaults should not be assumed safe;
- rebuildable/expendable search indexes;
- large-Vault benchmark requirements;
- packed immutable segments as the leading architecture to benchmark against flat-file baselines;
- PKSM Mystery Gift pack architecture;
- PKHeX BinLinker / `.pkl` architecture and PoGoEncTool generator pattern;
- proposed PokeBank binary data-pack generator;
- event redistribution/license uncertainty;
- `wcparse` as a secondary Wonder Card oracle;
- PokéAPI/Veekun for generic metadata generation;
- host-side libFuzzer/AFL++ parser fuzzing;
- staged-session/backup lessons from PKVault;
- disposable Horizon/sdmc power-cut checkpoint planning.

---

## Ribbon / Bank-exclusive / DNS-event preservation planning

Open:

```text
docs/research/2026-09-09/RIBBON_BANK_EVENT_PRESERVATION_REFERENCES.md
```

This routes three community references without promoting them directly into legality truth:

- Hallowed Tower DNS Exploit / Mystery Gift tables;
- Athis' Ribbon Handbook and Bank-deadline Ribbon Master planning;
- the r/PokemonHome Pokémon Bank Exclusives Masterpost.

Use them as preservation/checklist/workflow leads and verify hard game/event rules independently.

---

## Legacy-console recovery, GameCube, Pokémon Box and Stadium

Open:

```text
docs/research/2026-09-09/LEGACY_RECOVERY_GAMECUBE_STADIUM.md
```

Contains:

- Gen III redundant-sector recovery;
- Gen IV general/storage recovery rules;
- game-specific recovery framework guidance;
- Pokémon Box: Ruby & Sapphire format research;
- Stadium / Stadium 2 format + region/endian research;
- Colosseum/XD reference stack: PKHeX + PokéBridge + PkmGCTools/LibPkmGC + Dolphin;
- wrapper/container normalization and recovery-report ideas;
- preservation rule: never silently modify the damaged original.

---

## Existing reference matrix remains relevant

Also keep:

```text
docs/RESEARCH_REFERENCE_MATRIX.md
docs/RESEARCH_INTAKE_2026-09-09.md
docs/GAMECUBE_REFERENCE_AUDIT_2026-09-09.md
docs/research/2026-09-09/SOURCE_MANIFEST.md
```

The matrix still classifies external projects by integration role/license posture. The September intake is the earlier condensed research pass. The dedicated notes above are the newer routing-friendly consolidation of tonight's broader sweep.

Important already-indexed references include:

```text
PKHeX
PKSM-Core
current upstream kiasta/PKSE
pkHouse / pkBakery
Poke_Transporter_GB
Pokemon-Community-Conversion-Standard (PCCS)
PokeFinder
PoGoEncTool
EventsGallery
Switch-Gift-Data-Manager
JKSV / libnx
DrasticDS_nx
PokéBridge
PkmGCTools / LibPkmGC
Dolphin
RoC's PC
HOME Live Plugin
hactool / LibHac
```

---

## Research conclusions promoted to permanent design rules

These are architectural constraints, not implementation claims:

```text
1. GameVersion != StructuralRevision.
2. Unknown save revision is never automatically writable.
3. Save-container identity != current profile display identity.
4. Preserve raw/original Pokémon and encoded text bytes when untouched.
5. Preserve unknown wrapper bytes instead of silently normalizing them away.
6. Emulator/container normalization belongs before the generation parser.
7. Recovery framework may be generic; candidate-selection rules are game-specific.
8. Conversion is sequential boundary logic, not one opaque any->any converter.
9. OfficialObserved / OfficialModeled / CompatibilityConversion provenance are distinct.
10. Never fabricate HOME Trackers or official server history.
11. SD-file transactions and Horizon savedata transactions are separate abstractions.
12. Vault Pokémon payloads should be immutable/content-addressed where practical.
13. Search/index databases are rebuildable caches, not the only copy of the collection.
14. Host-generated compact data packs are preferred over giant runtime JSON/SQLite datasets.
15. Fixture provenance/redistribution status is tracked separately from technical usefulness.
16. Parser safety should eventually include fuzzing + corruption fixtures + hardware tests.
17. Flat one-Pokémon-per-file Vault storage is a benchmark baseline, not the preferred production design.
18. Community re-delivery of an event is not proof of original official redemption.
19. Bank->HOME official-path capture is deadline-bound preservation work before February 2027.
```

---

## Still genuinely unresolved after the sweep

Research reduced many unknowns, but these remain intentionally open:

1. exact Switch savedata electrical durability after specific transaction checkpoints — requires sacrificial hardware testing;
2. real 10k/50k/100k/250k Vault performance on Switch — requires `vaultbench.nro` rather than guesses;
3. redistribution rights for wholesale official Mystery Gift/event corpuses — do not assume public availability means bundle permission;
4. final implementation/license review for every external source before any code/data is copied;
5. complete modern HOME-native before/after coverage across every game/language route.

Classic transfer boundaries III->IV, IV->V, V->VI and VI->VII are now considered sufficiently researched to support later differential implementation/testing, subject to fixture provenance and redistribution checks.

Do not reopen already-solved research unless implementation evidence contradicts these notes.
