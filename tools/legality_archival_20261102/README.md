# Verified legality archival tools (2026-11-02)

These are the actual one-session inventory, safety-audit, packaging, upload, verification and recovery sources. They contain public repository references and historical local layout paths, never credential values or private conversations. The uploader obtains the configured GitHub credential in memory and never writes it to a log.

See `docs/legality/archival-20261102-5fea7345/REPORT.md` and `RECOVERY.md` for supported recovery steps. The historical packaging/publication scripts intentionally use the frozen session layout and refuse overwriting their evidence outputs; they are provenance, not an instruction to republish or delete data.

`restore_evidence_v4.py` restores downloaded content to a new destination. `cleanup_guard_v4.py` defaults to dry-run validation. Its explicit `--execute` option is for later user-reviewed cleanup only; no cleanup was run during archival. Original source trees, Git metadata, global/private Codex state, SDK/public-source caches and local fuzz/corpus/probe originals remain excluded.

Large evidence, full manifests and raw measurements are GitHub Release assets. Only the small index, guides, proof summaries and tools are stored in Git. The engine evidence tag remains at 2144a682; this archival documentation must not move PR #314 or merge application branches.
