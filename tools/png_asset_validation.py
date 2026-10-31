#!/usr/bin/env python3
"""Small stdlib PNG structural validator for device/recovery asset gates."""
from __future__ import annotations

import zlib
from pathlib import Path

PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"


def validate_png(path: Path) -> tuple[bool, str, tuple[int, int] | None]:
    try:
        data = path.read_bytes()
    except OSError as exc:
        return False, f"read failed: {exc}", None
    if len(data) < 33 or not data.startswith(PNG_SIGNATURE):
        return False, "missing PNG signature/minimum structure", None

    pos = len(PNG_SIGNATURE)
    saw_ihdr = False
    saw_idat = False
    saw_iend = False
    dimensions: tuple[int, int] | None = None

    while pos < len(data):
        if pos + 12 > len(data):
            return False, "truncated PNG chunk header", dimensions
        length = int.from_bytes(data[pos:pos + 4], "big")
        chunk_type = data[pos + 4:pos + 8]
        chunk_data_start = pos + 8
        chunk_data_end = chunk_data_start + length
        crc_end = chunk_data_end + 4
        if crc_end > len(data):
            return False, f"truncated {chunk_type!r} chunk", dimensions

        stored_crc = int.from_bytes(data[chunk_data_end:crc_end], "big")
        actual_crc = zlib.crc32(chunk_type)
        actual_crc = zlib.crc32(data[chunk_data_start:chunk_data_end], actual_crc) & 0xFFFFFFFF
        if stored_crc != actual_crc:
            return False, f"CRC mismatch in {chunk_type.decode('latin1')} chunk", dimensions

        if not saw_ihdr:
            if chunk_type != b"IHDR" or length != 13:
                return False, "first PNG chunk is not a 13-byte IHDR", dimensions
            width = int.from_bytes(data[chunk_data_start:chunk_data_start + 4], "big")
            height = int.from_bytes(data[chunk_data_start + 4:chunk_data_start + 8], "big")
            if width <= 0 or height <= 0:
                return False, "PNG has zero dimensions", None
            dimensions = (width, height)
            saw_ihdr = True
        elif chunk_type == b"IHDR":
            return False, "duplicate IHDR chunk", dimensions

        if chunk_type == b"IDAT":
            saw_idat = True
        if chunk_type == b"IEND":
            if length != 0:
                return False, "IEND chunk is not empty", dimensions
            saw_iend = True
            pos = crc_end
            break

        pos = crc_end

    if not saw_ihdr:
        return False, "missing IHDR", dimensions
    if not saw_idat:
        return False, "missing IDAT", dimensions
    if not saw_iend:
        return False, "missing IEND", dimensions
    return True, "", dimensions


def is_valid_png(path: Path) -> bool:
    return validate_png(path)[0]
