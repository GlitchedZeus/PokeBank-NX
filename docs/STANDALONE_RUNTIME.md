# PokeBank NX standalone runtime contract

PokeBank NX is a standalone all-in-one Nintendo Switch Pokémon management application. External Pokémon homebrew and PC tools may be development references or optional migration tools, but are not prerequisites for normal advertised operation.

## Runtime ownership

Canonical PokeBank-owned root:

`sdmc:/switch/PokeBank-NX/`

Current owned paths are produced through `Utils/PokeBankPaths.h`. New runtime code must not create files under `/PKSE/`, PKSM, JKSV, Checkpoint, pkDex, pkHouse, or another application's namespace.

Current owned subtrees used by implemented features are:

- `config/` — settings and legacy-source bindings;
- `logs/` — opt-in diagnostics;
- `backups/` — PokeBank-created installed-title backup workspaces;
- `exports/gen2/` — staged Generation II exports;
- `banks/legacy-pkse/` — inherited bank format retained only as legacy app-owned storage. Its `PKSEBANK` magic remains unchanged for format compatibility and it is **not** the future Master Vault.

Existing `/PKSE/` data is not silently moved, deleted, overwritten, or required. A future explicit importer may offer optional migration.

## Runtime dependency audit classification

- inherited PKSE-derived C++ compiled into the NRO: INTERNAL COMPILED CODE — OK;
- `PokeVault::` and `PKSEFramebuffer` internal symbols: INTERNAL COMPILED CODE — OK;
- pinned PKSM-Core host/oracle and narrow adapter references: INTERNAL/DEVELOPER REFERENCE — OK, with required notices retained;
- PKHeX: DEVELOPER/TEST ORACLE — OK, not an end-user runtime requirement;
- pkDex and pkHouse: DEVELOPER/RESEARCH REFERENCES — OK;
- JKSV and Checkpoint: save-lifecycle/reference/optional user tooling only — not required;
- RetroArch paths: external SAVE SOURCE discovery — allowed; RetroArch is not invoked as a helper;
- `/PKSE/` runtime writes: FOREIGN APP-OWNED WRITE PATH — removed from normal runtime;
- visible PokeVault wording in the staged Gen II exporter: USER-VISIBLE LEGACY BRANDING — removed.

## Safety boundary

Standalone ownership does not authorize direct source mutation. Live installed-title writes and live RetroArch writes remain hard-disabled by policy. The Generation II staged editor continues to preserve original bytes, edit an independent buffer, repair checksum/mirrors, strict-reload, preserve supported RTC footer bytes, and export a separate edited save plus original backup and manifest.

Japanese Generation II remains read-only.
