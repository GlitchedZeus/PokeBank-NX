#!/usr/bin/env python3
"""L5 Stage 5 independent four-lane candidate-history hardening gate.

Without --full, ONLY L5's isolated audit ledger is compiled and executed.
With --full, exact Git blobs from four separately supplied sibling checkouts
are copied to a temporary overlay; actual L1/L2/L3/L4 original headers are
compiled under GCC and Clang, normal and ASan/UBSan. Never edits sibling lanes.
An independent candidate-scoped QA match is NOT a global Invalid verdict.
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
LEDGER="tools/audit_legality_l5_stage5_candidate_ledger.h"
UNIT="tests/fixtures/legality_l5_stage5_joint_ledger.cpp"
BRIDGE="tests/fixtures/legality_l5_stage5_live_four_lane.cpp"
L5_PINS={
    LEDGER:"580a3690554bf55761ccca5d4548da75c9980b34",
    UNIT:"e12aca2d911e86036a62a1f54e9e00ba48448c20",
    BRIDGE:"c10fcee44dbe78e6d6f81fd054c3b2344c459eec",
}
LANE_REFS={
    "L1":"f7138ad95ed1e0cccab283d51e5ba87502bb3f79",
    "L2":"110372b2a877c41881eb5466392684d378b4a90d",
    "L3":"2bf58df8033e045e0d4483a3900617ff61b350ea",
    "L4":"28fa55a9e56b9de7a40137b73199dd9051c53552",
}
SOURCES={
 "L1":{"Gen4HgssOriginalTreeStage.h":"9c767ede360d503baa5831db3ce9c6eff04c9441"},
 "L2":{"Gen4EggIVHistoricalCandidateSet.h":"47d5a3d5f04c5bb9d2246ec6e2ec694c0fb409cc",
       "Gen4EggIVInheritanceStage.h":"11e72b655d6963b5f31f07017122e28a5a7a4996"},
 "L3":{"Gen3GbaJointMethodHistoryCore.h":"c3368b800d9ae34125ebc65278d829c26219bf66"},
 "L4":{"Gen4EventClockReceiptEvidence.h":"69506bbc1b75355991a591415c64312311949e00"},
}
LEDGER_ATTEST=re.compile(
    r"^L5 Stage5 candidate evidence ledger PASS (900024) assertions "
    r"\(not global legality\)\n?$")
BRIDGE_ATTEST=re.compile(
    r"^L5 Stage5 four ORIGINAL lane helper integration PASS ([0-9]+) "
    r"assertions \(candidate-scoped only\)\n?$")
def blob(file:Path)->str:
    contents=file.read_bytes()
    return hashlib.sha1(b"blob "+str(len(contents)).encode("ascii")+
                        b"\0"+contents).hexdigest()
def pin(file:Path,expected:str)->None:
    if not file.is_file():
        raise RuntimeError(f"required source missing: {file}")
    got=blob(file)
    if got!=expected:
        raise RuntimeError(f"source changed: {file}: {got} != {expected}")
def source_overlay(roots:dict[str,Path],where:Path)->dict:
    saved={}
    destination=where/"Legality"
    destination.mkdir(parents=True)
    for lane,files in SOURCES.items():
        if lane not in roots:
            raise RuntimeError("all four exact sibling checkouts required")
        for name,expected in files.items():
            source=roots[lane]/"include"/"Legality"/name
            pin(source,expected)
            target=destination/name
            target.write_bytes(source.read_bytes())
            pin(target,expected)
            saved[f"{lane}/{name}"]=expected
    return saved
def command(argv:list[str],timeout:int,env:dict|None=None)->str:
    try:
        p=subprocess.run(argv,cwd=ROOT,env=env,text=True,
                         capture_output=True,timeout=timeout,check=False)
    except subprocess.TimeoutExpired as error:
        raise RuntimeError(f"timed out: {argv}") from error
    if p.returncode:
        raise RuntimeError(f"command failed ({p.returncode}): {argv}\n{p.stdout}\n{p.stderr}")
    return p.stdout
def checked(text:str,full:bool)->int:
    match=(BRIDGE_ATTEST if full else LEDGER_ATTEST).fullmatch(text)
    if match is None:
        raise RuntimeError("incorrect or missing native attestation: "+repr(text[-200:]))
    count=int(match.group(1))
    if full and count<200000:
        raise RuntimeError(f"cross-lane source regression unexpectedly undercovered: {count}")
    return count
def run_fixture(source:str,overlay:Path,full:bool,timeout:int)->list[dict]:
    rows=[]
    for compiler in ("g++","clang++"):
        cc=shutil.which(compiler)
        if not cc:raise RuntimeError(f"required C++ compiler missing: {compiler}")
        for sanitizer in (False,True):
            options=(["-O1","-g","-fsanitize=address,undefined","-fno-omit-frame-pointer"]
                     if sanitizer else ["-O2"])
            binary=overlay.parent/(
                compiler.replace("+","p")+("-san" if sanitizer else "-normal")+
                ("-four" if full else "-ledger"))
            command([cc,"-std=c++20","-Wall","-Wextra","-Wpedantic","-Werror",
                     *options,"-I",str(overlay),
                     str(ROOT/source),"-o",str(binary)],timeout)
            env=os.environ.copy()
            env["ASAN_OPTIONS"]="detect_leaks=0:halt_on_error=1"
            env["UBSAN_OPTIONS"]="halt_on_error=1:print_stacktrace=1"
            result=command([str(binary)],timeout,env)
            rows.append({"compiler":compiler,"sanitized":sanitizer,
                         "executed":True,"assertions":checked(result,full),
                         "status":"PASS","evidence":result.strip()})
    if len({row["assertions"] for row in rows})!=1:
        raise RuntimeError("compiler/sanitizer assertion count drift")
    return rows
def selftest()->None:
    normal="L5 Stage5 candidate evidence ledger PASS 900024 assertions (not global legality)\n"
    assert checked(normal,False)==900024
    cross="L5 Stage5 four ORIGINAL lane helper integration PASS 300000 assertions (candidate-scoped only)\n"
    assert checked(cross,True)==300000
    failures=(normal.replace("900024","900023"),
              normal.replace("PASS","FAIL"),normal+"FORGED",
              cross.replace("300000","199999"),
              cross.replace("candidate-scoped","globally Invalid"),
              cross+"TRAILING")
    for bad in failures:
        try:checked(bad,bad.startswith("L5 Stage5 four"))
        except RuntimeError:pass
        else:raise AssertionError("fake evidence accepted")
    print("L5 Stage5 source-attestation self-test PASS (8 positive/negative cases)")
def main()->None:
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test",action="store_true")
    parser.add_argument("--full",action="store_true")
    for lane in SOURCES:parser.add_argument(f"--{lane.lower()}-root",type=Path)
    parser.add_argument("--timeout",type=int,default=600)
    parser.add_argument("--output",type=Path)
    args=parser.parse_args()
    if args.self_test:selftest();return
    if args.timeout<1:parser.error("timeout must be positive")
    for name,expected in L5_PINS.items():pin(ROOT/name,expected)
    roots={lane:getattr(args,lane.lower()+"_root") for lane in SOURCES}
    if args.full and not all(roots.values()):
        raise RuntimeError("real four-lane source compilation requested but checkouts missing: BLOCKED")
    results=[]; pinned={}
    with tempfile.TemporaryDirectory(prefix="l5-stage5-crosslane-") as temp:
        work=Path(temp);overlay=work/"overlay"
        (overlay/"tools").mkdir(parents=True)
        (overlay/"tools"/Path(LEDGER).name).write_bytes((ROOT/LEDGER).read_bytes())
        pin(overlay/LEDGER,L5_PINS[LEDGER])
        if args.full:
            pinned=source_overlay(roots,overlay)
        results=run_fixture(BRIDGE if args.full else UNIT,overlay,args.full,args.timeout)
    report={
      "schema":"POKEBANK_NX_L5_STAGE5_CROSSLANE_V1",
      "status":"SOURCE_PINNED_CANDIDATE_TEST_PASS_NO_RELEASE_CLAIM" if args.full
               else "LEDGER_PASS_ACTUAL_LANE_HEADERS_BLOCKED",
      "pinned_sibling_refs":LANE_REFS,
      "source_blobs_verified":pinned,"l5_source_blobs_verified":L5_PINS,
      "actual_four_sibling_headers_executed":args.full,
      "independent_audit_ledger_executed":not args.full,
      "normal_and_asan_ubsan_both_compilers":True,
      "production_legality_analyzer_executed":False,
      "authenticated_original_retail_receipts":0,
      "globally_invalid_conclusions":0,
      "full_pkhex_parity_cells_certified":0,
      "release_approved":False,"runs":results,
    }
    result=json.dumps(report,sort_keys=True,indent=2)+"\n"
    print(result,end="")
    if args.output:args.output.write_text(result,encoding="utf8")
if __name__=="__main__":main()
