#!/usr/bin/env python3
"""Exact native Product PK5 vs unmodified PKHeX.Core field-level differential."""
import argparse
import hashlib
import json
from pathlib import Path
def run(vectors:Path,oracle:Path):
    expected={}
    for row in vectors.read_text().splitlines():
        name,hex_bytes,status,species,nature,ev,pid=row.split('\t')
        if name in expected: raise ValueError('duplicate fixture')
        expected[name]=(hex_bytes,status,int(species),int(nature),int(ev),int(pid))
    seen=set()
    cohorts={}
    for line in oracle.read_text().splitlines():
        x=json.loads(line);name=x['id']
        if name not in expected or name in seen: raise ValueError('unexpected oracle row')
        seen.add(name)
        h,s,sp,na,ev,pid=expected[name]
        assert x['rawSha256']==hashlib.sha256(bytes.fromhex(h)).hexdigest()
        assert x['nativeStatus']==s
        assert x['synthetic'] and not x['sourceHistoryAuthenticated']
        cohorts[s]=cohorts.get(s,0)+1
        if s=='Parsed':
            assert x['checksumValid'] and x['sanity']==0
            assert (x['species'],x['nature'],x['evTotal'],x['pid'])==(sp,na,ev,pid)
            assert x['originalPKHeX'] is not None
        elif s=='InvalidSanity': assert x['sanity']!=0
        elif s=='InvalidChecksum': assert not x['checksumValid']
        elif s=='InvalidSpecies': assert x['checksumValid'] and x['species']>649
        elif s=='InvalidNature': assert x['checksumValid'] and x['nature']>24
        elif s=='InvalidEVTotal': assert x['checksumValid'] and x['evTotal']>510
        elif s=='EmptySlot': assert x['species']==0
        else: raise ValueError(s)
    wanted={'Parsed':48,'InvalidSanity':48,'InvalidChecksum':48,
            'InvalidSpecies':48,'InvalidNature':48,'InvalidEVTotal':48,'EmptySlot':2}
    if seen!=set(expected) or cohorts!=wanted: raise ValueError('missing cohort')
    print(json.dumps({'differential':'PASS','syntheticVectors':len(seen),
                      'authenticatedRetailParity':False},sort_keys=True))
if __name__=='__main__':
    a=argparse.ArgumentParser()
    a.add_argument('--vectors',required=True,type=Path)
    a.add_argument('--oracle',required=True,type=Path)
    b=a.parse_args()
    run(b.vectors,b.oracle)
