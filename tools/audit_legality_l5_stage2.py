#!/usr/bin/env python3
"""Exact-blob L5 Stage 2 native helper QA and optional full analyzer gate.

The header test uses real production Gen12TimeCapsuleEvidence.h, not a model.
The --native-analyzer gate requires a complete exact checkout and calls the
actual Legality::analyze; it never silently downgrades to source-only tests.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parents[1]
PRODUCTION="include/Legality/Gen12TimeCapsuleEvidence.h"
FIXTURE="tests/fixtures/legality_l5_stage2_gen12_native_history_fuzz.cpp"
SOURCE_BLOB="4be5eda0dac2f43e886024d2febc07d9639abc18"
FIXTURE_BLOB="bf110896cbea4c0632f41ac11bd2247064c77e74"
EXPECTED=re.compile(r"^L5 Stage2 exact production Gen I/II Time Capsule helper: 1507328 PASS; source-capable species=162; conditional later-move=3088; malformed move inputs=15499 \(not retail histories\)\n?$")

def blob(path: Path) -> str:
    payload=path.read_bytes()
    return hashlib.sha1(b"blob "+str(len(payload)).encode("ascii")+b"\0"+payload).hexdigest()

def verify_pins() -> dict:
    result={}
    for name,pin in ((PRODUCTION,SOURCE_BLOB),(FIXTURE,FIXTURE_BLOB)):
        path=ROOT/name
        if not path.is_file():raise FileNotFoundError(f"missing exact source: {path}")
        actual=blob(path)
        if actual!=pin:raise RuntimeError(f"source drift: {name} expected {pin}, actual {actual}")
        result[name]=actual
    return result

def execute(args: list[str], timeout: int, environment: dict[str,str] | None=None) -> str:
    result=subprocess.run(args,cwd=ROOT,text=True,capture_output=True,
                          timeout=timeout,env=environment,check=False)
    if result.returncode != 0:
        raise RuntimeError(f"exit {result.returncode}: {args}\n{result.stdout}\n{result.stderr}")
    return result.stdout

def run_helper(compiler: str, sanitized: bool, temp: Path, timeout: int) -> dict:
    binary=temp/(compiler.replace("+","p")+("-san" if sanitized else "-normal"))
    flags=["-std=c++20","-Wall","-Wextra","-Wpedantic","-Werror","-Iinclude"]
    if sanitized:flags.extend(["-O1","-g","-fsanitize=address,undefined","-fno-omit-frame-pointer"])
    else:flags.append("-O2")
    execute([compiler,*flags,FIXTURE,"-o",str(binary)],timeout)
    environment=os.environ.copy()
    if sanitized:
        environment.update(ASAN_OPTIONS="detect_leaks=0:halt_on_error=1",
                           UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
    output=execute([str(binary)],timeout,environment)
    if EXPECTED.fullmatch(output) is None:raise RuntimeError("helper attestation mismatch: "+repr(output))
    return {"compiler":compiler,"sanitized":sanitized,"executed":True,
            "checks":1507328,"status":"PASS","evidence":output.strip()}

def run_native(timeout: int) -> list[dict]:
    if not (ROOT/"Makefile.host").is_file():
        raise RuntimeError("full source checkout missing: Makefile.host absent; native analyzer BLOCKED")
    if not (ROOT/"tools/audit_legality_l5_stage2_native.mk").is_file():
        raise RuntimeError("native analyzer sidecar missing")
    results=[]
    for target in ("l5-stage2-baseline","l5-stage2-baseline-sanitize"):
        out=execute(["make","-f","Makefile.host", "-f",
                     "tools/audit_legality_l5_stage2_native.mk",target],timeout)
        if "L5 Stage2 real PK1/PK2 Legality::analyze matrix: 1530 PASS" not in out:
            raise RuntimeError("native analyzer did not attest 1530 real cases")
        results.append({"target":target,"status":"PASS","actual_production_analyzer":True})
    return results

def self_test() -> None:
    reference=("L5 Stage2 exact production Gen I/II Time Capsule helper: 1507328 PASS; "
               "source-capable species=162; conditional later-move=3088; "
               "malformed move inputs=15499 (not retail histories)\n")
    assert EXPECTED.fullmatch(reference)
    for altered in (reference.replace("1507328","1507327"),
                    reference.replace("PASS","FAIL"),
                    reference.replace("source-capable species=162","source-capable species=163"),
                    reference+"FAKE"):
        assert EXPECTED.fullmatch(altered) is None
    print("L5 Stage2 runner 5 attestation controls PASS")

def main() -> int:
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test",action="store_true")
    parser.add_argument("--native-analyzer",action="store_true")
    parser.add_argument("--timeout",type=int,default=600)
    parser.add_argument("--json-out",type=Path)
    args=parser.parse_args()
    if args.self_test:self_test();return 0
    if args.timeout<1:parser.error("timeout must be positive")
    pins=verify_pins()
    with tempfile.TemporaryDirectory(prefix="l5-stage2-native-") as temp:
        jobs=[run_helper(compiler,sanitized,Path(temp),args.timeout)
              for compiler in ("g++","clang++") for sanitized in (False,True)]
    native=run_native(args.timeout) if args.native_analyzer else []
    report={"schema":"L5_STAGE2_REAL_SOURCE_NATIVE_QA_V1", "source_pins":pins,
            "header_source_binaries":jobs,"actual_analyzer_binaries":native,
            "actual_analyzer_executed":bool(native),"authenticated_original_retail_histories":0,
            "release_approved":False,"status":"HELPER_SCOPE_PASS_NATIVE_BLOCKED" if not native else "NATIVE_SYNTHETIC_PASS_RELEASE_BLOCKED"}
    output=json.dumps(report,indent=2,sort_keys=True)+"\n"
    print(output,end="")
    if args.json_out:args.json_out.write_text(output,encoding="utf-8")
    return 0

if __name__=="__main__":raise SystemExit(main())
