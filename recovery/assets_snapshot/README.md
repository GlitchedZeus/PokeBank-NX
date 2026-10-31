# Complete RomFS recovery snapshot

This directory contains the **complete generated RomFS snapshot** used by routine recovery.

The committed snapshot has historical provenance that may reference older branches or application SHAs. Those identifiers are evidence of where the snapshot came from; they are **not** current development or publishing targets.

For current recovery work:

```bash
python3 tools/recover_workspace.py
python3 tools/pack_recovery_snapshot.py
git add -f recovery/assets_snapshot/
git commit -m "recovery: snapshot complete verified RomFS"
```

Before pushing anything, re-fetch GitHub and use the **current reviewed branch/PR** for the active task. Do not push recovery output directly to the historical `feature/pokebank-playable` branch.

The packer creates an uncompressed deterministic tar and splits it into 80 MiB parts so no individual recovery file exceeds GitHub's normal single-file size ceiling.

Expected generated files:

```text
manifest.json
romfs-recovery.tar.part000
romfs-recovery.tar.part001
...
```

Future `tools/recover_workspace.py` runs should prefer this committed snapshot. Pinned network regeneration remains a fallback.

Do not commit a partial snapshot. `tools/pack_recovery_snapshot.py` refuses to run unless the normal device asset preflight passes first.
