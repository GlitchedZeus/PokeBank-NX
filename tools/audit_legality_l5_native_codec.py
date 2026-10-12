#!/usr/bin/env python3
"""Compile and run the actual PokeBank NX production PK3/PK4 encryption code.

Compares every one of 348 independently authored, committed, synthetic PK3/PK4
vectors against the accepted C++ cryptography. No game saves are opened or
written and these are NOT original Pokémon or global legality verdicts.

From a FULL repo checkout:
  python3 tools/audit_legality_l5_native_codec.py
  python3 tools/audit_legality_l5_native_codec.py --sanitize
  python3 tools/audit_legality_l5_native_codec.py --sanitize --source-pin
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
VECTOR_PATH = ROOT/"tests/fixtures/legality_l5_native_ev_vectors.json"
CLI = ROOT/"tests/fixtures/legality_l5_native_codec_batch.cpp"
SOURCE_PATHS = {
    "PK3": "src/Encryption/Encryption3FRLG.cpp",
    "PK4": "src/Encryption/Encryption4.cpp",
    "shuffle": "include/Encryption/Encryption.h",
    "PK3wrapper": "include/Pokemon/Pokemon3FRLG.h",
    "PK4wrapper": "src/Pokemon/Pokemon4ReadOnly.cpp",
}

def gitblob(path: Path) -> str:
    data=path.read_bytes()
    return hashlib.sha1(b"blob "+str(len(data)).encode("ascii")+b"\0"+data).hexdigest()

def rows(obj: dict) -> str:
    lines=[]
    for item in obj["fixtures"]:
        lines.append(" ".join([
            item["id"],str(item["generation"]),"1",str(item["pid"]),
            str(item["shuffle_index"]),str(item["ev_hp"]),
            str(int(item["is_egg"])),str(item["stored_checksum"]),
            item["encrypted_hex"]]))
    for item in obj["corrupt_controls"]:
        lines.append(" ".join([
            item["id"],str(item["generation"]),"0",
            "0","0","0","0","0",item["encrypted_hex"]]))
    assert len(lines)==348 and len(set(x.split(" ",1)[0] for x in lines))==348
    return "\n".join(lines)+"\n"

def verify(sanitize: bool, source_pin: bool) -> dict:
    fixture=json.loads(VECTOR_PATH.read_text(encoding="utf-8"))
    assert fixture["provenance_grade"]=="synthetic_native_format_nonretail"
    assert len(fixture["fixtures"])==336
    assert len(fixture["corrupt_controls"])==12
    if source_pin:
        for key, rel in SOURCE_PATHS.items():
            expected=fixture["source_blobs"][key]
            actual=gitblob(ROOT/rel)
            if actual!=expected:
                raise RuntimeError(f"Source drift {rel}: expected {expected}, got {actual}")
    with tempfile.TemporaryDirectory(prefix="l5-native-codec-") as tmp:
        exe=Path(tmp)/"native_codec"
        args=["g++","-std=c++20","-Wall","-Wextra","-Wpedantic","-Werror",
              "-Iinclude",
              str(CLI),
              str(ROOT/"src/Encryption/Encryption3FRLG.cpp"),
              str(ROOT/"src/Encryption/Encryption4.cpp")]
        args += (["-O1","-fsanitize=address,undefined","-fno-omit-frame-pointer"]
                 if sanitize else ["-O2"])
        args += ["-o",str(exe)]
        subprocess.run(args,check=True,cwd=ROOT)
        result=subprocess.run([str(exe)],input=rows(fixture),text=True,
                              check=True,capture_output=True,cwd=ROOT)
        # Negative input validation: strict size, hex, format and field ranges.
        cells=rows(fixture).splitlines()[0].split(" ")
        malformed=[]
        bad=cells.copy();bad[8]=bad[8][:-2];malformed.append(bad)
        bad=cells.copy();bad[8]=bad[8]+"00";malformed.append(bad)
        bad=cells.copy();bad[8]=bad[8][:-1]+"z";malformed.append(bad)
        bad=cells.copy();bad[1]="7";malformed.append(bad)
        bad=cells.copy();bad[5]="256";malformed.append(bad)
        bad=cells.copy();bad[3]="4294967296";malformed.append(bad)
        bad=cells.copy();bad[4]="24";malformed.append(bad)
        bad=cells.copy();bad[6]="2";malformed.append(bad)
        for i,bad in enumerate(malformed):
            rejected=subprocess.run([str(exe)],input=" ".join(bad)+"\n",
                                    text=True,capture_output=True,cwd=ROOT)
            if (rejected.returncode == 0 or
                "L5 native crypto failure on case 1:" not in rejected.stderr):
                raise RuntimeError(f"Malformed input {i} not specifically rejected: "
                                   f"{rejected.stderr!r}")
    if ("348 PASS (336 native byte roundtrips, 12 corrupted checksum rejects, " +
            "251904 single-bit data corruptions rejected)") not in result.stdout:
        raise RuntimeError("Compiled executable did not attest all 348 rows: "+
                           result.stdout+" STDERR "+result.stderr)
    return {"status":"PASS","input":"synthetic native PK3/PK4, no retail authenticity",
            "sanitized":sanitize,"exact_sources_checked":source_pin,
            "true_production_crypto_code_executed":True,
            "actual_legality_analyzer_executed":False,
            "total":348,"roundtrips":336,"expected_rejects":12,
            "single_bit_ciphertext_mutations_rejected":251904,
            "malformed_input_cases_rejected":8,
            "message":result.stdout.strip()}

def main() -> None:
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sanitize",action="store_true")
    parser.add_argument("--source-pin",action="store_true")
    parser.add_argument("--json-out",type=Path)
    opts=parser.parse_args()
    report=verify(opts.sanitize,opts.source_pin)
    output=json.dumps(report,sort_keys=True,indent=2)+"\n"
    print(output)
    if opts.json_out:
        opts.json_out.write_text(output,encoding="utf-8")

if __name__=="__main__":
    main()
