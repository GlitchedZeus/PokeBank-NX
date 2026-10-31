#!/usr/bin/env python3
"""The Switch build compiles the app with -fno-rtti (Makefile CXXFLAGS); host tests do not.

A dynamic_cast or typeid in app code therefore passes every host gate and only fails in the devkitA64
candidate build (that happened once, Oct 28, in UI/ClassicJapaneseNames.h). This guard fails on the
host instead: no dynamic_cast / typeid in src/ or include/ (comments ignored). Test code may use RTTI.
"""
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
PATTERN = re.compile(r"\b(dynamic_cast\s*<|typeid\s*\()")


def strip_comments(text: str) -> str:
    text = re.sub(r"/\*.*?\*/", lambda m: "\n" * m.group(0).count("\n"), text, flags=re.S)
    return re.sub(r"//[^\n]*", "", text)


def main() -> int:
    makefile = (ROOT / "Makefile").read_text(encoding="utf-8")
    failures = []
    if "-fno-rtti" not in makefile:
        failures.append("Makefile no longer builds the app with -fno-rtti; update this guard")
    for path in sorted(list((ROOT / "src").rglob("*")) + list((ROOT / "include").rglob("*"))):
        if path.suffix not in {".cpp", ".h", ".hpp", ".inc"} or not path.is_file():
            continue
        code = strip_comments(path.read_text(encoding="utf-8", errors="replace"))
        for n, line in enumerate(code.splitlines(), 1):
            if PATTERN.search(line):
                failures.append(f"{path.relative_to(ROOT)}:{n}: RTTI is unavailable on the device: {line.strip()}")
    if failures:
        print("Device build has no RTTI: FAIL")
        for f in failures:
            print("  " + f)
        return 1
    print("Device build has no RTTI: no dynamic_cast / typeid in app code: PASS")
    return 0


if __name__ == "__main__":
    sys.exit(main())
