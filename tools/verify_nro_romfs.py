#!/usr/bin/env python3
"""Verify the RomFS that is actually packaged inside a built PokeBank NX .nro.

check_device_assets.py proves the *source* romfs/ tree is complete before `make`.
This tool closes the remaining gap that let a hardware NRO lose its scenery: it parses
the NRO's ASET section, walks the real RomFS directory/file tables, and fails when

  * a required presentation asset (region backdrop, trainer portrait, game cover, font,
    type icon, ball icon, HOME sprite, return host) is absent from the package, or
  * any file in the local romfs/ tree is missing from the package or differs byte-for-byte.

stdlib only; read-only. Usage:
    python3 tools/verify_nro_romfs.py PokeBankNX.nro [--romfs romfs] [--list]
"""
from __future__ import annotations

import argparse
import hashlib
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
EMPTY = 0xFFFFFFFF

GAME_CARDS = (
    "red_gb", "blue_gb", "yellow_gb",
    "gold_gbc", "silver_gbc", "crystal_gbc",
    "ruby_gba", "sapphire_gba", "emerald_gba", "firered_gba", "leafgreen_gba",
    "diamond_nds", "pearl_nds", "platinum_nds", "heartgold_nds", "soulsilver_nds",
    "black_nds", "white_nds", "black2_nds", "white2_nds",
)
TRAINER_PORTRAITS = (
    "red", "leaf", "gold", "kris", "brendan", "may", "lucas", "dawn", "ethan", "lyra",
    "chase", "elaine", "victor", "gloria", "rei", "akari", "florian", "juliana", "paxton",
    "harmony",
)
REGION_BACKDROPS = (
    "kanto", "johto", "hoenn", "sinnoh", "unova", "kalos", "alola", "galar", "hisui", "paldea",
)
BALL_ICONS = (
    "master-ball", "ultra-ball", "great-ball", "poke-ball", "safari-ball", "net-ball",
    "dive-ball", "nest-ball", "repeat-ball", "timer-ball", "luxury-ball", "premier-ball",
    "dusk-ball", "heal-ball", "quick-ball", "cherish-ball",
)
FONTS = ("Nunito.ttf", "NotoSansSymbols.ttf", "NotoSansSymbols2.ttf")
HOME_DEX_MAX = 1025
GEN1_DEX_MAX = 151


def required_paths() -> list[str]:
    """Every RomFS path the runtime asks for by name and that must never silently vanish."""
    paths: list[str] = []
    paths += [f"game_cards/{n}.png" for n in GAME_CARDS]
    paths += [f"trainer_portraits/{n}.png" for n in TRAINER_PORTRAITS]
    paths += [f"region_backdrops/{n}.png" for n in REGION_BACKDROPS]
    paths += [f"sprites/items/{n}.png" for n in BALL_ICONS]
    paths += [f"sprites/types/{i}.png" for i in range(18)]
    paths += [f"fonts/{n}" for n in FONTS]
    paths += [f"sprites/pokemon_hd/{n}.png" for n in range(1, HOME_DEX_MAX + 1)]
    paths += [f"sprites/pokemon_hd/{n}s.png" for n in range(1, GEN1_DEX_MAX + 1)]
    paths.append("runtime/PokeBankReturnHost.nro")
    return paths


class RomfsError(ValueError):
    pass


def _u32(data: bytes, off: int) -> int:
    if off + 4 > len(data):
        raise RomfsError(f"truncated u32 at {off:#x}")
    return struct.unpack_from("<I", data, off)[0]


def _u64(data: bytes, off: int) -> int:
    if off + 8 > len(data):
        raise RomfsError(f"truncated u64 at {off:#x}")
    return struct.unpack_from("<Q", data, off)[0]


def extract_romfs(nro: bytes) -> bytes:
    """Return the RomFS image embedded in an NRO's ASET section."""
    if len(nro) < 0x80 or nro[0x10:0x14] != b"NRO0":
        raise RomfsError("not an NRO (missing NRO0 magic)")
    body_size = _u32(nro, 0x18)
    if body_size > len(nro) - 0x38:
        raise RomfsError("NRO has no asset section after its body")
    aset = body_size
    if nro[aset:aset + 4] != b"ASET":
        raise RomfsError("NRO asset section has no ASET magic (built without --romfsdir?)")
    romfs_off = _u64(nro, aset + 0x28)
    romfs_size = _u64(nro, aset + 0x30)
    if romfs_size == 0:
        raise RomfsError("NRO asset section declares an empty RomFS")
    start = aset + romfs_off
    if start + romfs_size > len(nro):
        raise RomfsError("NRO RomFS extends past end of file")
    return nro[start:start + romfs_size]


