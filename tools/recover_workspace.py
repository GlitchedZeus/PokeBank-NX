#!/usr/bin/env python3
"""Reconstruct PokeBank NX generated build assets from GitHub-tracked recovery state.

Normal use:
    python3 tools/recover_workspace.py

Recovery order:
    1. complete committed RomFS snapshot, when present;
    2. pinned/tracked source regeneration as a fallback;
    3. tracked project overrides;
    4. offline device asset preflight.

This is the routine RECOVERY path. It is deliberately not a Git archaeology tool.
If this succeeds it prints RECOVERY COMPLETE. Only if it fails should a session
fall back to forensic worktree/reflog recovery.
"""
from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import os
import shutil
import subprocess
import sys
import tarfile
import tempfile
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
STATE_PATH = ROOT / "recovery" / "RECOVERY_STATE.json"
ROMFS = ROOT / "romfs"
HD_DIR = ROMFS / "sprites" / "pokemon_hd"
TYPE_DIR = ROMFS / "sprites" / "types"
FONT_DIR = ROMFS / "fonts"
GAME_CARD_DIR = ROMFS / "game_cards"
OVERRIDE_ROOT = ROOT / "assets" / "recovery_overrides" / "romfs"

FONT_URLS = {
    "Nunito.ttf": "https://github.com/google/fonts/raw/main/ofl/nunito/Nunito%5Bwght%5D.ttf",
    "NotoSansSymbols.ttf": "https://github.com/google/fonts/raw/main/ofl/notosanssymbols/NotoSansSymbols%5Bwght%5D.ttf",
    "NotoSansSymbols2.ttf": "https://github.com/google/fonts/raw/main/ofl/notosanssymbols2/NotoSansSymbols2-Regular.ttf",
}


def die(stage: str, reason: str) -> "NoReturn":
    print("\nRECOVERY BLOCKED")
    print(f"Stage: {stage}")
    print(f"Reason: {reason}")
    raise SystemExit(1)


def run(stage: str, argv: list[str], cwd: Path = ROOT) -> None:
    print(f"\n== {stage} ==")
    print("$ " + " ".join(argv))
    try:
        subprocess.run(argv, cwd=cwd, check=True)
    except FileNotFoundError as exc:
        die(stage, str(exc))
    except subprocess.CalledProcessError as exc:
        die(stage, f"command exited {exc.returncode}")


