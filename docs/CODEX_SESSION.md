# PokeBank NX — Codex Session Entrypoint

> **Codex/internal engineering file.** The root `README.md` is the user's human-facing project dashboard. Do not rewrite it or use it to expand implementation scope unless the user explicitly asks for README work.

## Authority order

For every coding/recovery session, use this order:

1. `CURRENT_STATUS.md` — exact current engineering state.
2. `docs/CODEX_SESSION.md` — session discipline and authority rules.
3. `docs/NEXT_CODEX_PROMPT.md` — authoritative active task, scope and STOP point.
4. `docs/PROJECT_RESOURCE_INDEX.md` — navigation only; use it to find supporting docs/resources relevant to the active task.

If an old roadmap, build record, session log, project map, prompt, issue, README section, or research file conflicts with the first three files above, the first three win for the current session.

## Session rules

1. Preserve all local/uncommitted/recovery/build/generated-asset work before syncing, resetting, cleaning, restoring, switching refs, rebasing, deleting generated state, or changing worktrees.
2. Read `CURRENT_STATUS.md` before deciding what source/artifact state is current.
3. Read and execute `docs/NEXT_CODEX_PROMPT.md` as the authoritative task prompt.
4. Use `docs/PROJECT_RESOURCE_INDEX.md` to open only the detailed docs relevant to the active subsystem instead of reading the entire repository.
5. Check `docs/RESEARCH_REFERENCE_MATRIX.md` before independently rebuilding/researching a Pokémon format, transfer rule, event format, protocol, artwork pipeline, or helper feature that may already have a mature community reference.
6. Treat `README.md`, `docs/FUTURE_PRODUCT_VISION.md`, `docs/FEATURE_FEASIBILITY_MATRIX.md`, `docs/V1_ROADMAP.md`, `docs/V2_ROADMAP.md`, `docs/MODDED_SWITCH_FEATURE_BACKLOG.md`, `docs/PRODUCT_DECISIONS.md`, and other product-planning documents as future context only unless `docs/NEXT_CODEX_PROMPT.md` explicitly promotes something into the active milestone.
7. Do not treat `docs/PROJECT_MAP.md`, `docs/NEXT_SESSION_PLAN.md`, old `PROMPT_*` files, old session logs, or earlier sections of `BUILD_RECORD.md` as current state unless the authoritative prompt explicitly points to them. They are preserved historical/recovery evidence and may contain older "current" states.
8. Reuse completed verification and generated assets when the exact application source is unchanged and the authoritative prompt permits reuse. Do not rerun expensive work merely because a session restarted.
9. Push coherent checkpoints only to `origin/feature/pokebank-playable` unless the authoritative prompt says otherwise.
10. Never push PokeBank NX custom code upstream to PKSE.
11. Use HIGH reasoning.
12. Stop when the authoritative prompt reaches its STOP condition; do not roll into the next roadmap feature automatically.

## Current dedicated recovery launcher

For the recovery session whose goal is to finish the corrected FRLG artifact and preserve/publish it to GitHub, use:

```text
docs/PROMPT_RECOVERY_PUBLISH_NRO_2026-09-09.md
```

That recovery prompt is a launcher/packaging clarification. `CURRENT_STATUS.md` and `docs/NEXT_CODEX_PROMPT.md` remain authoritative for application source and engineering scope.

## Short launcher for normal chat

```text
Continue PokeBank NX on feature/pokebank-playable. Use HIGH reasoning. Preserve all local/recovery/build/generated-asset work before touching refs or worktrees. Read CURRENT_STATUS.md and docs/CODEX_SESSION.md, then execute docs/NEXT_CODEX_PROMPT.md exactly. Use docs/PROJECT_RESOURCE_INDEX.md only as supporting navigation. Push coherent checkpoints only to origin/feature/pokebank-playable; never push custom code upstream.
```
