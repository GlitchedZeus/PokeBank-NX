#!/usr/bin/env python3
"""Delete host build outputs that are older than any file they were built from.

tools/qa/cxx_with_deps.sh records, for every output under the host build directory, the sources and
headers its last compile read (<build>/.hdrdeps/*.d). Make only knows the .cpp prerequisites, so a
header-only change would otherwise reuse a stale binary. This sweep runs when Makefile.host is parsed,
before make decides what to rebuild: an output older than any recorded file (or with a recorded file
that no longer exists) is deleted, so make rebuilds it.

Usage: host_header_sweep.py [BUILD_DIR]   (prints one line per deleted output)
"""
from __future__ import annotations

import os
import sys
from pathlib import Path


def sweep(build: Path) -> list[str]:
    deleted = []
    depdir = build / ".hdrdeps"
    if not depdir.is_dir():
        return deleted
    for dep in depdir.glob("*.d"):
        out = Path(dep.name[:-2].replace("%", "/"))
        try:
            built = out.stat().st_mtime
        except FileNotFoundError:
            continue
        stale = None
        for line in dep.read_text(encoding="utf-8", errors="replace").splitlines():
            line = line.strip()
            if not line:
                continue
            try:
                if os.stat(line).st_mtime > built:
                    stale = line
                    break
            except FileNotFoundError:
                stale = line
                break
        if stale:
            out.unlink(missing_ok=True)
            deleted.append(f"{out} (newer or missing: {stale})")
    return deleted


def main(argv: list[str]) -> int:
    build = Path(argv[1] if len(argv) > 1 else "build-host")
    for line in sweep(build):
        print(f"host-header-sweep: rebuilding {line}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