def load_state() -> dict:
    try:
        return json.loads(STATE_PATH.read_text(encoding="utf-8"))
    except Exception as exc:
        die("load recovery state", f"{STATE_PATH}: {exc}")


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as fh:
        for chunk in iter(lambda: fh.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def safe_members(tf: tarfile.TarFile):
    for member in tf.getmembers():
        path = Path(member.name)
        if path.is_absolute() or ".." in path.parts or not path.parts or path.parts[0] != "romfs":
            die("restore committed RomFS snapshot", f"unsafe archive member: {member.name}")
        yield member


def restore_committed_snapshot(state: dict) -> bool:
    info = state.get("full_romfs_snapshot") or {}
    manifest_rel = info.get("manifest")
    if not manifest_rel:
        return False
    manifest_path = ROOT / manifest_rel
    if not manifest_path.is_file():
        return False

    print("\n== restore complete GitHub RomFS snapshot ==")
    try:
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    except Exception as exc:
        die("restore committed RomFS snapshot", f"invalid manifest: {exc}")

    parts = manifest.get("parts") or []
    if not parts:
        die("restore committed RomFS snapshot", "manifest contains no archive parts")

    snapshot_dir = manifest_path.parent
    with tempfile.TemporaryDirectory(prefix="pokebank-recovery-") as td:
        temp_root = Path(td)
        archive = temp_root / manifest.get("archive", "romfs-recovery.tar")
        with archive.open("wb") as out:
            for item in parts:
                part = snapshot_dir / item["name"]
                if not part.is_file():
                    die("restore committed RomFS snapshot", f"missing part: {part.relative_to(ROOT)}")
                actual = sha256(part)
                if actual != item.get("sha256"):
                    die("restore committed RomFS snapshot",
                        f"part hash mismatch for {part.name}: expected {item.get('sha256')}, got {actual}")
                if part.stat().st_size != int(item.get("size", -1)):
                    die("restore committed RomFS snapshot", f"part size mismatch for {part.name}")
                with part.open("rb") as src:
                    shutil.copyfileobj(src, out, length=1024 * 1024)

        expected_archive = manifest.get("archive_sha256")
        actual_archive = sha256(archive)
        if expected_archive and actual_archive != expected_archive:
            die("restore committed RomFS snapshot",
                f"archive hash mismatch: expected {expected_archive}, got {actual_archive}")

        extract_root = temp_root / "extract"
        extract_root.mkdir()
        try:
            with tarfile.open(archive, "r") as tf:
                tf.extractall(extract_root, members=safe_members(tf))
        except (tarfile.TarError, OSError) as exc:
            die("restore committed RomFS snapshot", str(exc))

        recovered = extract_root / "romfs"
        if not recovered.is_dir():
            die("restore committed RomFS snapshot", "archive did not contain romfs/")

        if ROMFS.exists():
            shutil.rmtree(ROMFS)
        shutil.move(str(recovered), str(ROMFS))

    print(f"Committed snapshot restored from {len(parts)} GitHub part(s).")
    return True


def download(url: str, dst: Path) -> None:
    dst.parent.mkdir(parents=True, exist_ok=True)
    req = urllib.request.Request(url, headers={"User-Agent": "PokeBank-NX-recovery"})
    try:
        with urllib.request.urlopen(req, timeout=60) as response:
            data = response.read()
    except Exception as exc:
        die("download generated asset", f"{url}: {exc}")
    if not data:
        die("download generated asset", f"empty response: {url}")
    tmp = dst.with_suffix(dst.suffix + ".recovery.tmp")
    tmp.write_bytes(data)
    tmp.replace(dst)


def ensure_pillow() -> None:
    if importlib.util.find_spec("PIL") is not None:
        return
    print("Pillow is missing; attempting local installation for HD sprite recovery...")
    run("install Pillow", [sys.executable, "-m", "pip", "install", "--user", "pillow"])
    if importlib.util.find_spec("PIL") is None:
        die("install Pillow", "Pillow still cannot be imported")


def png_count(path: Path) -> int:
    if not path.is_dir():
        return 0
    return sum(1 for p in path.iterdir() if p.is_file() and p.suffix.lower() == ".png")


def restore_hd_sprites(state: dict, force: bool) -> None:
    info = state["pokemon_hd"]
    expected = int(info["expected_png_count"])
    current = png_count(HD_DIR)
    print(f"HD renders before recovery: {current}/{expected}")

    if force or current != expected:
        ensure_pillow()
        argv = [sys.executable, str(ROOT / info["generator"])]
        if force:
            argv.append("--force")
        run("restore pinned HD Pokémon renders", argv)

    current = png_count(HD_DIR)
    if current != expected:
        die("restore pinned HD Pokémon renders", f"expected {expected} PNGs, found {current}")
    print(f"HD renders restored: {current}/{expected}")


def restore_type_icons(state: dict, force: bool) -> None:
    info = state["type_icons"]
    ref = info["pinned_ref"]
    template = info["path_template"]
    expected = int(info["expected_count"])
    TYPE_DIR.mkdir(parents=True, exist_ok=True)

    for api_id in range(1, expected + 1):
        local_id = api_id - 1
        dst = TYPE_DIR / f"{local_id}.png"
        if dst.is_file() and dst.stat().st_size > 0 and not force:
            continue
        remote_path = template.format(api_id=api_id)
        url = f"https://raw.githubusercontent.com/PokeAPI/sprites/{ref}/{remote_path}"
        print(f"type {api_id}/{expected} -> {dst.relative_to(ROOT)}")
        download(url, dst)

    count = png_count(TYPE_DIR)
    if count < expected:
        die("restore type icons", f"expected at least {expected} PNGs, found {count}")
    print(f"Type icons restored: {expected}/{expected}")


def restore_fonts(state: dict, force: bool) -> None:
    FONT_DIR.mkdir(parents=True, exist_ok=True)
    expected = list(state["fonts"]["expected"])
    for name in expected:
        dst = FONT_DIR / name
        if dst.is_file() and dst.stat().st_size > 0 and not force:
            continue
        url = FONT_URLS.get(name)
        if not url:
            die("restore fonts", f"no recovery URL configured for {name}")
        print(f"font -> {dst.relative_to(ROOT)}")
        download(url, dst)

    missing = [name for name in expected if not (FONT_DIR / name).is_file()]
    if missing:
        die("restore fonts", "missing after recovery: " + ", ".join(missing))
    print(f"Fonts restored: {len(expected)}/{len(expected)}")


def restore_game_cards(state: dict) -> None:
    info = state["game_cards"]
    source = ROOT / info["tracked_source"]
    GAME_CARD_DIR.mkdir(parents=True, exist_ok=True)
    for name in info["expected"]:
        src = source / name
        dst = GAME_CARD_DIR / name
        if not src.is_file():
            die("restore tracked game cards", f"tracked source missing: {src.relative_to(ROOT)}")
        shutil.copy2(src, dst)
    print(f"Game cards restored: {len(info['expected'])}/{len(info['expected'])}")


def apply_overrides() -> None:
    if not OVERRIDE_ROOT.is_dir():
        print("Tracked recovery overrides: none")
        return
    copied = 0
    for src in OVERRIDE_ROOT.rglob("*"):
        if not src.is_file():
            continue
        rel = src.relative_to(OVERRIDE_ROOT)
        dst = ROMFS / rel
        dst.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(src, dst)
        copied += 1
    print(f"Tracked recovery overrides applied: {copied}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--force", action="store_true",
                        help="ignore a committed snapshot and re-fetch generated network assets")
    parser.add_argument("--build", action="store_true",
                        help="after recovery/preflight, run the native make build")
    args = parser.parse_args()

    state = load_state()

    print("PokeBank NX deterministic workspace recovery")
    print(f"Repository: {state['repository']}")
    print(f"Branch: {state['branch']}")
    print(f"Application source checkpoint: {state['application_source']}")
    print(f"Recovery state: {STATE_PATH.relative_to(ROOT)}")

    snapshot_used = False
    if not args.force:
        snapshot_used = restore_committed_snapshot(state)

    if not snapshot_used:
        print("Complete GitHub RomFS snapshot not present; using pinned regeneration fallback.")
        restore_hd_sprites(state, args.force)
        restore_type_icons(state, args.force)
        restore_fonts(state, args.force)

    restore_game_cards(state)
    apply_overrides()

    run("device asset preflight", [sys.executable, str(ROOT / state["preflight"])])

    if args.build:
        if not os.environ.get("DEVKITPRO"):
            die("native build", "DEVKITPRO is not set; assets are recovered but native toolchain is unavailable")
        run("native NRO build", ["make"])

    print("\nRECOVERY COMPLETE")
    print(f"GitHub application source checkpoint: {state['application_source']}")
    print("Recovery source: " + ("committed GitHub RomFS snapshot" if snapshot_used else "pinned regeneration fallback"))
    print("Generated asset preflight: PASS")
    print("Active task can continue: YES")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
