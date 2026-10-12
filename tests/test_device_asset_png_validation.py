#!/usr/bin/env python3
from __future__ import annotations

import tempfile
import zlib
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
from png_asset_validation import validate_png  # noqa: E402


def chunk(kind: bytes, payload: bytes) -> bytes:
    crc = zlib.crc32(kind)
    crc = zlib.crc32(payload, crc) & 0xFFFFFFFF
    return len(payload).to_bytes(4, "big") + kind + payload + crc.to_bytes(4, "big")


def minimal_png() -> bytes:
    sig = b"\x89PNG\r\n\x1a\n"
    ihdr = (1).to_bytes(4, "big") + (1).to_bytes(4, "big") + bytes([8, 6, 0, 0, 0])
    raw_scanline = b"\x00\x00\x00\x00\x00"
    return sig + chunk(b"IHDR", ihdr) + chunk(b"IDAT", zlib.compress(raw_scanline)) + chunk(b"IEND", b"")


def main() -> None:
    with tempfile.TemporaryDirectory(prefix="pokebank-png-contract-") as td:
        root = Path(td)
        valid = root / "valid.png"
        valid.write_bytes(minimal_png())
        ok, why, size = validate_png(valid)
        assert ok, why
        assert size == (1, 1)

        zero = root / "zero.png"
        zero.write_bytes(b"")
        assert not validate_png(zero)[0]

        fake = root / "fake.png"
        fake.write_bytes(b"not a png")
        assert not validate_png(fake)[0]

        truncated = root / "truncated.png"
        truncated.write_bytes(minimal_png()[:-7])
        assert not validate_png(truncated)[0]

        bad_crc = root / "bad-crc.png"
        payload = bytearray(minimal_png())
        payload[-5] ^= 0x01
        bad_crc.write_bytes(payload)
        assert not validate_png(bad_crc)[0]

    print("device PNG validation contract: PASS")


if __name__ == "__main__":
    main()
