# Complete RomFS recovery snapshot

This directory is reserved for the **complete generated RomFS snapshot** used by routine recovery.

The project repository is private. Once a full verified asset tree exists, run:

```bash
python3 tools/recover_workspace.py
python3 tools/pack_recovery_snapshot.py
git add -f recovery/assets_snapshot/
git commit -m "recovery: snapshot complete verified RomFS"
git push origin feature/pokebank-playable
```

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
