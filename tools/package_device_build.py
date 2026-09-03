#!/usr/bin/env python3
"""Package one exact PokeBank NX .nro for device testing.

The script is stdlib-only and intentionally conservative. It is meant to be run while HEAD is the
exact application-source commit used for the clean native build, before later docs-only commits are
added.

It:
  * refuses a dirty Git worktree by default;
  * runs tools/check_device_assets.py by default;
  * records the full/short HEAD SHA;
  * requires the embedded short SHA to appear in the .nro bytes;
  * computes byte size and SHA-256;
  * copies to a source-addressed filename;
  * writes a plain-text manifest;
  * optionally writes a ZIP containing the .nro + manifest.

Example:
    python tools/package_device_build.py PokeBankNX.nro --label Second-Device --zip

This does NOT make a build DEVICE TESTED. Only a human physically running that exact artifact can
promote its status.
"""
from __future__ import annotations

import argparse
import hashlib
import shutil
import subprocess
import sys
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent


def git(*args: str) -> str:
    proc = subprocess.run(
        ["git", *args],
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    if proc.returncode != 0:
        raise SystemExit(f"git {' '.join(args)} failed:\n{proc.stderr.strip()}")
    return proc.stdout.strip()


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def run_asset_preflight() -> None:
    checker = ROOT / "tools" / "check_device_assets.py"
    if not checker.is_file():
        raise SystemExit(f"asset preflight tool missing: {checker}")
    proc = subprocess.run([sys.executable, str(checker)], cwd=ROOT)
    if proc.returncode != 0:
        raise SystemExit("device asset preflight failed; refusing to package visual-acceptance artifact")


def sanitize_label(label: str) -> str:
    cleaned = "".join(ch if ch.isalnum() or ch in "-_" else "-" for ch in label.strip())
    while "--" in cleaned:
        cleaned = cleaned.replace("--", "-")
    cleaned = cleaned.strip("-")
    if not cleaned:
        raise SystemExit("--label must contain at least one alphanumeric character")
    return cleaned


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("nro", type=Path, help="freshly built .nro to package")
    ap.add_argument("--label", default="Device-Test", help="artifact label, e.g. Second-Device")
    ap.add_argument("--out-dir", type=Path, default=ROOT / "dist-device", help="output directory")
    ap.add_argument("--zip", action="store_true", help="also create a ZIP with .nro + manifest")
    ap.add_argument("--allow-dirty", action="store_true", help="override dirty-worktree refusal (not recommended)")
    ap.add_argument("--skip-asset-preflight", action="store_true", help="skip local RomFS asset gate")
    ap.add_argument("--skip-embedded-sha-check", action="store_true", help="skip embedded short-SHA byte check")
    args = ap.parse_args()

    nro = args.nro.resolve()
    if not nro.is_file():
        raise SystemExit(f".nro not found: {nro}")
    if nro.suffix.lower() != ".nro":
        raise SystemExit(f"expected a .nro file, got: {nro.name}")

    toplevel = Path(git("rev-parse", "--show-toplevel")).resolve()
    if toplevel != ROOT:
        raise SystemExit(f"script repo root mismatch: git={toplevel} script={ROOT}")

    dirty = git("status", "--porcelain")
    if dirty and not args.allow_dirty:
        print("Worktree is dirty; exact device packaging requires a clean application-source checkout:", file=sys.stderr)
        print(dirty, file=sys.stderr)
        print("Commit/stash intentional changes, then clean-rebuild before packaging.", file=sys.stderr)
        return 1

    full_sha = git("rev-parse", "HEAD")
    short_sha = git("rev-parse", "--short=8", "HEAD")
    commit_message = git("log", "-1", "--pretty=%s")
    branch = git("rev-parse", "--abbrev-ref", "HEAD")

    if not args.skip_asset_preflight:
        run_asset_preflight()

    if not args.skip_embedded_sha_check:
        needle = short_sha.encode("ascii")
        data = nro.read_bytes()
        if needle not in data:
            raise SystemExit(
                f"embedded SHA check failed: ASCII {short_sha!r} was not found in {nro.name}.\n"
                "Refusing to package because the binary may not correspond to current HEAD."
            )

    size = nro.stat().st_size
    digest = sha256_file(nro)
    label = sanitize_label(args.label)

    out_dir = args.out_dir.resolve()
    out_dir.mkdir(parents=True, exist_ok=True)
    artifact_name = f"PokeBank-NX-{label}-{short_sha}.nro"
    artifact = out_dir / artifact_name
    manifest = out_dir / f"{artifact_name}.manifest.txt"

    if artifact.exists() or manifest.exists():
        raise SystemExit(
            f"refusing to overwrite existing packaged artifact/manifest in {out_dir};\n"
            "preserve previous evidence and choose a new output directory if this is intentional"
        )

    shutil.copy2(nro, artifact)

    manifest_text = f"""POKEBANK NX DEVICE ARTIFACT MANIFEST
====================================

Status: READY FOR DEVICE TEST / NOT DEVICE TESTED

Application source full SHA: {full_sha}
Application source short SHA: {short_sha}
Application source commit: {commit_message}
Git branch at packaging: {branch}

Original build output: {nro.name}
Artifact filename: {artifact.name}
Artifact size bytes: {size}
Artifact SHA-256: {digest}
Embedded short SHA check: {'SKIPPED' if args.skip_embedded_sha_check else 'PASS'}
Device asset preflight: {'SKIPPED' if args.skip_asset_preflight else 'PASS'}

Physical Switch status: NOT DEVICE TESTED

IMPORTANT
---------
This manifest proves packaging identity only. It does not prove runtime behavior or hardware safety.
Only record DEVICE TESTED after a human physically runs this exact filename/hash.
"""
    manifest.write_text(manifest_text, encoding="utf-8")

    zip_path = None
    if args.zip:
        zip_path = out_dir / f"PokeBank-NX-{label}-{short_sha}.zip"
        if zip_path.exists():
            raise SystemExit(f"refusing to overwrite existing ZIP: {zip_path}")
        with zipfile.ZipFile(zip_path, "w", compression=zipfile.ZIP_DEFLATED) as zf:
            zf.write(artifact, arcname=artifact.name)
            zf.write(manifest, arcname=manifest.name)

    print("DEVICE ARTIFACT PACKAGED")
    print(f"Application source: {full_sha}")
    print(f"Commit: {commit_message}")
    print(f"Artifact: {artifact}")
    print(f"Size: {size} bytes")
    print(f"SHA-256: {digest}")
    print(f"Manifest: {manifest}")
    if zip_path:
        print(f"ZIP: {zip_path}")
    print("Status: READY FOR DEVICE TEST / NOT DEVICE TESTED")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
