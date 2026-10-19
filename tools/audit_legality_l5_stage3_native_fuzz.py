#!/usr/bin/env python3
"""L5 Stage 3: compile and run original PK3/PK4 codec mutation fuzzers.

A full checkout must match all five exact original source blobs.
--isolated-original-codecs is restricted to a separately audited local copy
of four exact production code/headers and a narrowly pinned shuffle-table shim.
An isolated PASS explicitly does NOT certify the full entity or analyzer graph.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parents[1]
BLOBS={
 "src/Encryption/Encryption3FRLG.cpp":"133efded16f335220238d465d618c61ed57b2a9f",
 "src/Encryption/Encryption4.cpp":"406935761713db95ae738398b65ba0a37d4cf97b",
 "include/Encryption/Encryption3FRLG.h":"fd5e17b5cab61d4b1e98a8d0b6c7ee08036c1d88",
 "include/Encryption/Encryption4.h":"e823d3fb2fdd2008047d79e15faea1c2a12f48a0",
 "include/Encryption/Encryption.h":"9eab685b76511d5ea08e0fffdff63529a5824fe6",
 "tests/fixtures/legality_l5_stage3_native_crypto_fuzz.cpp":"15a0e2f6d7b37f93a7359ea9687c15180ceedf97",
}
ISOLATED_SHUFFLE_BLOB="329aa4851a04ba480c9c91f03c83e7f7c8d6e5ae"
EXPECTED=re.compile(
 r"^L5 Stage3 actual PK3/PK4 codec fuzz: 255566 PASS; "
 r"specimens=14336 single-bit=16896 metadata=36864 lengths=301 "
 r"\(synthetic transport, not complete analyzer or retail proof\)\n?$")
SOURCES=["tests/fixtures/legality_l5_stage3_native_crypto_fuzz.cpp",
         "src/Encryption/Encryption3FRLG.cpp","src/Encryption/Encryption4.cpp"]

def gitblob(data:bytes)->str:
    return hashlib.sha1(b"blob "+str(len(data)).encode("ascii")+b"\0"+data).hexdigest()

def pin(isolated:bool)->dict[str,str]:
    seen={}
    for path,expected in BLOBS.items():
        file=ROOT/path
        if not file.is_file():raise RuntimeError("required original source missing: "+path)
        found=gitblob(file.read_bytes())
        if isolated and path=="include/Encryption/Encryption.h":
            expected=ISOLATED_SHUFFLE_BLOB
        if found!=expected:
            raise RuntimeError(f"exact source blob drift: {path}: {found} != {expected}")
        seen[path]=found
    return seen

def execute(command:list[str],timeout:int,env:dict[str,str]|None=None)->str:
    try:
        r=subprocess.run(command,cwd=ROOT,env=env,capture_output=True,
                         text=True,timeout=timeout,check=False)
    except subprocess.TimeoutExpired as e:
        raise RuntimeError(f"native build/test TIMEOUT: {command}") from e
    if r.returncode:
        raise RuntimeError(f"native build/test failed {r.returncode}: {command}\n{r.stdout}\n{r.stderr}")
    return r.stdout

def build_run(compiler:str,sanitize:bool,temp:Path,timeout:int)->dict:
    binary=temp/(compiler.replace("+","p")+("-asan-ubsan" if sanitize else "-o2"))
    flags=["-std=c++20","-Wall","-Wextra","-Wpedantic","-Werror",
           # Original PK3 cpp contains GCC-only diagnostic pragma; keep
           # -Werror for every other warning instead of editing original.
           "-Wno-unknown-warning-option","-Iinclude"]
    flags += (["-O1","-g","-fsanitize=address,undefined","-fno-omit-frame-pointer"]
              if sanitize else ["-O2"])
    compile_cmd=[compiler,*flags,*SOURCES,"-o",str(binary)]
    execute(compile_cmd,timeout)
    env=os.environ.copy()
    env.update(ASAN_OPTIONS="detect_leaks=0:halt_on_error=1",
               UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
    actual=execute([str(binary)],timeout,env)
    if EXPECTED.fullmatch(actual) is None:
        raise RuntimeError(f"mismatched original-codec test attestation: {actual!r}")
    return {"compiler":compiler,"sanitized":sanitize,"checks":255566,
            "test_status":"PASS","reported":actual.strip()}

def selftest()->None:
    sample=("L5 Stage3 actual PK3/PK4 codec fuzz: 255566 PASS; "
            "specimens=14336 single-bit=16896 metadata=36864 lengths=301 "
            "(synthetic transport, not complete analyzer or retail proof)\n")
    assert EXPECTED.fullmatch(sample)
    assert gitblob(b"abc")=="f2ba8f84ab5c1bce84a7b441cb1959cfc7093b7f"
    for bad in (sample.replace("255566","255565"),
                sample.replace("PASS","FAIL"),sample.replace("14336","14335"),
                sample.replace("16896","16895"),sample+"fabricated"):
        assert EXPECTED.fullmatch(bad) is None
    print("L5 Stage3 native runner: 7 adversarial integrity/attestation controls PASS")

def main()->int:
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--self-test",action="store_true")
    ap.add_argument("--isolated-original-codecs",action="store_true")
    ap.add_argument("--normal-only",action="store_true")
    ap.add_argument("--sanitize-only",action="store_true")
    ap.add_argument("--timeout",type=int,default=900)
    ap.add_argument("--output",type=Path)
    args=ap.parse_args()
    if args.self_test:selftest();return 0
    if args.timeout<=0 or (args.normal_only and args.sanitize_only):
        ap.error("invalid test mode or timeout")
    seen=pin(args.isolated_original_codecs)
    modes=(False,) if args.normal_only else (True,) if args.sanitize_only else (False,True)
    rows=[]
    with tempfile.TemporaryDirectory(prefix="pbnx-l5-stage3-") as tmp:
        for compiler in ("g++","clang++"):
            for sanitize in modes:
                rows.append(build_run(compiler,sanitize,Path(tmp),args.timeout))
    report={
      "schema":"POKEBANK_NX_L5_STAGE3_EXACT_CODEC_FUZZ_V1",
      "source_blobs":seen,"isolated_dependency_shim":args.isolated_original_codecs,
      "production_crypto_cpp_executed":True,
      "production_entity_classes_executed":False,
      "full_legality_analyzer_executed":False,
      "retail_original_receipts_authenticated":0,
      "full_checkout_acceptance":"BLOCKED" if args.isolated_original_codecs else "CRYPTO_ONLY",
      "runs":rows,
    }
    output=json.dumps(report,indent=2,sort_keys=True)+"\n"
    print(output,end="")
    if args.output:args.output.write_text(output,encoding="utf-8")
    return 0

if __name__=="__main__":raise SystemExit(main())
