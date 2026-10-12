#!/usr/bin/env python3
"""L5 Stage 2 native storage test runner.

Default --full is fail-closed: independent C++ model AND real linked production
PK3/PK4 encryptors, each in optimized and ASan/UBSan modes.
--model-only is allowed for partial work but NEVER reports production PASS.
No source saves opened or changed. All executable outputs live in a temp dir.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
MODEL = "tests/fixtures/legality_l5_native_storage_adversarial.cpp"
NATIVE = "tests/fixtures/legality_l5_native_codec_adversarial.cpp"
CRYPT3 = "src/Encryption/Encryption3FRLG.cpp"
CRYPT4 = "src/Encryption/Encryption4.cpp"
PINS = {
    CRYPT3: "eec27b73a15d6652371ed2e066690ef0fa9fb7ef",
    CRYPT4: "406935761713db95ae738398b65ba0a37d4cf97b",
}
EXPECTED = {
    MODEL: "L5 Stage2 native-format adversarial MODEL PASS 1056464 assertions",
    NATIVE: "L5 Stage2 ACTUAL production PK3/PK4 codec PASS",
}


def git_blob(path: Path) -> str:
    data = path.read_bytes()
    return hashlib.sha1(b"blob " + str(len(data)).encode() + b"\0" + data).hexdigest()


def validate_scope(scope: str, production_runs: int) -> None:
    if scope == "full" and production_runs != 2:
        raise RuntimeError("Production normal AND sanitizer execution are mandatory")
    if scope == "model-only" and production_runs:
        raise RuntimeError("Model-only mode cannot claim production verification")


def execute(source: str, extra: list[str], mode: str, tmp: Path,
            clang: bool = False) -> dict[str, str | bool]:
    compiler = "clang++" if clang else "g++"
    sanitize = mode == "asan_ubsan"
    flags = ["-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror"]
    flags += ["-O1", "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer"] if sanitize else ["-O2"]
    name = ("prod" if extra else "model") + ("-clang" if clang else "-gcc") + "-" + mode
    exe = tmp / name
    compile_cmd = [compiler, *flags, "-Iinclude", source, *extra, "-o", str(exe)]
    subprocess.run(compile_cmd, check=True, cwd=ROOT, capture_output=True, text=True)
    env = os.environ.copy()
    if sanitize:
        env["ASAN_OPTIONS"] = "detect_leaks=0:halt_on_error=1"
        env["UBSAN_OPTIONS"] = "halt_on_error=1:print_stacktrace=1"
    run = subprocess.run([str(exe)], check=True, cwd=ROOT, env=env,
                         capture_output=True, text=True)
    if EXPECTED[source] not in run.stdout:
        raise RuntimeError("Missing exact executable evidence: " + run.stdout)
    return {"program": name, "compiled": True, "executed": True,
            "sanitized": sanitize, "output": run.stdout.strip()}


def run(scope: str, clang: bool, pin: bool) -> dict:
    validate_scope(scope, 0 if scope == "model-only" else 2)
    if pin and scope != "full":
        raise ValueError("--source-pin requires --full")
    required = [MODEL] + ([NATIVE, CRYPT3, CRYPT4] if scope == "full" else [])
    for item in required:
        if not (ROOT / item).is_file():
            raise FileNotFoundError("Required repository file absent: " + item)
    if pin:
        for file, expected in PINS.items():
            current = git_blob(ROOT / file)
            if current != expected:
                raise RuntimeError("Production source drift: " + file +
                                   " expected=" + expected + " actual=" + current)
    runs = []
    with tempfile.TemporaryDirectory(prefix="l5-stage2-native-") as directory:
        tmp = Path(directory)
        for mode in ("normal", "asan_ubsan"):
            runs.append(execute(MODEL, [], mode, tmp))
        if clang:
            runs.append(execute(MODEL, [], "normal", tmp, clang=True))
        if scope == "full":
            for mode in ("normal", "asan_ubsan"):
                runs.append(execute(NATIVE, [CRYPT3, CRYPT4], mode, tmp))
    production_count = sum(r["executed"] for r in runs if str(r["program"]).startswith("prod-"))
    validate_scope(scope, production_count)
    return {
        "stage": "L5_STAGE_2_NATIVE_STORAGE",
        "status": "PASS_FULL_NATIVE_SYNTHETIC" if scope == "full" else "PASS_MODEL_ONLY",
        "production_gate": "SATISFIED" if scope == "full" else "BLOCKED_NOT_EXECUTED",
        "source_pin_checked": pin, "synthetic_only": True,
        "authentic_original_retail_specimens": 0,
        "full_pkhex_parity_certificates": 0,
        "runs": runs,
    }


def self_test() -> None:
    validate_scope("full", 2)
    validate_scope("model-only", 0)
    for scope, count in (("full", 0), ("full", 1), ("model-only", 1)):
        try:
            validate_scope(scope, count)
        except RuntimeError:
            continue
        raise AssertionError((scope, count))
    assert len(PINS) == 2 and len(EXPECTED) == 2
    print("L5 Stage2 fail-closed runner scope self-test PASS (5 states)")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    group = parser.add_mutually_exclusive_group()
    group.add_argument("--full", action="store_true", help="Default; require linked production tests")
    group.add_argument("--model-only", action="store_true", help="Partial explicit mode; not production proof")
    group.add_argument("--self-test", action="store_true")
    parser.add_argument("--clang", action="store_true", help="Additional independent clang optimized model")
    parser.add_argument("--source-pin", action="store_true", help="Require exact inspected production crypto Git blobs")
    opts = parser.parse_args()
    if opts.self_test:
        self_test()
        return
    scope = "model-only" if opts.model_only else "full"
    print(json.dumps(run(scope, opts.clang, opts.source_pin), indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
