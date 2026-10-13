#!/usr/bin/env python3
"""Compare ACTUALLY EXECUTED pinned PKHeX.Core PK1 with ACTUAL NX PK1.

Not a PKHeX source model, not Gen I Red/Blue/Yellow retail encounter proof.
The built-in self-test verifies data integrity only and MUST NOT count as parity.
"""
from __future__ import annotations
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import sys

SCHEMA="L5_GEN1_LIVE_RUNTIME_RESULT_V1"
PIN="6501f0ab46e8f8ca048539dbaf8cae8cb104e722"
DATA="L5_GEN1_RBY_NATIVE_VECTORS_V1"

def check(cond:bool,code:str)->None:
    if not cond:raise ValueError(code)

def load_vectors(path:Path)->list[dict]:
    data=json.loads(path.read_text(encoding="utf8"))
    check(data.get("schema")==DATA,"VECTOR_SCHEMA")
    rows=data.get("cases",[])
    check(len(rows)==72,"VECTOR_72_EXPECTED")
    ids=set()
    for row in rows:
        ident=row.get("id")
        check(isinstance(ident,str) and ident not in ids,"DUPLICATE_VECTOR")
        ids.add(ident)
        check(row["species"] in (1,81) and row["move"] in (33,200),"VECTOR_CONTENT")
        check(row["dv16"] in (0,0x5555,0xffff),"VECTOR_DVS")
        check(row.get("stat_experience") in (0,32768,65535),"GEN1_STAT_EXP_DOMAIN")
        check(row["stat_experience"]=={0:0,0x5555:32768,0xffff:65535}[row["dv16"]],"STAT_EXP_PATTERN_DRIFT")
        check(row["experience"]==(135 if row["species"]==1 else 125),"NATIVE_LEVEL5_EXP_DRIFT")
        ordered=[0,1,32768,65534,65535]
        shift={0:0,0x5555:2,0xffff:4}[row["dv16"]]
        for i,k in enumerate(("stat_exp_hp","stat_exp_attack","stat_exp_defense",
                              "stat_exp_speed","stat_exp_special")):
            check(row.get(k)==ordered[(shift+i)%5],"DISTINCT_NATIVE_STAT_EXP_PATTERN:"+k)
        check(row["stat_exp_hp"]==row["stat_experience"],"HP_STAT_EXP_ALIAS")
        check(row["source"] in ("","red_gb","blue_gb","yellow_gb",
                                 "platinum_nds","unknown_source"),"VECTOR_SOURCE")
    return rows

def read_jsonl(path:Path,engine:str)->dict[str,dict]:
    result={}
    text=path.read_text(encoding="utf8").strip()
    for line in text.splitlines():
        row=json.loads(line)
        check(row.get("schema")==SCHEMA,"OBS_SCHEMA")
        check(row.get("engine")==engine,"OBS_ENGINE")
        check(row.get("execution_mode")==("actual_original_pkhex_core"
              if engine=="PKHEX_CORE" else "actual_production_pk1"),"OBS_NOT_LIVE")
        check(row.get("original_retail_history_authenticated") is False,
              "ORIGINAL_FABRICATION")
        if engine=="PKHEX_CORE":
            check(row.get("pkhex_revision")==PIN,"ORACLE_REVISION")
            check(row.get("original_source")=="RBY_VERSION_UNKNOWN",
                  "PKHEX_MISCLAIMED_ORIGINAL_VERSION")
        ident=row.get("id")
        check(isinstance(ident,str) and ident not in result,"DUPLICATE_OBSERVATION")
        result[ident]=row
    check(len(result)==72,"OBSERVATIONS_INCOMPLETE")
    return result

def dv_values(word:int)->dict:
    atk=(word>>12)&15;defense=(word>>8)&15;speed=(word>>4)&15;special=word&15
    hp=((atk&1)<<3)|((defense&1)<<2)|((speed&1)<<1)|(special&1)
    return dict(dv_hp=hp,dv_attack=atk,dv_defense=defense,
                dv_speed=speed,dv_special=special)

