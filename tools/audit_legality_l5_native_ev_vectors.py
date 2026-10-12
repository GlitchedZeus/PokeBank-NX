#!/usr/bin/env python3
"""L5 Q09 independent synthetic encrypted PK3/PK4 EV fixture verifier.

Only authored synthetic native-format vectors are used. No real retail Pokémon,
ROM, event cards, original save writes or user data.
Use: python3 tools/audit_legality_l5_native_ev_vectors.py
      python3 tools/audit_legality_l5_native_ev_vectors.py --json-out /tmp/report.json
A pass proves native-format crypt/checksum/EV/egg decoding of test vectors, NOT
an executed Legality::analyze or authentic historical PKHeX parity.
"""
from __future__ import annotations
import argparse
from collections import Counter
import json
from pathlib import Path
import struct

ROOT=Path(__file__).resolve().parents[1]
PATH=ROOT/"tests/fixtures/legality_l5_native_ev_vectors.json"
PERM=(
 (0,1,2,3),(0,1,3,2),(0,2,1,3),(0,3,1,2),(0,2,3,1),(0,3,2,1),
 (1,0,2,3),(1,0,3,2),(2,0,1,3),(3,0,1,2),(2,0,3,1),(3,0,2,1),
 (1,2,0,3),(1,3,0,2),(2,1,0,3),(3,1,0,2),(2,3,0,1),(3,2,0,1),
 (1,2,3,0),(1,3,2,0),(2,1,3,0),(3,1,2,0),(2,3,1,0),(3,2,1,0),
)

def u16(raw: bytes, offset: int) -> int:
    return struct.unpack_from("<H",raw,offset)[0]

def u32(raw: bytes, offset: int) -> int:
    return struct.unpack_from("<I",raw,offset)[0]

def swap_word_xor(buf: bytearray, offset: int, word: int) -> None:
    struct.pack_into("<I",buf,offset,u32(buf,offset)^word)

def checksum(raw: bytes, start: int, end: int) -> int:
    return sum(u16(raw,o) for o in range(start,end,2)) & 0xFFFF

def decode_pk3(data: bytes) -> tuple[bytes,bool]:
    if len(data)!=80: raise ValueError("PK3 record must have exact 80-byte length")
    copy=bytearray(data)
    pid=u32(copy,0)
    seed=pid^u32(copy,4)
    for off in range(0x20,0x50,4):
        swap_word_xor(copy,off,seed)
    shuffled=bytes(copy)
    order=PERM[pid%24]
    for block in range(4):
        location=order[block]
        copy[0x20+12*block:0x20+12*(block+1)]=shuffled[
            0x20+12*location:0x20+12*(location+1)]
    return bytes(copy),checksum(copy,0x20,0x50)==u16(copy,0x1c)

def decode_pk4(data: bytes) -> tuple[bytes,bool]:
    if len(data)!=136: raise ValueError("PK4 stored record must have exact 136-byte length")
    copy=bytearray(data)
    seed=u16(copy,6)
    for off in range(8,136,2):
        seed=(seed*0x41C64E6D+0x6073)&0xFFFFFFFF
        struct.pack_into("<H",copy,off,u16(copy,off)^(seed>>16))
    order=PERM[(u32(copy,0)>>13)&31] if ((u32(copy,0)>>13)&31)<24 else PERM[((u32(copy,0)>>13)&31)-24]
    shuffled=bytes(copy)
    for block in range(4):
        location=order[block]
        copy[8+32*block:8+32*(block+1)]=shuffled[
            8+32*location:8+32*(location+1)]
    return bytes(copy),checksum(copy,8,136)==u16(copy,6) and u16(copy,4)==0

def inspect(rec: dict, valid: bool) -> None:
    gen=rec["generation"]
    binary=bytes.fromhex(rec["encrypted_hex"])
    assert gen in (3,4)
    decoded,ok=(decode_pk3(binary) if gen==3 else decode_pk4(binary))
    assert ok==valid,(rec["id"],ok,valid)
    if not valid:
        return
    assert len(binary)==rec["native_length"]
    pid=u32(decoded,0)
    assert pid==rec["pid"],rec["id"]
    index=pid%24 if gen==3 else (pid>>13)&31
    assert index==rec["shuffle_index"]
    ev=decoded[0x38 if gen==3 else 0x18]
    assert ev==rec["ev_hp"]
    total=sum(decoded[(0x38 if gen==3 else 0x18):(0x3e if gen==3 else 0x1e)])
    assert total==rec["ev_total"]
    egg=(u32(decoded,0x48 if gen==3 else 0x38)&0x40000000)!=0
    assert egg==rec["is_egg"]
    assert u16(decoded,0x1c if gen==3 else 6)==rec["stored_checksum"]
    assert rec["nx_rule_over252"]==(ev>252)
    assert rec["pinned_pkhex_gen34_hardcap_reject"] is False
    assert rec["pinned_pkhex_unhatched_egg_ev_invalid"]==(egg and ev!=0)
    assert rec["expected_whole_native_legality"]=="UNDETERMINED"
    assert u16(decoded,0x20 if gen==3 else 8)==1

def audit(path: Path=PATH) -> dict:
    obj=json.loads(path.read_text(encoding="utf-8"))
    assert obj["schema"]=="L5_Q09_SYNTHETIC_NATIVE_EV_FIXTURE_SET_V1"
    assert obj["provenance_grade"]=="synthetic_native_format_nonretail"
    assert obj["original_historical_save_count"]==0
    assert obj["original_paired_save_count"]==0
    assert len(PERM)==24 and len({tuple(row) for row in PERM})==24
    assert all(sorted(p)==[0,1,2,3] for p in PERM)
    good=obj["fixtures"]
    bad=obj["corrupt_controls"]
    assert len(good)==336 and len(bad)==12
    assert len({x["id"] for x in good+bad})==348
    for row in good: inspect(row,True)
    for row in bad: inspect(row,False)
    by_gen=Counter(x["generation"] for x in good)
    by_egg=Counter(x["is_egg"] for x in good)
    assert by_gen=={3:144,4:192},by_gen
    assert by_egg=={True:168,False:168},by_egg
    assert {x["shuffle_index"] for x in good if x["generation"]==3}==set(range(24))
    assert {x["shuffle_index"] for x in good if x["generation"]==4}==set(range(32))
    for gen in (3,4):
        assert {x["ev_hp"] for x in good if x["generation"]==gen}=={0,1,252,253,254,255}
        assert {x["is_egg"] for x in good if x["generation"]==gen}=={False,True}
    return {"schema":"L5_Q09_NATIVE_FORMAT_VERIFICATION_V1",
            "status":"PASS","valid_records":len(good),"corrupted_rejected":len(bad),
            "pk3_shuffle_coverage":24,"pk4_shuffle_coverage":32,
            "scope":"synthetic native-format integrity only",
            "original_retail_pokemon_authenticated":0,
            "real_legality_analyzer_executed":False}

def main() -> None:
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--fixture",type=Path,default=PATH)
    parser.add_argument("--json-out",type=Path)
    args=parser.parse_args()
    result=audit(args.fixture)
    summary=json.dumps(result,indent=2,sort_keys=True)+"\n"
    print(summary)
    if args.json_out:
        args.json_out.write_text(summary,encoding="utf-8")

if __name__=="__main__":
    main()
