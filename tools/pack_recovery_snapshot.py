#!/usr/bin/env python3
"""Pack the complete generated RomFS into GitHub-safe recovery chunks.

Run only after `python3 tools/recover_workspace.py` and the asset preflight pass.
The output is intentionally split below GitHub's single-file size ceiling so the
private repository can hold a complete recovery snapshot without Git LFS.

Usage:
    python3 tools/pack_recovery_snapshot.py

Then commit/push the generated files under:
    recovery/assets_snapshot/
"""
from __future__ import annotations

import hashlib
import json
import os
import shutil
import tarfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
ROMFS = ROOT / "romfs"
OUT = ROOT / "recovery" / "assets_snapshot"
STATE = ROOT / "recovery" / "RECOVERY_STATE.json"
PREFLIGHT = ROOT / "tools" / "check_device_assets.py"
CHUNK_BYTES = 80 * 1024 * 1024
ARCHIVE_NAME = "romfs-recovery.tar"


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as fh:
        for chunk in iter(lambda: fh.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def normalized(info: tarfile.TarInfo) -> tarfile.TarInfo:
    info.uid = 0
    info.gid = 0
    info.uname = ""
    info.gname = ""
    info.mtime = 0
    info.mode = 0o755 if info.isdir() else 0o644
    return info


def main() -> int:
    if not ROMFS.is_dir():
        raise SystemExit("romfs/ missing; run tools/recover_workspace.py first")

    # Never snapshot a partial tree.
    import subprocess, sys
    subprocess.run([sys.executable, str(PREFLIGHT)], cwd=ROOT, check=True)

    state = json.loads(STATE.read_text(encoding="utf-8"))
    OUT.mkdir(parents=True, exist_ok=True)

    # Remove prior generated snapshot pieces so the new manifest cannot mix generations.
    for path in OUT.iterdir():
        if path.is_file() and (path.name.startswith(ARCHIVE_NAME + ".part") or path.name == "manifest.json"):
            path.unlink()

    archive = OUT / ARCHIVE_NAME
    with tarfile.open(archive, "w", format=tarfile.PAX_FORMAT) as tf:
        for path in sorted(ROMFS.rglob("*"), key=lambda p: p.as_posix()):
            rel = Path("romfs") / path.relative_to(ROMFS)
            tf.add(path, arcname=rel.as_posix(), recursive=False, filter=normalized)

    archive_hash = sha256(archive)
    total_size = archive.stat().st_size
    parts = []
    with archive.open("rb") as src:
        index = 0
        while True:
            data = src.read(CHUNK_BYTES)
            if not data:
                break
            part = OUT / f"{ARCHIVE_NAME}.part{index:03d}"
            part.write_bytes(data)
            parts.append({
                "name": part.name,
                "size": len(data),
                "sha256": sha256(part),
            })
            index += 1

    archive.unlink()

    file_count = sum(1 for p in ROMFS.rglob("*") if p.is_file())
    manifest = {
        "schema": 1,
        "archive": ARCHIVE_NAME,
        "archive_sha256": archive_hash,
        "archive_size": total_size,
        "chunk_bytes": CHUNK_BYTES,
        "file_count": file_count,
        "application_source": state.get("application_source"),
        "historical_full_visual_baseline": state.get("historical_full_visual_baseline"),
        "parts": parts,
        "note": "Commit every part plus this manifest to the private GitHub repository. Recovery prefers this snapshot over network regeneration."
    }
    (OUT / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")

    print("RECOVERY SNAPSHOT READY")
    print(f"files: {file_count}")
    print(f"archive bytes: {total_size}")
    print(f"archive sha256: {archive_hash}")
    print(f"parts: {len(parts)}")
    for part in parts:
        print(f"  {part['name']}  {part['size']}  {part['sha256']}")
    print("\nNEXT: git add -f recovery/assets_snapshot && commit/push it before calling the session safely saved.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