def evaluate(cases:list[dict],oracle:dict[str,dict],accepted:dict[str,dict],
             coordinator:dict[str,dict])->dict:
    keys={r["id"] for r in cases}
    check(keys==set(oracle)==set(accepted)==set(coordinator),"ID_OR_VECTOR_MISMATCH")
    mismatches=[];accepted_old=Counter()
    for v in cases:
        ident=v["id"]
        p,a,c=oracle[ident],accepted[ident],coordinator[ident]
        check(p.get("parsed") is True,"ORIGINAL_PKHEX_ANALYSIS_NOT_COMPLETED:"+ident)
        check(p.get("pk1_ability")==-1,"PKHEX_GEN1_ABSENT_ABILITY_CONTRACT:"+ident)
        check(p.get("pk1_gender") in (0,1,2),"PKHEX_GB_DERIVED_GENDER_DOMAIN:"+ident)
        for record in (p,a,c):
            for field in ("species","move","dv16","source"):
                check(record.get(field)==v[field],"INPUT_DRIFT:"+ident+":"+field)
            for field,expected in dv_values(v["dv16"]).items():
                check(record.get(field)==expected,"DV_ORIGINAL_DISAGREEMENT:"+ident+":"+field)
            for field in ("stat_exp_hp","stat_exp_attack","stat_exp_defense",
                          "stat_exp_speed","stat_exp_special"):
                check(record.get(field)==v[field],
                      "GEN1_16BIT_STAT_EXP_ORIGINAL_DISAGREEMENT:"+ident+":"+field)
            check(type(record.get("ability_invalid")) is bool,"ABILITY_BOOL")
            check(type(record.get("gender_invalid")) is bool,"GENDER_BOOL")
        # PKHeX PK1 has convenience-derived Gender and Ability=-1, whereas
        # NX's read-only PK1 has no native gender and neutral ability=0.
        # Compare emitted verifier defects and REAL DVs, never numeric
        # convenience values as if Gen I stored these modern fields.
        for native in (a,c):
            check(native.get("modern_ev_invalid") is False,
                  "GEN1_STAT_EXP_MISREAD_AS_MODERN_EV_INVALID:"+ident)
            check(native.get("move_domain_invalid")== (v["move"]==200),
                  "GEN1_NATIVE_MOVE_DOMAIN_FALSE_INVALID_OR_FALSE_NEGATIVE:"+ident)
        for field in ("ability_invalid","gender_invalid"):
            if a[field]!=p[field]:
                accepted_old[field]+=1
                mismatches.append({"id":ident,"context":v["source"],
                    "field":field,"oracle":p[field],
                    "accepted_nx":a[field],"coordinator_nx":c[field]})
            check(c[field]==p[field],"COORDINATOR_VS_LIVE_PKHEX:"+ident+":"+field)
        check(not p["ability_invalid"] and not p["gender_invalid"],
              "ACTUAL_PKHEX_GEN1_NON_NATIVE_REJECTION")
        if v["source"]=="":
            check(a["ability_invalid"],"F3_NOT_REPRODUCED")
            check(a["gender_invalid"]==(v["species"]==1),"F4_NOT_REPRODUCED")
        check(not c["ability_invalid"] and not c["gender_invalid"],
              "COORDINATOR_GEN1_FORMAT_GUARD")
        if v["source"]=="platinum_nds":
            check(a.get("source_conflict_warning") is True and
                  c.get("source_conflict_warning") is True,
                  "UNKNOWN_OR_CONFLICT_SOURCE_WAS_LOST")
        if v["source"] in ("red_gb","blue_gb","yellow_gb","unknown_source"):
            # No claim that source label alone proves the original encounter.
            check(v["expected_source_context"]!="Authenticated",
                  "ORIGINAL_SOURCE_CERTIFICATE_FABRICATED")
    check(accepted_old["ability_invalid"]>=12,"NO_ABILITY_BASELINE_DIFF")
    check(accepted_old["gender_invalid"]>=6,"NO_GENDER_BASELINE_DIFF")
    return {
      "schema":"L5_GEN1_REAL_PKHEX_NX_DIFF_V1",
      "evidence":"DIRECT_PKHEX_CORE_AND_TWO_REAL_NATIVE_NX_EXECUTABLES",
      "pkhex_commit":PIN,"vectors":len(cases),
      "original_game_version_specific_parity":"NOT_TESTED_PK1_HAS_NO_NATIVE_RBY_ID",
      "retail_original_witnesses":0,"full_gen1_legal_parity_certified":False,
      "accepted_baseline_non_native_false_invalid_counts":dict(accepted_old),
      "coordinator_non_native_defect_disagreements":0,
      "gen1_dv_field_differential_disagreements":0,
      "gen1_stat_experience_differential_disagreements":0,
      "stat_experience_native_boundaries":[0,32768,65535],
      "pkhex_global_legal_comparison":"NOT_CLAIMED_SYNTHETIC_CONTEXT_INCOMPLETE",
      "accepted_baseline_mismatch_samples":mismatches,
      "remaining_scope":"encounter/moves/gifts/trades/version-specific original histories",
      "release_gate":"BLOCKED",
    }

