#!/usr/bin/env python3
"""L5 Q12 source-pinned EV hard-cap differential, not native production execution.

Read-only from an NX checkout:
 python3 tools/audit_legality_l5_ev_differential.py --self-test
 python3 tools/audit_legality_l5_ev_differential.py --native --sanitize
Optional independently fetched PKHeX@6501f0a checkout:
 python3 tools/audit_legality_l5_ev_differential.py --pkhex-root /path/to/pinned/PKHeX
Fail closed if the source code drifts from the pinned blob digest. A source rule
differential is not a finding that a whole PK3/PK4 specimen is impossible/valid.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
DATA = ROOT / "docs/legality/research/l5/Q12_EV_HARDCAP_FALSE_INVALID_DIFFERENTIAL.json"
NATIVE = ROOT / "tests/fixtures/legality_l5_ev_cap_reference_diff.cpp"

def blob_sha(content: bytes) -> str:
    return hashlib.sha1(b"blob " + str(len(content)).encode("ascii") + b"\0" + content).hexdigest()

def read_pinned(root: Path, path: str, expected_sha: str, anchors: tuple[str, ...]) -> None:
    data = (root / path).read_bytes()
    actual = blob_sha(data)
    if actual != expected_sha:
        raise RuntimeError(f"Pinned source drift: {path}: {actual}, expected {expected_sha}")
    source = data.decode("utf-8")
    for anchor in anchors:
        if anchor not in source:
            raise RuntimeError(f"Missing source guard {anchor!r} in {path}")

def assert_research_manifest() -> dict:
    doc = json.loads(DATA.read_text(encoding="utf-8"))
    assert doc["catalog"] == ["Q12","Q14","Q18"]
    p = doc["source_pins"]
    read_pinned(ROOT, p["nx_path"], p["nx_git_blob_sha"], (
        "if (ev[i] > 252)", "if (evTotal > 510)",
        "add(r, Severity::Invalid, std::string(statName(i)) + \" EV over 252"))
    assert doc["source_semantics"]["pkhex_gen3_4_max_ev_per_stat"] == 255
    assert doc["source_semantics"]["pkhex_gen6_onward_hard_limit"] == 252
    assert len(doc["fixture_vectors"]) == 24
    return doc

def reference_check(root: Path, report: dict) -> dict:
    p=report["source_pins"]
    files=[
        (p["pkhex_verifier_path"],p["pkhex_verifier_blob_sha"],(
            "if (format >= 6 && IsAnyAboveHardLimit6(evs))",
            "else if (format < 5) // 3/4",
            "if (sum > EffortValues.Max510)")),
        (p["pkhex_limits_path"],p["pkhex_limits_blob_sha"],(
            "public const byte Max255 = 255;",
            "public const byte Max252 = 252;",
            "public const ushort Max510 = 510;")),
        (p["pkhex_g3_path"],p["pkhex_g3_blob_sha"],("MaxEV => EffortValues.Max255",)),
        (p["pkhex_g4_path"],p["pkhex_g4_blob_sha"],("MaxEV => EffortValues.Max255",)),
    ]
    for path,digest,anchors in files:
        read_pinned(root,path,digest,anchors)
    return {"verified_files":len(files),"result":"PINNED_BYTE_IDENTICAL"}

def exhaustive() -> dict:
    results={}
    for gen in (3,4):
        count=0
        for first in range(256):
            for second in range(256):
                assert first+second <= 510
                nx_reject = first>252 or second>252
                ref_gen3_4_hardcap_reject = False  # pinned verifier hardcaps only format>=6
                count += int(nx_reject and not ref_gen3_4_hardcap_reject)
        assert count==1527, (gen,count)
        results[str(gen)]={"tested_two_stat_combinations":65536,
                           "hardcap_disagreements":count}
    assert sum(x["hardcap_disagreements"] for x in results.values())==3054
    return results

def native_build(sanitize: bool) -> dict:
    compiler=shutil.which("g++")
    if not compiler:
        raise RuntimeError("g++ unavailable: C++ fixture NOT RUN")
    flags=(["-O1","-fsanitize=address,undefined","-fno-omit-frame-pointer"]
           if sanitize else ["-O2"])
    with tempfile.TemporaryDirectory(prefix="l5-ev-q12-") as work:
        exe=Path(work)/"ev"
        subprocess.run([compiler,"-std=c++20","-Werror","-Wall","-Wextra",
                        "-Wpedantic",*flags,str(NATIVE),"-o",str(exe)],
                       check=True,cwd=ROOT)
        result=subprocess.run([str(exe)],check=True,text=True,
                              capture_output=True,cwd=ROOT)
    if "131072 bounded cases, 3054" not in result.stdout:
        raise RuntimeError(f"Wrong witness count: {result.stdout!r}")
    return {"status":"PASS","sanitized":sanitize,"description":result.stdout.strip(),
            "not_original_PK3_PK4_analyzer_execution":True}

def main() -> int:
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test",action="store_true")
    parser.add_argument("--pkhex-root",type=Path)
    parser.add_argument("--native",action="store_true")
    parser.add_argument("--sanitize",action="store_true")
    parser.add_argument("--json-out",type=Path)
    args=parser.parse_args()
    document=assert_research_manifest()
    result={"schema":"L5_Q12_PINNED_EV_RULE_DIFFERENTIAL_RESULT_V1",
            "nx_source_checked":"PINNED_BYTE_IDENTICAL",
            "pkhex_source_checked":"NOT_PROVIDED_PINNED_REPO",
            "bounded_sweep":exhaustive(),"rules_are_not_whole_pokemon_certification":True}
    if args.pkhex_root:
        result["pkhex_source_checked"]=reference_check(args.pkhex_root,document)
    if args.native:
        result["standalone_normal"]=native_build(False)
    if args.sanitize:
        result["standalone_asan_ubsan"]=native_build(True)
    text=json.dumps(result,indent=2,sort_keys=True)+"\n"
    print(text)
    if args.json_out:
        args.json_out.write_text(text,encoding="utf-8")
    return 0
if __name__=="__main__":
    raise SystemExit(main())
