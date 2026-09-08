# PokeBank NX — Codex Session Entrypoint

> **Codex/internal engineering file.** The root `README.md` is the human-facing project dashboard and must not be used to expand implementation scope.

For every coding session:

1. Preserve all local/uncommitted/recovery work before syncing, resetting, cleaning, restoring, switching refs, rebasing, or changing worktrees.
2. Read `CURRENT_STATUS.md` for the exact current engineering state.
3. Read and execute `docs/NEXT_CODEX_PROMPT.md` as the authoritative task prompt.
4. Treat `README.md`, `docs/FUTURE_PRODUCT_VISION.md`, `docs/FEATURE_FEASIBILITY_MATRIX.md`, `docs/V1_ROADMAP.md`, `docs/V2_ROADMAP.md`, and other product-planning documents as future context only unless `docs/NEXT_CODEX_PROMPT.md` explicitly promotes something into the active milestone.
5. Push coherent checkpoints only to `origin/feature/pokebank-playable` unless the authoritative prompt says otherwise.
6. Never push PokeBank NX custom code upstream to PKSE.
7. Use HIGH reasoning.

Short launcher for chat:

```text
Continue PokeBank NX on feature/pokebank-playable. Use HIGH reasoning. Preserve all local/recovery work before touching refs. Read CURRENT_STATUS.md and docs/CODEX_SESSION.md, then execute docs/NEXT_CODEX_PROMPT.md exactly. Push coherent checkpoints only to origin/feature/pokebank-playable; never push custom code upstream.
```
