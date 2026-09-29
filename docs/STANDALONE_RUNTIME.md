# PokeBank NX standalone runtime contract

Last updated: **2026-09-29**

PokeBank NX is a standalone Nintendo Switch Pokémon management application. External Pokémon homebrew and PC tools may be development references or optional user tooling, but are not prerequisites for the normal PokeBank NX experience.

## Runtime ownership

Canonical PokeBank-owned root:

`sdmc:/switch/PokeBank-NX/`

PokeBank-owned runtime state includes configuration, launch bindings, logs/diagnostics, backups/working copies, exports and legacy app-owned compatibility storage.

New runtime code must not silently write into another application's namespace.

## External applications

- **RetroArch save discovery** — external read-only save source.
- **RetroArch executable/core** — optional user-invoked launch target when a proven app-owned binding exists.
- **DraStic / melonDS save discovery** — external read-only Gen IV sources where supported.
- **DraStic / melonDS executable** — optional launch target only when emulator/content handoff is proven.
- **JKSV / Checkpoint** — optional save-lifecycle tooling; not a runtime dependency.
- **PKHeX / PKSM-Core** — development/oracle/reference roles as documented elsewhere.

PokeBank NX does not require RetroArch or another emulator to run. It may launch a configured emulator/game when the user explicitly requests Launch.

## Launch ownership

Launch bindings belong to PokeBank NX.

A save path does not prove a ROM/content path. If content cannot be identified safely, request **Link Game File** and store the binding in PokeBank-owned configuration.

Launching never grants write permission.

## Safety boundary

```text
installed-title source     read-only unless a future adapter is separately approved
RetroArch source           read-only
DraStic / melonDS source   read-only
PokeBank staged workspace  app-owned
Launch permission          separate from write permission
ambiguous source/content   fail closed
```

Future writeback remains source-specific: backup → staged working copy → validation → explicit transaction → readback/rollback.
