#!/usr/bin/env python3
"""Regression for Makefile.host.deps: a header-only change rebuilds every host output that includes it.

Runs in a throw-away tree with the real tools/qa/cxx_with_deps.sh, tools/qa/host_header_sweep.py and
Makefile.host.deps, and a tiny Makefile shaped like the host rules (only .cpp prerequisites, `$^` in the
recipe). Checks: first build records the header; an untouched tree is up to date; touching the header
rebuilds exactly the dependent output and not an unrelated one; a deleted header forces a rebuild (which
then fails like a clean CI build would); a failing compile keeps its exit status; `CXX=` on the command
line is still wrapped.
"""
from __future__ import annotations

import os
import shutil
import subprocess
import sys
import tempfile
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
MAKEFILE = """CXX ?= g++
CXXFLAGS ?= -std=c++20 -O0
HOST_BUILD := build-host
$(HOST_BUILD)/dep: tests/dep.cpp
\t@mkdir -p $(HOST_BUILD)
\t$(CXX) $(CXXFLAGS) -Iinclude $^ -o $@
$(HOST_BUILD)/plain: tests/plain.cpp
\t@mkdir -p $(HOST_BUILD)
\t$(CXX) $(CXXFLAGS) -Iinclude $^ -o $@
include Makefile.host.deps
"""


def make(root: Path, *args: str) -> subprocess.CompletedProcess:
    return subprocess.run(["make", "-f", "Makefile", *args], cwd=root, capture_output=True, text=True)


def bump(path: Path) -> None:
    later = time.time() + 5
    os.utime(path, (later, later))


def main() -> int:
    failures = 0

    def check(ok: bool, what: str, detail: str = "") -> None:
        nonlocal failures
        failures += not ok
        print(f"{'PASS' if ok else 'FAIL'}  {what}" + (f": {detail}" if not ok and detail else ""))

    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        (root / "tools/qa").mkdir(parents=True)
        for name in ("cxx_with_deps.sh", "host_header_sweep.py"):
            shutil.copy2(ROOT / "tools/qa" / name, root / "tools/qa" / name)
        shutil.copy2(ROOT / "Makefile.host.deps", root / "Makefile.host.deps")
        (root / "Makefile").write_text(MAKEFILE)
        (root / "include").mkdir()
        (root / "tests").mkdir()
        header = root / "include/value.h"
        header.write_text("inline int value() { return 1; }\n")
        (root / "tests/dep.cpp").write_text('#include "value.h"\nint main() { return value() == 1 ? 0 : 1; }\n')
        (root / "tests/plain.cpp").write_text("int main() { return 0; }\n")

        first = make(root, "build-host/dep", "build-host/plain")
        check(first.returncode == 0, "first build", first.stderr[-400:])
        record = root / "build-host/.hdrdeps/build-host%dep.d"
        check(record.is_file() and "include/value.h" in record.read_text(), "header recorded for its output")

        again = make(root, "build-host/dep", "build-host/plain")
        check("cxx_with_deps.sh" not in again.stdout, "untouched tree is up to date", again.stdout)

        plain_before = (root / "build-host/plain").stat().st_mtime_ns
        bump(header)
        touched = make(root, "build-host/dep", "build-host/plain")
        check(touched.returncode == 0 and "tests/dep.cpp" in touched.stdout,
              "header touch rebuilds the output that includes it", touched.stdout)
        check((root / "build-host/plain").stat().st_mtime_ns == plain_before and "tests/plain.cpp" not in touched.stdout,
              "an output that does not include the header is not rebuilt", touched.stdout)

        header.unlink()
        gone = make(root, "build-host/dep")
        check(gone.returncode != 0 and "rebuilding build-host/dep" in gone.stdout,
              "a deleted header forces a rebuild that fails like a clean build", gone.stdout + gone.stderr[-300:])
        header.write_text("inline int value() { return 1; }\n")

        (root / "tests/dep.cpp").write_text("int main() { return undefined_symbol; }\n")
        broken = make(root, "build-host/dep")
        check(broken.returncode != 0, "a failing compile keeps its non-zero status")

        if shutil.which("clang++"):
            (root / "tests/dep.cpp").write_text('#include "value.h"\nint main() { return value() == 1 ? 0 : 1; }\n')
            shutil.rmtree(root / "build-host")
            clang = make(root, "CXX=clang++", "build-host/dep")
            check(clang.returncode == 0 and "cxx_with_deps.sh clang++" in clang.stdout,
                  "CXX= on the command line is still wrapped", clang.stdout + clang.stderr[-300:])

    print(f"{'FAIL' if failures else 'PASS'}: host header dependency regression ({failures} failure(s))")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
