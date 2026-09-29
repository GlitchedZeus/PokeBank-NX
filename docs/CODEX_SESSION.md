# PokeBank NX — Engineering Session Entrypoint

Last updated: **2026-09-29**

> Internal engineering file. The root README is public/product-facing and must not become a branch log or audit dump.

## Authority order

1. `CURRENT_STATUS.md`
2. `docs/CODEX_SESSION.md`
3. `docs/NEXT_CODEX_PROMPT.md`
4. `docs/PROJECT_RESOURCE_INDEX.md`

Live GitHub is authoritative over any recorded SHA or branch name.

## Git discipline

- re-fetch the active PR before changing code;
- preserve newer commits;
- never reset/rebase backward;
- never force-push over newer work;
- never push custom PokeBank NX code to PKSE upstream;
- keep application, docs and artifact SHAs distinct.

Historical prompts are evidence, not authority.

## Research routing

Use `docs/PROJECT_RESOURCE_INDEX.md` and `docs/RESEARCH_CURRENT_INDEX.md` to open only the references required by the current task.

## Safety

```text
source saves immutable
live installed/emulator writes disabled
staged app-owned editing allowed
ambiguous/unsupported input fails closed
Launch != write permission
cross-game True Move locked
```

## Current boundary

MAIN development is PR #92 and combines hardware-accepted Gen I–III editors, active Gen IV full-editor work, Product Home / Classic Game Sources, real Party/Dex/trainer presentation, Settings/Items quick actions and cursor memory.

The current goal is one exact green integrated NRO for hardware testing. Full touch controls start only after that UI candidate is physically accepted.

## Recovery

Read `docs/RECOVERY_CONTRACT.md` and recover from the live origin state. Do not assume `feature/pokebank-playable` is current.
