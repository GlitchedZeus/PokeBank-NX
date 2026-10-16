#!/usr/bin/env python3
"""Exact-source L6 Stage 5 four-lane native/production adversarial runner.

Always distinguish executable, immutable synthetic candidate tests from the
production-linked analyzer. A standalone PASS cannot clear the native gate.
The runner makes no changes to saves, shared Makefiles, CI or production code.
"""
from __future__ import annotations
import argparse
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
STANDALONE = "tests/fixtures/legality_l6_stage5_joint_history.cpp"
PRODUCTION = "tests/fixtures/legality_l6_stage5_native_analyzer.cpp"
STAGE3 = ROOT / "tools/audit_legality_stage3_cross_lane.py"


def production_sources() -> list[str]:
    spec = importlib.util.spec_from_file_location("l6_stage3_sources", STAGE3)
    if spec is None or spec.loader is None:
        raise RuntimeError("cannot load pinned Stage 3 production source manifest")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    sources = list(module.SOURCES)
    if not sources or sources[0] != "tests/fixtures/legality_stage3_cross_lane_contracts.cpp":
        raise RuntimeError("Stage 3 analyzer manifest changed; independent review required")
    return [PRODUCTION, *sources[1:]]


def run(compiler: str, sanitize: bool, native: bool) -> dict[str, object]:
    source = PRODUCTION if native else STANDALONE
    sources = production_sources() if native else [source]
    mode = "asan-ubsan" if sanitize else "normal"
    command = [compiler, "-std=c++20", "-Wall", "-Wextra", "-Wpedantic",
               "-Werror", "-Wno-unused-parameter", "-I.", "-Iinclude"]
    command += ["-O1", "-g", "-fsanitize=address,undefined",
                "-fno-omit-frame-pointer"] if sanitize else ["-O2"]
    if native and "clang" in Path(compiler).name:
        # Same production-source allowances as the Stage 3 manifest's own Clang build.
        command += ["-Wno-unknown-warning-option", "-Wno-unused-function"]
    with tempfile.TemporaryDirectory(prefix="pbnx-l6-stage5-") as directory:
        executable = str(Path(directory) / "stage5")
        compile_cmd = [*command, *sources, "-o", executable]
        subprocess.run(compile_cmd, cwd=ROOT, check=True, timeout=900)
        env = os.environ.copy()
        env.update({"ASAN_OPTIONS": "detect_leaks=0:halt_on_error=1",
                    "UBSAN_OPTIONS": "halt_on_error=1:print_stacktrace=1"})
        completed = subprocess.run([executable], cwd=ROOT, env=env,
                                   text=True, capture_output=True,
                                   check=True, timeout=180)
    expected = ("L6 Stage5 production-linked encrypted PK4 + cross-lane: 512 PASS"
                if native else "L6 Stage5 authenticated joint-route adversarial checks: 72448 PASS")
    if expected not in completed.stdout:
        raise RuntimeError(f"unexpected {source} output: {completed.stdout}\n{completed.stderr}")
    return {"compiler": compiler, "mode": mode, "kind": "production-linked" if native else "standalone",
            "checks": 512 if native else 72448, "status": "PASS",
            "command": compile_cmd[:-2] + ["-o", "<temporary executable>"],
            "stdout": completed.stdout.strip()}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler", choices=("g++", "clang++", "both"), default="both")
    modes = parser.add_mutually_exclusive_group()
    modes.add_argument("--normal", action="store_true")
    modes.add_argument("--sanitize", action="store_true")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    compilers = ("g++", "clang++") if args.compiler == "both" else (args.compiler,)
    modes_to_run = (True,) if args.sanitize else (False,) if args.normal else (False, True)
    results: list[dict[str, object]] = []
    for compiler in compilers:
        for sanitize in modes_to_run:
            for native in (False, True):
                print(f"L6 Stage 5: {compiler} {'sanitizer' if sanitize else 'normal'} "
                      f"{'production' if native else 'standalone'}", flush=True)
                try:
                    results.append(run(compiler, sanitize, native))
                except (subprocess.CalledProcessError, subprocess.TimeoutExpired, RuntimeError, OSError) as error:
                    results.append({"compiler": compiler, "mode": "asan-ubsan" if sanitize else "normal",
                                    "kind": "production-linked" if native else "standalone",
                                    "status": "FAIL", "error": str(error)})
    output = {"schema": "POKEBANK_NX_L6_STAGE5_JOINT_NATIVE_V1",
              "passed": len(results) == len(compilers) * len(modes_to_run) * 2 and
                        all(r["status"] == "PASS" for r in results),
              "results": results}
    summary = json.dumps(output, indent=2, sort_keys=True) + "\n"
    print(summary)
    if args.output:
        args.output.write_text(summary, encoding="utf-8")
    return 0 if output["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