def selftest()->None:
    v={"id":"x","species":1,"level":5,"move":33,"dv16":0xffff,
       "experience":135,"stat_experience":65535,"stat_exp_hp":65535,
       "stat_exp_attack":0,"stat_exp_defense":1,"stat_exp_speed":32768,
       "stat_exp_special":65534,
       "source":"","expected_source_context":"Unknown"}
    check(dv_values(0)==dict(dv_hp=0,dv_attack=0,dv_defense=0,
                             dv_speed=0,dv_special=0),"DV_ZERO")
    check(dv_values(65535)==dict(dv_hp=15,dv_attack=15,dv_defense=15,
                                 dv_speed=15,dv_special=15),"DV_MAX")
    check(dv_values(0x5555)==dict(dv_hp=15,dv_attack=5,dv_defense=5,
                                dv_speed=5,dv_special=5),"DV_PATTERN")
    p={"schema":SCHEMA,"engine":"PKHEX_CORE","execution_mode":"actual_original_pkhex_core",
       "pkhex_revision":PIN,"original_source":"RBY_VERSION_UNKNOWN",
       "original_retail_history_authenticated":False,"id":"x","source":"",
       "species":1,"move":33,"dv16":65535,"ability_invalid":False,
       "gender_invalid":False,"parsed":True,"pk1_ability":-1,"pk1_gender":0,
       **{name:v[name] for name in ("stat_exp_hp","stat_exp_attack","stat_exp_defense",
                                   "stat_exp_speed","stat_exp_special")},**dv_values(65535)}
    a={**p,"engine":"POKEBANK_NX_ANALYZER","execution_mode":"actual_production_pk1",
       "ability_invalid":True,"gender_invalid":True,"move_domain_invalid":False,
       "modern_ev_invalid":False}
    c={**a,"ability_invalid":False,"gender_invalid":False}
    # The small partial input fails closed before claiming runtime parity.
    try:evaluate([v],{"x":p},{"x":a},{"x":c})
    except ValueError as ex:check(str(ex)=="NO_ABILITY_BASELINE_DIFF","SELFTEST_WRONG_BLOCKER")
    else:raise AssertionError("Partial runtime sample accidentally certified")
    # A single source-only canned response is explicitly rejected by protocol.
    check(p["execution_mode"]!="source_modeled","SELFTEST_CANNED")
    print("L5 Gen I true-oracle differential integrity self-test PASS (no PKHeX binary executed)")

def main()->int:
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--self-test",action="store_true")
    ap.add_argument("--vectors",type=Path)
    ap.add_argument("--pkhex-output",type=Path)
    ap.add_argument("--accepted-output",type=Path)
    ap.add_argument("--coordinator-output",type=Path)
    ap.add_argument("--json-out",type=Path)
    args=ap.parse_args()
    if args.self_test:selftest();return 0
    check(all((args.vectors,args.pkhex_output,args.accepted_output,
               args.coordinator_output)),"REQUIRES_ALL_THREE_EXECUTED_ENGINES")
    vectors=load_vectors(args.vectors)
    result=evaluate(vectors,read_jsonl(args.pkhex_output,"PKHEX_CORE"),
                    read_jsonl(args.accepted_output,"POKEBANK_NX_ANALYZER"),
                    read_jsonl(args.coordinator_output,"POKEBANK_NX_ANALYZER"))
    payload=json.dumps(result,sort_keys=True,indent=2)+"\n"
    print(payload,end="")
    if args.json_out:args.json_out.write_text(payload,encoding="utf8")
    return 0
if __name__=="__main__":sys.exit(main())
