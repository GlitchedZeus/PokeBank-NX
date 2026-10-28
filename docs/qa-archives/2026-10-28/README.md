# Independent QA evidence — 2026-10-28

**Archive only. Do not merge this branch or apply patches without review.**

These are unmerged, locally verified patches and Git bundles delivered by the independent Claude API QA agent. The archive contains 15 entries (including its own README and SHA-256 manifest) collected from the user's uploads. It contains **no original game saves**.

## Download

[QA evidence ZIP](./PokeBank-NX-independent-QA-2026-10-28.zip)

SHA-256 of ZIP: `e6b9b0edf522fee277972f3a29f5ba7b9419084f32a0b2b287680652a3eeed1f` (46,710 bytes).

## Contents

- `campaign-1/`: Gen IV Bag Remove compaction patch and the original QA Git bundle (`dabba3cb`, `b09a0169`).
- `campaign-2/`: updated Gen IV Berry/TM Add sorting and PKSE issue #147 receiving Handling Trainer patch series, original QA Git bundle (`39431be2`, `7b164d70`, `62b76716`, `1c4a5ea1`), standalone handler fix and regression patch, provenance, SHA-256 manifest, report and selected test logs.

The `MANIFEST.sha256` inside the ZIP records exact checksums for its preserved files; `campaign-2/SHA256SUMS.txt` is the original QA agent's manifest and includes some files **not** present in this ZIP.

## Safety and integration rules

- This archive is not a released, audited, or hardware-tested NRO.
- Do not merge `archive/qa-campaigns-2026-10-28` into `main` or `integration/gen15-combined`.
- Do not apply the standalone patches *and* full series for the same change, or duplicate edits.
- Rebase/review on a dedicated local branch against the current source, add regression tests, and run complete gates.
- The #147 Handling Trainer patch is **not release-ready** until the receiving trainer language and potential friendship/memory rules are verified; avoid half-populated HT.
- Keep Japanese Gen I/II editing locked, legality PR #314 separate, and original save files immutable.

The archive branch is independent of ongoing application development and was created for preservation only.