def walk_romfs(romfs: bytes) -> dict[str, bytes]:
    """Parse a Switch homebrew RomFS image into {relative/path: file bytes}."""
    if len(romfs) < 0x50:
        raise RomfsError("RomFS header truncated")
    header_size = _u64(romfs, 0x00)
    if header_size != 0x50:
        raise RomfsError(f"unexpected RomFS header size {header_size:#x}")
    dir_off, dir_size = _u64(romfs, 0x18), _u64(romfs, 0x20)
    file_off, file_size = _u64(romfs, 0x38), _u64(romfs, 0x40)
    data_off = _u64(romfs, 0x48)
    for name, off, size in (("dir", dir_off, dir_size), ("file", file_off, file_size)):
        if off + size > len(romfs):
            raise RomfsError(f"RomFS {name} table out of bounds")
    dirs = romfs[dir_off:dir_off + dir_size]
    files = romfs[file_off:file_off + file_size]

    def name_at(table: bytes, entry: int, fixed: int) -> str:
        length = _u32(table, entry + fixed - 4)
        raw = table[entry + fixed:entry + fixed + length]
        if len(raw) != length:
            raise RomfsError("RomFS entry name truncated")
        return raw.decode("utf-8")

    out: dict[str, bytes] = {}
    seen_dirs: set[int] = set()
    budget = [len(dirs) + len(files) + 16]  # bounds a corrupt sibling cycle

    def visit_dir(entry: int, prefix: str) -> None:
        if entry in seen_dirs:
            raise RomfsError("RomFS directory cycle")
        seen_dirs.add(entry)
        child_file = _u32(dirs, entry + 0x0C)
        while child_file != EMPTY:
            budget[0] -= 1
            if budget[0] < 0:
                raise RomfsError("RomFS file chain does not terminate")
            name = name_at(files, child_file, 0x20)
            offset = _u64(files, child_file + 0x08)
            size = _u64(files, child_file + 0x10)
            begin = data_off + offset
            if begin + size > len(romfs):
                raise RomfsError(f"{prefix}{name}: data out of bounds")
            out[prefix + name] = romfs[begin:begin + size]
            child_file = _u32(files, child_file + 0x04)
        child_dir = _u32(dirs, entry + 0x08)
        while child_dir != EMPTY:
            budget[0] -= 1
            if budget[0] < 0:
                raise RomfsError("RomFS directory chain does not terminate")
            visit_dir(child_dir, prefix + name_at(dirs, child_dir, 0x18) + "/")
            child_dir = _u32(dirs, child_dir + 0x04)

    visit_dir(0, "")
    return out


def local_tree(romfs_dir: Path) -> dict[str, bytes]:
    tree: dict[str, bytes] = {}
    if not romfs_dir.is_dir():
        return tree
    for path in sorted(romfs_dir.rglob("*")):
        if path.is_file():
            tree[path.relative_to(romfs_dir).as_posix()] = path.read_bytes()
    return tree


def verify(packaged: dict[str, bytes], local: dict[str, bytes], required: list[str]) -> list[str]:
    errors: list[str] = []
    missing = [p for p in required if not packaged.get(p)]
    if missing:
        preview = ", ".join(missing[:15]) + (" ..." if len(missing) > 15 else "")
        errors.append(f"{len(missing)} required asset(s) absent/empty in packaged RomFS: {preview}")
    for path, data in local.items():
        got = packaged.get(path)
        if got is None:
            errors.append(f"local romfs/{path} was not packaged")
        elif got != data:
            errors.append(f"packaged {path} differs from local romfs/{path}")
    return errors


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("nro", type=Path)
    parser.add_argument("--romfs", type=Path, default=ROOT / "romfs")
    parser.add_argument("--list", action="store_true", help="print every packaged path")
    args = parser.parse_args(argv)

    try:
        packaged = walk_romfs(extract_romfs(args.nro.read_bytes()))
    except (OSError, RomfsError, UnicodeDecodeError) as exc:
        print(f"PACKAGED ROMFS: FAIL ({exc})")
        return 1

    if args.list:
        for path in sorted(packaged):
            print(f"{len(packaged[path]):>10}  {path}")

    errors = verify(packaged, local_tree(args.romfs), required_paths())
    total = sum(len(v) for v in packaged.values())
    digest = hashlib.sha256(b"".join(k.encode() + b"\0" + v for k, v in sorted(packaged.items())))
    print(f"packaged RomFS: {len(packaged)} files, {total / (1024 * 1024):.1f} MiB, "
          f"content digest {digest.hexdigest()[:16]}")
    for group in ("region_backdrops/", "trainer_portraits/", "game_cards/", "sprites/items/",
                  "sprites/types/", "sprites/pokemon_hd/", "fonts/"):
        print(f"  {group:<22} {sum(1 for p in packaged if p.startswith(group))}")
    if errors:
        for error in errors:
            print(f"FAIL  {error}")
        print(f"PACKAGED ROMFS: FAIL ({len(errors)} problem(s)) - do not distribute this NRO")
        return 1
    print("PACKAGED ROMFS: PASS")
    return 0


if __name__ == "__main__":
    sys.exit(main())
