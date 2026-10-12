#!/usr/bin/env python3
"""L6 Stage 2 read-only native PK4 bounds regression matrix.

--header-only compiles the real shared native helper in GCC and Clang,
normal and ASan/UBSan. Default also links actual Pokemon4ReadOnly and its
complete Stage 2 production dependency list. Missing source, failed builds,
incorrect attestation, timeouts and wrong exact HEAD fail closed.
"""
from __future__ import annotations
import argparse
import json
import os
from pathlib import Path
import re
import runpy
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
HEADER_TEST = "tests/fixtures/legality_l6_stage2_bounds_contract.cpp"
FULL_TEST = "tests/fixtures/legality_l6_stage2_pk4_offset_native.cpp"
STAGE2 = "tools/audit_legality_stage2_native_formats.py"
EXPECTED = {"header": ("L6 Stage 2 production bounds", 644426),
            "linked": ("L6 Stage 2 linked PK4 boundary", 6948)}


def check_output(kind: str, output: str) -> int:
    prefix, count = EXPECTED[kind]
    if not re.fullmatch(re.escape(prefix) + rf": {count} PASS \([^\n]+\)\n?", output):
        raise RuntimeError(f"invalid {kind} native attestation: {output!r}")
    return count


def full_sources() -> list[str]:
    source = ROOT / STAGE2
    if not source.is_file():
        raise FileNotFoundError(source)
    members = runpy.run_path(str(source), run_name="l6_source_inventory")
    sources = members["SOURCES"]
    if not sources or sources[0] != "tests/fixtures/legality_stage2_native_format_properties.cpp":
        raise RuntimeError("Stage 2 linked production manifest changed: review before use")
    required = ("src/Encryption/Encryption4.cpp", "src/Pokemon/Pokemon4ReadOnly.cpp",
                "src/Pokemon/PersonalInfo4PT.cpp", "src/Utils/StringHelpers.cpp")
    if any(x not in sources for x in required):
        raise RuntimeError("Stage 2 production linkage missing required component")
    return [FULL_TEST, *sources[1:]]


def invoke(argv: list[str], timeout: int, env: dict[str, str] | None = None) -> str:
    try:
        completed = subprocess.run(argv, cwd=ROOT, env=env, text=True,
                                   capture_output=True, timeout=timeout, check=False)
    except subprocess.TimeoutExpired as error:
        raise RuntimeError(f"native command timed out: {argv!r}") from error
    if completed.returncode:
        raise RuntimeError(f"native command failed [{completed.returncode}] {argv!r}:\n"
                           + completed.stdout + completed.stderr)
    return completed.stdout


def run(compiler: str, sanitize: bool, kind: str, location: Path,
        timeout: int) -> dict:
    test = HEADER_TEST if kind == "header" else FULL_TEST
    if not (ROOT / test).is_file():
        raise FileNotFoundError(ROOT / test)
    sources = [HEADER_TEST] if kind == "header" else full_sources()
    for path in sources:
        if not (ROOT / path).is_file():
            raise FileNotFoundError(ROOT / path)
    executable = location / f"l6-{kind}-{compiler}-{'san' if sanitize else 'normal'}"
    flags = ["-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-Iinclude"]
    if kind == "linked":
        flags += ["-Wno-unused-parameter"]
    if sanitize:
        flags += ["-O0", "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
    else:
        flags += ["-O2"]
    invoke([compiler, *flags, *sources, "-o", str(executable)], timeout)
    environment = os.environ.copy()
    if sanitize:
        environment["ASAN_OPTIONS"] = "detect_leaks=0:halt_on_error=1"
        environment["UBSAN_OPTIONS"] = "halt_on_error=1:print_stacktrace=1"
    stdout = invoke([str(executable)], timeout, environment)
    count = check_output(kind, stdout)
    return {"compiler": compiler, "asan_ubsan": sanitize, "scope": kind,
            "assertions": count, "status": "PASS"}


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--header-only", action="store_true")
    ap.add_argument("--self-test", action="store_true")
    ap.add_argument("--timeout", type=int, default=360)
    ap.add_argument("--expect-head")
    ap.add_argument("--json-out", type=Path)
    args = ap.parse_args()
    if args.timeout <= 0:
        ap.error("--timeout must be positive")
    if args.self_test:
        for kind, (title, n) in EXPECTED.items():
            template = f"{title}: {n} PASS (property check)\n"
            assert check_output(kind, template) == n
            for bad in (template.replace("PASS", "FAIL"),
                        template.replace(str(n), str(n+1)), template+"fake"):
                try:
                    check_output(kind, bad)
                except RuntimeError:
                    continue
                raise AssertionError("spoofed native attestation accepted")
        print("L6 Stage 2 native runner self-test: 8 controls PASS")
        return 0
    if args.expect_head:
        actual = invoke(["git", "rev-parse", "HEAD"], 15).strip()
        if actual != args.expect_head:
            raise RuntimeError("exact HEAD mismatch: expected " + args.expect_head + " got " + actual)
    kinds = ("header",) if args.header_only else ("header", "linked")
    results = []
    with tempfile.TemporaryDirectory(prefix="l6-stage2-bounds-") as temporary:
        for kind in kinds:
            for compiler in ("g++", "clang++"):
                for sanitize in (False, True):
                    results.append(run(compiler, sanitize, kind, Path(temporary), args.timeout))
    data = {"schema": "L6_STAGE2_PRODUCTION_NATIVE_BOUNDS_V1",
            "production_linked": not args.header_only,
            "actual_full_repository_built": not args.header_only,
            "full_integration_release_ready": False,
            "runs": results, "passed": len(results) == (4 if args.header_only else 8)}
    output = json.dumps(data, indent=2, sort_keys=True) + "\n"
    print(output, end="")
    if args.json_out:
        args.json_out.write_text(output, encoding="utf-8")
    return 0 if data["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
