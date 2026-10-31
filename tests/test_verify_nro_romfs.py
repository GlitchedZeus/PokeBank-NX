#!/usr/bin/env python3
"""Executable regression for tools/verify_nro_romfs.py.

Builds real Switch-format RomFS images (same header/dir/file table layout as devkitPro's
build_romfs) inside a minimal NRO + ASET container, then proves the verifier:
  * walks nested directories and returns exact bytes,
  * passes a complete package,
  * fails when region backdrops / trainer portraits are dropped (the reported hardware regression),
  * fails when a packaged file differs from the local romfs/ tree,
  * fails cleanly on an NRO without ASET/RomFS and on corrupt tables.
"""
from __future__ import annotations

import struct
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))

import verify_nro_romfs as v  # noqa: E402

EMPTY = 0xFFFFFFFF


def _align4(n: int) -> int:
    return (n + 3) & ~3


def build_romfs(files: dict[str, bytes]) -> bytes:
    """Minimal build_romfs-compatible writer (hash tables are present but unused by the walker)."""
    tree: dict = {"dirs": {}, "files": {}}
    for path, data in sorted(files.items()):
        parts = path.split("/")
        node = tree
        for part in parts[:-1]:
            node = node["dirs"].setdefault(part, {"dirs": {}, "files": {}})
        node["files"][parts[-1]] = data

    dir_entries: list[dict] = []
    file_entries: list[dict] = []
    blobs: list[bytes] = []

    def add_dir(node: dict, name: str, parent: int) -> int:
        index = len(dir_entries)
        entry = {"name": name, "parent": parent, "sibling": EMPTY, "child_dir": EMPTY,
                 "child_file": EMPTY}
        dir_entries.append(entry)
        prev_file = None
        for fname, data in sorted(node["files"].items()):
            fi = len(file_entries)
            offset = sum(len(b) for b in blobs)
            blobs.append(data + b"\0" * ((-len(data)) % 16))
            file_entries.append({"name": fname, "parent": index, "sibling": EMPTY,
                                 "offset": offset, "size": len(data)})
            if prev_file is None:
                entry["child_file"] = ("file", fi)
            else:
                file_entries[prev_file]["sibling"] = ("file", fi)
            prev_file = fi
        prev_dir = None
        for dname, child in sorted(node["dirs"].items()):
            di = add_dir(child, dname, index)
            if prev_dir is None:
                entry["child_dir"] = ("dir", di)
            else:
                dir_entries[prev_dir]["sibling"] = ("dir", di)
            prev_dir = di
        return index

    add_dir(tree, "", 0)

    dir_offsets, off = [], 0
    for e in dir_entries:
        dir_offsets.append(off)
        off += 0x18 + _align4(len(e["name"].encode()))
    file_offsets, off = [], 0
    for e in file_entries:
        file_offsets.append(off)
        off += 0x20 + _align4(len(e["name"].encode()))

    def ref(value) -> int:
        if value == EMPTY:
            return EMPTY
        kind, idx = value
        return dir_offsets[idx] if kind == "dir" else file_offsets[idx]

    dir_table = b""
    for e in dir_entries:
        name = e["name"].encode()
        dir_table += struct.pack("<IIIIII", dir_offsets[e["parent"]], ref(e["sibling"]),
                                 ref(e["child_dir"]), ref(e["child_file"]), EMPTY, len(name))
        dir_table += name + b"\0" * (_align4(len(name)) - len(name))
    file_table = b""
    for e in file_entries:
        name = e["name"].encode()
        file_table += struct.pack("<IIQQII", dir_offsets[e["parent"]], ref(e["sibling"]),
                                  e["offset"], e["size"], EMPTY, len(name))
        file_table += name + b"\0" * (_align4(len(name)) - len(name))

    dir_hash = struct.pack("<I", EMPTY)
    file_hash = struct.pack("<I", EMPTY)
    off = 0x50
    dir_hash_off = off; off += len(dir_hash)
    dir_off = off; off += len(dir_table)
    file_hash_off = off; off += len(file_hash)
    file_off = off; off += len(file_table)
    data_off = (off + 15) & ~15
    header = struct.pack("<10Q", 0x50, dir_hash_off, len(dir_hash), dir_off, len(dir_table),
                         file_hash_off, len(file_hash), file_off, len(file_table), data_off)
    body = header + dir_hash + dir_table + file_hash + file_table
    body += b"\0" * (data_off - len(body))
    return body + b"".join(blobs)


