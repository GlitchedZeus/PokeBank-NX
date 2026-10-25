#!/usr/bin/env python3
"""Adversarial archive integrity checks; the original archive stays untouched."""
import argparse
import binascii
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--archive", required=True, type=Path)
parser.add_argument("--pgf", required=True, type=Path)
args = parser.parse_args()
manifest = json.loads((ROOT / "docs/legality/historical-evidence-20261025/FULL_CARD_MANIFEST.json").read_text())
entries = manifest["files"]
before = {e["path"]: hashlib.sha256((args.archive / e["path"]).read_bytes()).hexdigest() for e in entries}
with tempfile.TemporaryDirectory(prefix="full-card-controls-") as temporary:
    clone = Path(temporary) / "archive"
    for entry in entries:
        dest = clone / entry["path"]
        dest.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(args.archive / entry["path"], dest)
    target = clone / entries[0]["path"]
    original = target.read_bytes()
    command = [sys.executable, str(ROOT / "tools/legality_completion/full_card_evidence.py"),
               "--archive", str(clone), "--pgf", str(args.pgf.resolve()), "--check"]
    subprocess.run(command, check=True, capture_output=True)
    tampered = bytearray(original)
    tampered[0xCE] ^= 0x10
    tampered[718:] = binascii.crc_hqx(tampered[:718], 0xFFFF).to_bytes(2, "little")
    for data in (original[:-1], original + b"\0", bytes(tampered), bytes(720)):
        target.write_bytes(data)
        result = subprocess.run(command, capture_output=True, text=True)
        assert result.returncode != 0 and "size/SHA256 mismatch" in result.stderr
    target.write_bytes(original)
    subprocess.run(command, check=True, capture_output=True)
after = {e["path"]: hashlib.sha256((args.archive / e["path"]).read_bytes()).hexdigest() for e in entries}
assert before == after
print("Full-card generator: original archive accepts; truncation/extension/CRC-recomputed alteration/zero forgery refuse; originals immutable PASS")
