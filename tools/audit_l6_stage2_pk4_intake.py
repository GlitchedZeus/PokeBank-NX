#!/usr/bin/env python3
"""Exact native PK4 intake contract, GCC/Clang normal or ASan/UBSan.

This contract uses production Gen4NativeRecordBoundary.h and Encryption4.h
without altered copies. It does not claim full parser coverage.
"""
from __future__ import annotations
import argparse
import json
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
TEST = "tests/fixtures/legality_l6_stage2_pk4_intake.cpp"
EXPECTED = "L6 Stage 2 bounded PK4 intake: 1028 source-preserving cases PASS"


def run(compiler: str, sanitized: bool) -> dict[str, object]:
    with tempfile.TemporaryDirectory(prefix="pbnx-l6-pk4-intake-") as directory:
        target = str(Path(directory) / "pk4_intake")
        command = [compiler, "-std=c++20", "-Wall", "-Wextra", "-Wpedantic",
                   "-Werror", "-Iinclude", TEST]
        command += (["-O0", "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
                    if sanitized else ["-O2"])
        command += ["-o", target]
        subprocess.run(command, cwd=ROOT, check=True)
        environment = os.environ.copy()
        environment.update({"ASAN_OPTIONS": "detect_leaks=0:halt_on_error=1",
                            "UBSAN_OPTIONS": "halt_on_error=1:print_stacktrace=1"})
        tested = subprocess.run([target], cwd=ROOT, check=True, capture_output=True,
                                text=True, env=environment)
        if EXPECTED not in tested.stdout:
            raise RuntimeError("unexpected test output: " + tested.stdout + tested.stderr)
    return {"compiler": compiler, "sanitized": sanitized, "passed": True,
            "synthetic_cases": 1028, "stdout": tested.stdout.strip()}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler", choices=("g++", "clang++"), default="g++")
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--normal", action="store_true")
    mode.add_argument("--sanitize", action="store_true")
    parser.add_argument("--json-out", type=Path)
    args = parser.parse_args()
    modes = [True] if args.sanitize else [False] if args.normal else [False, True]
    result = {"schema": "POKEBANK_NX_L6_PK4_INTAKE_V1",
              "runs": [run(args.compiler, sanitized) for sanitized in modes]}
    output = json.dumps(result, sort_keys=True, indent=2) + "\n"
    print(output)
    if args.json_out:
        args.json_out.write_text(output, encoding="utf-8")


if __name__ == "__main__":
    main()
