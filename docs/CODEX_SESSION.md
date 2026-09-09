# PokeBank NX — Codex Session Entrypoint

> **Codex/internal engineering file.** The root `README.md` is the user's human-facing project dashboard. Do not rewrite it or use it to expand implementation scope unless the user explicitly asks for README work.

## Authority order

For every coding/recovery session, use this order:

1. `CURRENT_STATUS.md` — exact current engineering state.
2. `docs/CODEX_SESSION.md` — session discipline and authority rules.
3. `docs/NEXT_CODEX_PROMPT.md` — authoritative active task, scope and STOP point.
4. `docs/PROJECT_RESOURCE_INDEX.md` — navigation only.

If an old roadmap, build record, session log, project map, prompt, issue, README section or research file conflicts with the first three files above, the first three win for the current session.

## Resource selection rule

After reading the three authority files, use `docs/PROJECT_RESOURCE_INDEX.md` to select only the supporting material needed for the task.

Default pattern:

```text
one game-family chunk
        +
zero/one relevant cross-cutting chunk
```

Examples:

```text
FRLG save bug     -> GBA chunk + Safety/Build only if needed
3DS save parser   -> 3DS chunk
Vault work        -> Vault chunk
GB -> GBA transfer -> GB/GBC chunk + Transfer chunk
Create Pokémon    -> Create + Legality chunk
```

Do not read every generation, roadmap or research file before starting.

## Session rules

1. Preserve all local/uncommitted/recovery/build/generated-asset work before syncing, resetting, cleaning, restoring, switching refs, rebasing, deleting generated state or changing worktrees.
2. Read `CURRENT_STATUS.md` before deciding what source/artifact state is current.
3. Read and execute `docs/NEXT_CODEX_PROMPT.md` as the authoritative task prompt.
4. Use `docs/PROJECT_RESOURCE_INDEX.md` to open only the relevant game-family/system chunks.
5. Check `docs/RESEARCH_REFERENCE_MATRIX.md` before independently rebuilding/researching a Pokémon format, transfer rule, event format, protocol, artwork pipeline or helper feature that may already have a mature reference.
6. Treat `README.md`, future vision/roadmap/backlog docs and product planning as context only unless `NEXT_CODEX_PROMPT.md` explicitly promotes something into the active milestone.
7. Treat `docs/history/`, old `PROMPT_*` files, session logs, `PROJECT_MAP.md`, `NEXT_SESSION_PLAN.md`, old device reports and older `BUILD_RECORD.md` sections as historical/recovery evidence, not current state, unless the authoritative prompt points to them.
8. Reuse completed verification and generated assets when the exact application source is unchanged and the authoritative prompt permits reuse. Do not rerun expensive work merely because a session restarted.
9. Push coherent checkpoints early to `origin/feature/pokebank-playable` unless the authoritative prompt says otherwise.
10. Never push PokeBank NX custom code upstream to PKSE.
11. Keep application-source SHA, documentation SHA and artifact SHA distinct.
12. Use HIGH reasoning.
13. Stop at the authoritative prompt's STOP condition; do not roll into the next roadmap feature automatically.

## Historical prompts / recovery material

Completed or superseded prompt/status snapshots are preserved under:

```text
docs/history/
```

Older standalone recovery/session prompts remain in `docs/` for archaeology and exact historical procedures. Do not execute one merely because its filename sounds relevant; `NEXT_CODEX_PROMPT.md` must explicitly point to it first.

## Short launcher for normal chat

```text
Continue PokeBank NX on feature/pokebank-playable. Use HIGH reasoning. Before touching refs or worktrees, preserve all local/uncommitted/recovery/build/generated-asset work. Read CURRENT_STATUS.md and docs/CODEX_SESSION.md, then execute docs/NEXT_CODEX_PROMPT.md exactly. Use docs/PROJECT_RESOURCE_INDEX.md to open only the relevant game/system chunk. Push coherent checkpoints early only to origin/feature/pokebank-playable; never push custom code upstream.
```
