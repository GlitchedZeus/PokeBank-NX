#!/usr/bin/env python3
"""Run audit-only Stage 4 native gate-ledger tests, normal or ASan/UBSan."""
from __future__ import annotations
import argparse
import json
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
FIXTURE = "tests/fixtures/legality_l6_stage4_gate_ledger.cpp"
EXPECTED = "L6 Stage 4 exact-head gate ledger: 6709 PASS"

def run(sanitize: bool, compiler: str) -> dict[str, object]:
    with tempfile.TemporaryDirectory(prefix="pbnx-l6-stage4-") as folder:
        target = str(Path(folder) / "gate-ledger")
        command = [compiler, "-std=c++20", "-Wall", "-Wextra", "-Wpedantic",
                   "-Werror", FIXTURE, "-o", target]
        command += (["-O1", "-g", "-fsanitize=address,undefined",
                     "-fno-sanitize-recover=all", "-fno-omit-frame-pointer"]
                    if sanitize else ["-O2"])
        subprocess.run(command, cwd=ROOT, check=True)
        env = os.environ.copy()
        env.update(ASAN_OPTIONS="detect_leaks=0:halt_on_error=1",
                   UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
        executed = subprocess.run([target], cwd=ROOT, check=True,
                                  capture_output=True, text=True, env=env)
        if EXPECTED not in executed.stdout:
            raise RuntimeError("unrecognized L6 Stage 4 native result: " +
                               executed.stdout + executed.stderr)
    return {"compiler": compiler, "sanitized": sanitize, "checks": 6709,
            "status": "PASS", "source_linked_original_retail": False,
            "stdout": executed.stdout.strip()}

def main() -> None:
    p = argparse.ArgumentParser(description=__doc__)
    g = p.add_mutually_exclusive_group()
    g.add_argument("--normal", action="store_true")
    g.add_argument("--sanitize", action="store_true")
    p.add_argument("--compiler", default="g++")
    p.add_argument("--json-out", type=Path)
    args = p.parse_args()
    profiles = [False] if args.normal else [True] if args.sanitize else [False, True]
    data = {"schema": "POKEBANK_NX_L6_STAGE4_NATIVE_V1",
            "runs": [run(profile, args.compiler) for profile in profiles]}
    output = json.dumps(data, sort_keys=True, indent=2) + "\n"
    print(output)
    if args.json_out:
        args.json_out.write_text(output, encoding="utf-8")

if __name__ == "__main__":
    main()