def build_nro(romfs: bytes | None) -> bytes:
    body_size = 0x1000
    body = bytearray(body_size)
    body[0x10:0x14] = b"NRO0"
    struct.pack_into("<I", body, 0x18, body_size)
    if romfs is None:
        return bytes(body)
    aset = bytearray(0x38)
    aset[0:4] = b"ASET"
    struct.pack_into("<QQ", aset, 0x28, len(aset), len(romfs))
    return bytes(body) + bytes(aset) + romfs


def complete_tree() -> dict[str, bytes]:
    files = {}
    for i, path in enumerate(v.required_paths()):
        files[path] = f"payload-{i}-{path}".encode()
    return files


FAILURES: list[str] = []


def check(condition: bool, label: str) -> None:
    print(("PASS  " if condition else "FAIL  ") + label)
    if not condition:
        FAILURES.append(label)


def main() -> int:
    files = complete_tree()
    nro = build_nro(build_romfs(files))
    packaged = v.walk_romfs(v.extract_romfs(nro))
    check(packaged == files, "walker returns every nested RomFS file with exact bytes")
    # QA-F1: the Gen V covers are runtime-requested artwork and must be guarded
    # exactly like Gen I-IV covers (prepare_release_romfs.sh stages all twenty).
    for card in ("black_nds", "white_nds", "black2_nds", "white2_nds"):
        check(f"game_cards/{card}.png" in v.required_paths(), f"Gen V {card} cover is required")
    for portrait in ("hilbert", "hilda", "nate", "rosa"):
        check(f"trainer_portraits/{portrait}.png" in v.required_paths(),
              f"Gen V {portrait} portrait is required")
    missing_gen5 = {k: b for k, b in files.items() if k != "game_cards/black2_nds.png"}
    gen5_package = v.walk_romfs(v.extract_romfs(build_nro(build_romfs(missing_gen5))))
    check(any("required asset" in e for e in v.verify(gen5_package, missing_gen5, v.required_paths())),
          "NRO without a Gen V cover is rejected")
    check(v.verify(packaged, files, v.required_paths()) == [], "complete package passes")

    stripped = {k: d for k, d in files.items()
                if not k.startswith(("region_backdrops/", "trainer_portraits/"))}
    regressed = v.walk_romfs(v.extract_romfs(build_nro(build_romfs(stripped))))
    errors = v.verify(regressed, {}, v.required_paths())
    check(bool(errors) and errors[0].startswith("34 required asset(s)"),
          "missing region backdrops / trainer portraits fail the preflight")

    local = {"region_backdrops/kanto.png": b"approved art"}
    check(any("differs" in e for e in v.verify(packaged, local, [])),
          "byte-different packaged asset fails")
    check(any("was not packaged" in e for e in v.verify(packaged, {"extra/x.png": b"x"}, [])),
          "local romfs file absent from package fails")

    for label, blob in (("NRO without ASET", build_nro(None)), ("non-NRO", b"\0" * 256)):
        try:
            v.extract_romfs(blob)
            check(False, f"{label} rejected")
        except v.RomfsError:
            check(True, f"{label} rejected")

    corrupt = bytearray(build_romfs(files))
    struct.pack_into("<Q", corrupt, 0x40, 1 << 40)
    try:
        v.walk_romfs(bytes(corrupt))
        check(False, "out-of-bounds RomFS file table rejected")
    except v.RomfsError:
        check(True, "out-of-bounds RomFS file table rejected")

    with tempfile.TemporaryDirectory() as tmp:
        tmpdir = Path(tmp)
        (tmpdir / "romfs" / "region_backdrops").mkdir(parents=True)
        (tmpdir / "romfs" / "region_backdrops" / "kanto.png").write_bytes(files["region_backdrops/kanto.png"])
        good = tmpdir / "good.nro"
        good.write_bytes(nro)
        bad = tmpdir / "bad.nro"
        bad.write_bytes(build_nro(build_romfs(stripped)))
        check(v.main([str(good), "--romfs", str(tmpdir / "romfs")]) == 0, "CLI passes complete NRO")
        check(v.main([str(bad), "--romfs", str(tmpdir / "romfs")]) == 1, "CLI fails artwork-less NRO")

    print(f"\n{'FAIL' if FAILURES else 'PASS'}: verify_nro_romfs regression ({len(FAILURES)} failure(s))")
    return 1 if FAILURES else 0


if __name__ == "__main__":
    raise SystemExit(main())
