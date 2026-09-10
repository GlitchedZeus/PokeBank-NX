# PokeBank NX — Codex Session Entrypoint

> **Codex/internal engineering file.** The root `README.md` is the user's human-facing project dashboard. Do not rewrite it or use it to expand implementation scope unless the user explicitly asks for README work.

## Authority order

For every normal coding session, use this order:

1. `CURRENT_STATUS.md` — exact current engineering state.
2. `docs/CODEX_SESSION.md` — session discipline and authority rules.
3. `docs/NEXT_CODEX_PROMPT.md` — authoritative active task, scope and STOP point.
4. `docs/PROJECT_RESOURCE_INDEX.md` — navigation only.

If an old roadmap, build record, session log, project map, prompt, issue, README section or research file conflicts with the first three files above, the first three win for the current session.

## RECOVERY means RECOVER — not archaeology

When the user explicitly says **RECOVERY**, the normal path is now defined by:

```text
docs/RECOVERY_CONTRACT.md
recovery/RECOVERY_STATE.json
tools/recover_workspace.py
```

Routine recovery:

```text
origin/feature/pokebank-playable
        ↓
python3 tools/recover_workspace.py
        ↓
RECOVERY COMPLETE
        ↓
continue the active task
```

Do **not** start a routine RECOVERY by manually walking every worktree, reflog, stash, old prompt and recovery branch.

Forensic Git/worktree archaeology is an exception path only when:

- the normal GitHub recovery command fails;
- a required remote commit is genuinely missing;
- Git history has diverged unexpectedly; or
- the user explicitly asks to rescue unsaved local-only work.

The session-save contract is equally strict: no meaningful project-authored fix may exist only in `romfs/`, `build/`, `/mnt/data/`, a temporary Codex worktree, or an unpushed commit. Generated output may remain generated, but the generator/source pin/mapping/transform/legally-safe override required to reproduce it must be on GitHub.

Do not call temporary session downloads "durable storage" unless future sessions can actually retrieve them. GitHub source + recovery state is the permanent project source of truth.

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
FRLG save bug      -> GBA chunk + Safety/Build only if needed
3DS save parser    -> 3DS chunk
Vault work         -> Vault chunk
GB -> GBA transfer -> GB/GBC chunk + Transfer chunk
Create Pokémon     -> Create + Legality chunk
```

Do not read every generation, roadmap or research file before starting.

## Research routing rule

When the active task needs external/reference research, open:

```text
docs/RESEARCH_CURRENT_INDEX.md
```

first.

That file routes the September 9 research sweep into small subsystem notes for:

```text
save revisions / profiles / containers / localization
transfers / fixtures / HOME provenance
Vault / data packs / events / fuzzing / performance
legacy recovery / GameCube / Pokémon Box / Stadium
Switch safe file replacement
```

Then open only the one note relevant to the active task.

`docs/RESEARCH_REFERENCE_MATRIX.md` remains the classification/license-oriented master list of external projects. `docs/RESEARCH_INTAKE_2026-09-09.md` remains the earlier condensed intake. Do not read all three files automatically.

## Session rules

1. For routine RECOVERY, follow `docs/RECOVERY_CONTRACT.md` and run `python3 tools/recover_workspace.py`; do not substitute broad archaeology.
2. For a normal coding session, read `CURRENT_STATUS.md` before deciding what source/artifact state is current.
3. Read and execute `docs/NEXT_CODEX_PROMPT.md` as the authoritative task prompt.
4. Use `docs/PROJECT_RESOURCE_INDEX.md` to open only the relevant game-family/system chunks.
5. Before independently rebuilding/researching a Pokémon format, transfer rule, event format, protocol, artwork pipeline or helper feature that may already have a mature reference, check `docs/RESEARCH_CURRENT_INDEX.md` and then the relevant dedicated note / `docs/RESEARCH_REFERENCE_MATRIX.md` classification entry.
6. Treat `README.md`, future vision/roadmap/backlog docs and product planning as context only unless `NEXT_CODEX_PROMPT.md` explicitly promotes something into the active milestone.
7. Treat `docs/history/`, old `PROMPT_*` files, session logs, `PROJECT_MAP.md`, `NEXT_SESSION_PLAN.md`, old device reports and older `BUILD_RECORD.md` sections as historical/recovery evidence, not current state, unless the authoritative prompt points to them.
8. Reuse completed verification and generated assets when the exact application source is unchanged and the authoritative prompt permits reuse. Do not rerun expensive work merely because a session restarted.
9. Push coherent checkpoints early to `origin/feature/pokebank-playable` unless the authoritative prompt says otherwise.
10. Before calling a session safely saved, every project-authored code/test/mapping/generator/manual correction must be committed, pushed and remote-verified. A generated/manual fix may not live only in ignored `romfs/` output.
11. Never push PokeBank NX custom code upstream to PKSE.
12. Keep application-source SHA, documentation SHA and artifact SHA distinct.
13. Use HIGH reasoning.
14. Stop at the authoritative prompt's STOP condition; do not roll into the next roadmap feature automatically.

## Historical prompts / recovery material

Completed or superseded prompt/status snapshots are preserved under:

```text
docs/history/
```

Older standalone recovery/session prompts remain in `docs/` for archaeology and exact historical procedures. Do not execute one merely because its filename sounds relevant; `NEXT_CODEX_PROMPT.md` must explicitly point to it first.

## Short launcher for normal coding

```text
Continue PokeBank NX on feature/pokebank-playable. Use HIGH reasoning. Read CURRENT_STATUS.md and docs/CODEX_SESSION.md, then execute docs/NEXT_CODEX_PROMPT.md exactly. Use docs/PROJECT_RESOURCE_INDEX.md to open only the relevant game/system chunk and docs/RESEARCH_CURRENT_INDEX.md only when reference research is needed. Push coherent checkpoints early only to origin/feature/pokebank-playable; never push custom code upstream.
```

## Short launcher when the user says RECOVERY

```text
RECOVERY. Use origin/feature/pokebank-playable as the normal source of truth. Read docs/RECOVERY_CONTRACT.md, CURRENT_STATUS.md and docs/NEXT_CODEX_PROMPT.md, then run `python3 tools/recover_workspace.py`. If it prints RECOVERY COMPLETE, continue the active task immediately. Do not inspect reflogs/worktrees/history unless that deterministic recovery path fails or I explicitly ask you to rescue unsaved local-only work.
```
